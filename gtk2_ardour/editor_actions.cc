/*
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2006-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2006-2007 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007-2014 David Robillard <d@drobilla.net>
 * Copyright (C) 2012-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2013-2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2016 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2014-2019 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015 André Nusser <andre.nusser@googlemail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gio/gio.h>
#include <ytk/gtkiconfactory.h>

#include "pbd/file_utils.h"

#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/utils.h"

#include "ardour/filesystem_paths.h"
#include "ardour/profile.h"
#include "ardour/session.h"
#include "ardour/types.h"

#include "temporal/bbt_time.h"

#include "canvas/canvas.h"
#include "canvas/pixbuf.h"

#include "LuaBridge/LuaBridge.h"

#include "actions.h"
#include "ardour_ui.h"
#include "control_point.h"
#include "editing.h"
#include "editor.h"
#include "gui_thread.h"
#include "luainstance.h"
#include "main_clock.h"
#include "time_axis_view.h"
#include "ui_config.h"
#include "utils.h"
#include "pbd/i18n.h"
#include "audio_time_axis.h"
#include "editor_group_tabs.h"
#include "editor_routes.h"
#include "editor_regions.h"
#include "midi_region_view.h"

using namespace Gtk;
using namespace Glib;
using namespace std;
using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace PBD;
using namespace Editing;

using Gtkmm2ext::Bindings;

RefPtr<Action>
Editor::register_region_action (RefPtr<ActionGroup> group, RegionActionTarget tgt, char const * name, char const * label, sigc::slot<void> slot)
{
	RefPtr<Action> act = ActionManager::register_action (group, name, label, slot);
	ActionManager::session_sensitive_actions.push_back (act);
	region_action_map.insert (make_pair<string,RegionAction> (name, RegionAction (act,tgt)));
	return act;
}

void
Editor::register_toggle_region_action (RefPtr<ActionGroup> group, RegionActionTarget tgt, char const * name, char const * label, sigc::slot<void> slot)
{
	RefPtr<Action> act = ActionManager::register_toggle_action (group, name, label, slot);
	ActionManager::session_sensitive_actions.push_back (act);
	region_action_map.insert (make_pair<string,RegionAction> (name, RegionAction (act,tgt)));
}

void
Editor::register_actions ()
{
	RefPtr<Action> act;

	editor_actions = ActionManager::create_action_group (own_bindings, X_("Editor"));
	editor_menu_actions = ActionManager::create_action_group (own_bindings, X_("EditorMenu"));

	/* non-operative menu items for menu bar */

	ActionManager::register_action (editor_menu_actions, X_("AlignMenu"), _("Align"));
	ActionManager::register_action (editor_menu_actions, X_("Autoconnect"), _("Autoconnect"));
	ActionManager::register_action (editor_menu_actions, X_("AutomationMenu"), _("Automation"));
	ActionManager::register_action (editor_menu_actions, X_("Crossfades"), _("Crossfades"));
	ActionManager::register_action (editor_menu_actions, X_("Edit"), _("Edit"));
	ActionManager::register_action (editor_menu_actions, X_("Tempo"), _("Tempo"));
	ActionManager::register_action (editor_menu_actions, X_("EditCursorMovementOptions"), _("Move Selected Marker"));
	ActionManager::register_action (editor_menu_actions, X_("EditSelectRangeOptions"), _("Select Range Operations"));
	ActionManager::register_action (editor_menu_actions, X_("EditSelectRegionOptions"), _("Select Regions"));
	ActionManager::register_action (editor_menu_actions, X_("EditPointMenu"), _("Edit Point"));
	ActionManager::register_action (editor_menu_actions, X_("MarkerClickBehavior"), _("Marker Interaction"));
	ActionManager::register_action (editor_menu_actions, X_("FadeMenu"), _("Fade"));
	ActionManager::register_action (editor_menu_actions, X_("LatchMenu"), _("Latch"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenu"), _("Region"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuLayering"), _("Layering"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuPosition"), _("Position"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuMarkers"), _("Markers"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuEdit"), _("Edit"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuTrim"), _("Trim"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuGain"), _("Gain"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuRanges"), _("Ranges"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuFades"), _("Fades"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuMIDI"), _("MIDI"));
	ActionManager::register_action (editor_menu_actions, X_("RegionMenuDuplicate"), _("Duplicate"));
	ActionManager::register_action (editor_menu_actions, X_("Link"), _("Link"));
	ActionManager::register_action (editor_menu_actions, X_("ZoomFocusMenu"), _("Zoom Focus"));
	ActionManager::register_action (editor_menu_actions, X_("LocateToMarker"), _("Locate to Markers"));
	ActionManager::register_action (editor_menu_actions, X_("MarkerMenu"), _("Markers"));
	ActionManager::register_action (editor_menu_actions, X_("CueMenu"), _("Cues"));
	ActionManager::register_action (editor_menu_actions, X_("MeterFalloff"), _("Meter falloff"));
	ActionManager::register_action (editor_menu_actions, X_("MeterHold"), _("Meter hold"));
	ActionManager::register_action (editor_menu_actions, X_("MIDI"), _("MIDI Options"));
	ActionManager::register_action (editor_menu_actions, X_("MiscOptions"), _("Misc Options"));
	ActionManager::register_action (editor_menu_actions, X_("Monitoring"), _("Monitoring"));
	ActionManager::register_action (editor_menu_actions, X_("MoveActiveMarkMenu"), _("Active Mark"));
	ActionManager::register_action (editor_menu_actions, X_("MovePlayHeadMenu"), _("Playhead"));
	ActionManager::register_action (editor_menu_actions, X_("PlayMenu"), _("Play"));
	ActionManager::register_action (editor_menu_actions, X_("PrimaryClockMenu"), _("Primary Clock"));
	ActionManager::register_action (editor_menu_actions, X_("Pullup"), _("Pullup / Pulldown"));
	ActionManager::register_action (editor_menu_actions, X_("RegionEditOps"), _("Region operations"));
	ActionManager::register_action (editor_menu_actions, X_("RegionGainMenu"), _("Gain"));
	ActionManager::register_action (editor_menu_actions, X_("RulerMenu"), _("Rulers"));
	ActionManager::register_action (editor_menu_actions, X_("SavedViewMenu"), _("Editor Views"));
	ActionManager::register_action (editor_menu_actions, X_("ScrollMenu"), _("Scroll"));
	ActionManager::register_action (editor_menu_actions, X_("SecondaryClockMenu"), _("Secondary Clock"));
	ActionManager::register_action (editor_menu_actions, X_("Select"), _("Select"));
	ActionManager::register_action (editor_menu_actions, X_("SelectMenu"), _("Select"));
	ActionManager::register_action (editor_menu_actions, X_("SeparateMenu"), _("Separate"));
	ActionManager::register_action (editor_menu_actions, X_("ConsolidateMenu"), _("Consolidate"));
	ActionManager::register_action (editor_menu_actions, X_("AnalyzeMenu"), _("Analyze"));
	ActionManager::register_action (editor_menu_actions, X_("SetLoopMenu"), _("Loop"));
	ActionManager::register_action (editor_menu_actions, X_("SetPunchMenu"), _("Punch"));
	ActionManager::register_action (editor_menu_actions, X_("Solo"), _("Solo"));
	ActionManager::register_action (editor_menu_actions, X_("Subframes"), _("Subframes"));
	ActionManager::register_action (editor_menu_actions, X_("SyncMenu"), _("Sync"));
	ActionManager::register_action (editor_menu_actions, X_("TempoMenu"), _("Tempo"));
	ActionManager::register_action (editor_menu_actions, X_("MappingMenu"), _("Mapping"));
	ActionManager::register_action (editor_menu_actions, X_("Timecode"), _("Timecode fps"));
	ActionManager::register_action (editor_menu_actions, X_("LayerDisplay"), _("Region Layers"));

	ActionManager::register_action (editor_menu_actions, X_("GridChoiceTriplets"), _("Triplets"));
	ActionManager::register_action (editor_menu_actions, X_("GridChoiceQuintuplets"), _("Quintuplets"));
	ActionManager::register_action (editor_menu_actions, X_("GridChoiceSeptuplets"), _("Septuplets"));

	act = ActionManager::register_action (editor_menu_actions, X_("TrackHeightMenu"), _("Height"));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	ActionManager::register_action (editor_menu_actions, X_("TrackMenu"), _("Track"));
	ActionManager::register_action (editor_menu_actions, X_("TrackPlaylistMenu"), _("Playlists"));
	ActionManager::register_action (editor_menu_actions, X_("Tools"), _("Tools"));
	ActionManager::register_action (editor_menu_actions, X_("View"), _("View"));
	ActionManager::register_action (editor_menu_actions, X_("ZoomFocus"), _("Zoom Focus"));
	ActionManager::register_action (editor_menu_actions, X_("ZoomMenu"), _("Zoom"));
	ActionManager::register_action (editor_menu_actions, X_("LuaScripts"), _("Lua Scripts"));

	register_region_actions ();

	/* add named actions for the editor */

	/* We don't bother registering "unlock" because it would be insensitive
	   when required. Editor::unlock() must be invoked directly.
	*/
	ActionManager::register_action (editor_actions, "lock", S_("Session|Lock"), sigc::mem_fun (*this, &Editor::lock));

	/* attachments visibility (editor-mixer-strip, bottom properties, sidebar list) */

	show_editor_list_action = ActionManager::register_toggle_action (editor_actions, "show-editor-list", _("Show Editor List"), sigc::mem_fun (*this, &Tabbable::att_right_button_toggled));
	ActionManager::session_sensitive_actions.push_back (show_editor_list_action);
	right_attachment_button.set_related_action (show_editor_list_action);

	show_editor_mixer_action = ActionManager::register_toggle_action (editor_actions, "show-editor-mixer", _("Show Editor Mixer"), sigc::mem_fun (*this, &Tabbable::att_left_button_toggled));
	ActionManager::session_sensitive_actions.push_back (show_editor_mixer_action);
	left_attachment_button.set_related_action (show_editor_mixer_action);

	show_editor_props_action = ActionManager::register_toggle_action (editor_actions, "show-editor-props", _("Show Editor Properties Box"), sigc::mem_fun (*this, &Tabbable::att_bottom_button_toggled));
	ActionManager::session_sensitive_actions.push_back (show_editor_props_action);
	bottom_attachment_button.set_related_action (show_editor_props_action);

	reg_sens (editor_actions, "playhead-to-next-region-boundary", _("Playhead to Next Region Boundary"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_next_region_boundary), true));
	reg_sens (editor_actions, "playhead-to-next-region-boundary-noselection", _("Playhead to Next Region Boundary (No Track Selection)"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_next_region_boundary), false));
	reg_sens (editor_actions, "playhead-to-previous-region-boundary", _("Playhead to Previous Region Boundary"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_previous_region_boundary), true));
	reg_sens (editor_actions, "playhead-to-previous-region-boundary-noselection", _("Playhead to Previous Region Boundary (No Track Selection)"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_previous_region_boundary), false));

	reg_sens (editor_actions, "playhead-to-next-region-start", _("Playhead to Next Region Start"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_next_region_point), _playhead_cursor, RegionPoint (Start)));
	reg_sens (editor_actions, "playhead-to-next-region-end", _("Playhead to Next Region End"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_next_region_point), _playhead_cursor, RegionPoint (End)));
	reg_sens (editor_actions, "playhead-to-next-region-sync", _("Playhead to Next Region Sync"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_next_region_point), _playhead_cursor, RegionPoint (SyncPoint)));

	reg_sens (editor_actions, "playhead-to-previous-region-start", _("Playhead to Previous Region Start"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_previous_region_point), _playhead_cursor, RegionPoint (Start)));
	reg_sens (editor_actions, "playhead-to-previous-region-end", _("Playhead to Previous Region End"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_previous_region_point), _playhead_cursor, RegionPoint (End)));
	reg_sens (editor_actions, "playhead-to-previous-region-sync", _("Playhead to Previous Region Sync"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_previous_region_point), _playhead_cursor, RegionPoint (SyncPoint)));

	reg_sens (editor_actions, "selected-marker-to-next-region-boundary", _("To Next Region Boundary"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_next_region_boundary), true));
	reg_sens (editor_actions, "selected-marker-to-next-region-boundary-noselection", _("To Next Region Boundary (No Track Selection)"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_next_region_boundary), false));
	reg_sens (editor_actions, "selected-marker-to-previous-region-boundary", _("To Previous Region Boundary"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_previous_region_boundary), true));
	reg_sens (editor_actions, "selected-marker-to-previous-region-boundary-noselection", _("To Previous Region Boundary (No Track Selection)"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_previous_region_boundary), false));

	reg_sens (editor_actions, "edit-cursor-to-next-region-start", _("To Next Region Start"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (Start)));
	reg_sens (editor_actions, "edit-cursor-to-next-region-end", _("To Next Region End"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (End)));
	reg_sens (editor_actions, "edit-cursor-to-next-region-sync", _("To Next Region Sync"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (SyncPoint)));

	reg_sens (editor_actions, "edit-cursor-to-previous-region-start", _("To Previous Region Start"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (Start)));
	reg_sens (editor_actions, "edit-cursor-to-previous-region-end", _("To Previous Region End"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (End)));
	reg_sens (editor_actions, "edit-cursor-to-previous-region-sync", _("To Previous Region Sync"), sigc::bind (sigc::mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (SyncPoint)));

	reg_sens (editor_actions, "edit-cursor-to-range-start", _("To Range Start"), sigc::mem_fun(*this, &Editor::selected_marker_to_selection_start));
	reg_sens (editor_actions, "edit-cursor-to-range-end", _("To Range End"), sigc::mem_fun(*this, &Editor::selected_marker_to_selection_end));

	reg_sens (editor_actions, "playhead-to-range-start", _("Playhead to Range Start"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_selection_start), _playhead_cursor));
	reg_sens (editor_actions, "playhead-to-range-end", _("Playhead to Range End"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_to_selection_end), _playhead_cursor));

	reg_sens (editor_actions, "select-all-objects", _("Select All Objects"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_objects), SelectionSet));

	reg_sens (editor_actions, "select-loop-range", _("Set Range to Loop Range"), sigc::mem_fun(*this, &Editor::set_selection_from_loop));
	reg_sens (editor_actions, "select-punch-range", _("Set Range to Punch Range"), sigc::mem_fun(*this, &Editor::set_selection_from_punch));
	reg_sens (editor_actions, "select-from-regions", _("Set Range to Selected Regions"), sigc::mem_fun(*this, &Editor::set_selection_from_region));

	reg_sens (editor_actions, "edit-current-tempo", _("Edit Current Tempo"), sigc::mem_fun(*this, &Editor::edit_current_tempo));
	reg_sens (editor_actions, "edit-current-meter", _("Edit Current Time Signature"), sigc::mem_fun(*this, &Editor::edit_current_meter));

	reg_sens (editor_actions, "select-all-after-edit-cursor", _("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true, false));
	reg_sens (editor_actions, "alternate-select-all-after-edit-cursor", _("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true, false));
	reg_sens (editor_actions, "select-all-before-edit-cursor", _("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false, false));
	reg_sens (editor_actions, "alternate-select-all-before-edit-cursor", _("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false, false));

	reg_sens (editor_actions, "select-all-between-cursors", _("Select All Overlapping Edit Range"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), false));
	reg_sens (editor_actions, "select-all-within-cursors", _("Select All Inside Edit Range"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), true));

	reg_sens (editor_actions, "select-range-between-cursors", _("Select Edit Range"), sigc::mem_fun(*this, &Editor::select_range_between));

	reg_sens (editor_actions, "select-all-in-punch-range", _("Select All in Punch Range"), sigc::mem_fun(*this, &Editor::select_all_selectables_using_punch));
	reg_sens (editor_actions, "select-all-in-loop-range", _("Select All in Loop Range"), sigc::mem_fun(*this, &Editor::select_all_selectables_using_loop));

	reg_sens (editor_actions, "select-next-route", _("Select Next Track or Bus"), sigc::bind (sigc::mem_fun(*this, &Editor::select_next_stripable), true));
	reg_sens (editor_actions, "select-prev-route", _("Select Previous Track or Bus"), sigc::bind (sigc::mem_fun(*this, &Editor::select_prev_stripable), true));

	reg_sens (editor_actions, "select-next-stripable", _("Select Next Strip"), sigc::bind (sigc::mem_fun(*this, &Editor::select_next_stripable), false));
	reg_sens (editor_actions, "select-prev-stripable", _("Select Previous Strip"), sigc::bind (sigc::mem_fun(*this, &Editor::select_prev_stripable), false));

	reg_sens (editor_actions, "toggle-all-existing-automation", _("Toggle All Existing Automation"), sigc::mem_fun (*this, &Editor::toggle_all_existing_automation));
	reg_sens (editor_actions, "toggle-layer-display", _("Toggle Layer Display"), sigc::mem_fun (*this, &Editor::toggle_layer_display));

	reg_sens (editor_actions, "layer-display-stacked", _("Stacked layer display"), sigc::mem_fun (*this, &Editor::layer_display_stacked));
	reg_sens (editor_actions, "layer-display-overlaid", _("Overlaid layer display"), sigc::mem_fun (*this, &Editor::layer_display_overlaid));

	act = reg_sens (editor_actions, "show-plist-selector", _("Show Playlist Selector"), sigc::mem_fun (*this, &Editor::launch_playlist_selector));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	/* these "overlap" with Region/nudge-* and also Common/nudge-* but
	 * provide a single editor-related action that will nudge a region,
	 * selected marker or playhead
	 */

	reg_sens (editor_actions, "nudge-forward", _("Nudge Later"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_forward), false, false));
	reg_sens (editor_actions, "alternate-nudge-forward", _("Nudge Later"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_forward), false, false));
	reg_sens (editor_actions, "nudge-backward", _("Nudge Earlier"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_backward), false, false));
	reg_sens (editor_actions, "alternate-nudge-backward", _("Nudge Earlier"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_backward), false, false));

	act = reg_sens (editor_actions, "track-record-enable-toggle", _("Toggle Record Enable"), sigc::mem_fun(*this, &Editor::toggle_record_enable));
	ActionManager::track_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-solo-toggle", _("Toggle Solo"), sigc::mem_fun(*this, &Editor::toggle_solo));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-mute-toggle", _("Toggle Mute"), sigc::mem_fun(*this, &Editor::toggle_mute));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-solo-isolate-toggle", _("Toggle Solo Isolate"), sigc::mem_fun(*this, &Editor::toggle_solo_isolate));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	for (int i = 1; i <= 12; ++i) {
		string const a = string_compose (X_("save-visual-state-%1"), i);
		string const n = string_compose (_("Save View %1"), i);
		reg_sens (editor_actions, a.c_str(), n.c_str(), sigc::bind (sigc::mem_fun (*this, &Editor::start_visual_state_op), i - 1));
	}

	for (int i = 1; i <= 12; ++i) {
		string const a = string_compose (X_("goto-visual-state-%1"), i);
		string const n = string_compose (_("Go to View %1"), i);
		reg_sens (editor_actions, a.c_str(), n.c_str(), sigc::bind (sigc::mem_fun (*this, &Editor::cancel_visual_state_op), i - 1));
	}

	reg_sens (editor_actions, "zoom-to-session", _("Zoom to Session"), sigc::mem_fun(*this, &Editor::temporal_zoom_session));
	reg_sens (editor_actions, "zoom-to-extents", _("Zoom to Extents"), sigc::mem_fun(*this, &Editor::temporal_zoom_extents));
	reg_sens (editor_actions, "zoom-to-selection", _("Zoom to Selection"), sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_selection), Both));
	reg_sens (editor_actions, "zoom-to-selection-horiz", _("Zoom to Selection (Horizontal)"), sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_selection), Horizontal));
	reg_sens (editor_actions, "toggle-zoom", _("Toggle Zoom State"), sigc::mem_fun(*this, &Editor::swap_visual_state));

	reg_sens (editor_actions, "expand-tracks", _("Expand Track Height"), sigc::bind (sigc::mem_fun (*this, &Editor::tav_zoom_step), false));
	reg_sens (editor_actions, "shrink-tracks", _("Shrink Track Height"), sigc::bind (sigc::mem_fun (*this, &Editor::tav_zoom_step), true));

	reg_sens (editor_actions, "fit_1_track", _("Fit 1 Track"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 1));
	reg_sens (editor_actions, "fit_2_tracks", _("Fit 2 Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 2));
	reg_sens (editor_actions, "fit_4_tracks", _("Fit 4 Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 4));
	reg_sens (editor_actions, "fit_8_tracks", _("Fit 8 Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 8));
	reg_sens (editor_actions, "fit_16_tracks", _("Fit 16 Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 16));
	reg_sens (editor_actions, "fit_32_tracks", _("Fit 32 Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 32));
	reg_sens (editor_actions, "fit_all_tracks", _("Fit All Tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 0));
	reg_sens (editor_actions, "fit_selected_tracks", _("Fit Selected Tracks"), sigc::mem_fun(*this, &Editor::fit_selection));

	reg_sens (editor_actions, "zoom_10_ms", _("Zoom to 10 ms"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10));
	reg_sens (editor_actions, "zoom_100_ms", _("Zoom to 100 ms"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 100));
	reg_sens (editor_actions, "zoom_1_sec", _("Zoom to 1 sec"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 1000));
	reg_sens (editor_actions, "zoom_10_sec", _("Zoom to 10 sec"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10 * 1000));
	reg_sens (editor_actions, "zoom_1_min", _("Zoom to 1 min"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 60 * 1000));
	reg_sens (editor_actions, "zoom_5_min", _("Zoom to 5 min"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 5 * 60 * 1000));
	reg_sens (editor_actions, "zoom_10_min", _("Zoom to 10 min"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10 * 60 * 1000));

	act = reg_sens (editor_actions, "move-selected-tracks-up", _("Move Selected Tracks Up"), sigc::bind (sigc::mem_fun(*this, &Editor::move_selected_tracks), true));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "move-selected-tracks-down", _("Move Selected Tracks Down"), sigc::bind (sigc::mem_fun(*this, &Editor::move_selected_tracks), false));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "scroll-tracks-up", _("Scroll Tracks Up"), sigc::mem_fun(*this, &Editor::scroll_tracks_up));
	act = reg_sens (editor_actions, "scroll-tracks-down", _("Scroll Tracks Down"), sigc::mem_fun(*this, &Editor::scroll_tracks_down));
	act = reg_sens (editor_actions, "step-tracks-up", _("Step Tracks Up"), sigc::hide_return (sigc::bind (sigc::mem_fun(*this, &Editor::scroll_up_one_track), true)));
	act = reg_sens (editor_actions, "step-tracks-down", _("Step Tracks Down"), sigc::hide_return (sigc::bind (sigc::mem_fun(*this, &Editor::scroll_down_one_track), true)));
	act = reg_sens (editor_actions, "select-topmost", _("Select Topmost Track"), (sigc::mem_fun(*this, &Editor::select_topmost_track)));

	reg_sens (editor_actions, "scroll-backward", _("Scroll Backward"), sigc::bind (sigc::mem_fun(*this, &Editor::scroll_backward), 0.8f));
	reg_sens (editor_actions, "scroll-forward", _("Scroll Forward"), sigc::bind (sigc::mem_fun(*this, &Editor::scroll_forward), 0.8f));
	reg_sens (editor_actions, "center-playhead", _("Center Playhead"), sigc::mem_fun(*this, &Editor::center_playhead));
	reg_sens (editor_actions, "center-edit-cursor", _("Center Edit Point"), sigc::mem_fun(*this, &Editor::center_edit_point));

	reg_sens (editor_actions, "scroll-playhead-forward", _("Playhead Forward"), sigc::bind (sigc::mem_fun(*this, &Editor::scroll_playhead), true));;
	reg_sens (editor_actions, "scroll-playhead-backward", _("Playhead Backward"), sigc::bind (sigc::mem_fun(*this, &Editor::scroll_playhead), false));

	reg_sens (editor_actions, "playhead-to-edit", _("Playhead to Active Mark"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_align), true));
	reg_sens (editor_actions, "edit-to-playhead", _("Active Mark to Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::cursor_align), false));

	toggle_reg_sens (editor_actions, "toggle-skip-playback", _("Use Skip Ranges"), sigc::mem_fun(*this, &Editor::toggle_skip_playback));

	reg_sens (editor_actions, "set-loop-from-edit-range", _("Set Loop from Selection"), sigc::bind (sigc::mem_fun(*this, &Editor::set_loop_from_selection), false));
	reg_sens (editor_actions, "set-punch-from-edit-range", _("Set Punch from Selection"), sigc::mem_fun(*this, &Editor::set_punch_from_selection));
	reg_sens (editor_actions, "set-session-from-edit-range", _("Set Session Start/End from Selection"), sigc::mem_fun(*this, &Editor::set_session_extents_from_selection));

	reg_sens (editor_actions, "find-and-display-stripable", _("Find & Display Track/Bus"), sigc::mem_fun (*this, &Editor::find_and_display_track));

	if (Profile->get_mixbus ()) {
		reg_sens (editor_actions, "copy-paste-section", _("Copy/Paste Range Section to Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), CopyPasteSection));
		reg_sens (editor_actions, "cut-paste-section", _("Cut/Paste Range Section to Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), CutPasteSection));
		reg_sens (editor_actions, "insert-section", _("Insert Time Section at Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), InsertSection));
	} else {
		reg_sens (editor_actions, "copy-paste-section", _("Copy/Paste Range Section to Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), CopyPasteSection));
		reg_sens (editor_actions, "cut-paste-section", _("Cut/Paste Range Section to Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), CutPasteSection));
		reg_sens (editor_actions, "insert-section", _("Insert Time Section at Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), InsertSection));
	}

	reg_sens (editor_actions, "delete-section", _("Delete Range Section"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), DeleteSection));
	reg_sens (editor_actions, "alternate-delete-section", _("Delete Range Section"), sigc::bind (sigc::mem_fun(*this, &Editor::cut_copy_section), DeleteSection));

	/* this is a duplicated action so that the main menu can use a different label */
	reg_sens (editor_actions, "main-menu-play-selected-regions", _("Play Selected Regions"), sigc::mem_fun (*this, &Editor::play_selected_region));
	reg_sens (editor_actions, "main-menu-tag-selected-regions", _("Tag Selected Regions"), sigc::mem_fun (*this, &Editor::tag_selected_region));

	reg_sens (editor_actions, "group-selected-regions", _("Group Selected Regions"), sigc::mem_fun (*this, &Editor::group_selected_regions));
	reg_sens (editor_actions, "ungroup-selected-regions", _("Ungroup Selected Regions"), sigc::mem_fun (*this, &Editor::ungroup_selected_regions));

	reg_sens (editor_actions, "play-from-edit-point", _("Play from Edit Point"), sigc::mem_fun(*this, &Editor::play_from_edit_point));
	reg_sens (editor_actions, "play-from-edit-point-and-return", _("Play from Edit Point and Return"), sigc::mem_fun(*this, &Editor::play_from_edit_point_and_return));

	reg_sens (editor_actions, "play-edit-range", _("Play Edit Range"), sigc::mem_fun(*this, &Editor::play_edit_range));

	reg_sens (editor_actions, "set-playhead", _("Playhead to Mouse"), sigc::mem_fun(*this, &Editor::set_playhead_cursor));
	reg_sens (editor_actions, "set-edit-point", _("Active Marker to Mouse"), sigc::mem_fun(*this, &Editor::set_edit_point));
	reg_sens (editor_actions, "set-auto-punch-range", _("Set Auto Punch In/Out from Playhead"), sigc::mem_fun(*this, &Editor::set_auto_punch_range));

	reg_sens (editor_actions, "duplicate", _("Duplicate"), sigc::bind (sigc::mem_fun(*this, &Editor::duplicate_range), false));

	/* Open the dialogue to duplicate selected regions multiple times */
	reg_sens (editor_actions, "multi-duplicate", _("Multi-Duplicate..."),
	          sigc::bind (sigc::mem_fun (*this, &Editor::duplicate_range), true));


	selection_undo_action = reg_sens (editor_actions, "undo-last-selection-op", _("Undo Selection Change"), sigc::mem_fun(*this, &Editor::undo_selection_op));
	selection_redo_action = reg_sens (editor_actions, "redo-last-selection-op", _("Redo Selection Change"), sigc::mem_fun(*this, &Editor::redo_selection_op));

	reg_sens (editor_actions, "export-audio", _("Export Audio"), sigc::mem_fun(*this, &Editor::export_audio));
	reg_sens (editor_actions, "export-range", _("Export Range"), sigc::mem_fun(*this, &Editor::export_range));

	act = reg_sens (editor_actions, "editor-separate", _("Separate"), sigc::mem_fun(*this, &Editor::separate_region_from_selection));
	ActionManager::mouse_edit_point_requires_canvas_actions.push_back (act);

	act = reg_sens (editor_actions, "separate-from-punch", _("Separate Using Punch Range"), sigc::mem_fun(*this, &Editor::separate_region_from_punch));
	act = reg_sens (editor_actions, "separate-from-loop", _("Separate Using Loop Range"), sigc::mem_fun(*this, &Editor::separate_region_from_loop));

	act = reg_sens (editor_actions, "editor-crop", _("Crop"), sigc::mem_fun(*this, &Editor::crop_region_to_selection));
	ActionManager::time_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "add-range-marker-from-selection", _("Add Range Marker from Selection"), sigc::mem_fun(*this, &Editor::add_location_from_selection));
	ActionManager::session_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "add-tempo-from-playhead", _("Add Tempo Marker at Playhead"), sigc::mem_fun(*this, &Editor::add_tempo_from_playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "add-meter-from-playhead", _("Add Time Signature at Playhead"), sigc::mem_fun(*this, &Editor::add_meter_from_playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "editor-consolidate-with-processing", _("Consolidate Range (with processing)"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), ReplaceRange, true));
	ActionManager::time_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "editor-consolidate", _("Consolidate Range"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), ReplaceRange, false));
	ActionManager::time_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "editor-analyze-loudness", _("Loudness Analysis"), sigc::mem_fun(*this, &Editor::loudness_analyze_range_selection));
	ActionManager::time_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "editor-analyze-spectrum", _("Spectral Analysis"), sigc::mem_fun(*this, &Editor::spectral_analyze_range_selection));
	ActionManager::time_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "editor-loudness-assistant", _("Loudness Assistant"), sigc::bind (sigc::mem_fun(*this, &Editor::loudness_assistant), true));
	ActionManager::time_selection_sensitive_actions.push_back (act);


	reg_sens (editor_actions, "split-region", _("Split/Separate"), sigc::mem_fun (*this, &Editor::split_region));

	reg_sens (editor_actions, "editor-fade-range", _("Fade Range Selection"), sigc::mem_fun(*this, &Editor::fade_range));

	act = ActionManager::register_action (editor_actions, "set-tempo-from-edit-range", _("Set Tempo from Edit Range = Bar"), sigc::mem_fun(*this, &Editor::use_range_as_bar));
	ActionManager::time_selection_sensitive_actions.push_back (act);

	toggle_reg_sens (editor_actions, "toggle-log-window", _("Log"),
			sigc::mem_fun (ARDOUR_UI::instance(), &ARDOUR_UI::toggle_errors));

	reg_sens (editor_actions, "alternate-tab-to-transient-forwards", _("Move to Next Transient"), sigc::bind (sigc::mem_fun(*this, &Editor::tab_to_transient), true));
	reg_sens (editor_actions, "alternate-tab-to-transient-backwards", _("Move to Previous Transient"), sigc::bind (sigc::mem_fun(*this, &Editor::tab_to_transient), false));
	reg_sens (editor_actions, "tab-to-transient-forwards", _("Move to Next Transient"), sigc::bind (sigc::mem_fun(*this, &Editor::tab_to_transient), true));
	reg_sens (editor_actions, "tab-to-transient-backwards", _("Move to Previous Transient"), sigc::bind (sigc::mem_fun(*this, &Editor::tab_to_transient), false));

	reg_sens (editor_actions, "crop", _("Crop"), sigc::mem_fun(*this, &Editor::crop_region_to_selection));

//	reg_sens (editor_actions, "finish-add-range", _("Finish Add Range"), sigc::bind (sigc::mem_fun(*this, &Editor::keyboard_selection_finish), true));

	reg_sens (
		editor_actions,
		"move-range-start-to-previous-region-boundary",
		_("Move Range Start to Previous Region Boundary"),
		sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), false, false)
		);

	reg_sens (
		editor_actions,
		"move-range-start-to-next-region-boundary",
		_("Move Range Start to Next Region Boundary"),
		sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), false, true)
		);

	reg_sens (
		editor_actions,
		"move-range-end-to-previous-region-boundary",
		_("Move Range End to Previous Region Boundary"),
		sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), true, false)
		);

	reg_sens (
		editor_actions,
		"move-range-end-to-next-region-boundary",
		_("Move Range End to Next Region Boundary"),
		sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), true, true)
		);

	act = reg_sens (editor_actions, "remove-last-capture", _("Remove Last Capture"), (sigc::mem_fun(*this, &Editor::remove_last_capture)));
	act = reg_sens (editor_actions, "tag-last-capture", _("Tag Last Capture"), (sigc::mem_fun(*this, &Editor::tag_last_capture)));

	ActionManager::register_toggle_action (editor_actions, "toggle-stationary-playhead", _("Stationary Playhead"), (mem_fun(*this, &Editor::toggle_stationary_playhead)));

	show_touched_automation_action = ActionManager::register_toggle_action (editor_actions, "show-touched-automation", _("Show Automation Lane on Touch"), (mem_fun(*this, &Editor::toggle_show_touched_automation)));

	act = reg_sens (editor_actions, "insert-time", _("Insert Time"), (sigc::mem_fun(*this, &Editor::do_insert_time)));
	ActionManager::track_selection_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "remove-time", _("Remove Time"), (mem_fun(*this, &Editor::do_remove_time)));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::track_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "remove-gaps", _("Remove Gaps"), (sigc::mem_fun(*this, &Editor::do_remove_gaps)));
	ActionManager::track_selection_sensitive_actions.push_back (act);
	ActionManager::session_sensitive_actions.push_back (act);

	/*global playlist actions */
	ActionManager::register_action (editor_actions, "new-playlists-for-armed-tracks", _("New Playlist For Rec-Armed Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_armed_tracks), false));
	ActionManager::register_action (editor_actions, "new-playlists-for-all-tracks", _("New Playlist For All Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_all_tracks), false));
	act = ActionManager::register_action (editor_actions, "new-playlists-for-selected-tracks", _("New Playlist For Selected Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_selected_tracks), false));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	ActionManager::register_action (editor_actions, "copy-playlists-for-armed-tracks", _("Copy Playlist For Rec-Armed Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_armed_tracks), true));
	ActionManager::register_action (editor_actions, "copy-playlists-for-all-tracks", _("Copy Playlist For All Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_all_tracks), true));
	act = ActionManager::register_action (editor_actions, "copy-playlists-for-selected-tracks", _("Copy Playlist For Selected Tracks"), sigc::bind (sigc::mem_fun (*this, &Editor::new_playlists_for_selected_tracks), true));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "toggle-track-active", _("Toggle Active"), (sigc::mem_fun(*this, &Editor::toggle_tracks_active)));
	ActionManager::route_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "remove-track", _("Remove Selected Track(s)"), (sigc::mem_fun(*this, &Editor::remove_tracks)));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "fit-selection", _("Fit Selection (Vertical)"), sigc::mem_fun(*this, &Editor::fit_selection));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, "track-height-largest", _("Largest"), sigc::bind (
				sigc::mem_fun(*this, &Editor::set_track_height), HeightLargest));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-height-larger", _("Larger"), sigc::bind (
				sigc::mem_fun(*this, &Editor::set_track_height), HeightLarger));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-height-large", _("Large"), sigc::bind (
				sigc::mem_fun(*this, &Editor::set_track_height), HeightLarge));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-height-normal", _("Normal"), sigc::bind (
				sigc::mem_fun(*this, &Editor::set_track_height), HeightNormal));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);
	act = reg_sens (editor_actions, "track-height-small", _("Small"), sigc::bind (
				sigc::mem_fun(*this, &Editor::set_track_height), HeightSmall));
	ActionManager::stripable_selection_sensitive_actions.push_back (act);

	toggle_reg_sens (editor_actions, "sound-midi-notes", _("Sound Selected MIDI Notes"), sigc::mem_fun (*this, &Editor::toggle_sound_midi_notes));

	Glib::RefPtr<ActionGroup> marker_click_actions = ActionManager::create_action_group (own_bindings, X_("MarkerClickBehavior"));
	RadioAction::Group marker_click_group;

	radio_reg_sens (marker_click_actions, marker_click_group, "marker-click-select-only", _("Marker Click Only Selects"), sigc::bind (sigc::mem_fun(*this, &Editor::marker_click_behavior_chosen), Editing::MarkerClickSelectOnly));
	radio_reg_sens (marker_click_actions, marker_click_group, "marker-click-locate", _("Locate to Marker on Click"), sigc::bind (sigc::mem_fun(*this, &Editor::marker_click_behavior_chosen), Editing::MarkerClickLocate));
	radio_reg_sens (marker_click_actions, marker_click_group, "marker-click-locate-when-stopped", _("Locate To Marker When Transport Is Not Rolling "), sigc::bind (sigc::mem_fun(*this, &Editor::marker_click_behavior_chosen), Editing::MarkerClickLocateWhenStopped));
	ActionManager::register_action (editor_actions, X_("cycle-marker-click-behavior"), _("Next Marker Click Mode"), sigc::mem_fun (*this, &Editor::cycle_marker_click_behavior));

	Glib::RefPtr<ActionGroup> lua_script_actions = ActionManager::create_action_group (own_bindings, X_("LuaAction"));

	for (int i = 1; i <= MAX_LUA_ACTION_SCRIPTS; ++i) {
		string const a = string_compose (X_("script-%1"), i);
		string const n = string_compose (_("Unset #%1"), i);
		act = ActionManager::register_action (lua_script_actions, a.c_str(), n.c_str(), sigc::bind (sigc::mem_fun (*this, &Editor::trigger_script), i - 1));
		act->set_tooltip (_("No action bound\nRight-click to assign"));
		act->set_sensitive (false);
	}

	ActionManager::register_action (editor_actions, "step-mouse-mode", _("Step Mouse Mode"), sigc::bind (sigc::mem_fun(*this, &Editor::step_mouse_mode), true));

	RadioAction::Group edit_point_group;
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-playhead"), _("Playhead"), (sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtPlayhead)));
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-mouse"), _("Mouse"), (sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtMouse)));
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-selected-marker"), _("Marker"), (sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtSelectedMarker)));

	ActionManager::register_action (editor_actions, "cycle-edit-point", _("Change Edit Point"), sigc::bind (sigc::mem_fun (*this, &Editor::cycle_edit_point), false));
	ActionManager::register_action (editor_actions, "cycle-edit-point-with-marker", _("Change Edit Point Including Marker"), sigc::bind (sigc::mem_fun (*this, &Editor::cycle_edit_point), true));

	ActionManager::register_action (editor_actions, "set-edit-ripple", _("Ripple"), bind (mem_fun (*this, &Editor::set_edit_mode), Ripple));
	ActionManager::register_action (editor_actions, "set-edit-slide", _("Slide"), sigc::bind (sigc::mem_fun (*this, &Editor::set_edit_mode), Slide));
	ActionManager::register_action (editor_actions, "set-edit-lock", S_("EditMode|Lock"), sigc::bind (sigc::mem_fun (*this, &Editor::set_edit_mode), Lock));
	ActionManager::register_action (editor_actions, "cycle-edit-mode", _("Cycle Edit Mode"), sigc::mem_fun (*this, &Editor::cycle_edit_mode));

	ActionManager::register_action (editor_actions, "set-ripple-selected", _("Selected"), bind (mem_fun (*this, &Editor::set_ripple_mode), RippleSelected));
	ActionManager::register_action (editor_actions, "set-ripple-all", _("All"), sigc::bind (sigc::mem_fun (*this, &Editor::set_ripple_mode), RippleAll));
	ActionManager::register_action (editor_actions, "set-ripple-interview", S_("Interview"), sigc::bind (sigc::mem_fun (*this, &Editor::set_ripple_mode), RippleInterview));

	ActionManager::register_toggle_action (editor_actions, X_("show-marker-lines"), _("Show Marker Lines"), sigc::mem_fun (*this, &Editor::toggle_marker_lines));

	/* RULERS */

	Glib::RefPtr<ActionGroup> ruler_actions = ActionManager::create_action_group (own_bindings, X_("Rulers"));
	ruler_minsec_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-minsec-ruler"), _("Mins:Secs"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_timecode_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-timecode-ruler"), _("Timecode"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_samples_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-samples-ruler"), _("Samples"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_bbt_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-bbt-ruler"), _("Bars:Beats"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_meter_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-meter-ruler"), _("Time Signature"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_tempo_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-tempo-ruler"), _("Tempo"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_range_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-range-ruler"), _("Range Markers"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_section_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-arrangement-ruler"), _("Arrangement"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	ruler_marker_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-marker-ruler"), _("Location Markers"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));

	RadioAction::Group marker_choice_group;
	RadioAction::Group range_choice_group;

	all_marker_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, marker_choice_group, X_("show-all-markers"), _("All Markers"), sigc::bind (sigc::mem_fun(*this, &Editor::show_marker_type), all_marker_types)));
	cd_marker_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, marker_choice_group, X_("show-cd-markers"), _("Only CD Markers"), sigc::bind (sigc::mem_fun(*this, &Editor::show_marker_type), CDMarks)));
	scene_marker_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, marker_choice_group, X_("show-cue-markers"), _("Only Cue Markers"), sigc::bind (sigc::mem_fun(*this, &Editor::show_marker_type), CueMarks)));
	cue_marker_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, marker_choice_group, X_("show-scene-markers"), _("Only Scene Markers"), sigc::bind (sigc::mem_fun(*this, &Editor::show_marker_type), SceneMarks)));
	location_marker_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, marker_choice_group, X_("show-location-markers"), _("Only Location Markers"), sigc::bind (sigc::mem_fun(*this, &Editor::show_marker_type), LocationMarks)));

	all_range_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, range_choice_group, X_("show-all-ranges"), _("All Ranges"), sigc::bind (sigc::mem_fun(*this, &Editor::show_range_type), all_range_types)));
	session_range_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, range_choice_group, X_("show-session-range"), _("Only Session Range"), sigc::bind (sigc::mem_fun(*this, &Editor::show_range_type), SessionRange)));
	punch_range_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, range_choice_group, X_("show-punch-range"), _("Only Punch Range"), sigc::bind (sigc::mem_fun(*this, &Editor::show_range_type), PunchRange)));
	loop_range_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, range_choice_group, X_("show-loop-range"), _("Only Loop Range"), sigc::bind (sigc::mem_fun(*this, &Editor::show_range_type), LoopRange)));
	other_range_action = Glib::RefPtr<RadioAction>::cast_static (ActionManager::register_radio_action (ruler_actions, range_choice_group, X_("show-other-ranges"), _("Only Named Ranges"), sigc::bind (sigc::mem_fun(*this, &Editor::show_range_type), OtherRange)));

	ActionManager::register_action (editor_menu_actions, X_("VideoMonitorMenu"), _("Video Monitor"));

	ruler_video_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (ruler_actions, X_("toggle-video-ruler"), _("Video Timeline"), sigc::mem_fun(*this, &Editor::toggle_ruler_visibility)));
	xjadeo_proc_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("ToggleJadeo"), _("Video Monitor"), sigc::mem_fun (*this, &Editor::set_xjadeo_proc)));

	xjadeo_ontop_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-ontop"), _("Always on Top"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 1)));
	xjadeo_timecode_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-timecode"), _("Timecode"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 2)));
	xjadeo_frame_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-frame"), _("Frame number"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 3)));
	xjadeo_osdbg_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-osdbg"), _("Timecode Background"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 4)));
	xjadeo_fullscreen_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-fullscreen"), _("Fullscreen"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 5)));
	xjadeo_letterbox_action = Glib::RefPtr<ToggleAction>::cast_static (ActionManager::register_toggle_action (editor_actions, X_("toggle-vmon-letterbox"), _("Letterbox"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 6)));
	xjadeo_zoom_100 = reg_sens (editor_actions, "zoom-vmon-100", _("Original Size"), sigc::bind (sigc::mem_fun (*this, &Editor::set_xjadeo_viewoption), (int) 7));

	/* set defaults here */

	no_ruler_shown_update = true;

	ruler_minsec_action->set_active (false);
	ruler_timecode_action->set_active (true);
	ruler_samples_action->set_active (false);
	ruler_bbt_action->set_active (true);
	ruler_meter_action->set_active (true);
	ruler_tempo_action->set_active (true);
	ruler_range_action->set_active (true);
	ruler_marker_action->set_active (true);

	ruler_video_action->set_active (false);
	xjadeo_proc_action->set_active (false);
	xjadeo_proc_action->set_sensitive (false);
	xjadeo_ontop_action->set_active (false);
	xjadeo_ontop_action->set_sensitive (false);
	xjadeo_timecode_action->set_active (false);
	xjadeo_timecode_action->set_sensitive (false);
	xjadeo_frame_action->set_active (false);
	xjadeo_frame_action->set_sensitive (false);
	xjadeo_osdbg_action->set_active (false);
	xjadeo_osdbg_action->set_sensitive (false);
	xjadeo_fullscreen_action->set_active (false);
	xjadeo_fullscreen_action->set_sensitive (false);
	xjadeo_letterbox_action->set_active (false);
	xjadeo_letterbox_action->set_sensitive (false);
	xjadeo_zoom_100->set_sensitive (false);

	no_ruler_shown_update = false;

	/* REGION LIST */

	Glib::RefPtr<ActionGroup> rl_actions = ActionManager::create_action_group (own_bindings, X_("RegionList"));
	RadioAction::Group sort_type_group;
	RadioAction::Group sort_order_group;

	/* the region list popup menu */
	act = ActionManager::register_action (rl_actions, X_("rlAudition"), _("Audition"), sigc::mem_fun(*this, &Editor::audition_region_from_region_list));
	ActionManager::region_list_selection_sensitive_actions.push_back (act);

	ActionManager::register_action (rl_actions, X_("removeUnusedRegions"), _("Remove Unused"), sigc::mem_fun (*_regions, &EditorRegions::remove_unused_regions));

	act = reg_sens (editor_actions, X_("addExistingPTFiles"), _("Import PT session"), sigc::mem_fun (*this, &Editor::external_pt_dialog));
	ActionManager::write_sensitive_actions.push_back (act);

	act = reg_sens (editor_actions, X_("LoudnessAssistant"), _("Loudness Assistant..."), sigc::bind (sigc::mem_fun (*this, &Editor::loudness_assistant), false));
	ActionManager::write_sensitive_actions.push_back (act);

	/* the next two are duplicate items with different names for use in two different contexts */

	act = reg_sens (editor_actions, X_("addExternalAudioToRegionList"), _("Import to Source List..."), sigc::bind (sigc::mem_fun(*this, &Editor::add_external_audio_action), ImportAsRegion));
	ActionManager::write_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, X_("importFromSession"), _("Import from Session"), sigc::mem_fun(*this, &Editor::session_import_dialog));
	ActionManager::write_sensitive_actions.push_back (act);


	act = ActionManager::register_action (editor_actions, X_("bring-into-session"), _("Bring all media into session folder"), sigc::mem_fun(*this, &Editor::bring_all_sources_into_session));
	ActionManager::write_sensitive_actions.push_back (act);

	ActionManager::register_toggle_action (editor_actions, X_("ToggleSummary"), _("Show Summary"), sigc::mem_fun (*this, &Editor::set_summary));

	ActionManager::register_toggle_action (editor_actions, X_("ToggleGroupTabs"), _("Show Group Tabs"), sigc::mem_fun (*this, &Editor::set_group_tabs));

	ActionManager::register_action (editor_actions, X_("toggle-midi-input-active"), _("Toggle MIDI Input Active for Editor-Selected Tracks/Busses"),
	                           sigc::bind (sigc::mem_fun (*this, &Editor::toggle_midi_input_active), false));


	/* MIDI stuff */
	reg_sens (editor_actions, "quantize", _("Quantize"), sigc::mem_fun (*this, &Editor::quantize_region));

	act = ActionManager::register_toggle_action (editor_actions, "set-mouse-mode-object-range", _("Smart Mode"), sigc::mem_fun (*this, &Editor::mouse_mode_object_range_toggled));
	smart_mode_action = Glib::RefPtr<ToggleAction>::cast_static (act);
	smart_mode_button.set_related_action (smart_mode_action);
	smart_mode_button.set_text (_("Smart"));
	smart_mode_button.set_name ("mouse mode button");
}

static void _lua_print (std::string s) {
#ifndef NDEBUG
	std::cout << "LuaInstance: " << s << "\n";
#endif
	PBD::info << "LuaInstance: " << s << endmsg;
}

void
Editor::trigger_script_by_name (const std::string script_name, const std::string in_args)
{
	string script_path;
	ARDOUR::LuaScriptList scr = LuaScripting::instance ().scripts(LuaScriptInfo::EditorAction);
	for (ARDOUR::LuaScriptList::const_iterator s = scr.begin(); s != scr.end(); ++s) {

		if ((*s)->name == script_name) {
			script_path = (*s)->path;

			if (!Glib::file_test (script_path, Glib::FILE_TEST_EXISTS | Glib::FILE_TEST_IS_REGULAR)) {
#ifndef NDEBUG
				cerr << "Lua Script action: path to " << script_path << " does not appear to be valid" << endl;
#endif
				return;
			}

#ifdef MIXBUS
			bool sandbox = false; // mixer state save/reset/restore needs os.*
#else
			bool sandbox = UIConfiguration::instance().get_sandbox_all_lua_scripts ();
#endif

			LuaState lua (true, sandbox);
			lua.Print.connect (&_lua_print);
			lua_State* L = lua.getState();
			LuaInstance::register_classes (L, sandbox);
			LuaBindings::set_session (L, _session);
			luabridge::push <PublicEditor *> (L, &PublicEditor::instance());
			lua_setglobal (L, "Editor");
			lua.do_command ("function ardour () end");
			lua.do_file (script_path);
			luabridge::LuaRef args (luabridge::newTable (L));

			args[1] = in_args;

			try {
				luabridge::LuaRef fn = luabridge::getGlobal (L, "factory");
				if (fn.isFunction()) {
					fn (args)();
				}
			} catch (luabridge::LuaException const& e) {
#ifndef NDEBUG
				cerr << "LuaException:" << e.what () << endl;
#endif
				PBD::warning << "LuaException: " << e.what () << endmsg;
			} catch (...) {
				cerr << "Lua script failed: " << script_path << endl;
			}
			return;
		}
	}
#ifndef NDEBUG
	cerr << "Lua script was not found: " << script_name << endl;
#endif
}

void
Editor::load_bindings ()
{
	own_bindings = Bindings::get_bindings (editor_name());
	EditingContext::load_shared_bindings ();
	bindings.push_back (own_bindings);
	set_widget_bindings (contents(), bindings, Gtkmm2ext::ARDOUR_BINDING_KEY);
}

void
Editor::toggle_skip_playback ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), "toggle-skip-playback");
	bool s = Config->get_skip_playback ();
	if (tact->get_active() != s) {
		Config->set_skip_playback (tact->get_active());
	}
}

void
Editor::toggle_ruler_visibility ()
{
	if (no_ruler_shown_update) {
		return;
	}

	update_ruler_visibility ();
	store_ruler_visibility ();
}

void
Editor::set_summary ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleSummary"));
	_session->config.set_show_summary (tact->get_active ());
}

void
Editor::set_group_tabs ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleGroupTabs"));
	_session->config.set_show_group_tabs (tact->get_active ());
}

void
Editor::set_close_video_sensitive (bool onoff)
{
	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Main"), X_("CloseVideo"));
	act->set_sensitive (onoff);
}

void
Editor::set_xjadeo_sensitive (bool onoff)
{
	xjadeo_proc_action->set_sensitive(onoff);
}

void
Editor::toggle_xjadeo_proc (int state)
{
	switch(state) {
		case 1:
			xjadeo_proc_action->set_active(true);
			break;
		case 0:
			xjadeo_proc_action->set_active(false);
			break;
		default:
			xjadeo_proc_action->set_active(!xjadeo_proc_action->get_active());
			break;
	}
	bool onoff = xjadeo_proc_action->get_active();
	xjadeo_ontop_action->set_sensitive(onoff);
	xjadeo_timecode_action->set_sensitive(onoff);
	xjadeo_frame_action->set_sensitive(onoff);
	xjadeo_osdbg_action->set_sensitive(onoff);
	xjadeo_fullscreen_action->set_sensitive(onoff);
	xjadeo_letterbox_action->set_sensitive(onoff);
	xjadeo_zoom_100->set_sensitive(onoff);
}

void
Editor::set_xjadeo_proc ()
{
	if (xjadeo_proc_action->get_active()) {
		ARDOUR_UI::instance()->video_timeline->open_video_monitor();
	} else {
		ARDOUR_UI::instance()->video_timeline->close_video_monitor();
	}
}

void
Editor::toggle_xjadeo_viewoption (int what, int state)
{
	Glib::RefPtr<Gtk::ToggleAction> action;
	switch (what) {
		case 1:
			action = xjadeo_ontop_action;
			break;
		case 2:
			action = xjadeo_timecode_action;
			break;
		case 3:
			action = xjadeo_frame_action;
			break;
		case 4:
			action = xjadeo_osdbg_action;
			break;
		case 5:
			action = xjadeo_fullscreen_action;
			break;
		case 6:
			action = xjadeo_letterbox_action;
			break;
		case 7:
			return;
		default:
			return;
	}

	switch(state) {
		case 1:
			action->set_active(true);
			break;
		case 0:
			action->set_active(false);
			break;
		default:
			action->set_active(!action->get_active());
			break;
	}
}

void
Editor::set_xjadeo_viewoption (int what)
{
	Glib::RefPtr<Gtk::ToggleAction> action;
	switch (what) {
		case 1:
			action = xjadeo_ontop_action;
			break;
		case 2:
			action = xjadeo_timecode_action;
			break;
		case 3:
			action = xjadeo_frame_action;
			break;
		case 4:
			action = xjadeo_osdbg_action;
			break;
		case 5:
			action = xjadeo_fullscreen_action;
			break;
		case 6:
			action = xjadeo_letterbox_action;
			break;
		case 7:
			ARDOUR_UI::instance()->video_timeline->control_video_monitor(what, 0);
			return;
		default:
			return;
	}
	if (action->get_active()) {
		ARDOUR_UI::instance()->video_timeline->control_video_monitor(what, 1);
	} else {
		ARDOUR_UI::instance()->video_timeline->control_video_monitor(what, 0);
	}
}

void
Editor::edit_current_meter ()
{
	edit_meter_section (Temporal::TempoMap::use()->metric_at (ARDOUR_UI::instance()->primary_clock->last_when()).get_editable_meter());
}

void
Editor::edit_current_tempo ()
{
	edit_tempo_section (Temporal::TempoMap::use()->metric_at (ARDOUR_UI::instance()->primary_clock->last_when()).get_editable_tempo());
}

RefPtr<RadioAction>
Editor::edit_point_action (EditPoint ep)
{
	const char* action = 0;
	RefPtr<Action> act;

	switch (ep) {
	case Editing::EditAtPlayhead:
		action = X_("edit-at-playhead");
		break;
	case Editing::EditAtSelectedMarker:
		action = X_("edit-at-selected-marker");
		break;
	case Editing::EditAtMouse:
		action = X_("edit-at-mouse");
		break;
	default:
		fatal << string_compose (_("programming error: %1: %2"), "Editor: impossible edit point type", (int) ep) << endmsg;
		abort(); /*NOTREACHED*/
	}

	act = ActionManager::get_action (X_("Editor"), action);

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		return ract;

	} else  {
		error << string_compose (_("programming error: %1: %2"), "Editor::edit_point_action could not find action to match edit point.", action) << endmsg;
		return RefPtr<RadioAction> ();
	}
}

void
Editor::edit_point_chosen (EditPoint ep)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<RadioAction> ract = edit_point_action (ep);

	if (ract && ract->get_active()) {
		set_edit_point_preference (ep);
	}
}

RefPtr<RadioAction>
Editor::marker_click_behavior_action (MarkerClickBehavior m)
{
	const char* action = 0;
	RefPtr<Action> act;

	switch (m) {
	case MarkerClickSelectOnly:
		action = X_("marker-click-select-only");
		break;
	case MarkerClickLocate:
		action = X_("marker-click-locate");
		break;
	case MarkerClickLocateWhenStopped:
		action = X_("marker-click-locate-when-stopped");
		break;
	}

	return ActionManager::get_radio_action (X_("MarkerClickBehavior"), action);
}

void
Editor::toggle_sound_midi_notes ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("sound-midi-notes"));
	bool s = UIConfiguration::instance().get_sound_midi_notes();
	if (tact->get_active () != s) {
		UIConfiguration::instance().set_sound_midi_notes (tact->get_active());
	}
}

void
Editor::marker_click_behavior_chosen (Editing::MarkerClickBehavior m)
{
	RefPtr<RadioAction> ract = marker_click_behavior_action (m);
	if (ract && ract->get_active()) {
		set_marker_click_behavior (m);
	}
}

void
Editor::capture_sources_changed (bool cleared)
{
	if (cleared || !_session || _session->actively_recording ()) {
		ActionManager::get_action (X_("Editor"), X_("remove-last-capture"))->set_sensitive (false);
	} else {
		ActionManager::get_action (X_("Editor"), X_("remove-last-capture"))->set_sensitive (_session->have_last_capture_sources ());
	}
}

/** A Configuration parameter has changed.
 * @param parameter_name Name of the changed parameter.
 */
void
Editor::parameter_changed (std::string p)
{
	EditingContext::parameter_changed (p);

	if (p == "auto-loop") {
		update_loop_range_view ();
	} else if (p == "punch-in") {
		update_punch_range_view ();
	} else if (p == "punch-out") {
		update_punch_range_view ();
	} else if (p == "timecode-format") {
		update_just_timecode ();
	} else if (p == "show-region-fades") {
		update_region_fade_visibility ();
	} else if (p == "ripple-mode") {
		ripple_mode_selector.set_text (ripple_mode_strings [Config->get_ripple_mode()]);
	} else if (p == "edit-mode") {
		edit_mode_selector.set_text (edit_mode_strings [Config->get_edit_mode()]);
		if (Config->get_edit_mode()==Ripple) {
			ripple_mode_selector.show();
		} else {
			ripple_mode_selector.hide();
		}
	} else if (p == "show-track-meters") {
		toggle_meter_updating();
	} else if (p == "show-summary") {

		bool const s = _session->config.get_show_summary ();
		if (s) {
			_summary_hbox.show ();
		} else {
			_summary_hbox.hide ();
		}

		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleSummary"));
		if (tact->get_active () != s) {
			tact->set_active (s);
		}
	} else if (p == "show-group-tabs") {

		bool const s = _session ? _session->config.get_show_group_tabs () : true;
		if (s) {
			_group_tabs->show ();
		} else {
			_group_tabs->hide ();
		}

		reset_controls_layout_width ();

		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleGroupTabs"));
		if (tact->get_active () != s) {
			tact->set_active (s);
		}
	} else if (p == "timecode-offset" || p == "timecode-offset-negative") {
		update_just_timecode ();
	} else if (p == "sound-midi-notes") {
		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("sound-midi-notes"));
		bool s = UIConfiguration::instance().get_sound_midi_notes();
		if (tact->get_active () != s) {
			tact->set_active (s);
		}
	} else if (p == "show-region-gain") {
		set_gain_envelope_visibility ();
	} else if (p == "skip-playback") {
		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("toggle-skip-playback"));
		bool s = Config->get_skip_playback ();
		if (tact->get_active () != s) {
			tact->set_active (s);
		}
	} else if (p == "track-name-number") {
		queue_redisplay_track_views ();
	} else if (p == "default-time-domain") {
		stretch_marker_cb.set_sensitive (_session->config.get_default_time_domain () == Temporal::BeatTime);
		restore_ruler_visibility();  /* NOTE: if user has explicitly set rulers then this will have no effect */
	}
}

void
Editor::reset_canvas_action_sensitivity (bool onoff)
{
	if (_edit_point != EditAtMouse) {
		onoff = true;
	}

	for (vector<Glib::RefPtr<Action> >::iterator x = ActionManager::mouse_edit_point_requires_canvas_actions.begin();
	     x != ActionManager::mouse_edit_point_requires_canvas_actions.end(); ++x) {
		(*x)->set_sensitive (onoff);
	}
}

void
Editor::register_region_actions ()
{
	_region_actions = ActionManager::create_action_group (own_bindings, X_("Region"));

	/* PART 1: actions that operate on the selection, and for which the edit point type and location is irrelevant */

	/* Remove selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "remove-region", _("Remove"), sigc::mem_fun (*this, &Editor::remove_selected_regions));

	/* Offer dialogue box to rename the first selected region */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "rename-region", _("Rename..."), sigc::mem_fun (*this, &Editor::rename_region));

	/* Raise all selected regions by 1 layer */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "raise-region", _("Raise"), sigc::mem_fun (*this, &Editor::raise_region));

	/* Raise all selected regions to the top */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "raise-region-to-top", _("Raise to Top"), sigc::mem_fun (*this, &Editor::raise_region_to_top));

	/* Lower all selected regions by 1 layer */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "lower-region", _("Lower"), sigc::mem_fun (*this, &Editor::lower_region));

	/* Lower all selected regions to the bottom */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "lower-region-to-bottom", _("Lower to Bottom"), sigc::mem_fun (*this, &Editor::lower_region_to_bottom));

	/* Move selected regions to their original (`natural') position */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "naturalize-region", _("Move to Original Position"), sigc::mem_fun (*this, &Editor::naturalize_region));

	/* Change `locked' status of selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "region-lock", _("Lock"), sigc::mem_fun(*this, &Editor::region_lock));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "region-unlock", _("Unlock"), sigc::mem_fun(*this, &Editor::region_unlock));

	/* Toggle `locked' status of selected regions */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-lock", _("Lock (toggle)"), sigc::mem_fun(*this, &Editor::toggle_region_lock));
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-video-lock", _("Lock to Video"), sigc::mem_fun(*this, &Editor::toggle_region_video_lock));

	/* Remove sync points from selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "remove-region-sync", _("Remove Sync"), sigc::mem_fun(*this, &Editor::remove_region_sync));

	/* Mute or unmute selected regions */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-mute", _("Mute"), sigc::mem_fun(*this, &Editor::toggle_region_mute));

	/* Open the normalize dialogue to operate on the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "normalize-region", _("Normalize..."), sigc::mem_fun(*this, &Editor::normalize_region));

	/* Reverse selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "reverse-region", _("Reverse"), sigc::mem_fun (*this, &Editor::reverse_region));

	/* Split selected multi-channel regions into mono regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "split-multichannel-region", _("Make Mono Regions"), sigc::mem_fun (*this, &Editor::split_multichannel_region));

	/* Boost selected region gain */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "boost-region-gain", _("Boost Gain"), sigc::bind (sigc::mem_fun(*this, &Editor::adjust_region_gain), true));

	/* Cut selected region gain */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "cut-region-gain", _("Cut Gain"), sigc::bind (sigc::mem_fun(*this, &Editor::adjust_region_gain), false));

	/* Reset selected region gain */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "reset-region-gain", _("Reset Gain"), sigc::mem_fun(*this, &Editor::reset_region_gain));

	/* Open the pitch shift dialogue for any selected audio regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "pitch-shift-region", _("Pitch Shift..."), sigc::mem_fun (*this, &Editor::pitch_shift_region));

	/* Open the transpose dialogue for any selected MIDI regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "transpose-region", _("Transpose..."), sigc::mem_fun (*this, &Editor::transpose_region));

	/* Toggle selected region opacity */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-opaque-region", _("Opaque"), sigc::mem_fun (*this, &Editor::toggle_opaque_region));

	/* Toggle active status of selected regions' fade in */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-fade-in", _("Fade In"), sigc::bind (sigc::mem_fun (*this, &Editor::toggle_region_fades), 1));

	/* Toggle active status of selected regions' fade out */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-fade-out", _("Fade Out"), sigc::bind (sigc::mem_fun(*this, &Editor::toggle_region_fades), -1));

	/* Toggle active status of selected regions' fade in and out */
	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-fades", _("Fades"), sigc::bind (sigc::mem_fun(*this, &Editor::toggle_region_fades), 0));

	/* Duplicate selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "duplicate-region", _("Duplicate"), sigc::bind (sigc::mem_fun (*this, &Editor::duplicate_regions), 1));

	/* Open the dialogue to duplicate selected regions multiple times */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "multi-duplicate-region", _("Multi-Duplicate..."), sigc::bind (sigc::mem_fun(*this, &Editor::duplicate_range), true));

	/* Fill tracks with selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "region-fill-track", _("Fill Track"), sigc::mem_fun (*this, &Editor::region_fill_track));

	/* Set up the loop range from the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "set-loop-from-region", _("Set Loop Range"), sigc::bind (sigc::mem_fun (*this, &Editor::set_loop_from_region), false));

	/* Set up the loop range from the selected regions, and start playback of it */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "loop-region", _("Loop"), sigc::bind  (sigc::mem_fun(*this, &Editor::set_loop_from_region), true));

	/* Set the punch range from the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "set-punch-from-region", _("Set Punch"), sigc::mem_fun (*this, &Editor::set_punch_from_region));

	/* Add a single range marker around all selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "add-range-marker-from-region", _("Add Single Range Marker"), sigc::mem_fun (*this, &Editor::add_location_from_region));

	/* Add a range marker around each selected region */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "add-range-markers-from-region", _("Add Range Marker Per Region"), sigc::mem_fun (*this, &Editor::add_locations_from_region));

	/* Snap selected regions to the grid */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "snap-regions-to-grid", _("Snap Position to Grid"), sigc::mem_fun (*this, &Editor::snap_regions_to_grid));

	/* Close gaps in selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "close-region-gaps", _("Close Gaps"), sigc::mem_fun (*this, &Editor::close_region_gaps));

	/* Open the Rhythm Ferret dialogue for the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "show-rhythm-ferret", _("Rhythm Ferret..."), sigc::mem_fun (*this, &Editor::show_rhythm_ferret));

	/* Export the first selected region */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "export-region", _("Export..."), sigc::mem_fun (*this, &Editor::export_region));

	/* Separate under selected regions: XXX not sure what this does */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "separate-under-region", _("Separate Under"), sigc::mem_fun (*this, &Editor::separate_under_selected_regions));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "set-fade-in-length", _("Set Fade In Length"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_length), true));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "alternate-set-fade-in-length", _("Set Fade In Length"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_length), true));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "set-fade-out-length", _("Set Fade Out Length"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_length), false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "alternate-set-fade-out-length", _("Set Fade Out Length"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_length), false));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "set-tempo-from-region", _("Set Tempo from Region = Bar"), sigc::mem_fun (*this, &Editor::set_tempo_from_region));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "split-region-at-transients", _("Split at Percussion Onsets"), sigc::mem_fun(*this, &Editor::split_region_at_transients));

	/* Open the list editor dialogue for the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "show-region-list-editor", _("List Editor..."), sigc::mem_fun (*this, &Editor::show_midi_list_editor));

	/* Open the region properties dialogue for the selected regions */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "show-region-properties", _("Properties..."), sigc::mem_fun (*this, &Editor::show_region_properties));

	/* Edit the region in a separate region pianoroll window */
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "edit-region-pianoroll-window", _("Edit in separate window..."), sigc::mem_fun (*this, &Editor::edit_region_in_pianoroll_window));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "play-selected-regions", _("Play Selected Regions"), sigc::mem_fun(*this, &Editor::play_selected_region));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "tag-selected-regions", _("Tag Selected Regions"), sigc::mem_fun(*this, &Editor::tag_selected_region));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "bounce-regions-processed", _("Bounce (with processing)"), (sigc::bind (sigc::mem_fun (*this, &Editor::bounce_region_selection), true)));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "bounce-regions-unprocessed", _("Bounce (without processing)"), (sigc::bind (sigc::mem_fun (*this, &Editor::bounce_region_selection), false)));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "combine-regions", _("Combine"), sigc::mem_fun (*this, &Editor::combine_regions));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "uncombine-regions", _("Uncombine"), sigc::mem_fun (*this, &Editor::uncombine_regions));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "loudness-analyze-region", _("Loudness Analysis..."), sigc::mem_fun (*this, &Editor::loudness_analyze_region_selection));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "spectral-analyze-region", _("Spectral Analysis..."), sigc::mem_fun (*this, &Editor::spectral_analyze_region_selection));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "reset-region-gain-envelopes", _("Reset Envelope"), sigc::mem_fun (*this, &Editor::reset_region_gain_envelopes));

	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-gain-envelope-active", _("Envelope Active"), sigc::mem_fun (*this, &Editor::toggle_gain_envelope_active));

	register_toggle_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "toggle-region-polarity", _("Invert Polarity"), sigc::mem_fun (*this, &Editor::toggle_region_polarity));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "quantize-region", _("Quantize..."), sigc::mem_fun (*this, &Editor::quantize_region));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "legatize-region", _("Legatize"), sigc::bind(sigc::mem_fun (*this, &Editor::legatize_region), false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "deinterlace-midi", _("Deinterlace Into Layers"), sigc::mem_fun (*this, &Editor::deinterlace_selected_midi_regions));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "transform-region", _("Transform..."), sigc::mem_fun (*this, &Editor::transform_region));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "remove-overlap", _("Remove Overlap"), sigc::bind(sigc::mem_fun (*this, &Editor::legatize_region), true));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "insert-patch-change", _("Insert Patch Change..."), sigc::bind (sigc::mem_fun (*this, &Editor::insert_patch_change), false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "insert-patch-change-context", _("Insert Patch Change..."), sigc::bind (sigc::mem_fun (*this, &Editor::insert_patch_change), true));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "fork-region", _("Unlink all selected regions"), sigc::mem_fun (*this, &Editor::fork_selected_regions));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "fork-regions-from-unselected", _("Unlink from unselected"), sigc::mem_fun (*this, &Editor::fork_regions_from_unselected));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "strip-region-silence", _("Strip Silence..."), sigc::mem_fun (*this, &Editor::strip_region_silence));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions), "set-selection-from-region", _("Set Range Selection"), sigc::mem_fun (*this, &Editor::set_selection_from_region));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "nudge-forward", _("Nudge Later"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_forward), false, false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "alternate-nudge-forward", _("Nudge Later"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_forward), false, false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "nudge-backward", _("Nudge Earlier"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_backward), false, false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "alternate-nudge-backward", _("Nudge Earlier"), sigc::bind (sigc::mem_fun (*this, &Editor::nudge_backward), false, false));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "sequence-regions", _("Sequence Regions"), sigc::mem_fun (*this, &Editor::sequence_regions));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "nudge-forward-by-capture-offset", _("Nudge Later by Capture Offset"), sigc::mem_fun (*this, &Editor::nudge_forward_capture_offset));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "nudge-backward-by-capture-offset", _("Nudge Earlier by Capture Offset"), sigc::mem_fun (*this, &Editor::nudge_backward_capture_offset));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "trim-region-to-loop", _("Trim to Loop"), sigc::mem_fun (*this, &Editor::trim_region_to_loop));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "trim-region-to-punch", _("Trim to Punch"), sigc::mem_fun (*this, &Editor::trim_region_to_punch));

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "trim-to-previous-region", _("Trim to Previous"), sigc::mem_fun(*this, &Editor::trim_region_to_previous_region_end));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EnteredRegions), "trim-to-next-region", _("Trim to Next"), sigc::mem_fun(*this, &Editor::trim_region_to_next_region_start));

	/* PART 2: actions that are not related to the selection, but for which the edit point type and location is important */

	register_region_action (_region_actions, RegionActionTarget (ListSelection), "insert-region-from-source-list", _("Insert Region from Source List"), sigc::bind (sigc::mem_fun (*this, &Editor::insert_source_list_selection), 1));

	/* PART 3: actions that operate on the selection and also require the edit point location */

	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "make-region-markers-cd", _("Convert Region Cue Markers to CD Markers"), sigc::bind (sigc::mem_fun (*this, &Editor::make_region_markers_global), true));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "make-region-markers-global", _("Convert Region Cue Markers to Location Markers"), sigc::bind (sigc::mem_fun (*this, &Editor::make_region_markers_global), false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "add-region-cue-marker", _("Add Region Cue Marker"), sigc::mem_fun (*this, &Editor::add_region_marker));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "clear-region-cue-markers", _("Clear Region Cue Markers"), sigc::mem_fun (*this, &Editor::clear_region_markers));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "set-region-sync-position", _("Set Sync Position"), sigc::mem_fun (*this, &Editor::set_region_sync_position));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "place-transient", _("Place Transient"), sigc::mem_fun (*this, &Editor::place_transient));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "trim-front", _("Trim Start at Edit Point"), sigc::mem_fun (*this, &Editor::trim_region_front));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "trim-back", _("Trim End at Edit Point"), sigc::mem_fun (*this, &Editor::trim_region_back));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-start", _("Align Start"), sigc::bind (sigc::mem_fun(*this, &Editor::align_regions), ARDOUR::Start));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-start-relative", _("Align Start Relative"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions_relative), ARDOUR::Start));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-end", _("Align End"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions), ARDOUR::End));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-end-relative", _("Align End Relative"), sigc::bind (sigc::mem_fun(*this, &Editor::align_regions_relative), ARDOUR::End));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-sync", _("Align Sync"), sigc::bind (sigc::mem_fun(*this, &Editor::align_regions), ARDOUR::SyncPoint));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "align-regions-sync-relative", _("Align Sync Relative"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions_relative), ARDOUR::SyncPoint));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "choose-top-region", _("Choose Top..."), sigc::bind (sigc::mem_fun (*this, &Editor::change_region_layering_order), false));
	register_region_action (_region_actions, RegionActionTarget (SelectedRegions|EditPointRegions), "choose-top-region-context-menu", _("Choose Top..."), sigc::bind (sigc::mem_fun (*this, &Editor::change_region_layering_order), true));

	/* desensitize them all by default. region selection will change this */
	sensitize_all_region_actions (false);
}

void
Editor::automation_create_point_at_edit_point ()
{
	AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (entered_track);
	if (!atv) {
		return;
	}

	timepos_t where (get_preferred_edit_position());;
	GdkEvent event;

	event.type = GDK_KEY_PRESS;
	event.button.button = 1;
	event.button.state = 0;

	atv->line()->add (atv->control(), &event, where, atv->line()->the_list()->eval (where), false, true);
}

void
Editor::automation_lower_points ()
{
	PointSelection& points (selection->points);

	if (points.empty()) {
		return;
	}

	AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (entered_track);

	if (!atv) {
		return;
	}

	begin_reversible_command (_("automation event lower"));
	add_command (new MementoCommand<AutomationList> (atv->line()->memento_command_binder(), &atv->line()->the_list()->get_state(), 0));
	atv->line()->the_list()->freeze ();
	for (auto & p : points) {
		atv->line()->the_list()->modify (p->model(), (*p->model())->when, max (0.0, (*p->model())->value - 0.1));
	}
	atv->line()->the_list()->thaw ();
	add_command (new MementoCommand<AutomationList>(atv->line()->memento_command_binder (), 0, &atv->line()->the_list()->get_state()));
	commit_reversible_command ();
}

void
Editor::automation_raise_points ()
{
	PointSelection& points (selection->points);

	if (points.empty()) {
		return;
	}

	AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (entered_track);

	if (!atv) {
		return;
	}

	begin_reversible_command (_("automation event raise"));
	add_command (new MementoCommand<AutomationList> (atv->line()->memento_command_binder(), &atv->line()->the_list()->get_state(), 0));
	atv->line()->the_list()->freeze ();
	for (auto & p : points) {
		atv->line()->the_list()->modify (p->model(), (*p->model())->when, min (1.0, (*p->model())->value + 0.1));
	}
	atv->line()->the_list()->thaw ();
	add_command (new MementoCommand<AutomationList>(atv->line()->memento_command_binder (), 0, &atv->line()->the_list()->get_state()));
	commit_reversible_command ();
}

void
Editor::automation_move_points_later ()
{
	PointSelection& points (selection->points);

	if (points.empty()) {
		return;
	}

	AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (entered_track);

	if (!atv) {
		return;
	}

	begin_reversible_command (_("automation points move later"));
	add_command (new MementoCommand<AutomationList> (atv->line()->memento_command_binder(), &atv->line()->the_list()->get_state(), 0));
	atv->line()->the_list()->freeze ();
	for (auto & p : points) {
		timepos_t model_time ((*p->model())->when);
		model_time += Temporal::BBT_Offset (0, 1, 0);
		atv->line()->the_list()->modify (p->model(), model_time, (*p->model())->value);
	}
	atv->line()->the_list()->thaw ();
	add_command (new MementoCommand<AutomationList>(atv->line()->memento_command_binder (), 0, &atv->line()->the_list()->get_state()));
	commit_reversible_command ();
}

void
Editor::automation_move_points_earlier ()
{
	PointSelection& points (selection->points);

	if (points.empty()) {
		return;
	}

	AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (entered_track);

	if (!atv) {
		return;
	}

	begin_reversible_command (_("automation points move earlier"));
	add_command (new MementoCommand<AutomationList> (atv->line()->memento_command_binder(), &atv->line()->the_list()->get_state(), 0));
	atv->line()->the_list()->freeze ();
	for (auto & p : points) {
		timepos_t model_time ((*p->model())->when);
		model_time = model_time.earlier (Temporal::BBT_Offset (0, 1, 0));
		atv->line()->the_list()->modify (p->model(), model_time, (*p->model())->value);
	}
	atv->line()->the_list()->thaw ();
	add_command (new MementoCommand<AutomationList>(atv->line()->memento_command_binder (), 0, &atv->line()->the_list()->get_state()));
	commit_reversible_command ();
}
