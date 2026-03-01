//
//  CUGraphicsBase.cpp
//  Cornell University Game Library (CUGL)
//
//  This header includes the necessary includes to use either OpenGL or
//  Vulkan in CUGL. It also has several enums and primitive classes to
//  abstract a common interface between the two APIs.
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
//  Version: 3/30/25 (Vulkan integration)
//
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/util/CUHashtools.h>
#include <cugl/core/assets/CUJsonValue.h>

using namespace cugl;
using namespace cugl::graphics;

#pragma mark Shader Support
/**
 * Creates a specialization constant with no data
 */
GLSLConstant::GLSLConstant() :
type(GLSLType::NONE),
location(0),
size(0),
data(nullptr) {}

/**
 * Creates a copy of the given specialization constant
 *
 * @param def   The specialization constant to copy
 */
GLSLConstant::GLSLConstant(const GLSLConstant& def) {
    type = def.type;
    location = def.location;
    size = def.size;
    data = def.data;
}

/**
 * Assigns this object to be a copy of the given specialization constant.
 *
 * @param def   The specialization constant to copy
 *
 * @return a reference to this object for chaining
 */
GLSLConstant& GLSLConstant::operator=(const GLSLConstant& def) {
    type = def.type;
    location = def.location;
    size = def.size;
    data = def.data;
    return *this;
}

/**
 * Creates a zero-sized binding value
 */
VertexBindInfo::VertexBindInfo() :
stride(0),
instance(false) {}

/**
 * Creates a copy of the given bind information
 *
 * @param info  The binding information to copy
 */
VertexBindInfo::VertexBindInfo(const VertexBindInfo& info) {
    stride = info.stride;
    instance = info.instance;
}

/**
 * Assigns this object to be a copy of the given binding information.
 *
 * @param info  The binding information to copy
 *
 * @return a reference to this object for chaining
 */
VertexBindInfo& VertexBindInfo::operator=(const VertexBindInfo& info) {
    stride = info.stride;
    instance = info.instance;
    return *this;
}

/**
 * Creates a default attribute definition.
 */
AttributeDef::AttributeDef() :
type(GLSLType::NONE),
group(0),
location(-1),
offset(0) {}

/**
 * Creates a copy of the given attribute definition
 *
 * @param def   The attribute definition to copy
 */
AttributeDef::AttributeDef(const AttributeDef& def) {
    type = def.type;
    group = def.group;
    location = def.location;
    offset = def.offset;
}

/**
 * Assigns this object to be a copy of the given attribute definition.
 *
 * @param def   The attribute definition to copy
 *
 * @return a reference to this object for chaining
 */
AttributeDef& AttributeDef::operator=(const AttributeDef& def) {
    type = def.type;
    group = def.group;
    location = def.location;
    offset = def.offset;
    return *this;
}

/**
 * Creates a default uniform definition.
 */
UniformDef::UniformDef() :
type(GLSLType::NONE),
stage(ShaderStage::VERTEX),
location(-1),
size(0) {}

/**
 * Creates a copy of the given uniform definition
 *
 * @param def   The uniform definition to copy
 */
UniformDef::UniformDef(const UniformDef& def) {
    type = def.type;
    stage = def.stage;
    location = def.location;
    size = def.size;
}

/**
 * Assigns this object to be a copy of the given uniform definition.
 *
 * @param def   The uniform definition to copy
 *
 * @return a reference to this object for chaining
 */
UniformDef& UniformDef::operator=(const UniformDef& def) {
    type = def.type;
    stage = def.stage;
    location = def.location;
    size = def.size;
    return *this;
}

/**
 * Creates a default resource definition.
 */
ResourceDef::ResourceDef() :
stage(ShaderStage::VERTEX),
type(ResourceType::TEXTURE),
set(-1),
location(-1),
arraysize(1),
blocksize(0),
unbounded(false) {}

/**
 * Creates a copy of the given resource definition
 *
 * @param def   The resource definition to copy
 */
ResourceDef::ResourceDef(const ResourceDef& def) {
    stage = def.stage;
    type = def.type;
    set = def.set;
    location = def.location;
    arraysize = def.arraysize;
    blocksize = def.blocksize;
}

/**
 * Assigns this object to be a copy of the given resource definition.
 *
 * @param def   The resource definition to copy
 *
 * @return a reference to this object for chaining
 */
ResourceDef& ResourceDef::operator=(const ResourceDef& def) {
    stage = def.stage;
    type = def.type;
    set = def.set;
    location = def.location;
    arraysize = def.arraysize;
    blocksize = def.blocksize;
    return *this;
}

#pragma mark -
#pragma mark Stencil Support

/**
 * Creates default stencil test.
 */
StencilState::StencilState() :
failOp(StencilOp::KEEP),
passOp(StencilOp::KEEP),
depthFailOp(StencilOp::KEEP),
compareOp(CompareOp::ALWAYS),
compareMask(0xff),
writeMask(0xff),
reference(0x00) {
}

/**
 * Creates a stencil test for the given mode
 *
 * Note that it is not sufficient to create a stencil test object from a
 * mode.  Modes often set other parameters in the graphics pipeline. Hence
 * this constructor should never be used by the game developer.
 *
 * @param mode  The stencil test mode
 */
StencilState::StencilState(StencilMode mode) : StencilState() {
    set(mode);
}

/**
 * Creates a copy of the given stencil test
 *
 * @param state The stencil test to copy
 */
StencilState::StencilState(const StencilState& state) : StencilState()  {
    std::memcpy(this,&state,sizeof(StencilState));
}

/**
 * Assigns this object to be equivalent to the given stencil mode.
 *
 * Note that it is not sufficient to create a stencil test object from a
 * mode.  Modes often set other parameters in the graphics pipeline. Hence
 * this assignment should never be used by the game developer.
 *
 * @param mode  The stencil test mode
 *
 * @return a reference to this object for chaining
 */
StencilState& StencilState::set(StencilMode mode) {
    switch (mode) {
        case StencilMode::CLIP:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::KEEP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::NOT_EQUAL;
            break;
        case StencilMode::CLIP_CLEAR:
            failOp = StencilOp::ZERO;
            passOp = StencilOp::ZERO;
            depthFailOp = StencilOp::ZERO;
            compareOp = CompareOp::NOT_EQUAL;
            break;
        case StencilMode::MASK:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::KEEP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::EQUAL;
            break;
        case StencilMode::MASK_CLEAR:
            failOp = StencilOp::ZERO;
            passOp = StencilOp::ZERO;
            depthFailOp = StencilOp::ZERO;
            compareOp = CompareOp::EQUAL;
            break;
        case StencilMode::STAMP:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::INCR_CLAMP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::ALWAYS;
            break;
        case StencilMode::CLAMP:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::INCR_CLAMP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::EQUAL;
            break;
        case StencilMode::STAMP_EVENODD:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::INVERT;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::ALWAYS;
            break;
        case StencilMode::STAMP_NONZERO:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::INCR_WRAP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::ALWAYS;
            break;
        case StencilMode::CLEAR:
            failOp = StencilOp::ZERO;
            passOp = StencilOp::ZERO;
            depthFailOp = StencilOp::ZERO;
            compareOp = CompareOp::ALWAYS;
            break;
        case StencilMode::NONE:
        default:
            failOp = StencilOp::KEEP;
            passOp = StencilOp::KEEP;
            depthFailOp = StencilOp::KEEP;
            compareOp = CompareOp::ALWAYS;
            break;
    }
    return *this;
}

/**
 * Assigns this object to be equivalent to the given stencil mode.
 *
 * Note that it is not sufficient to create a stencil test object from a
 * mode.  Modes often set other parameters in the graphics pipeline. Hence
 * this assignment should never be used by the game developer.
 *
 * @param mode  The stencil test mode
 *
 * @return a reference to this object for chaining
 */
StencilState& StencilState::operator=(StencilMode mode) {
    return set(mode);
}

/**
 * Assigns this object to be a copy of the given stencil test
 *
 * @param state The stencil test to copy
 *
 * @return a reference to this object for chaining
 */
StencilState& StencilState::operator=(const StencilState& state) {
    std::memcpy(this,&state,sizeof(StencilState));
    return *this;
}

/**
 * Returns true if the given stencil test is equal to this object
 *
 * @param state The stencil test to compare
 *
 * @return true if the given stencil test is equal to this object
 */
bool StencilState::operator==(const StencilState& state) const noexcept {
    bool success = failOp == state.failOp;
    success = success && (passOp == state.passOp);
    success = success && (depthFailOp == state.depthFailOp);
    success = success && (compareOp == state.compareOp);
    success = success && (compareMask == state.compareMask);
    success = success && (writeMask == state.writeMask);
    success = success && (reference == state.reference);
    return success;
}

/**
 * Returns true if the given stencil test is not equal to this object
 *
 * @param state The stencil test to compare
 *
 * @return true if the given stencil test is not equal to this object
 */
bool StencilState::operator!=(const StencilState& state) const noexcept {
    return !(*this == state);
}

/**
 * Returns a hash code for this object.
 *
 * This hash is used to manage pipeline swaps in Vulkan. It has no use
 * in OpenGL.
 *
 * @return a hash code for this object.
 */
size_t StencilState::hash() {
    std::size_t result=0;
    hashtool::hash_combine(result,
                           static_cast<int>(failOp), static_cast<int>(passOp),
                           static_cast<int>(depthFailOp), static_cast<int>(compareOp),
                           compareMask, writeMask, reference);
    return result;
}


#pragma mark -
#pragma mark Color Blending Support
/**
 * Creates default color blend.
 */
BlendState::BlendState() :
srcFactor(BlendFactor::ONE),
dstFactor(BlendFactor::ZERO),
srcAlphaFactor(BlendFactor::ONE),
dstAlphaFactor(BlendFactor::ZERO),
colorEq(BlendEq::ADD),
alphaEq(BlendEq::ADD) {
    color.set(0,0,0,0);
}

/**
 * Creates a color blend for the given mode
 *
 * @param mode  The color blend mode
 */
BlendState::BlendState(BlendMode mode) : BlendState() {
    set(mode);
}

/**
 * Creates a copy of the given color blend
 *
 * @param state The color blend to copy
 */
BlendState::BlendState(const BlendState& state) : BlendState() {
    std::memcpy(this,&state,sizeof(BlendState));
}

/**
 * Assigns this object to be equivalent to the given color blend mode.
 *
 * @param mode  The color blend mode
 *
 * @return a reference to this object for chaining
 */
BlendState& BlendState::set(BlendMode mode) {
    switch (mode) {
        case BlendMode::ADDITIVE:
            srcFactor = BlendFactor::ONE;
            dstFactor = BlendFactor::ONE;
            srcAlphaFactor = BlendFactor::ONE;
            dstAlphaFactor = BlendFactor::ONE;
            colorEq = BlendEq::ADD;
            alphaEq = BlendEq::ADD;
            break;
        case BlendMode::MULTIPLICATIVE:
            srcFactor = BlendFactor::DST_COLOR;
            dstFactor = BlendFactor::ZERO;
            srcAlphaFactor = BlendFactor::DST_ALPHA;
            dstAlphaFactor = BlendFactor::ZERO;
            colorEq = BlendEq::ADD;
            alphaEq = BlendEq::ADD;
            break;
        case BlendMode::ALPHA:
            srcFactor = BlendFactor::SRC_ALPHA;
            dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
            srcAlphaFactor = BlendFactor::SRC_ALPHA;
            dstAlphaFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
            colorEq = BlendEq::ADD;
            alphaEq = BlendEq::ADD;
            break;
        case BlendMode::PREMULT:
            srcFactor = BlendFactor::ONE;
            dstFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
            srcAlphaFactor = BlendFactor::ONE;
            dstAlphaFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
            colorEq = BlendEq::ADD;
            alphaEq = BlendEq::ADD;
            break;
        case BlendMode::MAXIMUM:
            srcFactor = BlendFactor::ONE;
            dstFactor = BlendFactor::ONE;
            srcAlphaFactor = BlendFactor::ONE;
            dstAlphaFactor = BlendFactor::ONE;
            colorEq = BlendEq::MAX;
            alphaEq = BlendEq::MAX;
            break;
        case BlendMode::MINIMUM:
            srcFactor = BlendFactor::ONE;
            dstFactor = BlendFactor::ONE;
            srcAlphaFactor = BlendFactor::ONE;
            dstAlphaFactor = BlendFactor::ONE;
            colorEq = BlendEq::MIN;
            alphaEq = BlendEq::MIN;
            break;
        case BlendMode::SOLID:
        default:
            srcFactor = BlendFactor::ONE;
            dstFactor = BlendFactor::ZERO;
            srcAlphaFactor = BlendFactor::ONE;
            dstAlphaFactor = BlendFactor::ZERO;
            colorEq = BlendEq::ADD;
            alphaEq = BlendEq::ADD;
            break;
    }
    return *this;
}


/**
 * Assigns this object to be equivalent to the given color blend mode.
 *
 * @param mode  The color blend mode
 *
 * @return a reference to this object for chaining
 */
BlendState& BlendState::operator=(BlendMode mode) {
    return set(mode);
}

/**
 * Assigns this object to be a copy of the given color blend
 *
 * @param state The color blend to copy
 *
 * @return a reference to this object for chaining
 */
BlendState& BlendState::operator=(const BlendState& state) {
    std::memcpy(this,&state,sizeof(BlendState));
    return *this;
}

/**
 * Returns true if the given color blend is equal to this object
 *
 * @param state The color blend to compare
 *
 * @return true if the given color blend is equal to this object
 */
bool BlendState::operator==(const BlendState& state) const noexcept {
    bool success = srcFactor == state.srcFactor;
    success = success && (dstFactor == state.dstFactor);
    success = success && (srcAlphaFactor == state.srcAlphaFactor);
    success = success && (dstAlphaFactor == state.dstAlphaFactor);
    success = success && (colorEq == state.colorEq);
    success = success && (alphaEq == state.alphaEq);
    success = success && (color == state.color);
    return success;
}

/**
 * Returns true if the given color blend is not equal to this object
 *
 * @param state The color blend to compare
 *
 * @return true if the given color blend is not equal to this object
 */
bool BlendState::operator!=(const BlendState& state) const noexcept {
    return !(*this == state);
}

/**
 * Returns a hash code for this object.
 *
 * This hash is used to manage pipeline swaps in Vulkan. It has no use
 * in OpenGL.
 *
 * @return a hash code for this object.
 */
size_t BlendState::hash() {
    std::size_t result=0;
    hashtool::hash_combine(result,
                           static_cast<int>(srcFactor), static_cast<int>(dstFactor),
                           static_cast<int>(srcAlphaFactor), static_cast<int>(dstAlphaFactor),
                           static_cast<int>(colorEq), static_cast<int>(alphaEq),
                           color.r, color.g, color.b, color.a);
    return result;
}


#pragma mark -
#pragma mark JSON Parsing
/**
 * Returns the texture filter for the given name
 *
 * This function converts JSON directory entries into {@link TextureFilter}
 * values. If the name is invalid, it returns {@link TextureFilter#LINEAR}.
 *
 * @param name  The JSON name for the texture filter
 *
 * @return the texture filter for the given name
 */
TextureFilter cugl::graphics::json_filter(const std::string name) {
    if (name == "nearest") {
        return TextureFilter::NEAREST;
    } else if (name == "linear") {
        return TextureFilter::LINEAR;
    } else if (name == "nearest-nearest") {
        return TextureFilter::NEAREST_MIPMAP_NEAREST;
    } else if (name == "linear-nearest") {
        return TextureFilter::LINEAR_MIPMAP_NEAREST;
    } else if (name == "nearest-linear") {
        return TextureFilter::NEAREST_MIPMAP_LINEAR;
    } else if (name == "linear-linear") {
        return TextureFilter::LINEAR_MIPMAP_LINEAR;
    }
    return TextureFilter::LINEAR;
}

/**
 * Returns the texture wrap for the given name
 *
 * This function converts JSON directory entries into {@link TextureWrap}
 * values. If the name is invalid, it returns {@link TextureWrap#CLAMP}.
 *
 * @param name  The JSON name for the texture wrap
 *
 * @return the texture wrap for the given name
 */
TextureWrap cugl::graphics::json_wrap(const std::string name) {
    if (name == "clamp") {
        return TextureWrap::CLAMP;
    } else if (name == "repeat") {
        return TextureWrap::REPEAT;
    } else if (name == "mirrored") {
        return TextureWrap::MIRROR;
    } else if (name == "border") {
        return TextureWrap::BORDER;
    }
    return TextureWrap::CLAMP;
}

/**
 * Returns the blend mode for the given name
 *
 * This function converts JSON directory entries into {@link BlendMode} values.
 * If the name is invalid, it returns {@link BlendMode#PREMULT}.
 *
 * @param name  The JSON name for the blend mode
 *
 * @return the blend mode for the given name
 */
BlendMode cugl::graphics::json_blend_mode(std::string name) {
    if (name == "solid" || name == "opaque") {
        return BlendMode::SOLID;
    } else if (name == "additive") {
        return BlendMode::ADDITIVE;
    } else if (name == "multiplicative") {
    /** Later color values are mutliplied with previous ones. */
        return BlendMode::MULTIPLICATIVE;
    } else if (name == "alpha") {
    /** Later color values are blended using (non-premultiplied) alpha */
        return BlendMode::ALPHA;
    } else if (name == "premultiplied") {
    /** Later color values are blended using premultiplied alpha */
        return BlendMode::PREMULT;
    } else if (name == "maximum") {
    /** Color values are maximum of all applied color values. */
        return BlendMode::MAXIMUM;
    } else if (name == "minimum") {
        /** Color values are minimum of all applied color values. */
        return BlendMode::MINIMUM;
    }
    return BlendMode::PREMULT;
}


/**
 * Returns the color value for the given JSON entry
 *
 * A color entry is either a four-element integer array (values 0..255) or a
 * string. Any string should be a web color or a Tkinter color name.
 *
 * @param entry     The JSON entry for the color
 * @param backup    Default color to use on failure
 *
 * @return the color value for the given JSON entry
 */
cugl::Color4 cugl::graphics::json_color(JsonValue* entry, std::string backup) {
    Color4 result;
    result.set(backup);
    if (entry == nullptr) {
        return result;
    }
    
    if (entry->isString()) {
        result.set(entry->asString(backup));
    } else {
        CUAssertLog(entry->size() >= 4, "'color' must be a four element number array");
        result.r = std::max(std::min(entry->get(0)->asInt(0),255),0);
        result.g = std::max(std::min(entry->get(1)->asInt(0),255),0);
        result.b = std::max(std::min(entry->get(2)->asInt(0),255),0);
        result.a = std::max(std::min(entry->get(3)->asInt(0),255),0);
    }
    return result;
}


#pragma mark -
#pragma mark SDL Conversion
/**
 * Flips the pixel data in an SDL surface.
 *
 * This converts the SDL to OpenGL coordinates, to make it easier to use.
 *
 * @param surf  The SDL surface
 */
void cugl::graphics::sdl_flip_vertically(SDL_Surface* surf) {
    if (!surf || !surf->pixels) {
        return;
    }
    
    int pitch = surf->pitch;
    int h     = surf->h;
    Uint8* p  = (Uint8*)surf->pixels;

    // temp buffer for one row
    Uint8* tmp = (Uint8*)SDL_malloc(pitch);
    if (!tmp) {
        CULogError("Unable to allocate memory to flip image");
        return;
    }

    for (int y = 0; y < h / 2; ++y) {
        Uint8* top    = p + y * pitch;
        Uint8* bottom = p + (h - 1 - y) * pitch;

        SDL_memcpy(tmp,    top,    pitch);
        SDL_memcpy(top,    bottom, pitch);
        SDL_memcpy(bottom, tmp,    pitch);
    }

    SDL_free(tmp);
}
