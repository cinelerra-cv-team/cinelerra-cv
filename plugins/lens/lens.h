
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

#ifndef LENS_H
#define LENS_H


#include "bchash.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "thread.h"


class LensEngine;
class LensGUI;
class LensMain;
class LensText;

#define FOV_CHANNELS 4


class LensSlider : public BC_FSlider
{
public:
	LensSlider(LensMain *client, 
		LensGUI *gui,
		LensText *text,
		float *output, 
		int x, 
		int y, 
		float min,
		float max);
	int handle_event();

	LensGUI *gui;
	LensMain *client;
	LensText *text;
	float *output;
};

class LensText : public BC_TextBox
{
public:
	LensText(LensMain *client, 
		LensGUI *gui,
		LensSlider *slider,
		float *output, 
		int x, 
		int y);
	int handle_event();

	LensGUI *gui;
	LensMain *client;
	LensSlider *slider;
	float *output;
};


class LensToggle : public BC_CheckBox
{
public:
	LensToggle(LensMain *client, 
		int *output, 
		int x, 
		int y,
		const char *text);
	int handle_event();

	LensMain *client;
	int *output;
};


class LensMode : public BC_PopupMenu
{
public:
	LensMode(LensMain *plugin,  
		LensGUI *gui,
		int x,
		int y);
	int handle_event();
	void create_objects();
	static int calculate_w(LensGUI *gui);
	static int from_text(char *text);
	static const char* to_text(int mode);
	void update(int mode);
	LensMain *plugin;
	LensGUI *gui;
};


class LensPresets : public BC_PopupMenu
{
public:
	LensPresets(LensMain *plugin,  
		LensGUI *gui,
		int x,
		int y,
		int w);
	int handle_event();
	void create_objects();
	int from_text(LensMain *plugin, char *text);
	const char* to_text(LensMain *plugin, int preset);
	void update(int preset);
	LensMain *plugin;
	LensGUI *gui;
};

class LensSavePreset : public BC_GenericButton
{
public:
	LensSavePreset(LensMain *plugin,  
		LensGUI *gui,
		int x,
		int y);
	int handle_event();
	LensMain *plugin;
	LensGUI *gui;
};

class LensDeletePreset : public BC_GenericButton
{
public:
	LensDeletePreset(LensMain *plugin,  
		LensGUI *gui,
		int x,
		int y);
	int handle_event();
	LensMain *plugin;
	LensGUI *gui;
};

class LensPresetText : public BC_TextBox
{
public:
	LensPresetText(LensMain *plugin,  
		LensGUI *gui,
		int x,
		int y,
		int w);
	int handle_event();
	LensMain *plugin;
	LensGUI *gui;
};

class LensGUI : public PluginClientWindow
{
public:
	LensGUI(LensMain *client);
	~LensGUI();
	
	void create_objects();

	LensMain *client;
	LensSlider *fov_slider[FOV_CHANNELS];
	LensText *fov_text[FOV_CHANNELS];
	LensSlider *aspect_slider;
	LensText *aspect_text;
	LensSlider *radius_slider;
	LensText *radius_text;
	LensSlider *centerx_slider;
	LensText *centerx_text;
	LensSlider *centery_slider;
	LensText *centery_text;
	LensMode *mode;
//	LensPresets *presets;
//	LensSavePreset *save_preset;
//	LensDeletePreset *delete_preset;
//	LensPresetText *preset_text;
	LensToggle *reverse;
	LensToggle *draw_guides;
};

class LensConfig
{
public:
	LensConfig();
	int equivalent(LensConfig &that);
	void copy_from(LensConfig &that);
	void interpolate(LensConfig &prev, 
		LensConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void boundaries();
	float fov[FOV_CHANNELS];
	int lock;
	float aspect;
	float radius;
	float center_x;
	float center_y;
	int draw_guides;
	int mode;
	enum
	{
		SHRINK,
		STRETCH,
		RECTILINEAR_SHRINK,
		RECTILINEAR_STRETCH
	};
};

class LensPreset
{
public:
	char title[BCTEXTLEN];
	float fov[FOV_CHANNELS];
	float aspect;
	float radius;
	int mode;
};





class LensPackage : public LoadPackage
{
public:
	LensPackage();
	int row1, row2;
};


class LensUnit : public LoadClient
{
public:
	LensUnit(LensEngine *engine, LensMain *plugin);
	~LensUnit();
	void process_package(LoadPackage *package);
	void process_stretch(LensPackage *pkg);
	void process_shrink(LensPackage *pkg);
	void process_rectilinear_stretch(LensPackage *pkg);
	void process_rectilinear_shrink(LensPackage *pkg);
	LensEngine *engine;
	LensMain *plugin;
};

class LensEngine : public LoadServer
{
public:
	LensEngine(LensMain *plugin);
	~LensEngine();
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	LensMain *plugin;
};

class LensMain : public PluginVClient
{
public:
	LensMain(PluginServer *server);
	~LensMain();

	PLUGIN_CLASS_MEMBERS(LensConfig)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_presets();
	void save_presets();
	int handle_opengl();
	
	LensEngine *engine;
	int lock;
	int current_preset;
	ArrayList<LensPreset*> presets;
};



#endif
