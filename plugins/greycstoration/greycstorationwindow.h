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

#ifndef GREYCSTORATIONWINDOW_H
#define GREYCSTORATIONWINDOW_H

class GreyCStorationThread;
class PluginWindow;

#include "filexml.inc"
#include "greycstorationplugin.h"
#include "mutex.h"
#include "pluginvclient.h"

PLUGIN_THREAD_HEADER(GreyCStorationMain, GreyCStorationThread, PluginWindow)

class GreyCAmpSlider;
class GreyCSharpSlider;
class GreyCAniSlider;
class GreyCNoiseSlider;

class PluginWindow : public BC_Window
{
public:
	PluginWindow(GreyCStorationMain *client, int x, int y);
	~PluginWindow();

	void create_objects();
	int close_event();

	GreyCStorationMain *client;

	// probably stupid way to accomplish this
	GreyCAmpSlider *greycamp_slider;
	GreyCSharpSlider *greycsharp_slider;
	GreyCAniSlider *greycani_slider;
	GreyCNoiseSlider * greycnoise_slider;
};


class GreyCAmpSlider : public BC_ISlider
{
public:
	GreyCAmpSlider(GreyCStorationMain *client, float *output, int x, int y);
	~GreyCAmpSlider();
	int handle_event();

	GreyCStorationMain *client;
	float *output;
};


class GreyCSharpSlider : public BC_FSlider
{
public:
	GreyCSharpSlider(GreyCStorationMain *client, float *output, int x, int y);
	~GreyCSharpSlider();
	int handle_event();

	GreyCStorationMain *client;
	float *output;
};


class GreyCAniSlider : public BC_FSlider
{
public:
	GreyCAniSlider(GreyCStorationMain *client, float *output, int x, int y);
	~GreyCAniSlider();
	int handle_event();

	GreyCStorationMain *client;
	float *output;
};

class GreyCNoiseSlider : public BC_FSlider
{
public:
	GreyCNoiseSlider(GreyCStorationMain *client, float *output, int x, int y);
	~GreyCNoiseSlider();
	int handle_event();

	GreyCStorationMain *client;
	float *output;
};

#endif
