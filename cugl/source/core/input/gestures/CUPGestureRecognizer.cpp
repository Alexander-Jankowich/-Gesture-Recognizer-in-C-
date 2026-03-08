//  CUPGestureRecognizer.cpp
//  Cornell University Game Library (CUGL)
//
//  This module implements support for $P+ algorithm.
//  This divorces DollarGesture recognition from the device
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


#include <cugl/core/assets/CUJsonValue.h>
#include <cugl/core/util/CUDebug.h>
#include <cugl/core/input/gestures/CUPGestureRecognizer.h>
#include <limits>
using namespace cugl;


#pragma mark -
#pragma mark Static $P+ helpers
/**
 *  Distance with Angle helper function
 * 
 * @param p1 first point 
 * @param p2 second point
 * 
 * @return float distance between points taking angle into account
 */
static float distance_with_angle(const PointCloudPoint& p1, const PointCloudPoint& p2){
    float dx = p2.position.x - p1.position.x;
    float dy = p2.position.y - p1.position.y;
    float da = p2.angle - p1.angle;

    return std::sqrt(dx * dx + dy * dy + da * da);
}

/**
 * Euclidean distance helper function
 * 
 * @param p1 first point
 * @param p2 second points
 * 
 * @return float distance between points 
 */
static float distance(const PointCloudPoint& p1, const PointCloudPoint& p2){
    float dx = p2.position.x - p1.position.x;
    float dy = p2.position.y - p1.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

/**
 * Length traversed by a point path
 * 
 * @param points vector of point cloud points
 * 
 * @return length traversed by point path
 * 
 */
static float cloud_path_length(const std::vector<PointCloudPoint>& points){
    float d = 0.0;
    for(int i = 1; i < points.size(); i++){
        if (points[i].id == points[i - 1].id){
            d += distance(points[i - 1], points[i]);
        }
    }
    return d;
}

/**
 * Centroid of the cloud point path
 * 
 * @param points vector of point cloud points
 * 
 * @return centroid
 */
static PointCloudPoint cloud_centroid(const std::vector<PointCloudPoint>& points){
    float x = 0.0;
    float y = 0.0;
    for(int i = 0; i < points.size(); i++){
        x += points[i].position.x;
        y += points[i].position.y;
    }
    x /= points.size();
    y /= points.size();
    PointCloudPoint result;
    result.angle = 0;
    result.position = Vec2(x,y);
    return result;

}

/**
 * Translates points to pt relative to centroid
 * 
 * @param points vector of point cloud points
 * 
 * @return points translated to centroid
 */
static std::vector<PointCloudPoint> cloud_translate(std::vector<PointCloudPoint> points,
PointCloudPoint pt){
    PointCloudPoint centroid = cloud_centroid(points);
    std::vector<PointCloudPoint> result;
    for(int i = 0; i < points.size(); i++){
        float qx = points[i].position.x + pt.position.x - centroid.position.x;
        float qy = points[i].position.y + pt.position.y - centroid.position.y;
        PointCloudPoint t_point;
        t_point.position = Vec2(qx,qy);
        t_point.angle = points[i].angle;
        t_point.id = points[i].id;
        result.push_back(t_point);
    }
    return result;
}

/**
 * Computes normalized turning angles 
 * 
 * @param points vector of point cloud points
 * 
 * @return points with normalized turning angles
 */
static std::vector<PointCloudPoint> compute_normalized_angles(
    const std::vector<PointCloudPoint>& points){
        std::vector<PointCloudPoint> new_points;
        new_points.push_back(points[0]);
        for (int i = 1; i < points.size() - 1; i++){
            float dx = (points[i + 1].position.x - points[i].position.x) *
            (points[i].position.x - points[i - 1].position.x);
            float dy = (points[i + 1].position.y- points[i].position.y) *
            (points[i].position.y - points[i - 1].position.y);
            float dn = distance(points[i + 1],points[i]) * 
            distance(points[i], points[i - 1]);
            if (dn < 1e-6f) dn = 1e-6f;
            float cosangle = std::max(-1.0f, std::min(1.0f, (dx + dy) / dn));
            float angle = std::acos(cosangle) / M_PI;
            PointCloudPoint new_point;
            new_point.position = Vec2(points[i].position.x, points[i].position.y);
            new_point.id = points[i].id;
            new_point.angle = angle;
            new_points.push_back(new_point);
        }
        PointCloudPoint new_point;
        new_point.position = points[points.size() - 1].position;
        new_point.id = points[points.size() - 1].id;
        new_points.push_back(new_point);
        return new_points;
    }



/** Resamples cloud points so they have exactly n even length points 
 * in path
 * 
 * @param points vector of point cloud points
 * @param n number of points to resample to
 * 
 * @return resampled points
*/
static std::vector<PointCloudPoint> cloud_resample(std::vector<PointCloudPoint> points, int n) {
    float I = cloud_path_length(points) / (n - 1); 
    float pathLen = cloud_path_length(points);
    if (I < 1e-6f) return points;
    float D = 0.0f;
    std::vector<PointCloudPoint> result;
    result.push_back(points[0]);
    for (int i = 1; i < points.size(); i++) {
        if ((int)result.size() >= n) break;

        if (points[i].id == points[i - 1].id) {
            float d = distance(points[i - 1], points[i]);
            if ((D + d) >= I) {
                float t = (I - D) / d;
                float qx = points[i - 1].position.x + 
                           t * (points[i].position.x - points[i - 1].position.x);

                float qy = points[i - 1].position.y + 
                           t * (points[i].position.y - points[i - 1].position.y);

                PointCloudPoint q_point;
                q_point.position = Vec2(qx, qy);
                q_point.id = points[i].id;
                q_point.angle = 0; 

                result.push_back(q_point);

                points.insert(points.begin() + i, q_point);

                D = 0.0f;
                i--;   
            }
            else {
                D += d;
            }
        }
    }
    if (result.size() == n - 1) {
        result.push_back(points.back());
    }

    return result;
}

/**
 * Responsible for normalizing the gesture and removing the size difference 
 * between all gestures.
 * 
 * @param points the point cloud to be normalized
 * 
 * @return the new normalized point cloud.
 */
static std::vector<PointCloudPoint> cloud_scale(const std::vector<PointCloudPoint>& points){
    float minX = INFINITY;
    float maxX = -INFINITY;
    float minY = INFINITY;
    float maxY = -INFINITY;
    for(int i = 0; i < points.size(); i++){
        minX = std::min(minX, points[i].position.x);
        maxX = std::max(maxX, points[i].position.x);
        minY = std::min(minY, points[i].position.y);
        maxY = std::max(maxY, points[i].position.y);
    }
    float size = std::max(maxX - minX, maxY - minY);
    if (size < 1e-6f) size = 1.0f;
    std::vector<PointCloudPoint> result;
    for (int i = 0; i < points.size(); i++){
        float qx = (points[i].position.x - minX) / size;
        float qy = (points[i].position.y - minY) / size;
        PointCloudPoint new_point;
        new_point.position = Vec2(qx,qy);
        new_point.id = points[i].id;
        result.push_back(new_point);
    }
    return result;

 }

/**
 * Computes the cloud_distance used in $P+ algorithm
 * uses euclidenan distance with angle to compute the 
 * total distance between the points and match points with their
 * closest partner.
 * 
 * @param pts1 the first point cloud
 * @param pts2 the second point cloud
 * 
 * @return the distance between the two point clouds 
 */
static float cloud_distance(const std::vector<PointCloudPoint>& pts1, 
                            const std::vector<PointCloudPoint>& pts2)
{
    if (pts1.empty() || pts2.empty()) return INFINITY; 

    std::vector<bool> matched(pts2.size(), false); 
    float sum = 0.0f;

    for (int i = 0; i < pts1.size(); i++) {
        int index = -1;
        float minDist = INFINITY;
        for (int j = 0; j < pts2.size(); j++) {     
            float d = distance_with_angle(pts1[i], pts2[j]);    
            if (d < minDist && !matched[j]) {     
                minDist = d;
                index = j;
            }
        }
        if (index != -1) {               
            matched[index] = true;
            sum += minDist;
        }
    }
    for (int i = 0; i < matched.size(); i++) {
        if (!matched[i]) {
            float minDist = INFINITY;
            for (int j = 0; j < pts1.size(); j++) {
                minDist = std::min(minDist, distance_with_angle(pts1[j], pts2[i]));
            }
            sum += minDist;
        }
    }

    return sum;
}

/**
 * Adds a unistroke gesture to the point cloud. Note that
 * an id must be provided to specify the stroke in the gesture
 * that the points belongs to. 
 * 
 * @param points the stroke to be converted
 * @param cloud the point cloud to be modified
 * @param stroke_id the id of the stroke
 */
static void convert_unistroke(const std::vector<Vec2>& points,
    std::vector<PointCloudPoint>& cloud,int stroke_id){
    for(auto& p : points){
        PointCloudPoint pt;
        pt.position = p;
        pt.angle = 0.0f;
        pt.id = stroke_id;
        cloud.push_back(pt);
    }
}

/**
 * Converts a multi stroke gesture into a point cloud returning 
 * the constructed point cloud
 * 
 * @param strokes a vector of strokes 
 * 
 * @return the newly constructed point cloud
*/
static std::vector<PointCloudPoint> convert_multi(
    const std::vector<std::vector<Vec2>>& strokes){
    std::vector<PointCloudPoint> cloud;
    int id = 0;
    for(auto& stroke : strokes){
        convert_unistroke(stroke, cloud, id);
        id += 1;
    }
    return cloud;
}

#pragma mark -
#pragma mark Gesture Matching

bool PGestureRecognizer::init() {
    _normlength = NUM_POINTS;
    _normcenter = Vec2(0.0f, 0.0f);
    _accuracy = 0.8f;
    _templates.clear();
    return true;
}

/**
 * Returns the name of the gesture with the closest match to the given one.
 *
 * The match will be performed using the $P+ algorithm. If
 * there is no match within the similarity threshold or orientation
 * tolerance, this method will return the empty string. A gesture must
 * consist of at least two points and at least one stroke
 *
 * @param points    a vector of points representing a candidate gesture.
 * 
 * @return the name of the gesture with the closest match to the given one.
 */
const std::string PGestureRecognizer::match(const std::vector<std::vector<Vec2>>& gesture) {
    if (_templates.empty() || gesture.empty()){
        return "";
    } 

    // Convert multi-stroke gesture to point cloud
    std::vector<PointCloudPoint> candidate = convert_multi(gesture);
    candidate = cloud_resample(candidate, _normlength);
    candidate = cloud_scale(candidate);
    candidate = cloud_translate(candidate, PointCloudPoint{_normcenter, 0, 0});
    candidate = compute_normalized_angles(candidate);

    float bestDist = std::numeric_limits<float>::max();
    std::string bestMatch = "";
    for (auto& pair : _templates) {
        for (auto& tmpl : pair.second) {
            float dist = cloud_distance(candidate, tmpl.getPoints());
            if (dist < bestDist) {
                bestDist = dist;
                bestMatch = pair.first;
            }
        }
    }

    float similarity = 1.0f - (bestDist / (float)_normlength);
    if (similarity < _accuracy) {
        return ""; 
    }

    return bestMatch;
}

/**
 * Returns the similarity measure of the named gesture to this one.
 *
 * The similarity measure will be computed using the $P+ algorithm. This 
 * algorithm is rotationally inv
 *
 * If there is no gesture of the given name, this method will return 0. A
 * gesture must consist of at least two points.
 *
 * @param name      the name of the gesture to compare
 * @param points    a vector of points representing a candidate gesture
 *
 * @return the similarity measure of the named gesture to this one.
*/
float PGestureRecognizer::similarity(
    const std::string name,
    const std::vector<std::vector<Vec2>>& points
) {
    auto it = _templates.find(name);
    if (it == _templates.end()) return 0.0f;

    std::vector<PointCloudPoint> candidate = convert_multi(points);
    candidate = cloud_resample(candidate, _normlength);
    candidate = cloud_scale(candidate);
    candidate = cloud_translate(candidate, PointCloudPoint{_normcenter,0,0});
    candidate = compute_normalized_angles(candidate);

    float bestScore = 0.0f;
    for (auto& tmpl : it->second) {
        float dist = cloud_distance(candidate, tmpl.getPoints());
        float score = 1.0f - (dist / (float)_normlength);
        if (score > bestScore) bestScore = score;
    }

    return bestScore;
}

#pragma mark -
#pragma mark Gesture Management

/**
 * Adds a new gesture template to the recognizer.
 *
 * @param name     the gesture name
 * @param points   a vector of strokes (multi-stroke)
 *
 * @return true if added successfull
 */
bool PGestureRecognizer::addGesture(
    const std::string name,
    const std::vector<std::vector<Vec2>>& points) {
    CUAssertLog(points.size() >= 1, "A gesture must have at least one stroke");
    CUAssertLog(points[0].size() > 2, "A gesture must have at least two points");

    std::vector<PointCloudPoint> cloud = convert_multi(points);
    cloud = cloud_resample(cloud, _normlength);
    cloud = cloud_scale(cloud);
    cloud = cloud_translate(cloud, PointCloudPoint{_normcenter,0,0});
    cloud = compute_normalized_angles(cloud);

    PointCloudGesture gesture(name, cloud);
    _templates[name].push_back(gesture);

    return true;
}

