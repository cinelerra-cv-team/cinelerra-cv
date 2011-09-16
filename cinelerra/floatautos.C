
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "automation.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "track.h"
#include "localsession.h"
#include "transportque.inc"

FloatAutos::FloatAutos(EDL *edl,
				Track *track, 
				float default_)
 : Autos(edl, track)
{
	this->default_ = default_;
	type = AUTOMATION_TYPE_FLOAT;
}

FloatAutos::~FloatAutos()
{
}

void FloatAutos::straighten(int64_t start, int64_t end)
{
	FloatAuto *current = (FloatAuto*)first;
	while(current)
	{
		FloatAuto *previous_auto = (FloatAuto*)PREVIOUS;
		FloatAuto *next_auto = (FloatAuto*)NEXT;

// Is current auto in range?		
		if(current->position >= start && current->position < end)
		{
			float current_value = current->get_value();

// Determine whether to set the control in point.
			if(previous_auto && previous_auto->position >= start)
			{
				float previous_value = previous_auto->get_value();
				current->set_control_in_value((previous_value - current_value) / 3.0);
			}

// Determine whether to set the control out point
			if(next_auto && next_auto->position < end)
			{
				float next_value = next_auto->get_value();
				current->set_control_out_value( (next_value - current_value) / 3.0);
			}
		}
		current = (FloatAuto*)NEXT;
	}
}

int FloatAutos::draw_joining_line(BC_SubWindow *canvas, int vertical, int center_pixel, int x1, int y1, int x2, int y2)
{
	if(vertical)
		canvas->draw_line(center_pixel - y1, x1, center_pixel - y2, x2);
	else
		canvas->draw_line(x1, center_pixel + y1, x2, center_pixel + y2);
}

Auto* FloatAutos::add_auto(int64_t position, float value)
{
	FloatAuto* current = (FloatAuto*)autoof(position);
	FloatAuto* result;
	
	insert_before(current, result = (FloatAuto*)new_auto());
	result->adjust_to_new_coordinates(position,value);
	
	return result;
}

Auto* FloatAutos::new_auto()
{
	FloatAuto *result = new FloatAuto(edl, this);
	result->set_value(default_);
	return result;
}

int FloatAutos::get_testy(float slope, int cursor_x, int ax, int ay)
{
	return (int)(slope * (cursor_x - ax)) + ay;
}

int FloatAutos::automation_is_constant(int64_t start, 
	int64_t length, 
	int direction,
	double &constant)
{
	int total_autos = total();
	int64_t end;
	if(direction == PLAY_FORWARD)
	{
		end = start + length;
	}
	else
	{
		end = start + 1;
		start -= length;
	}


// No keyframes on track
	if(total_autos == 0)
	{
		constant = ((FloatAuto*)default_auto)->get_value();
		return 1;
	}
	else
// Only one keyframe on track.
	if(total_autos == 1)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}
	else
// Last keyframe is before region
	if(last->position <= start)
	{
		constant = ((FloatAuto*)last)->get_value();
		return 1;
	}
	else
// First keyframe is after region
	if(first->position > end)
	{
		constant = ((FloatAuto*)first)->get_value();
		return 1;
	}

// Scan sequentially
	int64_t prev_position = -1;
	for(Auto *current = first; current; current = NEXT)
	{
		int test_current_next = 0;
		int test_previous_current = 0;
		FloatAuto *float_current = (FloatAuto*)current;

// keyframes before and after region but not in region
		if(prev_position >= 0 &&
			prev_position < start && 
			current->position >= end)
		{
// Get value now in case change doesn't occur
			constant = float_current->get_value();
			test_previous_current = 1;
		}
		prev_position = current->position;

// Keyframe occurs in the region
		if(!test_previous_current &&
			current->position < end && 
			current->position >= start)
		{

// Get value now in case change doesn't occur
			constant = float_current->get_value();

// Keyframe has neighbor
			if(current->previous)
			{
				test_previous_current = 1;
			}

			if(current->next)
			{
				test_current_next = 1;
			}
		}

		if(test_current_next)
		{
//printf("FloatAutos::automation_is_constant 1 %d\n", start);
			FloatAuto *float_next = (FloatAuto*)current->next;

// Change occurs between keyframes
			if(!EQUIV(float_current->get_value(), float_next->get_value()) ||
				!EQUIV(float_current->get_control_out_value(), 0) ||
				!EQUIV(float_next->get_control_in_value(), 0))
			{
				return 0;
			}
		}

		if(test_previous_current)
		{
			FloatAuto *float_previous = (FloatAuto*)current->previous;

// Change occurs between keyframes
			if(!EQUIV(float_current->get_value(), float_previous->get_value()) ||
				!EQUIV(float_current->get_control_in_value(), 0) ||
				!EQUIV(float_previous->get_control_out_value(), 0))
			{
// printf("FloatAutos::automation_is_constant %d %d %d %f %f %f %f\n", 
// start, 
// float_previous->position, 
// float_current->position, 
// float_previous->value, 
// float_current->value, 
// float_previous->control_out_value, 
// float_current->control_in_value);
				return 0;
			}
		}
	}

// Got nothing that changes in the region.
	return 1;
}

double FloatAutos::get_automation_constant(int64_t start, int64_t end)
{
	Auto *current_auto, *before = 0, *after = 0;
	
// quickly get autos just outside range	
	get_neighbors(start, end, &before, &after);

// no auto before range so use first
	if(before)
		current_auto = before;
	else
		current_auto = first;

// no autos at all so use default value
	if(!current_auto) current_auto = default_auto;

	return ((FloatAuto*)current_auto)->get_value();
}


float FloatAutos::get_value(int64_t position, 
	FloatAuto* &previous, 
	FloatAuto* &next)
{
// Calculate bezier equation at position
// prev and next will be used to shorten the search, if given

	previous = (FloatAuto*)get_prev_auto(position, PLAY_FORWARD, (Auto* &)previous, 0);
	next     = (FloatAuto*)get_next_auto(position, PLAY_FORWARD, (Auto* &)next, 0);

// Constant
	if(!next && !previous)
	{
		return ((FloatAuto*)default_auto)->get_value();
	}
	else
	if(!previous)
	{
		return next->get_value();
	}
	else
	if(!next)
	{
		return previous->get_value();
	}
	else
	if(next == previous || next->position == previous->position)
	{
		return previous->get_value();
	}
	else
	{
		if(EQUIV(previous->get_value(), next->get_value()) &&
		   EQUIV(previous->get_control_out_value(), 0) &&
		   EQUIV(next->get_control_in_value(), 0))
		{
			return previous->get_value();
		}
	}
	
// at this point: previous and next not NULL, positions differ, value not constant.

	return calculate_bezier(previous, next, position);
}


float FloatAutos::calculate_bezier(FloatAuto *previous, FloatAuto *next, int64_t position)
{
	if(next->position - previous->position == 0) return previous->get_value();

	float y0 = previous->get_value();
	float y3 = next->get_value();

// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
	float t = (float)(position - previous->position) / 
			(next->position - previous->position);

 	float tpow2 = t * t;
	float tpow3 = t * t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	float invtpow3 = invt * invt * invt;
	
	float result = (  invtpow3 * y0
		+ 3 * t     * invtpow2 * y1
		+ 3 * tpow2 * invt     * y2 
		+     tpow3            * y3);
//printf("FloatAutos::get_value(t=%5.3f)->%6.2f   (prev,pos,next)=(%d,%d,%d)\n", t, result, previous->position, position, next->position);

	return result;
}


float FloatAutos::calculate_bezier_derivation(FloatAuto *previous, FloatAuto *next, int64_t position)
// calculate the slope of the interpolating bezier function at given position.
// computed slope is based on the actual position scale (in frames or samples)
{
	float scale = next->position - previous->position;
	if(scale == 0)
		if(previous->get_control_out_position() != 0)
			return previous->get_control_out_value() / previous->get_control_out_position();
		else
			return 0;
	
	float y0 = previous->get_value();
	float y3 = next->get_value();
	
// control points
	float y1 = previous->get_value() + previous->get_control_out_value();
	float y2 = next->get_value() + next->get_control_in_value();
// normalized scale	
	float t = (float)(position - previous->position) / scale; 
	
 	float tpow2 = t * t;
	float invt = 1 - t;
	float invtpow2 = invt * invt;
	
	float slope = 3 * (
		- invtpow2              * y0
		- invt * ( 2*t - invt ) * y1
		+ t    * ( 2*invt - t ) * y2 
		+ tpow2                 * y3
		);
	
	return slope / scale;
}



void FloatAutos::get_extents(float *min, 
	float *max,
	int *coords_undefined,
	int64_t unit_start,
	int64_t unit_end)
{
	if(!edl)
	{
		printf("FloatAutos::get_extents edl == NULL\n");
		return;
	}

	if(!track)
	{
		printf("FloatAutos::get_extents track == NULL\n");
		return;
	}

// Use default auto
	if(!first)
	{
		FloatAuto *current = (FloatAuto*)default_auto;
		if(*coords_undefined)
		{
			*min = *max = current->get_value();
			*coords_undefined = 0;
		}

		*min = MIN(current->get_value(), *min);
		*max = MAX(current->get_value(), *max);
	}

// Test all handles
	for(FloatAuto *current = (FloatAuto*)first; current; current = (FloatAuto*)NEXT)
	{
		if(current->position >= unit_start && current->position < unit_end)
		{
			if(*coords_undefined)
			{
				*min = *max = current->get_value();
				*coords_undefined = 0;
			}
			
			*min = MIN(current->get_value(), *min);
			*min = MIN(current->get_value() + current->get_control_in_value(), *min);
			*min = MIN(current->get_value() + current->get_control_out_value(), *min);

			*max = MAX(current->get_value(), *max);
			*max = MAX(current->get_value() + current->get_control_in_value(), *max);
			*max = MAX(current->get_value() + current->get_control_out_value(), *max);
		}
	}

// Test joining regions
	FloatAuto *prev = 0;
	FloatAuto *next = 0;
	int64_t unit_step = edl->local_session->zoom_sample;
	if(track->data_type == TRACK_VIDEO)
		unit_step = (int64_t)(unit_step * 
			edl->session->frame_rate / 
			edl->session->sample_rate);
	unit_step = MAX(unit_step, 1);
	for(int64_t position = unit_start; 
		position < unit_end; 
		position += unit_step)
	{
		float value = get_value(position,prev,next);
		if(*coords_undefined)
		{
			*min = *max = value;
			*coords_undefined = 0;
		}
		else
		{
			*min = MIN(value, *min);
			*max = MAX(value, *max);
		}	
	}
}

void FloatAutos::dump()
{
	printf("	FloatAutos::dump %p\n", this);
	printf("	Default: position %lld value=%f\n", 
		default_auto->position, 
		((FloatAuto*)default_auto)->get_value());
	for(Auto* current = first; current; current = NEXT)
	{
		printf("	position %lld value=%7.3f invalue=%7.3f outvalue=%7.3f %s\n", 
			current->position, 
			((FloatAuto*)current)->get_value(),
			((FloatAuto*)current)->get_control_in_value(),
			((FloatAuto*)current)->get_control_out_value(),
			((FloatAuto*)current)->tangent_mode == FloatAuto::SMOOTH ? "smooth" :
			((FloatAuto*)current)->tangent_mode == FloatAuto::LINEAR ? "linear" : ""
			);
	}
}
