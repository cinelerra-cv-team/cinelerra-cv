
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

SelectionLeftBox::SelectionLeftBox(int x, int y, Selection *selection)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, "")
{
	this->selection = selection;
}

int SelectionLeftBox::handle_event()
{
	selection->handle_event();
}
