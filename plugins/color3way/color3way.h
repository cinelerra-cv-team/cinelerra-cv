
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

#ifndef COLOR3WAY_H
#define COLOR3WAY_H

class Color3WayMain;
class Color3WayEngine;

#define SHADOWS 0
#define MIDTONES 1
#define HIGHLIGHTS 2
#define SECTIONS 3
#define MAX_COLOR 1.0
#define ROOT_2 1.414214

#include "color3waywindow.h"
#include "condition.h"
#include "cicolors.h"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "thread.h"


class Color3WayConfig
{
public:
	Color3WayConfig();

	int equivalent(Color3WayConfig &that);
	void copy_from(Color3WayConfig &that);
	void interpolate(Color3WayConfig &prev, 
		Color3WayConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();
	void copy_to_all(int section);

	float hue_x[SECTIONS];
	float hue_y[SECTIONS];
	float value[SECTIONS];
	float saturation[SECTIONS];
};





class Color3WayPackage : public LoadPackage
{
public:
	Color3WayPackage();
	int row1, row2;
};


class Color3WayUnit : public LoadClient
{
public:
	Color3WayUnit(Color3WayMain *plugin, Color3WayEngine *server);
	void process_package(LoadPackage *package);
	Color3WayMain *plugin;
	YUV yuv;
};

class Color3WayEngine : public LoadServer
{
public:
	Color3WayEngine(Color3WayMain *plugin, int cpus);
	~Color3WayEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	Color3WayMain *plugin;
};




class Color3WayMain : public PluginVClient
{
public:
	Color3WayMain(PluginServer *server);
	~Color3WayMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(Color3WayConfig);
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();

	void get_aggregation(int *aggregate_interpolate,
		int *aggregate_gamma);

	void calculate_factors(float *r, float *g, float *b, int section);
	void calculate_factors(float *r, float *g, float *b, float x, float y);
	void process_pixel(float *r,
		float *g,
		float *b,
		float r_in, 
		float g_in, 
		float b_in,
		float x,
		float y);

// parameters needed for processor
	int reconfigure();

	Color3WayEngine *engine;
	int total_engines;


    int r_lookup_8[0x100];
    int g_lookup_8[0x100];
    int b_lookup_8[0x100];
    int r_lookup_16[0x10000];
    int g_lookup_16[0x10000];
    int b_lookup_16[0x10000];
    int redo_buffers;
	int need_reconfigure;
	int copy_to_all[SECTIONS];
	int w, h;
};



#endif
