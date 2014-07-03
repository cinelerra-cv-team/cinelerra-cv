
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
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "lens.h"


#include <string.h>




REGISTER_PLUGIN(LensMain)




LensConfig::LensConfig()
{
	for(int i = 0; i < FOV_CHANNELS; i++)
		fov[i] = 1.0;
	aspect = 1.0;
	radius = 1.0;
	mode = LensConfig::SHRINK;
	center_x = 50.0;
	center_y = 50.0;
	draw_guides = 0;
}

int LensConfig::equivalent(LensConfig &that)
{
	for(int i = 0; i < FOV_CHANNELS; i++)
		if(!EQUIV(fov[i], that.fov[i])) return 0;
	return EQUIV(aspect, that.aspect) &&
		EQUIV(radius, that.radius) &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y) &&
		mode == that.mode &&
		draw_guides == that.draw_guides;
}

void LensConfig::copy_from(LensConfig &that)
{
	for(int i = 0; i < FOV_CHANNELS; i++)
		fov[i] = that.fov[i];
	aspect = that.aspect;
	radius = that.radius;
	mode = that.mode;
	center_x = that.center_x;
	center_y = that.center_y;
	draw_guides = that.draw_guides;
}

void LensConfig::interpolate(LensConfig &prev, 
	LensConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	for(int i = 0; i < FOV_CHANNELS; i++)
		fov[i] = prev.fov[i] * prev_scale + next.fov[i] * next_scale;
	aspect = prev.aspect * prev_scale + next.aspect * next_scale;
	radius = prev.radius * prev_scale + next.radius * next_scale;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
	mode = prev.mode;
	draw_guides = prev.draw_guides;

	boundaries();
}

void LensConfig::boundaries()
{
	CLAMP(center_x, 0.0, 99.0);
	CLAMP(center_y, 0.0, 99.0);
	for(int i = 0; i < FOV_CHANNELS; i++)
		CLAMP(fov[i], 0.0, 1.0);
	CLAMP(aspect, 0.3, 3.0);
	CLAMP(radius, 0.3, 3.0);
}




LensSlider::LensSlider(LensMain *client, 
	LensGUI *gui,
	LensText *text,
	float *output, 
	int x, 
	int y, 
	float min,
	float max)
 : BC_FSlider(x, y, 0, 200, 200, min, max, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->text = text;
	set_precision(0.01);
}

int LensSlider::handle_event()
{
	float prev_output = *output;
	*output = get_value();
	text->update(*output);

	float difference = *output - prev_output;
	int is_fov = 0;

	if(client->lock)
	{
		for(int i = 0; i < FOV_CHANNELS; i++)
		{
			if(output == &client->config.fov[i])
			{
				is_fov = 1;
				break;
			}
		}

		if(is_fov)
		{
			for(int i = 0; i < FOV_CHANNELS; i++)
			{
				if(output != &client->config.fov[i])
				{
					client->config.fov[i] += difference;
					client->config.boundaries();
					gui->fov_slider[i]->update(client->config.fov[i]);
					gui->fov_text[i]->update(client->config.fov[i]);
				}
			}
		}
	}

	client->send_configure_change();
	return 1;
}



LensText::LensText(LensMain *client, 
	LensGUI *gui,
	LensSlider *slider,
	float *output, 
	int x, 
	int y)
 : BC_TextBox(x, y, 100, 1, *output)
{
	this->gui = gui;
	this->client = client;
	this->output = output;
	this->slider = slider;
}

int LensText::handle_event()
{
	float prev_output = *output;
	*output = atof(get_text());
	slider->update(*output);

	float difference = *output - prev_output;
	int is_fov = 0;

	if(client->lock)
	{
		for(int i = 0; i < FOV_CHANNELS; i++)
		{
			if(output == &client->config.fov[i])
			{
				is_fov = 1;
				break;
			}
		}

		if(is_fov)
		{
			for(int i = 0; i < FOV_CHANNELS; i++)
			{
				if(output != &client->config.fov[i])
				{
					client->config.fov[i] += difference;
					client->config.boundaries();
					gui->fov_slider[i]->update(client->config.fov[i]);
					gui->fov_text[i]->update(client->config.fov[i]);
				}
			}
		}
	}

	client->send_configure_change();
	return 1;
}



LensToggle::LensToggle(LensMain *client, 
	int *output, 
	int x, 
	int y,
	const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
	this->client = client;
}

int LensToggle::handle_event()
{
	*output = get_value();
	client->send_configure_change();
	return 1;
}










LensMode::LensMode(LensMain *plugin,  
	LensGUI *gui,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	calculate_w(gui),
	"",
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int LensMode::handle_event()
{
	plugin->config.mode = from_text(get_text());
	plugin->send_configure_change();
	return 1;

}

void LensMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(LensConfig::SHRINK)));
	add_item(new BC_MenuItem(to_text(LensConfig::STRETCH)));
	add_item(new BC_MenuItem(to_text(LensConfig::RECTILINEAR_STRETCH)));
	add_item(new BC_MenuItem(to_text(LensConfig::RECTILINEAR_SHRINK)));
	update(plugin->config.mode);
}

void LensMode::update(int mode)
{
	char string[BCTEXTLEN];
	sprintf(string, "%s", to_text(mode));
	set_text(string);
}

int LensMode::calculate_w(LensGUI *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(LensConfig::STRETCH)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(LensConfig::SHRINK)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(LensConfig::RECTILINEAR_STRETCH)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(LensConfig::RECTILINEAR_SHRINK)));
	return result + 50;
}

int LensMode::from_text(char *text)
{
	if(!strcmp(text, "Sphere Stretch")) return LensConfig::STRETCH;
	else
	if(!strcmp(text, "Sphere Shrink")) return LensConfig::SHRINK;
	else
	if(!strcmp(text, "Rectilinear Stretch")) return LensConfig::RECTILINEAR_STRETCH;
	else
	if(!strcmp(text, "Rectilinear Shrink")) return LensConfig::RECTILINEAR_SHRINK;
	return LensConfig::STRETCH;
}

const char* LensMode::to_text(int mode)
{
	switch(mode)
	{
		case LensConfig::STRETCH:
			return "Sphere Stretch";
			break;
		case LensConfig::SHRINK:
			return "Sphere Shrink";
			break;
		case LensConfig::RECTILINEAR_STRETCH:
			return "Rectilinear Stretch";
			break;
		case LensConfig::RECTILINEAR_SHRINK:
			return "Rectilinear Shrink";
			break;
	}
	return "Stretch";
}





// LensPresets::LensPresets(LensMain *plugin,  
// 	LensGUI *gui,
// 	int x,
// 	int y,
// 	int w)
//  : BC_PopupMenu(x,
//  	y,
// 	w,
// 	"",
// 	1)
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int LensPresets::handle_event()
// {
// 	return 1;
// }
// 
// void LensPresets::create_objects()
// {
// // Remove existing items
// 	int total = total_items();
// 	for(int i = 0; i < total; i++)
// 	{
// 		BC_MenuItem *item = get_item(0);
// 		remove_item(item);
// 	}
// 
// // Create current items
// 	plugin->load_presets();
// 	for(int i = 0; i < plugin->presets.total; i++)
// 	{
// 		add_item(new BC_MenuItem(plugin->presets.values[i]->title));
// 	}
// 
// // Update text
// 	if(plugin->current_preset >= 0 &&
// 		plugin->current_preset < plugin->presets.total)
// 	{
// 		set_text(plugin->presets.values[plugin->current_preset]->title);
// 	}
// 	else
// 	{
// 		set_text("None");
// 	}
// }
// 
// int LensPresets::from_text(LensMain *plugin, char *text)
// {
// }
// 
// char* LensPresets::to_text(LensMain *plugin, int preset)
// {
// }
// 
// void LensPresets::update(int preset)
// {
// }
// 
// 
// 
// 
// 
// 
// LensSavePreset::LensSavePreset(LensMain *plugin,  
// 	LensGUI *gui,
// 	int x,
// 	int y)
//  : BC_GenericButton(x, y, "Save Preset")
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int LensSavePreset::handle_event()
// {
// }
// 
// 
// 
// 
// 
// 
// 
// LensDeletePreset::LensDeletePreset(LensMain *plugin,  
// 	LensGUI *gui,
// 	int x,
// 	int y)
//  : BC_GenericButton(x, y, "Delete Preset")
// {
// }
// 
// int LensDeletePreset::handle_event()
// {
// }
// 
// 
// 
// 
// 
// 
// 
// LensPresetText::LensPresetText(LensMain *plugin,  
// 	LensGUI *gui,
// 	int x,
// 	int y,
// 	int w)
//  : BC_TextBox(x, y, w, 1, "")
// {
// 	this->plugin = plugin;
// 	this->gui = gui;
// }
// 
// int LensPresetText::handle_event()
// {
// }













LensGUI::LensGUI(LensMain *client)
 : PluginClientWindow(client,
	350, 
	510, 
	350, 
	510, 
	0)
{
	this->client = client;
}

LensGUI::~LensGUI()
{
}


void LensGUI::create_objects()
{
	int x = 10;
	int y = 10;
	int x1;
	BC_Title *title;
	LensToggle *toggle;
	
	for(int i = 0; i < FOV_CHANNELS; i++)
	{
		switch(i)
		{
			case 0: add_tool(title = new BC_Title(x, y, _("R Field of View:"))); break;
			case 1: add_tool(title = new BC_Title(x, y, _("G Field of View:"))); break;
			case 2: add_tool(title = new BC_Title(x, y, _("B Field of View:"))); break;
			case 3: add_tool(title = new BC_Title(x, y, _("A Field of View:"))); break;
		}

		y += title->get_h() + 5;
		add_tool(fov_slider[i] = new LensSlider(client, 
			this,
			0,
			&client->config.fov[i], 
			x, 
			y, 
			0.0001,
			1.0));
		x1 = x + fov_slider[i]->get_w() + 5;
		add_tool(fov_text[i] = new LensText(client, 
			this,
			fov_slider[i],
			&client->config.fov[i], 
			x1, 
			y));
		fov_slider[i]->text = fov_text[i];
		y += fov_text[i]->get_h() + 5;
	}

	add_tool(toggle = new LensToggle(client, 
		&client->lock, 
		x, 
		y,
		"Lock"));
	y += toggle->get_h() + 10;
	
	BC_Bar *bar;
	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;

	add_tool(title = new BC_Title(x, y, _("Aspect Ratio:")));
	y += title->get_h() + 5;
	add_tool(aspect_slider = new LensSlider(client, 
		this,
		0,
		&client->config.aspect, 
		x, 
		y, 
		0.333,
		3.0));
	x1 = x + aspect_slider->get_w() + 5;
	add_tool(aspect_text = new LensText(client, 
		this,
		aspect_slider,
		&client->config.aspect, 
		x1, 
		y));
	aspect_slider->text = aspect_text;
	y += aspect_text->get_h() + 5;


	add_tool(title = new BC_Title(x, y, _("Radius:")));
	y += title->get_h() + 5;
	add_tool(radius_slider = new LensSlider(client, 
		this,
		0,
		&client->config.radius, 
		x, 
		y, 
		0.333,
		3.0));
	x1 = x + radius_slider->get_w() + 5;
	add_tool(radius_text = new LensText(client, 
		this,
		radius_slider,
		&client->config.radius, 
		x1, 
		y));
	radius_slider->text = radius_text;
	y += radius_text->get_h() + 5;


	add_tool(title = new BC_Title(x, y, _("Center X:")));
	y += title->get_h() + 5;
	add_tool(centerx_slider = new LensSlider(client, 
		this,
		0,
		&client->config.center_x, 
		x, 
		y, 
		0.0,
		99.0));
	x1 = x + centerx_slider->get_w() + 5;
	add_tool(centerx_text = new LensText(client, 
		this,
		centerx_slider,
		&client->config.center_x, 
		x1, 
		y));
	centerx_slider->text = centerx_text;
	centerx_slider->set_precision(1.0);
	y += centerx_text->get_h() + 5;


	add_tool(title = new BC_Title(x, y, _("Center Y:")));
	y += title->get_h() + 5;
	add_tool(centery_slider = new LensSlider(client, 
		this,
		0,
		&client->config.center_y, 
		x, 
		y, 
		0.0,
		99.0));
	x1 = x + centery_slider->get_w() + 5;
	add_tool(centery_text = new LensText(client, 
		this,
		centery_slider,
		&client->config.center_y, 
		x1, 
		y));
	centery_slider->text = centery_text;
	centery_slider->set_precision(1.0);
	y += centery_text->get_h() + 10;

	add_tool(bar = new BC_Bar(x, y, get_w() - x * 2));
	y += bar->get_h() + 5;


// 	add_tool(reverse = new LensToggle(client, 
// 		&client->config.reverse, 
// 		x, 
// 		y,
// 		_("Reverse")));
// 	y += reverse->get_h() + 5;

	add_tool(draw_guides = new LensToggle(client, 
		&client->config.draw_guides, 
		x, 
		y,
		_("Draw center")));
	y += draw_guides->get_h() + 5;

	
	add_tool(title = new BC_Title(x, y, _("Mode:")));
	add_tool(mode = new LensMode(client, 
		this, 
		x + title->get_w() + 5, 
		y));
	mode->create_objects();
	y += mode->get_h() + 5;


// 	add_tool(title = new BC_Title(x, y, _("Preset:")));
// 	add_tool(presets = new LensPresets(client, 
// 		this, 
// 		x + title->get_w() + 5, 
// 		y,
// 		get_w() - x - title->get_w() - 50));
// 	presets->create_objects();
// 	y += presets->get_h() + 5;
// 
// 	add_tool(save_preset = new LensSavePreset(client,
// 		this,
// 		x, 
// 		y));
// 	add_tool(preset_text = new LensPresetText(client,
// 		this,
// 		x + save_preset->get_w() + 5,
// 		y,
// 		get_w() - x - save_preset->get_w() - 10));
// 	y += preset_text->get_h() + 5;
// 	add_tool(delete_preset = new LensDeletePreset(client,
// 		this,
// 		x,
// 		y));

	show_window();
	flush();
}







LensMain::LensMain(PluginServer *server)
 : PluginVClient(server)
{
	
	engine = 0;
	lock = 0;
	current_preset = -1;
}

LensMain::~LensMain()
{
	
	delete engine;
	presets.remove_all_objects();
}

NEW_WINDOW_MACRO(LensMain, LensGUI)
#include "picon_png.h"
NEW_PICON_MACRO(LensMain)
LOAD_CONFIGURATION_MACRO(LensMain, LensConfig)
int LensMain::is_realtime() { return 1; }
const char* LensMain::plugin_title() { return N_("Lens"); }

void LensMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((LensGUI*)thread->window)->lock_window("LensMain::update_gui");
			for(int i = 0; i < FOV_CHANNELS; i++)
			{
				((LensGUI*)thread->window)->fov_slider[i]->update(config.fov[i]);
				((LensGUI*)thread->window)->fov_text[i]->update(config.fov[i]);
			}
			((LensGUI*)thread->window)->aspect_slider->update(config.aspect);
			((LensGUI*)thread->window)->aspect_text->update(config.aspect);
			((LensGUI*)thread->window)->radius_slider->update(config.radius);
			((LensGUI*)thread->window)->radius_text->update(config.radius);
			((LensGUI*)thread->window)->centerx_slider->update(config.center_x);
			((LensGUI*)thread->window)->centerx_text->update(config.center_x);
			((LensGUI*)thread->window)->centery_slider->update(config.center_y);
			((LensGUI*)thread->window)->centery_text->update(config.center_y);
			((LensGUI*)thread->window)->mode->update(config.mode);
			((LensGUI*)thread->window)->draw_guides->update(config.draw_guides);
			((LensGUI*)thread->window)->unlock_window();
		}
	}
}

void LensMain::save_presets()
{
	char path[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(path, "%slenspresets.rc", BCASTDIR);
	BC_Hash *defaults = new BC_Hash(path);

// Save presets
	defaults->update("TOTAL_PRESETS", presets.total);
	for(int i = 0; i < presets.total; i++)
	{
		LensPreset *preset = presets.values[i];
		sprintf(string, "TITLE_%d", i);
		defaults->update(string, preset->title);

		for(int j = 0; j < FOV_CHANNELS; j++)
		{
			sprintf(string, "FOCAL_LENGTH_%d_%d", i, j);
			defaults->update(string, preset->fov[j]);
		}

		sprintf(string, "ASPECT_%d", i);
		defaults->update(string, preset->aspect);
		sprintf(string, "RADIUS_%d", i);
		defaults->update(string, preset->radius);
		sprintf(string, "MODE_%d", i);
		defaults->update(string, preset->mode);
	}
	
	defaults->save();
	delete defaults;
}


void LensMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];



// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("LENS");
	for(int i = 0; i < FOV_CHANNELS; i++)
	{
		sprintf(string, "FOCAL_LENGTH%d", i);
		output.tag.set_property(string, config.fov[i]);
	}
	output.tag.set_property("ASPECT", config.aspect);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.tag.set_property("DRAW_GUIDES", config.draw_guides);
	output.append_tag();
	output.terminate_string();

}


void LensMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];


	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("LENS"))
			{
				for(int i = 0; i < FOV_CHANNELS; i++)
				{
					sprintf(string, "FOCAL_LENGTH%d", i);
					config.fov[i] = input.tag.get_property(string, config.fov[i]);
				}
				config.aspect = input.tag.get_property("ASPECT", config.aspect);
				config.radius = input.tag.get_property("RADIUS", config.radius);
				config.mode = input.tag.get_property("MODE", config.mode);
				config.center_x = input.tag.get_property("CENTER_X", config.center_x);
				config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
				config.draw_guides = input.tag.get_property("DRAW_GUIDES", config.draw_guides);
			}
		}
	}
}



int LensMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	VFrame *input;
	load_configuration();
	
	if(get_use_opengl())
	{
		input = frame;
	}
	else
	{
		input = new_temp(frame->get_w(), frame->get_h(), frame->get_color_model());
	}
	
	read_frame(input, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());


	if(get_use_opengl())
	{
		run_opengl();
		return 0;
	}
	else
	{
		if(!engine) engine = new LensEngine(this);
		engine->process_packages();
		if(config.draw_guides)
		{
// Draw center
#define CENTER_H 20
#define CENTER_W 20
#define DRAW_GUIDES(components, type, max) \
{ \
	type **rows = (type**)get_output()->get_rows(); \
	if(center_x >= 0 && center_x < w || \
		center_y >= 0 && center_y < h) \
	{ \
		type *hrow = rows[center_y] + components * (center_x - CENTER_W / 2); \
		for(int i = center_x - CENTER_W / 2; i <= center_x + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < w) \
			{ \
				hrow[0] = max - hrow[0]; \
				hrow[1] = max - hrow[1]; \
				hrow[2] = max - hrow[2]; \
				hrow += components; \
			} \
		} \
 \
		for(int i = center_y - CENTER_W / 2; i <= center_y + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < h) \
			{ \
				type *vrow = rows[i] + center_x * components; \
				vrow[0] = max - vrow[0]; \
				vrow[1] = max - vrow[1]; \
				vrow[2] = max - vrow[2]; \
			} \
		} \
	} \
}

			int w = get_output()->get_w();
			int h = get_output()->get_h();
			int center_x = (int)(config.center_x * w / 100);
			int center_y = (int)(config.center_y * h / 100);
			switch(get_output()->get_color_model())
			{
				case BC_RGB_FLOAT:
					DRAW_GUIDES(3, float, 1.0)
					break;
				case BC_RGBA_FLOAT:
					DRAW_GUIDES(4, float, 1.0)
					break;
				case BC_RGB888:
					DRAW_GUIDES(3, unsigned char, 0xff)
					break;
				case BC_RGBA8888:
					DRAW_GUIDES(4, unsigned char, 0xff)
					break;
				case BC_YUV888:
					DRAW_GUIDES(3, unsigned char, 0xff)
					break;
				case BC_YUVA8888:
					DRAW_GUIDES(4, unsigned char, 0xff)
					break;
			}

		}
	}

	return 0;
}


int LensMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *shrink_frag = 
		"uniform sampler2D tex;\n"
		"uniform vec2 texture_extents;\n"
		"uniform vec2 image_extents;\n"
		"uniform vec2 aspect;\n"
		"uniform vec2 center_coord;\n"
		"uniform vec4 border_color;\n"
		"uniform vec4 r;\n"
		"uniform vec4 max_z;\n"
		"void main()\n"
		"{\n"
		"	vec2 outcoord = gl_TexCoord[0].st * texture_extents;\n"
		"	vec2 coord_diff = outcoord - center_coord;\n"
		"	if(coord_diff.x == 0.0 && coord_diff.y == 0.0)\n"
		"	{\n"
		"		gl_FragColor = texture2D(tex, outcoord);\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		float z = sqrt(coord_diff.x * coord_diff.x +\n"
		"						coord_diff.y * coord_diff.y);\n"
		"		float a2 = atan(coord_diff.y, coord_diff.x);\n"
		"		vec4 a1 = asin(vec4(z, z, z, z) / r);\n"
		"		vec4 z_in = a1 * max_z * 2.0 / 3.14159;\n"
		"		vec4 in_x;\n"
		"		vec4 in_y;\n"
		"		in_x = z_in * cos(a2) * aspect.x + center_coord.x;\n"
		"		in_y = z_in * sin(a2) * aspect.y + center_coord.y;\n"
		"		if(z > r.r || in_x.r < 0.0 || in_x.r >= image_extents.x || in_y.r < 0.0 || in_y.r >= image_extents.y)\n"
		"			gl_FragColor.r = border_color.r;\n"
		"		else\n"
		"			gl_FragColor.r = texture2D(tex, vec2(in_x.r, in_y.r) / texture_extents).r;\n"
		"		if(z > r.g || in_x.g < 0.0 || in_x.g >= image_extents.x || in_y.g < 0.0 || in_y.g >= image_extents.y)\n"
		"			gl_FragColor.g = border_color.g;\n"
		"		else\n"
		"			gl_FragColor.g = texture2D(tex, vec2(in_x.g, in_y.g) / texture_extents).g;\n"
		"		if(z > r.b || in_x.b < 0.0 || in_x.b >= image_extents.x || in_y.b < 0.0 || in_y.b >= image_extents.y)\n"
		"			gl_FragColor.b = border_color.b;\n"
		"		else\n"
		"			gl_FragColor.b = texture2D(tex, vec2(in_x.b, in_y.b) / texture_extents).b;\n"
		"		if(z > r.a || in_x.a < 0.0 || in_x.a >= image_extents.x || in_y.a < 0.0 || in_y.a >= image_extents.y)\n"
		"			gl_FragColor.a = border_color.a;\n"
		"		else\n"
		"			gl_FragColor.a = texture2D(tex, vec2(in_x.a, in_y.a) / texture_extents).a;\n"
		"	}\n"
		"}\n";

	static const char *stretch_frag = 
		"uniform sampler2D tex;\n"
		"uniform vec2 texture_extents;\n"
		"uniform vec2 image_extents;\n"
		"uniform vec2 aspect;\n"
		"uniform vec2 center_coord;\n"
		"uniform vec4 border_color;\n"
		"uniform vec4 r;\n"
		"void main()\n"
		"{\n"
		"	vec2 outcoord = gl_TexCoord[0].st * texture_extents;\n"
		"	vec2 coord_diff = outcoord - center_coord;\n"
		"	float z = sqrt(coord_diff.x * coord_diff.x +\n"
		"					coord_diff.y * coord_diff.y);\n"
		"	vec4 a1 = (vec4(z, z, z, z) / (3.14159 * r / 2.0)) * (3.14159 / 2.0);\n"
		"	vec4 z_in = r * sin(a1);\n"
		"	float a2;\n"
		"	if(coord_diff.x == 0.0)\n"
		"	{\n"
		"		if(coord_diff.y < 0.0)\n"
		"			a2 = 3.0 * 3.14159 / 2.0;\n"
		"		else\n"
		"			a2 = 3.14159 / 2.0;\n"
		"	}\n"
		"	else\n"
		"		a2 = atan(coord_diff.y, coord_diff.x);\n"
		"	vec4 in_x;\n"
		"	vec4 in_y;\n"
		"	in_x = z_in * cos(a2) * aspect.x + center_coord.x;\n"
		"	in_y = z_in * sin(a2) * aspect.y + center_coord.y;\n"
		"	if(in_x.r < 0.0 || in_x.r >= image_extents.x || in_y.r < 0.0 || in_y.r >= image_extents.y)\n"
		"		gl_FragColor.r = border_color.r;\n"
		"	else\n"
		"		gl_FragColor.r = texture2D(tex, vec2(in_x.r, in_y.r) / texture_extents).r;\n"
		"	if(in_x.g < 0.0 || in_x.g >= image_extents.x || in_y.g < 0.0 || in_y.g >= image_extents.y)\n"
		"		gl_FragColor.g = border_color.g;\n"
		"	else\n"
		"		gl_FragColor.g = texture2D(tex, vec2(in_x.g, in_y.g) / texture_extents).g;\n"
		"	if(in_x.b < 0.0 || in_x.b >= image_extents.x || in_y.b < 0.0 || in_y.b >= image_extents.y)\n"
		"		gl_FragColor.b = border_color.b;\n"
		"	else\n"
		"		gl_FragColor.b = texture2D(tex, vec2(in_x.b, in_y.b) / texture_extents).b;\n"
		"	if(in_x.a < 0.0 || in_x.a >= image_extents.x || in_y.a < 0.0 || in_y.a >= image_extents.y)\n"
		"		gl_FragColor.a = border_color.a;\n"
		"	else\n"
		"		gl_FragColor.a = texture2D(tex, vec2(in_x.a, in_y.a) / texture_extents).a;\n"
		"}\n";


	static const char *rectilinear_stretch_frag = 
		"uniform sampler2D tex;\n"
		"uniform vec2 texture_extents;\n"
		"uniform vec2 image_extents;\n"
		"uniform vec2 aspect;\n"
		"uniform vec2 center_coord;\n"
		"uniform vec4 border_color;\n"
		"uniform vec4 r;\n"
		"uniform float radius;\n"
		"void main()\n"
		"{\n"
		"	vec2 outcoord = gl_TexCoord[0].st * texture_extents;\n"
		"	vec2 coord_diff = outcoord - center_coord;\n"
		"	float z = sqrt(coord_diff.x * coord_diff.x +\n"
		"					coord_diff.y * coord_diff.y);\n"
		"	vec4 radius1 = (vec4(z, z, z, z) / r) * 2.0 * radius;\n"
		"	vec4 z_in = r * atan(radius1) / (3.14159 / 2.0);\n"
		"\n"
		"	float angle;\n"
		"	if(coord_diff.x == 0.0)\n"
		"	{\n"
		"		if(coord_diff.y < 0.0)\n"
		"			angle = 3.0 * 3.14159 / 2.0;\n"
		"		else\n"
		"			angle = 3.14159 / 2.0;\n"
		"	}\n"
		"	else\n"
		"		angle = atan(coord_diff.y, coord_diff.x);\n"
		"	vec4 in_x;\n"
		"	vec4 in_y;\n"
		"\n"
		"	in_x = z_in * cos(angle) * aspect.x + center_coord.x;\n"
		"	in_y = z_in * sin(angle) * aspect.y + center_coord.y;\n"
		"	if(in_x.r < 0.0 || in_x.r >= image_extents.x || in_y.r < 0.0 || in_y.r >= image_extents.y)\n"
		"		gl_FragColor.r = border_color.r;\n"
		"	else\n"
		"		gl_FragColor.r = texture2D(tex, vec2(in_x.r, in_y.r) / texture_extents).r;\n"
		"	if(in_x.g < 0.0 || in_x.g >= image_extents.x || in_y.g < 0.0 || in_y.g >= image_extents.y)\n"
		"		gl_FragColor.g = border_color.g;\n"
		"	else\n"
		"		gl_FragColor.g = texture2D(tex, vec2(in_x.g, in_y.g) / texture_extents).g;\n"
		"	if(in_x.b < 0.0 || in_x.b >= image_extents.x || in_y.b < 0.0 || in_y.b >= image_extents.y)\n"
		"		gl_FragColor.b = border_color.b;\n"
		"	else\n"
		"		gl_FragColor.b = texture2D(tex, vec2(in_x.b, in_y.b) / texture_extents).b;\n"
		"	if(in_x.a < 0.0 || in_x.a >= image_extents.x || in_y.a < 0.0 || in_y.a >= image_extents.y)\n"
		"		gl_FragColor.a = border_color.a;\n"
		"	else\n"
		"		gl_FragColor.a = texture2D(tex, vec2(in_x.a, in_y.a) / texture_extents).a;\n"
		"}\n";

	static const char *rectilinear_shrink_frag = 
		"uniform sampler2D tex;\n"
		"uniform vec2 texture_extents;\n"
		"uniform vec2 image_extents;\n"
		"uniform vec2 aspect;\n"
		"uniform vec2 center_coord;\n"
		"uniform vec4 border_color;\n"
		"uniform vec4 r;\n"
		"uniform float radius;\n"
		"void main()\n"
		"{\n"
		"	vec2 outcoord = gl_TexCoord[0].st * texture_extents;\n"
		"	vec2 coord_diff = outcoord - center_coord;\n"
		"	float z = sqrt(coord_diff.x * coord_diff.x +\n"
		"					coord_diff.y * coord_diff.y);\n"
		"	vec4 radius1 = (vec4(z, z, z, z) / r) * 2.0 * radius;\n"
		"	vec4 z_in = r * atan(radius1) / (3.14159 / 2.0);\n"
		"\n"
		"	float angle;\n"
		"	if(coord_diff.x == 0.0)\n"
		"	{\n"
		"		if(coord_diff.y < 0.0)\n"
		"			angle = 3.0 * 3.14159 / 2.0;\n"
		"		else\n"
		"			angle = 3.14159 / 2.0;\n"
		"	}\n"
		"	else\n"
		"		angle = atan(coord_diff.y, coord_diff.x);\n"
		"	vec4 in_x;\n"
		"	vec4 in_y;\n"
		"\n"
		"	in_x = z_in * cos(angle) * aspect.x + center_coord.x;\n"
		"	in_y = z_in * sin(angle) * aspect.y + center_coord.y;\n"
		"	if(in_x.r < 0.0 || in_x.r >= image_extents.x || in_y.r < 0.0 || in_y.r >= image_extents.y)\n"
		"		gl_FragColor.r = border_color.r;\n"
		"	else\n"
		"		gl_FragColor.r = texture2D(tex, vec2(in_x.r, in_y.r) / texture_extents).r;\n"
		"	if(in_x.g < 0.0 || in_x.g >= image_extents.x || in_y.g < 0.0 || in_y.g >= image_extents.y)\n"
		"		gl_FragColor.g = border_color.g;\n"
		"	else\n"
		"		gl_FragColor.g = texture2D(tex, vec2(in_x.g, in_y.g) / texture_extents).g;\n"
		"	if(in_x.b < 0.0 || in_x.b >= image_extents.x || in_y.b < 0.0 || in_y.b >= image_extents.y)\n"
		"		gl_FragColor.b = border_color.b;\n"
		"	else\n"
		"		gl_FragColor.b = texture2D(tex, vec2(in_x.b, in_y.b) / texture_extents).b;\n"
		"	if(in_x.a < 0.0 || in_x.a >= image_extents.x || in_y.a < 0.0 || in_y.a >= image_extents.y)\n"
		"		gl_FragColor.a = border_color.a;\n"
		"	else\n"
		"		gl_FragColor.a = texture2D(tex, vec2(in_x.a, in_y.a) / texture_extents).a;\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();
	unsigned int frag_shader;
	switch(config.mode)
	{
		case LensConfig::SHRINK:
			frag_shader = VFrame::make_shader(0,
				shrink_frag,
				0);
			break;
		case LensConfig::STRETCH:
			frag_shader = VFrame::make_shader(0,
				stretch_frag,
				0);
			break;
		case LensConfig::RECTILINEAR_STRETCH:
			frag_shader = VFrame::make_shader(0,
				rectilinear_stretch_frag,
				0);
			break;
		case LensConfig::RECTILINEAR_SHRINK:
			frag_shader = VFrame::make_shader(0,
				rectilinear_shrink_frag,
				0);
			break;
	}



	if(frag_shader > 0)
	{
		float border_color[] = { 0, 0, 0, 0 };
		if(BC_CModels::is_yuv(get_output()->get_color_model()))
		{
			border_color[1] = 0.5;
			border_color[2] = 0.5;
		}

		double x_factor = config.aspect;
		double y_factor = 1.0 / config.aspect;
		if(x_factor < 1) x_factor = 1;
		if(y_factor < 1) y_factor = 1;

		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
		glUniform2f(glGetUniformLocation(frag_shader, "aspect"), 
			x_factor, 
			y_factor);
		glUniform2f(glGetUniformLocation(frag_shader, "center_coord"), 
				(GLfloat)get_input()->get_w() * config.center_x / 100.0,
				(GLfloat)get_input()->get_h() * config.center_y / 100.0);
		glUniform2f(glGetUniformLocation(frag_shader, "texture_extents"), 
				(GLfloat)get_input()->get_texture_w(),
				(GLfloat)get_input()->get_texture_h());
		glUniform2f(glGetUniformLocation(frag_shader, "image_extents"), 
				(GLfloat)get_input()->get_w(),
				(GLfloat)get_input()->get_h());

		int width = get_output()->get_w();
		int height = get_output()->get_h();
		float *fov = config.fov;
		float dim;
		float max_z;
		switch(config.mode)
		{
			case LensConfig::SHRINK:
				dim = MAX(width, height) * config.radius;
				max_z = dim * sqrt(2.0) / 2;
				glUniform4fv(glGetUniformLocation(frag_shader, "border_color"), 
						1,
						(GLfloat*)border_color);
				glUniform4f(glGetUniformLocation(frag_shader, "max_z"), 
					max_z / fov[0],
					max_z / fov[1],
					max_z / fov[2],
					max_z / fov[3]);
				glUniform4f(glGetUniformLocation(frag_shader, "r"), 
					(max_z / fov[0]) * 2 / M_PI,
					(max_z / fov[1]) * 2 / M_PI,
					(max_z / fov[2]) * 2 / M_PI,
					(max_z / fov[3]) * 2 / M_PI);
				break;

			case LensConfig::STRETCH:
				dim = MAX(width, height) * config.radius;
				max_z = dim * sqrt(2.0) / 2;
				glUniform4f(glGetUniformLocation(frag_shader, "r"), 
					max_z / M_PI / (fov[0] / 2.0),
					max_z / M_PI / (fov[1] / 2.0),
					max_z / M_PI / (fov[2] / 2.0),
					max_z / M_PI / (fov[3] / 2.0));
				break;

			case LensConfig::RECTILINEAR_STRETCH:
				max_z = sqrt(SQR(width) + SQR(height)) / 2;
				glUniform4f(glGetUniformLocation(frag_shader, "r"), 
					max_z / M_PI / (fov[0] / 2.0),
					max_z / M_PI / (fov[1] / 2.0),
					max_z / M_PI / (fov[2] / 2.0),
					max_z / M_PI / (fov[3] / 2.0));
				glUniform1f(glGetUniformLocation(frag_shader, "radius"), 
					config.radius);
				break;

			case LensConfig::RECTILINEAR_SHRINK:
				max_z = sqrt(SQR(width) + SQR(height)) / 2;
				glUniform4f(glGetUniformLocation(frag_shader, "r"), 
					max_z / M_PI / (fov[0] / 2.0),
					max_z / M_PI / (fov[1] / 2.0),
					max_z / M_PI / (fov[2] / 2.0),
					max_z / M_PI / (fov[3] / 2.0));
				glUniform1f(glGetUniformLocation(frag_shader, "radius"), 
					config.radius);
				break;
		}


		get_output()->init_screen();
		get_output()->bind_texture(0);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		get_output()->draw_texture();
		glUseProgram(0);


		if(config.draw_guides)
		{
			int w = get_output()->get_w();
			int h = get_output()->get_h();
			int center_x = (int)(config.center_x * w / 100);
			int center_y = (int)(config.center_y * h / 100);

			glDisable(GL_TEXTURE_2D);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glBegin(GL_LINES);
			glVertex3f(center_x, -h + center_y - CENTER_H / 2, 0.0);
			glVertex3f(center_x, -h + center_y + CENTER_H / 2, 0.0);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(center_x - CENTER_W / 2, -h + center_y, 0.0);
			glVertex3f(center_x + CENTER_W / 2, -h + center_y, 0.0);
			glEnd();
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glBegin(GL_LINES);
			glVertex3f(center_x - 1, -h + center_y - CENTER_H / 2 - 1, 0.0);
			glVertex3f(center_x - 1, -h + center_y + CENTER_H / 2 - 1, 0.0);
			glEnd();
			glBegin(GL_LINES);
			glVertex3f(center_x - CENTER_W / 2 - 1, -h + center_y - 1, 0.0);
			glVertex3f(center_x + CENTER_W / 2 - 1, -h + center_y - 1, 0.0);
			glEnd();
		}
		get_output()->set_opengl_state(VFrame::SCREEN);
	}
	
#endif
	return 0;
}








LensPackage::LensPackage()
 : LoadPackage() {}





LensUnit::LensUnit(LensEngine *engine, LensMain *plugin)
 : LoadClient(engine)
{
	this->plugin = plugin;
}

LensUnit::~LensUnit()
{
}

void LensUnit::process_shrink(LensPackage *pkg)
{

	float *fov = plugin->config.fov;
	float aspect = plugin->config.aspect;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int width = plugin->get_input()->get_w();
	int height = plugin->get_input()->get_h();
	double x_factor = aspect;
	double y_factor = 1.0 / aspect;
	if(x_factor < 1) x_factor = 1;
	if(y_factor < 1) y_factor = 1;
	double dim = MAX(width, height) * plugin->config.radius;
	double max_z[FOV_CHANNELS];
	double center_x = width * plugin->config.center_x / 100.0;
	double center_y = height * plugin->config.center_y / 100.0;
	double r[FOV_CHANNELS];

//	max_z[0] = sqrt(SQR(width) + SQR(height)) / 2 / fov[0];
//	max_z[1] = sqrt(SQR(width) + SQR(height)) / 2 / fov[1];
//	max_z[2] = sqrt(SQR(width) + SQR(height)) / 2 / fov[2];
//	max_z[3] = sqrt(SQR(width) + SQR(height)) / 2 / fov[3];
 	max_z[0] = dim * sqrt(2.0) / 2 / fov[0];
 	max_z[1] = dim * sqrt(2.0) / 2 / fov[1];
 	max_z[2] = dim * sqrt(2.0) / 2 / fov[2];
 	max_z[3] = dim * sqrt(2.0) / 2 / fov[3];
	r[0] = max_z[0] * 2 / M_PI;
	r[1] = max_z[1] * 2 / M_PI;
	r[2] = max_z[2] * 2 / M_PI;
	r[3] = max_z[3] * 2 / M_PI;

#define DO_LENS_SHRINK(type, components, chroma) \
{ \
	type **in_rows = (type**)plugin->get_temp()->get_rows(); \
	type **out_rows = (type**)plugin->get_input()->get_rows(); \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
		type *in_row = in_rows[y]; \
		double y_diff = y - center_y; \
 \
		for(int x = 0; x < width; x++) \
		{ \
			double x_diff = x - center_x; \
			if(!x_diff && !y_diff) \
			{ \
				type *in_pixel = in_row + x * components; \
				for(int c = 0; c < components; c++) \
				{ \
					*out_row++ = *in_pixel++; \
				} \
				continue; \
			} \
 \
			double z = sqrt(x_diff * x_diff + y_diff * y_diff); \
			double a2 = atan(y_diff / x_diff); \
			if(x_diff < 0.0) a2 += M_PI; \
 \
 			for(int i = 0; i < components; i++) \
			{ \
				if(z > r[i]) \
				{ \
					*out_row++ = black[i]; \
				} \
				else \
				{ \
					double a1 = asin(z / r[i]); \
					double z_in = a1 * max_z[i] * 2 / M_PI; \
 \
					float x_in = z_in * cos(a2) * x_factor + center_x; \
					float y_in = z_in * sin(a2) * y_factor + center_y; \
 \
 					if(x_in < 0.0 || x_in >= width - 1 || \
						y_in < 0.0 || y_in >= height - 1) \
					{ \
						*out_row++ = black[i]; \
					} \
					else \
					{ \
						float y1_fraction = y_in - floor(y_in); \
						float y2_fraction = 1.0 - y1_fraction; \
						float x1_fraction = x_in - floor(x_in); \
						float x2_fraction = 1.0 - x1_fraction; \
						type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
						type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
						*out_row++ = (type)(in_pixel1[i] * x2_fraction * y2_fraction + \
									in_pixel2[i] * x2_fraction * y1_fraction + \
									in_pixel1[i + components] * x1_fraction * y2_fraction + \
									in_pixel2[i + components] * x1_fraction * y1_fraction); \
					} \
				} \
			} \
		} \
	} \
 \
 	type *out_pixel = out_rows[(int)center_y] + (int)center_x * components; \
 	type *in_pixel = in_rows[(int)center_y] + (int)center_x * components; \
	for(int c = 0; c < components; c++) \
	{ \
		*out_pixel++ = *in_pixel++; \
	} \
}


		switch(plugin->get_input()->get_color_model())
		{
			case BC_RGB888:
				DO_LENS_SHRINK(unsigned char, 3, 0x0);
				break;
			case BC_RGBA8888:
				DO_LENS_SHRINK(unsigned char, 4, 0x0);
				break;
			case BC_RGB_FLOAT:
				DO_LENS_SHRINK(float, 3, 0.0);
				break;
			case BC_RGBA_FLOAT:
				DO_LENS_SHRINK(float, 4, 0.0);
				break;
			case BC_YUV888:
				DO_LENS_SHRINK(unsigned char, 3, 0x80);
				break;
		case BC_YUVA8888:
				DO_LENS_SHRINK(unsigned char, 4, 0x80);
				break;
		}
}

void LensUnit::process_stretch(LensPackage *pkg)
{
	float *fov = plugin->config.fov;
	float aspect = plugin->config.aspect;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	double x_factor = aspect;
	double y_factor = 1.0 / aspect;
	if(x_factor < 1) x_factor = 1;
	if(y_factor < 1) y_factor = 1;
	int width = plugin->get_input()->get_w();
	int height = plugin->get_input()->get_h();
	double dim = MAX(width, height) * plugin->config.radius;
	double max_z = dim * sqrt(2.0) / 2;
	double center_x = width * plugin->config.center_x / 100.0;
	double center_y = height * plugin->config.center_y / 100.0;
	double r[FOV_CHANNELS];

	r[0] = max_z / M_PI / (fov[0] / 2.0);
	r[1] = max_z / M_PI / (fov[1] / 2.0);
	r[2] = max_z / M_PI / (fov[2] / 2.0);
	r[3] = max_z / M_PI / (fov[3] / 2.0);

#define DO_LENS_STRETCH(type, components, chroma) \
{ \
	type **in_rows = (type**)plugin->get_temp()->get_rows(); \
	type **out_rows = (type**)plugin->get_input()->get_rows(); \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
		type *in_row = in_rows[y]; \
		double y_diff = y - center_y; \
 \
		for(int x = 0; x < width; x++) \
		{ \
			double x_diff = (x - center_x); \
			double z = sqrt(x_diff * x_diff + \
				y_diff * y_diff); \
			double a2; \
			if(x == center_x) \
			{ \
				if(y < center_y) \
					a2 = 3 * M_PI / 2; \
				else \
					a2 = M_PI / 2; \
			} \
			else \
			{ \
				a2 = atan(y_diff / x_diff); \
			} \
			if(x_diff < 0.0) a2 += M_PI; \
 \
			for(int i = 0; i < components; i++) \
			{ \
				double a1 = (z / (M_PI * r[i] / 2)) * (M_PI / 2); \
				double z_in = r[i] * sin(a1); \
 \
				double x_in = z_in * cos(a2) * x_factor + center_x; \
				double y_in = z_in * sin(a2) * y_factor + center_y; \
 \
 				if(x_in < 0.0 || x_in >= width - 1 || \
					y_in < 0.0 || y_in >= height - 1) \
				{ \
					*out_row++ = black[i]; \
				} \
				else \
				{ \
					float y1_fraction = y_in - floor(y_in); \
					float y2_fraction = 1.0 - y1_fraction; \
					float x1_fraction = x_in - floor(x_in); \
					float x2_fraction = 1.0 - x1_fraction; \
					type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
					type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
					*out_row++ = (type)(in_pixel1[i] * x2_fraction * y2_fraction + \
								in_pixel2[i] * x2_fraction * y1_fraction + \
								in_pixel1[i + components] * x1_fraction * y2_fraction + \
								in_pixel2[i + components] * x1_fraction * y1_fraction); \
				} \
			} \
		} \
	} \
 \
 	type *out_pixel = out_rows[(int)center_y] + (int)center_x * components; \
 	type *in_pixel = in_rows[(int)center_y] + (int)center_x * components; \
	for(int c = 0; c < components; c++) \
	{ \
		*out_pixel++ = *in_pixel++; \
	} \
}


		switch(plugin->get_input()->get_color_model())
		{
			case BC_RGB888:
				DO_LENS_STRETCH(unsigned char, 3, 0x0);
				break;
			case BC_RGBA8888:
				DO_LENS_STRETCH(unsigned char, 4, 0x0);
				break;
			case BC_RGB_FLOAT:
				DO_LENS_STRETCH(float, 3, 0.0);
				break;
			case BC_RGBA_FLOAT:
				DO_LENS_STRETCH(float, 4, 0.0);
				break;
			case BC_YUV888:
				DO_LENS_STRETCH(unsigned char, 3, 0x80);
				break;
			case BC_YUVA8888:
				DO_LENS_STRETCH(unsigned char, 4, 0x80);
				break;
		}
}

void LensUnit::process_rectilinear_stretch(LensPackage *pkg)
{
	float *fov = plugin->config.fov;
	float aspect = plugin->config.aspect;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	double x_factor = aspect;
	double y_factor = 1.0 / aspect;
	if(x_factor < 1) x_factor = 1;
	if(y_factor < 1) y_factor = 1;
	int width = plugin->get_input()->get_w();
	int height = plugin->get_input()->get_h();
//	double dim = MAX(width, height) * plugin->config.radius;
//	double max_z = dim * sqrt(2.0) / 2;
	double max_z = sqrt(SQR(width) + SQR(height)) / 2;
	double center_x = width * plugin->config.center_x / 100.0;
	double center_y = height * plugin->config.center_y / 100.0;
	double r[FOV_CHANNELS];

	r[0] = max_z / M_PI / (fov[0] / 2.0);
	r[1] = max_z / M_PI / (fov[1] / 2.0);
	r[2] = max_z / M_PI / (fov[2] / 2.0);
	r[3] = max_z / M_PI / (fov[3] / 2.0);

#define DO_LENS_RECTILINEAR_STRETCH(type, components, chroma) \
{ \
	type **in_rows = (type**)plugin->get_temp()->get_rows(); \
	type **out_rows = (type**)plugin->get_input()->get_rows(); \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
		type *in_row = in_rows[y]; \
		double y_diff = y - center_y; \
 \
		for(int x = 0; x < width; x++) \
		{ \
			double x_diff = (x - center_x); \
/* Compute magnitude */ \
			double z = sqrt(x_diff * x_diff + \
				y_diff * y_diff); \
/* Compute angle */ \
			double angle; \
			if(x == center_x) \
			{ \
				if(y < center_y) \
					angle = 3 * M_PI / 2; \
				else \
					angle = M_PI / 2; \
			} \
			else \
			{ \
				angle = atan(y_diff / x_diff); \
			} \
			if(x_diff < 0.0) angle += M_PI; \
 \
			for(int i = 0; i < components; i++) \
			{ \
/* Compute new radius */ \
				double radius1 = (z / r[i]) * 2 * plugin->config.radius; \
				double z_in = r[i] * atan(radius1) / (M_PI / 2); \
 \
				double x_in = z_in * cos(angle) * x_factor + center_x; \
				double y_in = z_in * sin(angle) * y_factor + center_y; \
 \
 				if(x_in < 0.0 || x_in >= width - 1 || \
					y_in < 0.0 || y_in >= height - 1) \
				{ \
					*out_row++ = black[i]; \
				} \
				else \
				{ \
					float y1_fraction = y_in - floor(y_in); \
					float y2_fraction = 1.0 - y1_fraction; \
					float x1_fraction = x_in - floor(x_in); \
					float x2_fraction = 1.0 - x1_fraction; \
					type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
					type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
					*out_row++ = (type)(in_pixel1[i] * x2_fraction * y2_fraction + \
								in_pixel2[i] * x2_fraction * y1_fraction + \
								in_pixel1[i + components] * x1_fraction * y2_fraction + \
								in_pixel2[i + components] * x1_fraction * y1_fraction); \
				} \
			} \
		} \
	} \
 \
 	type *out_pixel = out_rows[(int)center_y] + (int)center_x * components; \
 	type *in_pixel = in_rows[(int)center_y] + (int)center_x * components; \
	for(int c = 0; c < components; c++) \
	{ \
		*out_pixel++ = *in_pixel++; \
	} \
}


		switch(plugin->get_input()->get_color_model())
		{
			case BC_RGB888:
				DO_LENS_RECTILINEAR_STRETCH(unsigned char, 3, 0x0);
				break;
			case BC_RGBA8888:
				DO_LENS_RECTILINEAR_STRETCH(unsigned char, 4, 0x0);
				break;
			case BC_RGB_FLOAT:
				DO_LENS_RECTILINEAR_STRETCH(float, 3, 0.0);
				break;
			case BC_RGBA_FLOAT:
				DO_LENS_RECTILINEAR_STRETCH(float, 4, 0.0);
				break;
			case BC_YUV888:
				DO_LENS_RECTILINEAR_STRETCH(unsigned char, 3, 0x80);
				break;
			case BC_YUVA8888:
				DO_LENS_RECTILINEAR_STRETCH(unsigned char, 4, 0x80);
				break;
		}
}

void LensUnit::process_rectilinear_shrink(LensPackage *pkg)
{
	float *fov = plugin->config.fov;
	float aspect = plugin->config.aspect;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	double x_factor = aspect;
	double y_factor = 1.0 / aspect;
	if(x_factor < 1) x_factor = 1;
	if(y_factor < 1) y_factor = 1;
	int width = plugin->get_input()->get_w();
	int height = plugin->get_input()->get_h();
	double max_z = MAX(width, height) / 2 * plugin->config.radius;
	double center_x = width * plugin->config.center_x / 100.0;
	double center_y = height * plugin->config.center_y / 100.0;
	double r[FOV_CHANNELS];

	r[0] = max_z / fov[0];
	r[1] = max_z / fov[1];
	r[2] = max_z / fov[2];
	r[3] = max_z / fov[3];

#define DO_LENS_RECTILINEAR_SHRINK(type, components, chroma) \
{ \
	type **in_rows = (type**)plugin->get_temp()->get_rows(); \
	type **out_rows = (type**)plugin->get_input()->get_rows(); \
	type black[4] = { 0, chroma, chroma, 0 }; \
 \
	for(int y = row1; y < row2; y++) \
	{ \
		type *out_row = out_rows[y]; \
		type *in_row = in_rows[y]; \
		double y_diff = y - center_y; \
 \
		for(int x = 0; x < width; x++) \
		{ \
			double x_diff = (x - center_x); \
/* Compute magnitude */ \
			double z = sqrt(x_diff * x_diff + \
				y_diff * y_diff); \
/* Compute angle */ \
			double angle; \
			if(x == center_x) \
			{ \
				if(y < center_y) \
					angle = 3 * M_PI / 2; \
				else \
					angle = M_PI / 2; \
			} \
			else \
			{ \
				angle = atan(y_diff / x_diff); \
			} \
			if(x_diff < 0.0) angle += M_PI; \
 \
			for(int i = 0; i < components; i++) \
			{ \
/* Compute new radius */ \
				double radius1 = z / r[i]; \
				double z_in = r[i] * tan(radius1) / (M_PI / 2); \
 \
				double x_in = z_in * cos(angle) * x_factor + center_x; \
				double y_in = z_in * sin(angle) * y_factor + center_y; \
 \
 				if(x_in < 0.0 || x_in >= width - 1 || \
					y_in < 0.0 || y_in >= height - 1) \
				{ \
					*out_row++ = black[i]; \
				} \
				else \
				{ \
					float y1_fraction = y_in - floor(y_in); \
					float y2_fraction = 1.0 - y1_fraction; \
					float x1_fraction = x_in - floor(x_in); \
					float x2_fraction = 1.0 - x1_fraction; \
					type *in_pixel1 = in_rows[(int)y_in] + (int)x_in * components; \
					type *in_pixel2 = in_rows[(int)y_in + 1] + (int)x_in * components; \
					*out_row++ = (type)(in_pixel1[i] * x2_fraction * y2_fraction + \
								in_pixel2[i] * x2_fraction * y1_fraction + \
								in_pixel1[i + components] * x1_fraction * y2_fraction + \
								in_pixel2[i + components] * x1_fraction * y1_fraction); \
				} \
			} \
		} \
	} \
 \
 	type *out_pixel = out_rows[(int)center_y] + (int)center_x * components; \
 	type *in_pixel = in_rows[(int)center_y] + (int)center_x * components; \
	for(int c = 0; c < components; c++) \
	{ \
		*out_pixel++ = *in_pixel++; \
	} \
}


		switch(plugin->get_input()->get_color_model())
		{
			case BC_RGB888:
				DO_LENS_RECTILINEAR_SHRINK(unsigned char, 3, 0x0);
				break;
			case BC_RGBA8888:
				DO_LENS_RECTILINEAR_SHRINK(unsigned char, 4, 0x0);
				break;
			case BC_RGB_FLOAT:
				DO_LENS_RECTILINEAR_SHRINK(float, 3, 0.0);
				break;
			case BC_RGBA_FLOAT:
				DO_LENS_RECTILINEAR_SHRINK(float, 4, 0.0);
				break;
			case BC_YUV888:
				DO_LENS_RECTILINEAR_SHRINK(unsigned char, 3, 0x80);
				break;
			case BC_YUVA8888:
				DO_LENS_RECTILINEAR_SHRINK(unsigned char, 4, 0x80);
				break;
		}
}

void LensUnit::process_package(LoadPackage *package)
{
	LensPackage *pkg = (LensPackage*)package;

	switch(plugin->config.mode)
	{
		case LensConfig::SHRINK:
			process_shrink(pkg);
			break;
		case LensConfig::STRETCH:
			process_stretch(pkg);
			break;
		case LensConfig::RECTILINEAR_STRETCH:
			process_rectilinear_stretch(pkg);
			break;
		case LensConfig::RECTILINEAR_SHRINK	:
			process_rectilinear_shrink(pkg);
			break;
	}
}





LensEngine::LensEngine(LensMain *plugin)
 : LoadServer(plugin->PluginClient::smp + 1, plugin->PluginClient::smp + 1)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

LensEngine::~LensEngine()
{
}

void LensEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		LensPackage *package = (LensPackage*)LoadServer::get_package(i);
		package->row1 = plugin->get_input()->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->get_input()->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* LensEngine::new_client()
{
	return new LensUnit(this, plugin);
}

LoadPackage* LensEngine::new_package()
{
	return new LensPackage;
}


