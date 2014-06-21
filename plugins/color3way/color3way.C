
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



#include "filexml.h"
#include "color3way.h"
#include "bchash.h"
#include "language.h"
#include "playback3d.h"

#include "aggregated.h"
#include "../interpolate/aggregated.h"
#include "../gamma/aggregated.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(Color3WayMain)



Color3WayConfig::Color3WayConfig()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = 0.0;
		hue_y[i] = 0.0;
		value[i] = 0.0;
		saturation[i] = 0.0;
	}
}

int Color3WayConfig::equivalent(Color3WayConfig &that)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		if(!EQUIV(hue_x[i], that.hue_x[i]) ||
			!EQUIV(hue_y[i], that.hue_y[i]) ||
			!EQUIV(value[i], that.value[i]) ||
			!EQUIV(saturation[i], that.saturation[i])) return 0;
	}
	return 1;
}

void Color3WayConfig::copy_from(Color3WayConfig &that)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = that.hue_x[i];
		hue_y[i] = that.hue_y[i];
		value[i] = that.value[i];
		saturation[i] = that.saturation[i];
	}
}

void Color3WayConfig::interpolate(Color3WayConfig &prev, 
	Color3WayConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = prev.hue_x[i] * prev_scale + next.hue_x[i] * next_scale;
		hue_y[i] = prev.hue_y[i] * prev_scale + next.hue_y[i] * next_scale;
		value[i] = prev.value[i] * prev_scale + next.value[i] * next_scale;
		saturation[i] = prev.saturation[i] * prev_scale + next.saturation[i] * next_scale;
	}
}


void Color3WayConfig::boundaries()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		float point_radius = sqrt(SQR(hue_x[i]) + SQR(hue_y[i]));
		if(point_radius > 1)
		{
			float angle = atan2(hue_x[i], 
		 				hue_y[i]);
			hue_x[i] = sin(angle);
			hue_y[i] = cos(angle);
		}
	}
}

void Color3WayConfig::copy_to_all(int section)
{
	for(int i = 0; i < SECTIONS; i++)
	{
		hue_x[i] = hue_x[section];
		hue_y[i] = hue_y[section];
		value[i] = value[section];
		saturation[i] = saturation[section];
	}
}















Color3WayPackage::Color3WayPackage()
 : LoadPackage()
{
}






Color3WayUnit::Color3WayUnit(Color3WayMain *plugin, 
	Color3WayEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

// Lower = sharper curve
#define SHADOW_GAMMA 32.0
#define HIGHLIGHT_GAMMA 32.0
// Keep value == 0 from blowing up
#define FUDGE (1.0 / 256.0)
// Scale curve from 0 - 1
#define SHADOW_BORDER (1.0 / ((1.0 / SHADOW_GAMMA + FUDGE) / FUDGE))
#define HIGHLIGHT_BORDER (1.0 / ((1.0 / HIGHLIGHT_GAMMA + FUDGE) / FUDGE))

#define SHADOW_CURVE(value) \
	(((1.0 / (((value) / SHADOW_GAMMA + FUDGE) / FUDGE)) - SHADOW_BORDER) / (1.0 - SHADOW_BORDER))

#define SHADOW_LINEAR(value) (1.0 - (value))

#define HIGHLIGHT_CURVE(value) \
	(((1.0 / (((1.0 - value) / HIGHLIGHT_GAMMA + FUDGE) / FUDGE)) - HIGHLIGHT_BORDER) / (1.0 - HIGHLIGHT_BORDER))

#define HIGHLIGHT_LINEAR(value) \
	(value)

#define MIDTONE_CURVE(value, factor) \
	((factor) <= 0 ? \
	(1.0 - SHADOW_LINEAR(value) - HIGHLIGHT_CURVE(value)) : \
	(1.0 - SHADOW_CURVE(value) - HIGHLIGHT_LINEAR(value)))

#define TOTAL_TRANSFER(value, factor) \
	(factor[SHADOWS] * SHADOW_LINEAR(value) + \
	factor[MIDTONES] * MIDTONE_CURVE(value, factor[MIDTONES]) + \
	factor[HIGHLIGHTS] * HIGHLIGHT_LINEAR(value))

#define PROCESS_PIXEL(r, g, b) \
/* Apply hue */ \
	r = r + TOTAL_TRANSFER(r, r_factor); \
	g = g + TOTAL_TRANSFER(g, g_factor); \
	b = b + TOTAL_TRANSFER(b, b_factor); \
/* Apply saturation/value */ \
	float h, s, v; \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	v += TOTAL_TRANSFER(v, v_factor); \
	s += TOTAL_TRANSFER(s, s_factor); \
	s = MAX(s, 0); \
	HSV::hsv_to_rgb(r, g, b, h, s, v);


#define PROCESS(type, max, components, is_yuv) \
{ \
	type *in = (type*)plugin->get_input()->get_rows()[i]; \
	type *out = (type*)plugin->get_input()->get_rows()[i]; \
	for(int j = 0; j < w; j++) \
	{ \
/* Convert to RGB float */ \
	 	float r; \
	 	float g; \
	 	float b; \
		if(is_yuv) \
		{ \
			YUV::yuv_to_rgb_f(r, \
				g, \
				b, \
				(float)in[0] / max, \
				(float)in[1] / max - 0.5, \
				(float)in[2] / max - 0.5); \
			in += 3; \
		} \
		else \
		{ \
		 	r = (float)*in++ / max; \
		 	g = (float)*in++ / max; \
		 	b = (float)*in++ / max; \
		} \
 \
		PROCESS_PIXEL(r, g, b) \
 \
/* Convert to project colormodel */ \
		if(is_yuv) \
		{ \
			float y, u, v; \
			YUV::rgb_to_yuv_f(r, g, b, y, u, v); \
			r = y; \
			g = u + 0.5; \
			b = v + 0.5; \
		} \
		if(max == 0xff) \
		{ \
			CLAMP(r, 0, 1); \
			CLAMP(g, 0, 1); \
			CLAMP(b, 0, 1); \
		} \
		*out++ = (type)(r * max); \
		*out++ = (type)(g * max); \
		*out++ = (type)(b * max); \
		if(components == 4) \
		{ \
			in++; \
			out++; \
		} \
	} \
}

#define CALCULATE_FACTORS(s_out, v_out, s_in, v_in) \
	s_out = s_in; \
	v_out = v_in;



void Color3WayUnit::process_package(LoadPackage *package)
{
	Color3WayPackage *pkg = (Color3WayPackage*)package;
	int w = plugin->get_input()->get_w();

	float r_factor[SECTIONS];
	float g_factor[SECTIONS];
	float b_factor[SECTIONS];
	float s_factor[SECTIONS];
	float v_factor[SECTIONS];


	for(int i = 0; i < SECTIONS; i++)
	{
		plugin->calculate_factors(&r_factor[i], &g_factor[i], &b_factor[i], i);
		CALCULATE_FACTORS(s_factor[i], 
			v_factor[i], 
			plugin->config.saturation[i], 
			plugin->config.value[i])
// printf("Color3WayUnit::process_package %d %f %f %f %f %f\n", 
// __LINE__, 
// r_factor[i], 
// g_factor[i], 
// b_factor[i], 
// s_factor[i], 
// v_factor[i]);
	}



	for(int i = pkg->row1; i < pkg->row2; i++)
	{
		switch(plugin->get_input()->get_color_model())
		{
			case BC_RGB888:
				PROCESS(unsigned char, 0xff, 3, 0)
				break;
			case BC_RGBA8888:
				PROCESS(unsigned char, 0xff, 4, 0)
				break;
			case BC_YUV888:
				PROCESS(unsigned char, 0xff, 3, 1)
				break;
			case BC_YUVA8888:
				PROCESS(unsigned char, 0xff, 4, 1)
				break;
			case BC_RGB_FLOAT:
				PROCESS(float, 1.0, 3, 0)
				break;
			case BC_RGBA_FLOAT:
				PROCESS(float, 1.0, 4, 0)
				break;
		}
	}
}






Color3WayEngine::Color3WayEngine(Color3WayMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

Color3WayEngine::~Color3WayEngine()
{
}

void Color3WayEngine::init_packages()
{

#if 0
printf("Color3WayEngine::init_packages %d\n", __LINE__);
for(int i = 0; i <= 255; i++)
{
	printf("%f\t%f\t%f\n", 
		SHADOW_CURVE((float)i / 255), 
		MIDTONE_CURVE((float)i / 255), 
		HIGHLIGHT_CURVE((float)i / 255));
}
#endif

	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		Color3WayPackage *pkg = (Color3WayPackage*)get_package(i);
		pkg->row1 = plugin->get_input()->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->get_input()->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* Color3WayEngine::new_client()
{
	return new Color3WayUnit(plugin, this);
}

LoadPackage* Color3WayEngine::new_package()
{
	return new Color3WayPackage;
}










Color3WayMain::Color3WayMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	w = 500;
	h = 300;
	for(int i = 0; i < SECTIONS; i++) copy_to_all[i] = 0;
}

Color3WayMain::~Color3WayMain()
{
	
	delete engine;
}

const char* Color3WayMain::plugin_title() { return N_("Color 3 Way"); }
int Color3WayMain::is_realtime() { return 1; }


int Color3WayMain::reconfigure()
{
	
	return 0;
}

void Color3WayMain::process_pixel(float *r,
	float *g,
	float *b,
	float r_in, 
	float g_in, 
	float b_in,
	float x,
	float y)
{
	float r_factor[SECTIONS];
	float g_factor[SECTIONS];
	float b_factor[SECTIONS];
	float s_factor[SECTIONS];
	float v_factor[SECTIONS];
	for(int i = 0; i < SECTIONS; i++)
	{
		calculate_factors(r_factor + i, 
			g_factor + i, 
			b_factor + i, 
			x, 
			y);
		CALCULATE_FACTORS(s_factor[i], v_factor[i], 0, 0)
	}
	
	PROCESS_PIXEL(r_in, g_in, b_in);
	*r = r_in;
	*g = g_in;
	*b = b_in;
}

void Color3WayMain::calculate_factors(float *r, 
	float *g, 
	float *b, 
	float x, 
	float y)
{
//	float h = atan2(-x, -y) * 360 / 2 / M_PI + 180;
//	float v = 1.0;
//	float s = sqrt(SQR(x) + SQR(y));
//	HSV::hsv_to_rgb(*r, *g, *b, h, s, v);
//printf("Color3WayMain::calculate_factors %d %f %f %f %f %f\n", __LINE__, x, y, h, s, v);

	*r = sqrt(SQR(x) + SQR(y - -1));
	*g = sqrt(SQR(x - -1.0 / ROOT_2) + SQR(y - 1.0 / ROOT_2));
	*b = sqrt(SQR(x - 1.0 / ROOT_2) + SQR(y - 1.0 / ROOT_2));

	*r = 1.0 - *r;
	*g = 1.0 - *g;
	*b = 1.0 - *b;
}

void Color3WayMain::calculate_factors(float *r, float *g, float *b, int section)
{
	calculate_factors(r, g, b, config.hue_x[section], config.hue_y[section]);


//printf("Color3WayMain::calculate_factors %d %f %f %f\n", __LINE__, *r, *g, *b);
}







LOAD_CONFIGURATION_MACRO(Color3WayMain, Color3WayConfig)
NEW_WINDOW_MACRO(Color3WayMain, Color3WayWindow)





int Color3WayMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	need_reconfigure |= load_configuration();

	if(!engine) engine = new Color3WayEngine(this, 
//		1);
		PluginClient::smp + 1);

//printf("Color3WayMain::process_realtime 1 %d\n", need_reconfigure);
	if(need_reconfigure)
	{

		reconfigure();
		need_reconfigure = 0;
	}



	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		get_use_opengl());

	int aggregate_interpolate = 0;
	int aggregate_gamma = 0;
	get_aggregation(&aggregate_interpolate,
		&aggregate_gamma);



	engine->process_packages();


	return 0;
}


void Color3WayMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((Color3WayWindow*)thread->window)->lock_window("Color3WayMain::update_gui");
		((Color3WayWindow*)thread->window)->update();
		((Color3WayWindow*)thread->window)->unlock_window();
	}
}




void Color3WayMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("COLOR3WAY");
	for(int i = 0; i < SECTIONS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "HUE_X_%d", i);
		output.tag.set_property(string, config.hue_x[i]);
		sprintf(string, "HUE_Y_%d", i);
		output.tag.set_property(string, config.hue_y[i]);
		sprintf(string, "VALUE_%d", i);
		output.tag.set_property(string, config.value[i]);
		sprintf(string, "SATURATION_%d", i);
		output.tag.set_property(string, config.saturation[i]);
		if(is_defaults())
		{
			sprintf(string, "COPY_TO_ALL_%d", i);
			output.tag.set_property(string, copy_to_all[i]);
		}
	}

	if(is_defaults())
	{
		output.tag.set_property("W",  w);
		output.tag.set_property("H",  h);
	}
	
	output.append_tag();
	output.terminate_string();
}

void Color3WayMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COLOR3WAY"))
			{
				for(int i = 0; i < SECTIONS; i++)
				{
					char string[BCTEXTLEN];
					sprintf(string, "HUE_X_%d", i);
					config.hue_x[i] = input.tag.get_property(string, config.hue_x[i]);
					sprintf(string, "HUE_Y_%d", i);
					config.hue_y[i] = input.tag.get_property(string, config.hue_y[i]);
					sprintf(string, "VALUE_%d", i);
					config.value[i] = input.tag.get_property(string, config.value[i]);
					sprintf(string, "SATURATION_%d", i);
					config.saturation[i] = input.tag.get_property(string, config.saturation[i]);
					
					if(is_defaults())
					{
						sprintf(string, "COPY_TO_ALL_%d", i);
						copy_to_all[i] = input.tag.get_property(string, copy_to_all[i]);
					}
				}

				if(is_defaults())
				{
					w = input.tag.get_property("W", w);
					h = input.tag.get_property("H", h);
				}
			}
		}
	}
}

void Color3WayMain::get_aggregation(int *aggregate_interpolate,
	int *aggregate_gamma)
{
	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
		!strcmp(get_output()->get_prev_effect(0), "Gamma"))
	{
		*aggregate_interpolate = 1;
		*aggregate_gamma = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Interpolate Pixels"))
	{
		*aggregate_interpolate = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Gamma"))
	{
		*aggregate_gamma = 1;
	}
}

int Color3WayMain::handle_opengl()
{
#ifdef HAVE_GL

	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
	int aggregate_interpolate = 0;
	int aggregate_gamma = 0;

	get_aggregation(&aggregate_interpolate,
		&aggregate_gamma);

printf("Color3WayMain::handle_opengl %d %d\n", aggregate_interpolate, aggregate_gamma);
	if(aggregate_interpolate)
		INTERPOLATE_COMPILE(shader_stack, current_shader)

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, current_shader, aggregate_interpolate)

	COLORBALANCE_COMPILE(shader_stack, 
		current_shader, 
		aggregate_gamma || aggregate_interpolate)

	shader = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		shader_stack[4], 
		shader_stack[5], 
		shader_stack[6], 
		shader_stack[7], 
		0);

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);

		if(aggregate_interpolate) INTERPOLATE_UNIFORMS(shader);
		if(aggregate_gamma) GAMMA_UNIFORMS(shader);

		COLORBALANCE_UNIFORMS(shader);

	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
}







