
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

#include "assets.h"
#include "batchrender.h"
#include "bcsignals.h"
#include "cache.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "featheredits.h"
#include "filesystem.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "levelwindow.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "menuaeffects.h"
#include "menuveffects.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "quit.h"
#include "record.h"
#include "render.h"
#include "savefile.h"
#include "setformat.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "viewmenu.h"
#include "zoombar.h"
#include "exportedl.h"

#include <string.h>


MainMenu::MainMenu(MWindow *mwindow, MWindowGUI *gui)
 : BC_MenuBar(0, 0, gui->get_w())
{
	this->gui = gui;
	this->mwindow = mwindow; 
}

MainMenu::~MainMenu()
{
}

int MainMenu::create_objects()
{
	BC_Menu *viewmenu, *windowmenu, *settingsmenu, *trackmenu;
	PreferencesMenuitem *preferences;

	recent_load = new BC_RecentList("PATH", mwindow->defaults);
SET_TRACE

	add_menu(filemenu = new BC_Menu(_("File")));
	filemenu->add_item(new_project = new New(mwindow));
	new_project->create_objects();

SET_TRACE
// file loaders
	filemenu->add_item(load_file = new Load(mwindow, this));
	load_file->create_objects();

SET_TRACE
// new and load can be undone so no need to prompt save
	Save *save;                   //  affected by saveas
	filemenu->add_item(save = new Save(mwindow));
	SaveAs *saveas;
	filemenu->add_item(saveas = new SaveAs(mwindow));
	save->create_objects(saveas);
	saveas->set_mainmenu(this);
	filemenu->add_item(record = new RecordMenuItem(mwindow));

	filemenu->add_item(render = new RenderItem(mwindow));
	filemenu->add_item(new ExportEDLItem(mwindow));
	filemenu->add_item(new BatchRenderMenuItem(mwindow));
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(quit_program = new Quit(mwindow));
	quit_program->create_objects(save);
	filemenu->add_item(new DumpEDL(mwindow));
	filemenu->add_item(new DumpPlugins(mwindow));
	filemenu->add_item(new LoadBackup(mwindow));
	filemenu->add_item(new SaveBackup(mwindow));

	BC_Menu *editmenu;
	add_menu(editmenu = new BC_Menu(_("Edit")));
	editmenu->add_item(undo = new Undo(mwindow));
	editmenu->add_item(redo = new Redo(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new Cut(mwindow));
	editmenu->add_item(new Copy(mwindow));
	editmenu->add_item(new Paste(mwindow));
	editmenu->add_item(new Clear(mwindow));
	editmenu->add_item(new PasteSilence(mwindow));
	editmenu->add_item(new MuteSelection(mwindow));
	editmenu->add_item(new TrimSelection(mwindow));
	editmenu->add_item(new SelectAll(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new ClearLabels(mwindow));

	BC_Menu *keyframemenu;
	add_menu(keyframemenu = new BC_Menu(_("Keyframes")));
	keyframemenu->add_item(new CutKeyframes(mwindow));
	keyframemenu->add_item(new CopyKeyframes(mwindow));
	keyframemenu->add_item(new PasteKeyframes(mwindow));
	keyframemenu->add_item(new ClearKeyframes(mwindow));
	keyframemenu->add_item(new StraightenKeyframes(mwindow));
	keyframemenu->add_item(new BC_MenuItem("-"));
	keyframemenu->add_item(new CopyDefaultKeyframe(mwindow));
	keyframemenu->add_item(new PasteDefaultKeyframe(mwindow));




	add_menu(audiomenu = new BC_Menu(_("Audio")));
	audiomenu->add_item(new AddAudioTrack(mwindow));
	audiomenu->add_item(new DefaultATransition(mwindow));
	audiomenu->add_item(new MapAudio1(mwindow));
	audiomenu->add_item(new MapAudio2(mwindow));
	audiomenu->add_item(aeffects = new MenuAEffects(mwindow));

	add_menu(videomenu = new BC_Menu(_("Video")));
	videomenu->add_item(new AddVideoTrack(mwindow));
	videomenu->add_item(new DefaultVTransition(mwindow));
	videomenu->add_item(veffects = new MenuVEffects(mwindow));

	add_menu(trackmenu = new BC_Menu(_("Tracks")));
	trackmenu->add_item(new MoveTracksUp(mwindow));
	trackmenu->add_item(new MoveTracksDown(mwindow));
	trackmenu->add_item(new DeleteTracks(mwindow));
	trackmenu->add_item(new DeleteTrack(mwindow));
	trackmenu->add_item(new ConcatenateTracks(mwindow));

	add_menu(settingsmenu = new BC_Menu(_("Settings")));

	settingsmenu->add_item(new SetFormat(mwindow));
	settingsmenu->add_item(preferences = new PreferencesMenuitem(mwindow));
	mwindow->preferences_thread = preferences->thread;
	settingsmenu->add_item(labels_follow_edits = new LabelsFollowEdits(mwindow));
	settingsmenu->add_item(plugins_follow_edits = new PluginsFollowEdits(mwindow));
	settingsmenu->add_item(cursor_on_frames = new CursorOnFrames(mwindow));
	settingsmenu->add_item(new SaveSettingsNow(mwindow));
	settingsmenu->add_item(loop_playback = new LoopPlayback(mwindow));
	settingsmenu->add_item(new SetBRenderStart(mwindow));
// set scrubbing speed
//	ScrubSpeed *scrub_speed;
//	settingsmenu->add_item(scrub_speed = new ScrubSpeed(mwindow));
//	if(mwindow->edl->session->scrub_speed == .5) 
//		scrub_speed->set_text(_("Fast Shuttle"));






	add_menu(viewmenu = new BC_Menu(_("View")));
	viewmenu->add_item(show_assets = new ShowAssets(mwindow, "0"));
	viewmenu->add_item(show_titles = new ShowTitles(mwindow, "1"));
	viewmenu->add_item(show_transitions = new ShowTransitions(mwindow, "2"));
	viewmenu->add_item(fade_automation = new ShowAutomation(mwindow, _("Fade"), "3", AUTOMATION_FADE));
	viewmenu->add_item(mute_automation = new ShowAutomation(mwindow, _("Mute"), "4", AUTOMATION_MUTE));
	viewmenu->add_item(mode_automation = new ShowAutomation(mwindow, _("Overlay mode"), "5", AUTOMATION_MODE));
	viewmenu->add_item(pan_automation = new ShowAutomation(mwindow, _("Pan"), "6", AUTOMATION_PAN));
	viewmenu->add_item(plugin_automation = new PluginAutomation(mwindow, "7"));
	viewmenu->add_item(mask_automation = new ShowAutomation(mwindow, _("Mask"), "8", AUTOMATION_MASK));
	viewmenu->add_item(camera_x = new ShowAutomation(mwindow, _("Camera X"), "", AUTOMATION_CAMERA_X));
	viewmenu->add_item(camera_y = new ShowAutomation(mwindow, _("Camera Y"), "", AUTOMATION_CAMERA_Y));
	viewmenu->add_item(camera_z = new ShowAutomation(mwindow, _("Camera Z"), "", AUTOMATION_CAMERA_Z));
	viewmenu->add_item(project_x = new ShowAutomation(mwindow, _("Projector X"), "", AUTOMATION_PROJECTOR_X));
	viewmenu->add_item(project_y = new ShowAutomation(mwindow, _("Projector Y"), "", AUTOMATION_PROJECTOR_Y));
	viewmenu->add_item(project_z = new ShowAutomation(mwindow, _("Projector Z"), "", AUTOMATION_PROJECTOR_Z));


	add_menu(windowmenu = new BC_Menu(_("Window")));
	windowmenu->add_item(show_vwindow = new ShowVWindow(mwindow));
	windowmenu->add_item(show_awindow = new ShowAWindow(mwindow));
	windowmenu->add_item(show_cwindow = new ShowCWindow(mwindow));
	windowmenu->add_item(show_gwindow = new ShowGWindow(mwindow));
	windowmenu->add_item(show_lwindow = new ShowLWindow(mwindow));
	windowmenu->add_item(new TileWindows(mwindow));

SET_TRACE
	return 0;
}

int MainMenu::load_defaults(BC_Hash *defaults)
{
	init_loads(defaults);
	init_aeffects(defaults);
	init_veffects(defaults);
	return 0;
}

void MainMenu::update_toggles(int use_lock)
{
	if(use_lock) lock_window("MainMenu::update_toggles");
	labels_follow_edits->set_checked(mwindow->edl->session->labels_follow_edits);
	plugins_follow_edits->set_checked(mwindow->edl->session->plugins_follow_edits);
	cursor_on_frames->set_checked(mwindow->edl->session->cursor_on_frames);
	loop_playback->set_checked(mwindow->edl->local_session->loop_playback);
	show_titles->set_checked(mwindow->edl->session->show_titles);
	show_transitions->set_checked(mwindow->edl->session->auto_conf->transitions);
	fade_automation->update_toggle();
	mute_automation->update_toggle();
	pan_automation->update_toggle();
	camera_x->update_toggle();
	camera_y->update_toggle();
	camera_z->update_toggle();
	project_x->update_toggle();
	project_y->update_toggle();
	project_z->update_toggle();
	plugin_automation->set_checked(mwindow->edl->session->auto_conf->plugins);
	mode_automation->update_toggle();
	mask_automation->update_toggle();
	if(use_lock) mwindow->gui->unlock_window();
}

int MainMenu::save_defaults(BC_Hash *defaults)
{
	save_aeffects(defaults);
	save_veffects(defaults);
	return 0;
}





int MainMenu::quit()
{
	quit_program->handle_event();
	return 0;
}





// ================================== load most recent

int MainMenu::init_aeffects(BC_Hash *defaults)
{
	total_aeffects = defaults->get("TOTAL_AEFFECTS", 0);
	
	char string[1024], title[1024];
	if(total_aeffects) audiomenu->add_item(new BC_MenuItem("-"));
	
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->get(string, title);
		audiomenu->add_item(aeffect[i] = new MenuAEffectItem(aeffects, title));
	}
	return 0;
}

int MainMenu::init_veffects(BC_Hash *defaults)
{
	total_veffects = defaults->get("TOTAL_VEFFECTS", 0);
	
	char string[1024], title[1024];
	if(total_veffects) videomenu->add_item(new BC_MenuItem("-"));
	
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->get(string, title);
		videomenu->add_item(veffect[i] = new MenuVEffectItem(veffects, title));
	}
	return 0;
}

int MainMenu::init_loads(BC_Hash *defaults)
{
//printf("MainMenu::init_loads 1\n");
//printf("MainMenu::init_loads 1\n");
	char string[BCTEXTLEN], path[BCTEXTLEN], filename[BCTEXTLEN];
//printf("MainMenu::init_loads 1\n");
	FileSystem dir;
	
	recent_load->load_items();

	int total_loads = recent_load->items.total;
//printf("MainMenu::init_loads 2\n");
	if(total_loads > 0) filemenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_loads; i++)
	{
		char *path = recent_load->items.values[i]->get_text();

		filemenu->add_item(load[i] = new LoadPrevious(mwindow));
//printf("MainMenu::init_loads 5\n");
		dir.extract_name(filename, path, 0);
//printf("MainMenu::init_loads 6\n");
		load[i]->set_text(filename);
//printf("MainMenu::init_loads 7\n");
		load[i]->set_path(path);
//printf("MainMenu::init_loads 8\n");
	}
//printf("MainMenu::init_loads 9\n");
	return 0;
}

// ============================ save most recent

int MainMenu::save_aeffects(BC_Hash *defaults)
{
	defaults->update("TOTAL_AEFFECTS", total_aeffects);
	char string[1024];
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->update(string, aeffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_veffects(BC_Hash *defaults)
{
	defaults->update("TOTAL_VEFFECTS", total_veffects);
	char string[1024];
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->update(string, veffect[i]->get_text());
	}
	return 0;
}

// =================================== add most recent

int MainMenu::add_aeffect(char *title)
{
// add bar for first effect
	if(total_aeffects == 0)
	{
		audiomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_aeffects; i++)
	{
		if(!strcmp(aeffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				aeffect[j]->set_text(aeffect[j - 1]->get_text());
			}
			aeffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_aeffects < TOTAL_EFFECTS)
	{
		audiomenu->add_item(aeffect[total_aeffects] = new MenuAEffectItem(aeffects, ""));
		total_aeffects++;
	}

// cycle effect down
	for(int i = total_aeffects - 1; i > 0; i--)
	{
	// set menu item text
		aeffect[i]->set_text(aeffect[i - 1]->get_text());
	}

// set up the new effect
	aeffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_veffect(char *title)
{
// add bar for first effect
	if(total_veffects == 0)
	{
		videomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_veffects; i++)
	{
		if(!strcmp(veffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				veffect[j]->set_text(veffect[j - 1]->get_text());
			}
			veffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_veffects < TOTAL_EFFECTS)
	{
		videomenu->add_item(veffect[total_veffects] = new MenuVEffectItem(veffects, ""));
		total_veffects++;
	}

// cycle effect down
	for(int i = total_veffects - 1; i > 0; i--)
	{
// set menu item text
		veffect[i]->set_text(veffect[i - 1]->get_text());
	}

// set up the new effect
	veffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_load(char *new_path)
{
	char filename[BCTEXTLEN];
	FileSystem dir;

	int total_loads = recent_load->items.total;

	if(total_loads == 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
	}

	int new_total = recent_load->add_item(NULL, new_path);

	if (new_total > total_loads) {
		// just create a new item if there is room for it
		int i = new_total - 1;
		load[i] = new LoadPrevious(mwindow);
		dir.extract_name(filename, new_path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(new_path);
		filemenu->add_item(load[i]);
	}

	// reassign the paths to adjust for the shift down
	for(int i = 0; i < new_total; i++) {
		char *path = recent_load->items.values[i]->get_text();
		dir.extract_name(filename, path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(path);
	}

	return 0;
}








// ================================== menu items


DumpCICache::DumpCICache(MWindow *mwindow)
 : BC_MenuItem(_("Dump CICache"))
{ this->mwindow = mwindow; }

int DumpCICache::handle_event()
{
//	mwindow->cache->dump();
	return 1;
}

DumpEDL::DumpEDL(MWindow *mwindow)
 : BC_MenuItem(_("Dump EDL"))
{ 
	this->mwindow = mwindow;
}

int DumpEDL::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->edl->dump();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}

DumpPlugins::DumpPlugins(MWindow *mwindow)
 : BC_MenuItem(_("Dump Plugins"))
{ 
	this->mwindow = mwindow;
}

int DumpPlugins::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->dump_plugins();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}


DumpAssets::DumpAssets(MWindow *mwindow)
 : BC_MenuItem(_("Dump Assets"))
{ this->mwindow = mwindow; }

int DumpAssets::handle_event()
{
	mwindow->assets->dump();
	return 1;
}

// ================================================= edit

Undo::Undo(MWindow *mwindow) : BC_MenuItem(_("Undo"), "z", 'z') 
{ 
	this->mwindow = mwindow; 
}
int Undo::handle_event()
{ 
	mwindow->undo_entry(mwindow->gui);
	return 1;
}
int Undo::update_caption(const char *new_caption)
{
	char string[1024];
	sprintf(string, _("Undo %s"), new_caption);
	set_text(string);
	return 0;
}


Redo::Redo(MWindow *mwindow) : BC_MenuItem(_("Redo"), "Shift+Z", 'Z') 
{ 
	set_shift(1); 
	this->mwindow = mwindow; 
}

int Redo::handle_event()
{ 
	mwindow->redo_entry(mwindow->gui);

	return 1;
}
int Redo::update_caption(const char *new_caption)
{
	char string[1024];
	sprintf(string, _("Redo %s"), new_caption);
	set_text(string);
	return 0;
}

CutKeyframes::CutKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Cut keyframes"), "Shift-X", 'X')
{ 
	set_shift(); 
	this->mwindow = mwindow; 
}

int CutKeyframes::handle_event()
{
	mwindow->cut_automation(); 
	return 0;
}

CopyKeyframes::CopyKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Copy keyframes"), "Shift-C", 'C')
{ 
	set_shift(); 
	this->mwindow = mwindow; 
}

int CopyKeyframes::handle_event()
{
	mwindow->copy_automation();
	return 1;
}

PasteKeyframes::PasteKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Paste keyframes"), "Shift-V", 'V')
{
	set_shift(); 
	this->mwindow = mwindow; 
}

int PasteKeyframes::handle_event()
{
	mwindow->paste_automation(); 
	return 1;
}

ClearKeyframes::ClearKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Clear keyframes"), "Shift-Del", DELETE)
{
	set_shift(); 
	this->mwindow = mwindow; 
}

int ClearKeyframes::handle_event()
{
	mwindow->clear_automation();
	return 1;
}


StraightenKeyframes::StraightenKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Straighten curves"))
{
	this->mwindow = mwindow; 
}

int StraightenKeyframes::handle_event()
{
	mwindow->straighten_automation();
	return 1;
}








CutDefaultKeyframe::CutDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Cut default keyframe"), "Alt-X", 'X')
{ 
	set_alt(); 
	this->mwindow = mwindow; 
}

int CutDefaultKeyframe::handle_event()
{
	mwindow->cut_default_keyframe(); 
	return 1;
}

CopyDefaultKeyframe::CopyDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Copy default keyframe"), "Alt-c", 'c')
{ 
	set_alt(); 
	this->mwindow = mwindow; 
}

int CopyDefaultKeyframe::handle_event()
{
	mwindow->copy_default_keyframe();
	return 1;
}

PasteDefaultKeyframe::PasteDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Paste default keyframe"), "Alt-v", 'v')
{
	set_alt(); 
	this->mwindow = mwindow; 
}

int PasteDefaultKeyframe::handle_event()
{
	mwindow->paste_default_keyframe(); 
	return 1;
}

ClearDefaultKeyframe::ClearDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Clear default keyframe"), "Alt-Del", DELETE)
{
	set_alt(); 
	this->mwindow = mwindow; 
}

int ClearDefaultKeyframe::handle_event()
{
	mwindow->clear_default_keyframe();
	return 1;
}

Cut::Cut(MWindow *mwindow)
 : BC_MenuItem(_("Cut"), "x", 'x') 
{
	this->mwindow = mwindow; 
}

int Cut::handle_event()
{
	mwindow->cut();
	return 1;
}

Copy::Copy(MWindow *mwindow)
 : BC_MenuItem(_("Copy"), "c", 'c') 
{
	this->mwindow = mwindow; 
}

int Copy::handle_event()
{
	mwindow->copy();
	return 1;
}

Paste::Paste(MWindow *mwindow)
 : BC_MenuItem(_("Paste"), "v", 'v') 
{
	this->mwindow = mwindow; 
}

int Paste::handle_event()
{
	mwindow->paste();
	return 1;
}

Clear::Clear(MWindow *mwindow)
 : BC_MenuItem(_("Clear"), "Del", DELETE) 
{
	this->mwindow = mwindow; 
}

int Clear::handle_event()
{
	mwindow->cwindow->gui->lock_window("Clear::handle_event");
	mwindow->clear_entry();
	mwindow->cwindow->gui->unlock_window();
	return 1;
}

PasteSilence::PasteSilence(MWindow *mwindow)
 : BC_MenuItem(_("Paste silence"), "Shift+Space", ' ')
{ 
	this->mwindow = mwindow; 
	set_shift(); 
}

int PasteSilence::handle_event()
{ 
	mwindow->paste_silence();
	return 1;
}

SelectAll::SelectAll(MWindow *mwindow)
 : BC_MenuItem(_("Select All"), "a", 'a')
{ 
	this->mwindow = mwindow; 
}

int SelectAll::handle_event()
{
	mwindow->select_all();
	return 1;
}

ClearLabels::ClearLabels(MWindow *mwindow) : BC_MenuItem(_("Clear labels")) 
{ 
	this->mwindow = mwindow; 
}

int ClearLabels::handle_event()
{
	mwindow->clear_labels();
	return 1;
}

MuteSelection::MuteSelection(MWindow *mwindow)
 : BC_MenuItem(_("Mute Region"), "m", 'm')
{
	this->mwindow = mwindow;
}

int MuteSelection::handle_event()
{
	mwindow->mute_selection();
	return 1;
}


TrimSelection::TrimSelection(MWindow *mwindow)
 : BC_MenuItem(_("Trim Selection"))
{
	this->mwindow = mwindow;
}

int TrimSelection::handle_event()
{
	mwindow->trim_selection();
	return 1;
}












// ============================================= audio

AddAudioTrack::AddAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), "t", 't')
{
	this->mwindow = mwindow;
}

int AddAudioTrack::handle_event()
{
	mwindow->add_audio_track_entry(0, 0);
	return 1;
}

DeleteAudioTrack::DeleteAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteAudioTrack::handle_event()
{
	return 1;
}

DefaultATransition::DefaultATransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), "u", 'u')
{
	this->mwindow = mwindow;
}

int DefaultATransition::handle_event()
{
	mwindow->paste_audio_transition();
	return 1;
}


MapAudio1::MapAudio1(MWindow *mwindow)
 : BC_MenuItem(_("Map 1:1"))
{
	this->mwindow = mwindow;
}

int MapAudio1::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_1_TO_1);
	return 1;
}

MapAudio2::MapAudio2(MWindow *mwindow)
 : BC_MenuItem(_("Map 5.1:2"))
{
	this->mwindow = mwindow;
}

int MapAudio2::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_5_1_TO_2);
	return 1;
}




// ============================================= video


AddVideoTrack::AddVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), "Shift-T", 'T')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddVideoTrack::handle_event()
{
	mwindow->add_video_track_entry();
	return 1;
}


DeleteVideoTrack::DeleteVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteVideoTrack::handle_event()
{
	return 1;
}



ResetTranslation::ResetTranslation(MWindow *mwindow)
 : BC_MenuItem(_("Reset Translation"))
{
	this->mwindow = mwindow;
}

int ResetTranslation::handle_event()
{
	return 1;
}



DefaultVTransition::DefaultVTransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), "Shift-U", 'U')
{
	set_shift();
	this->mwindow = mwindow;
}

int DefaultVTransition::handle_event()
{
	mwindow->paste_video_transition();
	return 1;
}














// ============================================ settings

DeleteTracks::DeleteTracks(MWindow *mwindow)
 : BC_MenuItem(_("Delete tracks"))
{
	this->mwindow = mwindow;
}

int DeleteTracks::handle_event()
{
	mwindow->delete_tracks();
	return 1;
}

DeleteTrack::DeleteTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete last track"), "d", 'd')
{
	this->mwindow = mwindow;
}

int DeleteTrack::handle_event()
{
	mwindow->delete_track();
	return 1;
}

MoveTracksUp::MoveTracksUp(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks up"), "Shift+Up", UP)
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksUp::handle_event()
{
	mwindow->move_tracks_up();
	return 1;
}

MoveTracksDown::MoveTracksDown(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks down"), "Shift+Down", DOWN)
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksDown::handle_event()
{
	mwindow->move_tracks_down();
	return 1;
}




ConcatenateTracks::ConcatenateTracks(MWindow *mwindow)
 : BC_MenuItem(_("Concatenate tracks"))
{
	set_shift(); 
	this->mwindow = mwindow;
}

int ConcatenateTracks::handle_event()
{
	mwindow->concatenate_tracks();
	return 1;
}





LoopPlayback::LoopPlayback(MWindow *mwindow)
 : BC_MenuItem(_("Loop Playback"), "Shift+L", 'L')
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->local_session->loop_playback);
	set_shift();
}

int LoopPlayback::handle_event()
{
	mwindow->toggle_loop_playback();
	set_checked(mwindow->edl->local_session->loop_playback);
	return 1;
}





SetBRenderStart::SetBRenderStart(MWindow *mwindow)
 : BC_MenuItem(_("Set background render"))
{
	this->mwindow = mwindow;
}

int SetBRenderStart::handle_event()
{
	mwindow->set_brender_start();
	return 1;
}







LabelsFollowEdits::LabelsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit labels")) 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->labels_follow_edits);
}

int LabelsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->set_labels_follow_edits(get_checked());
	return 1;
}




PluginsFollowEdits::PluginsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit effects")) 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->plugins_follow_edits);
}

int PluginsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->plugins_follow_edits = get_checked(); 
	return 1;
}




AutosFollowEdits::AutosFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Autos follow edits")) 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->autos_follow_edits);
}

int AutosFollowEdits::handle_event()
{ 
	mwindow->edl->session->autos_follow_edits ^= 1; 
	set_checked(!get_checked());
	return 1;
}


CursorOnFrames::CursorOnFrames(MWindow *mwindow)
 : BC_MenuItem(_("Align cursor on frames")) 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->cursor_on_frames);
}

int CursorOnFrames::handle_event()
{
	mwindow->edl->session->cursor_on_frames = !mwindow->edl->session->cursor_on_frames; 
	set_checked(mwindow->edl->session->cursor_on_frames);
	return 1;
}


ScrubSpeed::ScrubSpeed(MWindow *mwindow) : BC_MenuItem(_("Slow Shuttle"))
{
	this->mwindow = mwindow;
}

int ScrubSpeed::handle_event()
{
	if(mwindow->edl->session->scrub_speed == .5)
	{
		mwindow->edl->session->scrub_speed = 2;
		set_text(_("Slow Shuttle"));
	}
	else
	{
		mwindow->edl->session->scrub_speed = .5;
		set_text(_("Fast Shuttle"));
	}
	return 1;
}

SaveSettingsNow::SaveSettingsNow(MWindow *mwindow) : BC_MenuItem(_("Save settings now")) 
{ 
	this->mwindow = mwindow; 
}

int SaveSettingsNow::handle_event()
{
	mwindow->save_defaults();
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved settings."));
	return 1;
}



// ============================================ window





ShowVWindow::ShowVWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Viewer"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_vwindow);
}
int ShowVWindow::handle_event()
{
	mwindow->show_vwindow();
	return 1;
}

ShowAWindow::ShowAWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Resources"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_awindow);
}
int ShowAWindow::handle_event()
{
	mwindow->show_awindow();
	return 1;
}

ShowCWindow::ShowCWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Compositor"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_cwindow);
}
int ShowCWindow::handle_event()
{
	mwindow->show_cwindow();
	return 1;
}


ShowGWindow::ShowGWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Overlays"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_gwindow);
}
int ShowGWindow::handle_event()
{
	mwindow->show_gwindow();
	return 1;
}


ShowLWindow::ShowLWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Levels"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_lwindow);
}
int ShowLWindow::handle_event()
{
	mwindow->show_lwindow();
	return 1;
}

TileWindows::TileWindows(MWindow *mwindow)
 : BC_MenuItem(_("Default positions"))
{
	this->mwindow = mwindow;
}
int TileWindows::handle_event()
{
	mwindow->tile_windows();
	return 1;
}


