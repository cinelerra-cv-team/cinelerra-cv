#ifndef SURFSCAN_H
#define SURFSCAN_H


// Wrapper & extra functions for object tracking




#ifdef __cplusplus
extern "C"
{
#endif

#include "opencv2/core/core_c.h"


int locatePlanarObject(const CvSeq* objectKeypoints, 
	const CvSeq* objectDescriptors,
    const CvSeq* imageKeypoints, 
	const CvSeq* imageDescriptors,
    const CvPoint src_corners[4], 
	CvPoint dst_corners[4],
	int *(*point_pairs),
	int (*total_pairs));

void locate_points(const CvSeq* objectKeypoints, 
	const CvSeq* objectDescriptors,
    const CvSeq* imageKeypoints, 
	const CvSeq* imageDescriptors,
	int *(*points),
	int *(*sizes),
	int *total_points);


#ifdef __cplusplus

}
#endif




#endif


