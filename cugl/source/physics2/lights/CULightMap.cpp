//
//  CULightMap.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is part of a reimagining of Kalle Hameleinen's box2dLights
//  (version 1.5). Box2dLights is distributed under the Apache license. For the
//  original source code, refer to
//
//    https://github.com/libgdx/box2dlights
//
//  This code has been redesigned to solve architectural issues in that library,'
//  particularly with the way it couples geometry and rendering. This version is
//  compatible with our CUGL's rendering backend (either OpenGL or Vulkan).
//  Future plans are to offload the raycasting to a compute shader.
//
//  This specific module is a layer of composited lights. While it is typically
//  attached to a LightBatch, it has additional shaders (and framebuffers) for
//  the compositing pass. The map can either be rendered directly to the screen
//  or it can be drawn to a texture for later use in a SpriteBatch.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 1/4/26
//
#include <cugl/physics2/lights/CULightMap.h>
#include <cugl/core/CUDisplay.h>
#include <cugl/core/math/CUOrthographicCamera.h>
#include <cugl/graphics/CUVertexBuffer.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/physics2/CUObstacleWorld.h>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::physics2;
using namespace cugl::physics2::lights;

/** Gaussian blur fragment shader */
const std::string blurShaderFrag =
#include "shaders/Gaussian.frag"
;

/** Gaussian blur vertex shader */
const std::string blurShaderVert =
#include "shaders/Gaussian.vert"
;

/** Shadow composite fragment shader */
const std::string shadowShaderFrag =
#include "shaders/ShadowShader.frag"
;

/** Shadow composite vertex shader */
const std::string shadowShaderVert =
#include "shaders/ShadowShader.vert"
;

/**
 * The vertex type for the internal shaders
 *
 * This is vertex is used to process a full-screen quad.
 */
class MapVertex {
public:
    /** The vertex position */
    Vec2 position;
    /** The vertex texture coordinate */
    Vec2 texCoord;
};

#pragma mark -
#pragma mark LightMap
/**
 * Creates a degenerate light map with no buffers.
 *
 * You must initialize the map before using it.
 */
LightMap::LightMap() :
_world(nullptr),
_units(1),
_blurpass(1),
_gamma(false),
_premult(true),
_offscreen(false) {
    _ambient = Color4::BLACK;
}

/**
 * Deletes the internal shaders and resets all attributes.
 *
 * You must reinitialize the light map to use it.
 */
void LightMap::dispose() {
    if (_world != nullptr) {
        for(auto it = _lights.begin(); it != _lights.end(); ++it) {
            _world->removeObject(it->second);
        }
        auto self = shared_from_this();
        _world->removeObject(self);
        _world = nullptr;
    }
    _units = 1;
    _blurpass = 1;
    _bounds.size.set(0,0);
    _gamma = false;
    _premult = true;
    _offscreen = false;
    
    _batch = nullptr;
    _camera = nullptr;
    _framebuffer1 = nullptr;
    _framebuffer2 = nullptr;
    _gaussianShader = nullptr;
    _shadowShader = nullptr;
    _quadVertices = nullptr;
}

/**
 * Initializes a new light map using the given physics units
 *
 * The physics units are the number of pixels per box2d unit. This method
 * sets the bounds of the light map to be the size of the display divided
 * by the physics units. That way, the physics world takes up the entire
 * display.
 *
 * If this map uses offscreen rendering, then the framebuffer is guaranteed
 * to be the same size as the display.
 *
 * @param units The physics units
 *
 * @return true if initialization was successful.
 */
bool LightMap::init(float units) {
    Rect bounds = Display::get()->getViewBounds();
    bounds.origin.setZero();
    bounds.size /= units;
    return initWithBounds(bounds,units);
}

/**
 * Initializes a new light map with the given bounds.
 *
 * The physics units are the number of pixels per box2d unit. They are
 * computed by dividing the size of the display by the box2d bounds and
 * taking the smallest dimension.
 *
 * If this map uses offscreen rendering, then the framebuffer is guaranteed
 * to be the same size as the display.
 *
 * @param bounds    The bounds of the physics world
 *
 * @return true if initialization was successful.
 */
bool LightMap::initWithBounds(const Rect& bounds) {
    Rect screen = Display::get()->getViewBounds();
    Vec2 scale = screen.size/bounds.size;
    float units = std::min(scale.x,scale.y);
    return initWithBounds(bounds,units);
}

/**
 * Initializes a new light map with the given bounds and physics units.
 *
 * The physics units are the number of pixels per box2d unit. This map will
 * draw to a region the size of bounds times the physics units, which may
 * or may not be the entire display
 *
 * If this map uses offscreen rendering, then the framebuffer will also be
 * the size of bounds times the physics units.
 *
 * @param bounds    The bounds of the physics world
 * @param units     The physics units
 *
 * @return true if initialization was successful.
 */
bool LightMap::initWithBounds(const Rect& bounds, float units) {
    if (units <= 0) {
        CUAssertLog(false,"Physics units must be positive");
        return false;
    }
    
    _bounds = bounds;
    _camera = OrthographicCamera::allocOffset(_bounds);

    setPhysicsUnits(units);
    
    // Create the quad to draw
    _quadVertices = VertexBuffer::alloc(6,sizeof(MapVertex));
    std::vector<MapVertex> verts;
    verts.emplace_back();
    verts.back().position.set(-1,-1);
    verts.back().texCoord.set(0,0);
    verts.emplace_back();
    verts.back().position.set(-1,1);
    verts.back().texCoord.set(0,1);
    verts.emplace_back();
    verts.back().position.set(1,1);
    verts.back().texCoord.set(1,1);
    verts.emplace_back();
    verts.back().position.set(1,-1);
    verts.back().texCoord.set(1,0);
    _quadVertices->loadData(verts.data(), sizeof(MapVertex)*verts.size());

    // Create the light batch
    _batch = LightBatch::allocWithBounds(_bounds);
    
    // Create the post-process shaders
    // Time to compile the shader
    ShaderSource vsource;
    vsource.initOpenGL(blurShaderVert, ShaderStage::VERTEX);
    ShaderSource fsource;
    fsource.initOpenGL(blurShaderFrag, ShaderStage::FRAGMENT);
    _gaussianShader = GraphicsShader::alloc(vsource, fsource);

    AttributeDef adef;
    adef.type = GLSLType::VEC2;
    adef.group = 0;
    adef.location = 0;
    adef.offset = offsetof(MapVertex,position);
    _gaussianShader->declareAttribute("aPosition", adef);

    adef.location = 1;
    adef.offset = offsetof(MapVertex,texCoord);
    _gaussianShader->declareAttribute("aTexCoord", adef);
    
    UniformDef udef;
    udef.type = GLSLType::VEC2;
    udef.location = 0;
    udef.size = sizeof(Vec2);
    udef.stage = ShaderStage::FRAGMENT;
    _gaussianShader->declareUniform("uTexelSize", udef);

    udef.location = 1;
    udef.size = sizeof(int);
    _gaussianShader->declareUniform("uDirection", udef);

    ResourceDef rdef;
    rdef.type = ResourceType::TEXTURE;
    rdef.location = 0;
    rdef.stage = ShaderStage::FRAGMENT;
    rdef.arraysize = 1;
    rdef.blocksize = 0;
    rdef.set = 0;
    rdef.unbounded = true;
    _gaussianShader->declareResource("uTexture", rdef);

    vsource.initOpenGL(shadowShaderVert, ShaderStage::VERTEX);
    fsource.initOpenGL(shadowShaderFrag, ShaderStage::FRAGMENT);
    _shadowShader = GraphicsShader::alloc(vsource, fsource);
    
    adef.type = GLSLType::VEC2;
    adef.group = 0;
    adef.location = 0;
    adef.offset = offsetof(MapVertex,position);
    _shadowShader->declareAttribute("aPosition", adef);

    adef.location = 1;
    adef.offset = offsetof(MapVertex,texCoord);
    _shadowShader->declareAttribute("aTexCoord", adef);

    udef.type = GLSLType::UCOLOR;
    udef.location = 0;
    udef.size = sizeof(Color4);
    udef.stage = ShaderStage::FRAGMENT;
    _shadowShader->declareUniform("uAmbient", udef);

    udef.type = GLSLType::INT;
    udef.location = 1;
    udef.size = sizeof(int);
    _shadowShader->declareUniform("uType", udef);
    
    _shadowShader->declareResource("uTexture", rdef);
    
    return (_gaussianShader->compile() && _shadowShader->compile());
}

#pragma mark -
#pragma mark Attributes

/**
 * Sets the bounds of this light batch in physics units.
 *
 * This value and {@link #getPhysicsUnits} is used to determine the
 * {@link OrthogonalCamera} for this batch.
 *
 * @param bounds    The bounds of this light batch in physics units.
 */
void LightMap::setBounds(const Rect& bounds) {
    _bounds = bounds;
    _camera = OrthographicCamera::allocOffset(_bounds);
    _batch->setBounds(_bounds);
    // Recreate the framebuffer
    setPhysicsUnits(_units);
}

/**
 * Sets the physics units for this light.
 *
 * Physics units are the number of pixels per box2d unit. This value is
 * used to create the internal framebuffers.
 *
 * @param units The physics units for this light
 */
void LightMap::setPhysicsUnits(float units) {
    _units = units;
    
    Size size = _bounds.size*_units;
    _framebuffer1 = FrameBuffer::alloc(size.width, size.height);
    _framebuffer2 = FrameBuffer::alloc(size.width, size.height);
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        it->second->setPhysicsUnits(_units);
    }
}

/**
 * Adds a light to this map
 *
 * Adding a light will associate it with the {@link #getWorld} and assign
 * the physics units to match this map. However, a light will only be
 * drawn if it is active.
 *
 * @param light The light to add
 */
void LightMap::addLight(const std::shared_ptr<Light>& light) {
    if (light == nullptr) {
        return;
    }
    size_t addr = (size_t)(light.get());
    _lights[addr] = light;
    light->setPhysicsUnits(_units);
    if (_world) {
        _world->addObject(light);
    }
}

/**
 * Removes a light from this map
 *
 * Adding a light will disassociate it from the {@link #getWorld}.
 *
 * @param light The light to remove
 *
 * @return true if the light was successfully removed
 */
bool LightMap::removeLight(const std::shared_ptr<Light>& light) {
    if (light == nullptr) {
        return false;
    }
    
    size_t addr = (size_t)(light.get());
    auto it = _lights.find(addr);
    if (it == _lights.end()) {
        return false;
    }
    
    _lights.erase(it);
    if (_world) {
        _world->removeObject(light);
    }
    return true;
}

/**
 * Returns the list of lights associated with this map
 *
 * @return the list of lights associated with this map
 */
const std::vector<std::shared_ptr<Light>> LightMap::getLights() const {
    std::vector<std::shared_ptr<Light>>  result;
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

/**
 * Removes all lights from this map.
 */
void LightMap::clearLights() {
    if (_world) {
        for(auto it = _lights.begin(); it != _lights.end(); ++it) {
            _world->removeObject(it->second);
        }
    }
    _lights.clear();
}

#pragma mark -
#pragma mark Physics

/**
 * Detatches the given obstacle world from this map.
 *
 * This method sets the world for use in calls to {@link #update}.
 *
 * @param world The world to attach.
 */
void LightMap::attachWorld(const std::shared_ptr<ObstacleWorld>& obj) {
    if (_world) {
        detachWorld(_world.get());
    }
    
    _world = obj;
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        _world->addObject(it->second);
    }
    auto self = shared_from_this();
    _world->addObject(self);
}

/**
 * Detatches the given obstacle world from this object.
 *
 * This method ensures safe clean-up.
 *
 * @param world The world to detatch.
 */
void LightMap::detachWorld(const ObstacleWorld* obj) {
    _world = nullptr;
}

/**
 * Updates all of the active lights in the scene
 *
 * This method uses raycasting on the world provided by {@link #attachWorld}.
 * If there is no world provided, this method does nothing.
 */
void LightMap::update() {
    if (!_world) {
        return;
    }
    
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        if (it->second->isActive()) {
            it->second->update(_world->getWorld(), _bounds);
        }
    }
}

#pragma mark -
#pragma mark Rendering
/**
 * Returns the texture for the offscreen rendering buffer
 *
 * If {@link #isOffscreen} is false, this method will return nullptr.
 * Otherwise, it will return a texture containing the results of the
 * most recent call to {@link #render}
 */
const std::shared_ptr<graphics::Texture> LightMap::getTexture() const {
    if (_offscreen) {
        return _framebuffer2->getTexture();
    }
    return nullptr;
}

/**
 * Draws the lights in the scene.
 *
 * This method draws a shadow layer created by the most recent call to
 * {@link #update}. Areas not covered by lights will use {@link #getAmbient}
 * for their color.
 *
 * If {@link #isOffscreen} is false, this method will draw directly to the
 * screen. Otherwise, it will draw the results to an offscreen buffer that
 * can be accessed with {@link #getTexture}.
 */
void LightMap::render() {
    _framebuffer1->clear();
    _batch->setPremultiplied(_premult);
    _batch->setGamma(_gamma);
    _batch->begin(_framebuffer1);
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        if (it->second->isActive()) {
            _batch->draw(it->second);
        }
    }
    _batch->end();
    
    Size size = _framebuffer1->getSize();
    size.width = 1.0f/size.width;
    size.height = 1.0f/size.height;
    for(int ii = 0; ii < _blurpass; ii++) {
        _framebuffer2->clear();
        _gaussianShader->begin(_framebuffer2);
        _gaussianShader->push("uDirection", Vec2::UNIT_X);
        _gaussianShader->push("uTexelSize", size);
        _gaussianShader->setTexture("uTexture", _framebuffer1->getTexture());
        _gaussianShader->setVertices(0, _quadVertices);
        _gaussianShader->setDrawMode(DrawMode::TRIANGLE_FAN);
        _gaussianShader->draw(4);
        _gaussianShader->end();
        
        _framebuffer1->clear();
        _gaussianShader->begin(_framebuffer1);
        _gaussianShader->push("uDirection", Vec2::UNIT_Y);
        _gaussianShader->push("uTexelSize", size);
        _gaussianShader->setTexture("uTexture", _framebuffer2->getTexture());
        _gaussianShader->setVertices(0, _quadVertices);
        _gaussianShader->setDrawMode(DrawMode::TRIANGLE_FAN);
        _gaussianShader->draw(4);
        _gaussianShader->end();
    }
    
    if (_offscreen) {
        _framebuffer2->clear();
        _shadowShader->begin(_framebuffer2);
    } else {
        _shadowShader->begin();
    }
    
    if (_premult) {
        _shadowShader->setBlendMode(BlendMode::PREMULT);
    } else {
        _shadowShader->setBlendMode(BlendMode::ALPHA);
    }
    
    _shadowShader->pushColor4("uAmbient",_ambient);
    _shadowShader->pushInt("uType",_premult ? 3 : 2);
    _shadowShader->setTexture("uTexture", _framebuffer1->getTexture());
    _shadowShader->setVertices(0, _quadVertices);
    _shadowShader->setDrawMode(DrawMode::TRIANGLE_FAN);
    _shadowShader->draw(4);
    _shadowShader->end();
}

/**
 * Draws the lights within the given bounds.
 *
 * This method draws a shadow layer created by the most recent call to
 * {@link #update}. Areas not covered by lights will use {@link #getAmbient}
 * for their color.
 *
 * The bounds is used to restrict the portion of the display or framebuffer
 * that this method uses when drawing (e.g. it will apply a view scissor).
 * The bounds window is measured in box2d units, and it should fit within
 * the bounds of the light map. The origin of this window is the bottom
 * left.
 *
 * If {@link #isOffscreen} is false, this method will draw directly to the
 * screen. Otherwise, it will draw the results to an offscreen buffer that
 * can be accessed with {@link #getTexture}.
 *
 * @param x         The x-coordinate of the scissor origin
 * @param y         The x-coordinate of the scissor origin
 * @param width     The scissor width
 * @param height    The scissor height
 */
void LightMap::render(float x, float y, float width, float height) {
    _framebuffer1->clear();
    _batch->setPremultiplied(_premult);
    _batch->setGamma(_gamma);
    _batch->begin(_framebuffer1);
    for(auto it = _lights.begin(); it != _lights.end(); ++it) {
        if (it->second->isActive()) {
            _batch->draw(it->second);
        }
    }
    _batch->end();
    
    Size size = _framebuffer1->getSize();
    size.width = 1.0f/size.width;
    size.height = 1.0f/size.height;
    for(int ii = 0; ii < _blurpass; ii++) {
        _framebuffer2->clear();
        _gaussianShader->begin(_framebuffer2);
        _gaussianShader->push("uDirection", Vec2::UNIT_X);
        _gaussianShader->push("uTexelSize", size);
        _gaussianShader->setTexture("uTexture", _framebuffer1->getTexture());
        _gaussianShader->setVertices(0, _quadVertices);
        _gaussianShader->setDrawMode(DrawMode::TRIANGLE_FAN);
        _gaussianShader->draw(4);
        _gaussianShader->end();
        
        _framebuffer1->clear();
        _gaussianShader->begin(_framebuffer1);
        _gaussianShader->push("uDirection", Vec2::UNIT_Y);
        _gaussianShader->push("uTexelSize", size);
        _gaussianShader->setTexture("uTexture", _framebuffer2->getTexture());
        _gaussianShader->setVertices(0, _quadVertices);
        _gaussianShader->setDrawMode(DrawMode::TRIANGLE_FAN);
        _gaussianShader->draw(4);
        _gaussianShader->end();
    }
    
    Rect bounds;
    bounds.set(x,y,width,height);
    bounds.origin - _bounds.origin;
    if (_offscreen) {
        Affine2 affine;
        affine.scale(_units);
        bounds *= affine;
        _framebuffer2->clear();
        _shadowShader->begin(_framebuffer2);
    } else {
        auto fb = FrameBuffer::getDisplay();
        Size size = Display::get()->getViewBounds().size;
        size /= _bounds.size;

        Affine2 affine;
        affine.scale(std::min(size.width, size.height));
        bounds *= affine;
        _shadowShader->begin();
    }
    
    if (_premult) {
        _shadowShader->setBlendMode(BlendMode::PREMULT);
    } else {
        _shadowShader->setBlendMode(BlendMode::ALPHA);
    }
    _shadowShader->enableViewScissor(true);
    _shadowShader->setViewScissor(bounds.origin.x, bounds.origin.y,
                                  bounds.size.width, bounds.size.height);
    
    _shadowShader->pushColor4("uAmbient",_ambient);
    _shadowShader->pushInt("uType",_premult ? 3 : 2);
    _shadowShader->setTexture("uTexture", _framebuffer1->getTexture());
    _shadowShader->setVertices(0, _quadVertices);
    _shadowShader->setDrawMode(DrawMode::TRIANGLE_FAN);
    _shadowShader->draw(4);
    _shadowShader->end();    
}


    
