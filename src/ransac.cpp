#include "ransac.hpp"
#include <math.h> 
#include <boost/concept_check.hpp>

using namespace stereo::ransac;
using namespace Eigen;


void Pairs::add( const Vector3d& a, const Vector3d& b, double dist )
{
    pair pair;
    pair.index = pairs.size();
    pair.distance = dist;

    x.push_back( a );
    p.push_back( b );
    pairs.push_back( pair );
}

double Pairs::trim( size_t n_po )
{
    // sort the pairs by distance
    std::sort( pairs.begin(), pairs.end() );

    if( n_po < pairs.size() )
    {
	pairs.resize( n_po );
    }

    if( pairs.size() > 0 )
    {
	// and set the maximum distance as the next d_box value
	return pairs.back().distance;
    }
    else {
	return std::numeric_limits<double>::quiet_NaN();
    }
}

Affine3d Pairs::getTransform()
{
    if( size() < MIN_PAIRS )
	throw std::runtime_error("not enough pairs to get transform");

    // calculate the mean and covariance values of x and p
    Vector3d mu_p(Vector3d::Zero()), 
	     mu_x(Vector3d::Zero());
    Matrix3d sigma_px(Matrix3d::Zero());
    double mu_d = 0;


    for(size_t i=0;i<pairs.size();i++) {
	const size_t idx = pairs[i].index;
	Vector3d &pv( p[idx] );
	Vector3d &xv( x[idx] );

	const double d = pairs[i].distance;
	mu_d += d*d;

	mu_p += pv;
	mu_x += xv;

	sigma_px += pv * xv.transpose();
    }
    const double n_inv = 1.0/pairs.size();
    mu_p *= n_inv;
    mu_x *= n_inv;
    mu_d *= n_inv;

    sigma_px = sigma_px * n_inv - mu_p*mu_x.transpose();

    // form the symmetric 4x4 matrix Q
    Matrix3d A = sigma_px-sigma_px.transpose();
    Vector3d delta = Vector3d( A(1,2), A(2,0), A(0,1) );

    Matrix4d q_px;
    q_px << sigma_px.trace(), delta.transpose(), 
	 delta, A - Matrix3d::Identity() * sigma_px.trace();

    // do an eigenvalue decomposition of Q
    Quaterniond q_R;
    EigenSolver<Matrix4d> eigenSolver(q_px);
    double max_eig = eigenSolver.eigenvalues().real().maxCoeff();
    for(int i=0;i<eigenSolver.eigenvalues().rows();i++) {
	if(eigenSolver.eigenvalues()(i) == max_eig) {
	    Vector4d max_eigv = eigenSolver.eigenvectors().col(i).real();
	    q_R = Quaterniond( max_eigv(0), max_eigv(1), max_eigv(2), max_eigv(3) );
	    break;
	}
    }

    // resulting transformation that will align p to x, if applied to p 
    Vector3d q_T = mu_x - q_R * mu_p;
    Affine3d t( Translation3d( q_T ) * q_R );

    mse = mu_d;
    
    return t;
}

size_t Pairs::size() const 
{
    return pairs.size();
}

double Pairs::getMeanSquareError() const
{
    return mse;
}

void Pairs::clear()
{
    pairs.clear();
    x.clear();
    p.clear();
}

