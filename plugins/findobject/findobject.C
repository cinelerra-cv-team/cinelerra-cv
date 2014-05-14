
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

#include "affine.h"
#include "cicolors.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "findobject.h"
#include "findobjectwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "surfscan.h"
#include "transportque.h"

#include "opencv2/video/tracking.hpp"
#include "opencv2/video/background_segm.hpp"


#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN(FindObjectMain)



FindObjectConfig::FindObjectConfig()
{
	global_range_w = 5;
	global_range_h = 5;
	draw_keypoints = 1;
	draw_border = 1;
	replace_object = 0;
	draw_object_border = 1;
	global_block_w = MIN_BLOCK;
	global_block_h = MIN_BLOCK;
	block_x = 50;
	block_y = 50;
	object_layer = 0;
	replace_layer = 1;
	scene_layer = 2;
	algorithm = NO_ALGORITHM;
	vmin = 10;
	vmax = 256;
	smin = 30;
	blend = 100;
}

void FindObjectConfig::boundaries()
{
	CLAMP(global_range_w, MIN_RADIUS, MAX_RADIUS);
	CLAMP(global_range_h, MIN_RADIUS, MAX_RADIUS);
	CLAMP(global_block_w, MIN_BLOCK, MAX_BLOCK);
	CLAMP(global_block_h, MIN_BLOCK, MAX_BLOCK);
	CLAMP(block_x, 0, 100);
	CLAMP(block_y, 0, 100);
	CLAMP(object_layer, MIN_LAYER, MAX_LAYER);
	CLAMP(replace_layer, MIN_LAYER, MAX_LAYER);
	CLAMP(scene_layer, MIN_LAYER, MAX_LAYER);
	CLAMP(vmin, MIN_CAMSHIFT, MAX_CAMSHIFT);
	CLAMP(vmax, MIN_CAMSHIFT, MAX_CAMSHIFT);
	CLAMP(smin, MIN_CAMSHIFT, MAX_CAMSHIFT);
	CLAMP(blend, MIN_BLEND, MAX_BLEND);
}

int FindObjectConfig::equivalent(FindObjectConfig &that)
{
	int result = 
		global_range_w == that.global_range_w &&
		global_range_h == that.global_range_h &&
		draw_keypoints == that.draw_keypoints &&
		draw_border == that.draw_border &&
		replace_object == that.replace_object &&
		draw_object_border == that.draw_object_border &&
		global_block_w == that.global_block_w &&
		global_block_h == that.global_block_h &&
		block_x == that.block_x &&
		block_y == that.block_y &&
		object_layer == that.object_layer &&
		replace_layer == that.replace_layer &&
		scene_layer == that.scene_layer &&
		algorithm == that.algorithm &&
		vmin == that.vmin &&
		vmax == that.vmax &&
		smin == that.smin &&
		blend == that.blend;
	return result;
}

void FindObjectConfig::copy_from(FindObjectConfig &that)
{
	global_range_w = that.global_range_w;
	global_range_h = that.global_range_h;
	draw_keypoints = that.draw_keypoints;
	draw_border = that.draw_border;
	replace_object = that.replace_object;
	draw_object_border = that.draw_object_border;
	global_block_w = that.global_block_w;
	global_block_h = that.global_block_h;
	block_x = that.block_x;
	block_y = that.block_y;
	object_layer = that.object_layer;
	replace_layer = that.replace_layer;
	scene_layer = that.scene_layer;
	algorithm = that.algorithm;
	vmin = that.vmin;
	vmax = that.vmax;
	smin = that.smin;
	blend = that.blend;
}

void FindObjectConfig::interpolate(FindObjectConfig &prev, 
	FindObjectConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}








FindObjectMain::FindObjectMain(PluginServer *server)
 : PluginVClient(server)
{
	bzero(&blob_param, sizeof(CvBlobTrackerAutoParam1));
	blob_pTracker = 0;


	object_image = 0;
	prev_object = 0;
	scene_image = 0;
	object_image_w = 0;
	object_image_h = 0;
	scene_image_w = 0;
	scene_image_h = 0;
	storage = 0;
	object_keypoints = 0;
	object_descriptors = 0;
	scene_keypoints = 0;
	scene_descriptors = 0;
	affine = 0;
	temp = 0;
	overlayer = 0;
	init_border = 1;
}

FindObjectMain::~FindObjectMain()
{
// This releases all the arrays
	if(storage) cvReleaseMemStorage(&storage);
	if(object_image) cvReleaseImage(&object_image);
	if(scene_image) cvReleaseImage(&scene_image);
	if(prev_object) delete [] prev_object;
	delete affine;
	delete temp;
	delete overlayer;
	
    if(blob_param.pBT) cvReleaseBlobTracker(&blob_param.pBT);
    if(blob_param.pBD) cvReleaseBlobDetector(&blob_param.pBD);
    if(blob_param.pBTGen) cvReleaseBlobTrackGen(&blob_param.pBTGen);
    if(blob_param.pBTA) cvReleaseBlobTrackAnalysis(&blob_param.pBTA);
    if(blob_param.pFG) cvReleaseFGDetector(&blob_param.pFG);
    if(blob_pTracker) cvReleaseBlobTrackerAuto(&blob_pTracker);
	
}

const char* FindObjectMain::plugin_title() { return N_("Find Object"); }
int FindObjectMain::is_realtime() { return 1; }
int FindObjectMain::is_multichannel() { return 1; }


NEW_WINDOW_MACRO(FindObjectMain, FindObjectWindow)

LOAD_CONFIGURATION_MACRO(FindObjectMain, FindObjectConfig)



void FindObjectMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("FindObjectMain::update_gui");
			
			char string[BCTEXTLEN];

			((FindObjectWindow*)thread->window)->global_range_w->update(config.global_range_w);
			((FindObjectWindow*)thread->window)->global_range_h->update(config.global_range_h);
			((FindObjectWindow*)thread->window)->global_block_w->update(config.global_block_w);
			((FindObjectWindow*)thread->window)->global_block_h->update(config.global_block_h);
			((FindObjectWindow*)thread->window)->block_x->update(config.block_x);
			((FindObjectWindow*)thread->window)->block_y->update(config.block_y);
			((FindObjectWindow*)thread->window)->block_x_text->update((float)config.block_x);
			((FindObjectWindow*)thread->window)->block_y_text->update((float)config.block_y);

			((FindObjectWindow*)thread->window)->draw_keypoints->update(config.draw_keypoints);
			((FindObjectWindow*)thread->window)->draw_border->update(config.draw_border);
			((FindObjectWindow*)thread->window)->replace_object->update(config.replace_object);
			((FindObjectWindow*)thread->window)->draw_object_border->update(config.draw_object_border);


			((FindObjectWindow*)thread->window)->object_layer->update(
				(int64_t)config.object_layer);
			((FindObjectWindow*)thread->window)->replace_layer->update(
				(int64_t)config.replace_layer);
			((FindObjectWindow*)thread->window)->scene_layer->update(
				(int64_t)config.scene_layer);
			((FindObjectWindow*)thread->window)->algorithm->set_text(
				FindObjectAlgorithm::to_text(config.algorithm));

			((FindObjectWindow*)thread->window)->vmin->update(
				(int64_t)config.vmin);
			((FindObjectWindow*)thread->window)->vmax->update(
				(int64_t)config.vmax);
			((FindObjectWindow*)thread->window)->smin->update(
				(int64_t)config.smin);
			((FindObjectWindow*)thread->window)->blend->update(
				(int64_t)config.blend);

			((FindObjectWindow*)thread->window)->flush();
			thread->window->unlock_window();
		}
// printf("FindObjectMain::update_gui %d %d %d %d\n", 
// __LINE__, 
// config.mode1,
// config.mode2,
// config.mode3);
	}
}




void FindObjectMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("FINDOBJECT");

	output.tag.set_property("GLOBAL_BLOCK_W", config.global_block_w);
	output.tag.set_property("GLOBAL_BLOCK_H", config.global_block_h);
	output.tag.set_property("BLOCK_X", config.block_x);
	output.tag.set_property("BLOCK_Y", config.block_y);
	output.tag.set_property("GLOBAL_RANGE_W", config.global_range_w);
	output.tag.set_property("GLOBAL_RANGE_H", config.global_range_h);
	output.tag.set_property("DRAW_KEYPOINTS", config.draw_keypoints);
	output.tag.set_property("DRAW_BORDER", config.draw_border);
	output.tag.set_property("REPLACE_OBJECT", config.replace_object);
	output.tag.set_property("DRAW_OBJECT_BORDER", config.draw_object_border);
	output.tag.set_property("OBJECT_LAYER", config.object_layer);
	output.tag.set_property("REPLACE_LAYER", config.replace_layer);
	output.tag.set_property("SCENE_LAYER", config.scene_layer);
	output.tag.set_property("ALGORITHM", config.algorithm);
	output.tag.set_property("VMIN", config.vmin);
	output.tag.set_property("VMAX", config.vmax);
	output.tag.set_property("SMIN", config.smin);
	output.tag.set_property("BLEND", config.blend);
	output.append_tag();
	output.terminate_string();
}

void FindObjectMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("FINDOBJECT"))
			{
				config.global_block_w = input.tag.get_property("GLOBAL_BLOCK_W", config.global_block_w);
				config.global_block_h = input.tag.get_property("GLOBAL_BLOCK_H", config.global_block_h);
				config.block_x = input.tag.get_property("BLOCK_X", config.block_x);
				config.block_y = input.tag.get_property("BLOCK_Y", config.block_y);
				config.global_range_w = input.tag.get_property("GLOBAL_RANGE_W", config.global_range_w);
				config.global_range_h = input.tag.get_property("GLOBAL_RANGE_H", config.global_range_h);
				config.draw_keypoints = input.tag.get_property("DRAW_KEYPOINTS", config.draw_keypoints);
				config.draw_border = input.tag.get_property("DRAW_BORDER", config.draw_border);
				config.replace_object = input.tag.get_property("REPLACE_OBJECT", config.replace_object);
				config.draw_object_border = input.tag.get_property("DRAW_OBJECT_BORDER", config.draw_object_border);
				config.object_layer = input.tag.get_property("OBJECT_LAYER", config.object_layer);
				config.replace_layer = input.tag.get_property("REPLACE_LAYER", config.replace_layer);
				config.scene_layer = input.tag.get_property("SCENE_LAYER", config.scene_layer);
				config.algorithm = input.tag.get_property("ALGORITHM", config.algorithm);
				config.vmin = input.tag.get_property("VMIN", config.vmin);
				config.vmax = input.tag.get_property("VMAX", config.vmax);
				config.smin = input.tag.get_property("SMIN", config.smin);
				config.blend = input.tag.get_property("BLEND", config.blend);
			}
		}
	}
	config.boundaries();
}





void FindObjectMain::draw_pixel(VFrame *frame, int x, int y)
{
	if(!(x >= 0 && y >= 0 && x < frame->get_w() && y < frame->get_h())) return;

#define DRAW_PIXEL(x, y, components, do_yuv, max, type) \
{ \
	type **rows = (type**)frame->get_rows(); \
	rows[y][x * components] = max - rows[y][x * components]; \
	if(!do_yuv) \
	{ \
		rows[y][x * components + 1] = max - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = max - rows[y][x * components + 2]; \
	} \
	else \
	{ \
		rows[y][x * components + 1] = (max / 2 + 1) - rows[y][x * components + 1]; \
		rows[y][x * components + 2] = (max / 2 + 1) - rows[y][x * components + 2]; \
	} \
	if(components == 4) \
		rows[y][x * components + 3] = max; \
}


	switch(frame->get_color_model())
	{
		case BC_RGB888:
			DRAW_PIXEL(x, y, 3, 0, 0xff, unsigned char);
			break;
		case BC_RGBA8888:
			DRAW_PIXEL(x, y, 4, 0, 0xff, unsigned char);
			break;
		case BC_RGB_FLOAT:
			DRAW_PIXEL(x, y, 3, 0, 1.0, float);
			break;
		case BC_RGBA_FLOAT:
			DRAW_PIXEL(x, y, 4, 0, 1.0, float);
			break;
		case BC_YUV888:
			DRAW_PIXEL(x, y, 3, 1, 0xff, unsigned char);
			break;
		case BC_YUVA8888:
			DRAW_PIXEL(x, y, 4, 1, 0xff, unsigned char);
			break;
		case BC_RGB161616:
			DRAW_PIXEL(x, y, 3, 0, 0xffff, uint16_t);
			break;
		case BC_YUV161616:
			DRAW_PIXEL(x, y, 3, 1, 0xffff, uint16_t);
			break;
		case BC_RGBA16161616:
			DRAW_PIXEL(x, y, 4, 0, 0xffff, uint16_t);
			break;
		case BC_YUVA16161616:
			DRAW_PIXEL(x, y, 4, 1, 0xffff, uint16_t);
			break;
	}
}


void FindObjectMain::draw_line(VFrame *frame, int x1, int y1, int x2, int y2)
{
	int w = labs(x2 - x1);
	int h = labs(y2 - y1);
//printf("FindObjectMain::draw_line 1 %d %d %d %d\n", x1, y1, x2, y2);

	if(!w && !h)
	{
		draw_pixel(frame, x1, y1);
	}
	else
	if(w > h)
	{
// Flip coordinates so x1 < x2
		if(x2 < x1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = y2 - y1;
		int denominator = x2 - x1;
		for(int i = x1; i <= x2; i++)
		{
			int y = y1 + (int64_t)(i - x1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, i, y);
		}
	}
	else
	{
// Flip coordinates so y1 < y2
		if(y2 < y1)
		{
			y2 ^= y1;
			y1 ^= y2;
			y2 ^= y1;
			x1 ^= x2;
			x2 ^= x1;
			x1 ^= x2;
		}
		int numerator = x2 - x1;
		int denominator = y2 - y1;
		for(int i = y1; i <= y2; i++)
		{
			int x = x1 + (int64_t)(i - y1) * (int64_t)numerator / (int64_t)denominator;
			draw_pixel(frame, x, i);
		}
	}
//printf("FindObjectMain::draw_line 2\n");
}

void FindObjectMain::draw_rect(VFrame *frame, int x1, int y1, int x2, int y2)
{
	draw_line(frame, x1, y1, x2, y1);
	draw_line(frame, x2, y1 + 1, x2, y2);
	draw_line(frame, x2 - 1, y2, x1, y2);
	draw_line(frame, x1, y2 - 1, x1, y1 + 1);
}



// Convert to greyscale & crop
void FindObjectMain::grey_crop(unsigned char *dst, 
	VFrame *src, 
	int x1, 
	int y1,
	int x2,
	int y2,
	int dst_w,
	int dst_h)
{
// Dimensions of dst frame
	int w = x2 - x1;
	int h = y2 - y1;

	bzero(dst, dst_w * dst_h);

//printf("FindObjectMain::grey_crop %d %d %d\n", __LINE__, w, h);
	for(int i = 0; i < h; i++)
	{

#define RGB_TO_VALUE(r, g, b) \
((r) * R_TO_Y + (g) * G_TO_Y + (b) * B_TO_Y)


#define CONVERT(in_type, max, components, is_yuv) \
{ \
	in_type *input = ((in_type*)src->get_rows()[i + y1]) + x1 * components; \
	unsigned char *output = dst + i * dst_w; \
 \
	for(int j = 0; j < w; j++) \
	{ \
/* Y channel only */ \
		if(is_yuv) \
		{ \
			*output = *input; \
		} \
/* RGB */ \
		else \
		{ \
			float r = (float)input[0] / max; \
			float g = (float)input[1] / max; \
			float b = (float)input[2] / max; \
			*output = RGB_TO_VALUE(r, g, b); \
		} \
 \
		input += components; \
		output++; \
	} \
}
		switch(src->get_color_model())
		{
			case BC_RGB888:
			{
				CONVERT(unsigned char, 0xff, 3, 0)
				break;
			}

			case BC_RGBA8888:
			{
				CONVERT(unsigned char, 0xff, 4, 0)
				break;
			}

			case BC_RGB_FLOAT:
			{
				CONVERT(float, 1.0, 3, 0)
				break;
			}

			case BC_RGBA_FLOAT:
			{
				CONVERT(float, 1.0, 4, 0)
				break;
			}

			case BC_YUV888:
			{
				CONVERT(unsigned char, 0xff, 3, 1)
				break;
			}

			case BC_YUVA8888:
			{
				CONVERT(unsigned char, 0xff, 4, 1)
				break;
			}
		}
	}
}


void FindObjectMain::process_surf()
{

	if(!object_image)
	{
// Only does greyscale
		object_image = cvCreateImage( 
			cvSize(object_image_w, object_image_h), 
			8, 
			1);
	}

	if(!scene_image)
	{
// Only does greyscale
		scene_image = cvCreateImage( 
			cvSize(scene_image_w, scene_image_h), 
			8, 
			1);
	}

// Select only region with image size
// Does this do anything?
	cvSetImageROI( object_image, cvRect( 0, 0, object_w, object_h ) );
	cvSetImageROI( scene_image, cvRect( 0, 0, scene_w, scene_h ) );

	if(!prev_object) prev_object = new unsigned char[object_image_w * object_image_h];
	memcpy(prev_object, object_image->imageData, object_image_w * object_image_h);
	grey_crop((unsigned char*)scene_image->imageData, 
		get_input(scene_layer), 
		scene_x1, 
		scene_y1, 
		scene_x2, 
		scene_y2,
		scene_image_w,
		scene_image_h);


	grey_crop((unsigned char*)object_image->imageData, 
		get_input(object_layer), 
		object_x1, 
		object_y1, 
		object_x2, 
		object_y2,
		object_image_w,
		object_image_h);


	if(!storage) storage = cvCreateMemStorage(0);
	CvSURFParams params = cvSURFParams(500, 1);


//printf("FindObjectMain::process_surf %d\n", __LINE__);

// Only compute keypoints if the image changed
	if(memcmp(prev_object, object_image->imageData, object_image_w * object_image_h))
	{
		if(object_keypoints) cvClearSeq(object_keypoints);
		if(object_descriptors) cvClearSeq(object_descriptors);
		cvExtractSURF(object_image, 
			0, 
			&object_keypoints, 
			&object_descriptors, 
			storage, 
			params,
			0);
	}

//printf("FindObjectMain::process_surf %d object keypoints=%d\n", __LINE__, object_keypoints->total);
// Draw the keypoints
// 		for(int i = 0; i < object_keypoints->total; i++)
// 		{
//         	CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( object_keypoints, i );
// 			int size = r1->size / 4;
// 			draw_rect(frame[object_layer], 
//   				r1->pt.x + object_x1 - size, 
//   				r1->pt.y + object_y1 - size, 
//   				r1->pt.x + object_x1 + size, 
//  				r1->pt.y + object_y1 + size);
// 		}


//printf("FindObjectMain::process_surf %d\n", __LINE__);

// TODO: make the surf data persistent & check for image changes instead
	if(scene_keypoints) cvClearSeq(scene_keypoints);
	if(scene_descriptors) cvClearSeq(scene_descriptors);
	cvExtractSURF(scene_image, 
		0, 
		&scene_keypoints, 
		&scene_descriptors, 
		storage, 
		params,
		0);

// Draw the keypoints
// 		for(int i = 0; i < scene_keypoints->total; i++)
// 		{
//         	CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( scene_keypoints, i );
// 			int size = r1->size / 4;
// 			draw_rect(frame[scene_layer], 
//   				r1->pt.x + scene_x1 - size, 
//   				r1->pt.y + scene_y1 - size, 
//   				r1->pt.x + scene_x1 + size, 
//  				r1->pt.y + scene_y1 + size);
// 		}

// printf("FindObjectMain::process_surf %d %d %d scene keypoints=%d\n", 
// __LINE__, 
// scene_w,
// scene_h,
// scene_keypoints->total);

	int *point_pairs = 0;
	int total_pairs = 0;
	CvPoint src_corners[4] = 
	{
		{ 0, 0 }, 
		{ object_w, 0 }, 
		{ object_w, object_h }, 
		{ 0, object_h }
	};

	CvPoint dst_corners[4] = 
	{
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	};

//printf("FindObjectMain::process_surf %d\n", __LINE__);
	if(scene_keypoints->total &&
		object_keypoints->total &&
		locatePlanarObject(object_keypoints, 
		object_descriptors, 
		scene_keypoints, 
		scene_descriptors, 
		src_corners, 
		dst_corners,
		&point_pairs,
		&total_pairs))
	{





// Draw keypoints in the scene & object layer
		if(config.draw_keypoints)
		{
//printf("FindObjectMain::process_surf %d total pairs=%d\n", __LINE__, total_pairs);
			for(int i = 0; i < total_pairs; i++)
			{
        		CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( object_keypoints, point_pairs[i * 2] );
        		CvSURFPoint* r2 = (CvSURFPoint*)cvGetSeqElem( scene_keypoints, point_pairs[i * 2 + 1] );


				int size = r2->size * 1.2 / 9 * 2;
				draw_rect(get_input(scene_layer), 
  					r2->pt.x + scene_x1 - size, 
  					r2->pt.y + scene_y1 - size, 
  					r2->pt.x + scene_x1 + size, 
 					r2->pt.y + scene_y1 + size);
				draw_rect(get_input(object_layer), 
  					r1->pt.x + object_x1 - size, 
  					r1->pt.y + object_y1 - size, 
  					r1->pt.x + object_x1 + size, 
 					r1->pt.y + object_y1 + size);
			}
		}


//printf("FindObjectMain::process_surf %d\n", __LINE__);
// Get object outline in the scene layer
		border_x1 = dst_corners[0].x + scene_x1;
		border_y1 = dst_corners[0].y + scene_y1;
		border_x2 = dst_corners[1].x + scene_x1;
		border_y2 = dst_corners[1].y + scene_y1;
		border_x3 = dst_corners[2].x + scene_x1;
		border_y3 = dst_corners[2].y + scene_y1;
		border_x4 = dst_corners[3].x + scene_x1;
		border_y4 = dst_corners[3].y + scene_y1;
//printf("FindObjectMain::process_surf %d\n", __LINE__);


		
	}
//printf("FindObjectMain::process_surf %d\n", __LINE__);



// for(int i = 0; i < object_y2 - object_y1; i++)
// {
// 	unsigned char *dst = get_input(object_layer)->get_rows()[i];
// 	unsigned char *src = (unsigned char*)object_image->imageData + i * (object_x2 - object_x1);
// 	for(int j = 0; j < object_x2 - object_x1; j++)
// 	{
// 		*dst++ = *src;
// 		*dst++ = 0x80;
// 		*dst++ = 0x80;
// 		src++;
// 	}
// }


// Frees the image structures
	if(point_pairs) free(point_pairs);
}



void FindObjectMain::process_camshift()
{
// Some user defined parameters
	int vmin = config.vmin;
	int vmax = config.vmax;
	int smin = config.smin;
	float hranges[] = { 0, 180 };
	const float* phranges = hranges;


// Create aligned, RGB images
	if(!object_image)
	{
		object_image = cvCreateImage( 
			cvSize(object_image_w, object_image_h), 
			8, 
			3);
	}

	if(!scene_image)
	{
		scene_image = cvCreateImage( 
			cvSize(scene_image_w, scene_image_h), 
			8, 
			3);
	}

// Temporary row pointers
	unsigned char **object_rows = new unsigned char*[object_image_h];
	unsigned char **scene_rows = new unsigned char*[scene_image_h];
	for(int i = 0; i < object_image_h; i++)
	{
		object_rows[i] = (unsigned char*)(object_image->imageData + i * object_image_w * 3);
	}
	for(int i = 0; i < scene_image_h; i++)
	{
		scene_rows[i] = (unsigned char*)(scene_image->imageData + i * scene_image_w * 3);
	}

// Transfer object & scene to RGB images for OpenCV
	if(!prev_object) prev_object = new unsigned char[object_image_w * object_image_h * 3];
// Back up old object image
	memcpy(prev_object, object_image->imageData, object_image_w * object_image_h * 3);

	BC_CModels::transfer(object_rows,
		get_input(object_layer)->get_rows(),
		0,
		0,
		0,
		0,
		0,
		0,
		object_x1,
		object_y1,
		object_w,
		object_h,
		0,
		0,
		object_w,
		object_h,
		get_input(object_layer)->get_color_model(),
		BC_RGB888,
		0,
		0,
		0);
	BC_CModels::transfer(scene_rows,
		get_input(scene_layer)->get_rows(),
		0,
		0,
		0,
		0,
		0,
		0,
		scene_x1,
		scene_y1,
		scene_w,
		scene_h,
		0,
		0,
		scene_w,
		scene_h,
		get_input(scene_layer)->get_color_model(),
		BC_RGB888,
		0,
		0,
		0);

	delete [] object_rows;
	delete [] scene_rows;

// from camshiftdemo.cpp
// Compute new object	
	if(memcmp(prev_object, 
		object_image->imageData, 
		object_image_w * object_image_h * 3) ||
		!hist.dims)
	{
		Mat image(object_image);
		Mat hsv, hue, mask;
		cvtColor(image, hsv, CV_RGB2HSV);
    	int _vmin = vmin, _vmax = vmax;
//printf("FindObjectMain::process_camshift %d\n", __LINE__);

    	inRange(hsv, 
			Scalar(0, smin, MIN(_vmin,_vmax)),
        	Scalar(180, 256, MAX(_vmin, _vmax)), 
			mask);
    	int ch[] = { 0, 0 };
    	hue.create(hsv.size(), hsv.depth());
    	mixChannels(&hsv, 1, &hue, 1, ch, 1);

		Rect selection = Rect(0, 0, object_w, object_h);
		trackWindow = selection;
		int hsize = 16;
		Mat roi(hue, selection), maskroi(mask, selection);
		calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
		normalize(hist, hist, 0, 255, CV_MINMAX);
	}


// compute scene
	Mat image(scene_image);
	Mat hsv, hue, mask, backproj;
	cvtColor(image, hsv, CV_RGB2HSV);
    int _vmin = vmin, _vmax = vmax;

    inRange(hsv, 
		Scalar(0, smin, MIN(_vmin,_vmax)),
        Scalar(180, 256, MAX(_vmin, _vmax)), 
		mask);
    int ch[] = {0, 0};
    hue.create(hsv.size(), hsv.depth());
    mixChannels(&hsv, 1, &hue, 1, ch, 1);
	
//printf("FindObjectMain::process_camshift %d %d %d\n", __LINE__, hist.dims, hist.size[1]);
	RotatedRect trackBox = RotatedRect(
		Point2f((object_x1 + object_x2) / 2, (object_y1 + object_y2) / 2), 
		Size2f(object_w, object_h), 
		0);
	trackWindow = Rect(0, 
		0,
        scene_w, 
		scene_h);
	if(hist.dims > 0)
	{
		

		calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
		backproj &= mask;
//printf("FindObjectMain::process_camshift %d\n", __LINE__);
// 		if(trackWindow.width <= 0 ||
// 			trackWindow.height <= 0)
// 		{
// 			trackWindow.width = object_w;
// 			trackWindow.height = object_h;
// 		}

		trackBox = CamShift(backproj, 
			trackWindow,
        	TermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ));
//printf("FindObjectMain::process_camshift %d\n", __LINE__);


//     	if( trackWindow.area() <= 1 )
//     	{
//         	int cols = backproj.cols;
// 			int rows = backproj.rows;
// 			int r = (MIN(cols, rows) + 5) / 6;
//         	trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
//                         	   trackWindow.x + r, trackWindow.y + r) &
//                     	  Rect(0, 0, cols, rows);
//     	}
	}
// printf("FindObjectMain::process_camshift %d %d %d %d %d\n", 
// __LINE__,
// trackWindow.x,
// trackWindow.y,
// trackWindow.width,
// trackWindow.height);


// Draw mask over scene
	if(config.draw_keypoints)
	{
		for(int i = 0; i < scene_h; i++)
		{
			switch(get_input(scene_layer)->get_color_model())
			{
				case BC_YUV888:
				{
					unsigned char *input = backproj.data + i * scene_image_w;
					unsigned char *output = get_input(scene_layer)->get_rows()[i + scene_y1] + scene_x1 * 3;
					for(int j = 0; j < scene_w; j++)
					{
						output[0] = *input;
						output[1] = 0x80;
						output[2] = 0x80;
						output += 3;
						input++;
					}
					break;
				}
			}
		}
	}

// Get object outline in the scene layer
// printf("FindObjectMain::process_camshift %d %d %d %d %d %d\n", 
// __LINE__,
// (int)trackBox.center.x,
// (int)trackBox.center.y,
// (int)trackBox.size.width,
// (int)trackBox.size.height,
// (int)trackBox.angle);
	double angle = trackBox.angle * 2 * M_PI / 360;
	double angle1 = atan2(-(double)trackBox.size.height / 2, -(double)trackBox.size.width / 2) + angle;
	double angle2 = atan2(-(double)trackBox.size.height / 2, (double)trackBox.size.width / 2) + angle;
	double angle3 = atan2((double)trackBox.size.height / 2, (double)trackBox.size.width / 2) + angle;
	double angle4 = atan2((double)trackBox.size.height / 2, -(double)trackBox.size.width / 2) + angle;
	double radius = sqrt(SQR(trackBox.size.height / 2) + SQR(trackBox.size.width / 2));
	border_x1 = (int)(trackBox.center.x + cos(angle1) * radius) + scene_x1;
	border_y1 = (int)(trackBox.center.y + sin(angle1) * radius) + scene_y1;
	border_x2 = (int)(trackBox.center.x + cos(angle2) * radius) + scene_x1;
	border_y2 = (int)(trackBox.center.y + sin(angle2) * radius) + scene_y1;
	border_x3 = (int)(trackBox.center.x + cos(angle3) * radius) + scene_x1;
	border_y3 = (int)(trackBox.center.y + sin(angle3) * radius) + scene_y1;
	border_x4 = (int)(trackBox.center.x + cos(angle4) * radius) + scene_x1;
	border_y4 = (int)(trackBox.center.y + sin(angle4) * radius) + scene_y1;

}


#define APPLY_MASK(type, max, components, do_yuv) \
{ \
	type *output_row = (type*)get_input(scene_layer)->get_rows()[i]; \
	unsigned char *mask_row = mask_rows[i]; \
	int chroma_offset = (int)(max + 1) / 2; \
 \
	for(int j  = 0; j < scene_w; j++) \
	{ \
		if(components == 4) \
		{ \
			output_row[j * 4 + 3] = output_row[j * 4 + 3] * mask_row[j] / 255; \
		} \
		else \
		{ \
			output_row[j * 3] = output_row[j * 3] * mask_row[j] / 255; \
			output_row[j * 3 + 1] = output_row[j * 3 + 1] * mask_row[j] / 255; \
			output_row[j * 3 + 2] = output_row[j * 3 + 2] * mask_row[j] / 255; \
 \
			if(do_yuv) \
			{ \
				output_row[j * 3 + 1] += chroma_offset * (255 - mask_row[j]) / 255; \
				output_row[j * 3 + 2] += chroma_offset * (255 - mask_row[j]) / 255; \
			} \
		} \
	} \
}



// blobtrack_sample.cpp
void FindObjectMain::process_blob()
{
	if(!blob_initialized)
	{
		blob_initialized = 1;
		
		blob_param.FGTrainFrames = 5;


/* Create FG Detection module: */
		blob_param.pFG = cvCreateFGDetectorBase(CV_BG_MODEL_FGD, NULL);
/* Create Blob Entrance Detection module: */
        blob_param.pBD = cvCreateBlobDetectorCC();
/* Create blob tracker module: */
        blob_param.pBT = cvCreateBlobTrackerCCMSPF();
/* Create whole pipline: */
        blob_pTracker = cvCreateBlobTrackerAuto1(&blob_param);
		
	}


/* Process: */
	IplImage*   pMask = NULL;

// Create aligned, RGB images
	if(!scene_image)
	{
		scene_image = cvCreateImage( 
			cvSize(scene_image_w, scene_image_h), 
			8, 
			3);
	}

// Temporary row pointers
	unsigned char **scene_rows = new unsigned char*[scene_image_h];
	for(int i = 0; i < scene_image_h; i++)
	{
		scene_rows[i] = (unsigned char*)(scene_image->imageData + i * scene_image_w * 3);
	}

	BC_CModels::transfer(scene_rows,
		get_input(scene_layer)->get_rows(),
		0,
		0,
		0,
		0,
		0,
		0,
		scene_x1,
		scene_y1,
		scene_w,
		scene_h,
		0,
		0,
		scene_w,
		scene_h,
		get_input(scene_layer)->get_color_model(),
		BC_RGB888,
		0,
		0,
		0);

	delete [] scene_rows;

    blob_pTracker->Process(scene_image, pMask);
printf("FindObjectMain::process_blob %d %ld %d\n", __LINE__, get_source_position(), blob_pTracker->GetBlobNum());


#if 0
	if(blob_pTracker->GetFGMask())
	{
		IplImage* pFG = blob_pTracker->GetFGMask();
printf("FindObjectMain::process_blob %d %ld\n", __LINE__, get_source_position());
		
// Temporary row pointers
		unsigned char **mask_rows = new unsigned char*[scene_image_h];
		for(int i = 0; i < scene_image_h; i++)
		{
			mask_rows[i] = (unsigned char*)(pFG->imageData + i * scene_image_w);
		}

		for(int i = 0; i < scene_image_h; i++)
		{
			switch(get_input(scene_layer)->get_color_model())
			{
				case BC_RGB888:
					APPLY_MASK(unsigned char, 0xff, 3, 0)
					break;
				case BC_RGB_FLOAT:
					APPLY_MASK(float, 1.0, 3, 0)
					break;
				case BC_YUV888:
					APPLY_MASK(unsigned char, 0xff, 3, 1)
					break;
				case BC_RGBA8888:
					APPLY_MASK(unsigned char, 0xff, 4, 0)
					break;
				case BC_RGBA_FLOAT:
					APPLY_MASK(float, 1.0, 4, 0)
					break;
				case BC_YUVA8888:
					APPLY_MASK(unsigned char, 0xff, 4, 1)
					break;
			}
		}

		delete [] mask_rows;

		
	}
#endif
	

}







int FindObjectMain::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	int prev_algorithm = config.algorithm;
	if(load_configuration())
	{
		init_border = 1;
	}

	w = frame[0]->get_w();
	h = frame[0]->get_h();
//printf("FindObjectMain::process_buffer %d\n", __LINE__);


// Get the layer containing the object.
	object_layer = config.object_layer;
// Get the layer to search in.
	scene_layer = config.scene_layer;
// Get the layer with the replacement object
	replace_layer = config.replace_layer;

	object_layer = MIN(object_layer, PluginClient::get_total_buffers() - 1);
	scene_layer = MIN(scene_layer, PluginClient::get_total_buffers() - 1);
	replace_layer = MIN(replace_layer, PluginClient::get_total_buffers() - 1);

// printf("FindObjectMain::process_buffer %d %d %d %d %d %d\n", 
// __LINE__,
// PluginClient::get_total_buffers(),
// config.object_layer,
// config.scene_layer,
// object_layer,
// scene_layer);
// 
// Create cropped images
// TODO: use oblique corners & affine transform
	object_w = (int)(config.global_block_w * w / 100);
	object_h = (int)(config.global_block_h * h / 100);
	object_x1;
	object_y1;
	object_x2;
	object_y2;

	object_x1 = (int)(config.block_x * w / 100 - object_w / 2);
	object_y1 = (int)(config.block_y * h / 100 - object_h / 2);
	object_x2 = object_x1 + object_w;
	object_y2 = object_y1 + object_h;
	CLAMP(object_x1, 0, frame[0]->get_w() - 1);
	CLAMP(object_x2, 0, frame[0]->get_w() - 1);
	CLAMP(object_y1, 0, frame[0]->get_h() - 1);
	CLAMP(object_y2, 0, frame[0]->get_h() - 1);
	object_w = object_x2 - object_x1;
	object_h = object_y2 - object_y1;


	scene_w = (int)(config.global_range_w * w / 100);
	scene_h = (int)(config.global_range_h * h / 100);
	scene_x1;
	scene_y1;
	scene_x2;
	scene_y2;
	scene_x1 = (int)(config.block_x * w / 100 - scene_w / 2);
	scene_y1 = (int)(config.block_y * h / 100 - scene_h / 2);
	scene_x2 = scene_x1 + scene_w;
	scene_y2 = scene_y1 + scene_h;
	CLAMP(scene_x1, 0, frame[0]->get_w() - 1);
	CLAMP(scene_x2, 0, frame[0]->get_w() - 1);
	CLAMP(scene_y1, 0, frame[0]->get_h() - 1);
	CLAMP(scene_y2, 0, frame[0]->get_h() - 1);
	scene_w = scene_x2 - scene_x1;
	scene_h = scene_y2 - scene_y1;

// Get quantized sizes
	int object_image_w = object_w;
	int object_image_h = object_h;
	int scene_image_w = scene_w;
	int scene_image_h = scene_h;
	if(object_w % QUANTIZE) object_image_w += QUANTIZE - (object_w % QUANTIZE);
	if(object_h % QUANTIZE) object_image_h += QUANTIZE - (object_h % QUANTIZE);
	if(scene_w % QUANTIZE) scene_image_w += QUANTIZE - (scene_w % QUANTIZE);
	if(scene_h % QUANTIZE) scene_image_h += QUANTIZE - (scene_h % QUANTIZE);

	if(object_image && 
		(object_image_w != this->object_image_w ||
		object_image_h != this->object_image_h ||
		prev_algorithm != config.algorithm))
	{
		cvReleaseImage(&object_image);
		object_image = 0;
		delete [] prev_object;
		prev_object = 0;
	}
	this->object_image_w = object_image_w;
	this->object_image_h = object_image_h;

	if(scene_image && 
		(scene_image_w != this->scene_image_w ||
		scene_image_h != this->scene_image_h ||
		prev_algorithm != config.algorithm))
	{
		cvReleaseImage(&scene_image);
		scene_image = 0;
	}
	this->scene_image_w = scene_image_w;
	this->scene_image_h = scene_image_h;




//printf("FindObjectMain::process_buffer %d object_w=%d object_h=%d object_image_w=%d object_image_h=%d\n", __LINE__, object_w, object_h, object_image_w, object_image_h);
//printf("FindObjectMain::process_buffer %d scene_w=%d scene_h=%d scene_image_w=%d scene_image_h=%d\n", __LINE__, scene_w, scene_h, scene_image_w, scene_image_h);
//printf("FindObjectMain::process_buffer %d total_layers=%d\n", __LINE__, get_total_buffers());

// Read in the input frames
	for(int i = 0; i < PluginClient::get_total_buffers(); i++)
	{
		read_frame(frame[i], 
			i, 
			start_position, 
			frame_rate);
	}


// Search for object
	if(config.algorithm != NO_ALGORITHM &&
		(config.replace_object ||
		config.draw_border ||
		config.draw_keypoints))
	{


		switch(config.algorithm)
		{
			case ALGORITHM_SURF:
				process_surf();
				break;
			
			case ALGORITHM_CAMSHIFT:
				process_camshift();
				break;
				
			case ALGORITHM_BLOB:
				process_blob();
				break;
		}


		if(init_border)
		{
			border_x1_accum = border_x1;
			border_y1_accum = border_y1;
			border_x2_accum = border_x2;
			border_y2_accum = border_y2;
			border_x3_accum = border_x3;
			border_y3_accum = border_y3;
			border_x4_accum = border_x4;
			border_y4_accum = border_y4;
			init_border = 0;
		}
		else
		{
			border_x1_accum = (float)border_x1 * config.blend / 100 + 
				border_x1_accum * (100 - config.blend) / 100;
			border_y1_accum = (float)border_y1 * config.blend / 100 + 
				border_y1_accum * (100 - config.blend) / 100;
			border_x2_accum = (float)border_x2 * config.blend / 100 + 
				border_x2_accum * (100 - config.blend) / 100;
			border_y2_accum = (float)border_y2 * config.blend / 100 + 
				border_y2_accum * (100 - config.blend) / 100;
			border_x3_accum = (float)border_x3 * config.blend / 100 + 
				border_x3_accum * (100 - config.blend) / 100;
			border_y3_accum = (float)border_y3 * config.blend / 100 + 
				border_y3_accum * (100 - config.blend) / 100;
			border_x4_accum = (float)border_x4 * config.blend / 100 + 
				border_x4_accum * (100 - config.blend) / 100;
			border_y4_accum = (float)border_y4 * config.blend / 100 + 
				border_y4_accum * (100 - config.blend) / 100;
		}

// Replace object in the scene layer
		if(config.replace_object)
		{

// Some trickery to get the affine transform to alpha blend into the output
			if(!affine) affine = new AffineEngine(get_project_smp() + 1,
				get_project_smp() + 1);

//printf("FindObjectMain::process_surf %d replace_layer=%d\n", __LINE__, replace_layer);
			if(!temp)
				temp = new VFrame(w, 
					h, 
					get_input(scene_layer)->get_color_model());
			if(!overlayer)
				overlayer = new OverlayFrame(get_project_smp() + 1);

			temp->clear_frame();
			affine->process(temp,
				get_input(replace_layer),
				0, 
				AffineEngine::PERSPECTIVE,
				border_x1_accum * 100 / w,
				border_y1_accum * 100 / h,
				border_x2_accum * 100 / w,
				border_y2_accum * 100 / h,
				border_x3_accum * 100 / w,
				border_y3_accum * 100 / h,
				border_x4_accum * 100 / w,
				border_y4_accum * 100 / h,
				1);

			overlayer->overlay(get_input(scene_layer), 
				temp, 
				0, 
				0, 
				w, 
				h, 
				0, 
				0, 
				w, 
				h, 
				1,        // 0 - 1
				TRANSFER_NORMAL,
				NEAREST_NEIGHBOR);
		}


		if(config.draw_border)
		{
			draw_line(get_input(scene_layer), 
				border_x1_accum, 
				border_y1_accum, 
				border_x2_accum, 
				border_y2_accum);
			draw_line(get_input(scene_layer), 
				border_x2_accum, 
				border_y2_accum, 
				border_x3_accum, 
				border_y3_accum);
			draw_line(get_input(scene_layer), 
				border_x3_accum, 
				border_y3_accum, 
				border_x4_accum, 
				border_y4_accum);
			draw_line(get_input(scene_layer), 
				border_x4_accum, 
				border_y4_accum, 
				border_x1_accum, 
				border_y1_accum);
		}

	}

// Draw object outline in the object layer
	if(config.draw_object_border)
	{
		draw_line(frame[object_layer], object_x1, object_y1, object_x2, object_y1);
		draw_line(frame[object_layer], object_x2, object_y1, object_x2, object_y2);
		draw_line(frame[object_layer], object_x2, object_y2, object_x1, object_y2);
		draw_line(frame[object_layer], object_x1, object_y2, object_x1, object_y1);

		draw_line(frame[object_layer], scene_x1, scene_y1, scene_x2, scene_y1);
		draw_line(frame[object_layer], scene_x2, scene_y1, scene_x2, scene_y2);
		draw_line(frame[object_layer], scene_x2, scene_y2, scene_x1, scene_y2);
		draw_line(frame[object_layer], scene_x1, scene_y2, scene_x1, scene_y1);
	}




	return 0;
}


