#ifndef __STEREO_RANSAC_HPP__
#define __STEREO_RANSAC_HPP__

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <Eigen/LU> 
#include <Eigen/Eigenvalues> 

namespace stereo
{
namespace ransac
{

typedef std::vector<size_t> vector_size_t;


class Pairs
{
public:
    const static unsigned int MIN_PAIRS = 3;

    /** add a single pair, and the distance between that a and b
     */
    void add( const Eigen::Vector3d& a, const Eigen::Vector3d& b, double dist );

    /** trim the pairs to the @param n_po pairs with the lowest distance.
     * Will @return the largest distance of those @param n_po pairs.
     */
    double trim( size_t n_po );

    /** will return the transform that has to be applied to B, so that the MSE
     * between the invididual pairs of A and B is minimized.
     */
    Eigen::Affine3d getTransform();

    double getMeanSquareError() const;

    /** will return the number of pairs in the object
     */
    size_t size() const;

    /** remove all pairs */
    void clear();

public:
    std::vector<Eigen::Vector3d> x, p;

    struct pair
    {
        size_t index;
        double distance;

        bool operator < ( const pair other ) const
        {
            return distance < other.distance;
        }
    };
    std::vector<pair> pairs;

    double mse;
};

struct FitTransform
{
    typedef Eigen::Affine3d Model;
    typedef double Real;

    const std::vector<Eigen::Vector3d>& x, p;
    double errorThreshold;

    FitTransform( const std::vector<Eigen::Vector3d>& x, const std::vector<Eigen::Vector3d>& p, double errorThreshold = 0.1 )
	: x( x ), p( p ), errorThreshold( errorThreshold ) 
    {
	assert( x.size() == p.size() );
    }

    virtual ~FitTransform() {};

    size_t getSampleCount( void ) const
    {
	return x.size();
    }

    bool fitModel( const vector_size_t& useIndices, Eigen::Affine3d& model ) const
    {
	if( useIndices.size() < 3 )
	{
	    std::cout << useIndices.size() << std::endl;
	    return false;
	}

	stereo::ransac::Pairs pairs;
	for( size_t i = 0; i < useIndices.size(); i++ )
	{
	    const size_t index = useIndices[i];
	    const Eigen::Vector3d& v1 = x[index];
	    const Eigen::Vector3d& v2 = p[index];
	    pairs.add( v1, v2, (v2-v1).norm() ); 
	}

	// get the model
	Eigen::Affine3d m = pairs.getTransform();

	// test if the model is valid 
	for( size_t i = 0; i < useIndices.size(); i++ )
	{
	    double dist = testSample( useIndices[i], m ); 
	    if( dist > errorThreshold )
	    {
		return false;
	    }
	}

	model = m;
	return true;
    }

    virtual double testSample( size_t index, const Eigen::Affine3d& model ) const
    {
	const Eigen::Vector3d& v1 = x[index];
	const Eigen::Vector3d& v2 = model * p[index];

	const double dist = (v2-v1).norm(); 
	return dist;
    }
};

struct FitTransformUncertain : public FitTransform
{
    const std::vector<float>& x_e, p_e;

    FitTransformUncertain( const std::vector<Eigen::Vector3d>& x, const std::vector<Eigen::Vector3d>& p, 
	    const std::vector<float>& x_e, const std::vector<float>& p_e, double errorThreshold = 0.1 )
	: FitTransform( x, p, errorThreshold ), x_e( x_e ), p_e( p_e )
    {
    }

    virtual ~FitTransformUncertain() {};

    virtual double testSample( size_t index, const Eigen::Affine3d& model ) const
    {
	const Eigen::Vector3d& v1 = x[index];
	const Eigen::Vector3d& v2 = model * p[index];

	const float e1 = x_e[index];
	const float e2 = p_e[index];

	// TODO this is a very crude normalization for the error
	const double dist = (v2-v1).norm() / sqrt(pow(e1,2) + pow(e2,2)); 
	return dist;
    }
};

//
// pickRandomIndex and ransacSingleModel are copied from MRPT:
//
// http://code.google.com/p/mrpt/
//

template <typename T>
void pickRandomIndex( T p_size, T p_pick, vector_size_t& p_ind )
{
    assert( p_size >= p_pick );

    vector_size_t a( p_size );
    for( size_t i = 0; i < p_size; i++ )
	a[i] = i;

    std::random_shuffle( a.begin(), a.end() );
    p_ind.resize( p_pick );
    for( size_t i = 0 ; i < p_pick; i++ )
	p_ind[i] = a[i];
}

template<typename TModelFit>
bool ransacSingleModel( const TModelFit& p_state,
	size_t p_kernelSize,
	const typename TModelFit::Real& p_fitnessThreshold,
	typename TModelFit::Model& p_bestModel,
	vector_size_t& p_inliers,
        size_t hardIterLimit = 100 )
{
    size_t bestScore = 0;
    size_t iter = 0;
    size_t softIterLimit = 1; // will be updated by the size of inliers
    size_t nSamples = p_state.getSampleCount();
    vector_size_t ind( p_kernelSize );

    while ( iter < softIterLimit && iter < hardIterLimit )
    {
	bool degenerate = true;
	typename TModelFit::Model currentModel;
	size_t i = 0;
	while ( degenerate )
	{
	    pickRandomIndex( nSamples, p_kernelSize, ind );
	    degenerate = !p_state.fitModel( ind, currentModel );
	    i++;
	    if( i > hardIterLimit )
		return false;
	}

	vector_size_t inliers;

	for( size_t i = 0; i < nSamples; i++ )
	{
	    if( p_state.testSample( i, currentModel ) < p_fitnessThreshold )
		inliers.push_back( i );
	}
	assert( inliers.size() > 0 );

	// Find the number of inliers to this model.
	const size_t ninliers = inliers.size();

	if ( ninliers > bestScore )
	{
	    bestScore = ninliers;
	    p_bestModel = currentModel;
	    p_inliers = inliers;

	    // Update the estimation of maxIter to pick dataset with no outliers at propability p
	    float f =  ninliers / static_cast<float>( nSamples );
	    float p = 1 -  pow( f, static_cast<float>( p_kernelSize ) );
	    float eps = std::numeric_limits<float>::epsilon();
	    p = std::max( eps, p);	// Avoid division by -Inf
	    p = std::min( 1-eps, p);	// Avoid division by 0.
	    softIterLimit = log(1-p) / log(p);
	}

	iter++;
    }

    return true;
}

}
}

#endif
