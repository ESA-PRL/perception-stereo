#ifndef __SPARSE_STEREO_TYPES_HPP__
#define __SPARSE_STEREO_TYPES_HPP__ 

#include <base/eigen.h>
#include <vector>
#include <base/time.h>
#include <envire/maps/Featurecloud.hpp>

namespace stereo
{
enum DETECTOR
{
    DETECTOR_SURF = 1,
    DETECTOR_GOOD = 2,
    DETECTOR_SURFGPU = 3,
    DETECTOR_STAR = 4,
    DETECTOR_MSER = 5,
    DETECTOR_SIFT = 6,
    DETECTOR_FAST = 7,
};

enum FILTER
{
    FILTER_NONE,
    FILTER_HOMOGRAPHY,
    FILTER_FUNDAMENTAL,
    FILTER_INTELLIGENT,
    FILTER_STEREO,
    FILTER_ISOMETRY,
};

struct DetectorConfiguration
{
    DetectorConfiguration()
	: SURFparam(170),
	goodParam(0.1),
	mserParam(3),
	starParam(9),
	fastParam(12)
    {}

    int SURFparam;
    float goodParam;
    float mserParam;
    float starParam;
    float fastParam;
};

struct FeatureConfiguration
{
    FeatureConfiguration() 
	: debugImage( true ),
	  targetNumFeatures( 100 ),
	  maxStereoYDeviation( 5 ),
	  knn( 1 ),
	  distanceFactor( 2.0 ),
	  isometryFilterMaxSteps( 1000 ),
	  isometryFilterThreshold( 0.1 ),
	  adaptiveDetectorParam( false ),
	  detectorType( DETECTOR_SURF ),
	  filterType( FILTER_STEREO )
    {}

    /** if set to true, the library will generate debug images during the
     * processing of the data
     */
    bool debugImage;

    /** the target number of features from the detector
     */
    int targetNumFeatures;

    /* the difference in pixels, that the stereo matcher allows, so that two
     * features are still considered epipolar
     */
    int maxStereoYDeviation;

    /** number of neares neighbours to check for feature correspondence.  a
     * value of 1 will just check the next neighbour. A value of 2 will check
     * the two nearest neighbours and apply the distanceFactor criterion for
     * filtering correspondences.
     */
    int knn;

    /** only used if knn >=2. For two features to be considered corresponding,
     * the next nearest neighbour needs to be (distance * distanceFactor) away
     * from the current neighbour. A value of 1.0 is equal to having knn = 1. A
     * value of 2.0 will make sure matches are quite unique. Usually something
     * like 1.6 is used.
     */
    int distanceFactor;

    /** maximum Ransac steps the isometry filter should use
     */
    int isometryFilterMaxSteps;

    /** threshold error value for a point to still be considered an inlier in
     * the isometryFilter
     */
    double isometryFilterThreshold;

    bool adaptiveDetectorParam;
    DetectorConfiguration detectorConfig;

    DETECTOR detectorType;
    FILTER filterType;
};

struct StereoFeatureArray
{
    base::Time time;

    typedef float Scalar;
    typedef Eigen::Matrix<Scalar, Eigen::Dynamic, 1, Eigen::DontAlign> Descriptor;

    int descriptorSize;
    envire::DESCRIPTOR descriptorType;

    std::vector<base::Vector3d> points;
    std::vector<envire::KeyPoint> keypoints;
    std::vector<Scalar> descriptors;

    StereoFeatureArray() : descriptorSize(0) {}

    void push_back( const base::Vector3d& point, const envire::KeyPoint& keypoint, const Descriptor& descriptor ) 
    {
	points.push_back( point );
	keypoints.push_back( keypoint );

	if( descriptorSize == 0 )
	    descriptorSize = descriptor.size();

	assert( descriptorSize == descriptor.size() );

	// try to have some efficiency in copying the descriptor data
	descriptors.resize( descriptors.size() + descriptorSize );
	memcpy( &descriptors[0] + descriptors.size() - descriptorSize, descriptor.data(), descriptorSize ); 
    }

    Eigen::Map<Descriptor> getDescriptor( size_t index )
    { 
	return Eigen::Map<Descriptor>( &descriptors[index*descriptorSize], descriptorSize ); 
    }

    Eigen::Map<const Descriptor> getDescriptor( size_t index ) const
    { 
	return Eigen::Map<const Descriptor>( &descriptors[index*descriptorSize], descriptorSize ); 
    }

    size_t size() const 
    { 
	return points.size(); 
    }

    void clear() 
    { 
	descriptorSize = 0;
	points.clear(); 
	descriptors.clear(); 
	keypoints.clear(); 
    }

    void copyTo( envire::Featurecloud& fc ) const
    {
	fc.clear();

	std::copy( points.begin(), points.end(), std::back_inserter( fc.vertices ) );
	fc.keypoints = keypoints;
	fc.descriptors = descriptors;
	fc.descriptorType = descriptorType;
	fc.descriptorSize = descriptorSize;
    }
};
}

#endif
