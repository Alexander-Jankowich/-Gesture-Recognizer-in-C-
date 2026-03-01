//
//  CUFigmaLayout.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides an support for a Figma layout.  A Figma layout
//  is a layout that directly matches what the figma looks like when
//  using figtree.
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
//  Author: Sebastian Rivera
//  Version: 4/28/25
//
#include <cugl/scene2/CUScene2.h>
#include <cugl/scene2/layout/CUFigmaLayout.h>

using namespace cugl::scene2;

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
 * All attributes are optional.  There are no required attributes.
 *
 * To look up the layout information of a scene graph node, we use the name
 * of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file. If the key is already
 * in use, this method will fail.
 *
 * @param key   The key identifying the layout information
 * @param data  A JSON object with the layout information
 *
 * @return true if the layout information was assigned to that key
 */
bool FigmaLayout::add(const std::string key, const std::shared_ptr<JsonValue>& data) {
    std::string horz = data->getString("x_anchor","center");
    std::string vert = data->getString("y_anchor","middle");
    Anchor anchor = getAnchor(horz, vert);

    float left_offset = data->getFloat("left_offset", 0.0f);
    float right_offset = data->getFloat("right_offset", 0.0f);
    float top_offset = data->getFloat("top_offset", 0.0f);
    float bottom_offset = data->getFloat("bottom_offset", 0.0f);
    
    
    bool x_absolute = data->getBool("x_absolute", false); 
    bool y_absolute = data->getBool("y_absolute", false); 

    return addMixed(key, anchor, left_offset, right_offset, top_offset, bottom_offset, x_absolute, y_absolute);
}
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
 * @param key          The key identifying the layout information
 * @param anchor       The anchor point to use
 * @param offset       The offset from the anchor point
 * @param x_absolute   Whether the x offset is absolute (in coordinate space)
 * @param y_absolute   Whether the y offset is absolute (in coordinate space)
 *
 * @return true if the layout information was assigned to that key
*/
bool FigmaLayout::addMixed(const std::string key, Anchor anchor, float left, float right, float top, float bottom, bool x_absolute, bool y_absolute) {
    auto last = _entries.find(key);
    if (last != _entries.end()) {
        return false;
    }

    Entry entry;
    entry.anchor = anchor;
    entry.left_offset = left;
    entry.right_offset = right;
    entry.top_offset = top;
    entry.bottom_offset = bottom;
    entry.x_absolute = x_absolute;
    entry.y_absolute = y_absolute;  
    _entries[key] = entry;
    return true;
}


/**
 * Removes the layout information for a given key.
 *
 * To look up the layout information of a scene graph node, we use the name
 * of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file.
 *
 * If the key is not in use, this method will fail.
 *
 * @param key   The key identifying the layout information
 *
 * @return true if the layout information was removed for that key
 */
bool FigmaLayout::remove(const std::string key) {
    auto entry = _entries.find(key);
    if (entry != _entries.end()) {
        _entries.erase(entry);
        return true;
    }
    return false;
}

/**
 * Performs a layout on the given node.
 *
 * This layout manager will searches for those children that are registered
 * with it. For those children, it repositions and/or resizes them according
 * to the layout information.
 *
 * This manager attaches a child node to one of nine "anchors" in the parent
 * (corners, sides or middle), together with a percentage (or absolute)
 * offset.  As the parent grows or shinks, the child will move according to
 * its anchor.  For example, nodes in the center will stay centered, while
 * nodes on the left side will move to keep the appropriate distance from
 * the left side. In fact, the stretching behavior is very similar to that
 * of a {@link NinePatch}.
 *
 * To look up the layout information of a scene graph node, this method uses
 * the name of the node.  This requires all nodes to have unique names. The
 * {@link SceneLoader} prefixes all child names by the parent name, so
 * this is the case in any well-defined JSON file.
 *
 * Children not registered with this layout manager are not affected.
 *
 * @param node  The scene graph node to rearrange
 */
void FigmaLayout::layout(scene2::SceneNode* node) {
    auto kids = node->getChildren();
    Rect bounds = node->getLayoutBounds();
    for(auto it = kids.begin(); it != kids.end(); ++it) {
        SceneNode* child = it->get();
        switch (child->getLayoutConstraint()) {
            case SceneNode::Constraint::LAYOUT:
            {
                auto jt = _entries.find(child->getName());
                if (jt != _entries.end()) {
                    Entry entry = jt->second;
                    float l_off = entry.x_absolute ? entry.left_offset : entry.left_offset * bounds.size.width;
                    float r_off = entry.x_absolute ? entry.right_offset : entry.right_offset * bounds.size.width;
                    float t_off = entry.y_absolute ? entry.top_offset : entry.top_offset * bounds.size.height;
                    float b_off = entry.y_absolute ? entry.bottom_offset : entry.bottom_offset * bounds.size.height;
                    placeFigmaNode(child, entry.anchor, bounds,
                                   Rect(l_off, t_off, l_off+r_off, t_off+b_off));
                }
                break;
            }
            case SceneNode::Constraint::SCENE:
                if (child->getScene() != nullptr) {
                    Vec2 anchor = child->getAnchor();
                    child->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
                    child->setPosition(0,0);
                    child->setContentSize(child->getScene()->getSize());
                    child->setAnchor(anchor);
                }
                break;
            case SceneNode::Constraint::SAFE:
                if (child->getScene() != nullptr) {
                    Vec2 anchor = child->getAnchor();
                    Rect bounds = child->getScene()->getSafeBounds();
                    child->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
                    child->setPosition(bounds.origin);
                    child->setContentSize(bounds.size);
                    child->setAnchor(anchor);
                }
                break;
            case SceneNode::Constraint::NONE:
                break;
        }
    }
}


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
void FigmaLayout::placeFigmaNode(SceneNode* node, Anchor anchor,
                                 const Rect& bounds, const Rect& offset) {
    Vec2 spot;
    switch (anchor) {
        case Anchor::TOP_LEFT:
            spot.set(0, bounds.size.height);
            spot += offset.origin;
            break;
        case Anchor::MIDDLE_LEFT:
            spot.set(0, bounds.size.height/2.0f);
            spot += offset.origin;
            break;
        case Anchor::TOP_CENTER:
            spot.set(bounds.size.width/2.0f, bounds.size.height);
            spot += offset.origin;
            break;
        case Anchor::CENTER:
            spot.set(bounds.size.width/2.0f, bounds.size.height/2.0f);
            spot += offset.origin;
            break;
        case Anchor::BOTTOM_CENTER:
            spot.set(bounds.size.width/2.0f, 0);
            spot += offset.origin;
            break;
        case Anchor::TOP_RIGHT:
            spot.set(bounds.size.width, bounds.size.height);
            spot += offset.origin;
            break;
        case Anchor::MIDDLE_RIGHT:
            spot.set(bounds.size.width, bounds.size.height/2.0f);
            spot += offset.origin;
            break;
        case Anchor::BOTTOM_RIGHT:
            spot.set(bounds.size.width, 0);
            spot += offset.origin;
            break;
        case Anchor::BOTTOM_LEFT:
            spot += offset.origin;
        case Anchor::NONE:
            break;
        case Anchor::LEFT_FILL:
            spot.set(0, node->getContentHeight()/2);
            node->setContentHeight(bounds.size.height - offset.size.height);
            spot += offset.origin;
            break;
        case Anchor::CENTER_FILL:
            spot.set(bounds.size.width/2.0f,node->getContentHeight()/2);
            node->setContentHeight(bounds.size.height - offset.size.height);
            spot += offset.origin;
            break;
        case Anchor::RIGHT_FILL:
            spot.set(bounds.size.width,node->getContentHeight()/2);
            node->setContentHeight(bounds.size.height - offset.size.height);
            spot += offset.origin;
            break;
        case Anchor::BOTTOM_FILL:
            spot.set(node->getContentWidth()/2,0);
            node->setContentWidth(bounds.size.width - offset.size.width);
            spot += offset.origin;
            break;
        case Anchor::MIDDLE_FILL:
            spot.set(node->getContentWidth()/2,bounds.size.height/2.0f);
            node->setContentWidth(bounds.size.width - offset.size.width);
            spot += offset.origin;
            break;
        case Anchor::TOP_FILL:
            spot.set(node->getContentWidth()/2,bounds.size.height);
            node->setContentWidth(bounds.size.width - offset.size.width);
            spot += offset.origin;
            break;
        case Anchor::TOTAL_FILL:
            float width  = bounds.size.width  - offset.size.width;
            float height = bounds.size.height - offset.size.height;
            node->setContentSize(width, height);
        
            float ax = node->getAnchor().x;
            float ay = node->getAnchor().y;
        
            float x = offset.origin.x + ax * width;
            float y = offset.origin.y + ay * height;
            spot.set(x, y);
            break;
    }
    
    spot += bounds.origin;
    node->setPosition(spot);
}
