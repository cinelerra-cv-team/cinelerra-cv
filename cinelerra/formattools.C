
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

#include "asset.h"
#include "guicast.h"
#include "file.h"
#include "formattools.h"
#include "language.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "preferences.h"
#include "quicktime.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>
#include "pipe.h"

const struct container_type ContainerSelection::media_containers[] =
{
	{ N_("AC3"), FILE_AC3, "AC3", "ac3" },
	{ N_("Apple/SGI AIFF"), FILE_AIFF, "AIFF", "aif" },
	{ N_("Sun/NeXT AU"), FILE_AU, "AU", "au" },
	{ N_("JPEG"), FILE_JPEG, "JPEG", "jpg" },
	{ N_("JPEG Sequence"), FILE_JPEG_LIST, "JPEG_LIST", "list" },
	{ N_("Microsoft AVI"), FILE_AVI, "AVI", "avi" },
#ifdef HAVE_OPENEXR
	{ N_("EXR"), FILE_EXR, "EXR", "exr" },
	{ N_("EXR Sequence"), FILE_EXR_LIST, "EXR_LIST", "list" },
#endif
	{ N_("YUV4MPEG Stream"), FILE_YUV, "YUV", "m2v" },
	{ N_("Microsoft WAV"), FILE_WAV, "WAV", "wav" },
	{ N_("QuickTime/MOV"), FILE_MOV, "MOV", "mov" },
	{ N_("Raw DV"), FILE_RAWDV, "RAWDV", "dv" },
	{ N_("OGG Theora/Vorbis"), FILE_OGG, "OGG", "ogg" },
	{ N_("Raw PCM"), FILE_PCM, "PCM", "pcm" },
	{ N_("PNG"), FILE_PNG, "PNG", "png" },
	{ N_("PNG Sequence"), FILE_PNG_LIST, "PNG_LIST", "list" },
	{ N_("TGA"), FILE_TGA, "TGA", "tga" },
	{ N_("TGA Sequence"), FILE_TGA_LIST, "TGA_LIST", "list" },
	{ N_("TIFF"), FILE_TIFF, "TIFF", "tif" },
	{ N_("TIFF Sequence"), FILE_TIFF_LIST, "TIFF_LIST", "list" },
	{ N_("MPEG"), FILE_MPEG, "MPEG", "mpg" },
	{ 0, 0 }
};

#define NUM_MEDIA_CONTAINERS (sizeof(ContainerSelection::media_containers) / sizeof(struct container_type) - 1)

int FormatPopup::brender_menu[] = { FILE_JPEG_LIST,
	FILE_PNG_LIST, FILE_TIFF_LIST };
int FormatPopup::frender_menu[] = { FILE_AC3 , FILE_AIFF, FILE_AU,
	FILE_JPEG, FILE_JPEG_LIST,
	FILE_AVI,
#ifdef HAVE_OPENEXR
	FILE_EXR, FILE_EXR_LIST,
#endif
	FILE_YUV, FILE_WAV,
	FILE_MOV,
	FILE_RAWDV,
	FILE_OGG, FILE_PCM,
	FILE_PNG, FILE_PNG_LIST, FILE_TGA, FILE_TGA_LIST,
	FILE_TIFF, FILE_TIFF_LIST
};

FormatTools::FormatTools(MWindow *mwindow,
	BC_WindowBase *window,
	Asset *asset)
{
	this->mwindow = mwindow;
	this->window = window;
	this->asset = asset;

	aparams_button = 0;
	vparams_button = 0;
	aparams_thread = 0;
	vparams_thread = 0;
	channels_tumbler = 0;
	path_textbox = 0;
	path_button = 0;
	path_recent = 0;
	w = 0;
}

FormatTools::~FormatTools()
{
	delete path_recent;
	delete path_button;
	delete path_textbox;
	delete format_popup;

	if(aparams_button) delete aparams_button;
	if(vparams_button) delete vparams_button;
	if(aparams_thread) delete aparams_thread;
	if(vparams_thread) delete vparams_thread;
	if(channels_tumbler) delete channels_tumbler;
}

int FormatTools::create_objects(int &init_x,
	int &init_y,
	int do_audio,    // Include support for audio
	int do_video,   // Include support for video
	int prompt_audio,  // Include checkbox for audio
	int prompt_video,
	int prompt_audio_channels,
	int prompt_video_compression,
	const char *locked_compressor,
	int recording,
	int *strategy,
	int brender,
	int horizontal_layout)
{
	int x = init_x;
	int y = init_y;
	int ylev = init_y;

	this->locked_compressor = locked_compressor;
	this->recording = recording;
	this->use_brender = brender;
	this->do_audio = do_audio;
	this->do_video = do_video;
	this->prompt_audio = prompt_audio;
	this->prompt_audio_channels = prompt_audio_channels;
	this->prompt_video = prompt_video;
	this->prompt_video_compression = prompt_video_compression;
	this->strategy = strategy;

// Modify strategy depending on render farm
	if(strategy)
	{
		if(mwindow->preferences->use_renderfarm)
		{
			if(*strategy == FILE_PER_LABEL)
				*strategy = FILE_PER_LABEL_FARM;
			else
			if(*strategy == SINGLE_PASS)
				*strategy = SINGLE_PASS_FARM;
		}
		else
		{
			if(*strategy == FILE_PER_LABEL_FARM)
				*strategy = FILE_PER_LABEL;
			else
			if(*strategy == SINGLE_PASS_FARM)
				*strategy = SINGLE_PASS;
		}
	}

	if(!recording)
	{
		window->add_subwindow(path_textbox = new FormatPathText(x, y, this));
		x += 305;
		path_recent = new BC_RecentList("PATH", mwindow->defaults,
					path_textbox, 10, x, y, 300, 100);
		window->add_subwindow(path_recent);
		path_recent->load_items(ContainerSelection::container_prefix(asset->format));

		x += 18;
		window->add_subwindow(path_button = new BrowseButton(
			mwindow,
			window,
			path_textbox, 
			x, 
			y, 
			asset->path,
			_("Output to file"),
			_("Select a file to write to:"),
			0));

// Set w for user.
		w = x + path_button->get_w() + 5;

		y += path_textbox->get_h() + 10;
	}
	else
		w = x + 305;

	x = init_x;
	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += 90;

	format_popup = new FormatPopup(window, x, y, &asset->format, this, use_brender);

	x = init_x;
	y += format_popup->get_h() + 10;
	if(do_audio)
	{
		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		window->add_subwindow(aparams_button = new FormatAParams(mwindow, this, x, y));
		x += aparams_button->get_w() + 10;
		if(prompt_audio) 
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y, this, asset->audio_data));
		}
		x = init_x;
		ylev = y;
		y += aparams_button->get_h() + 5;

		aparams_thread = new FormatAThread(this);
	}

	if(do_video)
	{
		if(horizontal_layout && do_audio){
			x += 370;
			y = ylev;
		}

		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		if(prompt_video_compression)
		{
			window->add_subwindow(vparams_button = new FormatVParams(mwindow, this, x, y));
			x += vparams_button->get_w() + 10;
		}

		if(prompt_video)
		{
			window->add_subwindow(video_switch = new FormatVideo(x, y, this, asset->video_data));
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		vparams_thread = new FormatVThread(this);
	}

	x = init_x;
	if(strategy)
	{
		window->add_subwindow(multiple_files = new FormatMultiple(mwindow, x, y, strategy));
		y += multiple_files->get_h() + 10;
	}
	init_y = y;
	return 0;
}

void FormatTools::update_driver(int driver)
{
	this->video_driver = driver;

	switch(driver)
	{
	case CAPTURE_DVB:
// Just give the user information about how the stream is going to be
// stored but don't change the asset.
// Want to be able to revert to user settings.
		if(asset->format != FILE_MPEG)
		{
			format_popup->update(FILE_MPEG);
			asset->format = FILE_MPEG;
		}
		locked_compressor = 0;
		audio_switch->update(1);
		video_switch->update(1);
		break;

	case CAPTURE_IEC61883:
	case CAPTURE_FIREWIRE:
		if(asset->format != FILE_AVI &&
			asset->format != FILE_MOV)
		{
			format_popup->update(FILE_MOV);
			asset->format = FILE_MOV;
		}
		else
			format_popup->update(asset->format);
		locked_compressor = QUICKTIME_DVSD;
		strcpy(asset->vcodec, QUICKTIME_DVSD);
		audio_switch->update(asset->audio_data);
		video_switch->update(asset->video_data);
		break;

	case CAPTURE_BUZ:
	case VIDEO4LINUX2JPEG:
		if(asset->format != FILE_AVI &&
			asset->format != FILE_MOV)
		{
			format_popup->update(FILE_MOV);
			asset->format = FILE_MOV;
		}
		else
			format_popup->update(asset->format);
		locked_compressor = QUICKTIME_MJPA;
		audio_switch->update(asset->audio_data);
		video_switch->update(asset->video_data);
		break;

	default:
		format_popup->update(asset->format);
		locked_compressor = 0;
		audio_switch->update(asset->audio_data);
		video_switch->update(asset->video_data);
		break;
	}
	close_format_windows();
}

int FormatTools::handle_event()
{
	return 0;
}

Asset* FormatTools::get_asset()
{
	return asset;
}

void FormatTools::update_extension()
{
	const char *extension = ContainerSelection::container_extension(asset->format);
	if(extension)
	{
		char *ptr = strrchr(asset->path, '.');
		if(!ptr)
		{
			ptr = asset->path + strlen(asset->path);
			*ptr = '.';
		}
		ptr++;
		strcpy(ptr, extension);

		int character1 = ptr - asset->path;
		int character2 = ptr - asset->path + strlen(extension);
		*(asset->path + character2) = 0;
		if(path_textbox) 
		{
			path_textbox->update(asset->path);
			path_textbox->set_selection(character1, character2, character2);
		}
	}
}

void FormatTools::update(Asset *asset, int *strategy)
{
	this->asset = asset;
	this->strategy = strategy;

	if(path_textbox) 
		path_textbox->update(asset->path);
	format_popup->update(asset->format);
	if(do_audio && audio_switch) audio_switch->update(asset->audio_data);
	if(do_video && video_switch) video_switch->update(asset->video_data);
	if(strategy)
	{
		multiple_files->update(strategy);
	}
	close_format_windows();
}

void FormatTools::close_format_windows()
{
	if(aparams_thread) aparams_thread->file->close_window();
	if(vparams_thread) vparams_thread->file->close_window();
}

int FormatTools::get_w()
{
	return w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;

	if(path_textbox) 
	{
		path_textbox->reposition_window(x, y);
		x += 305;
		path_button->reposition_window(x, y);
		x -= 305;
		y += 35;
	}

	format_title->reposition_window(x, y);
	x += 90;
	format_popup->reposition_window(x, y);

	x = init_x;
	y += format_popup->get_h() + 10;

	if(do_audio)
	{
		audio_title->reposition_window(x, y);
		x += 80;
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + 10;
		if(prompt_audio) audio_switch->reposition_window(x, y);

		x = init_x;
		y += aparams_button->get_h() + 20;
		if(prompt_audio_channels)
		{
			channels_title->reposition_window(x, y);
			x += 260;
			channels_button->reposition_window(x, y);
			x += channels_button->get_w() + 5;
			channels_tumbler->reposition_window(x, y);
			y += channels_button->get_h() + 20;
			x = init_x;
		}
	}

	if(do_video)
	{
		video_title->reposition_window(x, y);
		x += 80;
		if(prompt_video_compression)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + 10;
		}

		if(prompt_video)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		x = init_x;
	}

	if(strategy)
	{
		multiple_files->reposition_window(x, y);
		y += multiple_files->get_h() + 10;
	}

	init_y = y;
}

int FormatTools::set_audio_options()
{
	if(!aparams_thread->running())
	{
		aparams_thread->start();
	}
	else
	{
		aparams_thread->file->raise_window();
	}
	return 0;
}

int FormatTools::set_video_options()
{
	if(!vparams_thread->running())
	{
		vparams_thread->start();
	}
	else
	{
		vparams_thread->file->raise_window();
	}
	return 0;
}

void FormatTools::format_changed()
{
	if(!use_brender)
		update_extension();
	close_format_windows();
	if(path_recent)
		path_recent->load_items(ContainerSelection::container_prefix(asset->format));
}


FormatAParams::FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}

int FormatAParams::handle_event() 
{
	format->set_audio_options(); 
	return 1;
}


FormatVParams::FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure video compression"));
}

int FormatVParams::handle_event() 
{
	format->set_video_options(); 
	return 1;
}


FormatAThread::FormatAThread(FormatTools *format)
 : Thread()
{ 
	this->format = format; 
	file = new File;
}

FormatAThread::~FormatAThread() 
{
	delete file;
}

void FormatAThread::run()
{
	file->get_options(format, 1, 0);
}


FormatVThread::FormatVThread(FormatTools *format)
 : Thread()
{
	this->format = format;
	file = new File;
}

FormatVThread::~FormatVThread() 
{
	delete file;
}

void FormatVThread::run()
{
	file->get_options(format, 0, 1);
}

FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, 300, 1, format->asset->path) 
{
	this->format = format; 
}

int FormatPathText::handle_event() 
{
	strcpy(format->asset->path, get_text());
	format->handle_event();
	return 1;
}


FormatAudio::FormatAudio(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
	y,
	default_, 
	(char*)(format->recording ? _("Record audio tracks") : _("Render audio tracks")))
{ 
	this->format = format; 
}

int FormatAudio::handle_event()
{
	format->asset->audio_data = get_value();
	return 1;
}


FormatVideo::FormatVideo(int x, int y, FormatTools *format, int default_)
 : BC_CheckBox(x, 
	y,
	default_, 
	(char*)(format->recording ? _("Record video tracks") : _("Render video tracks")))
{
	this->format = format;
}

int FormatVideo::handle_event()
{
	format->asset->video_data = get_value();
	return 1;
}


FormatChannels::FormatChannels(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, 100, 1, format->asset->channels) 
{ 
	this->format = format;
}

int FormatChannels::handle_event() 
{
	format->asset->channels = atol(get_text());
	return 1;
}

FormatToTracks::FormatToTracks(int x, int y, int *output)
 : BC_CheckBox(x, y, *output, _("Overwrite project with output"))
{ 
	this->output = output; 
}

int FormatToTracks::handle_event()
{
	*output = get_value();
	return 1;
}


FormatMultiple::FormatMultiple(MWindow *mwindow, int x, int y, int *output)
 : BC_CheckBox(x, 
	y,
	(*output == FILE_PER_LABEL) || (*output == FILE_PER_LABEL_FARM), 
	_("Create new file at each label"))
{ 
	this->output = output;
	this->mwindow = mwindow;
}

int FormatMultiple::handle_event()
{
	if(get_value())
	{
		if(mwindow->preferences->use_renderfarm)
			*output = FILE_PER_LABEL_FARM;
		else
			*output = FILE_PER_LABEL;
	}
	else
	{
		if(mwindow->preferences->use_renderfarm)
			*output = SINGLE_PASS_FARM;
		else
			*output = SINGLE_PASS;
	}
	return 1;
}

void FormatMultiple::update(int *output)
{
	this->output = output;
	if(*output == FILE_PER_LABEL_FARM ||
		*output ==FILE_PER_LABEL)
		set_value(1);
	else
		set_value(0);
}

FormatPopup::FormatPopup(BC_WindowBase *parent,
	int x,
	int y,
	int *output,
	FormatTools *tools,
	int use_brender)
{
	int *menu;
	int length;

	if(use_brender)
	{
		menu = brender_menu;
		length = sizeof(brender_menu) / sizeof(int);
	}
	else
	{
		menu = frender_menu;
		length = sizeof(frender_menu) / sizeof(int);
	}
	current_menu = new struct selection_int[length + 1];

	for(int i = 0; i < length; i++)
	{
		const struct container_type *ct = ContainerSelection::get_item(menu[i]);
		current_menu[i].text = ct->text;
		current_menu[i].value = ct->value;
	}
	current_menu[length].text = 0;

	parent->add_subwindow(selection = new ContainerSelection(x, y, parent,
		current_menu, output, tools));
	selection->update(*output);
}

FormatPopup::~FormatPopup()
{
	delete [] current_menu;
}

int FormatPopup::get_h()
{
	return selection->get_h();
}

void FormatPopup::update(int value)
{
	selection->update(value);
}

void FormatPopup::reposition_window(int x, int y)
{
	selection->reposition_window(x, y);
}


ContainerSelection::ContainerSelection(int x, int y, BC_WindowBase *base,
	selection_int *menu, int *value, FormatTools *tools)
 : Selection(x, y , base, menu, value, SELECTION_VARWIDTH)
{
	disable(1);
	this->tools = tools;
}

void ContainerSelection::update(int value)
{
	BC_TextBox::update(_(container_to_text(value)));
}

int ContainerSelection::handle_event()
{
	if(current_int && current_int->value != *intvalue)
	{
		*intvalue = current_int->value;
		tools->format_changed();
	}
	return 1;
}

const char *ContainerSelection::container_to_text(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].text;
	}
	return N_("Unknown");
}

int ContainerSelection::text_to_container(char *string)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(!strcmp(media_containers[i].text, string))
			return media_containers[i].value;
	}
// Backward compatibility
	if(!strcmp(string, "Quicktime for Linux"))
		return FILE_MOV;
	return FILE_UNKNOWN;
}

const struct container_type *ContainerSelection::get_item(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return &media_containers[i];
	}
	return 0;
}

const char *ContainerSelection::container_prefix(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].prefix;
	}
	return "UNKNOWN";
}

const char *ContainerSelection::container_extension(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].extension;
	}
	return 0;
}
