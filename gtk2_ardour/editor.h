/*
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2005-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2006-2009 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2008-2011 Sakari Bergen <sakari.bergen@beatwaves.net>
 * Copyright (C) 2008 Hans Baier <hansfbaier@googlemail.com>
 * Copyright (C) 2013-2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2017 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2014-2019 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015-2019 Damien Zammit <damien@zamaudio.com>
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

#pragma once

#include <sys/time.h>

#include <cmath>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <optional>

#include <ytkmm/comboboxtext.h>
#include <ytkmm/layout.h>

#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/dndtreeview.h"

#include "pbd/controllable.h"
#include "pbd/signals.h"

#include "ardour/import_status.h"
#include "ardour/tempo.h"
#include "ardour/location.h"
#include "ardour/types.h"

#include "canvas/fwd.h"
#include "canvas/ruler.h"

#include "widgets/ardour_button.h"
#include "widgets/ardour_dropdown.h"
#include "widgets/ardour_spacer.h"
#include "widgets/metabutton.h"
#include "widgets/pane.h"

#include "application_bar.h"
#include "ardour_dialog.h"
#include "public_editor.h"
#include "editing.h"
#include "enums.h"
#include "editor_items.h"
#include "region_selection.h"
#include "selection_memento.h"
#include "trigger_clip_picker.h"
#include "tempo_curve.h"

#include "ptformat/ptformat.h"

namespace Gtkmm2ext {
	class Bindings;
}

namespace Evoral {
	class SMF;
}

namespace ARDOUR {
	class AudioPlaylist;
	class AudioRegion;
	class AudioTrack;
	class ChanCount;
	class Filter;
	class Location;
	class MidiOperator;
	class MidiRegion;
	class MidiTrack;
	class Playlist;
	class Region;
	class RouteGroup;
	class Session;
	class Track;
}

class AnalysisWindow;
class AudioClock;
class AudioRegionView;
class AudioStreamView;
class AudioTimeAxisView;
class EditorAutomationLine;
class AutomationSelection;
class AutomationTimeAxisView;
class BundleManager;
class ControlPoint;
class CursorContext;
class DragManager;
class EditNoteDialog;
class EditorCursor;
class EditorGroupTabs;
class EditorLocations;
class EditorRegions;
class EditorSections;
class EditorSources;
class EditorRoutes;
class EditorRouteGroups;
class EditorSnapshots;
class EditorSummary;
class GUIObjectState;
class ArdourMarker;
class MidiRegionView;
class MidiView;
class MidiExportDialog;
class MixerStrip;
class MouseCursors;
class NoteBase;
class Pianoroll;
class PluginSelector;
class ProgressReporter;
class QuantizeDialog;
class RegionPeakCursor;
class RhythmFerret;
class RulerDialog;
class SectionBox;
class Selection;
class SelectionPropertiesBox;
class SoundFileOmega;
class StreamView;
class GridLines;
class TimeAxisView;
class TimeInfoBox;
class TimeFXDialog;
class TimeSelection;
class RegionLayeringOrderEditor;
class VerboseCursor;

class Editor : public PublicEditor, public PBD::ScopedConnectionList
{
public:
	Editor ();
	~Editor ();

	void             set_session (ARDOUR::Session*);

	Gtk::Window* use_own_window (bool and_fill_it);

	void             first_idle ();
	virtual bool     have_idled () const { return _have_idled; }

	bool pending_locate_request() const { return _pending_locate_request; }

	samplepos_t leftmost_sample() const { return _leftmost_sample; }

	samplecnt_t current_page_samples() const {
		return (samplecnt_t) _visible_canvas_width* samples_per_pixel;
	}

	double visible_canvas_height () const {
		return _visible_canvas_height;
	}
	double trackviews_height () const;

	XMLNode& get_state () const;
	int set_state (const XMLNode&, int version);

	void step_mouse_mode (bool next);
	bool internal_editing() const;

	void remove_midi_note (ArdourCanvas::Item*, GdkEvent*);

	void foreach_time_axis_view (sigc::slot<void,TimeAxisView&>);
	void add_to_idle_resize (TimeAxisView*, int32_t);

	StripableTimeAxisView* get_stripable_time_axis_by_id (const PBD::ID& id) const;

	void consider_auditioning (std::shared_ptr<ARDOUR::Region>);
	void hide_a_region (std::shared_ptr<ARDOUR::Region>);
	void show_a_region (std::shared_ptr<ARDOUR::Region>);

#ifdef USE_RUBBERBAND
	std::vector<std::string> rb_opt_strings;
	int rb_current_opt;
#endif

	/* things that need to be public to be used in the main menubar */

	void new_region_from_selection ();
	void separate_regions_between (const TimeSelection&);
	void separate_region_from_selection ();
	void separate_under_selected_regions ();
	void separate_region_from_punch ();
	void separate_region_from_loop ();
	void separate_regions_using_location (ARDOUR::Location&);
	void transition_to_rolling (bool forward);

	/* selection */

	Selection& get_selection() const { return *selection; }
	bool get_selection_extents (Temporal::timepos_t &start, Temporal::timepos_t &end) const;  // the time extents of the current selection, whether Range, Region(s), Control Points, or Notes
	Selection& get_cut_buffer() const { return *cut_buffer; }

	std::list<SelectableOwner*> selectable_owners();

	void get_regionviews_at_or_after (Temporal::timepos_t const &, RegionSelection&);

	void set_selection (std::list<Selectable*>, ARDOUR::SelectionOperation);

	std::shared_ptr<ARDOUR::Route> current_mixer_stripable () const;

	bool extend_selection_to_track (TimeAxisView&);

	void edit_region_in_pianoroll_window ();

	void play_selection ();
	void maybe_locate_with_edit_preroll (samplepos_t);
	void play_with_preroll ();
	void rec_with_preroll ();
	void rec_with_count_in ();
	void select_all_in_track (ARDOUR::SelectionOperation op);
	void select_all_objects (ARDOUR::SelectionOperation op);
	void invert_selection_in_track ();
	void invert_selection ();
	void deselect_all ();
	long select_range (Temporal::timepos_t const & , Temporal::timepos_t const &);

	void set_selected_regionview_from_region_list (std::shared_ptr<ARDOUR::Region> region, ARDOUR::SelectionOperation op = ARDOUR::SelectionSet);

	void remove_tracks ();

	/* tempo */

	// void update_grid ();

	/* analysis window */

	void loudness_analyze_region_selection();
	void loudness_analyze_range_selection();

	void spectral_analyze_region_selection();
	void spectral_analyze_range_selection();

	/* export */

	void export_audio ();
	void stem_export ();
	void export_selection ();
	void export_range ();
	void export_region ();
	void quick_export ();
	void surround_export ();

	/* export for analysis only */
	void loudness_assistant (bool);
	void loudness_assistant_marker ();
	void measure_master_loudness (samplepos_t start, samplepos_t end, bool);

	bool process_midi_export_dialog (MidiExportDialog& dialog, std::shared_ptr<ARDOUR::MidiRegion> midi_region);

	void ensure_time_axis_view_is_visible (TimeAxisView const & tav, bool at_top);
	void tav_zoom_step (bool coarser);
	void tav_zoom_smooth (bool coarser, bool force_all);

	void                         cycle_marker_click_behavior ();
	void                         set_marker_click_behavior (Editing::MarkerClickBehavior);
	Editing::MarkerClickBehavior get_marker_click_behavior () const { return marker_click_behavior; }

	/* stuff that AudioTimeAxisView and related classes use */

	void clear_playlist (std::shared_ptr<ARDOUR::Playlist>);

	void clear_grouped_playlists (RouteUI* v);

	void get_onscreen_tracks (TrackViewList&);

	Width editor_mixer_strip_width;
	void maybe_add_mixer_strip_width (XMLNode&) const;
	void show_editor_mixer (bool yn);
	void create_editor_mixer ();
	void showhide_att_left (bool);
	void set_selected_mixer_strip (TimeAxisView&);
	void mixer_strip_width_changed ();
	void hide_track_in_display (TimeAxisView* tv, bool apply_to_selection = false);
	void show_track_in_display (TimeAxisView* tv, bool move_into_view = false);
	void tempo_curve_selected (Temporal::TempoPoint const * ts, bool yn);

	/* nudge is initiated by transport controls owned by ARDOUR_UI */

	Temporal::timecnt_t get_nudge_distance (Temporal::timepos_t const & pos, Temporal::timecnt_t& next) const;
	Temporal::timecnt_t get_paste_offset (Temporal::timepos_t const & pos, unsigned paste_count, Temporal::timecnt_t const & duration);

	void nudge_forward (bool next, bool force_playhead);
	void nudge_backward (bool next, bool force_playhead);

	/* nudge initiated from context menu */

	void nudge_forward_capture_offset ();
	void nudge_backward_capture_offset ();

	void sequence_regions ();

	/* playhead/screen stuff */

	void set_stationary_playhead (bool yn);
	void toggle_stationary_playhead ();
	bool stationary_playhead() const { return _stationary_playhead; }

	bool dragging_playhead () const { return _dragging_playhead; }

	void toggle_zero_line_visibility ();
	void set_summary ();
	void set_group_tabs ();

	/* returns the left-most and right-most time that the gui should allow the user to scroll to */
	std::pair <Temporal::timepos_t,Temporal::timepos_t> session_gui_extents (bool use_extra = true) const;

	/* RTAV Automation display option */
	bool show_touched_automation () const;

	/* fades */

	void toggle_region_fades (int dir);
	void update_region_fade_visibility ();

	/* floating windows/transient */

	void ensure_float (Gtk::Window&);

	void scroll_tracks_down_line ();
	void scroll_tracks_up_line ();

	bool scroll_up_one_track (bool skip_child_views = false);
	bool scroll_down_one_track (bool skip_child_views = false);

	void scroll_left_step ();
	void scroll_right_step ();

	void scroll_left_half_page ();
	void scroll_right_half_page ();

	void select_topmost_track ();

	void cleanup_regions ();

	void prepare_for_cleanup ();
	void finish_cleanup ();

	void maximise_editing_space();
	void restore_editing_space();

	double get_y_origin () const;
	void reposition_and_zoom (samplepos_t, double);

	void toggle_meter_updating();

	void show_rhythm_ferret();

	void goto_visual_state (uint32_t);
	void save_visual_state (uint32_t);

	TrackViewList const & get_track_views () const {
		return track_views;
	}

	void do_import (std::vector<std::string>              paths,
	                Editing::ImportDisposition            disposition,
	                Editing::ImportMode                   mode,
	                ARDOUR::SrcQuality                    quality,
	                ARDOUR::MidiTrackNameSource           mts,
	                ARDOUR::MidiTempoMapDisposition       mtd,
	                Temporal::timepos_t&                  pos,
	                std::shared_ptr<ARDOUR::PluginInfo> instrument = std::shared_ptr<ARDOUR::PluginInfo>(),
	                std::shared_ptr<ARDOUR::Track>      track = std::shared_ptr<ARDOUR::Track>(),
	                bool with_markers = false);

	void do_embed (std::vector<std::string>              paths,
	               Editing::ImportDisposition            disposition,
	               Editing::ImportMode                   mode,
	               Temporal::timepos_t&                  pos,
	               std::shared_ptr<ARDOUR::PluginInfo> instrument = std::shared_ptr<ARDOUR::PluginInfo>(),
	               std::shared_ptr<ARDOUR::Track>      track = std::shared_ptr<ARDOUR::Track>());

	void get_regionview_corresponding_to (std::shared_ptr<ARDOUR::Region> region, std::vector<RegionView*>& regions);

	void get_regionviews_by_id (PBD::ID const id, RegionSelection & regions) const;
	void get_per_region_note_selection (std::list<std::pair<PBD::ID, std::set<std::shared_ptr<Evoral::Note<Temporal::Beats> > > > >&) const;

	TrackViewList axis_views_from_routes (std::shared_ptr<ARDOUR::RouteList>) const;

	void set_snapped_cursor_position (Temporal::timepos_t const & pos);

	void begin_selection_op_history ();
	void begin_reversible_selection_op (std::string cmd_name);
	void commit_reversible_selection_op ();
	void abort_reversible_selection_op ();
	void undo_selection_op ();
	void redo_selection_op ();
	void add_command (PBD::Command* cmd);
	void add_commands (std::vector<PBD::Command*> cmds);

	PBD::HistoryOwner& history();

	void begin_reversible_command (std::string cmd_name);
	void begin_reversible_command (GQuark);
	void abort_reversible_command ();
	void commit_reversible_command ();

	MixerStrip* get_current_mixer_strip () const {
		return current_mixer_strip;
	}

	void maybe_autoscroll (bool, bool, bool);
	bool autoscroll_active() const;

	void set_current_trimmable (std::shared_ptr<ARDOUR::Trimmable>);
	void set_current_movable (std::shared_ptr<ARDOUR::Movable>);

	double clamp_verbose_cursor_x (double);
	double clamp_verbose_cursor_y (double);

	void get_pointer_position (double &, double &) const;

	TimeAxisView* stepping_axis_view () {
		return _stepping_axis_view;
	}

	void set_stepping_axis_view (TimeAxisView* v) {
		_stepping_axis_view = v;
	}

	ArdourCanvas::Container* get_trackview_group () const { return _trackview_group; }
	ArdourCanvas::Container* get_noscroll_group () const { return no_scroll_group; }
	ArdourCanvas::ScrollGroup* get_hscroll_group () const { return h_scroll_group; }
	ArdourCanvas::ScrollGroup* get_hvscroll_group () const { return hv_scroll_group; }
	ArdourCanvas::ScrollGroup* get_cursor_scroll_group () const { return cursor_scroll_group; }
	ArdourCanvas::Container* get_drag_motion_group () const { return _drag_motion_group; }

	ArdourCanvas::GtkCanvasViewport* get_canvas_viewport () const;
	ArdourCanvas::GtkCanvas* get_canvas () const;

	void override_visible_track_count ();

	/* Ruler metrics methods */

	void metric_get_timecode (std::vector<ArdourCanvas::Ruler::Mark>&, int64_t, int64_t, gint);
	void metric_get_bbt (std::vector<ArdourCanvas::Ruler::Mark>&, int64_t, int64_t, gint);
	void metric_get_samples (std::vector<ArdourCanvas::Ruler::Mark>&, int64_t, int64_t, gint);
	void metric_get_minsec (std::vector<ArdourCanvas::Ruler::Mark>&, int64_t, int64_t, gint);

	/* editing operations that need to be public */
	void split_regions_at (Temporal::timepos_t const & , RegionSelection&);
	void split_region_at_points (std::shared_ptr<ARDOUR::Region>, ARDOUR::AnalysisFeatureList&, bool can_ferret, bool select_new = false);
	RegionSelection get_regions_from_selection_and_mouse (Temporal::timepos_t const &);
	void do_remove_gaps ();
	void remove_gaps (Temporal::timecnt_t const & threshold, Temporal::timecnt_t const & leave, bool markers_too);

	void mouse_brush_insert_region (RegionView*, Temporal::timepos_t const & pos);

	void mouse_add_new_tempo_event (Temporal::timepos_t where);
	void mouse_add_new_meter_event (Temporal::timepos_t where);
	void edit_tempo_section (Temporal::TempoPoint&);
	void edit_meter_section (Temporal::MeterPoint&);
	void mouse_add_bbt_marker_event (Temporal::timepos_t where);
	void add_bbt_marker_at_playhead_cursor ();

	void edit_bbt (Temporal::MusicTimePoint&);

	bool should_ripple () const;
	bool should_ripple_all () const;  /* RippleAll will ripple all similar regions and the timeline markers */
	void do_ripple (std::shared_ptr<ARDOUR::Playlist>, Temporal::timepos_t const &, Temporal::timecnt_t const &, ARDOUR::RegionList* exclude, ARDOUR::PlaylistSet const& affected_pls, bool add_to_command);
	void do_ripple (std::shared_ptr<ARDOUR::Playlist>, Temporal::timepos_t const &, Temporal::timecnt_t const &, std::shared_ptr<ARDOUR::Region> exclude, bool add_to_command);
	void ripple_marks (std::shared_ptr<ARDOUR::Playlist> target_playlist, Temporal::timepos_t at, Temporal::timecnt_t const & distance);
	void get_markers_to_ripple (std::shared_ptr<ARDOUR::Playlist> target_playlist, Temporal::timepos_t const & pos, std::vector<ArdourMarker*>& markers);
	Temporal::timepos_t effective_ripple_mark_start (std::shared_ptr<ARDOUR::Playlist> target_playlist, Temporal::timepos_t pos);

	void add_region_marker ();
	void clear_region_markers ();
	void remove_region_marker (ARDOUR::CueMarker&);
	void make_region_markers_global (bool as_cd_markers);

	bool rb_click (GdkEvent*, Temporal::timepos_t const &);
	void line_drag_click (GdkEvent*, Temporal::timepos_t const &, double);

	void focus_on_clock();

	void set_zoom_focus (Editing::ZoomFocus);

	void temporal_zoom_selection (Editing::ZoomAxis);
	void temporal_zoom_session ();
	void temporal_zoom_extents ();

	void find_and_display_track ();

protected:
	void map_transport_state ();
	void map_position_change (samplepos_t);
	void transport_looped ();

	void on_realize();

	void suspend_route_redisplay ();
	void resume_route_redisplay ();

	RegionSelection region_selection();

	void do_undo (uint32_t n);
	void do_redo (uint32_t n);

	Temporal::timepos_t _get_preferred_edit_position (Editing::EditIgnoreOption, bool use_context_click, bool from_outside_canvas);

private:

	void color_handler ();
	void dpi_reset ();
	bool constructed;

	// to keep track of the playhead position for control_scroll
	std::optional<samplepos_t> _control_scroll_target;

	Gtk::HBox                    _bottom_hbox;
	SelectionPropertiesBox*      _properties_box;
	Pianoroll*                   _pianoroll;

	typedef std::pair<TimeAxisView*,XMLNode*> TAVState;

	struct VisualState {
		VisualState (bool with_tracks);
		~VisualState ();
		double              y_position;
		samplecnt_t         samples_per_pixel;
		samplepos_t        _leftmost_sample;
		Editing::ZoomFocus  zoom_focus;
		GUIObjectState*     gui_state;
	};

	std::list<VisualState*> undo_visual_stack;
	std::list<VisualState*> redo_visual_stack;
	VisualState* current_visual_state (bool with_tracks = true);
	void undo_visual_state ();
	void redo_visual_state ();
	void use_visual_state (VisualState&);
	bool no_save_visual;
	void swap_visual_state ();

	std::vector<VisualState*> visual_states;
	void start_visual_state_op (uint32_t n);
	void cancel_visual_state_op (uint32_t n);

	void set_samples_per_pixel (samplecnt_t);
	void on_samples_per_pixel_changed ();

	Editing::MouseMode effective_mouse_mode () const;
	void use_appropriate_mouse_mode_for_sections ();

	Editing::MarkerClickBehavior marker_click_behavior;

	enum JoinObjectRangeState {
		JOIN_OBJECT_RANGE_NONE,
		/** `join object/range' mode is active and the mouse is over a place where object mode should happen */
		JOIN_OBJECT_RANGE_OBJECT,
		/** `join object/range' mode is active and the mouse is over a place where range mode should happen */
		JOIN_OBJECT_RANGE_RANGE
	};

	JoinObjectRangeState _join_object_range_state;

	void update_join_object_range_location (double);

	Gtk::VBox                 _editor_list_vbox;
	Gtk::Notebook             _the_notebook;
	ArdourWidgets::MetaButton _notebook_tab1;
	ArdourWidgets::MetaButton _notebook_tab2;

	void add_notebook_page (std::string const&, std::string const&, Gtk::Widget&);

	ArdourWidgets::VPane editor_summary_pane;

	Gtk::EventBox meter_base;
	Gtk::EventBox marker_base;
	Gtk::HBox     marker_box;
	Gtk::VBox     scrollers_rulers_markers_box;

	void location_changed (ARDOUR::Location*);
	void location_flags_changed (ARDOUR::Location*);
	void refresh_location_display ();
	void update_section_rects ();
	bool section_rect_event (GdkEvent*, ARDOUR::Location*, ArdourCanvas::Rectangle*, std::string);
	void refresh_location_display_internal (const ARDOUR::Locations::LocationList&);
	void add_new_location (ARDOUR::Location*);
	ArdourCanvas::Container* add_new_location_internal (ARDOUR::Location*);
	void location_gone (ARDOUR::Location*);
	void loop_location_changed (ARDOUR::Location*);
	void remove_marker (ArdourCanvas::Item&);
	void remove_marker (ArdourMarker*);
	gint really_remove_global_marker (ARDOUR::Location* loc);
	gint really_remove_region_marker (ArdourMarker*);
	void goto_nth_marker (int nth);
	void jump_to_loop_marker (bool start);
	void trigger_script (int nth);
	void trigger_script_by_name (const std::string script_name, const std::string args = "");
	void toggle_marker_lines ();
	void set_marker_line_visibility (bool);
	void update_selection_markers ();
	void update_section_box ();
	void capture_sources_changed (bool);

	void jump_forward_to_mark_flagged (ARDOUR::Location::Flags, ARDOUR::Location::Flags, ARDOUR::Location::Flags);
	void jump_backward_to_mark_flagged (ARDOUR::Location::Flags, ARDOUR::Location::Flags, ARDOUR::Location::Flags);

	struct LocationMarkers {
		ArdourMarker* start;
		ArdourMarker* end;
		bool    valid;

		LocationMarkers () : start(0), end(0), valid (true) {}

		~LocationMarkers ();

		void hide ();
		void show ();

		void set_show_lines (bool);
		void set_selected (bool);
		void set_entered (bool);
		void setup_lines ();

		void set_name (const std::string&);
		void set_position (Temporal::timepos_t const & start, Temporal::timepos_t const & end = Temporal::timepos_t());
		void set_color (std::string const&);
	};

	void reparent_location_markers (LocationMarkers*, ArdourCanvas::Item*);

	ArdourCanvas::Duple upper_left() const;

	LocationMarkers*  find_location_markers (ARDOUR::Location*) const;
	ARDOUR::Location* find_location_from_marker (ArdourMarker*, bool& is_start) const;
	ArdourMarker* find_marker_from_location_id (PBD::ID const&, bool) const;
	TempoMarker* find_marker_for_tempo (Temporal::TempoPoint const &);
	MeterMarker* find_marker_for_meter (Temporal::MeterPoint const &);
	bool _show_marker_lines;

	typedef std::map<ARDOUR::Location*,LocationMarkers*> LocationMarkerMap;
	LocationMarkerMap location_markers;

	void update_marker_labels ();
	void update_marker_labels (ArdourCanvas::Item*);
	void check_marker_label (ArdourMarker*);

	/** A set of lists of Markers that are in each of the canvas groups
	 *  for the marker sections at the top of the editor.  These lists
	 *  are kept sorted in time order between marker movements, so that after
	 *  a marker has moved we can decide whether we need to update the labels
	 *  for all markers or for just a few.
	 */
	std::map<ArdourCanvas::Item*, std::list<ArdourMarker*> > _sorted_marker_lists;
	void remove_sorted_marker (ArdourMarker*);

	void hide_marker (ArdourCanvas::Item*, GdkEvent*);
	void clear_marker_display ();
	void mouse_add_new_range (Temporal::timepos_t);
	void mouse_add_new_loop (Temporal::timepos_t);
	void mouse_add_new_punch (Temporal::timepos_t);
	bool choose_new_marker_name(std::string &name, bool is_range=false);
	void update_marker_display ();
	void ensure_marker_updated (LocationMarkers* lam, ARDOUR::Location* location);
	void update_all_marker_lanes ();

	TimeAxisView*      clicked_axisview;
	RouteTimeAxisView* clicked_routeview;
	/** The last RegionView that was clicked on, or 0 if the last click was not
	 * on a RegionView.  This is set up by the canvas event handlers in
	 * editor_canvas_events.cc
	 */
	RegionView*        clicked_regionview;
	RegionSelection    latest_regionviews;
	uint32_t           clicked_selection;
	ControlPoint*      clicked_control_point;

	void sort_track_selection (TrackViewList&);

	void get_equivalent_regions (RegionView* rv, std::vector<RegionView*> &, PBD::PropertyID) const;
	void get_all_equivalent_regions (RegionView* rv, std::vector<RegionView*> &) const;
	RegionSelection get_equivalent_regions (RegionSelection &, PBD::PropertyID) const;
	RegionView* regionview_from_region (std::shared_ptr<ARDOUR::Region>) const;
	RouteTimeAxisView* rtav_from_route (std::shared_ptr<ARDOUR::Route>) const;

	void mapover_tracks_with_unique_playlists (sigc::slot<void,RouteTimeAxisView&,uint32_t> sl, TimeAxisView*, PBD::PropertyID) const;
	void mapover_all_tracks_with_unique_playlists (sigc::slot<void,RouteTimeAxisView&,uint32_t>) const;
	void mapped_get_equivalent_regions (RouteTimeAxisView&, uint32_t, RegionView*, std::vector<RegionView*>*) const;

	void mapover_grouped_routes (sigc::slot<void, RouteUI&> sl, RouteUI*, PBD::PropertyID) const;
	void mapover_armed_routes (sigc::slot<void, RouteUI&> sl) const;
	void mapover_selected_routes (sigc::slot<void, RouteUI&> sl) const;
	void mapover_all_routes (sigc::slot<void, RouteUI&> sl) const;

	void mapped_select_playlist_matching (RouteUI&, std::weak_ptr<ARDOUR::Playlist> pl);
	void mapped_use_new_playlist (RouteUI&, std::string name, std::string gid, bool copy, std::vector<std::shared_ptr<ARDOUR::Playlist> > const &);
	void mapped_clear_playlist (RouteUI&);

	void new_playlists_for_all_tracks(bool copy);
	void new_playlists_for_grouped_tracks(RouteUI* v, bool copy);
	void new_playlists_for_selected_tracks(bool copy);
	void new_playlists_for_armed_tracks(bool copy);

	void button_selection (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type);
	bool button_release_can_deselect;

	void catch_vanishing_regionview (RegionView*);

	void set_selected_track (TimeAxisView&, ARDOUR::SelectionOperation op = ARDOUR::SelectionSet, bool no_remove=false);
	void select_all_visible_lanes ();
	void select_all_tracks ();
	bool select_all_internal_edit (ARDOUR::SelectionOperation);

	bool set_selected_control_point_from_click (bool press, ARDOUR::SelectionOperation op = ARDOUR::SelectionSet);
	void set_selected_track_from_click (bool press, ARDOUR::SelectionOperation op = ARDOUR::SelectionSet, bool no_remove=false);
	void set_selected_track_as_side_effect (ARDOUR::SelectionOperation op, PBD::Controllable::GroupControlDisposition gcd = PBD::Controllable::UseGroup);
	bool set_selected_regionview_from_click (bool press, ARDOUR::SelectionOperation op = ARDOUR::SelectionSet);

	bool set_selected_regionview_from_map_event (GdkEventAny*, StreamView*, std::weak_ptr<ARDOUR::Region>);
	void collect_new_region_view (RegionView*);
	void collect_and_select_new_region_view (RegionView*);

	Gtk::Menu section_box_menu;
	Gtk::Menu track_context_menu;
	Gtk::Menu track_region_context_menu;
	Gtk::Menu track_selection_context_menu;

	GdkEvent context_click_event;

	void popup_track_context_menu (int, int, ItemType, bool);
	Gtk::Menu* build_track_context_menu ();
	Gtk::Menu* build_track_bus_context_menu ();
	Gtk::Menu* build_track_region_context_menu ();
	Gtk::Menu* build_track_selection_context_menu ();
	void add_section_context_items (Gtk::Menu_Helpers::MenuList&);
	void add_dstream_context_items (Gtk::Menu_Helpers::MenuList&);
	void add_bus_context_items (Gtk::Menu_Helpers::MenuList&);
	void add_region_context_items (Gtk::Menu_Helpers::MenuList&, std::shared_ptr<ARDOUR::Track>);
	void add_selection_context_items (Gtk::Menu_Helpers::MenuList&, bool time_selection_only = false);
	Gtk::MenuItem* _popup_region_menu_item;

	void popup_control_point_context_menu (ArdourCanvas::Item*, GdkEvent*);
	Gtk::Menu _control_point_context_menu;

	void initial_display ();
	void add_stripables (ARDOUR::StripableList&);
	void add_routes (ARDOUR::RouteList&);
	void timeaxisview_deleted (TimeAxisView*);
	void add_vcas (ARDOUR::VCAList&);

	Gtk::VBox global_vpacker;

	Gdk::Cursor* which_track_cursor () const;
	Gdk::Cursor* which_mode_cursor () const;
	Gdk::Cursor* which_trim_cursor (bool left_side) const;
	Gdk::Cursor* which_canvas_cursor (ItemType type) const;

	ApplicationBar           _application_bar;
	ArdourCanvas::GtkCanvas* _track_canvas;
	ArdourCanvas::GtkCanvasViewport* _track_canvas_viewport;

	RegionPeakCursor* _region_peak_cursor;

	void parameter_changed (std::string);
	void ui_parameter_changed (std::string);

	Gtk::EventBox            time_bars_event_box;
	Gtk::VBox                time_bars_vbox;

	ArdourCanvas::Container* tempo_group;
	ArdourCanvas::Container* meter_group;
	ArdourCanvas::Container* marker_group;
	ArdourCanvas::Container* range_marker_group;
	ArdourCanvas::Container* section_marker_group;

	/* parent for groups which themselves contain time markers */
	ArdourCanvas::Container* _time_markers_group;

	/* parent for group for selection marker (above ruler) */
	ArdourCanvas::Container* _selection_marker_group;
	LocationMarkers*         _selection_marker;

	/* The group containing all other groups that are scrolled vertically
	   and horizontally.
	*/
	ArdourCanvas::ScrollGroup* hv_scroll_group;

	/* The group containing all other groups that are scrolled horizontally ONLY
	*/
	ArdourCanvas::ScrollGroup* h_scroll_group;

	/* Scroll group for cursors, scrolled horizontally, above everything else
	*/
	ArdourCanvas::ScrollGroup* cursor_scroll_group;

	/* The group containing all trackviews. */
	ArdourCanvas::Container* no_scroll_group;

	/* The group containing all trackviews. */
	ArdourCanvas::Container* _trackview_group;

	/* The group holding things (mostly regions) while dragging so they
	 * are on top of everything else
	 */
	ArdourCanvas::Container* _drag_motion_group;

	/* a rect that sits at the bottom of all tracks to act as a drag-no-drop/clickable
	 * target area.
	 */
	ArdourCanvas::Rectangle* _canvas_drop_zone;
	bool canvas_drop_zone_event (GdkEvent* event);

	ArdourCanvas::Rectangle* _canvas_grid_zone;
	bool canvas_grid_zone_event (GdkEvent* event);

	static Gtk::Table* setup_ruler_new (Gtk::HBox&, std::vector<Gtk::Label*>&, std::string const&);
	static Gtk::Table* setup_ruler_new (Gtk::HBox&, std::vector<Gtk::Label*>&, Gtk::Label*);
	static void        setup_ruler_add (Gtk::Table*, ArdourWidgets::ArdourButton&, int pos = 0);

	Glib::RefPtr<Gtk::ToggleAction> ruler_minsec_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_timecode_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_samples_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_bbt_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_meter_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_tempo_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_range_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_section_action;
	Glib::RefPtr<Gtk::ToggleAction> ruler_marker_action;
	bool                            no_ruler_shown_update;

	Glib::RefPtr<Gtk::RadioAction> all_marker_action;
	Glib::RefPtr<Gtk::RadioAction> cd_marker_action;
	Glib::RefPtr<Gtk::RadioAction> scene_marker_action;
	Glib::RefPtr<Gtk::RadioAction> cue_marker_action;
	Glib::RefPtr<Gtk::RadioAction> location_marker_action;

	Glib::RefPtr<Gtk::RadioAction> all_range_action;
	Glib::RefPtr<Gtk::RadioAction> punch_range_action;
	Glib::RefPtr<Gtk::RadioAction> loop_range_action;
	Glib::RefPtr<Gtk::RadioAction> session_range_action;
	Glib::RefPtr<Gtk::RadioAction> other_range_action;

	Gtk::Widget* ruler_grabbed_widget;

	RulerDialog* ruler_dialog;

	void initialize_rulers ();
	void initialize_ruler_actions ();
	void update_just_timecode ();
	void compute_fixed_ruler_scale (); //calculates the RulerScale of the fixed rulers
	void update_fixed_rulers ();
	void update_tempo_based_rulers ();
	void popup_ruler_menu (Temporal::timepos_t const & where = Temporal::timepos_t (), ItemType type = RegionItem);
	void update_ruler_visibility ();
	void toggle_ruler_visibility ();
	void ruler_toggled (int);
	bool ruler_label_button_release (GdkEventButton*);
	void store_ruler_visibility ();
	void restore_ruler_visibility ();
	void show_rulers_for_grid ();

	enum MinsecRulerScale {
		minsec_show_msecs,
		minsec_show_seconds,
		minsec_show_minutes,
		minsec_show_hours,
		minsec_show_many_hours
	};

	MinsecRulerScale minsec_ruler_scale;

	samplecnt_t minsec_mark_interval;
	gint minsec_mark_modulo;
	gint minsec_nmarks;
	void set_minsec_ruler_scale (samplepos_t, samplepos_t);

	enum TimecodeRulerScale {
		timecode_show_bits,
		timecode_show_samples,
		timecode_show_seconds,
		timecode_show_minutes,
		timecode_show_hours,
		timecode_show_many_hours
	};

	TimecodeRulerScale timecode_ruler_scale;

	gint timecode_mark_modulo;
	gint timecode_nmarks;
	void set_timecode_ruler_scale (samplepos_t, samplepos_t);

	samplecnt_t _samples_ruler_interval;
	void set_samples_ruler_scale (samplepos_t, samplepos_t);

	ArdourCanvas::Ruler* timecode_ruler;
	ArdourCanvas::Ruler* bbt_ruler;
	ArdourCanvas::Ruler* samples_ruler;
	ArdourCanvas::Ruler* minsec_ruler;

	static double timebar_height;
	guint32 visible_timebars;
	Gtk::Menu* editor_ruler_menu;

	ArdourCanvas::Rectangle* tempo_bar;
	ArdourCanvas::Rectangle* meter_bar;
	ArdourCanvas::Rectangle* marker_bar;
	ArdourCanvas::Rectangle* range_marker_bar;
	ArdourCanvas::Rectangle* section_marker_bar;
	ArdourCanvas::Line*      ruler_separator;

	void toggle_cue_behavior ();

	Gtk::HBox _ruler_box_minsec;
	Gtk::HBox _ruler_box_timecode;
	Gtk::HBox _ruler_box_samples;
	Gtk::HBox _ruler_box_bbt;
	Gtk::HBox _ruler_box_tempo;
	Gtk::HBox _ruler_box_meter;
	Gtk::HBox _ruler_box_range;
	Gtk::HBox _ruler_box_marker;
	Gtk::HBox _ruler_box_section;
	Gtk::HBox _ruler_box_videotl;

	std::vector<Gtk::Label*> _ruler_labels;

	ArdourWidgets::ArdourButton  _ruler_btn_tempo_add;
	ArdourWidgets::ArdourButton  _ruler_btn_meter_add;
	ArdourWidgets::ArdourButton  _ruler_btn_range_prev;
	ArdourWidgets::ArdourButton  _ruler_btn_range_next;
	ArdourWidgets::ArdourButton  _ruler_btn_range_add;
	ArdourWidgets::ArdourButton  _ruler_btn_loc_prev;
	ArdourWidgets::ArdourButton  _ruler_btn_loc_next;
	ArdourWidgets::ArdourButton  _ruler_btn_loc_add;
	ArdourWidgets::ArdourButton  _ruler_btn_section_prev;
	ArdourWidgets::ArdourButton  _ruler_btn_section_next;
	ArdourWidgets::ArdourButton  _ruler_btn_section_add;

	/* videtimline related actions */
	Gtk::Label                      videotl_label;
	ArdourCanvas::Container*        videotl_group;
	Glib::RefPtr<Gtk::ToggleAction> ruler_video_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_proc_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_ontop_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_timecode_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_frame_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_osdbg_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_fullscreen_action;
	Glib::RefPtr<Gtk::ToggleAction> xjadeo_letterbox_action;
	Glib::RefPtr<Gtk::Action> xjadeo_zoom_100;
	void set_xjadeo_proc ();
	void toggle_xjadeo_proc (int state=-1);
	void set_close_video_sensitive (bool onoff);
	void set_xjadeo_sensitive (bool onoff);
	void set_xjadeo_viewoption (int);
	void toggle_xjadeo_viewoption (int what, int state=-1);
	void toggle_ruler_video (bool onoff) {ruler_video_action->set_active(onoff);}
	int videotl_bar_height; /* in units of timebar_height; default: 4 */
	int get_videotl_bar_height () const { return videotl_bar_height; }
	void toggle_region_video_lock ();

	samplepos_t playhead_cursor_sample () const;

	Temporal::timepos_t get_region_boundary (Temporal::timepos_t const & pos, int32_t dir, bool with_selection, bool only_onscreen);

	void    cursor_to_region_boundary (bool with_selection, int32_t dir);
	void    cursor_to_next_region_boundary (bool with_selection);
	void    cursor_to_previous_region_boundary (bool with_selection);
	void    cursor_to_next_region_point (EditorCursor*, ARDOUR::RegionPoint);
	void    cursor_to_previous_region_point (EditorCursor*, ARDOUR::RegionPoint);
	void    cursor_to_region_point (EditorCursor*, ARDOUR::RegionPoint, int32_t dir);
	void    cursor_to_selection_start (EditorCursor*);
	void    cursor_to_selection_end   (EditorCursor*);

	void    selected_marker_to_region_boundary (bool with_selection, int32_t dir);
	void    selected_marker_to_next_region_boundary (bool with_selection);
	void    selected_marker_to_previous_region_boundary (bool with_selection);
	void    selected_marker_to_next_region_point (ARDOUR::RegionPoint);
	void    selected_marker_to_previous_region_point (ARDOUR::RegionPoint);
	void    selected_marker_to_region_point (ARDOUR::RegionPoint, int32_t dir);
	void    selected_marker_to_selection_start ();
	void    selected_marker_to_selection_end   ();

	void    select_all_selectables_using_cursor (EditorCursor*, bool);
	void    select_all_selectables_using_edit (bool, bool);
	void    select_all_selectables_between (bool within);
	void    select_range_between ();

	std::shared_ptr<ARDOUR::Region> find_next_region (Temporal::timepos_t const &, ARDOUR::RegionPoint, int32_t dir, TrackViewList&, TimeAxisView** = 0);
	Temporal::timepos_t find_next_region_boundary (Temporal::timepos_t const &, int32_t dir, const TrackViewList&);

	std::set<Temporal::timepos_t> region_boundary_cache;
	void mark_region_boundary_cache_dirty () { _region_boundary_cache_dirty = true; }
	void build_region_boundary_cache ();
	bool	_region_boundary_cache_dirty;

	Gtk::HBox           toplevel_hpacker;

	Gtk::HBox           bottom_hbox;

	Gtk::Table          edit_packer;

	Gtk::Adjustment     unused_adjustment; // yes, really; Gtk::Layout constructor requires refs
	Gtk::Layout         controls_layout;
	bool control_layout_scroll (GdkEventScroll* ev);
	void reset_controls_layout_width ();
	void reset_controls_layout_height (int32_t height);

	enum Direction {
		LEFT,
		RIGHT,
		UP,
		DOWN
	};

	bool scroll_press (Direction);
	void scroll_release ();
	sigc::connection _scroll_connection;
	int _scroll_callbacks;

	double _full_canvas_height;    ///< full height of the canvas

	bool track_canvas_map_handler (GdkEventAny*);

	bool edit_controls_button_event (GdkEventButton*);
	Gtk::Menu* edit_controls_left_menu;
	Gtk::Menu* edit_controls_right_menu;

	Gtk::VBox           track_canvas_vbox;
	Gtk::VBox           edit_controls_vbox;
	Gtk::HBox           edit_controls_hbox;

	TriggerClipPicker    _trigger_clip_picker;

	void control_vertical_zoom_in_all ();
	void control_vertical_zoom_out_all ();
	void control_vertical_zoom_in_selected ();
	void control_vertical_zoom_out_selected ();
	void control_step_tracks_up ();
	void control_step_tracks_down ();
	void control_view (uint32_t);
	void control_scroll (float);
	void access_action (const std::string&, const std::string&);
	void set_toggleaction (const std::string&, const std::string&, bool);
	bool deferred_control_scroll (samplepos_t);
	sigc::connection control_scroll_connection;

	void tie_vertical_scrolling ();

	void visual_changer (const VisualChange&);

	/* track views */
	TrackViewList track_views;

	std::pair<TimeAxisView*, double> trackview_by_y_position (double, bool trackview_relative_offset = true) const;

	AxisView* axis_view_by_stripable (std::shared_ptr<ARDOUR::Stripable>) const;
	AxisView* axis_view_by_control (std::shared_ptr<ARDOUR::AutomationControl>) const;

	TimeAxisView* time_axis_view_from_stripable (std::shared_ptr<ARDOUR::Stripable> s) const {
		return dynamic_cast<TimeAxisView*> (axis_view_by_stripable (s));
	}

	TrackViewList get_tracks_for_range_action () const;

	Gtk::VBox list_vpacker;

	void queue_redisplay_track_views ();
	bool process_redisplay_track_views ();
	bool redisplay_track_views (); // do not call this directly, use above wrappers

	bool             _tvl_no_redisplay;
	bool             _tvl_redisplay_on_resume;
	sigc::connection _tvl_redisplay_connection;

	sigc::connection super_rapid_screen_update_connection;

	void super_rapid_screen_update ();

	int64_t _last_update_time;
	double _err_screen_engine;

	void session_going_away ();

	samplepos_t cut_buffer_start;
	samplecnt_t cut_buffer_length;

	std::weak_ptr<ARDOUR::Trimmable> _trimmable;
	std::weak_ptr<ARDOUR::Movable> _movable;

	bool button_press_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool button_press_handler_1 (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool button_press_handler_2 (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool button_release_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool button_press_dispatch (GdkEventButton*);
	bool button_release_dispatch (GdkEventButton*);
	bool motion_handler (ArdourCanvas::Item*, GdkEvent*, bool from_autoscroll = false);
	bool enter_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool leave_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool key_press_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);
	bool key_release_handler (ArdourCanvas::Item*, GdkEvent*, ItemType);

	/* KEYMAP HANDLING */

	void register_actions ();
	void register_region_actions ();

	void load_bindings ();

	/* CUT/COPY/PASTE */

	Temporal::timepos_t last_paste_pos;
	unsigned    paste_count;

	bool can_cut_copy () const;
	void cut_copy_points (Editing::CutCopyOp, Temporal::timepos_t const & earliest);
	void cut_copy_regions (Editing::CutCopyOp, RegionSelection&);
	void cut_copy_ranges (Editing::CutCopyOp);
	void cut_copy_midi (Editing::CutCopyOp);

	void mouse_paste ();
	void paste_internal (Temporal::timepos_t const & position, float times);

	/* EDITING OPERATIONS */

	void region_lock ();
	void region_unlock ();
	void toggle_region_lock ();
	void toggle_opaque_region ();
	void toggle_record_enable ();
	void toggle_solo ();
	void toggle_solo_isolate ();
	void toggle_mute ();

	void play_solo_selection (bool restart);

	enum LayerOperation {
		Raise,
		RaiseToTop,
		Lower,
		LowerToBottom
	};

	void do_layer_operation (LayerOperation);
	void raise_region ();
	void raise_region_to_top ();
	void change_region_layering_order (bool from_context_menu);
	void lower_region ();
	void lower_region_to_bottom ();
	void split_region_at_transients ();
	void crop_region_to_selection ();
	void crop_region_to (Temporal::timepos_t const & start, Temporal::timepos_t const & end);
	void set_sync_point (Temporal::timepos_t const &, const RegionSelection&);
	void set_region_sync_position ();
	void remove_region_sync();
	void align_regions (ARDOUR::RegionPoint);
	void align_regions_relative (ARDOUR::RegionPoint point);
	void align_region (std::shared_ptr<ARDOUR::Region>, ARDOUR::RegionPoint point, Temporal::timepos_t const & position);
	void align_region_internal (std::shared_ptr<ARDOUR::Region>, ARDOUR::RegionPoint point, Temporal::timepos_t const & position);
	void recover_regions (ARDOUR::RegionList);
	void remove_selected_regions ();
	void remove_regions (const RegionSelection&, bool can_ripple, bool as_part_of_other_command);
	void remove_clicked_region ();
	void show_region_properties ();
	void show_midi_list_editor ();
	void rename_region ();
	void duplicate_some_regions (RegionSelection&, float times);
	void duplicate_selection (float times);
	void region_fill_selection ();
	void combine_regions ();
	void uncombine_regions ();

	void region_fill_track ();
	void audition_playlist_region_standalone (std::shared_ptr<ARDOUR::Region>);
	void split_multichannel_region();
	void reverse_region ();
	void strip_region_silence ();
	void normalize_region ();
	void adjust_region_gain (bool up);
	void reset_region_gain ();
	void deinterlace_midi_regions (const RegionSelection& rs);
	void deinterlace_selected_midi_regions ();
	void set_tempo_curve_range (double& max, double& min) const;
	void insert_patch_change (bool from_context);
	void fork_selected_regions ();
	void fork_regions_from_unselected ();
	void start_track_drag (TimeAxisView&, int y, Gtk::Widget& w, bool can_change_cursor);
	void mid_track_drag (GdkEventMotion*, Gtk::Widget& e);
	void end_track_drag ();
	void maybe_move_tracks ();
	bool track_dragging() const;

	void do_insert_time ();
	void insert_time (Temporal::timepos_t const &, Temporal::timecnt_t const &, Editing::InsertTimeOption, bool, bool, bool, bool);

	void do_remove_time ();
	void remove_time (Temporal::timepos_t const & pos, Temporal::timecnt_t const & distance, Editing::InsertTimeOption opt, bool markers_too,
	                  bool locked_markers_too, bool tempo_too);

	void tab_to_transient (bool forward);

	void set_tempo_from_region ();
	void use_range_as_bar ();

	void define_one_bar (Temporal::timepos_t const & start, Temporal::timepos_t const & end, std::string const & from);

	void audition_region_from_region_list ();

	void naturalize_region ();

	void split_region ();

	void delete_ ();
	void paste (float times, bool from_context_menu);
	void keyboard_paste ();
	void cut_copy (Editing::CutCopyOp);

	void place_transient ();
	void remove_transient (ArdourCanvas::Item* item);
	void snap_regions_to_grid ();
	void close_region_gaps ();


	void region_from_selection ();
	void create_region_from_selection (std::vector<std::shared_ptr<ARDOUR::Region> >&);

	void play_from_start ();
	void play_from_edit_point ();
	void play_from_edit_point_and_return ();
	void play_selected_region ();
	void play_edit_range ();
	void play_location (ARDOUR::Location&);
	void loop_location (ARDOUR::Location&);

	void group_selected_regions ();
	void ungroup_selected_regions ();

	std::shared_ptr<ARDOUR::Playlist> current_playlist () const;
	void insert_source_list_selection (float times);
	void cut_copy_section (ARDOUR::SectionOperation const op);

	/* import & embed */

	void add_external_audio_action (Editing::ImportMode);

	int  check_whether_and_how_to_import(std::string, bool all_or_nothing = true);
	bool check_multichannel_status (const std::vector<std::string>& paths);

	SoundFileOmega* sfbrowser;

	void bring_in_external_audio (Editing::ImportMode mode,  samplepos_t& pos);

	bool  idle_drop_paths  (std::vector<std::string> paths, Temporal::timepos_t sample, double ypos, bool copy);
	void  drop_paths_part_two  (const std::vector<std::string>& paths, Temporal::timepos_t const & sample, double ypos, bool copy);

	int import_sndfiles (std::vector<std::string>              paths,
	                     Editing::ImportDisposition            disposition,
	                     Editing::ImportMode                   mode,
	                     ARDOUR::SrcQuality                    quality,
	                     Temporal::timepos_t&                  pos,
	                     int                                   target_regions,
	                     int                                   target_tracks,
	                     std::shared_ptr<ARDOUR::Track>&     track,
	                     std::string const&                    pgroup_id,
	                     bool                                  replace,
	                     bool                                  with_markers,
	                     std::shared_ptr<ARDOUR::PluginInfo> instrument = std::shared_ptr<ARDOUR::PluginInfo>());

	int embed_sndfiles (std::vector<std::string>              paths,
	                    bool                                  multiple_files,
	                    bool&                                 check_sample_rate,
	                    Editing::ImportDisposition            disposition,
	                    Editing::ImportMode                   mode,
	                    Temporal::timepos_t&                  pos,
	                    int                                   target_regions,
	                    int                                   target_tracks,
	                    std::shared_ptr<ARDOUR::Track>&     track,
	                    std::string const&                    pgroup_id,
	                    std::shared_ptr<ARDOUR::PluginInfo> instrument = std::shared_ptr<ARDOUR::PluginInfo>());

	int add_sources (std::vector<std::string>              paths,
	                 ARDOUR::SourceList&                   sources,
	                 Temporal::timepos_t&                  pos,
	                 Editing::ImportDisposition            disposition,
	                 Editing::ImportMode                   mode,
	                 int                                   target_regions,
	                 int                                   target_tracks,
	                 std::shared_ptr<ARDOUR::Track>&     track,
	                 std::string const&                    pgroup_id,
	                 bool                                  add_channel_suffix,
	                 std::shared_ptr<ARDOUR::PluginInfo> instrument = std::shared_ptr<ARDOUR::PluginInfo>());

	int finish_bringing_in_material (std::shared_ptr<ARDOUR::Region>     region,
	                                 uint32_t                              in_chans,
	                                 uint32_t                              out_chans,
	                                 Temporal::timepos_t&                  pos,
	                                 Editing::ImportMode                   mode,
	                                 std::shared_ptr<ARDOUR::Track>&     existing_track,
	                                 std::string const&                    new_track_name,
	                                 std::string const&                    pgroup_id,
	                                 std::shared_ptr<ARDOUR::PluginInfo> instrument);

	std::shared_ptr<ARDOUR::AudioTrack> get_nth_selected_audio_track (int nth) const;
	std::shared_ptr<ARDOUR::MidiTrack> get_nth_selected_midi_track (int nth) const;

	void toggle_midi_input_active (bool flip_others);

	ARDOUR::InterThreadInfo* current_interthread_info;

	AnalysisWindow* analysis_window;

	/* import & embed */
	void external_audio_dialog ();
	void session_import_dialog ();

	/* PT import specific */
	void external_pt_dialog ();
	ARDOUR::ImportStatus import_pt_status;
	static void*_import_pt_thread (void*);
	void* import_pt_thread ();
	PTFFormat import_ptf;

	/* import specific info */

	struct EditorImportStatus : public ARDOUR::ImportStatus {
		void clear () {
			ARDOUR::ImportStatus::clear ();
			track.reset ();
		}

		Editing::ImportMode mode;
		Temporal::timepos_t pos;
		int target_tracks;
		int target_regions;
		std::shared_ptr<ARDOUR::Track> track;
		bool replace;
	};

	EditorImportStatus import_status;
	static void*_import_thread (void*);
	void* import_thread ();
	void finish_import ();

	/* to support this ... */

	void import_audio (bool as_tracks);
	void do_import (std::vector<std::string> paths, bool split, bool as_tracks);
	void import_smf_tempo_map (Evoral::SMF const &, Temporal::timepos_t const & pos);
	void move_to_start ();
	void move_to_end ();
	void center_playhead ();
	void center_edit_point ();
	void playhead_forward_to_grid ();
	void playhead_backward_to_grid ();
	void scroll_playhead (bool forward);
	void scroll_backward (float pages=0.8f);
	void scroll_forward (float pages=0.8f);
	void scroll_tracks_down ();
	void scroll_tracks_up ();
	void move_selected_tracks (bool);
	void set_mark ();
	void clear_markers ();
	void clear_xrun_markers ();
	void clear_ranges ();
	void clear_cues ();
	void clear_scenes ();
	void clear_locations ();
	void unhide_markers ();
	void unhide_ranges ();
	void cursor_align (bool playhead_to_edit);
	void toggle_skip_playback ();

	void remove_last_capture ();

	void tag_last_capture ();
	void tag_selected_region ();
	void tag_regions (ARDOUR::RegionList);

	void select_all_selectables_using_time_selection ();
	void select_all_selectables_using_loop();
	void select_all_selectables_using_punch();
	void set_selection_from_range (ARDOUR::Location&);
	void set_selection_from_punch ();
	void set_selection_from_loop ();
	void set_selection_from_region ();

	void add_location_mark_with_flag (Temporal::timepos_t const & where, ARDOUR::Location::Flags flag, int32_t cue_id);
	void add_location_from_region ();
	void add_locations_from_region ();
	void add_location_from_selection ();
	void add_section_from_playhead ();
	void set_loop_from_selection (bool play);
	void set_punch_from_selection ();
	void set_punch_from_region ();
	void set_auto_punch_range();

	void set_session_start_from_playhead ();
	void set_session_end_from_playhead ();
	void set_session_extents_from_selection ();

	void set_loop_from_region (bool play);

	void set_punch_range (Temporal::timepos_t const & start, Temporal::timepos_t const & end, std::string cmd);

	void add_tempo_from_playhead_cursor ();
	void add_meter_from_playhead_cursor ();

	void toggle_location_at_playhead_cursor ();
	void add_location_from_playhead_cursor ();
	bool do_remove_location_at_playhead_cursor ();
	void remove_location_at_playhead_cursor ();
	bool select_new_marker;

	void toggle_all_existing_automation ();

	void toggle_layer_display ();
	void layer_display_stacked ();
	void layer_display_overlaid ();

	void launch_playlist_selector ();

	void reverse_selection ();
	void edit_envelope ();

	void set_punch_start_from_edit_point ();
	void set_punch_end_from_edit_point ();
	void set_loop_start_from_edit_point ();
	void set_loop_end_from_edit_point ();

	void keyboard_selection_begin (Editing::EditIgnoreOption = Editing::EDIT_IGNORE_NONE);
	void keyboard_selection_finish (bool add, Editing::EditIgnoreOption = Editing::EDIT_IGNORE_NONE);
	bool have_pending_keyboard_selection;
	samplepos_t pending_keyboard_selection_start;

	void move_range_selection_start_or_end_to_region_boundary (bool, bool);

	bool ignore_gui_changes;

	void escape ();
	void lock ();
	void unlock ();
	Gtk::Dialog* lock_dialog;

	int64_t _last_event_time;

	bool generic_event_handler (GdkEvent*);
	bool lock_timeout_callback ();
	void start_lock_event_timing ();

	Gtk::Menu fade_context_menu;

	Gtk::Menu xfade_in_context_menu;
	Gtk::Menu xfade_out_context_menu;
	void popup_xfade_in_context_menu (int, int, ArdourCanvas::Item*, ItemType);
	void popup_xfade_out_context_menu (int, int, ArdourCanvas::Item*, ItemType);
	void fill_xfade_menu (Gtk::Menu_Helpers::MenuList& items, bool start);

	void set_fade_in_shape (ARDOUR::FadeShape);
	void set_fade_out_shape (ARDOUR::FadeShape);

	void set_fade_length (bool in);
	void set_fade_in_active (bool);
	void set_fade_out_active (bool);

	void fade_range ();

	ARDOUR::PlaylistSet motion_frozen_playlists;

	bool _dragging_playhead;

	void marker_drag_motion_callback (GdkEvent*);
	void marker_drag_finished_callback (GdkEvent*);

	gint mouse_rename_region (ArdourCanvas::Item*, GdkEvent*);

	void add_region_drag (ArdourCanvas::Item*, GdkEvent*, RegionView*, bool copy);
	void start_create_region_grab (ArdourCanvas::Item*, GdkEvent*);
	void add_region_brush_drag (ArdourCanvas::Item*, GdkEvent*, RegionView*);
	void start_selection_grab (ArdourCanvas::Item*, GdkEvent*);

	void region_view_item_click (AudioRegionView&, GdkEventButton*);

	bool can_remove_control_point (ArdourCanvas::Item*);
	void remove_control_point (ArdourCanvas::Item*);

	/* Canvas event handlers */

	bool canvas_scroll_event (GdkEventScroll* event, bool from_canvas);
	bool canvas_control_point_event (GdkEvent* event,ArdourCanvas::Item*, ControlPoint*);
	bool canvas_velocity_event (GdkEvent* event,ArdourCanvas::Item*);
	bool canvas_velocity_base_event (GdkEvent* event,ArdourCanvas::Item*);
	bool canvas_line_event (GdkEvent* event,ArdourCanvas::Item*, EditorAutomationLine*);
	bool canvas_selection_rect_event (GdkEvent* event,ArdourCanvas::Item*, SelectionRect*);
	bool canvas_selection_start_trim_event (GdkEvent* event,ArdourCanvas::Item*, SelectionRect*);
	bool canvas_selection_end_trim_event (GdkEvent* event,ArdourCanvas::Item*, SelectionRect*);
	bool canvas_start_xfade_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*);
	bool canvas_end_xfade_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*);
	bool canvas_fade_in_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*);
	bool canvas_fade_in_handle_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*, bool trim = false);
	bool canvas_fade_out_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*);
	bool canvas_fade_out_handle_event (GdkEvent* event,ArdourCanvas::Item*, AudioRegionView*, bool trim = false);
	bool canvas_region_view_event (GdkEvent* event,ArdourCanvas::Item*, RegionView*);
	bool canvas_wave_view_event (GdkEvent* event,ArdourCanvas::Item*, RegionView*);
	bool canvas_frame_handle_event (GdkEvent* event,ArdourCanvas::Item*, RegionView*);
	bool canvas_region_view_name_highlight_event (GdkEvent* event,ArdourCanvas::Item*, RegionView*);
	bool canvas_region_view_name_event (GdkEvent* event,ArdourCanvas::Item*, RegionView*);
	bool canvas_feature_line_event (GdkEvent* event, ArdourCanvas::Item*, RegionView*);
	bool canvas_stream_view_event (GdkEvent* event,ArdourCanvas::Item*, RouteTimeAxisView*);
	bool canvas_marker_event (GdkEvent* event,ArdourCanvas::Item*, ArdourMarker*);
	bool canvas_tempo_marker_event (GdkEvent* event,ArdourCanvas::Item*, TempoMarker*);
	bool canvas_tempo_curve_event (GdkEvent* event,ArdourCanvas::Item*, TempoCurve*);
	bool canvas_meter_marker_event (GdkEvent* event,ArdourCanvas::Item*, MeterMarker*);
	bool canvas_bbt_marker_event (GdkEvent* event,ArdourCanvas::Item*, BBTMarker*);
	bool canvas_automation_track_event(GdkEvent* event, ArdourCanvas::Item*, AutomationTimeAxisView*);
	bool canvas_note_event (GdkEvent* event, ArdourCanvas::Item*);
	bool canvas_bg_event (GdkEvent* event, ArdourCanvas::Item*);

	bool canvas_ruler_event (GdkEvent* event, ArdourCanvas::Item*, ItemType);
	bool canvas_ruler_bar_event (GdkEvent* event, ArdourCanvas::Item*, ItemType, std::string const&);
	bool canvas_selection_marker_event (GdkEvent* event, ArdourCanvas::Item*);

	bool canvas_videotl_bar_event (GdkEvent* event, ArdourCanvas::Item*);
	void update_video_timeline (bool flush = false);
	void set_video_timeline_height (const int, bool force = false);
	bool is_video_timeline_locked ();
	void toggle_video_timeline_locked ();
	void set_video_timeline_locked (const bool);
	void queue_visual_videotimeline_update ();
	void embed_audio_from_video (std::string, samplepos_t n = 0, bool lock_position_to_video = true);

	bool track_selection_change_without_scroll () const {
		return _track_selection_change_without_scroll;
	}

	PBD::Signal<void()> EditorFreeze;
	PBD::Signal<void()> EditorThaw;

	Temporal::TempoMap::WritableSharedPtr begin_tempo_map_edit ();
	void abort_tempo_map_edit ();
	void mid_tempo_per_track_update (TimeAxisView&);
	void mid_tempo_per_region_update (RegionView*);
	bool ignore_map_change;

	Temporal::TempoMap::WritableSharedPtr begin_tempo_mapping (Temporal::DomainBounceInfo&);
	void abort_tempo_mapping ();
	void commit_tempo_mapping (Temporal::TempoMap::WritableSharedPtr&);

	enum MidTempoChanges {
		TempoChanged = 0x1,
		MeterChanged = 0x2,
		BBTChanged   = 0x4,
		MappingChanged = 0x8
	};

	void mid_tempo_change (MidTempoChanges);

	Editing::EditPoint edit_point() const { return _edit_point; }
	bool canvas_playhead_cursor_event (GdkEvent* event, ArdourCanvas::Item*);

	enum MarkerBarType {
		CueMarks = 0x1,
		SceneMarks = 0x2,
		CDMarks = 0x4,
		LocationMarks = 0x8
	};

	enum RangeBarType {
		PunchRange = 0x1,
		LoopRange = 0x2,
		SessionRange = 0x4,
		OtherRange = 0x8
	};

	static const MarkerBarType all_marker_types = MarkerBarType (CueMarks|SceneMarks|CDMarks|LocationMarks);
	static const RangeBarType all_range_types = RangeBarType (PunchRange|LoopRange|SessionRange|OtherRange);

	MarkerBarType visible_marker_types () const;
	RangeBarType visible_range_types () const;

	void set_visible_marker_types (MarkerBarType);
	void set_visible_range_types (RangeBarType);

protected:
	void _commit_tempo_map_edit (Temporal::TempoMap::WritableSharedPtr&, bool with_update = false);
	void automation_create_point_at_edit_point();
	void automation_raise_points ();
	void automation_lower_points ();
	void automation_move_points_later ();
	void automation_move_points_earlier ();

private:
	friend class DragManager;
	friend class EditorRouteGroups;
	friend class EditorRegions;
	friend class EditorSections;
	friend class EditorSources;

	/* non-public event handlers */

	bool canvas_section_box_event (GdkEvent* event);
	bool track_canvas_scroll (GdkEventScroll* event);

	bool track_canvas_button_press_event (GdkEventButton* event);
	bool track_canvas_button_release_event (GdkEventButton* event);
	bool track_canvas_motion_notify_event (GdkEventMotion* event);

	Gtk::Allocation _canvas_viewport_allocation;
	void track_canvas_viewport_allocate (Gtk::Allocation alloc);
	void track_canvas_viewport_size_allocated ();
	bool track_canvas_drag_motion (Glib::RefPtr<Gdk::DragContext> const &, int, int, guint);
	bool track_canvas_key_press (GdkEventKey*);
	bool track_canvas_key_release (GdkEventKey*);

	void set_playhead_cursor ();

	void toggle_region_mute ();

	void initialize_canvas ();

	/* playlist internal ops */

	bool stamp_new_playlist (std::string title, std::string &name, std::string &pgroup, bool copy);

	/* display control */

	/// true if we scroll the tracks rather than the playhead
	bool _stationary_playhead;
	/// true if we are in fullscreen mode
	bool _maximised;

	ArdourCanvas::Container* global_rect_group;

	void new_tempo_section ();

	void remove_tempo_marker (ArdourCanvas::Item*);
	void remove_meter_marker (ArdourCanvas::Item*);
	void remove_bbt_marker (ArdourCanvas::Item*);
	gint real_remove_tempo_marker (Temporal::TempoPoint const *);
	gint real_remove_meter_marker (Temporal::MeterPoint const *);
	gint real_remove_bbt_marker (Temporal::MusicTimePoint const *);

	void edit_tempo_marker (TempoMarker&);
	void edit_meter_marker (MeterMarker&);
	void edit_bbt_marker (BBTMarker&);
	void edit_control_point (ArdourCanvas::Item*);
	void edit_region (RegionView*);

	void edit_current_meter ();
	void edit_current_tempo ();

	void marker_menu_edit ();
	void marker_menu_remove ();
	void marker_menu_rename ();
	void edit_marker (ArdourMarker* marker, bool with_scene);
	bool edit_location (ARDOUR::Location& loc, bool with_scene, bool with_command);
	void toggle_tempo_continues ();
	void toggle_tempo_type ();
	void ramp_to_next_tempo ();
	void toggle_marker_menu_lock ();
	void toggle_marker_section ();
	void marker_menu_hide ();
	void marker_menu_set_origin ();
	void marker_menu_loop_range ();
	void marker_menu_select_all_selectables_using_range ();
	void marker_menu_select_using_range ();
	void marker_menu_separate_regions_using_location ();
	void marker_menu_play_from ();
	void marker_menu_play_range ();
	void marker_menu_set_playhead ();
	void marker_menu_set_from_playhead ();
	void marker_menu_set_from_selection (bool force_regions);
	void marker_menu_range_to_next ();
	void marker_menu_change_cue (int cue);
	void marker_menu_zoom_to_range ();
	void new_transport_marker_menu_set_loop ();
	void new_transport_marker_menu_set_punch ();
	void update_loop_range_view ();
	void update_punch_range_view ();
	void new_transport_marker_menu_popdown ();
	void marker_context_menu (GdkEventButton*, ArdourCanvas::Item*);
	void tempo_map_marker_context_menu (GdkEventButton*, ArdourCanvas::Item*);
	void new_transport_marker_context_menu (GdkEventButton*, ArdourCanvas::Item*);
	void build_range_marker_menu (ARDOUR::Location*, bool, bool);
	void build_marker_menu (ARDOUR::Location*);
	void build_tempo_marker_menu (TempoMarker*, bool);
	void build_meter_marker_menu (MeterMarker*, bool);
	void build_bbt_marker_menu (BBTMarker*);
	void build_new_transport_marker_menu ();

	void dynamic_cast_marker_object (void*, MeterMarker**, TempoMarker**, BBTMarker**) const;

	Gtk::Menu* tempo_marker_menu;
	Gtk::Menu* meter_marker_menu;
	Gtk::Menu* bbt_marker_menu;
	Gtk::Menu* marker_menu;
	Gtk::Menu* range_marker_menu;
	Gtk::Menu* new_transport_marker_menu;
	ArdourCanvas::Item* marker_menu_item;

	typedef std::list<MetricMarker*> Marks;
	Marks tempo_marks;
	Marks meter_marks;
	Marks bbt_marks;

	void remove_metric_marks ();
	void reset_metric_marks ();
	void reset_tempo_marks ();
	void reset_meter_marks ();
	void reset_bbt_marks ();

	void compute_current_bbt_points (Temporal::TempoMapPoints& grid, samplepos_t left, samplepos_t right);

	void reassociate_metric_markers (Temporal::TempoMap::SharedPtr const &);

	void reassociate_tempo_marker (Temporal::TempoMap::SharedPtr const & tmap, Temporal::Tempos const &, TempoMarker& marker);
	void reassociate_meter_marker (Temporal::TempoMap::SharedPtr const & tmap, Temporal::Meters const &, MeterMarker& marker);
	void reassociate_bartime_marker (Temporal::TempoMap::SharedPtr const & tmap, Temporal::MusicTimes const &, BBTMarker& marker);

	void make_bbt_marker (Temporal::MusicTimePoint const *, Marks::iterator before);
	void make_meter_marker (Temporal::MeterPoint const *, Marks::iterator before);
	void make_tempo_marker (Temporal::TempoPoint const * ts, Temporal::TempoPoint const *& prev_ts, uint32_t tc_color, samplecnt_t sr3, Marks::iterator before);
	void update_tempo_curves (double min_tempo, double max_tempo, samplecnt_t sr);

	void tempo_map_changed ();

	void redisplay_grid (bool immediate_redraw);

	/* toolbar */

	ArdourWidgets::ArdourButton   tav_expand_button;
	ArdourWidgets::ArdourButton   tav_shrink_button;
	ArdourWidgets::ArdourDropdown visible_tracks_selector;
	ArdourWidgets::ArdourDropdown zoom_preset_selector;

	int32_t                   _visible_track_count;
	void build_track_count_menu ();
	void set_visible_track_count (int32_t);

	void set_zoom_preset(int64_t);

	Gtk::VBox                toolbar_clock_vbox;
	Gtk::VBox                toolbar_selection_clock_vbox;
	Gtk::Table               toolbar_selection_clock_table;
	Gtk::Label               toolbar_selection_cursor_label;

	ArdourWidgets::ArdourButton smart_mode_button;
	Glib::RefPtr<Gtk::ToggleAction> smart_mode_action;

	void                     mouse_mode_chosen (Editing::MouseMode m);
	void			 mouse_mode_object_range_toggled ();
	bool                     ignore_mouse_mode_toggle;

	bool                     mouse_select_button_release (GdkEventButton*);

	Gtk::VBox                automation_box;
	Gtk::Button              automation_mode_button;

	//edit mode menu stuff
	ArdourWidgets::ArdourDropdown ripple_mode_selector;
	ArdourWidgets::ArdourDropdown	edit_mode_selector;
	void edit_mode_selection_done (ARDOUR::EditMode);
	void ripple_mode_selection_done (ARDOUR::RippleMode);
	void build_edit_mode_menu ();
	Gtk::VBox edit_mode_box;

	void set_ripple_mode (ARDOUR::RippleMode);

	void set_edit_mode (ARDOUR::EditMode);
	void cycle_edit_mode ();


	Gtk::CheckButton stretch_marker_cb;

	bool should_stretch_markers() const {
		return stretch_marker_cb.get_active ();
	}

	Gtk::HBox ebox_hpacker;
	Gtk::VBox ebox_vpacker;

	Gtk::HBox _box;

	//zoom focus menu stuff
	Editing::ZoomFocus effective_zoom_focus() const;

	void build_zoom_focus_menu ();

	/* Marker Click Radio */
	Glib::RefPtr<Gtk::RadioAction> marker_click_behavior_action (Editing::MarkerClickBehavior);
	void marker_click_behavior_chosen (Editing::MarkerClickBehavior);
	void marker_click_behavior_selection_done (Editing::MarkerClickBehavior);

	Gtk::HBox _track_box;

	Gtk::HBox _zoom_box;
	void zoom_adjustment_changed();

	void setup_toolbar ();

	void setup_tooltips ();

	Gtk::HBox toolbar_hbox;

	void setup_midi_toolbar ();

	void time_selection_changed ();
	void track_selection_changed ();
	void update_time_selection_display ();
	void presentation_info_changed (PBD::PropertyChange const &);
	void handle_gui_changes (std::string const&, void*);
	void region_selection_changed ();
	void catch_up_on_midi_selection ();
	sigc::connection editor_regions_selection_changed_connection;
	void sensitize_all_region_actions (bool);
	void sensitize_the_right_region_actions (bool because_canvas_crossing);
	bool _all_region_actions_sensitized;
	/** Flag to block region action handlers from doing what they normally do;
	 *  I tried Gtk::Action::block_activate() but this doesn't work (ie it doesn't
	 *  block) when setting a ToggleAction's active state.
	 */
	bool _ignore_region_action;
	bool _last_region_menu_was_main;
	void point_selection_changed ();
	void marker_selection_changed ();

	bool _track_selection_change_without_scroll;
	bool _editor_track_selection_change_without_scroll;

	void cancel_selection ();
	void cancel_time_selection ();

	bool get_smart_mode() const;

	bool audio_region_selection_covers (samplepos_t where);

	SectionBox* _section_box;

	/* transport range select process */

	ArdourCanvas::Rectangle* range_bar_drag_rect;
	ArdourCanvas::Rectangle* transport_bar_preroll_rect;
	ArdourCanvas::Rectangle* transport_bar_postroll_rect;
	ArdourCanvas::Rectangle* transport_loop_range_rect;
	ArdourCanvas::Rectangle* transport_punch_range_rect;
	ArdourCanvas::Line*      transport_punchin_line;
	ArdourCanvas::Line*      transport_punchout_line;
	ArdourCanvas::Rectangle* transport_preroll_rect;
	ArdourCanvas::Rectangle* transport_postroll_rect;

	ARDOUR::Location* transport_punch_location();

	ARDOUR::Location* temp_location;

	/* object rubberband select process */

	void select_all_within (Temporal::timepos_t const &, Temporal::timepos_t const &, double, double, std::list<SelectableOwner*> const &, ARDOUR::SelectionOperation, bool);

	EditorRouteGroups* _route_groups;
	EditorRoutes*      _routes;
	EditorRegions*     _regions;
	EditorSections*    _sections;
	EditorSources*     _sources;
	EditorSnapshots*   _snapshots;
	EditorLocations*   _locations;

	/* diskstream/route display management */
	Glib::RefPtr<Gdk::Pixbuf> rec_enabled_icon;
	Glib::RefPtr<Gdk::Pixbuf> rec_disabled_icon;

	Glib::RefPtr<Gtk::TreeSelection> route_display_selection;

	/* autoscrolling */

	bool autoscroll_canvas ();
	void start_canvas_autoscroll (bool allow_horiz, bool allow_vert, const ArdourCanvas::Rect& boundary);
	void stop_canvas_autoscroll ();

	/* trimming */
	void point_trim (GdkEvent*, Temporal::timepos_t const &);

	void trim_region_front();
	void trim_region_back();
	void trim_region (bool front);

	void trim_region_to_loop ();
	void trim_region_to_punch ();
	void trim_region_to_location (const ARDOUR::Location&, const char* cmd);

	void trim_to_region(bool forward);
	void trim_region_to_previous_region_end();
	void trim_region_to_next_region_start();

	bool show_gain_after_trim;

	/* Drag-n-Drop */
	void track_canvas_drag_data_received (
	        const Glib::RefPtr<Gdk::DragContext>& context,
	        gint                                  x,
	        gint                                  y,
	        const Gtk::SelectionData&             data,
	        guint                                 info,
	        guint                                 time);

	void drop_paths (
	        const Glib::RefPtr<Gdk::DragContext>& context,
	        gint                                  x,
	        gint                                  y,
	        const Gtk::SelectionData&             data,
	        guint                                 info,
	        guint                                 time);

	void drop_regions (
	        const Glib::RefPtr<Gdk::DragContext>& context,
	        gint                                  x,
	        gint                                  y,
	        const Gtk::SelectionData&             data,
	        guint                                 info,
	        guint                                 time);

	/* audio export */

	bool _no_not_select_reimported_tracks;

	enum BounceTarget {
		NewSource,
		NewTrigger,
		ReplaceRange
	};

	int  write_region_selection(RegionSelection&);
	bool write_region (std::string path, std::shared_ptr<ARDOUR::AudioRegion>);
	void bounce_region_selection (bool with_processing);
	void bounce_range_selection (BounceTarget, bool enable_processing);
	void external_edit_region ();

	int write_audio_selection (TimeSelection&);
	bool write_audio_range (ARDOUR::AudioPlaylist&, const ARDOUR::ChanCount& channels, std::list<ARDOUR::TimelineRange>&);

	void write_selection ();

	uint32_t selection_op_cmd_depth;
	uint32_t selection_op_history_it;

	std::list<XMLNode*> selection_op_history; /* used in *_reversible_selection_op */

	void update_title ();
	void update_title_s (const std::string & snapshot_name);

	void instant_save ();
	bool no_save_instant;

	std::shared_ptr<ARDOUR::AudioRegion> last_audition_region;

	/* freeze operations */

	ARDOUR::InterThreadInfo freeze_status;
	static void* _freeze_thread (void*);
	void* freeze_thread ();

	void freeze_route ();
	void unfreeze_route ();

	/* duplication */

	void duplicate_range (bool with_dialog);
	void duplicate_regions (float times);

	TimeFXDialog* current_timefx;
	static void* timefx_thread (void* arg);
	void do_timefx (bool fixed_end);

	int time_stretch (RegionSelection&, Temporal::ratio_t const & fraction, bool fixed_end);
	int pitch_shift (RegionSelection&, float cents);
	void pitch_shift_region ();

	/* editor-mixer strip */

	MixerStrip *current_mixer_strip;
	bool show_editor_mixer_when_tracks_arrive;
	Gtk::VBox current_mixer_strip_vbox;
	void cms_new (std::shared_ptr<ARDOUR::Route>);
	void current_mixer_strip_hidden ();

#ifdef __APPLE__
	void ensure_all_elements_drawn ();
#endif
	/* nudging tracks */

	void nudge_track (bool use_edit_point, bool forwards);

	static const int32_t default_width = 995;
	static const int32_t default_height = 765;

	/* nudge */

	ArdourWidgets::ArdourButton      nudge_forward_button;
	ArdourWidgets::ArdourButton      nudge_backward_button;
	Gtk::HBox        nudge_hbox;
	Gtk::VBox        nudge_vbox;
	AudioClock*       nudge_clock;

	bool nudge_forward_release (GdkEventButton*);
	bool nudge_backward_release (GdkEventButton*);

	/* audio filters */

	void apply_filter (ARDOUR::Filter&, std::string cmd, ProgressReporter* progress = 0);

	/* plugin setup */
	int plugin_setup (std::shared_ptr<ARDOUR::Route>, std::shared_ptr<ARDOUR::PluginInsert>, ARDOUR::Route::PluginSetupOptions);

	/* handling cleanup */

	int playlist_deletion_dialog (std::shared_ptr<ARDOUR::Playlist>);

	PBD::ScopedConnectionList session_connections;
	PBD::ScopedConnection tempo_map_connection;

	/* tracking step changes of track height */

	TimeAxisView* current_stepping_trackview;
	PBD::microseconds_t last_track_height_step_timestamp;
	gint track_height_step_timeout();
	sigc::connection step_timeout;

	bool left_track_canvas (GdkEventCrossing*);
	bool entered_track_canvas (GdkEventCrossing*);
	void set_entered_track (TimeAxisView*);
	void set_entered_regionview (RegionView*);
	gint left_automation_track ();

	std::pair<Temporal::timepos_t,Temporal::timepos_t> max_zoom_extent() const { return session_gui_extents(); }

	void reset_canvas_action_sensitivity (bool);
	void set_gain_envelope_visibility ();
	void set_region_gain_visibility (RegionView*);
	void toggle_gain_envelope_active ();
	void toggle_region_polarity ();
	void reset_region_gain_envelopes ();

	void session_state_saved (std::string);

	Glib::RefPtr<Gtk::Action>              selection_undo_action;
	Glib::RefPtr<Gtk::Action>              selection_redo_action;

	Glib::RefPtr<Gtk::ToggleAction> show_editor_mixer_action;
	Glib::RefPtr<Gtk::ToggleAction> show_editor_list_action;
	Glib::RefPtr<Gtk::ToggleAction> show_editor_props_action;
	Glib::RefPtr<Gtk::ToggleAction> show_touched_automation_action;

	void history_changed ();

	Editing::EditPoint _edit_point;

	ArdourWidgets::ArdourDropdown edit_point_selector;
	void build_edit_point_menu();

	void set_edit_point_preference (Editing::EditPoint ep, bool force = false);
	void cycle_edit_point (bool with_marker);
	void set_edit_point ();
	void edit_point_selection_done (Editing::EditPoint);
	void edit_point_chosen (Editing::EditPoint);
	Glib::RefPtr<Gtk::RadioAction> edit_point_action (Editing::EditPoint);
	std::vector<std::string> edit_point_strings;
	std::vector<std::string> edit_mode_strings;
	std::vector<std::string> ripple_mode_strings;

	void selected_marker_moved (ARDOUR::Location*);

	bool get_edit_op_range (Temporal::timepos_t& start, Temporal::timepos_t& end) const;

	void get_regions_at (RegionSelection&, Temporal::timepos_t const & where, const TrackViewList& ts) const;
	void get_regions_after (RegionSelection&, Temporal::timepos_t const & where, const TrackViewList& ts) const;

	RegionSelection get_regions_from_selection_and_edit_point (Editing::EditIgnoreOption = Editing::EDIT_IGNORE_NONE,
	                                                           bool use_context_click = false,
	                                                           bool from_outside_canvas = false);
	RegionSelection get_regions_from_selection_and_entered () const;

	void start_updating_meters ();
	void stop_updating_meters ();
	bool meters_running;

	void select_next_stripable (bool routes_only = true);
	void select_prev_stripable (bool routes_only = true);

	Temporal::timepos_t snap_to_minsec (Temporal::timepos_t const & start,
	                                    Temporal::RoundMode   direction,
	                                    ARDOUR::SnapPref    gpref) const;

	Temporal::timepos_t snap_to_cd_frames (Temporal::timepos_t const & start,
	                                       Temporal::RoundMode   direction,
	                                       ARDOUR::SnapPref    gpref) const;

	Temporal::timepos_t snap_to_timecode (Temporal::timepos_t const & start,
	                                      Temporal::RoundMode   direction,
	                                      ARDOUR::SnapPref    gpref) const;

	Temporal::timepos_t snap_to_grid (Temporal::timepos_t const & start,
	                                  Temporal::RoundMode   direction,
	                                  ARDOUR::SnapPref    gpref) const;

	void snap_to_internal (Temporal::timepos_t & first,
	                       Temporal::RoundMode   direction = Temporal::RoundNearest,
	                       ARDOUR::SnapPref     gpref = ARDOUR::SnapToAny_Visual,
	                       bool                for_mark  = false) const;

	Temporal::timepos_t snap_to_marker (Temporal::timepos_t const & presnap,
	                                    Temporal::RoundMode direction = Temporal::RoundNearest) const;

	double visible_canvas_width() const { return _visible_canvas_width; }

	RhythmFerret* rhythm_ferret;

	void fit_tracks (TrackViewList &);
	void fit_selection ();
	void set_track_height (Height);

	void _remove_tracks ();
	bool idle_remove_tracks ();
	void toggle_tracks_active ();

	bool _have_idled;
	int resize_idle_id;
	static gboolean _idle_resize (gpointer);
	bool idle_resize();
	int32_t _pending_resize_amount;
	TimeAxisView* _pending_resize_view;

	void visible_order_range (int*, int*) const;

	void located ();

	/** true if we've made a locate request that hasn't yet been processed */
	bool _pending_locate_request;

	/** if true, there is a pending Session locate which is the initial one when loading a session;
	    we need to know this so that we don't (necessarily) set the viewport to show the playhead
	    initially.
	*/
	bool _pending_initial_locate;

	Gtk::HBox _summary_hbox;
	EditorSummary* _summary;

	void region_view_added (RegionView*);
	void region_view_removed ();

	EditorGroupTabs* _group_tabs;
	void fit_route_group (ARDOUR::RouteGroup*);

	void step_edit_status_change (bool);
	void start_step_editing ();
	void stop_step_editing ();
	bool check_step_edit ();
	sigc::connection step_edit_connection;

	double _last_motion_y;

	RegionLayeringOrderEditor* layering_order_editor;
	void update_region_layering_order_editor ();

	/** Track that was the source for the last cut/copy operation.  Used as a place
	    to paste things iff there is no selected track.
	*/
	TimeAxisView* _last_cut_copy_source_track;

	/** true if a change in Selection->regions should change the selection in the region list.
	    See EditorRegions::selection_changed.
	*/
	bool _region_selection_change_updates_region_list;

	void setup_fade_images ();
	std::map<ARDOUR::FadeShape, Gtk::Image*> _xfade_in_images;
	std::map<ARDOUR::FadeShape, Gtk::Image*> _xfade_out_images;

	Gtk::MenuItem& action_menu_item (std::string const &);
	void action_pre_activated (Glib::RefPtr<Gtk::Action> const &);

	void follow_mixer_selection ();
	bool _following_mixer_selection;

	/* RTAV Automation display option */
	void toggle_show_touched_automation ();
	void set_show_touched_automation (bool);

	int time_fx (ARDOUR::RegionList&, Temporal::ratio_t ratio, bool pitching, bool fixed_end);
	void toggle_sound_midi_notes ();

	/** Flag for a bit of a hack wrt control point selection; see set_selected_control_point_from_click */
	bool _control_point_toggled_on_press;

	/** This is used by TimeAxisView to keep a track of the TimeAxisView that is currently being
	    stepped in height using ScrollZoomVerticalModifier+Scrollwheel.  When a scroll event
	    occurs, we do the step on this _stepping_axis_view if it is non-0 (and we set up this
	    _stepping_axis_view with the TimeAxisView underneath the mouse if it is 0).  Then Editor
	    resets _stepping_axis_view when the modifier key is released.  In this (hacky) way,
	    pushing the modifier key and moving the scroll wheel will operate on the same track
	    until the key is released (rather than skipping about to whatever happens to be
	    underneath the mouse at the time).
	*/
	TimeAxisView* _stepping_axis_view;
	void zoom_vertical_modifier_released();

	void bring_in_callback (Gtk::Label*, uint32_t n, uint32_t total, std::string name);
	void update_bring_in_message (Gtk::Label* label, uint32_t n, uint32_t total, std::string name);
	void bring_all_sources_into_session ();

	MainMenuDisabler* _main_menu_disabler;

	/* private helper functions to help with registering region actions */

	Glib::RefPtr<Gtk::Action> register_region_action (Glib::RefPtr<Gtk::ActionGroup> group, Editing::RegionActionTarget, char const* name, char const* label, sigc::slot<void> slot);
	void register_toggle_region_action (Glib::RefPtr<Gtk::ActionGroup> group, Editing::RegionActionTarget, char const* name, char const* label, sigc::slot<void> slot);

	void remove_gap_marker_callback (Temporal::timepos_t at, Temporal::timecnt_t distance);

	Editing::GridType determine_mapping_grid_snap (Temporal::timepos_t t);
	void choose_mapping_drag (ArdourCanvas::Item*, GdkEvent*);

	template<typename T>
	Temporal::TimeDomain drag_time_domain (T* thing_with_time_domain) {
		return thing_with_time_domain ? thing_with_time_domain->time_domain() : Temporal::AudioTime;
	}

	template<typename T>
	Temporal::TimeDomain drag_time_domain (std::shared_ptr<T> thing_with_time_domain) {
		return thing_with_time_domain ? thing_with_time_domain->time_domain() : Temporal::AudioTime;
	}

	void clear_tempo_markers_before (Temporal::timepos_t where, bool stop_at_music_times);
	void clear_tempo_markers_after (Temporal::timepos_t where, bool stop_at_music_times);
	void clear_tempo_markers () {
		clear_tempo_markers_after (Temporal::timepos_t (0), false);
	}

	Temporal::DomainBounceInfo* domain_bounce_info;

	friend class Drag;
	friend class RegionCutDrag;
	friend class RegionDrag;

	struct TrackDrag {
		RouteTimeAxisView* track;
		GdkCursor*         drag_cursor;
		GdkCursor*         predrag_cursor;
		TimeAxisView*      bump_track;
		double             start;
		double             current;
		double             previous;
		bool               have_predrag_cursor;
		int                direction;
		bool               first_move;
		bool               did_reorder;

		TrackDrag (RouteTimeAxisView* rtav, ARDOUR::Session& s)
			: track (rtav)
			, drag_cursor (nullptr)
			, predrag_cursor (nullptr)
			, bump_track (nullptr)
			, start (-1.)
			, current (0.)
			, previous (0.)
			, have_predrag_cursor (false)
			, direction (0)
			, first_move (true)
			, did_reorder (false)
		{}

	};
	TrackDrag* track_drag;

	MarkerBarType _visible_marker_types;
	RangeBarType _visible_range_types;
	void update_mark_and_range_visibility ();
	void show_marker_type (MarkerBarType);
	void show_range_type (RangeBarType);
	PBD::Signal<void()> VisibleMarkersChanged;
	PBD::Signal<void()> VisibleRangesChanged;

	friend class RegionMoveDrag;
	friend class TrimDrag;
	friend class MappingTwistDrag;
	friend class MappingEndDrag;
	friend class MeterMarkerDrag;
	friend class BBTMarkerDrag;
	friend class TempoMarkerDrag;
	friend class TempoCurveDrag;
	friend class TempoTwistDrag;
	friend class TempoEndDrag;
	friend class CursorDrag;
	friend class FadeInDrag;
	friend class FadeOutDrag;
	friend class MarkerDrag;
	friend class RegionGainDrag;
	friend class ControlPointDrag;
	friend class LineDrag;
	friend class RubberbandSelectDrag;
	friend class EditorRubberbandSelectDrag;
	friend class TimeFXDrag;
	friend class SelectionDrag;
	friend class RangeMarkerBarDrag;
	friend class MouseZoomDrag;
	friend class RegionCreateDrag;
	friend class RegionMotionDrag;
	friend class RegionInsertDrag;
	friend class VideoTimeLineDrag;

	friend class EditorSummary;
	friend class EditorGroupTabs;

	friend class EditorRoutes;
	friend class RhythmFerret;
};
