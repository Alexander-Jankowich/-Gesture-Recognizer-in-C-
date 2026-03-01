//
//  CUProgressBar.h
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
#ifndef __CU_PROGRESS_BAR_H__
#define __CU_PROGRESS_BAR_H__
#include <cugl/scene2/CUSceneNode2.h>
#include <cugl/scene2/CUPolygonNode.h>

namespace cugl {

    /**
     * The classes to construct a 2-d scene graph.
     *
     * Even though this is an optional package, this is one of the core features
     * of CUGL. These classes provide basic UI support (including limited Figma)
     * support. Any 2-d game will make extensive use of these classes. And
     * even 3-d games may use these classes for the HUD overlay.
     */
    namespace scene2 {

/**
 * This class is a node the represents an (animating) project bar.
 *
 * The progress bar may either be represented via a texture or a simple colored
 * rectangle. If it is a texture, it acts as a 3-patch (a nine patch with no
 * top or bottom component).  It will draw the left and right side of the
 * 3-patch and scale the middle component to match the progress. So if the
 * progress value is at 50%, the middle portion will be at half its maximum
 * size.
 */
class ProgressBar: public SceneNode {
protected:
    /** The progress percentage of this progress bar (between 0 and 1) */
    float _progress;

    /** The texture for the progress bar (if it exists) */
    std::shared_ptr<graphics::Texture> _texture;

    /** Whether we have generated render data for this node */
    bool _rendered;
    /** The interior interval */
    std::pair<float,float> _interior;
    /** The render vertices for this node */
    graphics::Mesh<graphics::SpriteVertex> _mesh;
    
    /** The blending equation for the texture */
    graphics::BlendState _blend;
    
public:
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
    ProgressBar();
    
    /**
     * Deletes this progress bar, disposing all resources
     */
    ~ProgressBar() { dispose(); }
    
    /**
     * Disposes all of the resources used by this node.
     *
     * A disposed progress bar can be safely reinitialized. Any children owned 
     * by this node will be released. They will be deleted if no other object
     * owns them.
     *
     * It is unsafe to call this on a progress bar that is still currently 
     * inside of a scene graph.
     */
    virtual void dispose() override;
    
    /**
     * Deactivates the default initializer.
     *
     * This initializer may not be used for a progress bar. A progress bar
     * either needs a texture or a size
     *
     * @return false
     */
    virtual bool init() override {
        CUAssertLog(false,"This node does not support the empty initializer");
        return false;
    }

    /**
     * Initializes a texture-less progress bar of the given size.
     *
     * The progress bar will be a rectangle with {@link #getColor} as its
     * fill color. The progress bar will be placed at the origin of the parent
     * and anchored at the bottom left.
     *
     * @param size  The progress bar size
     *
     * @return true if the progress bar is initialized properly, false otherwise.
     */
    bool initWithSize(const Size& size) {
        return initWithCaps(nullptr,0,0,size);
    }

    /**
     * Initializes a progress bar with the given texture.
     *
     * The progress bar will be the size of the texture. It will not have any
     * endcaps and will stretch the texture to the size of the bar. The progress
     * bar will be placed at the origin of the parent and anchored at the bottom
     * left.
     *
     * @param texture   The progress bar texture
     *
     * @return true if the progress bar is initialized properly, false otherwise.
     */
    bool initWithTexture(const std::shared_ptr<graphics::Texture>& texture) {
        return initWithCaps(texture,0,0);
    }
    
    /**
     * Initializes a progress bar with the given texture and size
     *
     * The progress bar texture will scale to the given size. It will not have
     * any endcaps and will stretch the texture to the size of the bar. The
     * progress bar will be placed at the origin of the parent and anchored at
     * the bottom left.
     *
     * @param texture   The progress bar texture
     * @param size      The progress bar size
     *
     * @return true if the progress bar is initialized properly, false otherwise.
     */
    bool initWithTexture(const std::shared_ptr<graphics::Texture>& texture,
                         const Size& size) {
        return initWithCaps(texture,0,0,size);
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
    bool initWithCaps(const std::shared_ptr<graphics::Texture>& texture, float left, float right);
    
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
    bool initWithCaps(const std::shared_ptr<graphics::Texture>& texture,
                      float left, float right, const Size& size);

    /**
     * Initializes a node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
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
    bool initWithData(const AssetManager* manager,
                      const std::shared_ptr<JsonValue>& data) override;

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated texture-less progress bar of the given size.
     *
     * The progress bar will be a rectangle with {@link #getColor} as its
     * fill color. The progress bar will be placed at the origin of the parent
     * and anchored at the bottom left.
     *
     * @param size  The progress bar size
     *
     * @return a newly allocated texture-less progress bar of the given size.
     */
    static std::shared_ptr<ProgressBar> allocWithSize(const Size& size) {
        std::shared_ptr<ProgressBar> node = std::make_shared<ProgressBar>();
        return (node->initWithSize(size) ? node : nullptr);
    }

    /**
     * Returns a newly allocated progress bar with the given texture.
     *
     * The progress bar will be the size of the texture. It will not have any
     * endcaps and will stretch the texture to the size of the bar. The progress
     * bar will be placed at the origin of the parent and anchored at the bottom
     * left.
     *
     * @param texture   The progress bar texture
     *
     * @return a newly allocated progress bar with the given texture.
     */
    static std::shared_ptr<ProgressBar> allocWithTexture(const std::shared_ptr<graphics::Texture>& texture) {
        std::shared_ptr<ProgressBar> node = std::make_shared<ProgressBar>();
        return (node->initWithTexture(texture) ? node : nullptr);
    }

    /**
     * Returns a newly allocated progress bar with the given texture and size
     *
     * The progress bar texture will scale to the given size. It will not have
     * any endcaps and will stretch the texture to the size of the bar. The
     * progress bar will be placed at the origin of the parent and anchored at
     * the bottom left.
     *
     * @param texture   The progress bar texture
     * @param size      The progress bar size
     *
     * @return a newly allocated progress bar with the given texture and size
     */
    static std::shared_ptr<ProgressBar> allocWithTexture(const std::shared_ptr<graphics::Texture>& texture,
                                                         const Size& size) {
        std::shared_ptr<ProgressBar> node = std::make_shared<ProgressBar>();
        return (node->initWithTexture(texture,size) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated progress bar with the texture and endcaps.
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
     * @return a newly allocated progress bar with the texture and endcaps.
     */
    static std::shared_ptr<ProgressBar> allocWithCaps(const std::shared_ptr<graphics::Texture>& texture,
                                                      float left, float right) {
        std::shared_ptr<ProgressBar> node = std::make_shared<ProgressBar>();
        return (node->initWithCaps(texture,left,right) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated progress bar with the given texture and size.
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
     * @return a newly allocated progress bar with the given texture and size.
     */
    static std::shared_ptr<ProgressBar> allocWithCaps(const std::shared_ptr<graphics::Texture>& texture,
                                                      float left, float right, const Size& size) {
        std::shared_ptr<ProgressBar> node = std::make_shared<ProgressBar>();
        return (node->initWithCaps(texture,left,right,size) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
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
     * @return a newly allocated node with the given JSON specificaton.
     */
    static std::shared_ptr<SceneNode> allocWithData(const AssetManager* manager,
                                                    const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<ProgressBar> result = std::make_shared<ProgressBar>();
        if (!result->initWithData(manager,data)) { result = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(result);
    }
    
#pragma mark -
#pragma mark Properties
    /**
     * Returns the percentage progress of this progress bar
     *
     * This value is a float between 0 and 1. Changing this value will alter
     * the size of the progress bar foreground.
     *
     * @return the percentage progress of this progress bar
     */
    float getProgress() const { return _progress; }

    /**
     * Sets the percentage progress of this progress bar
     *
     * This value is a float between 0 and 1. Changing this value will alter
     * the size of the progress bar foreground.
     *
     * @param progress  The percentage progress of this progress bar
     */
    void setProgress(float progress);
    
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
    virtual void setContentSize(const Size& size) override;
    
    /**
     * Sets the node texture to a new one allocated from a filename.
     *
     * This method will have no effect on the polygon vertices. This class
     * decouples the geometry from the texture. That is because we do not
     * expect the vertices to match the texture perfectly.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     */
    void setTexture(const std::string &filename) {
        std::shared_ptr<graphics::Texture> texture = graphics::Texture::allocWithFile(filename);
        setTexture(texture);
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
    void setTexture(const std::shared_ptr<graphics::Texture>& texture);
    
    /**
     * Returns the texture used by this node.
     *
     * This value can be nullptr. In that case the progress bar is a solid
     * rectangle with no endcaps.
     *
     * @return the texture used by this node
     */
    std::shared_ptr<graphics::Texture> getTexture() { return _texture; }
    
    /**
     * Returns the texture used by this node.
     *
     * This value can be nullptr. In that case the progress bar is a solid
     * rectangle with no endcaps.
     *
     * @return the texture used by this node
     */
    const std::shared_ptr<graphics::Texture>& getTexture() const { return _texture; }
    
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
    void setInterior(float left, float right);

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
     * @param interval  The interior interval
     */
    void setInterior(const std::pair<float,float> interval) {
        setInterior(interval.first,interval.second);
    }

    /**
     * Returns the interior internal defining progress bar.
     *
     * The interior interval is specified in pixel coordinates. It is with
     * respect to x-coordinates, which have their origin on the left.
     *
     * The interior interval is similar to a NinePatch. It the interval is (3,6)
     * then the left endcap ends at x=3 and the right endcap starts at x=6. This
     * value is ignored if there is no texture.
     *
     * @returns the interior internal defining progress bar.
     */
    const std::pair<float,float> getInterior() const { return _interior; }
    
    /**
     * Sets the color blend mode for this NinePatch.
     *
     * By default this value is {@link graphics::BlendMode#PREMULT}, as image
     * files are loaded with premultiplied alpha. You cannot determine the blend
     * mode for a node after it is set. You can only get the blend state for
     * that mode with the method {@link #getBlendState}.
     *
     * This blend state only affects the texture of the current node. It does
     * not affect any children of the node.
     *
     * @param mode  The color blend mode for this NinePatch.
     */
    void setBlendMode(const graphics::BlendMode& mode) {
        graphics::BlendState blend(mode);
        setBlendState(blend);
    }
    
    /**
     * Sets the color blend state for this NinePatch.
     *
     * This provides much more fine-tuned control over color blending that
     * {@link #setBlendMode}. By default this value is equivalent to
     * {@link graphics::BlendMode#PREMULT}, as image files are loaded with
     * premultiplied alpha.
     *
     * This blend state only affects the texture of the current node. It does
     * not affect any children of the node.
     *
     * @param blend The color blend state for this NinePatch.
     */
    void setBlendState(const graphics::BlendState& blend) {
        _blend = blend;
    }
    
    /**
     * Returns the color blend state for this NinePatch.
     *
     * Note that if color blending was set using {@link #setBlendMode}, that
     * blend mode was expanded into a blend state, and this value will be
     * the result of this expansion. By default this value is equivalent to
     * {@link graphics::BlendMode#PREMULT}, as image files are loaded with
     * premultiplied alpha.
     *
     * This blend state only affects the texture of the current node. It does
     * not affect any children of the node.
     *
     * @return the color blend state for this NinePatch.
     */
    const graphics::BlendState& getBlendState() const { return _blend; }
    
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
    virtual std::string toString(bool verbose=false) const override;
    
#pragma mark -
#pragma mark Rendering
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
    virtual void draw(const std::shared_ptr<graphics::SpriteBatch>& batch,
                      const Affine2& transform, Color4 tint) override;
    
    /**
     * Refreshes this node to restore the render data.
     */
    void refresh() { clearRenderData(); generateRenderData(); }
    
#pragma mark -
#pragma mark Internal Helpers
protected:
    /**
     * Allocate the render data necessary to render this node.
     */
    virtual void generateRenderData();
    
    /**
     * Clears the render data, releasing all vertices and indices.
     */
    void clearRenderData();

};
    }
}

#endif /* __CU_PROGRESS_BAR_H__ */
