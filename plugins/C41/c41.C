/*
 * C41 plugin for Cinelerra
 * Copyright (C) 2011 Florent Delannoy <florent at plui dot es>
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
#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

/* Class declarations */
class C41Effect;
class C41Window;

struct magic
{
	float min_r;
	float min_g;
	float min_b;
	float light;
	float gamma_g;
	float gamma_b;
};

class C41Config
{
public:
	C41Config();

	void copy_from(C41Config &src);
	int equivalent(C41Config &src);
	void interpolate(C41Config &prev,
			C41Config &next,
			long prev_frame,
			long next_frame,
			long current_frame);

	int active;
	int compute_magic;
	float fix_min_r;
	float fix_min_g;
	float fix_min_b;
	float fix_light;
	float fix_gamma_g;
	float fix_gamma_b;
};

class C41Enable : public BC_CheckBox
{
public:
	C41Enable(C41Effect *plugin, int *output, int x, int y, char *text);
	int handle_event();
	C41Effect *plugin;
	int *output;
};

class C41TextBox : public BC_TextBox
{
public:
	C41TextBox(C41Effect *plugin, float *value, int x, int y);
	int handle_event();
	C41Effect *plugin;
	float *boxValue;
};

class C41Button : public BC_GenericButton
{
public:
	C41Button(C41Effect *plugin, C41Window *window, int x, int y);
	int handle_event();
	C41Effect *plugin;
	C41Window *window;
	float *boxValue;
};

class C41Window : public PluginWindow
{
public:
	C41Window(C41Effect *plugin, int x, int y);
	void create_objects();
	int close_event();
	void update();
	void update_magic();
	C41Enable *active;
	C41Enable *compute_magic;
	BC_Title *min_r;
	BC_Title *min_g;
	BC_Title *min_b;
	BC_Title *light;
	BC_Title *gamma_g;
	BC_Title *gamma_b;
	C41TextBox *fix_min_r;
	C41TextBox *fix_min_g;
	C41TextBox *fix_min_b;
	C41TextBox *fix_light;
	C41TextBox *fix_gamma_g;
	C41TextBox *fix_gamma_b;
	C41Button *lock;
	C41Effect *plugin;
};

PLUGIN_THREAD_HEADER(C41Effect, C41Thread, C41Window);

class C41Effect : public PluginVClient
{
public:
	C41Effect(PluginServer *server);
	~C41Effect();
	int process_buffer(VFrame *frame,
			int64_t start_position,
			double frame_rate);
	int is_realtime();
	const char* plugin_title();
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_gui(void* data);
	int show_gui();
	void raise_window();
	int set_string();
	int load_configuration();
	float myLog2(float i) __attribute__ ((optimize(0)));
	float myPow2(float i) __attribute__ ((optimize(0)));
	float myPow(float a, float b);
	double difftime_nano(timespec start, timespec end);

	struct magic values;

	C41Config config;
	C41Thread *thread;
	BC_Hash *defaults;
};


REGISTER_PLUGIN(C41Effect);


/* Methods decarations */

// C41Config
C41Config::C41Config()
{
	active = 0;
	compute_magic = 0;

	fix_min_r = fix_min_g = fix_min_b = fix_light = fix_gamma_g = fix_gamma_b = 0.;
}

void C41Config::copy_from(C41Config &src)
{
	active = src.active;
	compute_magic = src.compute_magic;

	fix_min_r = src.fix_min_r;
	fix_min_g = src.fix_min_g;
	fix_min_b = src.fix_min_b;
	fix_light = src.fix_light;
	fix_gamma_g = src.fix_gamma_g;
	fix_gamma_b = src.fix_gamma_b;
}

int C41Config::equivalent(C41Config &src)
{
	return (src.active == active &&
		compute_magic == src.compute_magic &&
		EQUIV(src.fix_min_r, fix_min_r) &&
		EQUIV(src.fix_min_g, fix_min_g) &&
		EQUIV(src.fix_min_b, fix_min_b) &&
		EQUIV(src.fix_light, fix_light) &&
		EQUIV(src.fix_gamma_g, fix_gamma_g) &&
		EQUIV(src.fix_gamma_b, fix_gamma_b));
}

void C41Config::interpolate(C41Config &prev,
		C41Config &next,
		long prev_frame,
		long next_frame,
		long current_frame)
{
	active = prev.active;
	compute_magic = prev.compute_magic;

	fix_min_r = prev.fix_min_r;
	fix_min_g = prev.fix_min_g;
	fix_min_b = prev.fix_min_b;
	fix_light = prev.fix_light;
	fix_gamma_g = prev.fix_gamma_g;
	fix_gamma_b = prev.fix_gamma_b;
}

// C41Enable
C41Enable::C41Enable(C41Effect *plugin, int *output, int x, int y, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}

int C41Enable::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

// C41TextBox
C41TextBox::C41TextBox(C41Effect *plugin, float *value, int x, int y)
 : BC_TextBox(x, y, 160, 1, *value)
{
	this->plugin = plugin;
	this->boxValue = value;
}

int C41TextBox::handle_event()
{
	*boxValue = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


// C41Button
C41Button::C41Button(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Lock parameters"))
{
	this->plugin = plugin;
	this->window = window;
}

int C41Button::handle_event()
{
	plugin->config.fix_min_r = plugin->values.min_r;
	plugin->config.fix_min_g = plugin->values.min_g;
	plugin->config.fix_min_b = plugin->values.min_b;
	plugin->config.fix_light = plugin->values.light;
	plugin->config.fix_gamma_g = plugin->values.gamma_g;
	plugin->config.fix_gamma_b = plugin->values.gamma_b;

	window->update();
	plugin->send_configure_change();
	return 1;
}

// C41Window
C41Window::C41Window(C41Effect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 270, 620)
{
	this->plugin = plugin;
}

void C41Window::create_objects()
{
	int x = 10;
	int y = 10;

	add_subwindow(active = new C41Enable(plugin, &plugin->config.active, x, y, _("Activate processing")));
	y += 40;

	add_subwindow(compute_magic = new C41Enable(plugin, &plugin->config.compute_magic, x, y, _("Compute negfix values")));
	y += 20;
	add_subwindow(new BC_Title(x + 20, y, _("(uncheck for faster rendering)")));
	y += 40;

	add_subwindow(new BC_Title(x, y, _("Computed negfix values:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min R:")));
	add_subwindow(min_r = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(min_g = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(min_b = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(light = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(gamma_g = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(gamma_b = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(lock = new C41Button(plugin, this, x, y));
	y += 30;

	y += 20;
	add_subwindow(new BC_Title(x, y, _("negfix values to apply:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min R:")));
	add_subwindow(fix_min_r = new C41TextBox(plugin, &plugin->config.fix_min_r, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(fix_min_g = new C41TextBox(plugin, &plugin->config.fix_min_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(fix_min_b = new C41TextBox(plugin, &plugin->config.fix_min_b, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(fix_light = new C41TextBox(plugin, &plugin->config.fix_light, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(fix_gamma_g = new C41TextBox(plugin, &plugin->config.fix_gamma_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(fix_gamma_b = new C41TextBox(plugin, &plugin->config.fix_gamma_b, x + 80, y));
	y += 30;
	show_window();
	flush();
	update_magic();
}

void C41Window::update()
{
	active->update(plugin->config.active);
	compute_magic->update(plugin->config.compute_magic);

	fix_min_r->update(plugin->config.fix_min_r);
	fix_min_g->update(plugin->config.fix_min_g);
	fix_min_b->update(plugin->config.fix_min_b);
	fix_light->update(plugin->config.fix_light);
	fix_gamma_g->update(plugin->config.fix_gamma_g);
	fix_gamma_b->update(plugin->config.fix_gamma_b);

	update_magic();
}

void C41Window::update_magic()
{
	min_r->update(plugin->values.min_r);
	min_g->update(plugin->values.min_g);
	min_b->update(plugin->values.min_b);
	light->update(plugin->values.light);
	gamma_g->update(plugin->values.gamma_g);
	gamma_b->update(plugin->values.gamma_b);
}

WINDOW_CLOSE_EVENT(C41Window);
PLUGIN_THREAD_OBJECT(C41Effect, C41Thread, C41Window);

// C41Effect
C41Effect::C41Effect(PluginServer *server)
 : PluginVClient(server)
{
	memset(&values, 0, sizeof(values));
	PLUGIN_CONSTRUCTOR_MACRO
}

C41Effect::~C41Effect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

const char* C41Effect::plugin_title() { return N_("C41"); }

int C41Effect::is_realtime() { return 1; }

NEW_PICON_MACRO(C41Effect)
SHOW_GUI_MACRO(C41Effect, C41Thread)
RAISE_WINDOW_MACRO(C41Effect)
SET_STRING_MACRO(C41Effect)
LOAD_CONFIGURATION_MACRO(C41Effect, C41Config)


void C41Effect::update_gui()
{
	if(thread && load_configuration())
	{
		thread->window->lock_window("C41Effect::update_gui");
		thread->window->update();
		thread->window->unlock_window();
	}
}

void C41Effect::render_gui(void* data)
{
	// Updating values computed by process_frame
	struct magic *vp = (struct magic *)data;

	values = *vp;
	if(thread)
		thread->window->update_magic();
}

int C41Effect::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%sC41.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	config.active = defaults->get("ACTIVE", config.active);
	config.compute_magic = defaults->get("COMPUTE_MAGIC", config.compute_magic);

	config.fix_min_r = defaults->get("FIX_MIN_R", config.fix_min_r);
	config.fix_min_g = defaults->get("FIX_MIN_G", config.fix_min_g);
	config.fix_min_b = defaults->get("FIX_MIN_B", config.fix_min_b);
	config.fix_light = defaults->get("FIX_LIGHT", config.fix_light);
	config.fix_gamma_g = defaults->get("FIX_GAMMA_G", config.fix_gamma_g);
	config.fix_gamma_b = defaults->get("FIX_GAMMA_B", config.fix_gamma_b);

	return 0;
}

int C41Effect::save_defaults()
{
	defaults->update("ACTIVE", config.active);
	defaults->update("COMPUTE_MAGIC", config.compute_magic);

	defaults->update("FIX_MIN_R", config.fix_min_r);
	defaults->update("FIX_MIN_G", config.fix_min_g);
	defaults->update("FIX_MIN_B", config.fix_min_b);
	defaults->update("FIX_LIGHT", config.fix_light);
	defaults->update("FIX_GAMMA_G", config.fix_gamma_g);
	defaults->update("FIX_GAMMA_B", config.fix_gamma_b);
	defaults->save();
	return 0;
}

void C41Effect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("C41");
	output.tag.set_property("ACTIVE", config.active);
	output.tag.set_property("COMPUTE_MAGIC", config.compute_magic);

	output.tag.set_property("FIX_MIN_R", config.fix_min_r);
	output.tag.set_property("FIX_MIN_G", config.fix_min_g);
	output.tag.set_property("FIX_MIN_B", config.fix_min_b);
	output.tag.set_property("FIX_LIGHT", config.fix_light);
	output.tag.set_property("FIX_GAMMA_G", config.fix_gamma_g);
	output.tag.set_property("FIX_GAMMA_B", config.fix_gamma_b);

	output.append_tag();
	output.tag.set_title("/C41");
	output.append_tag();
	output.terminate_string();
}

void C41Effect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("C41"))
		{
			config.active = input.tag.get_property("ACTIVE", config.active);
			config.compute_magic = input.tag.get_property("COMPUTE_MAGIC", config.compute_magic);

			config.fix_min_r = input.tag.get_property("FIX_MIN_R", config.fix_min_r);
			config.fix_min_g = input.tag.get_property("FIX_MIN_G", config.fix_min_g);
			config.fix_min_b = input.tag.get_property("FIX_MIN_B", config.fix_min_b);
			config.fix_light = input.tag.get_property("FIX_LIGHT", config.fix_light);
			config.fix_gamma_g = input.tag.get_property("FIX_GAMMA_G", config.fix_gamma_g);
			config.fix_gamma_b = input.tag.get_property("FIX_GAMMA_B", config.fix_gamma_b);
		}
	}
}


/* Faster pow() approximation; borrowed from http://www.dctsystems.co.uk/Software/power.html
 * Tests on real-world data showed a max error of 4% and avg. error or .1 to .5%,
 * while accelerating rendering by a factor of 4.
 */
float C41Effect::myLog2(float i)
{
	float x;
	float y;
	float LogBodge = 0.346607f;
	x = *(int *)&i;
	x *= 1.0 / (1 << 23); // 1/pow(2,23);
	x = x - 127;

	y = x - floorf(x);
	y = (y - y * y) * LogBodge;
	return x + y;
}

float C41Effect::myPow2(float i)
{
	float PowBodge = 0.33971f;
	float x;
	float y = i - floorf(i);
	y = (y - y * y) * PowBodge;

	x = i + 127 - y;
	x *= (1 << 23);
	*(int*) &x = (int)x;
	return x;
}

float C41Effect::myPow(float a, float b)
{
	return myPow2(b * myLog2(a));
}


int C41Effect::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	load_configuration();

	read_frame(frame,
			0,
			start_position,
			frame_rate,
			0);

	int frame_w = frame->get_w();
	int frame_h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
		case BC_RGBA_FLOAT:
		case BC_RGBA8888:
		case BC_YUVA8888:
		case BC_RGB161616:
		case BC_YUV161616:
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			return 0; // Unsupported
		case BC_RGB_FLOAT:
			break;
	}

	if(config.compute_magic)
	{
		// Box blur!
		VFrame* tmp_frame = new VFrame(*frame);
		VFrame* blurry_frame = new VFrame(*frame);

		float** rows = (float**)frame->get_rows();
		float** tmp_rows = (float**)tmp_frame->get_rows();
		float** blurry_rows = (float**)blurry_frame->get_rows();
		for(int i = 0; i < frame_h; i++)
			for(int j = 0; j < (3*frame_w); j++)
				blurry_rows[i][j] = rows[i][j];

		int boxw = 5, boxh = 5;
		// 10 passes of Box blur should be good
		int pass;
		int x;
		int y;
		int y_up;
		int y_down;
		int x_right;
		int x_left;
		float component;
		for(pass=0; pass<10; pass++)
		{
			for(y = 0; y < frame_h; y++)
				for(x = 0; x < (3 * frame_w); x++)
					tmp_rows[y][x] = blurry_rows[y][x];
			for(y = 0; y < frame_h; y++)
			{
				y_up = (y - boxh < 0)? 0 : y - boxh;
				y_down = (y + boxh >= frame_h)? frame_h - 1 : y + boxh;
				for(x = 0; x < (3*frame_w); x++)
				{
					x_left = (x-(3*boxw) < 0)? 0 : x-(3*boxw);
					x_right = (x+(3*boxw) >= (3*frame_w))? (3*frame_w)-1 : x+(3*boxw);
					component=(tmp_rows[y_down][x_right]
							+tmp_rows[y_up][x_left]
							+tmp_rows[y_up][x_right]
							+tmp_rows[y_down][x_right])/4;
					blurry_rows[y][x]= component;
				}
			}
		}

		// Compute magic negfix values
		float minima_r = 50.;
		float minima_g = 50.;
		float minima_b = 50.;
		float maxima_r = 0.;
		float maxima_g = 0.;
		float maxima_b = 0.;

		// Shave the image in order to avoid black borders
		// Tolerance default: 5%, i.e. 0.05
#define TOLERANCE 0.20
#define SKIP_ROW if (i < (TOLERANCE * frame_h) || i > ((1-TOLERANCE)*frame_h)) continue
#define SKIP_COL if (j < (TOLERANCE * frame_w) || j > ((1-TOLERANCE)*frame_w)) continue

		for(int i = 0; i < frame_h; i++)
		{
			SKIP_ROW;
			float *row = (float*)blurry_frame->get_rows()[i];
			for(int j = 0; j < frame_w; j++, row += 3)
			{
				SKIP_COL;
				if(row[0] < minima_r) minima_r = row[0];
				if(row[0] > maxima_r) maxima_r = row[0];

				if(row[1] < minima_g) minima_g = row[1];
				if(row[1] > maxima_g) maxima_g = row[1];

				if(row[2] < minima_b) minima_b = row[2];
				if(row[2] > maxima_b) maxima_b = row[2];
			}
		}

		// Delete the VFrames we used for blurring
		delete tmp_frame;
		delete blurry_frame;

		values.min_r = minima_r;
		values.min_g = minima_g;
		values.min_b = minima_b;
		values.light = (minima_r / maxima_r) * 0.95;
		values.gamma_g = logf(maxima_r / minima_r) / logf(maxima_g / minima_g);
		values.gamma_b = logf(maxima_r / minima_r) / logf(maxima_b / minima_b);

		// Update GUI
		send_render_gui(&values);
	}

	// Apply the transformation
	if(config.active)
	{
		// Get the values from the config instead of the computed ones
		for(int i = 0; i < frame_h; i++)
		{
			float *row = (float*)frame->get_rows()[i];
			for(int j = 0; j < frame_w; j++, row += 3)
			{
				row[0] = (config.fix_min_r / row[0]) - config.fix_light;

				row[1] = myPow((config.fix_min_g / row[1]), config.fix_gamma_g) - config.fix_light;

				row[2] = myPow((config.fix_min_b / row[2]), config.fix_gamma_b) - config.fix_light;
			}
		}
	}

	return 0;
}

