
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */



// This is mainly a test for object tracking




#ifndef FINDOBJECT_H
#define FINDOBJECT_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "findobjectwindow.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

#include "opencv2/core/core_c.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/legacy/blobtrack.hpp"

class FindObjectMain;
class FindObjectWindow;

using namespace cv;




// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 200

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

#define MIN_LAYER 0
#define MAX_LAYER 255

#define MIN_BLEND 1
#define MAX_BLEND 100

// Sizes must be quantized a certain amount for OpenCV
#define QUANTIZE 8

// Storage for results
#define FINDOBJECT_FILE "/tmp/findobject"

#define MIN_CAMSHIFT 0
#define MAX_CAMSHIFT 256


#define NO_ALGORITHM 0
#define ALGORITHM_SURF 1
#define ALGORITHM_CAMSHIFT 2
#define ALGORITHM_BLOB 3

class FindObjectConfig
{
public:
	FindObjectConfig();

	int equivalent(FindObjectConfig &that);
	void copy_from(FindObjectConfig &that);
	void interpolate(FindObjectConfig &prev, 
		FindObjectConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();

	int global_range_w;
	int global_range_h;
// Block size as percent of image size
// Object must be a rectangle for the algorithm to work,
// so no oblique edges unless we also do an affine transform.
	float global_block_w;
	float global_block_h;
// Block position in percentage 0 - 100
	float block_x;
	float block_y;
// Draw key points
	int draw_keypoints;
// Draw border over object in scene layer
	int draw_border;
// Draw transparency over object in object layer
	int replace_object;
// Draw border over object
	int draw_object_border;

// Which layer is the object 0 or 1
	int object_layer;
// Which layer replaces the object
	int replace_layer;
// Which layer is the object searched for in
	int scene_layer;

	int algorithm;
// Camshift parameters
	int vmin, vmax, smin;
// Amount to blend new object position in
	int blend;
};




class FindObjectMain : public PluginVClient
{
public:
	FindObjectMain(PluginServer *server);
	~FindObjectMain();

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	void draw_vectors(VFrame *frame);
	int is_multichannel();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
// Calculate frame to copy from and frame to move
	void calculate_pointers(VFrame **frame, VFrame **src, VFrame **dst);
	void allocate_temp(int w, int h, int color_model);

	PLUGIN_CLASS_MEMBERS2(FindObjectConfig)


	AffineEngine *affine;
// Temporary for affine overlay
	VFrame *temp;
	OverlayFrame *overlayer;

	static void draw_pixel(VFrame *frame, int x, int y);
	static void draw_line(VFrame *frame, int x1, int y1, int x2, int y2);
	static void draw_rect(VFrame *frame, int x1, int y1, int x2, int y2);

	void grey_crop(unsigned char *dst, 
		VFrame *src, 
		int x1, 
		int y1,
		int x2,
		int y2,
		int dst_w,
		int dst_h);



	void process_surf();
	void process_camshift();
	void process_blob();



// clamped coordinates
	int object_w;
	int object_h;
	int object_x1;
	int object_y1;
	int object_x2;
	int object_y2;
	int scene_w;
	int scene_h;
	int scene_x1;
	int scene_y1;
	int scene_x2;
	int scene_y2;
// input frame size
	int w, h;
// clamped layers
	int object_layer;
	int scene_layer;
	int replace_layer;

// Latest coordinates of object in scene
	int border_x1;
	int border_y1;
	int border_x2;
	int border_y2;
	int border_x3;
	int border_y3;
	int border_x4;
	int border_y4;
// Coordinates of object in scene with blending
	float border_x1_accum;
	float border_y1_accum;
	float border_x2_accum;
	float border_y2_accum;
	float border_x3_accum;
	float border_y3_accum;
	float border_x4_accum;
	float border_y4_accum;
	int init_border;


	IplImage *object_image;
	IplImage *scene_image;


// Comparison with current object_image
	unsigned char *prev_object;
// Quantized sizes
	int object_image_w;
	int object_image_h;
	int scene_image_w;
	int scene_image_h;
	CvSeq *object_keypoints;
	CvSeq *object_descriptors;
	CvSeq *scene_keypoints;
	CvSeq *scene_descriptors;
	CvMemStorage *storage;

// camshift
// object histogram
	Mat hist;
	Rect trackWindow;


// Blob
	int blob_initialized;
	CvBlobTrackerAutoParam1 blob_param;
	CvBlobTrackerAuto* blob_pTracker;
	
	
};










#endif






