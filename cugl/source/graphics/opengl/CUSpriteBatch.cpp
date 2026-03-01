//
//  CUSpriteBatch.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides one-stop shopping for basic 2d graphics. Despite the
//  name, it is also capable of drawing solid shapes, as well as wireframes.
//  It also has support for color gradients and (rotational) scissor masks.
//
//  While it is possible to swap out the shader for this class, the shader is
//  very peculiar in how it uses uniforms. You should study the SpriteShader
//  GLSL files before making any shader changes to this class.
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
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/math/cu_math.h>
#include <cugl/graphics/CUSpriteBatch.h>
#include <cugl/graphics/CUFrameBuffer.h>
#include <cugl/graphics/CUVertexBuffer.h>
#include <cugl/graphics/CUIndexBuffer.h>
#include <cugl/graphics/CUUniformBuffer.h>
#include <cugl/graphics/CUTexture.h>
#include <cugl/graphics/CUGraphicsShader.h>
#include <cugl/graphics/CUFont.h>
#include <cugl/graphics/CUGlyphRun.h>
#include <cugl/graphics/CUGradient.h>
#include <cugl/graphics/CUScissor.h>
#include <cugl/graphics/CUTextLayout.h>

/**
 * Spritebatch fragment shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderFrag =
#include "shaders/SpriteShader.frag"
;

/**
 * Spritebatch vertex shader
 *
 * This trick uses C++11 raw string literals to put the shader in a separate
 * file without having to guarantee its presence in the asset directory.
 * However, to work properly, the #include statement below MUST be on its
 * own separate line.
 */
const std::string oglShaderVert =
#include "shaders/SpriteShader.vert"
;

using namespace cugl;
using namespace cugl::graphics;


#pragma mark PIPELINE FLAGS

/** The drawing type for a textured mesh */
#define TYPE_TEXTURE    1
/** The drawing type for a gradient mesh */
#define TYPE_GRADIENT   2
/** The drawing type for a scissored mesh */
#define TYPE_SCISSOR    4
/** The drawing type for a (simple) texture blur */
#define TYPE_GAUSSBLUR  8

/** The type uniform has changed */
#define DIRTY_DRAWTYPE          0x001
/** The drawing command has changed */
#define DIRTY_DRAWMODE          0x002
/** The blending state has changed */
#define DIRTY_BLEND             0x004
/** The stencil state has changed (includes color mask) */
#define DIRTY_STENCIL           0x008
/** The blur step has changed */
#define DIRTY_BLURSTEP          0x010
/** The perspective matrix has changed */
#define DIRTY_PERSPECTIVE       0x020
/** The texture has changed */
#define DIRTY_TEXTURE           0x040
/** The block offset has changed */
#define DIRTY_UNIBLOCK          0x080
/** All values have changed */
#define DIRTY_ALL_VALS          0x0FF

/**
 * Fills poly with a mesh defining the given rectangle.
 *
 * We need this method because we want to allow non-standard
 * polygons that represent a path, and not a triangulated
 * polygon.
 *
 * @param poly      The polygon to store the result
 * @param rect      The source rectangle
 * @param solid     Whether to triangulate the rectangle
 */
static void makeRect(Poly2& poly, const Rect& rect, bool solid) {
    poly.vertices.reserve(4);
    poly.vertices.push_back(rect.origin);
    poly.vertices.push_back(Vec2(rect.origin.x+rect.size.width, rect.origin.y));
    poly.vertices.push_back(rect.origin+rect.size);
    poly.vertices.push_back(Vec2(rect.origin.x, rect.origin.y+rect.size.height));
    if (solid) {
        poly.indices.reserve(6);
        poly.indices.push_back(0);
        poly.indices.push_back(1);
        poly.indices.push_back(2);
        poly.indices.push_back(0);
        poly.indices.push_back(2);
        poly.indices.push_back(3);
    } else {
        poly.indices.reserve(8);
        poly.indices.push_back(0);
        poly.indices.push_back(1);
        poly.indices.push_back(1);
        poly.indices.push_back(2);
        poly.indices.push_back(2);
        poly.indices.push_back(3);
        poly.indices.push_back(3);
        poly.indices.push_back(0);
    }
}

/** The default capacity of a sprite batch */
Uint32 SpriteBatch::DEFAULT_CAPACITY = 8192;


#pragma mark -
#pragma mark Context
/**
 * A class storing the drawing context for the associate shader.
 *
 * Because we want to minimize the number of times we load vertices
 * to the vertex buffer, all uniforms are recorded and delayed until the
 * final graphics call.  We include blending attributes as part of the
 * context, since they have similar performance characteristics to
 * other uniforms
 */
class SpriteBatch::Context {
public:
    /**
     * Creates a context of the default uniforms.
     */
    Context() {
        first = 0;
        last  = 0;
        draw  = DrawMode::TRIANGLES;
        blend = BlendMode::PREMULT;
        front = StencilMode::NONE;
        back  = StencilMode::NONE;
        stencil = true;
        colorMask = 0xf;
        perspective = std::make_shared<Mat4>();
        perspective->setIdentity();
        texture  = nullptr;
        blockptr = -1;
        blur  = 0;
        type  = 0;
        dirty = 0;
        pushed = false;
    }
    
    /**
     * Creates a copy of the given uniforms
     *
     * @param copy  The uniforms to copy
     */
    Context(Context* copy) {
        first = copy->first;
        last  = copy->last;
        type  = copy->type;
        draw  = copy->draw;
        blend = copy->blend;
        front = copy->front;
        back  = copy->back;
        stencil = copy->stencil;
        colorMask = copy->colorMask;
        perspective = copy->perspective;
        texture  = copy->texture;
        blockptr = copy->blockptr;
        blur  = copy->blur;
        pushed = false;
        dirty  = 0;
    }
    
    /**
     * Disposes this collection of uniforms
     */
    ~Context() {
        first = 0;
        last  = 0;
        perspective = nullptr;
        texture  = nullptr;
        blockptr = -1;
        stencil = false;
        pushed  = false;
        blur = 0;
        type = 0;
    }
    
    /**
     *
     * Resets this context to its default values
     */
    void reset() {
        first = 0;
        last  = 0;
        draw  = DrawMode::TRIANGLES;
        blend = BlendMode::PREMULT;
        front = StencilMode::NONE;
        back  = StencilMode::NONE;
        colorMask = 0xf;
        stencil = false;
        perspective = std::make_shared<Mat4>();
        perspective->setIdentity();
        texture  = nullptr;
        blockptr = -1;
        pushed = false;
        blur  = 0;
        type  = 0;
        dirty = 0;
    }
    
    /** The first vertex index position for this set of uniforms */
    Uint32 first;
    /** The last vertex index position for this set of uniforms */
    Uint32 last;
    /** The drawing type for the shader */
    Uint32 type;
    /** The stored drawing command */
    DrawMode draw;
    /** The stored blending state */
    BlendState blend;
    /** The front stencil state */
    StencilState front;
    /** The back stencil state */
    StencilState back;
    /** The color mask settings to suppress stencil drawing */
    Uint8 colorMask;
    /** Whether to even use the stencil */
    bool stencil;
    /** The stored perspective matrix */
    std::shared_ptr<Mat4> perspective;
    /** The stored texture */
    std::shared_ptr<Texture> texture;
    /** The radius for our blur function */
    float blur;
    /** The stored block offset for gradient and scissor */
    Sint64 blockptr;
    /** The dirty bits relative to the previous set of uniforms */
    Uint32 dirty;
    /** Whether we have pushed a uniform block for this context */
    bool pushed;
};

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate sprite batch with no buffers.
 *
 * You must initialize the buffer before using it.
 */
SpriteBatch::SpriteBatch() :
_initialized(false),
_active(false),
_inflight(false),
_vertData(nullptr),
_indxData(nullptr),
_color(Color4f::WHITE),
_context(nullptr),
_vertMax(0),
_vertSize(0),
_indxMax(0),
_indxSize(0),
_vertTotal(0),
_callTotal(0) {
    _shader = nullptr;
    _vertbuff = nullptr;
    _unifbuff = nullptr;
    _gradient = nullptr;
    _scissor  = nullptr;
}

/**
 * Deletes the vertex buffers and resets all attributes.
 *
 * You must reinitialize the sprite batch to use it.
 */
void SpriteBatch::dispose() {
    if (_vertData) {
        delete[] _vertData; _vertData = nullptr;
    }
    if (_indxData) {
        delete[] _indxData; _indxData = nullptr;
    }
    if (_context != nullptr) {
        delete _context; _context = nullptr;
    }
    _shader = nullptr;
    _vertbuff = nullptr;
    _unifbuff = nullptr;
    _gradient = nullptr;
    _scissor  = nullptr;
    
    _vertMax  = 0;
    _vertSize = 0;
    _indxMax  = 0;
    _indxSize = 0;
    _color = Color4f::WHITE;
    
    _vertTotal = 0;
    _callTotal = 0;
    
    _initialized = false;
    _inflight = false;
    _active = false;
}

/**
 * Initializes a sprite batch with the default vertex capacity.
 *
 * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
 * If the mesh exceeds these values, the sprite batch will flush before
 * before continuing to draw. Similarly uniform buffer is initialized with
 * 512 buffer positions. This means that the uniform buffer is comparable
 * in memory size to the vertices, but only allows 512 gradient or scissor
 * mask context switches before the sprite batch must flush. If you wish to
 * increase (or decrease) the capacity, use the alternate initializer.
 *
 * The sprite batch begins with the default blank texture, and color white.
 * The perspective matrix is the identity.
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init() {
    return init(DEFAULT_CAPACITY);
}

/** 
 * Initializes a sprite batch with the default vertex capacity.
 *
 * The index capacity will be 3 times the vertex capacity. The maximum
 * number of possible indices is the maximum size_t, so the vertex size
 * must be a third that.  In addition, the sprite batch will allocate
 * 1/16 of the vertex capacity for uniform blocks (for gradients and
 * scissor masks). This means that the uniform buffer is comparable
 * in memory size to the vertices while still allowing a reasonably
 * high rate of change for quads and regularly shaped sprites.
 *
 * If the mesh exceeds the capacity, the sprite batch will flush before
 * before continuing to draw. You should tune your system to have the
 * appropriate capacity.  To small a capacity will cause the system to
 * thrash.  However, too large a capacity could stall on memory transfers.
 *
 * The sprite batch begins with no active texture, and the color white.
 * The perspective matrix is the identity.
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init(unsigned int capacity) {
    ShaderSource vsource;
    ShaderSource fsource;
    vsource.initOpenGL(oglShaderVert, ShaderStage::VERTEX);
    fsource.initOpenGL(oglShaderFrag, ShaderStage::FRAGMENT);
    
    auto shader = GraphicsShader::alloc(vsource,fsource);
    shader->setAttributeStride(0, sizeof(SpriteVertex));
    
    // Set up the shader
    AttributeDef attrib;
    attrib.type = GLSLType::VEC2;
    attrib.group = 0;
    attrib.location = 0;
    attrib.offset = offsetof(SpriteVertex,position);
    shader->declareAttribute("aPosition", attrib);

    attrib.type = GLSLType::UCOLOR;
    attrib.location = 1;
    attrib.offset = offsetof(SpriteVertex,color);
    shader->declareAttribute("aColor", attrib);

    attrib.type = GLSLType::VEC2;
    attrib.location = 2;
    attrib.offset = offsetof(SpriteVertex,texcoord);
    shader->declareAttribute("aTexCoord", attrib);
    
    attrib.location = 3;
    attrib.offset = offsetof(SpriteVertex,gradcoord);
    shader->declareAttribute("aGradCoord", attrib);

    UniformDef unif;
    unif.type = GLSLType::MAT4;
    unif.location = 0;
    unif.size = sizeof(Mat4);
    unif.stage = ShaderStage::VERTEX;
    shader->declareUniform("uPerspective", unif);

    unif.type = GLSLType::INT;
    unif.location = 1;
    unif.size = sizeof(int);
    unif.stage = ShaderStage::FRAGMENT;
    shader->declareUniform("uType", unif);

    unif.type = GLSLType::INT;
    unif.location = 2;
    unif.size = sizeof(int);
    unif.stage = ShaderStage::FRAGMENT;
    shader->declareUniform("uPremult", unif);

    unif.type = GLSLType::VEC2;
    unif.location = 3;
    unif.size = sizeof(Vec2);
    unif.stage = ShaderStage::FRAGMENT;
    shader->declareUniform("uBlur", unif);

    ResourceDef resrc;
    resrc.type = ResourceType::TEXTURE;
    resrc.location = 0;
    resrc.stage = ShaderStage::FRAGMENT;
    resrc.arraysize = 1;
    resrc.blocksize = 0;
    resrc.set = 0;
    resrc.unbounded = true;
    shader->declareResource("uTexture", resrc);

    resrc.type = ResourceType::MULTI_BUFFER;
    resrc.location = 1;
    resrc.blocksize = capacity/16;
    shader->declareResource("uContext", resrc);

    if (!shader->compile()) {
        return false;
    }

    return init(capacity,shader);
}

/**
 * Initializes a sprite batch with the default vertex capacity and given shader
 *
 * The index capacity will be 3 times the vertex capacity. The maximum
 * number of possible indices is the maximum size_t, so the vertex size
 * must be a third that.  In addition, the sprite batch will allocate
 * 1/16 of the vertex capacity for uniform blocks (for gradients and
 * scissor masks). This means that the uniform buffer is comparable
 * in memory size to the vertices while still allowing a reasonably
 * high rate of change for quads and regularly shaped sprites.
 *
 * If the mesh exceeds the capacity, the sprite batch will flush before
 * before continuing to draw. You should tune your system to have the
 * appropriate capacity.  To small a capacity will cause the system to
 * thrash.  However, too large a capacity could stall on memory transfers.
 *
 * The sprite batch begins with no active texture, and the color white.
 * The perspective matrix is the identity.
 *
 * See the class description for the properties of a valid shader. You
 * should avoid this initializer unless you know what you are doing.
 *
 * @param shader The shader to use for this spritebatch
 *
 * @return true if initialization was successful.
 */
bool SpriteBatch::init(unsigned int capacity, const std::shared_ptr<GraphicsShader>& shader) {
    if (_initialized) {
        CUAssertLog(false, "SpriteBatch is already initialized");
        return false; // If asserts are turned off.
    } else if (shader == nullptr) {
        CUAssertLog(false, "SpriteBatch shader cannot be null");
        return false; // If asserts are turned off.
    }
    
    _shader = shader;

    // Set up data arrays;
    _vertMax = capacity;
    _vertData = new SpriteVertex[_vertMax];
    _indxMax = capacity*3;
    _indxData = new Uint32[_indxMax];

    // Create the GPU data structures
    _vertbuff = VertexBuffer::alloc(_vertMax,sizeof(SpriteVertex));
    _indxbuff = IndexBuffer::alloc(_indxMax);
    _unifbuff = UniformBuffer::alloc(40*sizeof(float),capacity/16);
    
    // Layout std140 format
    _unifbuff->setOffset("scMatrix", 0);
    _unifbuff->setOffset("scExtent", 48);
    _unifbuff->setOffset("scScale",  56);
    _unifbuff->setOffset("gdMatrix", 64);
    _unifbuff->setOffset("gdInner",  112);
    _unifbuff->setOffset("gdOuter",  128);
    _unifbuff->setOffset("gdExtent", 144);
    _unifbuff->setOffset("gdRadius", 152);
    _unifbuff->setOffset("gdFeathr", 156);
    
    _shader->setVertices(0, _vertbuff);
    _shader->setIndices(_indxbuff);
    _shader->setUniformBuffer("uContext", _unifbuff);

    _context = new Context();
    _context->dirty = DIRTY_ALL_VALS;
    return true;
}


#pragma mark -
#pragma mark Attributes
/**
 * Sets the active color of this sprite batch 
 *
 * All subsequent shapes and outlines drawn by this sprite batch will be
 * tinted by this color.  This color is white by default.
 *
 * @param color The active color for this sprite batch
 */
void SpriteBatch::setColor(const Color4 color) {
    if (_color == color) {
        return;
    }
    _color = color;
}

/**
 * Sets the shader for this sprite batch
 *
 * This value may NOT be changed during a drawing pass.
 *
 * @param shader The active shader for this sprite batch
 */
void SpriteBatch::setShader(const std::shared_ptr<GraphicsShader>& shader) {
    CUAssertLog(_active, "Attempt to reassign shader while drawing is active");
    CUAssertLog(shader != nullptr, "Shader cannot be null");
    _shader->setVertices(0, _vertbuff);
    _shader->setIndices(_indxbuff);
    _shader->setUniformBuffer("uContext", _unifbuff);
    _context->dirty = DIRTY_ALL_VALS;
}


/**
 * Sets the active perspective matrix of this sprite batch
 *
 * The perspective matrix is the combined modelview-projection from the
 * camera. By default, this is the identity matrix. Changing this value
 * will cause the sprite batch to flush.
 *
 * @param perspective   The active perspective matrix for this sprite batch
 */
void SpriteBatch::setPerspective(const Mat4& perspective) {
    if (_context->perspective.get() != &perspective) {
        if (_inflight) { record(); }
        auto matrix = std::make_shared<Mat4>(perspective);
        _context->perspective = matrix;
        _context->dirty = _context->dirty | DIRTY_PERSPECTIVE;
    }
}

/**
 * Returns the active perspective matrix of this sprite batch
 *
 * The perspective matrix is the combined modelview-projection from the
 * camera.  By default, this is the identity matrix.
 *
 * @return the active perspective matrix of this sprite batch
 */
const Mat4& SpriteBatch::getPerspective() const {
    return *(_context->perspective.get());
}

/**
 * Sets the active texture of this sprite batch
 *
 * All subsequent shapes and outlines drawn by this sprite batch will use
 * this texture.  If the value is nullptr, all shapes and outlines will be
 * draw with a solid color instead.  This value is nullptr by default.
 *
 * Changing this value will cause the sprite batch to flush. However, a
 * subtexture will not cause a pipeline flush. This is an important
 * argument for using texture atlases.
 *
 * @param texture   The active texture for this sprite batch
 */
void SpriteBatch::setTexture(const std::shared_ptr<Texture>& texture) {
    if (texture == _context->texture) {
        return;
    }

    if (_inflight) { record(); }
    if (texture == nullptr) {
        // Active texture is not null
        _context->dirty = _context->dirty | DIRTY_DRAWTYPE;
        _context->texture = nullptr;
        _context->type = _context->type & ~TYPE_TEXTURE;
    } else if (_context->texture == nullptr) {
        // Texture is not null
        _context->dirty = _context->dirty | DIRTY_DRAWTYPE | DIRTY_TEXTURE;
        _context->texture = texture;
        _context->type = _context->type | TYPE_TEXTURE;
    } else {
        // Both must be not nullptr
        if (_context->texture->id() != texture->id()) {
            _context->dirty = _context->dirty | DIRTY_TEXTURE;
        }
        _context->texture = texture;
    }
}

/**
 * Returns the active texture of this sprite batch
 *
 * All subsequent shapes and outlines drawn by this sprite batch will use
 * this texture.  If the value is nullptr, all shapes and outlines will be
 * drawn with a solid color instead.  This value is nullptr by default.
 *
 * @return the active texture of this sprite batch
 */
const std::shared_ptr<Texture>& SpriteBatch::getTexture() const {
    return _context->texture;
}

/**
 * Returns the active gradient of this sprite batch
 *
 * Gradients may be used in the place of (and together with) colors.
 * Gradients are like applied textures, and use the gradient coordinates
 * in {@link SpriteVertex} as their texture coordinates.
 *
 * Setting a gradient value does not guarantee that it will be used.
 * Gradients can be turned on or off by the {@link #useGradient} method.
 * By default, the gradient will not be used (as it is slower than
 * solid colors).
 *
 * All gradients are tinted by the active color. Unless you explicitly
 * want this tinting, you should set the active color to white before
 * drawing with an active gradient.
 *
 * This method returns a copy of the internal gradient. Changes to this
 * object have no effect on the sprite batch.
 *
 * @return The active gradient for this sprite batch
 */
std::shared_ptr<Gradient> SpriteBatch::getGradient() const {
    if (_gradient != nullptr) {
        return Gradient::allocCopy(_gradient);
    }
    return nullptr;
}

/**
 * Sets the active gradient of this sprite batch
 *
 * Gradients may be used in the place of (and together with) colors.
 * Gradients are like applied textures, and use the gradient coordinates
 * in {@link SpriteVertex} as their texture coordinates.
 *
 * If this value is nullptr, then no gradient is active. In that case,
 * the color vertex attribute will be interpretted as normal (e.g. a
 * traditional color vector).  This value is nullptr by default.
 *
 * All gradients are tinted by the active color. Unless you explicitly
 * want this tinting, you should set the active color to white before
 * drawing with an active gradient.
 *
 * This method acquires a copy of the gradient. Changes to the original
 * gradient after calling this method have no effect.
 *
 * @param gradient   The active gradient for this sprite batch
 */
void SpriteBatch::setGradient(const std::shared_ptr<Gradient>& gradient) {
    if (gradient == _gradient) {
        return;
    }
    
    if (_inflight) { record(); }
    if (gradient == nullptr) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_GRADIENT;
        _gradient = nullptr;
    } else {
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_GRADIENT;
        _gradient = Gradient::allocCopy(gradient);
    }
}

/**
 * Returns the active scissor mask of this sprite batch
 *
 * Scissor masks may be combined with all types of drawing (colors,
 * textures, and gradients).  They are specified in the same coordinate
 * system as {@link getPerspective}.
 *
 * If this value is nullptr, then no scissor mask is active. This value
 * is nullptr by default.
 *
 * This method returns a copy of the internal scissor. Changes to this
 * object have no effect on the sprite batch.
 *
 * @return The active scissor mask for this sprite batch
 */
std::shared_ptr<Scissor> SpriteBatch::getScissor() const {
    if (_scissor != nullptr) {
        return Scissor::alloc(_scissor);
    }
    return nullptr;
}

/**
 * Sets the active scissor mask of this sprite batch
 *
 * Scissor masks may be combined with all types of drawing (colors,
 * textures, and gradients).  They are specified in the same coordinate
 * system as {@link getPerspective}. 
 *
 * If this value is nullptr, then no scissor mask is active. This value
 * is nullptr by default.
 *
 * This method acquires a copy of the scissor. Changes to the original
 * scissor mask after calling this method have no effect.
 *
 * @param scissor   The active scissor mask for this sprite batch
 */
void SpriteBatch::setScissor(const std::shared_ptr<Scissor>& scissor) {
    if (scissor == _scissor) {
        return;
    }
    
    if (_inflight) { record(); }
    if (scissor == nullptr) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_SCISSOR;
        _scissor = nullptr;
    } else {
        _context->dirty = _context->dirty | DIRTY_UNIBLOCK | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_SCISSOR;
        _scissor = Scissor::alloc(scissor);
    }
}

/**
 * Sets the blur radius in pixels (0 if there is no blurring).
 *
 * This sprite batch supports a simple Gaussian blur. The blur
 * samples at 5 points along each axis. The values are essentially
 * at the radius, half-radius, and center. Because of the limited
 * sampling, large radii will start to produce a pixellation effect.
 * But it can produce acceptable blur effects with little cost to
 * performance. It is especially ideal for font-blur effects on
 * font atlases.
 *
 * When applying a blur to a {@link GlyphRun}, make sure that the
 * source {@link Font} has {@link Font#setPadding} set to at least
 * the blur radius. Otherwise, the blur will bleed into other glyphs.
 *
 * Setting this value to 0 will disable texture blurring.  This
 * value is 0 by default.
 *
 * @param radius    The blur radius in pixels
 */
void SpriteBatch::setBlur(float radius) {
    if (_context->blur == radius) {
        return;
    }
    
    if (_inflight) { record(); }
    if (radius == 0.0f) {
        // Active gradient is not null
        _context->dirty = _context->dirty | DIRTY_BLURSTEP | DIRTY_DRAWTYPE;
        _context->type = _context->type & ~TYPE_GAUSSBLUR;
    } else if (_context->blur == 0){
        _context->dirty = _context->dirty | DIRTY_BLURSTEP | DIRTY_DRAWTYPE;
        _context->type = _context->type | TYPE_GAUSSBLUR;
    } else {
        _context->dirty = _context->dirty | DIRTY_BLURSTEP;
    }
    _context->blur = radius;
}

/**
 * Returns the blur radius in pixels (0 if there is no blurring).
 *
 * This sprite batch supports a simple Gaussian blur. The blur
 * samples at 5 points along each axis. The values are essentially
 * at the radius, half-radius, and center. Because of the limited
 * sampling, large radii will start to produce a pixellation effect.
 * But it can produce acceptable blur effects with little cost to
 * performance. It is especially ideal for font-blur effects on
 * font atlases.
 *
 * When applying a blur to a {@link GlyphRun}, make sure that the
 * source {@link Font} has {@link Font#setPadding} set to at least
 * the blur radius. Otherwise, the blur will bleed into other glyphs.
 *
 * Setting this value to 0 will disable texture blurring.  This
 * value is 0 by default.
 *
 * @return the blur radius in pixels (0 if there is no blurring).
 */
float SpriteBatch::getBlur() const {
    return _context->blur;
}

/**
 * Sets the color blend state for this spritebatch
 *
 * This provides much more fine-tuned control over color blending that
 * {@link #setBlendMode}. This value is {@link BlendMode#PREMULT} by
 * default as image files are loaded with premultiplied alpha.
 *
 * @param state The color blend state for this spritebatch
 */
void SpriteBatch::setBlendState(const BlendState& blend) {
    if (_context->blend != blend) {
        if (_inflight) { record(); }
        _context->blend = blend;
        _context->dirty = _context->dirty | DIRTY_BLEND;
    }
}

/**
 * Returns the color blend state for this spritebatch
 *
 * Note that if color blending was set using {@link #setBlendMode}, that
 * blend mode was expanded into a blend state, and this value will be the
 * result of this expansion. This value is {@link BlendMode#PREMULT} by
 * default as image files are loaded with premultiplied alpha.
 *
 * @return the color blend state for this spritebatch
 */
const BlendState& SpriteBatch::getBlendState() const {
    return _context->blend;
}

/**
 * Sets whether stencil testing is enabled.
 *
 * None of the stencil methods will do anything unless this value
 * is set to true.
 *
 * @param enable    Whether to enable stencil testing
 */
void SpriteBatch::enableStencilTest(bool enable) {
    if (_context->stencil != enable) {
        if (_inflight) { record(); }
        _context->stencil = enable;
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Returns whether stencil testing is currently enabled in this shader.
 *
 * None of the stencil methods will do anything unless this value
 * is set to true.
 *
 * @return whether stencil testing  is currently enabled in this shader.
 */
bool SpriteBatch::usesStencilTest() const {
    return _context->stencil;
}

/**
 * Sets the stencil mode for this spritebatch
 *
 * A stencil mode is a predefined stencil setting that applies to both
 * front-facing and back-facing triangles. You cannot determine the stencil
 * mode for a spritebatch after it is set. You can only get the stencil
 * state for that mode with the methods {@link #getFrontStencilState} and
 * {@link #getBackStencilState}.
 *
 * Note that this method may also alter the color mask, as some stencil
 * modes require a specific color mask to achieve their affects.
 *
 * @param mode  The stencil mode for this spritebatch
 */
void SpriteBatch::setStencilMode(StencilMode mode) {
    StencilState stencil(mode);
    if (_context->front != stencil || _context->back != stencil) {
        if (_inflight) { record(); }
        _context->front = stencil;
        _context->back = stencil;
        switch (mode) {
            case StencilMode::CLEAR:
            case StencilMode::STAMP:
            case StencilMode::STAMP_EVENODD:
            case StencilMode::STAMP_NONZERO:
                _context->colorMask = 0;
                break;
            case StencilMode::CLIP:
            case StencilMode::CLIP_CLEAR:
            case StencilMode::MASK:
            case StencilMode::MASK_CLEAR:
            case StencilMode::CLAMP:
                _context->colorMask = 0xf;
            default:
                break;
        }
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Sets the stencil state (both front and back) for this spritebatch
 *
 * This method will change both the front and back stencil states to the
 * given value. You can access the individual settings with the methods
 * {@link #getFrontStencilState} and {@link #getBackStencilState}.
 *
 * @param stencil   The (front and back) stencil state
 */
void SpriteBatch::setStencilState(const StencilState& stencil) {
    if (_context->front != stencil || _context->back != stencil) {
        if (_inflight) { record(); }
        _context->front = stencil;
        _context->back = stencil;
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Sets the font-facing stencil state for this spritebatch
 *
 * This setting will only apply to triangles that are front-facing. Use
 * {@link #setBackStencilState} to affect back-facing triangles.
 *
 * @param stencil   The font-facing stencil state
 */
void SpriteBatch::setFrontStencilState(const StencilState& stencil) {
    if (_context->front != stencil) {
        if (_inflight) { record(); }
        _context->front = stencil;
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Sets the back-facing stencil state for this spritebatch
 *
 * This setting will only apply to triangles that are back-facing. Use
 * {@link #setFrontStencilState} to affect front-facing triangles.
 *
 * @param stencil   The back-facing stencil state
 */
void SpriteBatch::setBackStencilState(const StencilState& stencil) {
    if (_context->back != stencil) {
        if (_inflight) { record(); }
        _context->back = stencil;
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Returns the font-facing stencil state for this spritebatch
 *
 * This setting will only apply to triangles that are front-facing. Use
 * {@link #getBackStencilState} to see the setting for back-facing
 * triangles.
 *
 * @return the font-facing stencil state for this spritebatch
 */
const StencilState& SpriteBatch::getFrontStencilState() const {
    return _context->front;
}

/**
 * Returns the back-facing stencil state for this spritebatch
 *
 * This setting will only apply to triangles that are back-facing. Use
 * {@link #getFrontStencilState} to see the setting for front-facing
 * triangles.
 *
 * @return the back-facing stencil state for this spritebatch
 */
const StencilState& SpriteBatch::getBackStencilState() const {
    return _context->back;
}

/**
 * Sets the color mask for stencils in this spritebatch
 *
 * The color mask is used to prevent certain colors from being drawn.
 * This is useful for when you want to draw to a stencil buffer and
 * nothing else.
 *
 * @parm mask   The stencil color mask
 */
void SpriteBatch::setColorMask(Uint8 mask) {
    if (_context->colorMask != mask) {
        if (_inflight) { record(); }
        _context->colorMask = mask;
        _context->dirty = _context->dirty | DIRTY_STENCIL;
    }
}

/**
 * Returns the color mask for stencils in this spritebatch
 *
 * The color mask is used to prevent certain colors from being drawn.
 * This is useful for when you want to draw to a stencil buffer and
 * nothing else.
 *
 * @return the color mask for stencils in this spritebatch
 */
Uint8 SpriteBatch::getColorMask() const {
    return _context->colorMask;
}

/**
 * Sets the current draw mode.
 *
 * The value must be one of {@link DrawMode#TRIANGLES} or
 * {@link DrawMode#LINES}.
 *
 * @param mode  The new draw mode
 */
void SpriteBatch::setDrawMode(DrawMode mode) {
    if (_context->draw != mode) {
        if (_inflight) { record(); }
        _context->draw = mode;
        _context->dirty = _context->dirty | DIRTY_DRAWMODE;
    }
}

/**
 * Returns the current draw mode.
 *
 * The value must be one of {@link DrawMode#TRIANGLES} or
 * {@link DrawMode#LINES}.
 *
 * @return the current draw mode.
 */
DrawMode SpriteBatch::getDrawMode() const {
    return _context->draw;
}

#pragma mark -
#pragma mark Rendering
/**
 * Starts drawing with the current perspective matrix.
 *
 * This call will disable depth buffer writing. It enables blending and
 * texturing. You must call end() to complete drawing.
 *
 * Calling this method will reset the vertex and OpenGL call counters to 0.
 */
void SpriteBatch::begin() {
    _shader->setCullMode(CullMode::NONE);
    _shader->enableDepthTest(false);
    _shader->enableBlending(true);

    _framebuff = nullptr;
    _active = true;
    _callTotal = 0;
    _vertTotal = 0;
}

/**
 * Starts drawing with the given frame buffer.
 *
 * This call will disable depth buffer writing. It enables blending and
 * texturing. You must call either {@link #flush} or {@link #end} to
 * complete drawing.
 *
 * This allows the sprite batch to draw to an offscreen framebuffer.
 * You must call {@link #end} to resume drawing to the primary display.
 *
 * Calling this method will reset the vertex and draw call counters to 0.
 *
 * @param framebuffer   The offscreen framebuffer to draw to.
 */
void SpriteBatch::begin(const std::shared_ptr<FrameBuffer>& framebuffer) {
    _shader->setCullMode(CullMode::NONE);
    _shader->enableDepthTest(false);
    _shader->enableBlending(true);

    _framebuff = framebuffer;
    _active = true;
    _callTotal = 0;
    _vertTotal = 0;
}

/**
 * Completes the drawing pass for this sprite batch, flushing the buffer.
 *
 * This method will unbind the associated shader. It must always be called
 * after a call to {@link #begin}.
 */
void SpriteBatch::end() {
    CUAssertLog(_active,"SpriteBatch is not active");
    flush();
    _context->reset();
    _context->dirty = DIRTY_ALL_VALS;
    _active = false;
}


/**
 * Flushes the current mesh to the underlying shader.
 *
 * This methods converts all of the batched drawing commands into low-level
 * drawing operations in the associated graphics API. This method is useful
 * in OpenGL in that it allows us to recycle internal data structures,
 * reducing the memory footprint. However, it is rarely useful in Vulkan. It
 * is provided simply to keep the interface uniform between graphics APIs.
 */
void SpriteBatch::flush() {
    if (_indxSize == 0 || _vertSize == 0) {
        return;
    } else if (_context->first != _indxSize) {
        record();
    }
    
    // Load the actual data
    _vertbuff->loadData(_vertData, _vertSize);
    _indxbuff->loadData(_indxData, _indxSize);
    _unifbuff->flush();

    
    // Start the drawing pass
    _shader->setVertices(0, _vertbuff);
    _shader->setIndices(_indxbuff);
    _shader->setUniformBuffer("uContext", _unifbuff);
    
    if (_framebuff) {
        _shader->begin(_framebuff);
    } else {
        _shader->begin();
    }
    // Chunk the uniforms
    for(auto it = _history.begin(); it != _history.end(); ++it) {
        Context* next = *it;
        if (next->dirty & DIRTY_DRAWTYPE) {
             _shader->pushInt("uType", next->type);
        }
        if (next->dirty & DIRTY_DRAWMODE) {
            _shader->setDrawMode(next->draw);
        }
        if (next->dirty & DIRTY_BLEND) {
            _shader->setBlendState(next->blend);
        }
        if (next->dirty & DIRTY_STENCIL) {
            _shader->enableStencilTest(next->stencil);
            _shader->setFrontStencilState(next->front);
            _shader->setBackStencilState(next->back);
            _shader->setColorMask(next->colorMask);
        }
        if (next->dirty & DIRTY_BLURSTEP || next->dirty & DIRTY_TEXTURE) {
            blurTexture(next->texture,next->blur);
        }
        if (next->dirty & DIRTY_PERSPECTIVE) {
            _shader->pushMat4("uPerspective",*(next->perspective.get()));
        }
        if (next->dirty & DIRTY_TEXTURE) {
            auto texture = next->texture == nullptr ? Texture::getBlank() : next->texture;
            _shader->setTexture("uTexture", texture);
            if (next->texture == nullptr || !next->texture->isPremultiplied()) {
                _shader->pushInt("uPremult", 0);
            } else {
                _shader->pushInt("uPremult", 1);
            }
        }
        if (next->dirty & DIRTY_UNIBLOCK) {
            _shader->setBlock("uContext",(Uint32)next->blockptr);
        }
    
        
        Uint32 amt = next->last-next->first;
        _shader->drawIndexed(amt, next->first);
        _callTotal++;
    }
    
    _shader->end();
    
    // Increment the counters
    _vertTotal += _indxSize;
    
    _vertSize = _indxSize = 0;
    unwind();
    _context->first = 0;
    _context->last  = 0;
    _context->blockptr = -1;
}


#pragma mark -
#pragma mark Solid Shapes
/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The texture will fill the entire rectangle with texture coordinate 
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * @param rect      The rectangle to draw
 */
void SpriteBatch::fill(const Rect& rect) {
    setDrawMode(DrawMode::TRIANGLES);
    prepare(rect);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The texture will fill the entire rectangle with texture coordinate
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * @param rect      The rectangle to draw
 * @param offset    The rectangle offset
 */
void SpriteBatch::fill(const Rect& rect, const Vec2& offset) {
    Affine2 transform;
    transform.translate(offset.x,offset.y);
    setDrawMode(DrawMode::TRIANGLES);
    prepare(rect,transform);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively, you 
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * @param rect      The rectangle to draw
 * @param origin    The rotational offset in the rectangle
 * @param scale     The amount to scale the rectangle
 * @param angle     The amount to rotate the rectangle
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::fill(const Rect& rect, const Vec2& origin, const Vec2&scale,
                       float angle, const Vec2& offset) {
    Affine2 transform;
    transform.translate(-origin.x,-origin.y);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);

    setDrawMode(DrawMode::TRIANGLES);
    prepare(rect,transform);
}

/**
 * Draws the given rectangle filled with the current color and texture.
 *
 * The rectangle will transformed by the given matrix. The transform will 
 * be applied assuming the given origin, which is specified relative to 
 * the origin of the rectangle (not world coordinates).  So to apply the 
 * transform to the center of the rectangle, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle with texture coordinate
 * (0,1) at the bottom left corner identified by rect,origin. To draw only
 * part of a texture, use a subtexture to fill the rectangle with the
 * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
 * for more fine-tuned control.
 *
 * @param rect      The rectangle to draw
 * @param origin    The rotational offset in the rectangle
 * @param transform The coordinate transform
 */
void SpriteBatch::fill(const Rect& rect, const Vec2& origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setDrawMode(DrawMode::TRIANGLES);
    prepare(rect,matrix);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation 
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A 
 * vertical coordinate has texture coordinate 1-y/texture.height. As a 
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the 
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param poly      The polygon to draw
 */
void SpriteBatch::fill(const Poly2& poly) {
    setDrawMode(DrawMode::TRIANGLES);
    prepare(poly);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will be offset by the given position.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param poly      The polygon to draw
 * @param offset    The polygon offset
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2& offset) {
    setDrawMode(DrawMode::TRIANGLES);
    prepare(poly,offset);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param poly      The polygon to draw
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2& origin, const Vec2&scale,
                       float angle, const Vec2& offset) {
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);
    setDrawMode(DrawMode::TRIANGLES);
    prepare(poly,transform);
}

/**
 * Draws the given polygon filled with the current color and texture.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param poly      The polygon to draw
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::fill(const Poly2& poly, const Vec2& origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setDrawMode(DrawMode::TRIANGLES);
    prepare(poly,matrix);
}

#pragma mark -
#pragma mark Outlines
/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner 
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * @param rect      The rectangle to outline
 */
void SpriteBatch::outline(const Rect& rect) {
    setDrawMode(DrawMode::LINES);
    prepare(rect);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * @param rect      The rectangle to outline
 * @param offset    The polygon offset
 */
void SpriteBatch::outline(const Rect& rect, const Vec2& offset) {
    Affine2 transform;
    transform.translate(offset);
    setDrawMode(DrawMode::LINES);
    prepare(rect,transform);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * @param rect      The rectangle to outline
 * @param origin    The rotational offset in the rectangle
 * @param scale     The amount to scale the rectangle
 * @param angle     The amount to rotate the rectangle
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::outline(const Rect& rect, const Vec2& origin, const Vec2&scale,
                          float angle, const Vec2& offset) {
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate((Vec3)offset);
    setDrawMode(DrawMode::LINES);
    prepare(rect,transform);
}

/**
 * Outlines the given rectangle with the current color and texture.
 *
 * The rectangle will transformed by the given matrix. The transform will 
 * be applied assuming the given origin, which is specified relative to 
 * the origin of the rectangle (not world coordinates).  So to apply the 
 * transform to the center of the rectangle, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The drawing will be a wireframe of a rectangle.  The wireframe will
 * be textured with Texture coordinate (0,1) at the bottom left corner
 * identified by rect,origin. The remaining edges will correspond to the
 * edges of the texture. To draw only part of a texture, use a subtexture
 * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you can
 * use a {@link Poly2} for more fine-tuned control.
 *
 * @param rect      The rectangle to outline
 * @param origin    The rotational offset in the rectangle
 * @param transform The coordinate transform
 */
void SpriteBatch::outline(const Rect& rect, const Vec2& origin, const Affine2& transform) {
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setDrawMode(DrawMode::LINES);
    prepare(rect,matrix);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param path      The path to outline
 */
void SpriteBatch::outline(const Path2& path) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    setDrawMode(DrawMode::LINES);
    prepare(poly);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will be offset by the given position.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param path      The path to outline
 * @param offset    The path offset
 */
void SpriteBatch::outline(const Path2& path, const Vec2& offset) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    setDrawMode(DrawMode::LINES);
    prepare(poly,offset);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the path (not world
 * coordinates). Hence this origin is essentially the pixel coordinate
 * of the texture (see below) to assign as the rotational center.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param path      The path to outline
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the path
 * @param angle     The amount to rotate the path
 * @param offset    The path offset in world coordinates
 */
void SpriteBatch::outline(const Path2& path, const Vec2& origin, const Vec2&scale,
                          float angle, const Vec2& offset) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    Affine2 transform;
    Affine2::createTranslation(-origin.x,-origin.y,&transform);
    transform.scale(scale);
    transform.rotate(angle);
    transform.translate(offset);
    setDrawMode(DrawMode::LINES);
    prepare(poly,transform);
}

/**
 * Outlines the given path with the current color and texture.
 *
 * The path will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative
 * to the origin of the path (not world coordinates). Hence this origin
 * is essentially the pixel coordinate of the texture (see below) to
 * assign as the origin of this transform.
 *
 * The drawing will be a wireframe of a path, but the lines are textured.
 * The vertex coordinates will be determined by path vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply outlining the rectangle.
 *
 * One way to think of the path is as a "cookie cutter".  Treat the
 * path coordinates as pixel coordinates in the texture file, and use
 * that to determine how the texture fills the path. This may make the
 * path larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param path      The path to outline
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::outline(const Path2& path, const Vec2& origin, const Affine2& transform) {
    // This is a violation of spirit of poly, but makes code clean
    Poly2 poly(path.vertices);
    path.getIndices(poly.indices);
    Affine2 matrix;
    matrix.translate(-origin.x,-origin.y);
    matrix *= transform;
    setDrawMode(DrawMode::LINES);
    prepare(poly,matrix);
}

// The methods below are to make transition from the intro class easier
#pragma mark -
#pragma mark Convenience Methods
/**
 * Draws the texture (without tint) at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a rectangle of the size of the texture, with bottom left
 * corner at the given position.
 *
 * @param texture   The new active texture
 * @param position  The bottom left corner of the texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Vec2& position) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(position.x,position.y, (float)texture->getWidth(), (float)texture->getHeight()));
}

/**
 * Draws the tinted texture at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a rectangle of the size of the texture, with bottom left
 * corner at the given position.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param position  The bottom left corner of the texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Vec2& position) {
    setTexture(texture);
    setColor(color);
    fill(Rect(position.x,position.y, (float)texture->getWidth(), (float)texture->getHeight()));
}

/**
 * Draws the texture (without tint) inside the given bounds
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the specified rectangle filled with the texture.
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect& bounds) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds);
}

/**
 * Draws the texture (without tint) inside the given bounds
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the specified rectangle filled with the texture.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param bounds    The rectangle to texture
 */
 void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Rect& bounds) {
     setTexture(texture);
     setColor(color);
     fill(bounds);
 }

/**
 * Draws the texture (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given parameters.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified in texture pixel coordinates (e.g from the bottom
 * left corner).
 *
 * @param texture   The new active texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The texture offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Vec2& origin, const Vec2&scale, float angle,
                       const Vec2& offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, scale, angle, offset);
}

/**
 * Draws the tinted texture transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given parameters.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified in texture pixel coordinates (e.g from the bottom
 * left corner).
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The texture offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Vec2& origin, const Vec2&scale, float angle,
                       const Vec2& offset) {
    setTexture(texture);
    setColor(color);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, scale, angle, offset);
}

/**
 * Draws the texture (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively, you 
 * can use a {@link Poly2} for more fine-tuned control.
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect& bounds,
                       const Vec2& origin, const Vec2&scale, float angle,
                       const Vec2& offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds, origin, scale, angle, offset);
}

/**
 * Draws the tinted texture transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture.
 *
 * The rectangle will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin, 
 * which is specified relative to the origin of the rectangle (not world 
 * coordinates).  So to spin about the center, the origin should be width/2, 
 * height/2 of the rectangle.
 *
 * The texture will fill the entire rectangle before being transformed. 
 * Texture coordinate (0,1) will at the bottom left corner identified by 
 * rect,origin. To draw only part of a texture, use a subtexture to fill 
 * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively,
 * you can use a {@link Poly2} for more fine-tuned control.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the texture
 * @param angle     The amount to rotate the texture
 * @param offset    The rectangle offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Rect& bounds, const Vec2& origin, const Vec2&scale,
                       float angle, const Vec2& offset) {
    setTexture(texture);
    setColor(color);
    fill(bounds, origin, scale, angle, offset);
}

/**
 * Draws the texture (without tint) transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * @param texture   The new active texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Vec2& origin,
                       const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, transform);
}

/**
 * Draws the tinted texture transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws a texture-sized rectangle centered at the given origin, and
 * transformed by the given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Vec2& origin, const Affine2& transform)  {
    setTexture(texture);
    setColor(color);
    fill(Rect(0,0, (float)texture->getWidth(), (float)texture->getHeight()), origin, transform);
}
    
/**
 * Draws the texture (without tint) transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture, transformed by the 
 * given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * @param texture   The new active texture
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Rect& bounds,
                       const Vec2& origin, const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(bounds, origin, transform);
}

/**
 * Draws the tinted texture transformed by the matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It fills the specified rectangle with the texture, transformed by the 
 * given matrix.
 *
 * The transform will be applied assuming the given image origin, which is
 * specified in texture pixel coordinates (e.g from the bottom left corner).
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param bounds    The rectangle to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Rect& bounds, const Vec2& origin,
                       const Affine2& transform) {
    setTexture(texture);
    setColor(color);
    fill(bounds, origin, transform);
}

/**
 * Draws the textured polygon (without tint) at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, offset by the given value.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param offset    The polygon offset
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Poly2& poly, const Vec2& offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, offset);
}

/**
 * Draws the tinted, textured polygon at the given position
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, offset by the given value.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param offset    The polygon offset
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2& offset) {
    setTexture(texture);
    setColor(color);
    fill(poly, offset);
}

/**
 * Draws the textured polygon (without tint) transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, transformed by the given parameters.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Poly2& poly,
                       const Vec2& origin, const Vec2&scale, float angle,
                       const Vec2& offset) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, origin, scale, angle, offset);
}

/**
 * Draws the tinted, textured polygon transformed by the given parameters
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given parameters.
 *
 * The polygon will be scaled first, then rotated, and finally offset
 * by the given position. Rotation is measured in radians and is counter
 * clockwise from the x-axis.  Rotation will be about the provided origin,
 * which is specified relative to the origin of the polygon (not world 
 * coordinates). Hence this origin is essentially the pixel coordinate 
 * of the texture (see below) to assign as the rotational center.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param scale     The amount to scale the polygon
 * @param angle     The amount to rotate the polygon
 * @param offset    The polygon offset in world coordinates
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2& origin, const Vec2&scale,
                       float angle, const Vec2& offset) {
    setTexture(texture);
    setColor(color);
    fill(poly, origin, scale, angle, offset);
}

/**
 * Draws the textured polygon (without tint) transformed by the given matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given matrix.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture,
                       const Poly2& poly, const Vec2& origin,
                       const Affine2& transform) {
    setTexture(texture);
    setColor(Color4::WHITE);
    fill(poly, origin, transform);
}

/**
 * Draws the tinted, textured polygon transformed by the given matrix
 *
 * This is a convenience method that calls the appropriate fill method.
 * It sets both the texture and color (removing the previous active values).
 * It then draws the polygon, translated by the given matrix.
 *
 * The polygon will transformed by the given matrix. The transform will
 * be applied assuming the given origin, which is specified relative to the 
 * origin of the polygon (not world coordinates). Hence this origin is 
 * essentially the pixel coordinate of the texture (see below) to 
 * assign as the origin of this transform.
 *
 * The polygon tesselation will be determined by the indices in poly. If
 * the polygon has not been triangulated (by one of the triangulation
 * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
 * it may not draw properly.
 *
 * The vertex coordinates will be determined by polygon vertex position.
 * A horizontal position x has texture coordinate x/texture.width. A
 * vertical coordinate has texture coordinate 1-y/texture.height. As a
 * result, a rectangular polygon that has the same dimensions as the
 * texture is the same as simply drawing the texture.
 *
 * One way to think of the polygon is as a "cookie cutter".  Treat the
 * polygon coordinates as pixel coordinates in the texture filed, and use
 * that to determine how the texture fills the polygon. This may make the
 * polygon larger than you like in order to get the appropriate texturing.
 * You should use one of the transform methods to fix this.
 *
 * @param texture   The new active texture
 * @param color     The new active color
 * @param poly      The polygon to texture
 * @param origin    The image origin in pixel space
 * @param transform The coordinate transform
 */
void SpriteBatch::draw(const std::shared_ptr<Texture>& texture, const Color4 color,
                       const Poly2& poly, const Vec2& origin,
                       const Affine2& transform) {
    setTexture(texture);
    setColor(color);
    fill(poly, origin, transform);
}


#pragma mark -
#pragma mark Direct Mesh Drawing
/**
 * Draws the given mesh with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to
 * be drawn uniformly over the shape. The offset will be applied to the
 * vertex positions directly in world space.
 *
 * The drawing command will be determined by the mesh, and the triangulation
 * or lines determined by the mesh indices. The mesh vertices use their own
 * color values.  However, if tint is true, these values will be tinted
 * (i.e. multiplied) by the current active color.
 *
 * @param mesh      The sprite mesh
 * @param offset    The coordinate offset for the mesh
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const Mesh<SpriteVertex>& mesh, const Vec2& offset,
                           bool tint) {
    if (!mesh.indices.empty()) {
        setDrawMode(mesh.command);
        Affine2 transform;
        transform.translate(offset);
        prepare(mesh,transform,tint);
    }
}

/**
 * Draws the given mesh with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods. The texture no longer needs to be
 * drawn uniformly over the shape. The transform will be applied to the
 * vertex positions directly in world space.
 *
 * The drawing command will be determined by the mesh, and the triangulation
 * or lines determined by the mesh indices. The mesh vertices use their own
 * color values.  However, if tint is true, these values will be tinted
 * (i.e. multiplied) by the current active color.
 *
 * @param mesh      The sprite mesh
 * @param transform The coordinate transform
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const Mesh<SpriteVertex>& mesh,
                           const Affine2& transform, bool tint) {
    if (!mesh.indices.empty()) {
        setDrawMode(mesh.command);
        prepare(mesh,transform,tint);
    }
}

/**
 * Draws the vertices in a triangle fan with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to
 * be drawn uniformly over the shape. The offset will be applied to the
 * vertex positions directly in world space.
 *
 * The drawing command will be GL_TRIANGLES, and the triangulation will
 * be a mesh anchored on the first element. This method is ideal for
 * convex polygons.
 *
 * The mesh vertices use their own color values. However, if tint is true,
 * these values will be tinted (i.e. multiplied) by the current active
 * color.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param offset    The coordinate offset for the mesh
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const SpriteVertex* vertices, size_t size,
                           const Vec2& offset, bool tint) {
    if (size > 2) {
        setDrawMode(DrawMode::TRIANGLES);
        Affine2 transform;
        transform.translate(offset);
        prepare(vertices,size,transform,tint);
    }
}

/**
 * Draws the vertices in a triangle fan with the current texture and/or gradient.
 *
 * This method provides more fine tuned control over texture coordinates
 * that the other fill/outline methods.  The texture no longer needs to be
 * drawn uniformly over the shape. The transform will be applied to the
 * vertex positions directly in world space.
 *
 * The drawing command will be GL_TRIANGLES, and the triangulation will
 * be a mesh anchored on the first element. This method is ideal for
 * convex polygons.
 *
 * The mesh vertices use their own color values. However, if tint is true,
 * these values will be tinted (i.e. multiplied) by the current active
 * color.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param transform The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 */
void SpriteBatch::drawMesh(const SpriteVertex* vertices, size_t size,
                           const Affine2& transform, bool tint) {
    if (size > 2) {
        setDrawMode(DrawMode::TRIANGLES);
        prepare(vertices,size,transform,tint);
    }
}

#pragma mark -
#pragma mark Text Drawing
/**
 * Draws the text with the specified font at the given position.
 *
 * The position specifies the location of the left edge of the baseline of
 * the rendered text. The text will be displayed on only one line. For more
 * fine tuned control of text, you should use a {@link TextLayout}.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param position  The left edge of the text baseline
 */
void SpriteBatch::drawText(const std::string text,
                           const std::shared_ptr<Font>& font,
                           const Vec2& position) {
    std::unordered_map<size_t, std::shared_ptr<GlyphRun>> runs;
    font->getGlyphs(runs, text, position);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,Vec2::ZERO);
    }
}

/**
 * Draws the text with the specified font and transform
 *
 * The offset is measured from the left edge of the font baseline to identify
 * the origin of the rendered text. This origin is used when applying the
 * transform to the rendered text.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::string text,
                           const std::shared_ptr<Font>& font,
                           const Vec2& origin, const Affine2& transform) {
    std::unordered_map<size_t, std::shared_ptr<GlyphRun>> runs;
    font->getGlyphs(runs, text, -origin);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,transform);
    }
}

/**
 * Draws the text layout at the specified position
 *
 * The position specifies the location of the text layout origin. See the
 * specification of {@link TextLayout} for more information.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::shared_ptr<TextLayout>& text,
                           const Vec2& position) {
    std::unordered_map<size_t, std::shared_ptr<GlyphRun>> runs;
    text->getGlyphs(runs);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,position);
    }
}

/**
 * Draws the text layout with the given coordinate transform
 *
 * The transform is applied to the coordinate space of the {@link TextLayout}.
 *
 * By default, all text is rendered with white letters. However, this can
 * be tinted by the current sprite batch color to produce any color letters
 * required.
 *
 * @param text      The text to display
 * @param font      The font to render the text
 * @param origin    The rotational origin relative to the baseline
 * @param transform The coordinate transform
 */
void SpriteBatch::drawText(const std::shared_ptr<TextLayout>& text,
                           const Affine2& transform) {
    std::unordered_map<size_t, std::shared_ptr<GlyphRun>> runs;
    text->getGlyphs(runs);
    for(auto it = runs.begin(); it != runs.end(); ++it) {
        setTexture(it->second->texture);
        drawMesh(it->second->mesh,transform);
    }
}

#pragma mark -
#pragma mark Internal Helpers
/**
 * Records the current set of uniforms, freezing them.
 *
 * This method must be called whenever {@link prepare} is called for
 * a new set of uniforms.  It ensures that the vertices batched so far
 * will use the correct set of uniforms.
 */
void SpriteBatch::record() {
    Context* next = new Context(_context);
    _context->last = _indxSize;
    next->first = _indxSize;
    _history.push_back(_context);
    _context = next;
    _inflight = false;
}

/**
 * Deletes the recorded uniforms.
 *
 * This method is called upon flushing or cleanup.
 */
void SpriteBatch::unwind() {
    for(auto it = _history.begin(); it != _history.end(); ++it) {
        delete *it;
    }
    _history.clear();
}

/**
 * Sets the active uniform block to agree with the gradient and stroke.
 *
 * This method is called upon vertex preparation.
 *
 * @param context   The current uniform context
 */
void SpriteBatch::setUniformBlock(Context* context) {
    if (context->pushed || !(context->dirty & DIRTY_UNIBLOCK)) {
        return;
    }
    if (context->blockptr+1 >= _unifbuff->getBlockCount()) {
        flush();
    }
    float data[40];
    if (_scissor != nullptr) {
        _scissor->getData(data);
    } else {
        std::memset(data,0,16*sizeof(float));
    }
    if (_gradient != nullptr) {
        _gradient->std140(data+16);
    } else {
        std::memset(data+16,0,24*sizeof(float));
    }
    context->blockptr++;
    Uint8* bytes = reinterpret_cast<Uint8*>(&(data[0]));
    _unifbuff->pushBytes(context->blockptr,0,bytes,40*sizeof(float));
    context->pushed = true;
}

/**
 * Updates the shader with the current blur offsets
 *
 * Blur offsets depend upon the texture size. This method converts the
 * blur step into an offset in texture coordinates. It supports
 * non-square textures.
 *
 * If there is no active texture, the blur offset will be 0.
 *
 * @param texture   The texture to blur
 * @param step      The blur step in pixels
 */
void SpriteBatch::blurTexture(const std::shared_ptr<Texture>& texture, float step) {
    if (texture == nullptr) {
        _shader->pushVec2("uBlur", 0, 0);
        return;
    }
    Size size = texture->getSize();
    size.width  = step/size.width;
    size.height = step/size.height;
    _shader->pushVec2("uBlur",size.width,size.height);
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given rectangle to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * rectangle. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * @param rect  The rectangle to add to the buffer
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Rect& rect) {
    if (_vertSize+4 >= _vertMax ||  _indxSize+8 >= _indxMax) {
        flush();
    }
    
    Texture* texture = _context->texture.get();
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    Poly2 poly;
    makeRect(poly, rect, _context->draw == DrawMode::TRIANGLES);
    unsigned int vstart = _vertSize;
    int ii = 0;
    Uint32 clr = _color.getRGBA();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point;
        point.x = (point.x-rect.origin.x)/rect.size.width;
        point.y = (point.y-rect.origin.y)/rect.size.height;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x;
        _vertData[vstart+ii].gradcoord.y = point.y;
        _vertData[vstart+ii].color = clr;
        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given rectangle to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * rectangle. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 *
 * @param rect  The rectangle to add to the buffer
 * @param mat  	The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Rect& rect, const Affine2& mat) {
    if (_vertSize+4 > _vertMax ||  _indxSize+8 > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float tsmax, tsmin;
    float ttmax, ttmin;

    if (texture != nullptr) {
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    Poly2 poly;
    makeRect(poly, rect, _context->draw == DrawMode::TRIANGLES);

    unsigned int vstart = _vertSize;
    int ii = 0;
    Uint32 clr = _color.getRGBA();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point*mat;
        point.x = (point.x-rect.origin.x)/rect.size.width;
        point.y = (point.y-rect.origin.y)/rect.size.height;
        _vertData[vstart+ii].texcoord.x = point.x*tsmax+(1-point.x)*tsmin;
        _vertData[vstart+ii].texcoord.y = point.y*ttmax+(1-point.y)*ttmin;
        _vertData[vstart+ii].gradcoord.x = point.x;
        _vertData[vstart+ii].gradcoord.y = point.y;
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * @param poly  The polygon to add to the buffer
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly) {
    CUAssertLog(_context->draw == DrawMode::TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        return chunkify(poly,Mat4::IDENTITY);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    Uint32 clr = _color.getRGBA();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point;

        float s = point.x/twidth;
        float t = point.y/theight;

        _vertData[vstart+ii].texcoord.x = s*tsmax+(1-s)*tsmin;
        _vertData[vstart+ii].texcoord.y = t*ttmax+(1-t)*ttmin;
        _vertData[vstart+ii].gradcoord.x = s;
        _vertData[vstart+ii].gradcoord.y = t;
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly offset by the given vector.
 *
 * @param poly  The polygon to add to the buffer
 * @param off  	The offset to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly, const Vec2& off) {
    CUAssertLog(_context->draw == DrawMode::TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        Mat4 matrix;
        matrix.translate((Vec3)off);
        return chunkify(poly,matrix);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    Uint32 clr = _color.getRGBA();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = (*it)+off;
        
        float s = point.x/twidth;
        float t = point.y/theight;

        _vertData[vstart+ii].texcoord.x = s*tsmax+(1-s)*tsmin;
        _vertData[vstart+ii].texcoord.y = t*ttmax+(1-t)*ttmin;
        _vertData[vstart+ii].gradcoord.x = s;
        _vertData[vstart+ii].gradcoord.y = t;
        _vertData[vstart+ii].color = clr;

        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given polygon to the vertex buffer, but does not
 * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
 * polygon. This method will automatically flush if the maximum number
 * of vertices is reached.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 *
 * @param poly  The polygon to add to the buffer
 * @param mat  	The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Poly2& poly, const Affine2& mat) {
    CUAssertLog(_context->draw == DrawMode::TRIANGLES ?
                 poly.indices.size() % 3 == 0 :
                 poly.indices.size() % 2 == 0,
                "Polynomial has the wrong number of indices: %d", (int)poly.indices.size());
    if (poly.vertices.size() >= _vertMax || poly.indices.size()  >= _indxMax) {
        return chunkify(poly,mat);
    } else if (_vertSize+poly.vertices.size() > _vertMax ||
               _indxSize+poly.indices.size()  > _indxMax) {
        flush();
    }

    Texture* texture = _context->texture.get();
    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }
    
    setUniformBlock(_context);
    unsigned int vstart = _vertSize;
    int ii = 0;
    Uint32 clr = _color.getRGBA();
    for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
        Vec2 point = *it;
        _vertData[vstart+ii].position = point*mat;

        float s = point.x/twidth;
        float t = point.y/theight;

        _vertData[vstart+ii].texcoord.x = s*tsmax+(1-s)*tsmin;
        _vertData[vstart+ii].texcoord.y = t*ttmax+(1-t)*ttmin;
        _vertData[vstart+ii].gradcoord.x = s;
        _vertData[vstart+ii].gradcoord.y = t;
        _vertData[vstart+ii].color = clr;
        ii++;
    }
    
    int jj = 0;
    unsigned int istart = _indxSize;
    for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
        _indxData[istart+jj] = vstart+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments.  It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any size polygon.  This
 * is important for avoiding memory corruption.
 *
 * All vertices will be uniformly transformed by the transform matrix.
 *
 * @param poly  The polygon to add to the buffer
 * @param mat   The transform to apply to the vertices
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const Poly2& poly, const Affine2& mat) {
    Texture* texture = _context->texture.get();
    std::unordered_map<Uint32, Uint32> offsets;
    const std::vector<cugl::Vec2>* vertices = &(poly.vertices);
    const std::vector<Uint32>* indices = &(poly.indices);
    
    setUniformBlock(_context);
    int chunksize = _context->draw == DrawMode::TRIANGLES ? 3 : 2;
    unsigned int start = _vertSize;

    float twidth, theight;
    float tsmax, tsmin;
    float ttmax, ttmin;
    
    if (texture != nullptr) {
        twidth  = texture->getWidth();
        theight = texture->getHeight();
        tsmax = texture->getMaxS();
        tsmin = texture->getMinS();
        ttmax = texture->getMaxT();
        ttmin = texture->getMinT();
    } else {
        twidth  = poly.getBounds().size.width;
        theight = poly.getBounds().size.height;
        tsmax = 1.0f; tsmin = 0.0f;
        ttmax = 1.0f; ttmin = 0.0f;
    }

    Uint32 clr = _color.getRGBA();
    for(int ii = 0;  ii < indices->size(); ii += chunksize) {
        if (_indxSize+chunksize >= _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            offsets.clear();
        }
        
        for(int jj = 0; jj < chunksize; jj++) {
            auto search = offsets.find(indices->at(ii+jj));
            if (search != offsets.end()) {
                _indxData[_indxSize] = search->second;
            } else {
                Vec2 point = vertices->at(indices->at(ii+jj));
                _indxData[_indxSize] = _vertSize;
                _vertData[_vertSize].position = point*mat;
                
                float s = point.x/twidth;
                float t = point.y/theight;

                _vertData[_vertSize].texcoord.x = s*tsmax+(1-s)*tsmin;
                _vertData[_vertSize].texcoord.y = t*ttmax+(1-t)*ttmin;
                _vertData[_vertSize].gradcoord.x = s;
                _vertData[_vertSize].gradcoord.y = t;
                _vertData[_vertSize].color = clr;
                _vertSize++;
            }
            _indxSize++;
        }
    }

    _inflight = true;
    return (unsigned int)vertices->size()+start;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given mesh (both vertices and indices) to the
 * vertex buffer, but does not draw it.  You must call {@link #flush}
 * or {@link #end} to draw the complete mesh. This method will automatically
 * flush if the maximum number of vertices (or uniform blocks) is reached.
 *
 * @param mesh  The mesh to add to the buffer
 * @param mat   The transform to apply to the vertices
 * @param tint  Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const Mesh<SpriteVertex>& mesh, const Affine2& mat, bool tint) {
    CUAssertLog(mesh.isSliceable(), "Sprite batches only support sliceable meshes");
    if (mesh.vertices.size() >= _vertMax || mesh.indices.size() >= _indxMax) {
        return chunkify(mesh, mat, tint);
    } else if(_vertSize+mesh.vertices.size() > _vertMax || _indxSize+mesh.indices.size() > _indxMax) {
        flush();
    }
    
    setUniformBlock(_context);
    int ii = 0;
    tint = tint && _color != Color4::WHITE;
    for(auto it = mesh.vertices.begin(); it != mesh.vertices.end(); ++it) {
        _vertData[_vertSize+ii] = *it;
        _vertData[_vertSize+ii].position = it->position*mat;
        if (tint) {
            Color4f c = _vertData[_vertSize+ii].color;
            Uint32 r = round(_color.r*c.r);
            Uint32 g = round(_color.g*c.g);
            Uint32 b = round(_color.b*c.b);
            Uint32 a = round(_color.a*c.a);
            _vertData[_vertSize+ii].color.set(r,g,b,a);
        }
        ii++;
    }
    
    int jj = 0;
    for(auto it = mesh.indices.begin(); it != mesh.indices.end(); ++it) {
        _indxData[_indxSize+jj] = _vertSize+(*it);
        jj++;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments.  It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any size mesh. This is
 * important for avoiding memory corruption.
 *
 * @param mesh  The mesh to add to the buffer
 * @param mat   The transform to apply to the vertices
 * @param tint  Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const Mesh<SpriteVertex>& mesh, const Affine2& mat, bool tint) {
    std::unordered_map<Uint32, Uint32> offsets;
    
    setUniformBlock(_context);
    int chunksize = _context->draw == DrawMode::TRIANGLES ? 3 : 2;
    unsigned int start = _vertSize;
    
    for(int ii = 0;  ii < mesh.indices.size(); ii += chunksize) {
        if (_indxSize+chunksize >= _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            offsets.clear();
        }
        
        for(int jj = 0; jj < chunksize; jj++) {
            auto search = offsets.find(mesh.indices[ii+jj]);
            if (search != offsets.end()) {
                _indxData[_indxSize] = search->second;
            } else {
                _indxData[_indxSize] = _vertSize;
                _vertData[_vertSize] = mesh.vertices[ii+jj];
                _vertData[_vertSize].position *= mat;
                if (tint) {
                    Color4 shade(_vertData[_vertSize].color);
                    shade *= _color;
                    _vertData[_vertSize].color = shade;
                }
                _vertSize++;
            }
            _indxSize++;
        }
    }

    _inflight = true;
    return (unsigned int)(mesh.vertices.size()+start);
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method adds the given vertices to the vertex buffer. In addition,
 * this method adds the requisite indices to the index buffer to draw
 * these vertices as a triangle fan (anchored on the first element).
 * This method is ideal for meshes on convex polygons.
 *
 * With that said, this method does not actually draw the triangle fan.
 * You must call {@link #flush} or {@link #end} to draw the vertices.
 * This method will automatically flush if the maximum number of vertices
 * (or uniform blocks) is reached.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param mat       The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::prepare(const SpriteVertex* vertices, size_t size, const Affine2& mat, bool tint) {
    CUAssertLog(size >= 3, "Vertices do not form a triangle fan");
    if (size >= _vertMax || 3*(size-2) >= _indxMax) {
        return chunkify(vertices, size, mat, tint);
    } else if(_vertSize+size > _vertMax || _indxSize+3*(size-2) > _indxMax) {
        flush();
    }
    
    setUniformBlock(_context);
    int ii = 0;
    tint = tint && _color != Color4::WHITE;
    for(size_t kk = 0; kk < size; kk++) {
        _vertData[_vertSize+ii] = vertices[kk];
        _vertData[_vertSize+ii].position = vertices[kk].position*mat;
        if (tint) {
            Color4f c = _vertData[_vertSize+ii].color;
            Uint32 r = round(_color.r*c.r);
            Uint32 g = round(_color.g*c.g);
            Uint32 b = round(_color.b*c.b);
            Uint32 a = round(_color.a*c.a);
            _vertData[_vertSize+ii].color.set(r, g, b, a);
        }
        ii++;
    }
    
    int jj = 0;
    for(Uint32 kk = 2; kk < size; kk++) {
        _indxData[_indxSize+jj  ] = _vertSize;
        _indxData[_indxSize+jj+1] = _vertSize+kk-1;
        _indxData[_indxSize+jj+2] = _vertSize+kk;
        jj += 3;
    }
    
    _vertSize += ii;
    _indxSize += jj;
    _inflight = true;
    return ii;
}

/**
 * Returns the number of vertices added to the drawing buffer.
 *
 * This method is an alternate version of {@link #prepare} for the same
 * arguments. It runs slower (e.g. the compiler cannot easily optimize
 * the loops) but it is guaranteed to work on any number of vertices.
 * This is important for avoiding memory corruption.
 *
 * @param vertices  The vertices to draw as a triangle fan
 * @param size      The number of vertices in the fan
 * @param mat       The transform to apply to the vertices
 * @param tint      Whether to tint with the active color
 *
 * @return the number of vertices added to the drawing buffer.
 */
unsigned int SpriteBatch::chunkify(const SpriteVertex* vertices, size_t size, const Affine2& mat, bool tint) {
    setUniformBlock(_context);

    const int chunksize = 3;
    unsigned int start  = _vertSize;
    unsigned int anchor = start;

    bool fresh = true;
    for(int ii = 1;  ii < size; ii++) {
        if (_indxSize+chunksize > _indxMax || _vertSize+chunksize >= _vertMax) {
            flush();
            fresh = true;
        }
        
        if (fresh && size-ii >= 2) {
            anchor = _vertSize;
            _vertData[_vertSize++] = vertices[0];
            _vertData[_vertSize++] = vertices[ii];
        } else if (fresh) {
            anchor = _vertSize;
            _vertData[_vertSize++] = vertices[0];
            _vertData[_vertSize++] = vertices[size-2];
            _vertData[_vertSize++] = vertices[size-1];
            _indxData[_indxSize++] = anchor;
            _indxData[_indxSize++] = anchor+1;
            _indxData[_indxSize++] = anchor+2;
        } else {
            _vertData[_vertSize] = vertices[ii];
            _indxData[_indxSize++] = anchor;
            _indxData[_indxSize++] = _vertSize-1;
            _indxData[_indxSize++] = _vertSize;
            _vertSize++;
        }
    }

    _inflight = true;
    return (unsigned int)(size+start);
}
