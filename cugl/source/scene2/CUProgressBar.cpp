//
//  CUProgressBar.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a simple progress bar, which is useful
//  for displaying things such as asset loading.
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
//  Version: 12/2/25 (SDL3 Integration)
//
#include <cugl/scene2/CUProgressBar.h>
#include <cugl/scene2/CUScene2Loader.h>
#include <cugl/core/util/CUStringTools.h>
#include <cugl/core/assets/CUAssetManager.h>

using namespace cugl;
using namespace cugl::scene2;
using namespace cugl::graphics;

#define UNKNOWN_VALUE "<unknown>"

#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized progress bar with no size or texture information.
 *
 * You must initialize this progress bar before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
 * heap, use one of the static constructors instead.
 */
ProgressBar::ProgressBar() :
_progress(1.0f),
_rendered(false) {
    _blend = BlendMode::PREMULT;
    _interior.first  = 0;
    _interior.second = 0;
    _classname = "ProgressBar";
}

/**
 * Initializes a progress bar with the texture and endcaps.
 *
 * The progress bar will be the size of the texture. The values left and
 * right refer to pixels in the texture. They represent the end of the
 * left endcap and the start of the right endcap, respectively.
 *
 * The progress bar will be placed at the origin of the parent and anchored
 * at the bottom left.
 *
 * @param texture   The progress bar texture
 * @param left      The end of the left endcap in pixels
 * @param right     The start of the right endcap in pixels
 *
 * @return true if the progress bar is initialized properly, false otherwise.
 */
bool ProgressBar::initWithCaps(const std::shared_ptr<graphics::Texture>& texture,
                               float left, float right) {
    if (_texture != nullptr) {
        CUAssertLog(false, "Progress bar is already initialized");
        return false;
    } else if (!SceneNode::init()) {
        return false;
    }
    
    _texture = texture;
    auto txt = (_texture == nullptr ? Texture::getBlank() : _texture);
    _interior.first  = left;
    _interior.second = right > left ? right : txt->getWidth();
    setContentSize(txt->getSize());
    return true;
}

/**
 * Initializes a progress bar with the given texture and size.
 *
 * The progress bar will have the given size, which includes the size of
 * the endcaps. The values left and right refer to pixels in the texture.
 * They represent the end of the left endcap and the start of the right
 * endcap, respectively.
 *
 * The progress bar will be placed at the origin of the parent and anchored
 * at the bottom left.
 *
 * @param texture   The progress bar texture
 * @param left      The end of the left endcap in pixels
 * @param right     The start of the right endcap in pixels
 * @param size      The progress bar size
 *
 * @return true if the progress bar is initialized properly, false otherwise.
 */
bool ProgressBar::initWithCaps(const std::shared_ptr<graphics::Texture>& texture,
                               float left, float right, const Size& size) {
    if (_texture != nullptr) {
        CUAssertLog(false, "Progress bar is already initialized");
        return false;
    } else if (!SceneNode::init()) {
        return false;
    }
    
    _texture = texture;
    auto txt = (_texture == nullptr ? Texture::getBlank() : _texture);
    _interior.first  = left;
    _interior.second = right > left ? right : txt->getWidth();
    setContentSize(size);
    return true;
}

/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link SceneLoader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attributes:
 *
 *      "texture":  The name of a previously loaded texture asset
 *      "interior": An two-element array of numbers (left,right)
 *      "blend":    A string matching a blend mode.
 *
 * All attributes are optional.  There are no required attributes.
 *
 * @param manager   The asset manager handling this asset
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool ProgressBar::initWithData(const AssetManager* manager,
                               const std::shared_ptr<JsonValue>& data) {
    if (_texture != nullptr) {
        CUAssertLog(false, "Progress bar is already initialized");
        return false;
    } else if (!data) {
        return init();
    } else if (!SceneNode::initWithData(manager, data)) {
        return false;
    }
    
    // Set the texture
    std::shared_ptr<Loader<Texture>> load = manager->access<Texture>();
    std::string key = data->getString("texture",UNKNOWN_VALUE);
    setTexture(manager->get<Texture>(key));
    auto texture = (_texture == nullptr ? Texture::getBlank() : _texture);
    
    float left, right;
    if (data->has("interior")) {
        JsonValue* interval = data->get("interior").get();
        CUAssertLog(interval->size() == 2, "'interior' must be a pair of numbers");
        left  = interval->get(0)->asFloat(0.0f);
        right = interval->get(1)->asFloat(texture->getWidth());
    } else {
        left  = 0;
        right = texture->getWidth();
    }
    setInterior(left,right);
    _blend = json_blend_mode(data->getString("blend","premultiplied"));

    if (!data->has("size")) {
        setContentSize(texture->getSize());
    }
    
    return true;
}


/**
 * Disposes all of the resources used by this node.
 *
 * A disposed progress bar can be safely reinitialized. Any children owned
 * by this node will be released.  They will be deleted if no other object
 * owns them.
 *
 * It is unsafe to call this on a progress bar that is still currently
 * inside of a scene graph.
 */
void ProgressBar::dispose() {
    _texture = nullptr;
    _blend = BlendMode::PREMULT;
    _interior.first  = 0;
    _interior.second = 0;
    SceneNode::dispose();
}

#pragma mark -
#pragma mark Attributes
/**
 * Sets the percentage progress of this progress bar
 *
 * This value is a float between 0 and 1. Changing this value will alter
 * the size of the progress bar foreground.
 *
 * @param progress  The percentage progress of this progress bar
 */
void ProgressBar::setProgress(float progress) {
    _progress = progress;
    clearRenderData();
}

/**
 * Sets the untransformed size of the node.
 *
 * The content size remains the same no matter how the node is scaled or
 * rotated. All nodes must have a size, though it may be degenerate (0,0).
 *
 * Changing the size of a rectangle will not change the position of the
 * node.  However, if the anchor is not the bottom-left corner, it will
 * change the origin. The Node will grow out from an anchor on an edge,
 * and equidistant from an anchor in the center.
 *
 * @param size  The untransformed size of the node.
 */
void ProgressBar::setContentSize(const Size& size) {
    SceneNode::setContentSize(size);
    clearRenderData();
}

/**
 * Sets the node texture to the one specified.
 *
 * This method will have no effect on the polygon vertices. This class
 * decouples the geometry from the texture. That is because we do not
 * expect the vertices to match the texture perfectly.
 *
 * @param texture   A shared pointer to a Texture object.
 */
void ProgressBar::setTexture(const std::shared_ptr<Texture>& texture) {
    float w = (texture == nullptr ? Texture::getBlank()->getWidth() : texture->getWidth());
    if (_texture != texture) {
        _texture = texture;
        if (_interior.second > w) {
            _interior.second = w;
        }
        if (_interior.first >= _interior.second) {
            _interior.first = 0;
        }
        clearRenderData();
    }
}

/**
 * Sets the interior internal defining the progress bar.
 *
 * The interior interval is specified in pixel coordinates. It is with
 * respect to x-coordinates, which have their origin on the left.
 *
 * The interior interval is similar to a NinePatch. It the interval is (3,6)
 * then the left endcap ends at x=3 and the right endcap starts at x=6. This
 * value is ignored if there is no texture.
 *
 * @param left  The left side of the interior
 * @param right The right side of the interior
 */
void ProgressBar::setInterior(float left, float right) {
    CUAssertLog(left < right, "(%g,%g) is not a valid interval",left,right);
    float w = (_texture == nullptr ? Texture::getBlank()->getWidth() : _texture->getWidth());
    _interior.second = (left > w) ? w : right;
    _interior.first  = (left >= _interior.second) ? 0 : left;
    clearRenderData();
}

/**
 * Returns a string representation of this node for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this node for debuggging purposes.
 */
std::string ProgressBar::toString(bool verbose) const {
    std::stringstream ss;
    if (verbose) {
        ss << "cugl::ProgressBar";
    }
    size_t texid = (_texture == nullptr ? 0 : _texture->id());
    ss << "(tag:";
    ss <<  cugl::strtool::to_string(_tag);
    ss << ", name:" << _name;
    ss << ", texture:";
    ss <<  cugl::strtool::to_string((Uint64)texid);
    ss << ")";
    if (verbose) {
        ss << "\n";
        for(auto it = _children.begin(); it != _children.end(); ++it) {
            ss << "  " << (*it)->toString(verbose);
        }
    }
    
    return ss.str();
}

#pragma mark -
#pragma mark Internal Helpers
/**
 * Clears the render data, releasing all vertices and indices.
 */
void ProgressBar::clearRenderData() {
    _mesh.clear();
    _rendered = false;
}

/**
 * Allocate the render data necessary to render this node.
 */
void ProgressBar::generateRenderData() {
    CUAssertLog(!_rendered, "Render data is already present");
    if (_texture == nullptr) {
        return;
    }
    
    auto texture = _texture == nullptr ? Texture::getBlank(): _texture;
    float w = texture->getWidth();
    float interior = _contentSize.width-_interior.first-(w-_interior.second);
    float i1 = _interior.first;
    float i2 = i1+_progress*interior;
    float i3 = i2+(w-_interior.second);

    float t1 = (texture->getMaxS()-texture->getMinS())*i1/w+texture->getMinS();
    float t2 = (texture->getMaxS()-texture->getMinS())*i2/w+texture->getMinS();
    float t3 = (texture->getMaxS()-texture->getMinS())*_interior.second/w+texture->getMinS();

    _mesh.command = DrawMode::TRIANGLES;

    // Three quads with four pairs of vertices
    SpriteVertex sprite;
    sprite.position  = Vec2::ZERO;
    sprite.gradcoord = Vec2::ZERO;
    sprite.texcoord.set(texture->getMinS(),texture->getMinT());
    sprite.color = _tintColor;
    _mesh.vertices.push_back(sprite);

    sprite.position.y = _contentSize.height;
    sprite.gradcoord.y = 1.0f;
    sprite.texcoord.y  = texture->getMaxT();
    _mesh.vertices.push_back(sprite);

    sprite.position.set(i1,0);
    sprite.gradcoord.set(_interior.first/w,0);
    sprite.texcoord.set(t1,texture->getMinT());
    _mesh.vertices.push_back(sprite);

    sprite.position.y = _contentSize.height;
    sprite.gradcoord.y = 1.0f;
    sprite.texcoord.y  = texture->getMaxT();
    _mesh.vertices.push_back(sprite);
    
    _mesh.indices.push_back(0);
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(3);

    sprite.position.set(i2,0);
    sprite.gradcoord.set(i2/w,0);
    sprite.texcoord.set(t2,texture->getMinT());
    _mesh.vertices.push_back(sprite);

    sprite.position.y = _contentSize.height;
    sprite.gradcoord.y = 1.0f;
    sprite.texcoord.y  = texture->getMaxT();
    _mesh.vertices.push_back(sprite);
    
    _mesh.indices.push_back(2);
    _mesh.indices.push_back(4);
    _mesh.indices.push_back(3);
    _mesh.indices.push_back(3);
    _mesh.indices.push_back(4);
    _mesh.indices.push_back(5);

    sprite.position.y = 0;
    sprite.gradcoord.y = 0.0f;
    sprite.texcoord.x  = t3;
    sprite.texcoord.y  = texture->getMinT();
    _mesh.vertices.push_back(sprite);

    sprite.position.y = _contentSize.height;
    sprite.gradcoord.y = 1.0f;
    sprite.texcoord.y  = texture->getMaxT();
    _mesh.vertices.push_back(sprite);
    
    sprite.position.set(i3,0);
    sprite.gradcoord.set(1.0f,0);
    sprite.texcoord.set(texture->getMaxS(),texture->getMinT());
    _mesh.vertices.push_back(sprite);

    sprite.position.y = _contentSize.height;
    sprite.gradcoord.y = 1.0f;
    sprite.texcoord.y  = texture->getMaxT();
    _mesh.vertices.push_back(sprite);

    _mesh.indices.push_back(6);
    _mesh.indices.push_back(8);
    _mesh.indices.push_back(7);
    _mesh.indices.push_back(7);
    _mesh.indices.push_back(8);
    _mesh.indices.push_back(9);
    
    _rendered = true;
}

/**
 * Draws this Node via the given SpriteBatch.
 *
 * This method provides the code for drawing a progress bar designed to fill
 * the current node content size. This method only worries about drawing the
 * current node. It does not attempt to render the children. Note that this
 * method is guaranteed to only be called when the node is visible.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param transform The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void ProgressBar::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    if (!_rendered) {
        generateRenderData();
    }
        
    batch->setColor(tint);
    batch->setTexture(_texture == nullptr ? Texture::getBlank() : _texture);
    batch->setBlendState(_blend);
    batch->drawMesh(_mesh, transform);
}
