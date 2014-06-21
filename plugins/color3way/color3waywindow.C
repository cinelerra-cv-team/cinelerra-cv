
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

#include "bcsignals.h"
#include "color3waywindow.h"
#include "keys.h"
#include "language.h"
#include "theme.h"
#include "units.h"
#include <string.h>










Color3WayWindow::Color3WayWindow(Color3WayMain *plugin)
 : PluginClientWindow(plugin,
	plugin->w, 
	plugin->h, 
	500, 
	370, 
	1)
{ 
	this->plugin = plugin; 
}

Color3WayWindow::~Color3WayWindow()
{
}

void Color3WayWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = plugin->get_theme()->widget_border;
	int y = plugin->get_theme()->widget_border;

	for(int i = 0; i < SECTIONS; i++)
	{
		sections[i] = new Color3WaySection(plugin, 
			this,
			x,
			y,
			(get_w() - margin * 4) / 3,
			get_h() - margin * 2,
			i);
		sections[i]->create_objects();
		x += sections[i]->w + margin;
	}



	flash(0);
	show_window();
}

int Color3WayWindow::resize_event(int w, int h)
{
	int margin = plugin->get_theme()->widget_border;
	int x = sections[0]->x;
	int y = sections[0]->y;

	for(int i = 0; i < SECTIONS; i++)
	{
		sections[i]->reposition_window(x,
			y,
			(w - margin * 4) / 3,
			w - margin * 2);
		x += sections[i]->w + margin;
	}

	plugin->w = w;
	plugin->h = h;
	flush();
	return 1;
}


void Color3WayWindow::update()
{
	for(int i = 0; i < SECTIONS; i++)
	{
		sections[i]->update();
	}
}



Color3WaySection::Color3WaySection(Color3WayMain *plugin, 
		Color3WayWindow *gui,
		int x,
		int y,
		int w,
		int h,
		int section)
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

}

void Color3WaySection::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = this->x;
	int y = this->y;
	const char *titles[] = 
	{
		"Shadows",
		"Midtones",
		"Highlights"
	};
	
	
	gui->add_tool(title = new BC_Title(x + w / 2 - 
		gui->get_text_width(MEDIUMFONT, titles[section]) / 2,
		y,
		titles[section]));
	y += title->get_h() + margin;
	gui->add_tool(point = new Color3WayPoint(plugin, 
		gui,
		&plugin->config.hue_x[section], 
		&plugin->config.hue_y[section],
		x, 
		y,
		w / 2,
		section));
	y += point->get_h() + margin;
	
	gui->add_tool(value_title = new BC_Title(x, y, _("Value:")));
	y += value_title->get_h() + margin;
	gui->add_tool(value = new Color3WaySlider(plugin, 
		gui,
		&plugin->config.value[section], 
		x, 
		y,
		w,
		section));
	y += value->get_h() + margin;

	gui->add_tool(sat_title = new BC_Title(x, y, _("Saturation:")));
	y += sat_title->get_h() + margin;
	gui->add_tool(saturation = new Color3WaySlider(plugin, 
		gui,
		&plugin->config.saturation[section], 
		x, 
		y,
		w,
		section));
	y += saturation->get_h() + margin;

	gui->add_tool(reset = new Color3WayResetSection(plugin, 
		gui, 
		x, 
		y,
		section));

	y += reset->get_h() + margin;
	gui->add_tool(balance = new Color3WayBalanceSection(plugin, 
		gui, 
		x, 
		y,
		section));

	y += balance->get_h() + margin;
	gui->add_tool(copy = new Color3WayCopySection(plugin, 
		gui, 
		x, 
		y,
		section));
}

int Color3WaySection::reposition_window(int x, int y, int w, int h)
{
	int margin = plugin->get_theme()->widget_border;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	title->reposition_window(x + w / 2 -
		title->get_w() / 2,
		title->get_y());
	point->reposition_window(x, point->get_y(), w / 2);
	y = point->get_y() + point->get_h() + margin;
	value_title->reposition_window(x, y);
	y += value_title->get_h() + margin;
	value->reposition_window(x, y, w, value->get_h());
	value->set_pointer_motion_range(w);
	y += value->get_h() + margin;
	
	sat_title->reposition_window(x, y);
	y += sat_title->get_h() + margin;
	saturation->reposition_window(x, y, w, saturation->get_h());
	saturation->set_pointer_motion_range(w);
	y += saturation->get_h() + margin;
	reset->reposition_window(x, y);
	y += reset->get_h() + margin;
	balance->reposition_window(x, y);
	y += balance->get_h() + margin;
	copy->reposition_window(x, y);
	gui->flush();
	return 0;
}

void Color3WaySection::update()
{
	point->update();
	value->update(plugin->config.value[section]);
	saturation->update(plugin->config.saturation[section]);
}





Color3WayPoint::Color3WayPoint(Color3WayMain *plugin, 
	Color3WayWindow *gui,
	float *x_output, 
	float *y_output,
	int x, 
	int y,
	int radius,
	int section)
 : BC_SubWindow(x, 
 	y, 
	radius * 2,
	radius * 2)
{
	this->plugin = plugin;
	this->x_output = x_output;
	this->y_output = y_output;
	this->radius = radius;
	this->gui = gui;
	drag_operation = 0;
	status = COLOR_UP;
	memset(fg_images, 0, sizeof(BC_Pixmap*) * COLOR_IMAGES);
	bg_image = 0;
	active = 0;
	this->section = section;
}

Color3WayPoint::~Color3WayPoint()
{
	for(int i = 0; i < COLOR_IMAGES; i++)
		if(fg_images[i]) delete fg_images[i];
		
	delete bg_image;
}

int Color3WayPoint::initialize()
{
	BC_SubWindow::initialize();


	VFrame **data = plugin->get_theme()->get_image_set("color3way_point");
	for(int i = 0; i < COLOR_IMAGES; i++)
	{
		if(fg_images[i]) delete fg_images[i];
		fg_images[i] = new BC_Pixmap(gui, data[i], PIXMAP_ALPHA);
	}


	draw_face(1, 0);

	return 0;
}

int Color3WayPoint::reposition_window(int x, int y, int radius)
{
	this->radius = radius;
	BC_SubWindow::reposition_window(x, y, radius * 2, radius * 2);
	
	delete bg_image;
	bg_image = 0;
	draw_face(1, 0);
}

void Color3WayPoint::draw_face(int flash, int flush)
{
// Draw the color wheel
	if(!bg_image)
	{
		VFrame frame(0, -1, radius * 2, radius * 2, BC_RGB888, -1);
		for(int i = 0; i < radius * 2; i++)
		{
			unsigned char *row = frame.get_rows()[i];
			for(int j = 0; j < radius * 2; j++)
			{
				float point_radius = sqrt(SQR(i - radius) + SQR(j - radius));
				int r, g, b;

				if(point_radius < radius - 1)
				{
					float r_f, g_f, b_f;
					float angle = atan2((float)(j - radius) / radius, 
						(float)(i - radius) / radius) *
						360 /
						2 / 
						M_PI;
					angle -= 180;
					while(angle < 0)
						angle += 360;
					HSV::hsv_to_rgb(r_f, g_f, b_f, angle, point_radius / radius, 1.0);
					r = (int)(r_f * 0xff);
					g = (int)(g_f * 0xff);
					b = (int)(b_f * 0xff);
				}
				else
				if(point_radius < radius)
				{
					if(radius * 2 - i < j)
					{
						r = MEGREY >> 16;
						g = (MEGREY >> 8) &  0xff;
						b = MEGREY & 0xff;
					}
					else
					{
						r = 0;
						g = 0;
						b = 0;
					}
				}
				else
				{
					r = (gui->get_bg_color() & 0xff0000) >> 16;
					g = (gui->get_bg_color() & 0xff00) >> 8;
					b = (gui->get_bg_color() & 0xff);
				}

				*row++ = r;
				*row++ = g;
				*row++ = b;
			}
		}

		bg_image = new BC_Pixmap(get_parent(), &frame, PIXMAP_ALPHA);
	}

	draw_pixmap(bg_image);
	
// 	set_color(BLACK);
// 	draw_arc(0, 0, radius * 2, radius * 2, 45, 180);
// 	
// 	set_color(MEGREY);
// 	draw_arc(0, 0, radius * 2, radius * 2, 45, -180);


	fg_x = (int)(*x_output * (radius - fg_images[0]->get_w() / 2) + radius) - 
		fg_images[0]->get_w() / 2;
	fg_y = (int)(*y_output * (radius - fg_images[0]->get_h() / 2) + radius) - 
		fg_images[0]->get_h() / 2;
	
	draw_pixmap(fg_images[status], fg_x, fg_y);
	
// Text
	if(active)
	{
		int margin = plugin->get_theme()->widget_border;
		set_color(BLACK);
		set_font(MEDIUMFONT);
		char string[BCTEXTLEN];

		sprintf(string, "%.3f", *y_output);
		draw_text(radius - get_text_width(MEDIUMFONT, string) / 2, 
			get_text_ascent(MEDIUMFONT) + margin,
			string);

		sprintf(string, "%.3f", *x_output);
		draw_text(margin, 
			radius + get_text_ascent(MEDIUMFONT) / 2,
			string);


// 		float r_factor;
// 		float g_factor;
// 		float b_factor;
// 		plugin->calculate_factors(&r_factor, &g_factor, &b_factor, section);
// 
// 		sprintf(string, "%.3f", r_factor);
// 		draw_text(radius - get_text_width(MEDIUMFONT, string) / 2, 
// 			get_text_ascent(MEDIUMFONT) + margin,
// 			string);
// 
// 		sprintf(string, "%.3f", g_factor);
// 		draw_text(margin + radius - radius * 1.0 / ROOT_2, 
// 			radius + radius * 1.0 / ROOT_2 - margin,
// 			string);
// 
// 		sprintf(string, "%.3f", b_factor);
// 		draw_text(radius + radius * 1.0 / ROOT_2 - margin - get_text_width(MEDIUMFONT, string), 
// 			radius + radius * 1.0 / ROOT_2 - margin,
// 			string);
		set_font(MEDIUMFONT);
	}
	
	if(flash) this->flash(0);
	if(flush) this->flush();
}

int Color3WayPoint::deactivate()
{
	if(active)
	{
		active = 0;
		draw_face(1, 1);
	}
	return 1;
}

int Color3WayPoint::activate()
{
	if(!active)
	{
		get_top_level()->set_active_subwindow(this);
		active = 1;
	}
	return 1;
}

void Color3WayPoint::update()
{
	draw_face(1, 1);
}

int Color3WayPoint::button_press_event()
{
	if(is_event_win())
	{
		status = COLOR_DN;
		get_top_level()->deactivate();
		activate();
		draw_face(1, 1);

		starting_x = fg_x;
		starting_y = fg_y;
		offset_x = gui->get_relative_cursor_x();
		offset_y = gui->get_relative_cursor_y();
	}
	return 0;
}

int Color3WayPoint::button_release_event()
{
	if(status == COLOR_DN)
	{
		status = COLOR_HI;
		draw_face(1, 1);
		return 1;
	}
	return 0;
}

int Color3WayPoint::cursor_motion_event()
{
	int cursor_x = gui->get_relative_cursor_x();
	int cursor_y = gui->get_relative_cursor_y();
	int update_gui = 0;

//printf("Color3WayPoint::cursor_motion_event %d %d\n", __LINE__, status);
	if(status == COLOR_DN)
	{
//printf("Color3WayPoint::cursor_motion_event %d %d %d\n", __LINE__, starting_x, offset_x);
		int new_x = starting_x + cursor_x - offset_x;
		int new_y = starting_y + cursor_y - offset_y;

		*x_output = (float)(new_x + fg_images[0]->get_w() / 2 - radius) / 
			(radius - fg_images[0]->get_w() / 2);
		*y_output = (float)(new_y + fg_images[0]->get_h() / 2 - radius) / 
			(radius - fg_images[0]->get_h() / 2);

		plugin->config.boundaries();
		if(plugin->copy_to_all[section]) plugin->config.copy_to_all(section);
		plugin->send_configure_change();


		gui->update();

		
		handle_event();


		return 1;
	}

	return 0;
}

int Color3WayPoint::handle_event()
{
	return 1;
}


int Color3WayPoint::cursor_enter_event()
{
	if(is_event_win() && status != COLOR_HI && status != COLOR_DN)
	{
		status = COLOR_HI;
		draw_face(1, 1);
	}
	return 0;
}

int Color3WayPoint::cursor_leave_event()
{
	if(status == COLOR_HI)
	{
		status = COLOR_UP;
		draw_face(1, 1);
	}
	return 0;
}

int Color3WayPoint::keypress_event()
{
	int result = 0;
	if(!active) return 0;
	if(ctrl_down() || shift_down()) return 0;

	switch(get_keypress())
	{
		case UP:
			*y_output -= 0.001;
			result = 1;
			break;
		case DOWN:
			*y_output += 0.001;
			result = 1;
			break;
		case LEFT:
			*x_output -= 0.001;
			result = 1;
			break;
		case RIGHT:
			*x_output += 0.001;
			result = 1;
			break;
	}

	if(result)
	{
		plugin->config.boundaries();
		if(plugin->copy_to_all[section]) plugin->config.copy_to_all(section);
		plugin->send_configure_change();
		gui->update();
	}
	return result;
}








Color3WaySlider::Color3WaySlider(Color3WayMain *plugin, 
	Color3WayWindow *gui, 
	float *output, 
	int x, 
	int y,
	int w,
	int section)
 : BC_FSlider(x, 
 	y, 
	0, 
	w, 
	w,
	-1.0, 
	1.0, 
	*output)
{
	this->gui = gui;
	this->plugin = plugin;
	this->output = output;
	this->section = section;
    old_value = *output;
	set_precision(0.001);
}

Color3WaySlider::~Color3WaySlider()
{
}

int Color3WaySlider::handle_event()
{
	*output = get_value();
	if(plugin->copy_to_all[section]) plugin->config.copy_to_all(section);
	plugin->send_configure_change();
	gui->update();
	return 1;
}

char* Color3WaySlider::get_caption()
{
	sprintf(string, "%0.3f", *output);
	return string;
}





Color3WayResetSection::Color3WayResetSection(Color3WayMain *plugin, 
	Color3WayWindow *gui, 
	int x, 
	int y,
	int section)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
}

int Color3WayResetSection::handle_event()
{
	plugin->config.hue_x[section] = 0;
	plugin->config.hue_y[section] = 0;
	plugin->config.value[section] = 0;
	plugin->config.saturation[section] = 0;
	if(plugin->copy_to_all[section]) plugin->config.copy_to_all(section);
	plugin->send_configure_change();
	gui->update();
	return 1;
}






Color3WayCopySection::Color3WayCopySection(Color3WayMain *plugin, 
	Color3WayWindow *gui, 
	int x, 
	int y,
	int section)
 : BC_CheckBox(x, y, plugin->copy_to_all[section], _("Copy to all"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
}

int Color3WayCopySection::handle_event()
{
	if(get_value()) plugin->config.copy_to_all(section);
	plugin->copy_to_all[section] = get_value();
	gui->update();
	plugin->send_configure_change();
	return 1;
}





Color3WayBalanceSection::Color3WayBalanceSection(Color3WayMain *plugin, 
	Color3WayWindow *gui,
	int x, 
	int y,
	int section)
 : BC_GenericButton(x, y, _("White balance"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
}

int Color3WayBalanceSection::handle_event()
{
// Get colorpicker value
	float r = plugin->get_red();
	float g = plugin->get_green();
	float b = plugin->get_blue();

// Since the transfers aren't linear, use brute force search
	float step = 0.1;
	float center_x = 0;
	float center_y = 0;
	float range = 1;
	float best_diff = 255;
	float new_x = 0;
	float new_y = 0;
	
	while(step >= 0.001)
	{
		for(float x = center_x - range; x < center_x + range; x += step)
		{
			for(float y = center_y - range; y < center_y + range; y += step)
			{
				float new_r;
				float new_g;
				float new_b;
				plugin->process_pixel(&new_r,
					&new_g,
					&new_b,
					r, 
					g, 
					b,
					x,
					y);
				float min = MIN(new_r, new_g);
				min = MIN(min, new_b);
				float max = MAX(new_r, new_g);
				max = MAX(max, new_b);
				float diff = max - min;
				
				if(diff < best_diff)
				{
					best_diff = diff;
					new_x = x;
					new_y = y;
				}
			}
		}

		step /= 2;
		range /= 2;
		center_x = new_x;
		center_y = new_y;
	}

	new_x = Units::quantize(new_x, 0.001);
	new_y = Units::quantize(new_y, 0.001);

	plugin->config.hue_x[section] = new_x;
	plugin->config.hue_y[section] = new_y;
	plugin->config.boundaries();
	if(plugin->copy_to_all[section]) plugin->config.copy_to_all(section);
	plugin->send_configure_change();

	
	gui->update();

	return 1;
}









