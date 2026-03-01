//
//  CUSampler.h
//  Cornell University Game Library (CUGL)
//
//  This module provides the class for representing a texture sampler. A
//  sampler instructs the graphics API how to read image data and apply
//  filtering and other transformations for the shader.
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
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty. In no event will the authors be held liable for any damages
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
//  Version: 3/30/25 (Vulkan Integration)
//
#ifndef __CU_SAMPLER_H__
#define __CU_SAMPLER_H__
#include <cugl/graphics/CUGraphicsBase.h>
#include <cugl/core/math/CUMathBase.h>
#include <cugl/core/math/CUSize.h>

namespace cugl {
    /**
     * The classes and functions needed to construct a graphics pipeline.
     *
     * Initially these were part of the core CUGL module. However, in the 
     * transition to Vulkan, we discovered that we need to factor this out.
     * This allows us to provide options for OpenGL, Vulkan, and even headless
     * clients.
     */
namespace graphics {

// An opaque class for the graphics API
class SamplerData;

/**
 * This is a class representing a sampler.
 *
 * A sampler is second part of a {@link Texture}. It is used to to instruct
 * the texture how it should read and filter data from the underlying image.
 * In OpenGL, this is a simple struct. However, its allocation is a little
 * more involved in Vulkan and Metal.
 */
class Sampler {
#pragma mark Values
private:
    /** The graphics API sampler data */
    SamplerData* _data;

    /** The minimization algorithm */
    TextureFilter _minFilter;
    
    /** The maximization algorithm */
    TextureFilter _magFilter;
    
    /** The wrap-style for the horizontal texture coordinate */
    TextureWrap _wrapS;
    
    /** The wrap-style for the vertical texture coordinate */
    TextureWrap _wrapT;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new empty sampler with no filter or wrap-style.
     *
     * This method performs no allocations. You must call init to generate
     * a proper sampler.
     */
    Sampler() : _data(nullptr) {}
    
    /**
     * Deletes this sampler, disposing all resources
     */
    ~Sampler() { dispose(); }
    
    /**
     * Deletes the sampler and resets all attributes.
     *
     * You must reinitialize the sampler to use it.
     */
    void dispose();
    
    /**
     * Initializes a default sampler.
     *
     * The sampler will use a {@link TextureFilter#LINEAR} filter. It will also
     * use {@link TextureWrap#CLAMP} on both axes.
     *
     * @return true if initialization was successful.
     */
    bool init() {
        return init(TextureWrap::CLAMP, TextureWrap::CLAMP,
                    TextureFilter::LINEAR, TextureFilter::LINEAR);
    }

    /**
     * Initializes a sampler with the given wrap values
     *
     * The sampler will use a {@link TextureFilter#LINEAR} filter.
     *
     * @param wrapS     The horizontal wrap
     * @param wrapT     The vertical wrap
     *
     * @return true if initialization was successful.
     */
    bool initWithWrap(TextureWrap wrapS, TextureWrap wrapT) {
        return init(wrapS, wrapT, TextureFilter::LINEAR, TextureFilter::LINEAR);
    }
    
    /**
     * Initializes a sampler with the given filter values
     *
     * The sampler will use {@link TextureWrap#CLAMP} on both axes.
     *
     * @param magFilter The filter for magnifying an image
     * @param minFilter The filter for shrinking an image
     *
     * @return true if initialization was successful.
     */
    bool initWithFilters(TextureFilter magFilter, TextureFilter minFilter) {
        return init(TextureWrap::CLAMP, TextureWrap::CLAMP, magFilter, minFilter);
    }

    /**
     * Initializes a sampler with the given wrap and filter values
     *
     * @param wrapS     The horizontal wrap
     * @param wrapT     The vertical wrap
     * @param magFilter The filter for magnifying an image
     * @param minFilter The filter for shrinking an image
     *
     * @return true if initialization was successful.
     */
    bool init(TextureWrap wrapS, TextureWrap wrapT,
              TextureFilter magFilter, TextureFilter minFilter);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated a default sampler.
     *
     * The sampler will use a {@link TextureFilter#LINEAR} filter. It will also
     * use {@link TextureWrap#CLAMP} on both axes.
     *
     * @return a newly allocated a default sampler.
     */
    static std::shared_ptr<Sampler> alloc() {
        std::shared_ptr<Sampler> result = std::make_shared<Sampler>();
        return (result->init() ? result : nullptr);
    }

    /**
     * Returns a newly allocated sampler with the given wrap values
     *
     * The sampler will use a {@link TextureFilter#LINEAR} filter.
     *
     * @param wrapS     The horizontal wrap
     * @param wrapT     The vertical wrap
     *
     * @return a newly allocated sampler with the given wrap values
     */
    static std::shared_ptr<Sampler> allocWithWrap(TextureWrap wrapS, TextureWrap wrapT) {
        std::shared_ptr<Sampler> result = std::make_shared<Sampler>();
        return (result->initWithWrap(wrapS, wrapT) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated sampler with the given filter values
     *
     * The sampler will use {@link TextureWrap#CLAMP} on both axes.
     *
     * @param magFilter The filter for magnifying an image
     * @param minFilter The filter for shrinking an image
     *
     * @return a newly allocated sampler with the given filter values
     */
    static std::shared_ptr<Sampler> allocWithFilters(TextureFilter magFilter, TextureFilter minFilter) {
        std::shared_ptr<Sampler> result = std::make_shared<Sampler>();
        return (result->initWithFilters(magFilter, minFilter) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated a sampler with the given wrap and filter values
     *
     * @param wrapS     The horizontal wrap
     * @param wrapT     The vertical wrap
     * @param magFilter The filter for magnifying an image
     * @param minFilter The filter for shrinking an image
     *
     * @return a newly allocated a sampler with the given wrap and filter values
     */
    static std::shared_ptr<Sampler> alloc(TextureWrap wrapS, TextureWrap wrapT,
                                          TextureFilter magFilter, TextureFilter minFilter) {
        std::shared_ptr<Sampler> result = std::make_shared<Sampler>();
        return (result->init(wrapS, wrapT, magFilter, minFilter) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns a unique identifier for this sampler
     *
     * This value can be used in memory-based hashes.
     *
     * @return a unique identifier for this sampler
     */
    size_t id() const;
    
    /**
     * Returns the platform-specific implementation for this sampler
     *
     * The value returned is an opaque type encapsulating the information for
     * either OpenGL or Vulkan.
     *
     * @return the platform-specific implementation for this sampler
     */
    SamplerData* getImplementation() { return _data; }
    
    /**
     * Returns the magnification filter of this sampler.
     *
     * The magnification filter is the algorithm hint that the graphics card
     * uses to make an image larger. The default is LINEAR.
     *
     * @return the magnification filter of this sampler.
     */
    TextureFilter getMagFilter() const { return _magFilter; }
    
    /**
     * Sets the magnification filter of this sampler.
     *
     * The magnification filter is the algorithm hint that the graphics card
     * uses to make an image larger. The default is LINEAR.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param filter    The magnification filter of this sampler.
     */
    void setMagFilter(TextureFilter filter);
    
    /**
     * Returns the minimization filter of this sampler.
     *
     * The minimization filter is the algorithm hint that the graphics card
     * uses to make an image smaller. The default is LINEAR.
     *
     * @return the minimization filter of this sampler.
     */
    TextureFilter getMinFilter() const { return _minFilter; }
    
    /**
     * Sets the minimization filter of this sampler.
     *
     * The minimization filter is the algorithm hint that the graphics card
     * uses to make an image smaller. The default is LINEAR.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param filter    The min filter of this sampler.
     */
    void setMinFilter(TextureFilter filter);
    
    /**
     * Returns the horizontal wrap of this sampler.
     *
     * The default is CLAMP.
     *
     * @return the horizontal wrap of this sampler.
     */
    TextureWrap getWrapS() const { return _wrapS; }
    
    /**
     * Sets the horizontal wrap of this sampler.
     *
     * The default is CLAMP.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param wrap  The horizontal wrap setting of this sampler
     */
    void setWrapS(TextureWrap wrap);
    
    /**
     * Returns the vertical wrap of this sampler.
     *
     * The default is CLAMP.
     *
     * @return the vertical wrap of this sampler.
     */
    TextureWrap getWrapT() const { return _wrapT; }
    
    /**
     * Sets the vertical wrap of this sampler.
     *
     * The default is CLAMP.
     *
     * When this value is changed, the value will be pushed to the
     * implementation and all subsequent drawing commands will use the new
     * value. Previous drawing commands (including those in flight) are
     * unaffected.
     *
     * @param wrap  The vertical wrap setting of this sampler
     */
    void setWrapT(TextureWrap wrap);
    
};

    }

}

#endif /* __CU_SAMPLER_H__ */
