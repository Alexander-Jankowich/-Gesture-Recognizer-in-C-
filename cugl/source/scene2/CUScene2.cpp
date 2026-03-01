//
//  CUScene2.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for the root node of a (2d) scene graph.  After
//  much debate, we have decided to decouple this from the application class
//  (which is different than Cocos2d).
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
#include <cugl/scene2/CUScene2.h>
#include <cugl/core/util/CUStringTools.h>
#include <cugl/core/CUApplication.h>
#include <cugl/core/CUDisplay.h>
#include <sstream>
#include <algorithm>

using namespace cugl;
using namespace cugl::graphics;
using namespace cugl::scene2;

/**
 * Creates a new degenerate Scene on the stack.
 *
 * The scene has no camera and must be initialized.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
Scene2::Scene2() : Scene(),
_color(Color4::WHITE) {
}

/**
 * Disposes all of the resources used by this scene.
 *
 * A disposed Scene can be safely reinitialized. Any children owned by this
 * scene will be released.  They will be deleted if no other object owns them.
 */
void Scene2::dispose() {
    Scene::dispose();
    removeAllChildren();
    _color = Color4::WHITE;
    _batch = nullptr;
    _framebuffer = nullptr;
}


/**
 * Initializes a Scene to fill the display.
 *
 * This scene will have a viewport that matches the display and an
 * orthographic camera of the same size. The camera will have its origin
 * in the bottom left corner of the display.
 *
 * @return true if initialization was successful.
 */
bool Scene2::init() {
    if (!Scene::init()) {
        return false;
    }
    _safearea = Application::get()->getSafeBounds();
    _camera = OrthographicCamera::allocOffset(0, 0, _size.width, _size.height);
    _active = _camera != nullptr;
    return _active;
}

/**
 * Initializes a scene with the given viewport.
 *
 * The scene will have an orthographic camera whose viewport matches the
 * given value. If this does not align with the aspect ratio of the display
 * this can be distortionary. Scenes are not meant to be smaller than the
 * the size of the display. For those applications, you want a
 * {@link SceneNode2}.
 *
 * The viewport and camera will have its origin in the bottom left corner
 * of the display. The viewport origin affects the coordinate conversion
 * methods {@link Camera#project()} and {@link Camera#unproject()} which
 * are used to convert from the scene graph to a display. It is intended
 * to represent the offset of the viewport in a larger canvas.
 *
 * @param x         The viewport x offset
 * @param y         The viewport y offset
 * @param width     The viewport width
 * @param height    The viewport height
 *
 * @return true if initialization was successful.
 */
bool Scene2::initWithViewport(float x, float y, float width, float height) {
    if (!Scene::init()) {
        return false;
    }

    _safearea = Application::get()->getSafeBounds();
    float sx = width/_size.width;
    float sy = height/_size.height;
    _size.set(width,height);
    _safearea.origin.x *= sx;
    _safearea.origin.y *= sy;
    _safearea.size.width  *= sx;
    _safearea.size.height *= sy;

    _camera = OrthographicCamera::allocOffset(x, y, width, height);
    _active = _camera != nullptr;
    return _active;

}


/**
 * Initializes a Scene with the given size hint.
 *
 * 2D Scenes are designed to fill the entire screen. However, the size of
 * that screen can vary from device to device. To make scene design easier,
 * designs are typically locked to a dimension: width or height.
 *
 * This is the purpose of the size hint. If either of the values of hint
 * are non-zero, then the scene will lock that dimension to that particular
 * size. If both are non-zero, it will choose its dimension according to
 * the device orientation. Landscape will be height, while portrait will
 * pick width. Devices with no orientation will always priortize height
 * over width.
 *
 * If this initializer is used, the scene will fill the screen and the
 * viewport will match.
 *
 * @param width     The width size hint
 * @param height    The height size hint
 *
 * @return true if initialization was successful.
 */
bool Scene2::initWithHint(float width, float height) {
    if (!Scene::init()) {
        return false;
    }
    
    if (width < 0 || height < 0) {
        CUAssertLog(false,"Size hint (%g,%g) must be nonnegative",width,height);
        return false;
    }
    
    _safearea = Application::get()->getSafeBounds();
    if (width > 0 && height > 0) {
        Display::Orientation orient = Display::get()->getDisplayOrientation();
        if (orient == Display::Orientation::PORTRAIT ||
            orient == Display::Orientation::UPSIDE_DOWN) {
            height = 0;
        } else {
            width  = 0;
        }
    }

    float scale = 1.0;
    if (width > 0) {
        scale = width/_size.width;
    } else {
        scale = height/_size.height;
    }
    _size *= scale;
    _safearea.origin *= scale;
    _safearea.size   *= scale;
    
    _camera = OrthographicCamera::allocOffset(0, 0, _size.width, _size.height);
    _active = _camera != nullptr;
    return _active;
}

/**
 * Returns a string representation of this scene for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this scene for debuggging purposes.
 */
std::string Scene2::toString(bool verbose) const {
    std::stringstream ss;
    ss << (verbose ? "cugl::Scene2(name:" : "(name:");
    ss << _name << ")";
    return ss.str();
}

#pragma mark -
#pragma mark Scene Graph
/**
 * Returns the child at the given position.
 *
 * Children are not necessarily enumerated in the order that they are added.
 * Hence you should generally attempt to retrieve a child by tag or by name
 * instead.
 *
 * @param pos   The child position.
 *
 * @return the child at the given position.
 */
std::shared_ptr<scene2::SceneNode>& Scene2::getChild(unsigned int pos) {
    CUAssertLog(pos < _children.size(), "Position index out of bounds");
    return _children[pos];
}

/**
 * Returns the child at the given position.
 *
 * Children are not necessarily enumerated in the order that they are added.
 * Hence you should generally attempt to retrieve a child by tag or by name
 * instead.
 *
 * @param pos   The child position.
 *
 * @return the child at the given position.
 */
const std::shared_ptr<scene2::SceneNode>& Scene2::getChild(unsigned int pos) const {
    CUAssertLog(pos < _children.size(), "Position index out of bounds");
    return _children[pos];
}

/**
 * Returns the (first) child with the given tag.
 *
 * If there is more than one child of the given tag, it returns the first
 * one that is found. Children are not necessarily enumerated in the order
 * that they are added. Hence it is very important that tags be unique.
 *
 * @param tag   An identifier to find the child node.
 *
 * @return the (first) child with the given tag.
 */
std::shared_ptr<scene2::SceneNode> Scene2::getChildByTag(unsigned int tag) const  {
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        if ((*it)->getTag() == tag) {
            return *it;
        }
    }
    return nullptr;
}

/**
 * Returns the (first) child with the given name.
 *
 * If there is more than one child of the given name, it returns the first
 * one that is found. Children are not necessarily enumerated in the order
 * that they are added. Hence it is very important that names be unique.
 *
 * @param name  An identifier to find the child node.
 *
 * @return the (first) child with the given name.
 */
std::shared_ptr<scene2::SceneNode> Scene2::getChildByName(const std::string name) const {
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        if ((*it)->getName() == name) {
            return *it;
        }
    }
    return nullptr;
}

/**
 * Adds a child to this scene.
 *
 * Children are not necessarily enumerated in the order that they are added.
 * Hence you should generally attempt to retrieve a child by tag or by name
 * instead.
 *
 * @param child A child node.
 */
void Scene2::addChild(const std::shared_ptr<scene2::SceneNode>& child) {
    CUAssertLog(child->_childOffset == -1, "The child is already in a scene graph");
    CUAssertLog(child->_graph == nullptr,  "The child is already in a scene graph");
    child->_childOffset = (unsigned int)_children.size();
    
    // Add the child
    _children.push_back(child);
    child->setParent(nullptr);
    child->pushScene(this);
}

/**
 * Swaps the current child child1 with the new child child2.
 *
 * If inherit is true, the children of child1 are assigned to child2 after
 * the swap; this value is false by default.  The purpose of this value is
 * to allow transitions in the scene graph.
 *
 * This method is undefined if child1 is not a child of this scene.
 *
 * @param child1    The current child of this node
 * @param child2    The child to swap it with.
 * @param inherit   Whether the new child should inherit the children of child1.
 */
void Scene2::swapChild(const std::shared_ptr<scene2::SceneNode>& child1,
                       const std::shared_ptr<scene2::SceneNode>& child2,
                       bool inherit) {
    _children[child1->_childOffset] = child2;
    child2->_childOffset = child1->_childOffset;
    child2->setParent(nullptr);
    child1->setParent(nullptr);
    child2->pushScene(this);
    child1->pushScene(nullptr);

    // Check if we are dirty and/or inherit children
    if (inherit) {
        std::vector<std::shared_ptr<scene2::SceneNode>> grands = child1->getChildren();
        child1->removeAllChildren();
        for(auto it = grands.begin(); it != grands.end(); ++it) {
            child2->addChild(*it);
        }
    }
}

/**
 * Removes the child at the given position from this Node.
 *
 * Removing a child alters the position of every child after it.  Hence
 * it is unsafe to cache child positions.
 *
 * @param pos   The position of the child node which will be removed.
 */
void Scene2::removeChild(unsigned int pos) {
    CUAssertLog(pos < _children.size(), "Position index out of bounds");
    std::shared_ptr<scene2::SceneNode> child = _children[pos];
    child->setParent(nullptr);
    child->pushScene(nullptr);
    child->_childOffset = -1;
    for(int ii = pos; ii < _children.size()-1; ii++) {
        _children[ii] = _children[ii+1];
        _children[ii]->_childOffset = ii;
    }
    _children.resize(_children.size()-1);
}

/**
 * Removes a child from this Node.
 *
 * Removing a child alters the position of every child after it.  Hence
 * it is unsafe to cache child positions.
 *
 * If the child is not in this node, nothing happens.
 *
 * @param child The child node which will be removed.
 */
void Scene2::removeChild(const std::shared_ptr<scene2::SceneNode>& child) {
    CUAssertLog(_children[child->_childOffset] == child, 
                "The child is not in this scene graph");
    removeChild(child->_childOffset);
}

/**
 * Removes a child from the Scene by tag value.
 *
 * If there is more than one child of the given tag, it removes the first
 * one that is found. Children are not necessarily enumerated in the order
 * that they are added. Hence it is very important that tags be unique.
 *
 * @param tag   An integer to identify the node easily.
 */
void Scene2::removeChildByTag(unsigned int tag) {
    std::shared_ptr<scene2::SceneNode> child = getChildByTag(tag);
    if (child != nullptr) {
        removeChild(child->_childOffset);
    }
}

/**
 * Removes a child from the Scene by name.
 *
 * If there is more than one child of the given name, it removes the first
 * one that is found. Children are not necessarily enumerated in the order
 * that they are added. Hence it is very important that names be unique.
 *
 * @param name  A string to identify the node.
 */
void Scene2::removeChildByName(const std::string name) {
    std::shared_ptr<scene2::SceneNode> child = getChildByName(name);
    if (child != nullptr) {
        removeChild(child->_childOffset);
    }
}


/**
 * Removes all children from this Node.
 */
void Scene2::removeAllChildren() {
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        (*it)->setParent(nullptr);
        (*it)->_childOffset = -1;
        (*it)->pushScene(nullptr);
    }
    _children.clear();
}

#pragma mark -
#pragma mark Rendering
/**
 * Draws all of the children in this scene with the given SpriteBatch.
 *
 * This method with draw using {@link #getSpriteBatch}. If not sprite batch
 * has been assigned, nothing will be drawn.
 *
 * Rendering happens by traversing the the scene graph using an "Pre-Order"
 * tree traversal algorithm ( https://en.wikipedia.org/wiki/Tree_traversal#Pre-order ).
 * That means that parents are always draw before (and behind children).
 * To override this draw order, you should place an {@link OrderedNode}
 * in the scene graph to specify an alternative order.
 */
void Scene2::render() {
    if (_batch == nullptr) {
        return;
    } else if (_batch->isDrawing()) {
        _batch->end();
    }
    
    _batch->setPerspective(_camera->getCombined());
    if (_framebuffer) {
        _batch->begin(_framebuffer);
    } else {
        _batch->begin();
    }
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        (*it)->render(_batch, Affine2::IDENTITY, _color);
    }

    _batch->end();
}

#pragma mark -
#pragma mark Resizing
/**
 * Resizes the viewport and adjusts all the children to match
 *
 * This will resize the camera viewport for this scene to match the given
 * size. It will also resize the content size of any immediate child that
 * is (1) unscaled and (2) matches the size of the display or the safe
 * area. Finally, it will relayout those scene nodes, potentially causing
 * UI elements to move about the screen.
 *
 * @param width     The new viewport width
 * @param height    The new viewport height
 */
void Scene2::resizeViewport(float width, float height) {
    _size = Application::get()->getDrawableSize();
    _safearea = Application::get()->getSafeBounds();

    float sx = width/_size.width;
    float sy = height/_size.height;
    _size.set(width,height);
    _safearea.origin.x *= sx;
    _safearea.origin.y *= sy;
    _safearea.size.width  *= sx;
    _safearea.size.height *= sy;
    
    auto ortho = std::dynamic_pointer_cast<OrthographicCamera>(_camera);
    ortho->set(0, 0, _size.width, _size.height);
    ortho->update();
    
    // Relayout UI elements if possible
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        // This forces a resize
        (*it)->setScene(this);
        (*it)->doLayout();
    }
}

/**
 * Resizes the viewport according to the given hint.
 *
 * This will resize the camera viewport for this scene according to the
 * given hint. If either of the values of hint are non-zero, then the scene
 * will lock that dimension to that particular size. If both are non-zero,
 * it will choose its dimension according to the device orientation.
 * Landscape will be height, while portrait will pick width. Devices with
 * no orientation will always priortize height over width.
 *
 * Once the scene is resized, this method will also resize the content size
 * of any immediate child that is (1) unscaled and (2) matches the size of
 * the display or the safe area. Finally, it will relayout those scene
 * nodes, potentially causing UI elements to move about the screen.
 *
 * @param width     The width hint
 * @param height    The height hint
 */
void Scene2::resizeToHint(float width, float height) {
    _size = Application::get()->getDrawableSize();
    _safearea = Application::get()->getSafeBounds();

    if (width > 0 && height > 0) {
        Display::Orientation orient = Display::get()->getDisplayOrientation();
        if (orient == Display::Orientation::PORTRAIT ||
            orient == Display::Orientation::UPSIDE_DOWN) {
            height = 0;
        } else {
            width  = 0;
        }
    }
    
    float scale = 1.0;
    if (width > 0) {
        scale = width/_size.width;
    } else {
        scale = height/_size.height;
    }
    _size *= scale;
    _safearea.origin *= scale;
    _safearea.size   *= scale;
    
    auto ortho = std::dynamic_pointer_cast<OrthographicCamera>(_camera);
    ortho->set(0, 0, _size.width, _size.height);
    ortho->update();
    
    // Relayout UI elements if possible
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        // This forces a resize
        (*it)->setScene(this);
        (*it)->doLayout();
    }
}
    
