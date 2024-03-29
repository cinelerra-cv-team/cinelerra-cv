
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

#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "resizetrackthread.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"


ResizeTrackThread::ResizeTrackThread(MWindow *mwindow, int track_number)
 : Thread()
{
	this->mwindow = mwindow;
	this->track_number = track_number;
	window = 0;
}

ResizeTrackThread::~ResizeTrackThread()
{
	if(window)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
	}

	Thread::join();
}

void ResizeTrackThread::start_window(Track *track, int track_number)
{
	this->track_number = track_number;
	w1 = w = track->track_w;
	h1 = h = track->track_h;
	w_scale = h_scale = 1;
	start();
}


void ResizeTrackThread::run()
{
	ResizeTrackWindow *window = this->window = 
		new ResizeTrackWindow(mwindow, 
			this,
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
	window->create_objects();
	int result = window->run_window();
	this->window = 0;
	delete window;


	if(!result)
	{
		Track *track = mwindow->edl->tracks->get_item_number(track_number);

		if(track)
		{
			FrameSizeSelection::limits(&w, &h);
			mwindow->resize_track(track, w, h);
		}
	}

	if(((w % 4) || 
		(h % 4)) && 
		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This track's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}
}




ResizeTrackWindow::ResizeTrackWindow(MWindow *mwindow, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_Window(MWindow::create_title(N_("Resize Track")),
				x - 320 / 2,
				y - get_resources()->ok_images[0]->get_h() + 100 / 2,
				350,
				get_resources()->ok_images[0]->get_h() + 100, 
				350,
				get_resources()->ok_images[0]->get_h() + 100, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ResizeTrackWindow::~ResizeTrackWindow()
{
}

#define TRWIN_INTERVAL 25
#define TRWIN_BOXLEFT  75

void ResizeTrackWindow::create_objects()
{
	const char *mul = "x";
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Size:")));
	x += TRWIN_BOXLEFT;

	add_subwindow(framesize_selection = new SetTrackFrameSize(x, y,
		x + SELECTION_TB_WIDTH + TRWIN_INTERVAL, y,
		this, &thread->w, &thread->h));
	framesize_selection->update(thread->w, thread->h);

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Scale:")));
	x += TRWIN_BOXLEFT;
	add_subwindow(w_scale = new ResizeTrackScaleW(this, 
		thread,
		x,
		y));
	int v = (TRWIN_INTERVAL - get_text_width(MEDIUMFONT, mul, 1)) / 2;
	x += SELECTION_TB_WIDTH + v;
	add_subwindow(new BC_Title(x, y, mul));
	x += TRWIN_INTERVAL - v;
	add_subwindow(h_scale = new ResizeTrackScaleH(this, 
		thread,
		x,
		y));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
}

void ResizeTrackWindow::update(int changed_scale, 
	int changed_size)
{
//printf("ResizeTrackWindow::update %d %d %d\n", changed_scale, changed_size, changed_all);
	if(changed_scale)
	{
		thread->w = (int)(thread->w1 * thread->w_scale);
		thread->h = (int)(thread->h1 * thread->h_scale);
		framesize_selection->update(thread->w, thread->h);
	}
	else
	if(changed_size)
	{
		thread->w_scale = (double)thread->w / thread->w1;
		w_scale->update((float)thread->w_scale);
		thread->h_scale = (double)thread->h / thread->h1;
		h_scale->update((float)thread->h_scale);
	}
}

ResizeTrackScaleW::ResizeTrackScaleW(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, (float)thread->w_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackScaleW::handle_event()
{
	thread->w_scale = atof(get_text());
	gui->update(1, 0);
	return 1;
}

ResizeTrackScaleH::ResizeTrackScaleH(ResizeTrackWindow *gui, 
	ResizeTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, (float)thread->h_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeTrackScaleH::handle_event()
{
	thread->h_scale = atof(get_text());
	gui->update(1, 0);
	return 1;
}

SetTrackFrameSize::SetTrackFrameSize(int x1, int y1, int x2, int y2,
	ResizeTrackWindow *base, int *value1, int *value2)
 : FrameSizeSelection(x1, y1, x2, y2, base, value1, value2)
{
	this->gui = base;
}

int SetTrackFrameSize::handle_event()
{
	Selection::handle_event();
	gui->update(0, 1);
	return 1;
}

