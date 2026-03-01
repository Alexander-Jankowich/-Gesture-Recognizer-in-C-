//
//  CUGraphicsBase.h
//  Cornell University Game Library (CUGL)
//
//  This header defines the CUGL graphics
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
//  Version: 9/9/24 (Vulkan integration)
//
#ifndef __CU_GRAPHICS_BASE_H__
#define __CU_GRAPHICS_BASE_H__
#include <cugl/core/CUBase.h>
#include <cugl/core/math/CUColor4.h>
#include <type_traits>

#pragma mark Graphics Defines

/** Headless support */
#define CU_HEADLESS 0
/** Support for standard OpenGL   */
#define CU_OPENGL   1
/** Support for standard OpenGLES */
#define CU_OPENGLES 2
/** Support for Vulkan */
#define CU_VULKAN   3

// Define the OpenGL platforms
#if !defined(CU_USE_VULKAN) && !defined(CU_USE_OPENGL)
    #define CU_GRAPHICS_API CU_HEADLESS
#elif !defined(CU_USE_VULKAN)
    #if defined (SDL_PLATFORM_IOS)
        #define CU_GRAPHICS_API CU_OPENGLES
    #elif defined (SDL_PLATFORM_ANDROID)
        #define CU_GRAPHICS_API CU_OPENGLES
    #elif defined (SDL_PLATFORM_MACOS)
        #define CU_GRAPHICS_API CU_OPENGL
    #elif defined (SDL_PLATFORM_WIN32)
        #define CU_GRAPHICS_API CU_OPENGL
    #elif defined (SDL_PLATFORM_LINUX)
        #define CU_GRAPHICS_API CU_OPENGL
    #endif
#else
    #include <vulkan/vulkan.h>
    #define CU_GRAPHICS_API CU_VULKAN
#endif

namespace cugl {

// Forward declaration
class JsonValue;

    /**
     * The classes and functions needed to construct a graphics pipeline.
     *
     * Initially these were part of the core CUGL module. However, in the 
     * transition to Vulkan, we discovered that we need to factor this out.
     * This allows us to provide options for OpenGL, Vulkan, and even headless
     * clients.
     */
namespace graphics {

#pragma mark -
#pragma mark Shader Support
/**
 * An enum representing the supported GLSL types.
 *
 * This enum is used to connect C++ types to GLSL types. Note that not all
 * platforms support all types (particularly the user-defined {@link #BLOCK}
 * type.
 */
enum class GLSLType : int {
    /** Void type */
    NONE = 0,
    /** 32-bit signed int type */
    INT  = 1,
    /** 32-bit unsigned int type */
    UINT = 2,
    /** 32-bit float type */
    FLOAT = 3,
    /** Two-element float vector */
    VEC2 = 4,
    /** Three-element float vector */
    VEC3 = 5,
    /** Four-element float vector */
    VEC4 = 6,
    /** Two-element signed int vector */
    IVEC2 = 7,
    /** Three-element signed int vector */
    IVEC3 = 8,
    /** Four-element signed int vector */
    IVEC4 = 9,
    /** Two-element unsigned int vector */
    UVEC2 = 10,
    /** Three-element unsigned int vector */
    UVEC3 = 11,
    /** Four-element unsigned int vector */
    UVEC4 = 12,
    /** 2x2 float matrix */
    MAT2 = 13,
    /** 3x3 float matrix */
    MAT3 = 14,
    /** 4x4 float matrix */
    MAT4 = 15,
    /** 4 element float color */
    COLOR = 16,
    /** 32-bit int color */
    UCOLOR = 17,
    /** User-defined struct */
    BLOCK = 18,
};

/**
 * This class/struct represents a specialization constant in a shader
 *
 * CUGL supports specialization constants in both OpenGL and Vulkan, though
 * it implements them in OpenGL via meta-programming. A specialization constant
 * in OpenGL is prefixed by the macro CONSTANT_ID(n) where n is the id of the
 * constant, like this:
 *
 *     CONSTANT_ID(2) float time = 1.0;
 *
 * Setting a specialization constant in OpenGL will replace everything from the
 * CONSTANT_ID to the semicolon with a new declaration.
 *
 * Note that we only support built-in types (not structs) for specialization
 * constants in OpenGL.
 */
class GLSLConstant {
public:
    /**
     * The GLSL type of this constant
     */
    GLSLType type;
    
    /**
     * The constant location
     *
     * This is the value for layout(constant_id = n) in Vulkan and the macro
     * CONSTANT_ID(n) in OpenGL.
     */
    Uint32 location;
    
    /**
     * The size of this uniform.
     *
     * This value is necessary for type {@link GLSLType#BLOCK}. This value will
     * be computed for all other types.
     */
    size_t size;
    
    /**
     * The data associated with this specialization constant
     *
     * This pointer should be to a memory location with the data for the
     * specialization constant. We do not assume that the specialization
     * constant owns this information.
     */
    Uint8* data;
    
    /**
     * Creates a specialization constant with no data
     */
    GLSLConstant();
    
    /**
     * Creates a copy of the given specialization constant
     *
     * @param def   The specialization constant to copy
     */
    GLSLConstant(const GLSLConstant& def);
    
    /**
     * Assigns this object to be a copy of the given specialization constant.
     *
     * @param def   The specialization constant to copy
     *
     * @return a reference to this object for chaining
     */
    GLSLConstant& operator=(const GLSLConstant& def);
    
};

/**
 * This enum represents the different buffer access types
 *
 * A buffer can be any of {@link VertexBuffer}, {@link IndexBuffer},
 * {@link UniformBuffer} or {@link StorageBuffer}. These types are a
 * simplification of the buffer types provided by the various graphics APIs.
 * We find that they are sufficient for most of our purposes.
 */
enum class BufferAccess {
    /**
     * A write-only buffer that never changes.
     *
     * Buffers created with this type will only respond to an initial call to
     * load data. All other attempts to load to this buffer will fail.
     */
    STATIC = 0,
    
    /**
     * A write-only buffer that changes often.
     *
     * Buffers created with this type may be updated every animation frame.
     * Note, however, that it is not safe to update such a buffer more than
     * once a frame, as that can interfere with drawing calls. Once the
     * initial drawing call is made, all changes should cease.
     */
    DYNAMIC = 1,
    
    /**
     * A buffer that can be updated by a compute shader
     *
     * VULKAN ONLY: Buffers created with this type can be the output of a
     * compute shader, and then used as input to a graphics shader. However,
     * the results of the compute shader may not be accessed on the CPU side.
     */
    COMPUTE_ONLY = 2,
    
    /**
     * A buffer that can be updated by a compute shader and read on the CPU side
     *
     * VULKAN ONLY: Buffers created with this type can be the output of a
     * compute shader, and then used as input to a graphics shader. In addition,
     * it is possible to read the compute shader results on the CPU side.
     */
    COMPUTE_READ = 3,
    
    /**
     * The buffer has full compute and CPU access.
     *
     * Vulkan ONLY: Buffers created with this type can be the output of a
     * compute shader, and then used as input to a graphics shader. In addition,
     * they can be both read and write on the CPU side.
     */
    COMPUTE_ALL  = 4
};

/**
 * An enum representing the various stages of the graphics pipeline.
 *
 * This enum is used to refer to elements of a GLSL shader. In particular,
 * it references which file to look for a variable. Note that only the first
 * two stages are supported by OpenGL/OpenGLES.
 */
enum class ShaderStage : int {
    /** An undefined shader stage */
    UNDEFINED,
    /** The vertex shader stage (.vert) */
    VERTEX,
    /** The fragment shader stage (.frag) */
    FRAGMENT,
    /** The tesselation control shader stage (.tcl, VULKAN ONLY) */
    CONTROL,
    /** The tesselation evalation shader stage (.tel, VULKAN ONLY) */
    EVALUATION,
    /** The geometry shader stage (.geom, VULKAN ONLY) */
    GEOMETRY,
    /** The compute shader stage (.comp, VULKAN ONLY) */
    COMPUTE,
    /** All active graphics stages (excluding compute) */
    GRAPHICS,
    /** All active stages (including compute) */
    ALL,
};

// TODO: I think this type is internal. It does not appear to have external usage.
/**
 * This class/struct represents a binding information for a vertex buffer
 *
 * This class represents a binding point to connect a {@link VertexBuffer} to a
 * {@link GraphicsShader}. It allows us to swap out different vertex buffers
 * while the shader is active. A vertex buffer and a shader must have compatible
 * bind info in order for them to connect.
 */
class VertexBindInfo {
public:
    /** The stride of the vertex buffer */
    size_t stride;
    /** Whether this vertex buffer is instanced */
    bool instance;
    
    /**
     * Creates a zero-sized binding value
     */
    VertexBindInfo();
    
    /**
     * Creates a copy of the given bind information
     *
     * @param info  The binding information to copy
     */
    VertexBindInfo(const VertexBindInfo& info);
    
    /**
     * Assigns this object to be a copy of the given binding information.
     *
     * @param info  The binding information to copy
     *
     * @return a reference to this object for chaining
     */
    VertexBindInfo& operator=(const VertexBindInfo& info);
    
};

/**
 * This class represents a shader attribute variable.
 *
 * Attributes must be declared before the shader is compiled. The shader will
 * validate the GLSL code against declared attributes. Note that not all GLSL
 * types are supported on all platforms.
 *
 * Note that while GLSL supports the binding keyword for uniform buffers and
 * textures, there is no binding keyword for attributes. However, when we
 * specify an attribute, we still have to give it an binding value. The purpose
 * of this is for the organizing {@link VertexBuffer} objects. All attributes
 * in the same vertex buffer must share the same binding. Conversely, attributes
 * in distinct vertex buffers must have distinct bindings.
 *
 * In this class, we refer to this concept as a vertex `group` instead of a
 * binding, in order to make the difference clear. Technically, the group
 * number can be anything we choose. However, for performance reasons, we
 * prefer to start at 0 and assigning groups consecutively.
 */
class AttributeDef {
public:
    /** The GLSL type of this attribute */
    GLSLType type;
    /**
     * The vertex group for this attribute
     *
     * This is a user-defined value for grouping attributes that should be
     * stored in the same {@link VertexBuffer}.
     */
    Uint32 group;
    
    /**
     * The attribute location
     *
     * In OpenGL, setting this value to -1 causes the shader to determine this
     * location automatically. Vulkan requires that this value be set
     * explicitly.
     */
    Sint32 location;
    
    /** The stride offset of this attribute */
    size_t offset;
    
    /**
     * Creates a default attribute definition.
     */
    AttributeDef();
    
    /**
     * Creates a copy of the given attribute definition
     *
     * @param def   The attribute definition to copy
     */
    AttributeDef(const AttributeDef& def);
    
    /**
     * Assigns this object to be a copy of the given attribute definition.
     *
     * @param def   The attribute definition to copy
     *
     * @return a reference to this object for chaining
     */
    AttributeDef& operator=(const AttributeDef& def);
};

/**
 * This class represents a shader uniform variable.
 *
 * The meaning of a uniform depends on the graphics API being used. In OpenGL
 * a uniform includes any shader uniform that is not attached to a uniform
 * buffer (buffers are handled separately). In Vulkan, a uniform refers to a
 * push constant or a specialization constant.
 *
 * Uniforms must be declared before the shader is compiled. The shader will
 * validate the GLSL code against declared uniforms. Note that not all GLSL
 * types are supported on all platforms.
 */
class UniformDef {
public:
    /** The GLSL type of this uniform */
    GLSLType type;
    /** The shader stage where this uniform is located */
    ShaderStage stage;
    /**
     * The uniform location
     *
     * In OpenGL, setting this value to -1 causes the shader to determine this
     * location automatically. In Vulkan, this value is used to sort the push
     * constants so that we can determine their range.
     */
    Sint32 location;
    
    /**
     * The size of this uniform.
     *
     * This value is necessary for type {@link GLSLType#BLOCK}. This value will
     * be computed for all other types.
     */
    size_t size;
    
    /**
     * Creates a default uniform definition.
     */
    UniformDef();
    
    /**
     * Creates a copy of the given uniform definition
     *
     * @param def   The uniform definition to copy
     */
    UniformDef(const UniformDef& def);
    
    /**
     * Assigns this object to be a copy of the given uniform definition.
     *
     * @param def   The uniform definition to copy
     *
     * @return a reference to this object for chaining
     */
    UniformDef& operator=(const UniformDef& def);
    
};

/**
 * An enum representing a shader resource (buffer or texture).
 *
 * Resources are any value attached to a shader that cannot be represented
 * as GLSL primitive type. These primarily include textures and uniform
 * buffers. However, in the case of Vulkan, there is a wider array of types
 * supported. See the documentation for each enum value for the supported
 * types.
 */
enum class ResourceType : int {
    /**
     * A uniform buffer that is composed of just one block
     *
     * OpenGL and Vulkan: This type is supported in both APIs. It corresponds
     * to a uniform buffer that has only one block, which cannot change during
     * drawing. It corresponds to type uniform in GLSL.
     *
     * Note that this restriction is only on the number of blocks. It says
     * nothing about the read/write access to the buffer itself. It is possible
     * to have a dynamic (read/write) uniform buffer with just one block.
     */
    MONO_BUFFER = 0,
    /**
     * A uniform buffer that is composed of multiple blocks
     *
     * OpenGL and Vulkan: This type is supported in both APIs. It corresponds
     * to a buffer that can vary the block between drawing calls. It corresponds
     * to type uniform in GLSL.
     *
     * Note that this restriction is only on the number of blocks. It says
     * nothing about the read/write access to the buffer itself. It is possible
     * to have a static (write-once) uniform buffer with mutliple blocks.
     */
    MULTI_BUFFER = 1,
    /**
     * A storage buffer that is composed of just one block
     *
     * Vulkan ONLY: This type  corresponds to a storage buffer that has only one
     * block, which cannot change during shader executions. It is supported by
     * both graphics and compute shaders in Vulkan, but attempts to use this
     * resource with OpenGL will fail. It corresponds to type buffer in GLSL.
     *
     * Note that this restriction is only on the number of blocks. It says
     * nothing about the read/write access to the buffer itself. It is possible
     * to have a dynamic (read/write) storage buffer with just one block.
     */
    MONO_STORAGE = 2,
    /**
     * A storage buffer that is composed of multiple blocks
     *
     * Vulkan ONLY: This type  corresponds to a storage buffer that has only
     * multiple blocks that can vary between shader executions. It is supported
     * by both graphics and compute shaders in Vulkan, but attempts to use this
     * resource with OpenGL will fail. It corresponds to type buffer in GLSL.
     *
     * Note that this restriction is only on the number of blocks. It says
     * nothing about the read/write access to the buffer itself. It is possible
     * to have a static (write-once) storage buffer with mutliple blocks.
     */
    MULTI_STORAGE = 3,
    /**
     * A (2d) texture with sampler
     *
     * OpenGL and Vulkan: In OpenGL, this resource is a standard GL_TEXTURE_2D
     * object. In Vulkan, it is a combined image and sampler. It corresponds to
     * type sampler2D in GLSL.
     */
    TEXTURE = 4,
    /**
     * A (2d) texture array with sampler
     *
     * OpenGL and Vulkan: In OpenGL, this resource is a GL_TEXTURE_2D_ARRAY
     * object. In Vulkan, it is a combined image array and sampler (there is
     * only one sampler for the entire array). It corresponds to sampler2DArray
     * in GLSL.
     */
    TEXTURE_ARRAY = 5,
    /**
     * A sampler object
     *
     * Vulkan ONLY: Only Vulkan supports separating samplers from textures.
     * Attempts to use this resource with OpenGL will fail. It corresponds to
     * type sampler in GLSL.
     */
    SAMPLER = 6,
    /**
     * An image object
     *
     * Vulkan ONLY: An image object is a texture without a sampler. Only Vulkan
     * supports this separation. Attempts to use this resource with OpenGL will
     * fail. It corresponds to type texture2D in GLSL.
     */
    IMAGE = 7,
    /**
     * An image array object
     *
     * Vulkan ONLY: An image array is a texture array without a sampler. Only
     * Vulkan supports this separation. Attempts to use this resource with
     * OpenGL will fail. It corresponds to type texture2DArray in GLSL.
     */
    IMAGE_ARRAY = 8,
    /**
     * A vertex buffer that is the output to a compute shader
     *
     * Vulkan ONLY: This is a {@link VertexBuffer} with a compute type such
     * as {@link BufferAccess#COMPUTE_ONLY} or similar. It can be used as
     * a storage buffer in a compute shader, and a vertex buffer in a graphics
     * shader.
     */
    COMPUTE_VERTEX = 9,
    /**
     * A vertex buffer that is the output to a compute shader
     *
     * Vulkan ONLY: This is a {@link IndexBuffer} with a compute type such
     * as {@link BufferAccess#COMPUTE_ONLY} or similar. It can be used as
     * a storage buffer in a compute shader, and an index buffer in a graphics
     * shader.
     */
    COMPUTE_INDEX = 10,
    /**
     * A storage buffer that is the output to a compute shader
     *
     * Vulkan ONLY: This is a {@link StorageBuffer} with a compute type such
     * as {@link BufferAccess#COMPUTE_ONLY} or similar. It can be used as
     * a storage buffer in both a compute shader, and a graphics shader.
     *
     * This value should only be used for outputs to a compute shader. Inputs
     * should use type {@link MONO_STORAGE}.
     */
    COMPUTE_STORAGE = 11,
    /**
     * An image that is the output to a compute shader
     *
     * Vulkan ONLY: The is a {@link Image} with an access type such as
     * {@link ImageAccess#COMPUTE_ONLY} on similar. It can be used as
     * a storage image in a compute shader, and a sampled image in a graphics
     * shader. It corresponds to uniform image2D in GLSL.
     *
     * This value should only be used for outputs to a compute shader. Inputs
     * should use type {@link Image}.
     */
    COMPUTE_IMAGE = 12
};

/**
 * This class represents a shader resource variable.
 *
 * Resources are any value attached to a shader that cannot be represented
 * as GLSL primitive type. These primarily include textures and uniform
 * buffers. However, in the case of Vulkan, there is a wider array of types
 * supported. See {@link ResourceType} for the supported types.
 */

class ResourceDef {
public:
    /** The shader stage where this resource is located */
    ShaderStage stage;
    /** The resource type */
    ResourceType type;
    /**
     * The descriptor set for this resource
     *
     * Descriptor sets are Vulkan only. This should be set to -1 for OpenGL.
     */
    Sint32 set;
    
    /**
     * The resource location
     *
     * In OpenGL, setting this value to -1 causes the shader to determine this
     * location automatically. In Vulkan, this value is the binding location
     * in the relevant descriptor set.
     */
    Sint32 location;
    
    /**
     * The number of elements to attach to this resource.
     *
     * This value defines the size of a texture or buffer array in a GLSL
     * shader. It is only supported by Vulkan. OpenGL will ignore this value.
     */
    Uint32 arraysize;
    
    /**
     * Whether to make this resource an unbounded array.
     *
     * If this value is true, then the value arraysize will be replaced with
     * the device maximum. This can only be applied to the last resource in
     * a set. For all other resources, arraysize will be preserved.
     *
     * This feature is only supported by Vulkan, and not all versions of Vulkan
     * support it. The Vulkan version must be 1.2 or greater, or support the
     * descriptor indexing extension.
     */
    bool unbounded;
    
    /**
     * The blocksize of this resource.
     *
     * This value is only relevant for dynamic uniform and storage buffers. It
     * is ignored for all other resources.
     */
    size_t blocksize;
    
    /**
     * Creates a default resource definition.
     */
    ResourceDef();
    
    /**
     * Creates a copy of the given resource definition
     *
     * @param def   The resource definition to copy
     */
    ResourceDef(const ResourceDef& def);
    
    /**
     * Assigns this object to be a copy of the given resource definition.
     *
     * @param def   The resource definition to copy
     *
     * @return a reference to this object for chaining
     */
    ResourceDef& operator=(const ResourceDef& def);
};


#pragma mark -
#pragma mark Drawing Commands
/**
 * An enum representing the current drawing command.
 *
 * We only support drawing commands supported by both OpenGL and Vulkan. This
 * enum is an abstraction of contants like `GL_TRIANGLES` or the Vulkan
 * equivalent `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`.
 */
enum class DrawMode : int {
    /** An undefined draw mode */
    UNDEFINED      = 0,
    /** Draw a sequence of points */
    POINTS         = 1,
    /** Draw a sequence of line segment pairs */
    LINES          = 2,
    /** Draw an OpenGL line segment strip */
    LINE_STRIP     = 3,
    /** Draw a sequence of triangles defined as vertex triplets */
    TRIANGLES      = 4,
    /** Draw an OpenGL triangle strip */
    TRIANGLE_STRIP = 5,
    /** Draw an OpenGL triangle fan */
    TRIANGLE_FAN   = 6
};

/**
 * An enum defining the current facing of triangles/polygons.
 *
 * This enum is an abstraction of constants like `GL_CCW` or the Vulkan
 * equivalent `VK_FRONT_FACE_COUNTER_CLOCKWISE`.
 */
enum class FrontFace : int {
    /** Counter-clockwise triangles are forward facing */
    CCW = 0,
    /** Clockwise triangles are forward facing */
    CW = 1
};

/**
 * An enum representing the current cull mode.
 *
 * Culling is used to remove non-visible triangles. This enum is an abstraction
 * of constants like `GL_BACK` or the Vulkan equivalent `VK_CULL_MODE_BACK_BIT`.
 * Note that culling only applies to triangles, not points or lines.
 */
enum class CullMode : int {
    /** No triangle culling is performed */
    NONE = 0,
    /** Forward facing triangles are culled */
    FRONT = 1,
    /** Backwards facing triangles are culled */
    BACK = 2,
    /** All triangles are culled (but lines and points are drawn) */
    FRONT_BACK = 3
};


#pragma mark -
#pragma mark Stencil Support
/**
 * An enum representing comparison operators for depth and stencil tests.
 *
 * This enum is an abstraction of constants like `GL_NEVER` or the Vulkan
 * equivalent `VK_COMPARE_OP_NEVER`.
 */
enum class CompareOp : int {
    /** The comparison test always fails */
    NEVER = 0,
    /** The comparison tests succeeds if reference < test */
    LESS = 1,
    /** The comparison tests succeeds if reference == test */
    EQUAL = 2,
    /** The comparison tests succeeds if reference <= test */
    LESS_OR_EQUAL = 3,
    /** The comparison tests succeeds if reference > test */
    GREATER = 4,
    /** The comparison tests succeeds if reference != test */
    NOT_EQUAL = 5,
    /** The comparison tests succeeds if reference >= test */
    GREATER_OR_EQUAL = 6,
    /** The comparison test always succeeds */
    ALWAYS = 7,
};

/**
 * An enum of predefined stencil tests.
 *
 * While it is possible to define custom stencil tests via {@link StencilState},
 * there are some fairly common stencil tests that most applications will use.
 * This enum represents those tests to make it easier on the developer to use
 * the stencil buffer.
 */
enum class StencilMode : int {
    /**
     * Disables any stencil effects.
     *
     * This effect directs the pipeline to ignore the stencil buffer when
     * drawing. However, it does not clear the contents.
     */
    NONE = 0,
    
    /**
     * Restricts all drawing to a stenciled region.
     *
     * In order for this effect to do anything, you must have created a stencil
     * region with {@link StencilMode#STAMP} or one of its variants. This
     * effect will process the drawing commands normally, but restrict all
     * drawing to the stenciled region. This can be used to draw non-convex
     * shapes by making a stencil and drawing a rectangle over the stencil.
     */
    CLIP = 1,
    
    /**
     * Restricts all drawing to the stenciled region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link StencilMode#STAMP} or one of its variants.
     * This effect will process the drawing commands normally, but restrict all
     * drawing to the stenciled region. This can be used to draw non-convex
     * shapes by making a stencil and drawing a rectangle over the stencil.
     *
     * This mode is different from {@link StencilMode#CLIP} in that it will
     * zero out the pixels it draws in the stencil buffer, effectively removing
     * them from the stencil region. In many applications, this is a way to
     * clear the stencil buffer once it is no longer needed (especially if
     * depth and stencil are unified in the same attachment).
     */
    CLIP_CLEAR = 2,
    
    /**
     * Prohibits all drawing to the stenciled region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link StencilMode#STAMP} or one of its variants.
     * This effect will process the drawing commands normally, but reject any
     * attempts to draw to the stenciled region. This can be used to draw shape
     * borders on top of a solid shape.
     */
    MASK = 3,
    
    /**
     * Prohibits all drawing to the stenciled region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link StencilMode#STAMP} or one of its variants.
     * This effect will process the drawing commands normally, but reject any
     * attempts to draw to the stenciled region. This can be used to draw shape
     * borders on top of a solid shape.
     *
     * This mode is different from {@link StencilMode#CLIP} in that it will
     * zero out the pixels it draws in the stencil buffer, effectively removing
     * them from the stencil region. In many applications, this is a way to
     * clear the stencil buffer once it is no longer needed (especially if
     * depth and stencil are unified in the same attachment).
     */
    MASK_CLEAR = 4,
    
    /**
     * Adds a stencil region the buffer
     *
     * This effect will not have any immediate visible effects. Instead it
     * creates a stencil region for modes such as {@link StencilMode#CLIP},
     * {@link StencilMode#MASK}, and the like.
     *
     * The shapes are drawn to the stencil buffer additively to the buffer. It
     * ignores path orientation, and does not support holes. This allows the
     * effect to implement a nonzero fill rule on the shapes. The primary
     * application of this effect is to create stencils from extruded paths so
     * that overlapping sections are not drawn twice (which has negative
     * effects on alpha blending).
     */
    STAMP = 5,
    
    /**
     * Limits drawing so that each pixel is updated once.
     *
     * This effect is a variation of {@link StencilMode#STAMP} that also
     * draws as it writes to the stencil buffer. This guarantees that each
     * pixel is updated exactly once. This is used by extruded paths so that
     * overlapping sections are not drawn twice (which has negative
     * effects on alpha blending).
     */
    CLAMP = 6,
    
    /**
     * Adds a stencil region the buffer
     *
     * This effect will not have any immediate visible effects. Instead it is
     * a variation of {@link StencilMode#STAMP} that creates a stencil region
     * for modes such as {@link StencilMode#CLIP}, {@link StencilMode#MASK},
     * and the like.
     *
     * The shapes are drawn to the stencil buffer using an even-odd fill rule.
     * It respects orienation and can be used to draw a stencil for a polygon
     * with holes. This has the disadvantage that stamps drawn on top of each
     * other have an "erasing" effect. However, it does not have the polygon
     * limitations that {@link StencilMode#STAMP_NONZERO} does.
     */
    STAMP_EVENODD = 7,
    
    /**
     * Adds a stencil region the buffer
     *
     * This effect will not have any immediate visible effects. Instead it is
     * a variation of {@link StencilMode#STAMP} that creates a stencil region
     * for modes such as {@link StencilMode#CLIP}, {@link StencilMode#MASK},
     * and the like.
     *
     * The shapes are drawn to the stencil buffer using a nonzero fill rule.
     * It respects orienation and can be used to draw a stencil for a polygon
     * with holes. This has the advantage that (unlike an even-odd fill rule)
     * stamps are additive and can be drawn on top of each other. However, the
     * size of the stencil buffer means that more than 256 overlapping polygons
     * of the same orientation will cause unpredictable effects.
     */
    STAMP_NONZERO = 8,
    
    /**
     * Erases pixels from the stenciled region.
     *
     * This effect will not draw anything to the screen. Instead, it will
     * only draw to the stencil buffer directly. Any pixel drawn will be
     * zeroed in the buffer, removing it from the stencil region. The modes
     * {@link StencilMode#CLIP_CLEAR} and {@link StencilMode#MASK_CLEAR} are
     * built upon this idea. However, this mode allows you to clip without
     * drawing anything.
     */
    CLEAR = 9,
};

/**
 * An enum representing stencil operations.
 *
 * This enum is an abstraction of constants like `GL_KEEP` or the Vulkan
 * equivalent `VK_STENCIL_OP_KEEP`.
 */
enum class StencilOp : int {
    /** Keep the current value of the stencil buffer */
    KEEP = 0,
    /** Set the stencil buffer value to 0 */
    ZERO = 1,
    /** Replace the stencil buffer value with the reference value */
    REPLACE = 2,
    /** Increments the stencil buffer value, clamping to the maximum */
    INCR_CLAMP = 3,
    /** Decrements the stencil buffer value, clamping to the minimum */
    DECR_CLAMP = 4,
    /** Bitwise inverts the stencil buffer value */
    INVERT = 5,
    /** Increments the stencil buffer value, wrapping around on overflow */
    INCR_WRAP = 6,
    /** Decrements the stencil buffer value, wrapping around on underflow */
    DECR_WRAP = 7,
};

/**
 * This class represents the state of the current stencil test
 *
 * This class is used for fine-tuned control of the stencil buffer. For most
 * application {@link StencilMode} should be adequate. Note that changing any
 * of these values in Vulkans requires a pipeline swap.
 */
class StencilState {
public:
    /** The action performed when the stencil test fails */
    StencilOp   failOp;
    /** The action performed when the stencil test passes */
    StencilOp   passOp;
    /** The action performed when the stencil test passes, but the depth test fails */
    StencilOp   depthFailOp;
    /** The comparison operator fo the stencil test */
    CompareOp   compareOp;
    /** The reference value used by the comparison operation */
    Uint8       reference;
    /** The mask to apply to the value before comparison */
    Uint8       compareMask;
    /** The mask to apply to the value written by the stencil operation */
    Uint8       writeMask;
    
    /**
     * Creates default stencil test.
     */
    StencilState();
    
    /**
     * Creates a stencil test for the given mode
     *
     * Note that it is not sufficient to create a stencil test object from a
     * mode.  Modes often set other parameters in the graphics pipeline. Hence
     * this constructor should never be used by the game developer.
     *
     * @param mode  The stencil test mode
     */
    StencilState(StencilMode mode);
    
    /**
     * Creates a copy of the given stencil test
     *
     * @param state The stencil test to copy
     */
    StencilState(const StencilState& state);
    
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
    StencilState& set(StencilMode mode);
    
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
    StencilState& operator=(StencilMode mode);
    
    /**
     * Assigns this object to be a copy of the given stencil test
     *
     * @param state The stencil test to copy
     *
     * @return a reference to this object for chaining
     */
    StencilState& operator=(const StencilState& state);
    
    /**
     * Returns true if the given stencil test is equal to this object
     *
     * @param state The stencil test to compare
     *
     * @return true if the given stencil test is equal to this object
     */
    bool operator==(const StencilState& state) const noexcept;
    
    /**
     * Returns true if the given stencil test is not equal to this object
     *
     * @param state The stencil test to compare
     *
     * @return true if the given stencil test is not equal to this object
     */
    bool operator!=(const StencilState& state) const noexcept;
    
    /**
     * Returns a hash code for this object.
     *
     * This hash is used to manage pipeline swaps in Vulkan. It has no use
     * in OpenGL.
     *
     * @return a hash code for this object.
     */
    size_t hash();
};


#pragma mark -
#pragma mark Color Blending Support
/**
 * An enum of predefined color blend models
 *
 * While it is possible to define custom color blending via {@link BlendState},
 * there are some fairly common blend modes that most applications will use.
 * This enum represents those modes to make it easier on the developer to use
 * the color blending.
 */
enum class BlendMode : int {
    /** Later color values replace previous ones. */
    SOLID    = 0,
    /** Later color values are added to previous ones. */
    ADDITIVE = 1,
    /** Later color values are mutliplied with previous ones. */
    MULTIPLICATIVE = 2,
    /** Later color values are blended using (non-premultiplied) alpha */
    ALPHA   = 3,
    /** Later color values are blended using premultiplied alpha */
    PREMULT = 4,
    /** Color values are maximum of all applied color values. */
    MAXIMUM = 5,
    /** Color values are minimum of all applied color values. */
    MINIMUM = 6
    // TODO: SATURATE?  No one really uses this as far as I can tell.
};

/**
 * An enum representing the color blending factor.
 *
 * This enum is an abstraction of constants like `GL_SRC_ALPHA` or the Vulkan
 * equivalent `VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA`. Note that blend factors
 * have different meanings according to whether they are an RGB blend factor
 * or an alpha blend factor.
 *
 * In the documentation for each value, Rs is the red value of the source,
 * while Rd is the red value of the destination (and similarly G green, B blue,
 * and A alpha). Rc is the red of the constant color, which is a color set with
 * the graphics pipeline.
 */
enum class BlendFactor : int {
    /** An RGB factor (0,0,0), and an alpha factor of 0 **/
    ZERO    = 0,
    /** An RGB factor (1,1,1), and an alpha factor of 1 **/
    ONE     = 1,
    /** An RGB factor (Rs,Gs,Bs), and an alpha factor of As **/
    SRC_COLOR = 2,
    /** An RGB factor (1-Rs,1-Gs,1-Bs), and an alpha factor of 1-As **/
    ONE_MINUS_SRC_COLOR = 3,
    /** An RGB factor (Rd,Gd,Bd), and an alpha factor of Ad **/
    DST_COLOR = 4,
    /** An RGB factor (1-Rd,1-Gd,1-Bd), and an alpha factor of 1-Ad **/
    ONE_MINUS_DST_COLOR = 5,
    /** An RGB factor (As,As,As), and an alpha factor of As **/
    SRC_ALPHA = 6,
    /** An RGB factor (1-As,1-As,1-As), and an alpha factor of 1-As **/
    ONE_MINUS_SRC_ALPHA = 7,
    /** An RGB factor (Ad,Ad,Ad), and an alpha factor of Ad **/
    DST_ALPHA = 8,
    /** An RGB factor (1-Ad,1-Ad,1-Ad), and an alpha factor of 1-Ad **/
    ONE_MINUS_DST_ALPHA = 9,
    /** An RGB factor (Rc,Gc,Bc), and an alpha factor of Ac **/
    CONSTANT_COLOR = 10,
    /** An RGB factor (1-Rc,1-Gc,1-Bc), and an alpha factor of 1-Ac **/
    ONE_MINUS_CONSTANT_COLOR = 11,
    /** An RGB factor (Ac,Ac,Ac), and an alpha factor of Ac **/
    CONSTANT_ALPHA = 12,
    /** An RGB factor (1-Ac,1-Ac,1-Ac), and an alpha factor of 1-Ac **/
    ONE_MINUS_CONSTANT_ALPHA = 13,
    /** An RGB factor (f,f,f) where f = min(As,1-Ad), and an alpha factor of 1 **/
    SRC_ALPHA_SATURATE = 14
};

/**
 * An enum representing the color blending equation.
 *
 * This enum is an abstraction of constants like `GL_FUNC_ADD` or the Vulkan
 * equivalent `VK_BLEND_OP_ADD`.
 */
enum class BlendEq : int {
    /** Adds the scaled destination to the scaled source */
    ADD = 0,
    /** Subtracts the scaled destination from the scaled source */
    SUBTRACT = 1,
    /** Subtracts the scaled source from the scaled destination */
    REVERSE_SUBTRACT = 2,
    /**
     * Computes the min of each color value of the source and destination.
     *
     * Note that this equation ignores the color blend factor.
     */
    MIN = 3,
    /**
     * Computes the max of each color value of the source and destination.
     *
     * Note that this equation ignores the color blend factor.
     */
    MAX = 4,
};

/**
 * This class represents the state of the current color blending operations
 *
 * This class is used for fine-tuned control of the color blending. For most
 * application {@link BlendMode} should be adequate. Note that changing any
 * of these values in Vulkans requires a pipeline swap.
 */
class BlendState {
public:
    /** The blend factor for the source RGB color */
    BlendFactor srcFactor;
    /** The blend factor for the destination RGB color */
    BlendFactor dstFactor;
    /** The blend factor for the source alpha value */
    BlendFactor srcAlphaFactor;
    /** The blend factor for the destination alpha value */
    BlendFactor dstAlphaFactor;
    /** The blend equation for the RGB color values */
    BlendEq colorEq;
    /** The blend equation for the alpha value */
    BlendEq alphaEq;
    /** The constant color for this blend mode */
    Color4f color;
    
    /**
     * Creates default color blend.
     */
    BlendState();
    
    /**
     * Creates a color blend for the given mode
     *
     * @param mode  The color blend mode
     */
    BlendState(BlendMode mode);
    
    /**
     * Creates a copy of the given color blend
     *
     * @param state The color blend to copy
     */
    BlendState(const BlendState& state);
    
    /**
     * Assigns this object to be equivalent to the given color blend mode.
     *
     * @param mode  The color blend mode
     *
     * @return a reference to this object for chaining
     */
    BlendState& set(BlendMode mode);
    
    /**
     * Assigns this object to be equivalent to the given color blend mode.
     *
     * @param mode  The color blend mode
     *
     * @return a reference to this object for chaining
     */
    BlendState& operator=(BlendMode mode);
    
    /**
     * Assigns this object to be a copy of the given color blend
     *
     * @param state The color blend to copy
     *
     * @return a reference to this object for chaining
     */
    BlendState& operator=(const BlendState& state);
    
    /**
     * Returns true if the given color blend is equal to this object
     *
     * @param state The color blend to compare
     *
     * @return true if the given color blend is equal to this object
     */
    bool operator==(const BlendState& state) const noexcept;
    
    /**
     * Returns true if the given color blend is not equal to this object
     *
     * @param state The color blend to compare
     *
     * @return true if the given color blend is not equal to this object
     */
    bool operator!=(const BlendState& state) const noexcept;
    
    /**
     * Returns a hash code for this object.
     *
     * This hash is used to manage pipeline swaps in Vulkan. It has no use
     * in OpenGL.
     *
     * @return a hash code for this object.
     */
    size_t hash();
};

#pragma mark -
#pragma mark Texture Support
/**
 * An enum defining the access rules for and image.
 *
 * Access rules apply to the methods {@link Image#set} and {@link Image#get}.
 * They also indicate whether the image can be used in a compute shader
 * or not. It is assumed that all images can be sampled by a shader.
 */
enum class ImageAccess {
    /**
     * The image can only be used in shader.
     *
     * The image does not support read or write access. This format is
     * useful for images that associated with a {@link FrameBuffer} and
     * support post-processing effects.
     */
    SHADER_ONLY = 0,
    
    /**
     * This image is read only on the CPU side.
     *
     * The image supports {@link Image#get} in addition to being accessed
     * by a shader.
     */
    READ_ONLY   = 1,
    
    /**
     * The image is write-only on the CPU side.
     *
     * While the image can be accessed by a shader, no data can be loaded
     * into the image after it is initialized. However, it is possible to
     * add new data on the CPU side via {@link Image#set}.
     */
    WRITE_ONLY  = 2,
    
    /**
     * The image supports full CPU read and write access
     *
     * Both methods {@link Image#set} and {@link Image#get} are supported.
     */
    READ_WRITE  = 3,
    
    /**
     * The image can be accessed by a computer shader but has no CPU access.
     *
     * Vulkan ONLY: Neither methods {@link Image#set} nor {@link Image#get}
     * are supported. However, the image can be updated by a compute shader.
     */
    COMPUTE_ONLY = 4,
    
    /**
     * The image can be accessed by a computer shader and read from the CPU.
     *
     * Vulkan ONLY: The method {@link Image#get} is supported, but the image
     * can only be updated by a compute shader.
     */
    COMPUTE_READ = 5,
    
    /**
     * The image has full compute and CPU access.
     *
     * Vulkan ONLY: The methods {@link Image#get} and {@link Image#set} are
     * both supported. In addition, the image can be updated by a compute
     * shader.
     */
    COMPUTE_ALL  = 6,
};

/**
 * An enum listing the possible texture pixel formats.
 *
 * This enum defines the texel formats supported by CUGL. As CUGL must support
 * both OpenGLES and Vulkan, it only supports a small subset of formats. CUGL
 * will convert these values into the format for the appropriate platform.
 */
enum class TexelFormat {
    /**
     * An undefined/uninitialized texel format
     *
     * VULKAN ONLY: This format will fail in OpenGL
     */
    UNDEFINED = 0,
    
    /**
     * A texel format directly defined by the developer
     *
     * VULKAN ONLY: This format will fail in OpenGL
     */
    CUSTOM = 1,
    
    /**
     * RGB with alpha transparency, 8 bits per channel
     *
     * This is the default format
     */
    COLOR_RGBA = 2,
    
    /** RGB with alpha transparency, 4 bits per channel */
    COLOR_RGBA4 = 3,
    
    /**
     * BGRA (alpha with blue as the highest bit), 8 bits per channel
     *
     * While supported by both OpenGL and Vulkan, some Vulkan implementations
     * specifically need this format.
     */
    COLOR_BGRA = 4,
    
    /** BGRA (alpha with blue as the highest bit), 4 bits per channel */
    COLOR_BGRA4 = 5,
    
    /** RGB with no alpha, 8 bits per channel */
    COLOR_RGB = 6,
    
    /** RGB with no alpha, 4 bits per channel */
    COLOR_RGB4 = 7,
    
    /** Two color format (red-green) with 8 bits per channel */
    COLOR_RG = 8,
    
    /** Two color format (red-green) with 4 bits per channel */
    COLOR_RG4 = 9,
    
    /** A monocolor format (red), 8 bits per channel */
    COLOR_RED = 10,
    
    /**
     * A depth-only format
     *
     * The data type (for the only attribute) is float. On most platforms
     * this is 32 bit, though some platforms only use 16 bits.
     */
    DEPTH = 11,
    
    /**
     * A stencil only format
     *
     * The data type (for the only attribute) is an 8 bit integer.
     */
    STENCIL = 12,
    
    /**
     * A combined depth and stencil format
     *
     * This format gives 24 bytes to depth and 8 bits to the stencil.
     */
    DEPTH_STENCIL = 13
};

/**
 * An enum representing the texture filter options.
 *
 * This enum is an abstraction of constants like `GL_LINEAR` or the Vulkan
 * equivalent `VK_FILTER_LINEAR`. We use the OpenGL naming scheme as it allows
 * us to combine mipmap and texture filter into a single enumeration.
 */
enum class TextureFilter : int {
    /** Filtering selects the nearest value */
    NEAREST = 0,
    /** Filtering selects a linearly interpolated value */
    LINEAR  = 1,
    /** Filtering selects the nearest value from the nearest mipmap */
    NEAREST_MIPMAP_NEAREST = 2,
    /** Filtering selects a linearly interpolated value from the nearest mipmap */
    LINEAR_MIPMAP_NEAREST  = 3,
    /** Filtering selects the nearest value from a linearly interpolated mipmap */
    NEAREST_MIPMAP_LINEAR  = 4,
    /** Filtering selects a linearly interpolated value from a linearly interpolated mipmap */
    LINEAR_MIPMAP_LINEAR   = 5,
};

/**
 * An enum represeting a texture wrap parameter.
 *
 * This enum is an abstraction of constants like `GL_REPEAT` or the Vulkan
 * equivalent `VK_SAMPLER_ADDRESS_MODE_REPEAT`.
 */
enum class TextureWrap : int {
    /** Clamps texture values to the edges of the texture */
    CLAMP  = 0,
    /** Repeats the texture for values outside of [0,1] */
    REPEAT = 1,
    /** Repeats the texture for values outside of [0,1], reversing at odd values  */
    MIRROR = 2,
    /** Sets texture values outside of [0,1] to be a constant color */
    BORDER = 3
};
    
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
TextureFilter json_filter(const std::string name);

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
TextureWrap json_wrap(const std::string name);

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
BlendMode json_blend_mode(std::string name);

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
Color4 json_color(JsonValue* entry, std::string backup);


#pragma mark -
#pragma mark SDL Conversion
/**
 * Flips the pixel data in an SDL surface.
 *
 * This converts the SDL to OpenGL coordinates, to make it easier to use.
 *
 * @param surf  The SDL surface
 */
void sdl_flip_vertically(SDL_Surface* surf);

    }

}
#endif /* __CU_GRAPHICS_BASE_H__ */
