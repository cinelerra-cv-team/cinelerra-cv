
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

#include "cwindow.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "localsession.h"
#include "cwindowgui.h" 
#include "cpanel.h"
#include "patchbay.h"
#include "patchgui.h" 
#include "apatchgui.h"
#include "vpatchgui.h"
#include "track.h"
#include "maincursor.h"
#include "bcwindowbase.h"
#include "filexml.h"
#include "edlsession.h"
#include "autos.h"
//include "autoconf.h"

KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	key_show = 0;
	key_delete = 0;
	key_copy = 0;
	tan_smooth=tan_linear=tan_free = 0;
	tangent_mode_displayed = false;
}

KeyframePopup::~KeyframePopup()
{
	if(!tangent_mode_displayed)
	{
		delete tan_smooth;
		delete tan_linear;
		delete tan_free_t;
		delete tan_free  ;
		delete __hline__ ;
	}
}	// if they are currently displayed, the menu class will delete them automatically

void KeyframePopup::create_objects()
{
	add_item(key_show = new KeyframePopupShow(mwindow, this));
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));
	
    __hline__  = new BC_MenuItem("-");
    tan_smooth = new KeyframePopupTangentMode(mwindow, this, FloatAuto::SMOOTH);
    tan_linear = new KeyframePopupTangentMode(mwindow, this, FloatAuto::LINEAR);
    tan_free_t = new KeyframePopupTangentMode(mwindow, this, FloatAuto::TFREE );
    tan_free   = new KeyframePopupTangentMode(mwindow, this, FloatAuto::FREE  );
}

int KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
	handle_tangent_mode(0, 0);
	return 0;
}

int KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;
	handle_tangent_mode(autos, auto_keyframe);

	/* snap to cursor */
	double current_position = mwindow->edl->local_session->get_selectionstart(1);
	double new_position = keyframe_automation->track->from_units(keyframe_auto->position);
	mwindow->edl->local_session->set_selectionstart(new_position);
	mwindow->edl->local_session->set_selectionend(new_position);
	if (current_position != new_position)
	{
		mwindow->edl->local_session->set_selectionstart(new_position);
		mwindow->edl->local_session->set_selectionend(new_position);
		mwindow->gui->lock_window();
		mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);	
		mwindow->gui->unlock_window();
	}
	return 0;
}

void KeyframePopup::handle_tangent_mode(Autos *autos, Auto *auto_keyframe)
// determines the type of automation node. if floatauto, adds
// menu entries showing the tangent mode of the node
{
	if(!tangent_mode_displayed && autos && autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{ // append additional menu entries showing the tangent-mode
		add_item(__hline__);
		add_item(tan_smooth);
		add_item(tan_linear);
		add_item(tan_free_t);
		add_item(tan_free  );
		tangent_mode_displayed = true;
	}
	if(tangent_mode_displayed && (!autos || autos->get_type() != AUTOMATION_TYPE_FLOAT))
	{ // remove additional menu entries
		remove_item(tan_free  );
		remove_item(tan_free_t);
		remove_item(tan_linear);
		remove_item(tan_smooth);
		remove_item(__hline__ );
		tangent_mode_displayed = false;
	}
	if(tangent_mode_displayed && auto_keyframe)
	{ // set checkmarks to display current mode
		tan_smooth->toggle_mode((FloatAuto*)auto_keyframe);
		tan_linear->toggle_mode((FloatAuto*)auto_keyframe);
		tan_free_t->toggle_mode((FloatAuto*)auto_keyframe);
		tan_free  ->toggle_mode((FloatAuto*)auto_keyframe);
	}
}

KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupDelete::~KeyframePopupDelete()
{
}

int KeyframePopupDelete::handle_event()
{
	delete popup->keyframe_auto;
	mwindow->save_backup();
	mwindow->undo->update_undo(_("delete keyframe"), LOAD_ALL);

	mwindow->gui->update(0,
	        1,      // 1 for incremental drawing.  2 for full refresh
	        0,
	        0,
	        0,
            0,   
            0);
	mwindow->update_plugin_guis();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);

	return 1;
}

KeyframePopupShow::KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Show keyframe settings"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupShow::~KeyframePopupShow()
{
}

int KeyframePopupShow::handle_event()
{
	if (popup->keyframe_plugin)
	{
		mwindow->update_plugin_guis();
		mwindow->show_plugin(popup->keyframe_plugin);
	} else
	if (popup->keyframe_automation)
	{
/*

		mwindow->cwindow->gui->lock_window();
		int show_window = 1;
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->projector_autos ||
		   popup->keyframe_autos == (Autos *)popup->keyframe_automation->pzoom_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_PROJECTOR);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->camera_autos ||
		   popup->keyframe_autos == (Autos *)popup->keyframe_automation->czoom_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_CAMERA);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mode_autos)
		   
		{
			// no window to be shown
			show_window = 0;
			// first find the appropriate patchgui
			PatchBay *patchbay = mwindow->gui->patchbay;
			PatchGUI *patchgui = 0;
			for (int i = 0; i < patchbay->patches.total; i++)
				if (patchbay->patches.values[i]->track == popup->keyframe_automation->track)
					patchgui = patchbay->patches.values[i];		
			if (patchgui != 0)
			{
// FIXME: repositioning of the listbox needs support in guicast
//				int cursor_x = popup->get_relative_cursor_x();
//				int cursor_y = popup->get_relative_cursor_y();
//				vpatchgui->mode->reposition_window(cursor_x, cursor_y);


// Open the popup menu
				VPatchGUI *vpatchgui = (VPatchGUI *)patchgui;
				vpatchgui->mode->activate_menu();
			}
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mask_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_MASK);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pan_autos)
		   
		{
			// no window to be shown
			show_window = 0;
			// first find the appropriate patchgui
			PatchBay *patchbay = mwindow->gui->patchbay;
			PatchGUI *patchgui = 0;
			for (int i = 0; i < patchbay->patches.total; i++)
				if (patchbay->patches.values[i]->track == popup->keyframe_automation->track)
					patchgui = patchbay->patches.values[i];		
			if (patchgui != 0)
			{
// Open the popup menu at current mouse position
				APatchGUI *apatchgui = (APatchGUI *)patchgui;
				int cursor_x = popup->get_relative_cursor_x();
				int cursor_y = popup->get_relative_cursor_y();
				apatchgui->pan->activate(cursor_x, cursor_y);
			}
			

		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->fade_autos)
		   
		{
			// no window to be shown, so do nothing
			// IDEA: open window for fading
			show_window = 0;
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mute_autos)
		   
		{
			// no window to be shown, so do nothing
			// IDEA: directly switch
			show_window = 0;
		} else;
		

// ensure bringing to front
		if (show_window)
		{
			((CPanelToolWindow *)(mwindow->cwindow->gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]))->set_shown(0);
			((CPanelToolWindow *)(mwindow->cwindow->gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]))->set_shown(1);
		}
		mwindow->cwindow->gui->unlock_window();


*/
	}
	return 1;
}



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupCopy::~KeyframePopupCopy()
{
}

int KeyframePopupCopy::handle_event()
{
/*
	FIXME:
	we want to copy just keyframe under cursor, NOT all keyframes at this frame
	- very hard to do, so this is good approximation for now...
*/
	
#if 0
	if (popup->keyframe_automation)
	{
		FileXML file;
		EDL *edl = mwindow->edl;
		Track *track = popup->keyframe_automation->track;
		int64_t position = popup->keyframe_auto->position;
		AutoConf autoconf;
// first find out type of our auto
		autoconf.set_all(0);
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->projector_autos)
			autoconf.projector = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pzoom_autos)
			autoconf.pzoom = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->camera_autos)
			autoconf.camera = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->czoom_autos)
			autoconf.czoom = 1;		
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mode_autos)
		   	autoconf.mode = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mask_autos)
			autoconf.mask = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pan_autos)
			autoconf.pan = 1;		   
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->fade_autos)
			autoconf.fade = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mute_autos)
			autoconf.mute = 1;		


// now create a clipboard
		file.tag.set_title("AUTO_CLIPBOARD");
		file.tag.set_property("LENGTH", 0);
		file.tag.set_property("FRAMERATE", edl->session->frame_rate);
		file.tag.set_property("SAMPLERATE", edl->session->sample_rate);
		file.append_tag();
		file.append_newline();
		file.append_newline();

/*		track->copy_automation(position, 
			position, 
			&file,
			0,
			0);
			*/
		file.tag.set_title("TRACK");
// Video or audio
		track->save_header(&file);
		file.append_tag();
		file.append_newline();

		track->automation->copy(position, 
			position, 
			&file,
			0,
			0,
			&autoconf);
		
		
		
		file.tag.set_title("/TRACK");
		file.append_tag();
		file.append_newline();
		file.append_newline();
		file.append_newline();
		file.append_newline();



		file.tag.set_title("/AUTO_CLIPBOARD");
		file.append_tag();
		file.append_newline();
		file.terminate_string();

		mwindow->gui->lock_window();
		mwindow->gui->get_clipboard()->to_clipboard(file.string, 
			strlen(file.string), 
			SECONDARY_SELECTION);
		mwindow->gui->unlock_window();

	} else
#endif
		mwindow->copy_automation();
	return 1;
}



KeyframePopupTangentMode::KeyframePopupTangentMode(
	MWindow *mwindow, 
	KeyframePopup *popup, 
	int tangent_mode)
 : BC_MenuItem( get_labeltext(tangent_mode))
{
    this->tangent_mode = tangent_mode;
    this->mwindow = mwindow;
    this->popup = popup;
}

KeyframePopupTangentMode::~KeyframePopupTangentMode() { }


const char* KeyframePopupTangentMode::get_labeltext(int mode)
{
	switch(mode)
	{   case FloatAuto::SMOOTH: return _("smooth curve");
	    case FloatAuto::LINEAR: return _("linear segments");
	    case FloatAuto::TFREE:  return _("tangent edit");
	    case FloatAuto::FREE:   return _("disjoint edit");
	}
	return "misconfigured";
}


void KeyframePopupTangentMode::toggle_mode(FloatAuto *keyframe)
{
	set_checked(tangent_mode == keyframe->tangent_mode);
}


int KeyframePopupTangentMode::handle_event()
{
	if (popup->keyframe_autos && 
	    popup->keyframe_autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{
		((FloatAuto*)popup->keyframe_auto)->
			change_tangent_mode((FloatAuto::t_mode)tangent_mode);
		
		// if we switched to some "auto" mode, this may imply a
		// real change to parameters, so this needs to be undoable...
		mwindow->save_backup();
		mwindow->undo->update_undo(_("change keyframe tangent mode"), LOAD_ALL);
		
		mwindow->gui->update(0, 1, 0,0,0,0,0); // incremental redraw for canvas
		mwindow->cwindow->update(0,0, 1, 0,0); // redraw tool window in compositor
		mwindow->update_plugin_guis();
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_EDL);
	}
	return 1;
}
