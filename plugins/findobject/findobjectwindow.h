
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
 

#ifndef FINDOBJECTWINDOW_H
#define FINDOBJECTWINDOW_H

#include "guicast.h"
#include "findobject.inc"

class FindObjectLayer : public BC_TumbleTextBox
{
public:
	FindObjectLayer(FindObjectMain *plugin, 
		FindObjectWindow *gui, 
		int x, 
		int y,
		int *value);
	int handle_event();
	static int calculate_w(FindObjectWindow *gui);
	FindObjectMain *plugin;
	FindObjectWindow *gui;
	int *value;
};

class FindObjectGlobalRange : public BC_IPot
{
public:
	FindObjectGlobalRange(FindObjectMain *plugin, 
		int x, 
		int y,
		int *value);
	int handle_event();
	FindObjectMain *plugin;
	int *value;
};

class FindObjectBlockSize : public BC_FPot
{
public:
	FindObjectBlockSize(FindObjectMain *plugin, 
		int x, 
		int y,
		float *value);
	int handle_event();
	FindObjectMain *plugin;
	float *value;
};

class FindObjectBlockCenterText;

class FindObjectBlockCenter : public BC_FPot
{
public:
	FindObjectBlockCenter(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y,
		float *value);
	int handle_event();
	FindObjectWindow *gui;
	FindObjectMain *plugin;
	FindObjectBlockCenterText *center_text;
	float *value;
};

class FindObjectBlockCenterText : public BC_TextBox
{
public:
	FindObjectBlockCenterText(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y,
		float *value);
	int handle_event();
	FindObjectWindow *gui;
	FindObjectMain *plugin;
	FindObjectBlockCenter *center;
	float *value;
};



class FindObjectDrawBorder : public BC_CheckBox
{
public:
	FindObjectDrawBorder(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectDrawKeypoints : public BC_CheckBox
{
public:
	FindObjectDrawKeypoints(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectReplace : public BC_CheckBox
{
public:
	FindObjectReplace(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};


class FindObjectDrawObjectBorder : public BC_CheckBox
{
public:
	FindObjectDrawObjectBorder(FindObjectMain *plugin, 
		FindObjectWindow *gui,
		int x, 
		int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};




class FindObjectAlgorithm : public BC_PopupMenu
{
public:
	FindObjectAlgorithm(FindObjectMain *plugin, 
		FindObjectWindow *gui, 
		int x, 
		int y);
	int handle_event();
	void create_objects();
	static int calculate_w(FindObjectWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};


class FindObjectCamParam : public BC_IPot
{
public:
	FindObjectCamParam(FindObjectMain *plugin, 
		int x, 
		int y,
		int *value);
	int handle_event();
	FindObjectMain *plugin;
	int *value;
};


class FindObjectBlend : public BC_IPot
{
public:
	FindObjectBlend(FindObjectMain *plugin, 
		int x, 
		int y,
		int *value);
	int handle_event();
	FindObjectMain *plugin;
	int *value;
};


class FindObjectWindow : public PluginClientWindow
{
public:
	FindObjectWindow(FindObjectMain *plugin);
	~FindObjectWindow();

	void create_objects();
	char* get_radius_title();

	FindObjectGlobalRange *global_range_w;
	FindObjectGlobalRange *global_range_h;
	FindObjectBlockSize *global_block_w;
	FindObjectBlockSize *global_block_h;
	FindObjectBlockCenter *block_x;
	FindObjectBlockCenter *block_y;
	FindObjectBlockCenterText *block_x_text;
	FindObjectBlockCenterText *block_y_text;
	FindObjectDrawKeypoints *draw_keypoints;
	FindObjectDrawBorder *draw_border;
	FindObjectReplace *replace_object;
	FindObjectDrawObjectBorder *draw_object_border;
	FindObjectLayer *object_layer;
	FindObjectLayer *scene_layer;
	FindObjectLayer *replace_layer;
	FindObjectAlgorithm *algorithm;
	FindObjectCamParam *vmin;
	FindObjectCamParam *vmax;
	FindObjectCamParam *smin;
	FindObjectBlend *blend;
	FindObjectMain *plugin;
};





#endif // FINDOBJECTWINDOW_H



