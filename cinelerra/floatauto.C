
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

#include "autos.h"
#include "clip.h"
#include "edl.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "localsession.h"
#include "transportque.inc"
#include "automation.inc"

FloatAuto::FloatAuto(EDL *edl, FloatAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
	control_in_value = 0;
	control_out_value = 0;
	control_in_position = 0;
	control_out_position = 0;
	pos_valid    = -1; //"dirty"
	tangent_mode = SMOOTH;
//  note: in most cases the tangent_mode-value is set   
//        by the method interpolate_from() rsp. copy_from()
}

FloatAuto::~FloatAuto()
{
	// as we are going away, the neighbouring float auto nodes 
	// need to re-adjust their ctrl point positions and tangents
	if(is_floatauto_node(this))
	{
		if (next)
			((FloatAuto*)next)->tangent_dirty();
		if (previous)
			((FloatAuto*)previous)->tangent_dirty();
	}
}

int FloatAuto::operator==(Auto &that)
{
	return identical((FloatAuto*)&that);
}


int FloatAuto::operator==(FloatAuto &that)
{
	return identical((FloatAuto*)&that);
}


inline 
bool FloatAuto::is_floatauto_node(Auto *candidate)
{
	return (candidate && candidate->autos &&
		AUTOMATION_TYPE_FLOAT == candidate->autos->get_type());
}


int FloatAuto::identical(FloatAuto *src)
{
	return EQUIV(value, src->value) &&
		EQUIV(control_in_value, src->control_in_value) &&
		EQUIV(control_out_value, src->control_out_value);
		// ctrl positions ignored, as they may depend on neighbours
		// tangent_mode is ignored, no recalculations
}

/* Note: the following is essentially display-code and has been moved to: 
 *  TrackCanvas::value_to_percentage(float auto_value, int autogrouptype)
 * 
float FloatAuto::value_to_percentage()
{
}
float FloatAuto::value_to_percentage()
{
}
float FloatAuto::value_to_percentage()
{
}
*/


void FloatAuto::copy_from(Auto *that)
{
	copy_from((FloatAuto*)that);
}

void FloatAuto::copy_from(FloatAuto *that)
{
	Auto::copy_from(that);
	this->value = that->value;
	this->control_in_value = that->control_in_value;
	this->control_out_value = that->control_out_value;
	this->control_in_position = that->control_in_position;
	this->control_out_position = that->control_out_position;
	this->tangent_mode = that->tangent_mode;
// note: literate copy, no recalculations    
}

inline
void FloatAuto::handle_automatic_tangent_after_copy()
// in most cases, we don't want to use the manual tangent modes
// of the left neighbour used as a template for interpolation.
// Rather, we (re)set to automatically smoothed tangents. Note
// auto generated nodes (while tweaking values) indeed are
// inserted by using this "interpolation" approach, thus making 
// this defaulting to auto-smooth tangents very important.
{
	if(tangent_mode == FREE || tangent_mode == TFREE)
	{
		this->tangent_mode = SMOOTH;
	}
}


int FloatAuto::interpolate_from(Auto *a1, Auto *a2, int64_t pos, Auto *templ)
// bézier interpolates this->value and tangents for the given position
// between the positions of a1 and a2. If a1 or a2 are omitted, they default
// to this->previous and this->next. If this FloatAuto has automatic tangents,
// this may trigger re-adjusting of this and its neighbours in this->autos.
// Note while a1 and a2 need not be members of this->autos, automatic 
// readjustments are always done to the neighbours in this->autos.
// If the template is given, it will be used to fill out this
// objects fields prior to interpolating.
{
	if(!a1) a1 = previous;
	if(!a2) a2 = next;
	Auto::interpolate_from(a1, a2, pos, templ);
	handle_automatic_tangent_after_copy();
	
	// set this->value using bézier interpolation if possible
	if(is_floatauto_node(a1) && is_floatauto_node(a2) &&
	   a1->position <= pos && pos <= a2->position)
	{
		FloatAuto *left = (FloatAuto*)a1;
		FloatAuto *right = (FloatAuto*)a2;
		float new_value = FloatAutos::calculate_bezier(left, right, pos);
		float new_slope = FloatAutos::calculate_bezier_derivation(left, right, pos);
		
		this->adjust_to_new_coordinates(pos, new_value); // this may trigger smoothing
		
		this->set_control_in_value(new_slope * control_in_position);
		this->set_control_out_value(new_slope * control_out_position);
		return 1; //return true: interpolated indeed...
	}
	else
	{
		adjust_ctrl_positions(); // implies adjust_tangents()
		return 0; // unable to interpolate
	}
}


void FloatAuto::change_tangent_mode(t_mode new_mode)
{
	if(new_mode == TFREE && !(control_in_position && control_out_position))
		new_mode = FREE; // only if tangents on both sides...

	tangent_mode = new_mode;
	adjust_tangents();
}

void FloatAuto::toggle_tangent_mode()
{
	switch (tangent_mode) {
		case SMOOTH:	change_tangent_mode(TFREE);  break;
		case LINEAR:	change_tangent_mode(FREE);   break;
		case TFREE :	change_tangent_mode(LINEAR); break;
		case FREE  :	change_tangent_mode(SMOOTH); break;
	}
}


void FloatAuto::set_value(float newvalue)
{
	this->value=newvalue; 
	this->adjust_tangents();
	if(previous) ((FloatAuto*)previous)->adjust_tangents();
	if(next)     ((FloatAuto*)next)->adjust_tangents();
} 

void FloatAuto::set_control_in_value(float newvalue)
{
	switch(tangent_mode) {
		case TFREE:	control_out_value = control_out_position*newvalue / control_in_position;
		case FREE:	control_in_value = newvalue;
		default:	return; // otherwise calculated automatically...
	}
}

void FloatAuto::set_control_out_value(float newvalue)
{
	switch(tangent_mode) {
		case TFREE:	control_in_value = control_in_position*newvalue / control_out_position;
		case FREE:	control_out_value=newvalue;
		default:	return;
	}
}



inline int sgn(float value) { return (value == 0)?  0 : (value < 0) ? -1 : 1; }

inline float weighted_mean(float v1, float v2, float w1, float w2){
	if(0.000001 > fabs(w1 + w2))
		return 0;
	else
		return (w1 * v1 + w2 * v2) / (w1 + w2);
}




void FloatAuto::adjust_tangents()
// recalculates tangents if current mode 
// implies automatic adjustment of tangents
{
	if(!autos) return;

	if(tangent_mode == SMOOTH)
	{
		// normally, one would use the slope of chord between the neighbours.
		// but this could cause the curve to overshot extremal automation nodes.
		// (e.g when setting a fade node at zero, the curve could go negative)
		// we can interpret the slope of chord as a weighted mean value, where
		// the length of the interval is used as weight; we just use other 
		// weights: intervall length /and/ reciprocal of slope. So, if the
		// connection to one of the neighbours has very low slope this will
		// dominate the calculated tangent slope at this automation node.
		// if the slope goes beyond the zero line, e.g if left connection
		// has positive and right connection has negative slope, then
		// we force the calculated tangent to be horizontal.
		float s, dxl, dxr, sl, sr;
		calculate_slope((FloatAuto*) previous, this, sl, dxl);
		calculate_slope(this, (FloatAuto*) next, sr, dxr);
		
		if(0 < sgn(sl) * sgn(sr))
		{
			float wl = fabs(dxl) * (fabs(1.0/sl) + 1);
			float wr = fabs(dxr) * (fabs(1.0/sr) + 1);
			s = weighted_mean(sl, sr, wl, wr);
		}
		else s = 0; // fixed hoizontal tangent
		
		control_in_value = s * control_in_position;
		control_out_value = s * control_out_position;
	}
	
	else
	if(tangent_mode == LINEAR)
 	{
		float g, dx;
		if(previous)
		{
			calculate_slope(this, (FloatAuto*)previous, g, dx);
			control_in_value = g * dx / 3;
		}
		if(next)
		{
			calculate_slope(this, (FloatAuto*)next, g, dx);
			control_out_value = g * dx / 3;
	}	}
	
	else
	if(tangent_mode == TFREE && control_in_position && control_out_position)
 	{
		float gl = control_in_value / control_in_position;
		float gr = control_out_value / control_out_position;
		float wl = fabs(control_in_value);
		float wr = fabs(control_out_value);
		float g = weighted_mean(gl, gr, wl, wr);
		
		control_in_value = g * control_in_position;
		control_out_value = g * control_out_position;
	}
}

inline void FloatAuto::calculate_slope(FloatAuto *left, FloatAuto *right, float &dvdx, float &dx)
{
	dvdx=0; dx=0;
	if(!left || !right) return;

	dx = right->position - left->position;
	float dv = right->value - left->value;
	dvdx = (dx == 0) ? 0 : dv/dx;
}


void FloatAuto::adjust_ctrl_positions(FloatAuto *prev, FloatAuto *next)
// recalculates location of ctrl points to be
// always 1/3 and 2/3 of the distance to the
// next neighbours. The reason is: for this special
// distance the bézier function yields x(t) = t, i.e.
// we can use the y(t) as if it was a simple function y(x).
   
// This adjustment is done only on demand and involves 
// updating neighbours and adjust_tangents() as well.
{ 
	if(!prev && !next)
	{	// use current siblings
		prev = (FloatAuto*)this->previous;
		next = (FloatAuto*)this->next;
	}	
	
	if(prev)
	{	set_ctrl_positions(prev, this);
		prev->adjust_tangents();
	}
	else // disable tangent on left side
		control_in_position = 0;
	
	if(next) 
	{	set_ctrl_positions(this, next);
		next->adjust_tangents();
	}
	else // disable right tangent
		control_out_position = 0;
	
	this->adjust_tangents();
	pos_valid = position;
// tangents up-to-date	
}



inline void redefine_tangent(int64_t &old_pos, int64_t new_pos, float &ctrl_val)
{
	if(old_pos != 0)
		ctrl_val *= (float)new_pos / old_pos;
	old_pos = new_pos;
}


inline void FloatAuto::set_ctrl_positions(FloatAuto *prev, FloatAuto* next)
{
	int64_t distance = next->position - prev->position;
	redefine_tangent(prev->control_out_position, +distance / 3,  prev->control_out_value);
	redefine_tangent(next->control_in_position,  -distance / 3,  next->control_in_value);
}



void FloatAuto::adjust_to_new_coordinates(int64_t position, float value)
// define new position and value in one step, do necessary re-adjustments
{
	this->value = value;
	this->position = position;
	adjust_ctrl_positions();
}



int FloatAuto::value_to_str(char *string, float value)
{
	int j = 0, i = 0;
	if(value > 0) 
		sprintf(string, "+%.2f", value);
	else
		sprintf(string, "%.2f", value);

// fix number
	if(value == 0)
	{
		j = 0;
		string[1] = 0;
	}
	else
	if(value < 1 && value > -1) 
	{
		j = 1;
		string[j] = string[0];
	}
	else 
	{
		j = 0;
		string[3] = 0;
	}
	
	while(string[j] != 0) string[i++] = string[j++];
	string[i] = 0;

	return 0;
}

void FloatAuto::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title("AUTO");
	if(default_auto)
		file->tag.set_property("POSITION", 0);
	else
		file->tag.set_property("POSITION", position - start);
	file->tag.set_property("VALUE", value);
	file->tag.set_property("CONTROL_IN_VALUE", control_in_value / 2.0); // compatibility, see below
	file->tag.set_property("CONTROL_OUT_VALUE", control_out_value / 2.0);
	file->tag.set_property("TANGENT_MODE", (int)tangent_mode);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void FloatAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
	control_in_value = file->tag.get_property("CONTROL_IN_VALUE", control_in_value);
	control_out_value = file->tag.get_property("CONTROL_OUT_VALUE", control_out_value);
	tangent_mode = (t_mode)file->tag.get_property("TANGENT_MODE", FREE);
	
	// Compatibility to old session data format:
	// Versions previous to the bezier auto patch (Jun 2006) applied a factor 2
	// to the y-coordinates of ctrl points while calculating the bezier function.
	// To retain compatibility, we now apply this factor while loading
	control_in_value *= 2.0;
	control_out_value *= 2.0;

// restore ctrl positions and adjust tangents if necessary
	adjust_ctrl_positions();
}
