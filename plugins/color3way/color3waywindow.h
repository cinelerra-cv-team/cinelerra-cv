
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef COLOR3WAYWINDOW_H
#define COLOR3WAYWINDOW_H


class Color3WayThread;
class Color3WayWindow;
class Color3WaySlider;
class Color3WayPreserve;
class Color3WayLock;
class Color3WayWhite;
class Color3WayReset;
class Color3WayWindow;



#include "filexml.h"
#include "guicast.h"
#include "mutex.h"
#include "color3way.h"
#include "pluginclient.h"


class Color3WayPoint : public BC_SubWindow
{
public:
	Color3WayPoint(Color3WayMain *plugin, 
		Color3WayWindow *gui,
		float *x_output, 
		float *y_output,
		int x, 
		int y,
		int radius,
		int section);

	virtual ~Color3WayPoint();

	int handle_event();
	void update();
	int initialize();
	int cursor_enter_event();
	int cursor_leave_event();
	int cursor_motion_event();
	int button_release_event();
	int button_press_event();
	void draw_face(int flash, int flush);
	int reposition_window(int x, int y, int radius);
	int deactivate();
	int activate();
	int keypress_event();

	enum
	{
		COLOR_UP,
		COLOR_HI,
		COLOR_DN,
		COLOR_IMAGES
	};

	int active;
	int status;
	int fg_x;
	int fg_y;
	int starting_x;
	int starting_y;
	int offset_x;
	int offset_y;	
	int drag_operation;
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;

	float *x_output;
	float *y_output;
	int radius;
	BC_Pixmap *fg_images[COLOR_IMAGES];
	BC_Pixmap *bg_image;
};


class Color3WaySlider : public BC_FSlider
{
public:
	Color3WaySlider(Color3WayMain *plugin, 
		Color3WayWindow *gui, 
		float *output, 
		int x, 
		int y,
		int w,
		int section);
	~Color3WaySlider();
	int handle_event();
	char* get_caption();

	Color3WayMain *plugin;
	Color3WayWindow *gui;
	float *output;
    float old_value;
	int section;
	char string[BCTEXTLEN];
};


class Color3WayResetSection : public BC_GenericButton
{
public:
	Color3WayResetSection(Color3WayMain *plugin, 
		Color3WayWindow *gui, 
		int x, 
		int y,
		int section);
	int handle_event();
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
};

class Color3WayBalanceSection : public BC_GenericButton
{
public:
	Color3WayBalanceSection(Color3WayMain *plugin, 
		Color3WayWindow *gui, 
		int x, 
		int y,
		int section);
	int handle_event();
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
};

class Color3WayCopySection : public BC_CheckBox
{
public:
	Color3WayCopySection(Color3WayMain *plugin, 
		Color3WayWindow *gui, 
		int x, 
		int y,
		int section);
	int handle_event();
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int section;
};

class Color3WaySection
{
public:
	Color3WaySection(Color3WayMain *plugin, 
		Color3WayWindow *gui,
		int x,
		int y,
		int w,
		int h,
		int section);
	void create_objects();
	int reposition_window(int x, int y, int w, int h);
	void update();

	BC_Title *title;
	Color3WayMain *plugin;
	Color3WayWindow *gui;
	int x, y, w, h;
	int section;
	Color3WayPoint *point;
	BC_Title *value_title;
	Color3WaySlider *value;
	BC_Title *sat_title;
	Color3WaySlider *saturation;
	Color3WayResetSection *reset;
	Color3WayBalanceSection *balance;
	Color3WayCopySection *copy;
};


class Color3WayWindow : public PluginClientWindow
{
public:
	Color3WayWindow(Color3WayMain *plugin);
	~Color3WayWindow();

	void create_objects();
	void update();
	int resize_event(int w, int h);

	Color3WayMain *plugin;
	Color3WayPoint *active_point;
	
	Color3WaySection *sections[SECTIONS];
};


#endif
