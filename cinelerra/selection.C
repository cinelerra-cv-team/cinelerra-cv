
/*
 * CINELERRA
 * Copyright (C) 2014-2017 Einar Rünkaru <einarrunkaru at gmail dot com>
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

#include "selection.h"
#include "bcpopupmenu.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "bctextbox.h"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcwindow.h"
#include "bcwindowbase.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "clip.h"
#include "language.h"
#include "mwindow.h"
#include "theme.h"
#include "vframe.h"

#include <stdlib.h>

const struct selection_int SampleRateSelection::sample_rates[] =
{
	{ "8000", 8000 },
	{ "16000", 16000 },
	{ "22050", 22050 },
	{ "32000", 32000 },
	{ "44100", 44100 },
	{ "48000", 48000 },
	{ "96000", 96000 },
	{ "192000", 192000 },
	{ 0, 0 }
};

const struct selection_2int SampleBitsSelection::sample_bits[] =
{
	{ N_("8 Bit Linear"), SBITS_LINEAR8, 8 },
	{ N_("16 Bit Linear"), SBITS_LINEAR16, 16 },
	{ N_("24 Bit Linear"), SBITS_LINEAR24, 24 },
	{ N_("32 Bit Linear"), SBITS_LINEAR32, 32 },
	{ N_("u Law"), SBITS_ULAW, SBITS_ULAW },
	{ N_("IMA 4"), SBITS_IMA4, SBITS_IMA4 },
	{ N_("ADPCM"), SBITS_ADPCM, SBITS_ADPCM },
	{ N_("Float"), SBITS_FLOAT, SBITS_FLOAT },
	{ 0, 0, 0 }
};

const struct selection_double FrameRateSelection::frame_rates[] =
{
	{ "1", 1 },
	{ "5", 5 },
	{ "10", 10 },
	{ "12", 12 },
	{ "15", 15 },
	{ "23.97", 24 / 1.001 },
	{ "24", 24 },
	{ "25", 25 },
	{ "29.97", 30 / 1.001 },
	{ "30", 30 },
	{ "50", 50 },
	{ "59.94", 60 / 1.001 },
	{ "60", 60 },
	{ 0, 0 }
};

const struct selection_2int FrameSizeSelection::frame_sizes[] =
{
	{ "160 x 120", 160, 120 },
	{ "240 x 180", 240, 180 },
	{ "320 x 240", 320, 240 },
	{ "360 x 240", 360, 240 },
	{ "400 x 300", 400, 300 },
	{ "512 x 384", 512, 384 },
	{ "640 x 480", 640, 480 },
	{ "720 x 480", 720, 480 },
	{ "720 x 576", 720, 576 },
	{ "1280 x 720", 1280, 720 },
	{ "960 x 1080", 960, 1080 },
	{ "1920 x 1080", 1920, 1080 },
	{ "1920 x 1088", 1920, 1088 },
	{ 0, 0, 0 }
};

const struct selection_2double AspectRatioSelection::aspect_ratios[] =
{
	{ "Auto", -1., -1 },
	{ "3 : 2", 3.0, 2.0 },
	{ "4 : 3", 4.0, 3.0 },
	{ "16 : 9", 16.0, 9.0 },
	{ "2.10 : 1", 2.1, 1.0 },
	{ "2.20 : 1", 2.2, 1.0 },
	{ "2.25 : 1", 2.25, 1.0 },
	{ "2.30 : 1", 2.30, 1.0 },
	{ "21 : 9", 21, 9.0 },
	{ "2.66 : 1", 2.66, 1.0 },
	{ 0, 0, 0 }
};

SampleRateSelection::SampleRateSelection(int x, int y, BC_WindowBase *base, int *value)
 : Selection(x, y , base, sample_rates, value)
{
}

int SampleRateSelection::limits(int *rate)
{
	int result = 0;

	if(*rate < MIN_SAMPLE_RATE)
	{
		*rate = MIN_SAMPLE_RATE;
		result = -1;
	}

	if(*rate > MAX_SAMPLE_RATE)
	{
		*rate = MAX_SAMPLE_RATE;
		result = -1;
	}
	return result;
}

FrameRateSelection::FrameRateSelection(int x, int y, BC_WindowBase *base, double *value)
 : Selection(x, y , base, frame_rates, value)
{
}

int FrameRateSelection::limits(double *rate)
{
	int result = 0;
	double value = *rate;

	if(value < MIN_FRAME_RATE)
	{
		value = MIN_FRAME_RATE;
		result = -1;
	}

	if(value > MAX_FRAME_RATE)
	{
		value = MAX_FRAME_RATE;
		result = -1;
	}

	if(value > 29.5 && value < 30)
		value = (double)30000 / (double)1001;
	else
	if(value > 59.5 && value < 60)
		value = (double)60000 / (double)1001;
	else
	if(value > 23.5 && value < 24)
		value = (double)24000 / (double)1001;
	if(!result && !EQUIV(value, *rate))
		result = 1;
	*rate = value;
	return result;
}


FrameSizeSelection::FrameSizeSelection(int x1, int y1, int x2, int y2,
	BC_WindowBase *base, int *value1, int *value2, int swapvalues)
 : Selection(x1, y1, x2, y2, base, frame_sizes, value1, value2, 'x')
{
	if(swapvalues)
	{
		int x = x2 + SELECTION_TB_WIDTH +
		get_resources()->listbox_button[0]->get_w();
		base->add_subwindow(new SwapValues(x, y2, this, value1, value2));
	}
}

void FrameSizeSelection::update(int value1, int value2)
{
	firstbox->update(value1);
	BC_TextBox::update(value2);
}

void FrameSizeSelection::handle_swapvalues(int value1, int value2)
{
	update(value1, value2);
}

int FrameSizeSelection::limits(int *width, int *height)
{
	int v, result = 0;

	if(width)
	{
		v = *width;
		if(v > MAX_FRAME_WIDTH)
		{
			result = -1;
			v = MAX_FRAME_WIDTH;
		}
		if(v < MIN_FRAME_WIDTH)
		{
			result = -1;
			v = MIN_FRAME_WIDTH;
		}
		v &= ~1;
		if(!result && v != *width)
			result = 1;
		*width = v;
	}

	if(height)
	{
		v = *height;
		if(v < MIN_FRAME_HEIGHT)
		{
			result = -1;
			v = MIN_FRAME_WIDTH;
		}
		if(v > MAX_FRAME_WIDTH)
		{
			result = -1;
			v = MAX_FRAME_WIDTH;
		}
		v &= ~1;
		if(!result && v != *height)
			result = 1;
		*height = v;
	}
	return result;
}

AspectRatioSelection::AspectRatioSelection(int x1, int y1, int x2, int y2,
	BC_WindowBase *base, double *value1, double *value2, int *frame_w, int *frame_h)
 : Selection(x1, y1, x2, y2, base, aspect_ratios, value1, value2, ':')
{
	this->frame_w = frame_w;
	this->frame_h = frame_h;
}

void AspectRatioSelection::update_auto(double value1, double value2)
{
	if(value1 < 0 || value2 < 0)
		MWindow::create_aspect_ratio(value1, value2, *frame_w, *frame_h);

	*doublevalue = value2;
	*doublevalue2 = value1;

	firstbox->update(value1);
	BC_TextBox::update(value2);
}

Selection::Selection(int x, int y, BC_WindowBase *base,
	const struct selection_int items[], int *value, int options)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, "")
{
	BC_PopupMenu *popupmenu;
	int mxw = 0;

	firstbox = 0;

	if(options & SELECTION_VARWIDTH)
	{
		for(int i = 0; items[i].text; i++)
		{
			int w = base->get_text_width(MEDIUMFONT, _(items[i].text));
			if(w > mxw)
				mxw = w;
		}
		set_w(mxw + 10);
	}

	popupmenu = init_objects(x, y, base);
	intvalue = value;

	if(options & SELECTION_VARBITITEMS)
	{
		for(int i = 0; items[i].text; i++)
		{
			if(items[i].value & options)
				popupmenu->add_item(new SelectionItem(&items[i], this));
		}
	}
	else
	if(options & SELECTION_VARNUMITEMS)
	{
		for(int i = 0; items[i].text; i++)
		{
			if((1 << items[i].value) & options)
				popupmenu->add_item(new SelectionItem(&items[i], this));
		}
	}
	else
	{
		for(int i = 0; items[i].text; i++)
			popupmenu->add_item(new SelectionItem(&items[i], this));
	}
}

Selection::Selection(int x, int y, BC_WindowBase *base,
	const struct selection_2int items[], int *value, int options)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, "")
{
	BC_PopupMenu *popupmenu;
	int mxw = 0;

	firstbox = 0;

	if(options & SELECTION_VARWIDTH)
	{
		for(int i = 0; items[i].text; i++)
		{
			int w = base->get_text_width(MEDIUMFONT, _(items[i].text));
			if(w > mxw)
				mxw = w;
		}
		set_w(mxw + 10);
	}
	popupmenu = init_objects(x, y, base);
	intvalue = value;

	if(options & SELECTION_VARBITITEMS)
	{
		for(int i = 0; items[i].text; i++)
		{
			if(items[i].value1 & options)
				popupmenu->add_item(new SelectionItem(&items[i], 0, this));
		}
	}
	else
	if(options & SELECTION_VARNUMITEMS)
	{
		for(int i = 0; items[i].text; i++)
		{
			if((1 << items[i].value1) & options)
				popupmenu->add_item(new SelectionItem(&items[i], 0, this));
		}
	}
	else
	{
		for(int i = 0; items[i].text; i++)
			popupmenu->add_item(new SelectionItem(&items[i], 0, this));
	}
}

Selection::Selection(int x, int y, BC_WindowBase *base,
	const struct selection_double items[], double *value)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, "")
{
	BC_PopupMenu *popupmenu;

	firstbox = 0;
	popupmenu = init_objects(x, y, base);

	doublevalue = value;
	set_precision(2);

	for(int i = 0; items[i].text; i++)
		popupmenu->add_item(new SelectionItem(&items[i], this));
}

Selection::Selection(int x1, int y1, int x2, int y2, BC_WindowBase *base,
	const struct selection_2int items[], int *value1, int *value2, int separator)
 : BC_TextBox(x2, y2, SELECTION_TB_WIDTH, 1, "")
{
	BC_PopupMenu *popupmenu;

	base->add_subwindow(firstbox = new SelectionLeftBox(x1, y1, this));
	popupmenu = init_objects(x2, y2, base);

	intvalue = value2;
	intvalue2 = value1;

	if(separator && y1 == y2)
	{
		char stxt[2];
		int tw;

		stxt[0] = separator;
		stxt[1] = 0;
		tw = base->get_text_width(MEDIUMFONT, stxt, 1);

		int vh = (x2 - x1 - SELECTION_TB_WIDTH - tw) / 2;

		if(vh > 0)
			base->add_subwindow(new BC_Title(x1 + SELECTION_TB_WIDTH + vh, y1, stxt));
	}

	for(int i = 0; items[i].text; i++)
		popupmenu->add_item(new SelectionItem(&items[i], firstbox, this));
}

Selection::Selection(int x1, int y1, int x2, int y2, BC_WindowBase *base,
	const struct selection_2double items[], double *value1, double *value2, int separator)
 : BC_TextBox(x2, y2, SELECTION_TB_WIDTH, 1, "")
{
	BC_PopupMenu *popupmenu;

	base->add_subwindow(firstbox = new SelectionLeftBox(x1, y1, this));
	popupmenu = init_objects(x2, y2, base);

	doublevalue = value2;
	doublevalue2 = value1;

	set_precision(2);
	firstbox->set_precision(2);

	if(separator && y1 == y2)
	{
		char stxt[2];
		int tw;

		stxt[0] = separator;
		stxt[1] = 0;
		tw = base->get_text_width(MEDIUMFONT, stxt, 1);

		int vh = (x2 - x1 - SELECTION_TB_WIDTH - tw) / 2;

		if(vh > 0)
			base->add_subwindow(new BC_Title(x1 + SELECTION_TB_WIDTH + vh, y1, stxt));
	}

	for(int i = 0; items[i].text; i++)
		popupmenu->add_item(new SelectionItem(&items[i], firstbox, this));
}

void Selection::delete_subwindows()
{
	delete button;
	delete popupmenu;
	delete firstbox;
}

BC_PopupMenu *Selection::init_objects(int x, int y, BC_WindowBase *base)
{
	int x1 = x + get_w();
	int y1 = y + get_resources()->listbox_button[0]->get_h();

	base->add_subwindow(popupmenu = new BC_PopupMenu(x, y1, 0, "", POPUPMENU_USE_COORDS));
	base->add_subwindow(button = new SelectionButton(x1, y, popupmenu,
		get_resources()->listbox_button));

	intvalue = 0;
	intvalue2 = 0;
	doublevalue = 0;
	doublevalue2 = 0;
	current_int = 0;
	current_double = 0;
	current_2double = 0;
	return popupmenu;
}

int Selection::calculate_width()
{
	int w = get_w() + get_resources()->listbox_button[0]->get_w();

// If firstbox exists, then it is left or top
	if(firstbox && firstbox->get_y() == get_y())
		w += get_x() - firstbox->get_x();

	return w;
}

void Selection::disable(int option)
{
	defaultcolor = option;
	BC_TextBox::disable(option);
	if(!option)
		button->disable();
}

void Selection::enable(int option)
{
	BC_TextBox::enable();
	button->enable();
	if(option)
		BC_TextBox::disable(option);
}

void Selection::reposition_window(int x, int y)
{
	BC_TextBox::reposition_window(x, y);
	button->reposition_window(x + get_w(), y);
}

int Selection::handle_event()
{
	if(intvalue)
	{
		if(current_int)
		{
			*intvalue = current_int->value;
			current_int = 0;
		}
		else
			*intvalue = atoi(get_text());
	}
	if(intvalue2)
		*intvalue2 = atoi(firstbox->get_text());

	if(doublevalue)
	{
		if(current_double)
		{
			*doublevalue = current_double->value;
			current_double = 0;
		}
		else if(doublevalue2)
		{
			if(current_2double)
				current_2double = 0;
			else
			{
				*doublevalue = atof(get_text());
				*doublevalue2 = atof(firstbox->get_text());
			}
		}
		else
			*doublevalue = atof(get_text());
	}
	return 1;
}


SelectionButton::SelectionButton(int x, int y, BC_PopupMenu *popupmenu, VFrame **images)
 : BC_Button(x, y, images)
{
	this->popupmenu = popupmenu;
}

int SelectionButton::handle_event()
{
	popupmenu->activate_menu(1);
	return 1;
}


SelectionItem::SelectionItem(const struct selection_int *item, Selection *output)
 : BC_MenuItem(_(item->text))
{
	this->output = output;
	intitem = item;
	int2item = 0;
	doubleitem = 0;
	double2item = 0;
}

SelectionItem::SelectionItem(const struct selection_double *item, Selection *output)
 : BC_MenuItem(_(item->text))
{
	this->output = output;
	intitem = 0;
	int2item = 0;
	doubleitem = item;
	double2item = 0;
}

SelectionItem::SelectionItem(const struct selection_2int *item,
	BC_TextBox *output2, Selection *output1)
 : BC_MenuItem(_(item->text))
{
	this->output = output1;
	this->output2 = output2;
	int2item = item;
	intitem = 0;
	doubleitem = 0;
	double2item = 0;
}

SelectionItem::SelectionItem(const struct selection_2double *item,
	BC_TextBox *output2, Selection *output1)
 : BC_MenuItem(_(item->text))
{
	this->output = output1;
	this->output2 = output2;
	intitem = 0;
	int2item = 0;
	doubleitem = 0;
	double2item = item;
}

int SelectionItem::handle_event()
{
	if(intitem)
	{
		output->update(_(intitem->text));
		output->current_int = intitem;
	}
	if(doubleitem)
	{
		output->update(_(doubleitem->text));
		output->current_double = doubleitem;
	}
	if(int2item)
	{
		output->current_2int = int2item;
		if(output2)
		{
			output->update(int2item->value2);
			output2->update(int2item->value1);
		}
		else
			output->update(_(int2item->text));
	}
	if(double2item)
	{
		output->current_2double = double2item;
		output->update_auto(double2item->value1, double2item->value2);
	}
	output->handle_event();
	return 1;
}


SwapValues::SwapValues(int x, int y, FrameSizeSelection *output, int *value1, int *value2)
 : BC_Button(x, y, theme_global->get_image_set("swap_extents"))
{
	this->value1 = value1;
	this->value2 = value2;
	this->output = output;
	set_tooltip(_("Swap dimensions"));
}

int SwapValues::handle_event()
{
	int v = *value1;

	*value1 = *value2;
	*value2 = v;
	output->handle_swapvalues(*value1, *value2);
	return 0;
}


SelectionLeftBox::SelectionLeftBox(int x, int y, Selection *selection)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, "")
{
	this->selection = selection;
}

int SelectionLeftBox::handle_event()
{
	selection->handle_event();
	return 0;
}


SampleBitsSelection::SampleBitsSelection(int x, int y, BC_WindowBase *base, int *value, int bits)
 : Selection(x, y , base, sample_bits, value, bits | SELECTION_VARBITITEMS | SELECTION_VARWIDTH)
{
	disable(1);
}

int SampleBitsSelection::handle_event()
{
	if(current_2int)
	{
		*intvalue = current_2int->value2;
	}
	return 0;
}

void SampleBitsSelection::update_size(int size)
{
	for(int i = 0; sample_bits[i].value2; i++)
	{
		if(sample_bits[i].value2 == size)
			update(sample_bits[i].text);
	}
}

int SampleBitsSelection::samlpesize(int flag)
{
	if(flag & SBITS_LINEAR8)
		return 8;

	if(flag & SBITS_LINEAR16)
		return 16;

	if(flag & SBITS_LINEAR24)
		return 24;

	if(flag & SBITS_LINEAR32)
		return 32;

	return 16;
}

int SampleBitsSelection::sampleflag(int size)
{
	switch(size)
	{
	case 8:
		return SBITS_LINEAR8;

	case 16:
		return SBITS_LINEAR16;

	case 24:
		return SBITS_LINEAR16;

	case 32:
		return SBITS_LINEAR32;
	}
	return SBITS_LINEAR16;
}

const char *SampleBitsSelection::name(int size)
{
	for(int i = 0; sample_bits[i].text; i++)
		if(sample_bits[i].value2 == size)
			return _(sample_bits[i].text);
	return _("Unknown");
}
