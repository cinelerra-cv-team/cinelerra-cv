
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

#ifndef FLOATAUTO_H
#define FLOATAUTO_H

// Automation point that takes floating point values

class FloatAuto;

#include "auto.h"
#include "edl.inc"
#include "floatautos.inc"

class FloatAuto : public Auto
{
public:
	FloatAuto() {};
	FloatAuto(EDL *edl, FloatAutos *autos);
	~FloatAuto();

	int operator==(Auto &that);
	int operator==(FloatAuto &that);
	int identical(FloatAuto *src);
	void copy_from(Auto *that);
	void copy_from(FloatAuto *that);
	int interpolate_from(Auto *a1, Auto *a2, int64_t pos, Auto *templ=0); // bezier interpolation
	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	void load(FileXML *xml);


// "the value" (=payload of this keyframe)
	float get_value() {return this->value;}
	void  set_value(float newval);

// Possible policies to handle the tagents for the 
// bézier curves connecting adjacent automation points
	enum t_mode 
	{
		SMOOTH,     // tangents are coupled in order to yield a smooth curve
		LINEAR,     // tangents always pointing directly to neighbouring automation points
		TFREE,      // tangents on both sides coupled but editable by dragging the handles
		FREE        // tangents on both sides are independent and editable via GUI
	};

	t_mode tangent_mode;
	void change_tangent_mode(t_mode); // recalculates tangents as well
	void toggle_tangent_mode();       // cycles through all modes (e.g. by ctrl-click)
	

// Control values (y coords of bézier control point), relative to value
	float get_control_in_value()            {check_pos(); return this->control_in_value;}
	float get_control_out_value()           {check_pos(); return this->control_out_value;}
	void set_control_in_value(float newval);
	void set_control_out_value(float newval);
	
// get calculated x-position of control points for drawing, 
// relative to auto position, in native units of the track.
	int64_t get_control_in_position()       {check_pos(); return this->control_in_position;}
	int64_t get_control_out_position()      {check_pos(); return this->control_out_position;}
	
// define new position and value, re-adjust ctrl point, notify neighbours
	void adjust_to_new_coordinates(int64_t position, float value);



private:
	void adjust_tangents();             // recalc. ctrk in and out points, if automatic tangent mode (SMOOTH or LINEAR)
	void adjust_ctrl_positions(FloatAuto *p=0, FloatAuto *n=0); // recalc. x location of ctrl points, notify neighbours
	void set_ctrl_positions(FloatAuto*, FloatAuto*);
	void calculate_slope(FloatAuto* a1, FloatAuto* a2, float& dvdx, float& dx);
	void check_pos()                    { if(position != pos_valid) adjust_ctrl_positions(); }
	void tangent_dirty()                { pos_valid=-1; }
	static bool is_floatauto_node(Auto *candidate); // check is member of FloatAutos-Collection
	void handle_automatic_tangent_after_copy();	

// Control values are relative to value
	float value, control_in_value, control_out_value;
// X control positions relative to value position for drawing.
// In native units of the track.
	int64_t control_in_position, control_out_position;

	int64_t pos_valid;                  // 'dirty flag' to recalculate ctrl point positions on demand
	int value_to_str(char *string, float value);
};



#endif
