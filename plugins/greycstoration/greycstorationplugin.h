/*
 * GreyCStoration plugin for Cinelerra
 * Copyright (C) 2013 Slock Ruddy
 * Copyright (C) 2014-2015 Nicola Ferralis <feranick at hotmail dot com>
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

#ifndef GREYCSTORATION_H
#define GREYCSTORATION_H


class GreyCStorationMain;

#include "filexml.h"
#include "greycstorationwindow.h"
#include "guicast.h"
#include "pluginvclient.h"

class GreyCStorationConfig
{
public:
	GreyCStorationConfig();
	void copy_from(GreyCStorationConfig &that);
	int equivalent(GreyCStorationConfig &that);
	void interpolate(GreyCStorationConfig &prev,
		GreyCStorationConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);
	float amplitude    ;//   = 40.0f; //cimg_option("-dt",40.0f,"Regularization strength for one iteration (>=0)");
    float sharpness    ;//   = 0.8f; // cimg_option("-p",0.8f,"Contour preservation for regularization (>=0)");
    float anisotropy   ;//   = 0.8f; // cimg_option("-a",0.8f,"Regularization anisotropy (0<=a<=1)");
    float noise_scale ; // alpha
};

class GreyCStorationMain : public PluginVClient
{
public:
	GreyCStorationMain(PluginServer *server);
	~GreyCStorationMain();

	PLUGIN_CLASS_MEMBERS(GreyCStorationConfig, GreyCStorationThread);
	template<typename T> void GREYCSTORATION(VFrame *frame, int h, int w, int components);

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
};


#endif
