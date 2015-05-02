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

#include "bcdisplayinfo.h"
#include "greycstorationwindow.h"

#include "language.h"
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PLUGIN_THREAD_OBJECT(GreyCStorationMain, GreyCStorationThread, PluginWindow)

// configuration window
PluginWindow::PluginWindow(GreyCStorationMain *client, int x, int y)
 : BC_Window(client->get_gui_string(),
	x,
	y,
	300, // width
	180, // height
	300,
	180,
	0,
	0,
	1)
{
	this->client = client;
}

PluginWindow::~PluginWindow()
{
}

// controls in window
void PluginWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y + 10, _("Amplitude:")));
	add_tool(greycamp_slider = new GreyCAmpSlider(client, &(client->config.amplitude), x+title->get_w(), y));

	y += 30;
	add_subwindow(title = new BC_Title(x, y + 10, _("Sharpness:")));
	add_tool(greycsharp_slider = new GreyCSharpSlider(client, &(client->config.sharpness), x+title->get_w(), y));

	y += 30;
	add_subwindow(title = new BC_Title(x, y + 10, _("Anisotropy:")));
	add_tool(greycani_slider = new GreyCAniSlider(client, &(client->config.anisotropy), x+title->get_w(), y));

	y += 30;
	add_subwindow(title = new BC_Title(x, y + 10, _("Noise scale:")));
	add_tool(greycnoise_slider = new GreyCNoiseSlider(client, &(client->config.noise_scale), x+title->get_w(), y));


	show_window();
	flush();
}

int PluginWindow::close_event()
{
	set_done(1);
	return 1;
}


// amp slider implementation

GreyCAmpSlider::GreyCAmpSlider(GreyCStorationMain *client, float *output, int x, int y)
 : BC_ISlider(x,
	y,
	0,
	200,
	200,
	0,
	255, //MAX
	(int)*output,
	0,
	0,
	0)
{
	this->client = client;
	this->output = output;
}
GreyCAmpSlider::~GreyCAmpSlider()
{
}
int GreyCAmpSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}

// sharpslider


GreyCSharpSlider::GreyCSharpSlider(GreyCStorationMain *client, float *output, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0f,
	10.0f, //MAX
	(float)*output,
	0,
	0
	)
{
	this->client = client;
	this->output = output;
}

GreyCSharpSlider::~GreyCSharpSlider()
{
}

int GreyCSharpSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}

// anislider


GreyCAniSlider::GreyCAniSlider(GreyCStorationMain *client, float *output, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0f,
	1.0f, //MAX
	(float)*output,
	0,
	0
	)
{
	this->client = client;
	this->output = output;
}

GreyCAniSlider::~GreyCAniSlider()
{
}

int GreyCAniSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}


// noise scale

GreyCNoiseSlider::GreyCNoiseSlider(GreyCStorationMain *client, float *output, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0f,
	10.0f, //MAX
	(float)*output,
	0,
	0
	)
{
	this->client = client;
	this->output = output;
}

GreyCNoiseSlider::~GreyCNoiseSlider()
{
}

int GreyCNoiseSlider::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}
