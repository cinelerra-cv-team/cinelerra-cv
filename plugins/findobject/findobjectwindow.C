
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "language.h"
#include "findobject.h"
#include "findobjectwindow.h"
#include "theme.h"


FindObjectWindow::FindObjectWindow(FindObjectMain *plugin)
 : PluginClientWindow(plugin,
 	300, 
	550, 
	300,
	550,
	0)
{
	this->plugin = plugin; 
}

FindObjectWindow::~FindObjectWindow()
{
}

void FindObjectWindow::create_objects()
{
	int x1 = 10, x = 10, y = 10;
	int x2 = 310;
	BC_Title *title;


	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Algorithm:")));
	add_subwindow(algorithm = new FindObjectAlgorithm(plugin, 
		this,
		x1 + title->get_w() + 10, 
		y));
	algorithm->create_objects();
	y += algorithm->get_h() + plugin->get_theme()->widget_border;


	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Search radius:\n(W/H Percent of image)")));
	add_subwindow(global_range_w = new FindObjectGlobalRange(plugin, 
		x1 + title->get_w() + 10, 
		y,
		&plugin->config.global_range_w));
	add_subwindow(global_range_h = new FindObjectGlobalRange(plugin, 
		x1 + title->get_w() + 10 + global_range_w->get_w(), 
		y,
		&plugin->config.global_range_h));

	y += 50;
	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Object size:\n(W/H Percent of image)")));
	add_subwindow(global_block_w = new FindObjectBlockSize(plugin, 
		x1 + title->get_w() + 10, 
		y,
		&plugin->config.global_block_w));
	add_subwindow(global_block_h = new FindObjectBlockSize(plugin, 
		x1 + title->get_w() + 10 + global_block_w->get_w(), 
		y,
		&plugin->config.global_block_h));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Block X:")));
	add_subwindow(block_x = new FindObjectBlockCenter(plugin, 
		this, 
		x + title->get_w() + 10, 
		y,
		&plugin->config.block_x));
	add_subwindow(block_x_text = new FindObjectBlockCenterText(plugin, 
		this, 
		x + title->get_w() + 10 + block_x->get_w() + 10, 
		y + 10,
		&plugin->config.block_x));
	block_x->center_text = block_x_text;
	block_x_text->center = block_x;

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Block Y:")));
	add_subwindow(block_y = new FindObjectBlockCenter(plugin, 
		this, 
		x + title->get_w() + 10, 
		y,
		&plugin->config.block_y));
	add_subwindow(block_y_text = new FindObjectBlockCenterText(plugin, 
		this, 
		x + title->get_w() + 10 + block_y->get_w() + 10, 
		y + 10,
		&plugin->config.block_y));
	block_y->center_text = block_y_text;
	block_y_text->center = block_y;


	y += 40;
	add_subwindow(draw_keypoints = new FindObjectDrawKeypoints(plugin,
		this,
		x,
		y));

	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_border = new FindObjectDrawBorder(plugin,
		this,
		x,
		y));

	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(replace_object = new FindObjectReplace(plugin,
		this,
		x,
		y));

	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_object_border = new FindObjectDrawObjectBorder(plugin,
		this,
		x,
		y));


	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Object layer:")));
	object_layer = new FindObjectLayer(plugin, 
		this,
		x + title->get_w() + 10, 
		y,
		&plugin->config.object_layer);
	object_layer->create_objects();
	y += object_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Replacement object layer:")));
	replace_layer = new FindObjectLayer(plugin, 
		this,
		x + title->get_w() + 10, 
		y,
		&plugin->config.replace_layer);
	replace_layer->create_objects();
	y += replace_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Output/scene layer:")));
	scene_layer = new FindObjectLayer(plugin, 
		this,
		x + title->get_w() + 10, 
		y,
		&plugin->config.scene_layer);
	scene_layer->create_objects();
	y += scene_layer->get_h() + plugin->get_theme()->widget_border;


	add_subwindow(title = new BC_Title(x, y + 10, _("Object blend amount:")));
	add_subwindow(blend = new FindObjectBlend(plugin, 
		x + title->get_w() + plugin->get_theme()->widget_border, 
		y,
		&plugin->config.blend));
	y += blend->get_h();


	add_subwindow(title = new BC_Title(x, y + 10, _("Camshift VMIN:")));
	add_subwindow(vmin = new FindObjectCamParam(plugin, 
		x + title->get_w() + plugin->get_theme()->widget_border, 
		y,
		&plugin->config.vmin));
	y += vmin->get_h() * 2 / 3;

	add_subwindow(title = new BC_Title(x, y + 10, _("Camshift VMAX:")));
	add_subwindow(vmax = new FindObjectCamParam(plugin, 
		x + title->get_w() + vmin->get_w() + plugin->get_theme()->widget_border, 
		y,
		&plugin->config.vmax));
	y += vmin->get_h() * 2 / 3;

	add_subwindow(title = new BC_Title(x, y + 10, _("Camshift SMIN:")));
	add_subwindow(smin = new FindObjectCamParam(plugin, 
		x + title->get_w() + plugin->get_theme()->widget_border, 
		y,
		&plugin->config.smin));
	y += vmin->get_h();



	show_window(1);
}







FindObjectGlobalRange::FindObjectGlobalRange(FindObjectMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, 
		y, 
		(int64_t)*value,
		(int64_t)MIN_RADIUS,
		(int64_t)MAX_RADIUS)
{
	this->plugin = plugin;
	this->value = value;
}


int FindObjectGlobalRange::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}





FindObjectBlockSize::FindObjectBlockSize(FindObjectMain *plugin, 
	int x, 
	int y,
	float *value)
 : BC_FPot(x, 
		y, 
		(float)*value,
		(float)MIN_BLOCK,
		(float)MAX_BLOCK)
{
	this->plugin = plugin;
	this->value = value;
	set_precision(0.1);
}



int FindObjectBlockSize::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}







FindObjectBlockCenter::FindObjectBlockCenter(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y,
	float *value)
 : BC_FPot(x,
 	y,
	*value,
	(float)0, 
	(float)100)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	set_precision(0.1);
}

int FindObjectBlockCenter::handle_event()
{
	*value = get_value();
	center_text->update(*value);
	plugin->send_configure_change();
	return 1;
}






FindObjectBlockCenterText::FindObjectBlockCenterText(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y,
	float *value)
 : BC_TextBox(x,
 	y,
	75,
	1,
	*value)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	set_precision(1);
}

int FindObjectBlockCenterText::handle_event()
{
	*value = atof(get_text());
	center->update(*value);
	plugin->send_configure_change();
	return 1;
}







FindObjectDrawBorder::FindObjectDrawBorder(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
 	y, 
	plugin->config.draw_border,
	_("Draw border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawBorder::handle_event()
{
	plugin->config.draw_border = get_value();
	plugin->send_configure_change();
	return 1;
}






FindObjectDrawKeypoints::FindObjectDrawKeypoints(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
 	y, 
	plugin->config.draw_keypoints,
	_("Draw keypoints"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawKeypoints::handle_event()
{
	plugin->config.draw_keypoints = get_value();
	plugin->send_configure_change();
	return 1;
}




FindObjectReplace::FindObjectReplace(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
 	y, 
	plugin->config.replace_object,
	_("Replace object"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectReplace::handle_event()
{
	plugin->config.replace_object = get_value();
	plugin->send_configure_change();
	return 1;
}






FindObjectDrawObjectBorder::FindObjectDrawObjectBorder(FindObjectMain *plugin, 
	FindObjectWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
 	y, 
	plugin->config.draw_object_border,
	_("Draw object border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawObjectBorder::handle_event()
{
	plugin->config.draw_object_border = get_value();
	plugin->send_configure_change();
	return 1;
}






FindObjectLayer::FindObjectLayer(FindObjectMain *plugin, 
	FindObjectWindow *gui, 
	int x, 
	int y,
	int *value)
 : BC_TumbleTextBox(gui,
 	*value,
	MIN_LAYER,
	MAX_LAYER,
 	x, 
 	y, 
	calculate_w(gui))
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
}

int FindObjectLayer::handle_event()
{
	*value = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

int FindObjectLayer::calculate_w(FindObjectWindow *gui)
{
	int result = 0;
	result = gui->get_text_width(MEDIUMFONT, "000");
	return result + 50;
}








FindObjectAlgorithm::FindObjectAlgorithm(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
 	y, 
	calculate_w(gui),
	to_text(plugin->config.algorithm))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FindObjectAlgorithm::handle_event()
{
	plugin->config.algorithm = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void FindObjectAlgorithm::create_objects()
{
	add_item(new BC_MenuItem(to_text(NO_ALGORITHM)));
	add_item(new BC_MenuItem(to_text(ALGORITHM_SURF)));
	add_item(new BC_MenuItem(to_text(ALGORITHM_CAMSHIFT)));
	add_item(new BC_MenuItem(to_text(ALGORITHM_BLOB)));
}

int FindObjectAlgorithm::from_text(char *text)
{
	if(!strcmp(text, _("Don't Calculate"))) return NO_ALGORITHM;
	if(!strcmp(text, _("SURF"))) return ALGORITHM_SURF;
	if(!strcmp(text, _("CAMSHIFT"))) return ALGORITHM_CAMSHIFT;
	if(!strcmp(text, _("Blob"))) return ALGORITHM_BLOB;
}

char* FindObjectAlgorithm::to_text(int mode)
{
	switch(mode)
	{
		case NO_ALGORITHM:
			return _("Don't Calculate");
			break;
		case ALGORITHM_SURF:
			return _("SURF");
			break;
		case ALGORITHM_BLOB:
			return _("Blob");
			break;
		case ALGORITHM_CAMSHIFT:
		default:
			return _("CAMSHIFT");
			break;
	}
}

int FindObjectAlgorithm::calculate_w(FindObjectWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(NO_ALGORITHM)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_SURF)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_CAMSHIFT)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_BLOB)));
	return result + 50;
}








FindObjectCamParam::FindObjectCamParam(FindObjectMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, 
		y, 
		(int64_t)*value,
		(int64_t)MIN_CAMSHIFT,
		(int64_t)MAX_CAMSHIFT)
{
	this->plugin = plugin;
	this->value = value;
}


int FindObjectCamParam::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}








FindObjectBlend::FindObjectBlend(FindObjectMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, 
		y, 
		(int64_t)*value,
		(int64_t)MIN_BLEND,
		(int64_t)MAX_BLEND)
{
	this->plugin = plugin;
	this->value = value;
}


int FindObjectBlend::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}













