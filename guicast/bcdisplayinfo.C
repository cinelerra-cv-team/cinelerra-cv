
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

#include "bcdisplayinfo.h"
#include "clip.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>

#define TEST_X 100
#define TEST_Y 100
#define TEST_SIZE 128

int BC_DisplayInfo::top_border = -1;
int BC_DisplayInfo::left_border = -1;
int BC_DisplayInfo::bottom_border = -1;
int BC_DisplayInfo::right_border = -1;
int BC_DisplayInfo::auto_reposition_x = -1;
int BC_DisplayInfo::auto_reposition_y = -1;


BC_DisplayInfo::BC_DisplayInfo(const char *display_name, int show_error)
{
	init_window(display_name, show_error);
}

BC_DisplayInfo::~BC_DisplayInfo()
{
	XCloseDisplay(display);
}


void BC_DisplayInfo::parse_geometry(char *geom, int *x, int *y, int *width, int *height)
{
	XParseGeometry(geom, x, y, (unsigned int*)width, (unsigned int*)height);
}

void BC_DisplayInfo::test_window(int &x_out, 
	int &y_out, 
	int &x_out2, 
	int &y_out2)
{
	unsigned long mask = CWEventMask;
	XSetWindowAttributes attr;
	XSizeHints size_hints;
	char *txlist[2];
	XTextProperty titleprop;

	x_out = 0;
	y_out = 0;
	x_out2 = 0;
	y_out2 = 0;
	attr.event_mask = StructureNotifyMask;

	Window win = XCreateWindow(display, 
			rootwin, 
			TEST_X,
			TEST_Y,
			TEST_SIZE, 
			TEST_SIZE, 
			0, 
			default_depth, 
			InputOutput, 
			vis, 
			mask, 
			&attr);
	size_hints.flags = PPosition | PSize;
	size_hints.x = TEST_X;
	size_hints.y = TEST_Y;
	size_hints.width = TEST_SIZE;
	size_hints.height = TEST_SIZE;
	// Set the name of the window
	// Makes possible to create special config for the window
	txlist[0] = (char *)"guicast_test";
	txlist[1] = 0;
	XmbTextListToTextProperty(display, txlist, 1,
		XStdICCTextStyle, &titleprop);
	XSetWMProperties(display, win, &titleprop, &titleprop,
		0, 0, &size_hints, 0, 0);
	XFree(titleprop.value);

	XMapWindow(display, win); 
	XFlush(display);
	XSync(display, 0);
	// Wait until WM reacts
	usleep(20000);
	XDestroyWindow(display, win);
	XFlush(display);
	XSync(display, 0);

	int xm = -1, ym = -1;
	XEvent event;

	for(;;)
	{
		XNextEvent(display, &event);
		if(event.type == ConfigureNotify && event.xconfigure.window == win)
		{
			if(xm < event.xconfigure.x)
				xm = event.xconfigure.x;
			if(ym < event.xconfigure.y)
				ym = event.xconfigure.y;
		}
		if(event.type == DestroyNotify && event.xdestroywindow.window == win)
			break;
	}

// Create shift
	if(xm >= 0)
	{
		x_out = xm - TEST_X;
		y_out = ym - TEST_Y;
	}
}

void BC_DisplayInfo::init_borders()
{
	if(top_border < 0)
	{

		test_window(left_border, 
			top_border, 
			auto_reposition_x, 
			auto_reposition_y);
		right_border = left_border;
		bottom_border = left_border;
// printf("BC_DisplayInfo::init_borders border=%d %d auto=%d %d\n", 
// left_border, 
// top_border, 
// auto_reposition_x, 
// auto_reposition_y);
	}
}


int BC_DisplayInfo::get_top_border()
{
	init_borders();
	return top_border;
}

int BC_DisplayInfo::get_left_border()
{
	init_borders();
	return left_border;
}

int BC_DisplayInfo::get_right_border()
{
	init_borders();
	return right_border;
}

int BC_DisplayInfo::get_bottom_border()
{
	init_borders();
	return bottom_border;
}

void BC_DisplayInfo::init_window(const char *display_name, int show_error)
{
	if(display_name && display_name[0] == 0) display_name = NULL;
	
// This function must be the first Xlib
// function a multi-threaded program calls
	XInitThreads();

	if((display = XOpenDisplay(display_name)) == NULL)
	{
		if(show_error)
		{
  			printf("BC_DisplayInfo::init_window: cannot connect to X server.\n");
  			if(getenv("DISPLAY") == NULL)
    			printf("'DISPLAY' environment variable not set.\n");
			exit(1);
		}
		return;
 	}
	
	screen = DefaultScreen(display);
	rootwin = RootWindow(display, screen);
	vis = DefaultVisual(display, screen);
	default_depth = DefaultDepth(display, screen);
}


int BC_DisplayInfo::get_root_w()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return WidthOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_root_h()
{
	Screen *screen_ptr = XDefaultScreenOfDisplay(display);
	return HeightOfScreen(screen_ptr);
}

int BC_DisplayInfo::get_abs_cursor_x()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
	return abs_x;
}

int BC_DisplayInfo::get_abs_cursor_y()
{
	int abs_x, abs_y, win_x, win_y;
	unsigned int temp_mask;
	Window temp_win;

	XQueryPointer(display, 
	   rootwin, 
	   &temp_win, 
	   &temp_win,
       &abs_x, 
	   &abs_y, 
	   &win_x, 
	   &win_y, 
	   &temp_mask);
	return abs_y;
}
