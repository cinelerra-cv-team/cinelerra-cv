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

#include "clip.h"
#include "colormodels.h"
#include "bchash.h"
#include "filexml.h"
#include "greycstorationplugin.h"
#include "greycstorationwindow.h"
#include "language.h"
#include "picon_png.h"

#include <stdint.h>
#include <string.h>

#define cimg_plugin "greycstoration.h"

#include "CImg.h"

using namespace cimg_library;

REGISTER_PLUGIN(GreyCStorationMain)


GreyCStorationConfig::GreyCStorationConfig()
{
	amplitude    = 40.0f; //cimg_option("-dt",40.0f,"Regularization strength for one iteration (>=0)");
	sharpness    = 0.8f; // cimg_option("-p",0.8f,"Contour preservation for regularization (>=0)");
	anisotropy   = 0.8f; // cimg_option("-a",0.8f,"Regularization anisotropy (0<=a<=1)");
	noise_scale = 0.8f; // const float alpha = 0.6f; // cimg_option("-alpha",0.6f,"Noise scale(>=0)");
}

void GreyCStorationConfig::copy_from(GreyCStorationConfig &that)
{
	amplitude = that.amplitude;
	sharpness = that.sharpness;
	anisotropy = that.anisotropy;
	noise_scale = that.noise_scale;
}

int GreyCStorationConfig::equivalent(GreyCStorationConfig &that)
{
	return
		anisotropy == that.anisotropy &&
		sharpness == that.sharpness &&
		noise_scale == that.noise_scale &&
		amplitude == that.amplitude;
}

void GreyCStorationConfig::interpolate(GreyCStorationConfig &prev,
	GreyCStorationConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->amplitude = prev.amplitude * prev_scale + next.amplitude * next_scale;
	this->sharpness = prev.sharpness * prev_scale + next.sharpness * next_scale;
	this->anisotropy = prev.anisotropy * prev_scale + next.anisotropy * next_scale;
	this->noise_scale = prev.noise_scale * prev_scale + next.noise_scale * next_scale;
}



GreyCStorationMain::GreyCStorationMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

GreyCStorationMain::~GreyCStorationMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

const char* GreyCStorationMain::plugin_title() { return N_("GreyCStoration"); }
int GreyCStorationMain::is_realtime() { return 1; }

// flip code :)
template<typename T> void SWAP_PIXELS(int components,T* in,T* out)
{
	T temp = in[0];
	in[0] = out[0];
	out[0] = temp;

	temp = in[1];
	in[1] = out[1];
	out[1] = temp;

	temp = in[2];
	in[2] = out[2];
	out[2] = temp;

	if(components == 4)
	{
		temp = in[3];
		in[3] = out[3];
		out[3] = temp;
	}
}

template<typename T> void GreyCStorationMain::GREYCSTORATION(VFrame *frame,  int h, int w, int components)
{
	T **input_rows, **output_rows;
	T *input_row, *output_row;
	input_rows = ((T**)frame->get_rows());
	output_rows = ((T**)frame->get_rows());

	CImg<T> img(w,h,1,components);

	int i,j,k;

	T* data=img.ptr();

	// planar format

	for(i = 0; i<h; i++)
	{
		input_row=input_rows[i];
		for(j = 0; j < w; j++)
		{
			for (k=0;k<components;k++) {
				data[i*w+j+(w*h*k)]=input_row[k];
			}
			input_row+=components;
		}
	}


    const unsigned int nb_iter  = 1; //cimg_option("-iter",3,"Number of regularization iterations (>0)");
    const float sigma           = 1.1f; // cimg_option("-sigma",1.1f,"Geometry regularity (>=0)");
    const bool fast_approx      = true; // cimg_option("-fast",true,"Use fast approximation for regularization (0 or 1)");
    const float gauss_prec      = 2.0f; // cimg_option("-prec",2.0f,"Precision of the gaussian function for regularization (>0)");
    const float dl              = 0.8f; // cimg_option("-dl",0.8f,"Spatial integration step for regularization (0<=dl<=1)");
    const float da              = 30.0f; // cimg_option("-da",30.0f,"Angular integration step for regulatization (0<=da<=90)");
    const unsigned int interp   = 0; // cimg_option("-interp",0,"Interpolation type (0=Nearest-neighbor, 1=Linear, 2=Runge-Kutta)");
    const unsigned int tile     = 0; // cimg_option("-tile",0,"Use tiled mode (reduce memory usage");
    const unsigned int btile    = 4; // cimg_option("-btile",4,"Size of tile overlapping regions");
    const unsigned int threads  = 0; //cimg_option("-threads",1,"Number of threads used");

    img.greycstoration_run(config.amplitude,config.sharpness,config.anisotropy,config.noise_scale,sigma,1.0f,dl,da,gauss_prec,interp,fast_approx,tile,btile,threads);

	for(i = 0; i<h; i++)
	{
		input_row=input_rows[i];
		for(j = 0; j < w; j++)
		{
			for (k=0;k<components;k++) {
				input_row[k]=data[i*w+j+(w*h*k)];
			}
			input_row+=components;
		}
	}


}

int GreyCStorationMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int w = frame->get_w();
	int h = frame->get_h();
	int colormodel = frame->get_color_model();

	load_configuration();

	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		get_use_opengl());

	switch(colormodel)
	{
		case BC_RGB888:
		case BC_YUV888:
			GREYCSTORATION<unsigned char>(frame, h,w,3);
			break;
		case BC_RGB_FLOAT:
			GREYCSTORATION<float>(frame, h,w, 3);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			GREYCSTORATION<uint16_t>(frame, h,w, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			GREYCSTORATION<unsigned char>(frame, h,w, 4);
			break;
		case BC_RGBA_FLOAT:
			GREYCSTORATION<float>(frame, h,w, 4);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			GREYCSTORATION<uint16_t>(frame, h,w, 4);
			break;
	}
	return 0;
}


SHOW_GUI_MACRO(GreyCStorationMain, GreyCStorationThread)

RAISE_WINDOW_MACRO(GreyCStorationMain)

SET_STRING_MACRO(GreyCStorationMain)

NEW_PICON_MACRO(GreyCStorationMain)

LOAD_CONFIGURATION_MACRO(GreyCStorationMain, GreyCStorationConfig)

void GreyCStorationMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->greycamp_slider->update((int)config.amplitude);
		thread->window->greycsharp_slider->update((int)config.sharpness);
		thread->window->greycani_slider->update((int)config.anisotropy);
		thread->window->greycnoise_slider->update((int)config.noise_scale);
		thread->window->unlock_window();
	}
}

// these will end up into your project file (xml)
void GreyCStorationMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("GREYCSTORATION");

	output.tag.set_property("AMPLITUDE", config.amplitude);
	output.tag.set_property("SHARPNESS", config.sharpness);
	output.tag.set_property("ANISOTROPHY", config.anisotropy);
	output.tag.set_property("NOISE_SCALE", config.noise_scale);
	output.append_tag();

	output.tag.set_title("/GREYCSTORATION");
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void GreyCStorationMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("GREYCSTORATION"))
			{
				config.amplitude = input.tag.get_property("AMPLITUDE", config.amplitude);
				config.sharpness = input.tag.get_property("SHARPNESS", config.sharpness);
				config.anisotropy = input.tag.get_property("ANISOTROPHY", config.anisotropy);
				config.noise_scale = input.tag.get_property("NOISE_SCALE", config.noise_scale);
			}
		}
	}
}

// default values, saved in your $HOME/.bcast/greycstoration.rc (spec. below)
int GreyCStorationMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sgreycstoration.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.amplitude = defaults->get("GREYCSTORATION_AMPLITUDE", config.amplitude);
	config.sharpness = defaults->get("GREYCSTORATION_SHARPNESS", config.sharpness);
	config.anisotropy = defaults->get("GREYCSTORATION_ANISOTROPHY", config.anisotropy);
	config.noise_scale = defaults->get("GREYCSTORATION_NOISE_SCALE", config.noise_scale);
	return 0;
}

int GreyCStorationMain::save_defaults()
{
	defaults->update("GREYCSTORATION_AMPLITUDE", config.amplitude);
	defaults->update("GREYCSTORATION_SHARPNESS", config.sharpness);
	defaults->update("GREYCSTORATION_ANISOTROPHY", config.anisotropy);
	defaults->update("GREYCSTORATION_NOISE_SCALE", config.noise_scale);
	defaults->save();
	return 0;
}
