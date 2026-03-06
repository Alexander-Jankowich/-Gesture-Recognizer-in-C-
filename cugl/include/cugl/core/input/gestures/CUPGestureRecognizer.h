//  CUPGestureRecognizer.h
//  Cornell University Game Library (CUGL)
//
//  This module implements support for DollarGestures, built upon the CUGL
//  Path2 interface. This divorces DollarGesture recognition from the device
//  input, focusing solely on the geometry. This module is based upon the
//  following work:
//
//  Dollar Gestures
//  Wobbrock, J.O., Wilson, A.D.and Li, Y. (2007). Gestures without libraries,
//  toolkits or training: A $1 recognizer for user interface prototypes.
//  Proceedings of the ACM Symposium on User Interface Software and Technology
//  (UIST '07). Newport, Rhode Island (October 7 - 10, 2007). New York :
//  ACM Press, pp. 159 - 168.
//  https ://dl.acm.org/citation.cfm?id=1294238
//
//
// $P+ Recognizer 
//     Vatavu, R.-D. (2017). Improving gesture recognition accuracy on
//     touch screens for users with low vision. Proceedings of the ACM
//     Conference on Human Factors in Computing Systems (CHI '17). Denver,
//     Colorado (May 6-11, 2017). New York: ACM Press, pp. 4667-4679.
//     https ://dl.acm.org/citation.cfm?id=3025941
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
//  Authors: Alexander Jankowich
//  Version: 3/5/26 ($P+ Integration)

#ifndef __CU_PGESTURE_RECOGNIZER_H__
#define __CU_PGESTURE_RECOGNIZER_H__

#include <cugl/core/math/CUVec2.h>
#include <cugl/core/math/CUPath2.h>
#include <vector>
#include <unordered_map>

namespace cugl {

    //PointCloudPoint struct stores extra angle and id
    struct PointCloudPoint {
    Vec2 position;
    float angle = 0.0;
    int id; 
    };
 /**
 * A class representing a PointCloud gesture
 *
 * This object represents a normalized unistroke gesture stored inside of
 * a gesture recognizer. As the normalization algorithm is deteremined by
 * the current settings, there are no publicly accessible constructors for
 * this class.
 */
class PointCloudGesture {
private:
    /** Identifier string for a gesture **/
    std::string _name;

    /** Point Cloud Points */
    std::vector<PointCloudPoint> _points;

private:
    /**
     * Creates a gesture with a given name and points
     *
     * @param name      The gesture name
     * @param points    The gesture point cloud
     */
    PointCloudGesture(const std::string name, const std::vector<PointCloudPoint> cloud) {
        _name = name;
        _points = cloud;
    }
    
    /**
     * Sets this gesture to have the given points
     *
     * @param points    The gesture point sequence
     */
    void setPoints(const std::vector<PointCloudPoint> points) {
        _points = points;
    }
    // All access to the constructors
    friend class PGestureRecognizer;

public:
    /**
     * Returns the string identifier of this gesture
     *
     * @return the string identifier of this gesture
     */
    const std::string getName() const { return _name; }

    /**
     * Returns the vector of 2D points representing this gesture
     *
     * @return the vector of 2D points representing this gesture
     */
    const std::vector<PointCloudPoint>& getPoints() const { return _points; }
    

};
/**
 * A class representing the $P+ gesture recognition engine.
 *
 * This class contains all the required funtionality needed for recognizing
 * user defined gestures. It's major responsibilities are to store a collection
 * of template gestures (for comparison)/
 * Gesture matches are determined by computing a similarity score
 * between point clouds
 * The recognition algorithm uses $P+ for more information on this algorithm look
 * here:
 *
 * https ://dl.acm.org/citation.cfm?id=3025941
 *
 * Both of these algorithms are rotationally oblivious, meaning that they
 * can recognize the gestures at any orientation. Typically this is not what
 * we want in game development, however. Therfore, this recognizer includes
 * the option to reject any matches whose angles of rotation exceed a certain
 * threshold. See {@link #getOrientationTolerance} for more information.
 *
 * Similarity is determined on a scale of 0 to 1 where 1 is a complete match
 * and 0 is no match at all. A pure 0 is difficult to achieve. By default, we
 * consider any gesture a possible match if it has a similarity of at least
 * 0.8.
 */
class PGestureRecognizer {
private:
    /* The default number of point clouds*/
    const int NUM_POINTS_CLOUDS = 16;
    /* The default number of points ($P+)*/
    const int NUM_POINTS = 32;

    std::unordered_map<std::string,std::vector<PointCloudGesture>> _templates;
    float _accuracy = 0.8f; 
    size_t _normlength = NUM_POINTS;
    Vec2 _normcenter = Vec2(0.0f, 0.0f); 

public:
    bool init();    
    static std::shared_ptr<PGestureRecognizer> alloc() {
    std::shared_ptr<PGestureRecognizer> result = std::make_shared<PGestureRecognizer>();
    return (result->init() ? result : nullptr);
    }   

    /**
    * Empties the recongizer of all gestures and resets all attributes.
    *
    * This will set the sample size to 0, meaning no future matches are
    * possible. You must reinitialize the object to use it.
    */
    void dispose() {
        _templates.clear();
        _accuracy = 0.0f;
        _normlength = NUM_POINTS;
        _normcenter = Vec2(0,0);
    }

    /**
     * Sets the accuracy score required to recognize a gesture
     * 
     * @param acc the desired accuracy score [0,1]
     * 
    */
    void set_accuracy(float acc){
        _accuracy = acc;
    }

    #pragma mark Gesture Matching
    /**
     * Returns the name of the gesture with the closest match to the given one.
     *
     * The match will be performed using the current active algorithm. If
     * there is no match within the similarity threshold or orientation
     * tolerance, this method will return the empty string. A gesture must
     * consist of at least two points.
     *
     * @param points    a vector of points representing a candidate gesture.
     *
     * @return the name of the gesture with the closest match to the given one.
     */
    const std::string match(const std::vector<std::vector<Vec2>>& gesture);
      
    /**
     * Returns the similarity measure of the named gesture to this one.
     *
     * The similarity measure will be computed using the active algorithm. As
     * those algorithms are rotationally invariant, it will ignore the rotation
     * when computing that value. However, if the parameter invariant is set to
     * false, this method will return 0 for gestures not within the orientation
     * tolerance.
     *
     * If there is no gesture of the given name, this method will return 0. A
     * gesture must consist of at least two points.
     *
     * @param name      the name of the gesture to compare
     * @param points    a vector of points representing a candidate gesture
     *
     * @return the similarity measure of the named gesture to this one.
     */
    float similarity(const std::string name, const std::vector<std::vector<Vec2>>& points);
 
    
    
#pragma mark Gesture Management
    /**
     * Adds the given gesture to this recognizer using the given name.
     *
     * The gesture will be normalized before storing it. If the gesture has the
     * same name as an existing one, the previous gesture will be replaced.
     *
     * If the optional parameter unique is set to true, this method will first
     * check that the gesture is unique (e.g. it does not exceed the similarity
     * threshold when compared to any existing gestures) before adding it. If
     * the gesture is too close to an existing one, this method will return false.
     *
     * Note that uniqueness is determined according the current algorithm,
     * similarity threshold, and orientation tolerance. If any of these values
     * change, then uniqueness is no longer guaranteed.
     *
     * @param name      the gesture name
     * @param points    a vector of points representing a gesture
     *
     * @return true if the gesture was added to this recognizer
     */
    bool addGesture(const std::string name, const std::vector<std::vector<Vec2>>& points);
  
};  
}

#endif
