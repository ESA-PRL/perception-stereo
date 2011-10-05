#include <limits>
#include "opencv2/features2d/features2d.hpp"

namespace cv
{

class CV_EXPORTS_W PSURF : public CvSURFParams
{
public:
    //! the default constructor
    CV_WRAP PSURF();
    //! the full constructor taking all the necessary parameters
    CV_WRAP PSURF(double _hessianThreshold, int _nOctaves=4,
         int _nOctaveLayers=2, bool _extended=false, bool _upright=false);

    //! returns the descriptor size in float's (64 or 128)
    CV_WRAP int descriptorSize() const;
    //! finds the keypoints using fast hessian detector used in SURF
    CV_WRAP_AS(detect) void operator()(const Mat& img, const Mat& mask,
                    CV_OUT vector<KeyPoint>& keypoints) const;
    //! finds the keypoints and computes their descriptors. Optionally it can compute descriptors for the user-provided keypoints
    CV_WRAP_AS(detect) void operator()(const Mat& img, const Mat& mask,
                    CV_OUT vector<KeyPoint>& keypoints,
                    CV_OUT vector<float>& descriptors,
                    bool useProvidedKeypoints=false) const;
};

/*
 * SurfDescriptorExtractor
 */
class CV_EXPORTS PSurfDescriptorExtractor : public DescriptorExtractor
{
public:
    PSurfDescriptorExtractor( int nOctaves=4, int nOctaveLayers=2, bool extended=false, bool upright=false );

    virtual void read( const FileNode &fn );
    virtual void write( FileStorage &fs ) const;

    virtual int descriptorSize() const;
    virtual int descriptorType() const;

protected:
    virtual void computeImpl( const Mat& image, vector<KeyPoint>& keypoints, Mat& descriptors ) const;

    PSURF surf;
};

}
