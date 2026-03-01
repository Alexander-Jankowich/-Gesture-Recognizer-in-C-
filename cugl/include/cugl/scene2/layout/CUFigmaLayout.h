//
//  CUFigmaLayout.h
//  Cornell University Game Library (CUGL)
//
//  This module provides an support for a Figma layout. A Figma layout is a
//  layout that directly matches what the figma project looks like when using
//  the figtree plugin.
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
//  Author: Sebastian Rivera
//  Version: 7/3/24 (CUGL 3.0 reorganization)
//
#ifndef __CU_FIGMA_LAYOUT_H__
#define __CU_FIGMA_LAYOUT_H__
#include <cugl/scene2/layout/CULayout.h>
#include <unordered_map>

namespace  cugl {

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
 * This class provides a Figma layout manager.
 *
 * A Figma layout attaches a child node to one of nine "anchors" in the
 * parent (corners, sides or middle), together with a percentage (or absolute)
 * offset. As the parent grows or shinks, the child will move according to its
 * anchor. For example, nodes in the center will stay centered, while nodes on
 * the left side will move to keep the appropriate distance from the left side.
 * In fact, the stretching behavior is very similar to that of a
 * {@link NinePatch}.
 *
 * Layout information is indexed by key. To look up the layout information
 * of a scene graph node, we use the name of the node. This requires all
 * nodes to have unique names. The {@link Scene2Loader} prefixes all child
 * names by the parent name, so this is the case in any well-defined JSON file.
 */
class FigmaLayout : public Layout {
protected:
    /**
     * This inner class stores the layout information as a struct.
     *
     * Offsets may either be absolute or relative. A relative offset is
     * expressed as a percentage of width or height. An absolute offset is
     * expressed in terms of Node coordinates.
     */
    class Entry {
    public:
        /** The left x offset from the anchor in absolute or relative units */
        float left_offset;
        /** The right x offset from the anchor in absolute or relative units */
        float right_offset;
        /** The top y offset from the anchor in absolute or relative units */
        float top_offset;
        /** The bottom y offset from the anchor in absolute or relative units */
        float bottom_offset;
        /** The associated anchor point */
        Anchor anchor;
        /** Whether to use an absolute offset instead of a relative (percentage) one for x */
        bool x_absolute;
        /** Whether to use an absolute offset instead of a relative (percentage) one for y*/
        bool y_absolute;
    };
    
    /** The map of keys to layout information */
    std::unordered_map<std::string,Entry> _entries;
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a degenerate layout manager with no data.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    FigmaLayout() {}
    
    /**
     * Deletes this layout manager, disposing of all resources.
     */
    ~FigmaLayout() { dispose(); }
    
    /**
     * Initializes a new layout manager with the given JSON specificaton.
     *
     * The JSON specification format is simple. It only supports one (required)
     * attribute: 'type'. The type should specify "figma".
     *
     * @param data      The JSON object specifying the node
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithData(const std::shared_ptr<JsonValue>& data) override { return true; }
    
    /**
     * Deletes the layout resources and resets all attributes.
     *
     * A disposed layout manager can be safely reinitialized.
     */
    virtual void dispose() override { _entries.clear(); }

    /**
     * Returns a newly allocated layout manager.
     *
     * The layout manager is initially empty. Before using it to perform a
     * layout, layout information must be registered throught the {@link add}
     * method interface.
     *
     * @return a newly allocated layout manager.
     */
    static std::shared_ptr<FigmaLayout> alloc() {
        std::shared_ptr<FigmaLayout> result = std::make_shared<FigmaLayout>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated layout manager with the given JSON specificaton.
     *
     * The JSON specification format is simple. It only supports one (required)
     * attribute: 'type'. The type should specify "figma".
     *
     * @param data      The JSON object specifying the node
     *
     * @return a newly allocated layout manager with the given JSON specificaton.
     */
    static std::shared_ptr<FigmaLayout> allocWithData(const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<FigmaLayout> result = std::make_shared<FigmaLayout>();
        return (result->initWithData(data) ? result : nullptr);
    }
    
#pragma mark Layout
    /**
     * Assigns layout information for a given key.
     *
     * The JSON object may contain any of the following attribute values:
     *
     *      "x_anchor": One of 'left', 'center', 'right', or 'left+right'
     *      "y_anchor": One of 'bottom', 'middle', 'top', or 'top+bottom'
     *      "x_absolute": Whether to use absolute instead of relative (percentage) offsets in x
     *      "y_absolute": Whether to use absolute instead of relative (percentage) offsets in y
     *      "l_offset": A number indicating the left offset from the anchor.
     *                  If "x_absolute" is true, this is the distance in coordinate space.
     *                  Otherwise it is a percentage of the width.
     *      "r_offset": A number indicating the right offset from the anchor.
     *                  If "x_absolute" is true, this is the distance in coordinate space.
     *                  Otherwise it is a percentage of the width.
     *      "t_offset": A number indicating the top offset from the anchor.
     *                  If "y_absolute" is true, this is the distance in coordinate space.
     *                  Otherwise it is a percentage of the height.
     *      "b_offset": A number indicating the bottom offset from the anchor.
     *                  If "y_absolute" is true, this is the distance in coordinate space.
     *                  Otherwise it is a percentage of the height.
     *
     * All attributes are optional. There are no required attributes.
     *
     * To look up the layout information of a scene graph node, we use the name
     * of the node. This requires all nodes to have unique names. The
     * {@link Scene2Loader} prefixes all child names by the parent name, so
     * this is the case in any well-defined JSON file. If the key is already
     * in use, this method will fail.
     *
     * @param key   The key identifying the layout information
     * @param data  A JSON object with the layout information
     *
     * @return true if the layout information was assigned to that key
     */
    virtual bool add(const std::string key, const std::shared_ptr<JsonValue>& data) override;
    
    /**
     * Assigns layout information for a given key.
     *
     * This method allows for independent specification of absolute or relative 
     * offsets along the x and y axes. If an axis is marked as absolute, its offset 
     * is treated as a distance in Node coordinate space. Otherwise, the offset is 
     * interpreted as a percentage of the parent node's size in that dimension.
     *
     * To look up the layout information of a scene graph node, we use the name
     * of the node. This requires all nodes to have unique names. The
     * {@link Scene2Loader} prefixes all child names by the parent name, so
     * this is the case in any well-defined JSON file. If the key is already
     * in use, this method will fail.
     *
     * @param key           The key identifying the layout information
     * @param anchor        The anchor point to use
     * @param left          The offset from the left edge
     * @param right         The offset from the right edge
     * @param top           The offset from the top edge
     * @param bottom        The offset from the bottom edge
     * @param x_absolute    Whether the x offset is absolute (in coordinate space)
     * @param y_absolute    Whether the y offset is absolute (in coordinate space)
     *
     * @return true if the layout information was assigned to that key
     */
    bool addMixed(const std::string key, Anchor anchor,
                  float left, float right, float top, float bottom,
                  bool x_absolute, bool y_absolute);

    /**
     * Removes the layout information for a given key.
     *
     * To look up the layout information of a scene graph node, we use the name
     * of the node. This requires all nodes to have unique names. The
     * {@link Scene2Loader} prefixes all child names by the parent name, so
     * this is the case in any well-defined JSON file.
     *
     * If the key is not in use, this method will fail.
     *
     * @param key   The key identifying the layout information
     *
     * @return true if the layout information was removed for that key
     */
    virtual bool remove(const std::string key) override;
    
    /**
     * Performs a layout on the given node.
     *
     * This layout manager will searches for those children that are registered
     * with it. For those children, it repositions and/or resizes them according
     * to the layout information.
     *
     * This manager attaches a child node to one of nine "anchors" in the parent
     * (corners, sides or middle), together with a percentage (or absolute)
     * offset. As the parent grows or shinks, the child will move according to
     * its anchor. For example, nodes in the center will stay centered, while
     * nodes on the left side will move to keep the appropriate distance from
     * the left side. In fact, the stretching behavior is very similar to that
     * of a {@link NinePatch}.
     *
     * To look up the layout information of a scene graph node, this method uses
     * the name of the node. This requires all nodes to have unique names. The
     * {@link Scene2Loader} prefixes all child names by the parent name, so
     * this is the case in any well-defined JSON file.
     *
     * Children not registered with this layout manager are not affected.
     *
     * @param node  The scene graph node to rearrange
     */
    virtual void layout(scene2::SceneNode* node) override;
    
    /**
     * Repositions the given Figma node according the rules of its anchor.
     *
     * The repositioning is done relative to bounds, not the parent node. This
     * allows us to apply anchors to a subregion of the parent, like we do in
     * {@link GridLayout}.
     *
     * This function is different from {@link Layout#placeNode} in that it
     * follows the conventions of Figma layouts. For Figma nodes, the offset is
     * measured from the top-left corner of the parent. The size of the offset
     * is the total distance from the edges. So the width is the sum of the
     * distance from the left and right edges while the height is the sum of
     * the distance from the top and bottom edges
     *
     * @param node      The node to reposition
     * @param anchor    The anchor rule for this node
     * @param bounds    The area to compute the position from
     * @param offset    The Figma specified offset from the parent
     */
    static void placeFigmaNode(SceneNode* node, Anchor anchor,
                               const Rect& bounds,
                               const Rect& offset);
};
	}
}
#endif /* __CU_FIGMA_LAYOUT_H__ */
