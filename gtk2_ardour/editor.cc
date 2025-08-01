/*
 * Copyright (C) 2005-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2005-2009 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2006-2009 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2008 Hans Baier <hansfbaier@googlemail.com>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2014-2017 Nick Mainsbridge <mainsbridge@gmail.com>
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

/* Note: public Editor methods are documented in public_editor.h */

#include <stdint.h>
#include <unistd.h>
#include <cstdlib>
#include <cmath>
#include <string>
#include <algorithm>
#include <map>

#include "ardour_ui.h"
/*
 * ardour_ui.h include was moved to the top of the list
 * due to a conflicting definition of 'Style' between
 * Apple's MacTypes.h and BarController.
 */


#include <sigc++/bind.h>

#include "pbd/convert.h"
#include "pbd/error.h"
#include "pbd/enumwriter.h"
#include "pbd/memento_command.h"
#include "pbd/unknown_type.h"
#include "pbd/unwind.h"
#include "pbd/timersub.h"

#include <glibmm/datetime.h> /*for playlist group_id */
#include <glibmm/miscutils.h>
#include <glibmm/uriutils.h>
#include <ytkmm/image.h>
#include <ydkmm/color.h>
#include <ydkmm/bitmap.h>

#include <ytkmm/menu.h>
#include <ytkmm/menuitem.h>

#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/gtk_ui.h"
#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/window_title.h"
#include "gtkmm2ext/cell_renderer_pixbuf_toggle.h"

#include "ardour/analysis_graph.h"
#include "ardour/audio_track.h"
#include "ardour/audioengine.h"
#include "ardour/audioregion.h"
#include "ardour/lmath.h"
#include "ardour/location.h"
#include "ardour/profile.h"
#include "ardour/route.h"
#include "ardour/route_group.h"
#include "ardour/session_playlists.h"
#include "ardour/tempo.h"
#include "ardour/utils.h"
#include "ardour/vca_manager.h"
#include "ardour/vca.h"

#include "canvas/debug.h"
#include "canvas/note.h"
#include "canvas/text.h"

#include "widgets/ardour_spacer.h"
#include "widgets/eventboxext.h"
#include "widgets/tooltips.h"
#include "widgets/prompter.h"

#include "control_protocol/control_protocol.h"

#include "actions.h"
#include "analysis_window.h"
#include "ardour_message.h"
#include "audio_clock.h"
#include "audio_region_view.h"
#include "audio_streamview.h"
#include "audio_time_axis.h"
#include "automation_time_axis.h"
#include "bundle_manager.h"
#include "crossfade_edit.h"
#include "debug.h"
#include "editing.h"
#include "editing_convert.h"
#include "editor.h"
#include "editor_cursors.h"
#include "editor_drag.h"
#include "editor_group_tabs.h"
#include "editor_locations.h"
#include "editor_regions.h"
#include "editor_route_groups.h"
#include "editor_routes.h"
#include "editor_section_box.h"
#include "editor_sections.h"
#include "editor_snapshots.h"
#include "editor_sources.h"
#include "editor_summary.h"
#include "enums_convert.h"
#include "export_report.h"
#include "global_port_matrix.h"
#include "gui_object.h"
#include "gui_thread.h"
#include "keyboard.h"
#include "luainstance.h"
#include "marker.h"
#include "midi_region_view.h"
#include "midi_time_axis.h"
#include "midi_view.h"
#include "mixer_strip.h"
#include "mixer_ui.h"
#include "mouse_cursors.h"
#include "note_base.h"
#include "opts.h"
#include "pianoroll.h"
#include "plugin_setup_dialog.h"
#include "public_editor.h"
#include "quantize_dialog.h"
#include "region_peak_cursor.h"
#include "region_layering_order_editor.h"
#include "rgb_macros.h"
#include "rhythm_ferret.h"
#include "route_sorter.h"
#include "selection.h"
#include "selection_properties_box.h"
#include "simple_progress_dialog.h"
#include "sfdb_ui.h"
#include "time_axis_view.h"
#include "timers.h"
#include "ui_config.h"
#include "utils.h"
#include "vca_time_axis.h"
#include "verbose_cursor.h"

#include "pbd/i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace ArdourWidgets;
using namespace ARDOUR_UI_UTILS;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace Editing;
using namespace Temporal;

using PBD::internationalize;
using PBD::atoi;
using Gtkmm2ext::Keyboard;

double Editor::timebar_height = 15.0;

static const gchar *_edit_point_strings[] = {
	N_("Playhead"),
	N_("Marker"),
	N_("Mouse"),
	0
};

static const gchar *_edit_mode_strings[] = {
	N_("Slide"),
	N_("Ripple"),
	N_("Lock"),
	0
};

static const gchar *_ripple_mode_strings[] = {
	N_("Selected"),
	N_("All"),
	N_("Interview"),
	0
};

#ifdef USE_RUBBERBAND
static const gchar *_rb_opt_strings[] = {
	N_("Mushy"),
	N_("Smooth"),
	N_("Balanced multitimbral mixture"),
	N_("Unpitched percussion with stable notes"),
	N_("Crisp monophonic instrumental"),
	N_("Unpitched solo percussion"),
	N_("Resample without preserving pitch"),
#ifdef HAVE_SOUNDTOUCH
	N_("Vocal"),
#endif
	0
};
#endif

Editor::Editor ()
	: PublicEditor ()
	, editor_mixer_strip_width (Wide)
	, constructed (false)
	, _properties_box (nullptr)
	, _pianoroll (nullptr)
	, no_save_visual (false)
	, marker_click_behavior (MarkerClickSelectOnly)
	, _join_object_range_state (JOIN_OBJECT_RANGE_NONE)
	, _show_marker_lines (false)
	, clicked_axisview (nullptr)
	, clicked_routeview (nullptr)
	, clicked_regionview (nullptr)
	, clicked_selection (0)
	, clicked_control_point (nullptr)
	, button_release_can_deselect (true)
	, _popup_region_menu_item (nullptr)
	, _track_canvas (nullptr)
	, _track_canvas_viewport (nullptr)
	, _region_peak_cursor (nullptr)
	, tempo_group (nullptr)
	, meter_group (nullptr)
	, marker_group (nullptr)
	, range_marker_group (nullptr)
	, section_marker_group (nullptr)
	, _time_markers_group (nullptr)
	, _selection_marker_group (nullptr)
	, _selection_marker (new LocationMarkers)
	, hv_scroll_group (nullptr)
	, h_scroll_group (nullptr)
	, cursor_scroll_group (nullptr)
	, no_scroll_group (nullptr)
	, _trackview_group (nullptr)
	, _drag_motion_group (nullptr)
	, _canvas_drop_zone (nullptr)
	, _canvas_grid_zone (nullptr)
	, no_ruler_shown_update (false)
	,  ruler_grabbed_widget (nullptr)
	, ruler_dialog (nullptr)
	, minsec_mark_interval (0)
	, minsec_mark_modulo (0)
	, minsec_nmarks (0)
	, timecode_ruler_scale (timecode_show_many_hours)
	, timecode_mark_modulo (0)
	, timecode_nmarks (0)
	, _samples_ruler_interval (0)
	, timecode_ruler (nullptr)
	, bbt_ruler (nullptr)
	, samples_ruler (nullptr)
	, minsec_ruler (nullptr)
	, visible_timebars (0)
	, editor_ruler_menu (nullptr)
	, tempo_bar (nullptr)
	, meter_bar (nullptr)
	, marker_bar (nullptr)
	, range_marker_bar (nullptr)
	, section_marker_bar (nullptr)
	, ruler_separator (nullptr)
	, _ruler_btn_tempo_add ("+")
	, _ruler_btn_meter_add ("+")
	, _ruler_btn_range_prev ("<")
	, _ruler_btn_range_next (">")
	, _ruler_btn_range_add ("+")
	, _ruler_btn_loc_prev ("<")
	, _ruler_btn_loc_next (">")
	, _ruler_btn_loc_add ("+")
	, _ruler_btn_section_prev ("<")
	, _ruler_btn_section_next (">")
	, _ruler_btn_section_add ("+")
	, videotl_label (_("Video Timeline"))
	, videotl_group (nullptr)
	, videotl_bar_height (4)
	, _region_boundary_cache_dirty (true)
	, edit_packer (4, 4, true)
	, unused_adjustment (0.0, 0.0, 10.0, 400.0)
	, controls_layout (unused_adjustment, vertical_adjustment)
	, _scroll_callbacks (0)
	, _full_canvas_height (0)
	, edit_controls_left_menu (nullptr)
	, edit_controls_right_menu (nullptr)
	, _tvl_no_redisplay(false)
	, _tvl_redisplay_on_resume(false)
	, _last_update_time (0)
	, _err_screen_engine (0)
	, cut_buffer_start (0)
	, cut_buffer_length (0)
	, last_paste_pos (timepos_t::max (Temporal::AudioTime)) /* XXX NUTEMPO how to choose time domain */
	, paste_count (0)
	, sfbrowser (nullptr)
	, current_interthread_info (nullptr)
	, analysis_window (nullptr)
	, select_new_marker (false)
	, have_pending_keyboard_selection (false)
	, pending_keyboard_selection_start (0)
	, ignore_gui_changes (false)
	, lock_dialog (nullptr)
	, _last_event_time (g_get_monotonic_time ())
	, _dragging_playhead (false)
	, ignore_map_change (false)
	, _stationary_playhead (false)
	, _maximised (false)
	, global_rect_group (nullptr)
	, tempo_marker_menu (nullptr)
	, meter_marker_menu (nullptr)
	, bbt_marker_menu (nullptr)
	, marker_menu (nullptr)
	, range_marker_menu (nullptr)
	, new_transport_marker_menu (nullptr)
	, marker_menu_item (nullptr)
	, _visible_track_count (-1)
	,  toolbar_selection_clock_table (2,3)
	,  automation_mode_button (_("mode"))
	, _all_region_actions_sensitized (false)
	, _ignore_region_action (false)
	, _last_region_menu_was_main (false)
	, _track_selection_change_without_scroll (false)
	, _editor_track_selection_change_without_scroll (false)
	, _section_box (nullptr)
	, range_bar_drag_rect (nullptr)
	, transport_bar_preroll_rect (nullptr)
	, transport_bar_postroll_rect (nullptr)
	, transport_loop_range_rect (nullptr)
	, transport_punch_range_rect (nullptr)
	, transport_punchin_line (nullptr)
	, transport_punchout_line (nullptr)
	, transport_preroll_rect (nullptr)
	, transport_postroll_rect (nullptr)
	, temp_location (nullptr)
	, _route_groups (nullptr)
	, _routes (nullptr)
	, _regions (nullptr)
	, _sections (nullptr)
	, _snapshots (nullptr)
	, _locations (nullptr)
	, show_gain_after_trim (false)
	, _no_not_select_reimported_tracks (false)
	, selection_op_cmd_depth (0)
	, selection_op_history_it (0)
	, no_save_instant (false)
	, current_timefx (nullptr)
	, current_mixer_strip (nullptr)
	, show_editor_mixer_when_tracks_arrive (false)
	,  nudge_clock (new AudioClock (X_("nudge"), false, X_("nudge"), true, false, true))
	, current_stepping_trackview (nullptr)
	, last_track_height_step_timestamp (0)
	, _edit_point (EditAtMouse)
	, meters_running (false)
	, rhythm_ferret (nullptr)
	, _have_idled (false)
	, resize_idle_id (-1)
	, _pending_resize_amount (0)
	, _pending_resize_view (nullptr)
	, _pending_locate_request (false)
	, _pending_initial_locate (false)
	, _summary (nullptr)
	, _group_tabs (nullptr)
	, _last_motion_y (0)
	, layering_order_editor (nullptr)
	, _last_cut_copy_source_track (nullptr)
	, _region_selection_change_updates_region_list (true)
	, _following_mixer_selection (false)
	, _control_point_toggled_on_press (false)
	, _stepping_axis_view (nullptr)
	, _main_menu_disabler (nullptr)
	, domain_bounce_info (nullptr)
	, track_drag (nullptr)
	, _visible_marker_types (all_marker_types)
	, _visible_range_types (all_range_types)
{
	/* we are a singleton */

	PublicEditor::_instance = this;

	_have_idled = false;

	selection_op_history.clear();
	before.clear();

	edit_mode_strings = I18N (_edit_mode_strings);
	ripple_mode_strings = I18N (_ripple_mode_strings);
	edit_point_strings = I18N (_edit_point_strings);
#ifdef USE_RUBBERBAND
	rb_opt_strings = I18N (_rb_opt_strings);
	rb_current_opt = 4;
#endif

	timebar_height = std::max (13., ceil (17. * UIConfiguration::instance().get_ui_scale()));

	TimeAxisView::setup_sizes ();
	ArdourMarker::setup_sizes (timebar_height);
	TempoCurve::setup_sizes (timebar_height);

	Gtk::Table* rtbl;

	rtbl = setup_ruler_new (_ruler_box_minsec, _ruler_labels, _("Mins:Secs"));

	rtbl = setup_ruler_new (_ruler_box_timecode, _ruler_labels, _("Timecode"));

	rtbl = setup_ruler_new (_ruler_box_samples, _ruler_labels, _("Samples"));

	rtbl = setup_ruler_new (_ruler_box_bbt, _ruler_labels, _("Bars:Beats"));

	rtbl = setup_ruler_new (_ruler_box_tempo, _ruler_labels, _("Tempo"));
	setup_ruler_add (rtbl, _ruler_btn_tempo_add);

	rtbl = setup_ruler_new (_ruler_box_meter, _ruler_labels, _("Time Signature"));
	setup_ruler_add (rtbl, _ruler_btn_meter_add);

	rtbl = setup_ruler_new (_ruler_box_range, _ruler_labels, _("Range Markers"));
	setup_ruler_add (rtbl, _ruler_btn_range_prev, 0);
	setup_ruler_add (rtbl, _ruler_btn_range_add, 1);
	setup_ruler_add (rtbl, _ruler_btn_range_next, 2);

	rtbl = setup_ruler_new (_ruler_box_marker, _ruler_labels, _("Location Markers"));
	setup_ruler_add (rtbl, _ruler_btn_loc_prev, 0);
	setup_ruler_add (rtbl, _ruler_btn_loc_add, 1);
	setup_ruler_add (rtbl, _ruler_btn_loc_next, 2);

	rtbl = setup_ruler_new (_ruler_box_section, _ruler_labels, _("Arrangement Markers"));
	setup_ruler_add (rtbl, _ruler_btn_section_prev, 0);
	setup_ruler_add (rtbl, _ruler_btn_section_add, 1);
	setup_ruler_add (rtbl, _ruler_btn_section_next, 2);

	rtbl = setup_ruler_new (_ruler_box_videotl, _ruler_labels, &videotl_label);
	videotl_label.set_size_request (-1, 4 * timebar_height);

	initialize_canvas ();

	CairoWidget::set_focus_handler (sigc::mem_fun (ARDOUR_UI::instance(), &ARDOUR_UI::reset_focus));

	_summary = new EditorSummary (*this);

	TempoMap::MapChanged.connect (tempo_map_connection, invalidator (*this), std::bind (&Editor::tempo_map_changed, this), gui_context());

	selection->TimeChanged.connect (sigc::mem_fun(*this, &Editor::time_selection_changed));
	selection->TracksChanged.connect (sigc::mem_fun(*this, &Editor::track_selection_changed));

	ZoomChanged.connect (sigc::mem_fun (*this, &Editor::update_section_rects));

	editor_regions_selection_changed_connection = selection->RegionsChanged.connect (sigc::mem_fun(*this, &Editor::region_selection_changed));

	selection->MarkersChanged.connect (sigc::mem_fun(*this, &Editor::marker_selection_changed));

	edit_controls_vbox.set_spacing (0);
	vertical_adjustment.signal_value_changed().connect (sigc::mem_fun(*this, &Editor::tie_vertical_scrolling), true);
	_track_canvas->signal_map_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_map_handler));

	_group_tabs = new EditorGroupTabs (*this);
	controls_layout.add (edit_controls_vbox);

	controls_layout.signal_expose_event ().connect (sigc::bind (sigc::ptr_fun (&ArdourWidgets::ArdourIcon::expose_with_text), &controls_layout, ArdourWidgets::ArdourIcon::ShadedPlusSign, _("Right-click\nor Double-click here\nto add Track, Bus,\n or VCA.")));

	HSeparator* separator = manage (new HSeparator());
	separator->set_name("TrackSeparator");
	separator->set_size_request(-1, 1);
	separator->show();
	edit_controls_vbox.pack_end (*separator, false, false);

	controls_layout.set_name ("EditControlsBase");
	controls_layout.add_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::ENTER_NOTIFY_MASK|Gdk::LEAVE_NOTIFY_MASK|Gdk::SCROLL_MASK);
	controls_layout.signal_button_press_event().connect (sigc::mem_fun(*this, &Editor::edit_controls_button_event));
	controls_layout.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::edit_controls_button_event));
	controls_layout.signal_scroll_event().connect (sigc::mem_fun(*this, &Editor::control_layout_scroll), false);

	_group_tabs->signal_scroll_event().connect (sigc::mem_fun(*this, &Editor::control_layout_scroll), false);

	set_canvas_cursor (nullptr);

	ArdourCanvas::GtkCanvas* time_pad = manage (new ArdourCanvas::GtkCanvas ());

	ArdourCanvas::Line* pad_line_1 = new ArdourCanvas::Line (time_pad->root());
	pad_line_1->set (ArdourCanvas::Duple (0.0, 1.0), ArdourCanvas::Duple (100.0, 1.0));
	pad_line_1->set_outline_color (0xFF0000FF);
	pad_line_1->show();

	/* CAIROCANVAS */
	time_pad->show();

	edit_packer.set_col_spacings (0);
	edit_packer.set_row_spacings (0);
	edit_packer.set_homogeneous (false);
	edit_packer.set_border_width (0);
	edit_packer.set_name ("EditorWindow");

	time_bars_event_box.add (time_bars_vbox);
	time_bars_event_box.set_events (Gdk::BUTTON_PRESS_MASK|Gdk::BUTTON_RELEASE_MASK);
	time_bars_event_box.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::ruler_label_button_release));

#ifndef MIXBUS
	ArdourWidgets::ArdourDropShadow *axis_view_shadow = manage (new (ArdourWidgets::ArdourDropShadow));
	axis_view_shadow->set_size_request (4, -1);
	axis_view_shadow->set_name("EditorWindow");
	axis_view_shadow->show();

	edit_packer.attach (*axis_view_shadow,       0, 1, 0, 2,    FILL,        FILL|EXPAND, 0, 0);
#endif

	/* labels for the time bars */
	edit_packer.attach (time_bars_event_box,     1, 3, 0, 1,    FILL,        SHRINK,      5, 0);
	/* track controls */
	edit_packer.attach (*_group_tabs,            1, 2, 1, 2,    FILL,        FILL|EXPAND, 0, 0);
	edit_packer.attach (controls_layout,         2, 3, 1, 2,    FILL,        FILL|EXPAND, 0, 0);
	/* canvas */
	edit_packer.attach (*_track_canvas_viewport, 3, 4, 0, 2,    FILL|EXPAND, FILL|EXPAND, 0, 0);

	bottom_hbox.set_border_width (2);
	bottom_hbox.set_spacing (3);

	PresentationInfo::Change.connect (*this, MISSING_INVALIDATOR, std::bind (&Editor::presentation_info_changed, this, _1), gui_context());

	_route_groups = new EditorRouteGroups (*this);
	_routes = new EditorRoutes ();
	_regions = new EditorRegions (*this);
	_sources = new EditorSources (*this);
	_sections = new EditorSections (*this);
	_snapshots = new EditorSnapshots ();
	_locations = new EditorLocations (*this);
	_properties_box = new SelectionPropertiesBox ();

	/* these are static location signals */

	Location::start_changed.connect (*this, invalidator (*this), std::bind (&Editor::location_changed, this, _1), gui_context());
	Location::end_changed.connect (*this, invalidator (*this), std::bind (&Editor::location_changed, this, _1), gui_context());
	Location::changed.connect (*this, invalidator (*this), std::bind (&Editor::location_changed, this, _1), gui_context());

	add_notebook_page (_("Tracks"), _("Tracks & Busses"), _routes->widget ());
	add_notebook_page (_("Sources"), _("Sources"), _sources->widget ());
	add_notebook_page (_("Regions"), _("Regions"), _regions->widget ());
	add_notebook_page (_("Clips"), _("Clips"), _trigger_clip_picker);
	add_notebook_page (_("Arrange"), _("Arrangement"), _sections->widget ());
	add_notebook_page (_("Snaps"), _("Snapshots"), _snapshots->widget ());
	add_notebook_page (_("Groups"), _("Track & Bus Groups"), _route_groups->widget ());
	add_notebook_page (_("Marks"), _("Ranges & Marks"), _locations->widget ());

	_notebook_tab2.set_index (4);

	_the_notebook.set_show_tabs (false);
	_the_notebook.set_scrollable (true);
	_the_notebook.popup_disable ();
	_the_notebook.show_all ();

	_the_notebook.signal_switch_page().connect ([this](GtkNotebookPage*, guint page) {
			std::string label (_the_notebook.get_tab_label_text (*_the_notebook.get_nth_page (page)));
			_notebook_tab1.set_active (label);
			_notebook_tab2.set_active (label);
			instant_save ();
			});

	_notebook_tab1.set_name ("tab button");
	_notebook_tab2.set_name ("tab button");

	/* Pick up some settings we need to cache, early */

	XMLNode* settings = ARDOUR_UI::instance()->editor_settings();

	editor_summary_pane.set_check_divider_position (true);
	editor_summary_pane.add (edit_packer);

	Button* summary_arrow_left = manage (new Button);
	summary_arrow_left->add (*manage (new Arrow (ARROW_LEFT, SHADOW_NONE)));
	summary_arrow_left->signal_pressed().connect (sigc::hide_return (sigc::bind (sigc::mem_fun (*this, &Editor::scroll_press), LEFT)));
	summary_arrow_left->signal_released().connect (sigc::mem_fun (*this, &Editor::scroll_release));

	Button* summary_arrow_right = manage (new Button);
	summary_arrow_right->add (*manage (new Arrow (ARROW_RIGHT, SHADOW_NONE)));
	summary_arrow_right->signal_pressed().connect (sigc::hide_return (sigc::bind (sigc::mem_fun (*this, &Editor::scroll_press), RIGHT)));
	summary_arrow_right->signal_released().connect (sigc::mem_fun (*this, &Editor::scroll_release));

	VBox* summary_arrows_left = manage (new VBox);
	summary_arrows_left->pack_start (*summary_arrow_left);

	VBox* summary_arrows_right = manage (new VBox);
	summary_arrows_right->pack_start (*summary_arrow_right);

	Gtk::Frame* summary_frame = manage (new Gtk::Frame);
	summary_frame->set_shadow_type (Gtk::SHADOW_ETCHED_IN);

	summary_frame->add (*_summary);
	summary_frame->show ();

	_summary_hbox.pack_start (*summary_arrows_left, false, false);
	_summary_hbox.pack_start (*summary_frame, true, true);
	_summary_hbox.pack_start (*summary_arrows_right, false, false);

	editor_summary_pane.add (_summary_hbox);

	HBox* tabbox = manage (new HBox (true));
	tabbox->set_spacing (3);
	tabbox->pack_start (_notebook_tab1);
	tabbox->pack_start (_notebook_tab2);

	_editor_list_vbox.pack_start (*tabbox, false, false, 2);
	_editor_list_vbox.pack_start (_the_notebook);

	content_right_pane.set_drag_cursor (*_cursors->expand_left_right);
	editor_summary_pane.set_drag_cursor (*_cursors->expand_up_down);

	float fract;
	if (!settings || !settings->get_property ("edit-vertical-pane-pos", fract) || fract > 1.0) {
		/* initial allocation is 90% to canvas, 10% to summary */
		fract = 0.90;
	}
	editor_summary_pane.set_divider (0, fract);

	global_vpacker.set_spacing (0);
	global_vpacker.set_border_width (0);

	ArdourWidgets::ArdourDropShadow *toolbar_shadow = manage (new (ArdourWidgets::ArdourDropShadow));
	toolbar_shadow->set_size_request (-1, 4);
	toolbar_shadow->set_mode(ArdourWidgets::ArdourDropShadow::DropShadowBoth);
	toolbar_shadow->set_name("EditorWindow");
	toolbar_shadow->show();

	global_vpacker.pack_start (*toolbar_shadow, false, false);
	global_vpacker.pack_start (ebox_hpacker, true, true);

	/* pack all the main pieces into appropriate containers from _tabbable
	 */
	content_app_bar.add (_application_bar);
	content_att_right.add (_editor_list_vbox);
	content_att_bottom.add (_bottom_hbox);
	content_main_top.add (global_vpacker);
	content_main.add (editor_summary_pane);

	/* need to show the "contents" widget so that notebook will show if tab is switched to
	 */

	ebox_hpacker.show();
	global_vpacker.show();
	_bottom_hbox.show();

	/* register actions now so that set_state() can find them and set toggles/checks etc */

	load_bindings ();
	register_actions ();
	bind_mouse_mode_buttons ();
	set_action_defaults ();

	build_edit_mode_menu();
	build_zoom_focus_menu();
	build_track_count_menu();
	build_grid_type_menu();
	build_draw_midi_menus();
	build_edit_point_menu();

	setup_toolbar ();

	ARDOUR_UI::instance()->ActionsReady.connect_same_thread (*this, std::bind (&Editor::initialize_ruler_actions, this));

	RegionView::RegionViewGoingAway.connect (*this, invalidator (*this),  std::bind (&Editor::catch_vanishing_regionview, this, _1), gui_context());

	/* nudge stuff */

	nudge_forward_button.set_name ("nudge button");
	nudge_forward_button.set_icon(ArdourIcon::NudgeRight);

	nudge_backward_button.set_name ("nudge button");
	nudge_backward_button.set_icon(ArdourIcon::NudgeLeft);

	fade_context_menu.set_name ("ArdourContextMenu");

	Gtkmm2ext::Keyboard::the_keyboard().ZoomVerticalModifierReleased.connect (sigc::mem_fun (*this, &Editor::zoom_vertical_modifier_released));

	/* allow external control surfaces/protocols to do various things */

	ControlProtocol::ZoomToSession.connect (*this, invalidator (*this), std::bind (&Editor::temporal_zoom_session, this), gui_context());
	ControlProtocol::ZoomIn.connect (*this, invalidator (*this), std::bind (&Editor::temporal_zoom_step, this, false), gui_context());
	ControlProtocol::ZoomOut.connect (*this, invalidator (*this), std::bind (&Editor::temporal_zoom_step, this, true), gui_context());
	ControlProtocol::Undo.connect (*this, invalidator (*this), std::bind (&Editor::undo, this, true), gui_context());
	ControlProtocol::Redo.connect (*this, invalidator (*this), std::bind (&Editor::redo, this, true), gui_context());
	ControlProtocol::ScrollTimeline.connect (*this, invalidator (*this), std::bind (&Editor::control_scroll, this, _1), gui_context());
	ControlProtocol::StepTracksUp.connect (*this, invalidator (*this), std::bind (&Editor::control_step_tracks_up, this), gui_context());
	ControlProtocol::StepTracksDown.connect (*this, invalidator (*this), std::bind (&Editor::control_step_tracks_down, this), gui_context());
	ControlProtocol::GotoView.connect (*this, invalidator (*this), std::bind (&Editor::control_view, this, _1), gui_context());
	ControlProtocol::CloseDialog.connect (*this, invalidator (*this), Keyboard::close_current_dialog, gui_context());
	ControlProtocol::VerticalZoomInAll.connect (*this, invalidator (*this), std::bind (&Editor::control_vertical_zoom_in_all, this), gui_context());
	ControlProtocol::VerticalZoomOutAll.connect (*this, invalidator (*this), std::bind (&Editor::control_vertical_zoom_out_all, this), gui_context());
	ControlProtocol::VerticalZoomInSelected.connect (*this, invalidator (*this), std::bind (&Editor::control_vertical_zoom_in_selected, this), gui_context());
	ControlProtocol::VerticalZoomOutSelected.connect (*this, invalidator (*this), std::bind (&Editor::control_vertical_zoom_out_selected, this), gui_context());

	BasicUI::AccessAction.connect (*this, invalidator (*this), std::bind (&Editor::access_action, this, _1, _2), gui_context());

	/* problematic: has to return a value and thus cannot be x-thread */

	Session::AskAboutPlaylistDeletion.connect_same_thread (*this, std::bind (&Editor::playlist_deletion_dialog, this, _1));
	Route::PluginSetup.connect_same_thread (*this, std::bind (&Editor::plugin_setup, this, _1, _2, _3));

	TimeAxisView::CatchDeletion.connect (*this, invalidator (*this), std::bind (&Editor::timeaxisview_deleted, this, _1), gui_context());

	_ignore_region_action = false;
	_last_region_menu_was_main = false;

	_show_marker_lines = false;

	constructed = true;

	/* grab current parameter state */
	std::function<void (string)> pc (std::bind (&Editor::ui_parameter_changed, this, _1));
	UIConfiguration::instance().map_parameters (pc);

	setup_fade_images ();
}

Editor::~Editor()
{
	delete own_bindings;
	delete tempo_marker_menu;
	delete meter_marker_menu;
	delete marker_menu;
	delete range_marker_menu;
	delete new_transport_marker_menu;
	delete editor_ruler_menu;
	delete _popup_region_menu_item;
	delete _selection_marker;

	delete button_bindings;
	delete _routes;
	delete _route_groups;
	delete _track_canvas_viewport;
	delete _drags;
	delete nudge_clock;
	delete _verbose_cursor;
	delete _region_peak_cursor;
	delete quantize_dialog;
	delete _summary;
	delete _group_tabs;
	delete _regions;
	delete _snapshots;
	delete _sections;
	delete _locations;
	delete _pianoroll;
	delete _properties_box;
	delete selection;
	delete cut_buffer;
	delete _cursors;

	LuaInstance::destroy_instance ();

	for (list<XMLNode *>::iterator i = selection_op_history.begin(); i != selection_op_history.end(); ++i) {
		delete *i;
	}
	for (std::map<ARDOUR::FadeShape, Gtk::Image*>::const_iterator i = _xfade_in_images.begin(); i != _xfade_in_images.end (); ++i) {
		delete i->second;
	}
	for (std::map<ARDOUR::FadeShape, Gtk::Image*>::const_iterator i = _xfade_out_images.begin(); i != _xfade_out_images.end (); ++i) {
		delete i->second;
	}
}

Gtk::Table*
Editor::setup_ruler_new (Gtk::HBox& box, vector<Gtk::Label*>& labels, std::string const& name)
{
	Gtk::Label* rlbl = manage (new Gtk::Label (name));
	return setup_ruler_new (box, labels, rlbl);
}

Gtk::Table*
Editor::setup_ruler_new (Gtk::HBox& box, vector<Gtk::Label*>& labels, Gtk::Label* rlbl)
{
	rlbl->set_name ("EditorRulerLabel");
	rlbl->set_size_request (-1, (int)timebar_height);
	rlbl->set_alignment (1.0, 0.5);
	rlbl->show ();
	labels.push_back (rlbl);

	Gtk::Table* rtbl = manage (new Gtk::Table);
	rtbl->attach (*rlbl, 0, 1, 0, 1, EXPAND|FILL, SHRINK, 2, 0);
	rtbl->show ();

	box.pack_start (*rtbl, true, true);
	box.hide();
	box.set_no_show_all();
	return rtbl;
}

void
Editor::setup_ruler_add (Gtk::Table* rtbl, ArdourWidgets::ArdourButton& b, int pos)
{
	b.set_name ("editor ruler button");
	b.set_size_request (-1, (int)timebar_height -2);
	b.set_tweaks(ArdourButton::Tweaks(ArdourButton::ForceBoxy | ArdourButton::ForceFlat));
	b.set_elements (ArdourButton::Element(ArdourButton::Text));
	b.show ();
	rtbl->attach (b, pos + 1, pos + 2, 0, 1, SHRINK, SHRINK, 0, 1);
}

void
Editor::dpi_reset ()
{
	timebar_height = std::max (13., ceil (17. * UIConfiguration::instance().get_ui_scale()));

	_ruler_btn_tempo_add.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_meter_add.set_size_request (-1, (int)timebar_height -2);

	_ruler_btn_range_add.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_range_prev.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_range_next.set_size_request (-1, (int)timebar_height -2);

	_ruler_btn_loc_add.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_loc_prev.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_loc_prev.set_size_request (-1, (int)timebar_height -2);

	_ruler_btn_section_add.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_section_prev.set_size_request (-1, (int)timebar_height -2);
	_ruler_btn_section_next.set_size_request (-1, (int)timebar_height -2);

	timecode_ruler->set_y1 (timecode_ruler->y0() + timebar_height);
	bbt_ruler->set_y1 (bbt_ruler->y0() + timebar_height);
	samples_ruler->set_y1 (samples_ruler->y0() + timebar_height);
	minsec_ruler->set_y1 (minsec_ruler->y0() + timebar_height);
	meter_bar->set_y1 (meter_bar->y0() + timebar_height);
	tempo_bar->set_y1 (tempo_bar->y0() + timebar_height);
	marker_bar->set_y1 (marker_bar->y0() + timebar_height);
	range_marker_bar->set_y1 (range_marker_bar->y0() + timebar_height);
	section_marker_bar->set_y1 (section_marker_bar->y0() + timebar_height);

	for (auto const& l : _ruler_labels) {
		l->set_size_request (-1, (int)timebar_height);
	}
	videotl_label.set_size_request (-1, 4 * timebar_height);
	set_video_timeline_height (videotl_bar_height, true); // calls update_ruler_visibility();

	ArdourMarker::setup_sizes (timebar_height);
	TempoCurve::setup_sizes (timebar_height);

	clear_marker_display ();
	refresh_location_display  ();
}

bool
Editor::get_smart_mode () const
{
	return ((current_mouse_mode() == MouseObject) && smart_mode_action->get_active());
}

void
Editor::catch_vanishing_regionview (RegionView *rv)
{
	/* note: the selection will take care of the vanishing
	   audioregionview by itself.
	*/

	if (_drags->active() && _drags->have_item (rv->get_canvas_group()) && !_drags->ending()) {
		_drags->abort ();
	}

	if (clicked_regionview == rv) {
		clicked_regionview = 0;
	}

	if (entered_regionview == rv) {
		set_entered_regionview (0);
	}

	if (!_all_region_actions_sensitized) {
		sensitize_all_region_actions (true);
	}
}

void
Editor::set_entered_regionview (RegionView* rv)
{
	if (rv == entered_regionview) {
		return;
	}

	if (entered_regionview) {
		entered_regionview->exited ();
	}

	entered_regionview = rv;

	if (entered_regionview  != 0) {
		entered_regionview->entered ();
	}

	if (!_all_region_actions_sensitized && _last_region_menu_was_main) {
		/* This RegionView entry might have changed what region actions
		   are allowed, so sensitize them all in case a key is pressed.
		*/
		sensitize_all_region_actions (true);
	}
}

void
Editor::set_entered_track (TimeAxisView* tav)
{
	if (entered_track) {
		entered_track->exited ();
	}

	entered_track = tav;

	if (entered_track) {
		entered_track->entered ();
	}
}

void
Editor::instant_save ()
{
	if (!constructed || !_session || no_save_instant) {
		return;
	}

	_session->add_instant_xml (get_state());
}

void
Editor::control_vertical_zoom_in_all ()
{
	tav_zoom_smooth (false, true);
}

void
Editor::control_vertical_zoom_out_all ()
{
	tav_zoom_smooth (true, true);
}

void
Editor::control_vertical_zoom_in_selected ()
{
	tav_zoom_smooth (false, false);
}

void
Editor::control_vertical_zoom_out_selected ()
{
	tav_zoom_smooth (true, false);
}

void
Editor::control_view (uint32_t view)
{
	goto_visual_state (view);
}

void
Editor::control_step_tracks_up ()
{
	scroll_tracks_up_line ();
}

void
Editor::control_step_tracks_down ()
{
	scroll_tracks_down_line ();
}

void
Editor::control_scroll (float fraction)
{
	ENSURE_GUI_THREAD (*this, &Editor::control_scroll, fraction)

	if (!_session) {
		return;
	}

	double step = fraction * current_page_samples();

	/*
		_control_scroll_target is an optional<T>

		it acts like a pointer to an samplepos_t, with
		a operator conversion to boolean to check
		that it has a value could possibly use
		_playhead_cursor->current_sample to store the
		value and a boolean in the class to know
		when it's out of date
	*/

	if (!_control_scroll_target) {
		_control_scroll_target = _session->transport_sample();
		_dragging_playhead = true;
	}

	if ((fraction < 0.0f) && (*_control_scroll_target <= (samplepos_t) fabs(step))) {
		*_control_scroll_target = 0;
	} else if ((fraction > 0.0f) && (max_samplepos - *_control_scroll_target < step)) {
		*_control_scroll_target = max_samplepos - (current_page_samples()*2); // allow room for slop in where the PH is on the screen
	} else {
		*_control_scroll_target += (samplepos_t) trunc (step);
	}

	/* move visuals, we'll catch up with it later */

	_playhead_cursor->set_position (*_control_scroll_target);
	update_section_box ();
	UpdateAllTransportClocks (*_control_scroll_target);

	if (*_control_scroll_target > (current_page_samples() / 2)) {
		/* try to center PH in window */
		reset_x_origin (*_control_scroll_target - (current_page_samples()/2));
	} else {
		reset_x_origin (0);
	}

	/*
		Now we do a timeout to actually bring the session to the right place
		according to the playhead. This is to avoid reading disk buffers on every
		call to control_scroll, which is driven by ScrollTimeline and therefore
		probably by a control surface wheel which can generate lots of events.
	*/
	/* cancel the existing timeout */

	control_scroll_connection.disconnect ();

	/* add the next timeout */

	control_scroll_connection = Glib::signal_timeout().connect (sigc::bind (sigc::mem_fun (*this, &Editor::deferred_control_scroll), *_control_scroll_target), 250);
}

bool
Editor::deferred_control_scroll (samplepos_t /*target*/)
{
	_session->request_locate (*_control_scroll_target);
	/* reset for next stream */
	_control_scroll_target = std::nullopt;
	_dragging_playhead = false;
	return false;
}

void
Editor::access_action (const std::string& action_group, const std::string& action_item)
{
	if (!_session) {
		return;
	}

	ENSURE_GUI_THREAD (*this, &Editor::access_action, action_group, action_item)

	RefPtr<Action> act;
	try {
		act = ActionManager::get_action (action_group.c_str(), action_item.c_str());
		if (act) {
			act->activate();
		}
	} catch ( ActionManager::MissingActionException const& e) {
		cerr << "MissingActionException:" << e.what () << endl;
	}
}

void
Editor::set_toggleaction (const std::string& action_group, const std::string& action_item, bool s)
{
	ActionManager::set_toggleaction_state (action_group.c_str(), action_item.c_str(), s);
}

void
Editor::on_realize ()
{
	Realized ();

	if (UIConfiguration::instance().get_lock_gui_after_seconds()) {
		start_lock_event_timing ();
	}
}

void
Editor::start_lock_event_timing ()
{
	/* check if we should lock the GUI every 30 seconds */

	Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::lock_timeout_callback), 30 * 1000);
}

bool
Editor::generic_event_handler (GdkEvent* ev)
{
	switch (ev->type) {
	case GDK_BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
	case GDK_MOTION_NOTIFY:
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		if (contents().get_mapped()) {
			_last_event_time = g_get_monotonic_time ();
		}
		break;

	case GDK_LEAVE_NOTIFY:
		switch (ev->crossing.detail) {
		case GDK_NOTIFY_UNKNOWN:
		case GDK_NOTIFY_INFERIOR:
		case GDK_NOTIFY_ANCESTOR:
			break;
		case GDK_NOTIFY_VIRTUAL:
		case GDK_NOTIFY_NONLINEAR:
		case GDK_NOTIFY_NONLINEAR_VIRTUAL:
			/* leaving window, so reset focus, thus ending any and
			   all text entry operations.
			*/
			ARDOUR_UI::instance()->reset_focus (&contents());
			break;
		}
		break;

	default:
		break;
	}

	return false;
}

bool
Editor::lock_timeout_callback ()
{
	int64_t dt = g_get_monotonic_time () - _last_event_time;

	if (dt * 1e-6 > UIConfiguration::instance().get_lock_gui_after_seconds()) {
		lock ();
		/* don't call again. Returning false will effectively
		   disconnect us from the timer callback.

		   unlock() will call start_lock_event_timing() to get things
		   started again.
		*/
		return false;
	}

	return true;
}

void
Editor::map_position_change (samplepos_t sample)
{
	ENSURE_GUI_THREAD (*this, &Editor::map_position_change, sample)

	if (_session == 0) {
		return;
	}

	if (follow_playhead()) {
		center_screen (sample);
	}

	if (!_session->locate_initiated()) {
		_playhead_cursor->set_position (sample);
	}

	update_section_box ();
}

void
Editor::update_title ()
{
	ENSURE_GUI_THREAD (*this, &Editor::update_title);

	if (!own_window()) {
		return;
	}

	if (_session) {
		bool dirty = _session->dirty();

		string session_name;

		if (_session->snap_name() != _session->name()) {
			session_name = _session->snap_name();
		} else {
			session_name = _session->name();
		}

		if (dirty) {
			session_name = "*" + session_name;
		}

		WindowTitle title(session_name);
		title += S_("Window|Editor");
		title += Glib::get_application_name();
		own_window()->set_title (title.get_string());
	} else {
		/* ::session_going_away() will have taken care of it */
	}
}

void
Editor::set_session (Session *t)
{
	EditingContext::set_session (t);

	section_marker_bar->clear (true);

	if (!_session) {
		return;
	}

	/* initialize _leftmost_sample to the extents of the session
	 * this prevents a bogus setting of leftmost = "0" if the summary view asks for the leftmost sample
	 * before the visible state has been loaded from instant.xml */
	_leftmost_sample = session_gui_extents().first.samples();

	_trigger_clip_picker.set_session (_session);
	_application_bar.set_session (_session);
	nudge_clock->set_session (_session);
	_summary->set_session (_session);
	_group_tabs->set_session (_session);
	_route_groups->set_session (_session);
	_regions->set_session (_session);
	_sources->set_session (_session);
	_snapshots->set_session (_session);
	_sections->set_session (_session);
	_routes->set_session (_session);
	_locations->set_session (_session);
	_properties_box->set_session (_session);

	/* Cannot initialize in constructor, because pianoroll needs Actions */
	if (!_pianoroll) {
		// XXX this should really not happen here
		_pianoroll = new Pianoroll ("editor pianoroll", true);
		_pianoroll->get_canvas_viewport()->set_size_request (-1, 120);
	}
	_pianoroll->set_session (_session);

	/* _pianoroll is packed on demand in Editor::region_selection_changed */
	_bottom_hbox.show_all();

	if (rhythm_ferret) {
		rhythm_ferret->set_session (_session);
	}

	if (analysis_window) {
		analysis_window->set_session (_session);
	}

	if (sfbrowser) {
		sfbrowser->set_session (_session);
	}

	initial_display ();
	compute_fixed_ruler_scale ();

	/* Make sure we have auto loop and auto punch ranges */

	Location* loc = _session->locations()->auto_loop_location();
	if (loc != 0) {
		loc->set_name (_("Loop"));
	}

	loc = _session->locations()->auto_punch_location();
	if (loc != 0) {
		/* force name */
		loc->set_name (_("Punch"));
	}

	refresh_location_display ();
	update_section_rects ();

	/* restore rulers before calling set_state() which sets the grid,
	 * which changes rulers and calls store_ruler_visibility() overriding
	 * any settings saved with the session.
	 */
	restore_ruler_visibility ();

	/* This must happen after refresh_location_display(), as (amongst other things) we restore
	 * the selected Marker; this needs the LocationMarker list to be available.
	 */
	XMLNode* node = ARDOUR_UI::instance()->editor_settings();
	set_state (*node, Stateful::loading_state_version);

	/* catch up on selection state, etc. */

	PropertyChange sc;
	sc.add (Properties::selected);
	presentation_info_changed (sc);

	/* catch up with the playhead */

	_session->request_locate (_playhead_cursor->current_sample (), false, MustStop);
	_pending_initial_locate = true;

	update_title ();

	/* These signals can all be emitted by a non-GUI thread. Therefore the
	   handlers for them must not attempt to directly interact with the GUI,
	   but use PBD::Signal<T>::connect() which accepts an event loop
	   ("context") where the handler will be asked to run.
	*/

	_session->StepEditStatusChange.connect (_session_connections, invalidator (*this), std::bind (&Editor::step_edit_status_change, this, _1), gui_context());
	_session->TransportStateChange.connect (_session_connections, invalidator (*this), std::bind (&Editor::map_transport_state, this), gui_context());
	_session->TransportLooped.connect (_session_connections, invalidator (*this), std::bind (&Editor::transport_looped, this), gui_context());
	_session->PositionChanged.connect (_session_connections, invalidator (*this), std::bind (&Editor::map_position_change, this, _1), gui_context());
	_session->vca_manager().VCAAdded.connect (_session_connections, invalidator (*this), std::bind (&Editor::add_vcas, this, _1), gui_context());
	_session->RouteAdded.connect (_session_connections, invalidator (*this), std::bind (&Editor::add_routes, this, _1), gui_context());
	_session->DirtyChanged.connect (_session_connections, invalidator (*this), std::bind (&Editor::update_title, this), gui_context());
	_session->Located.connect (_session_connections, invalidator (*this), std::bind (&Editor::located, this), gui_context());
	_session->config.ParameterChanged.connect (_session_connections, invalidator (*this), std::bind (&Editor::parameter_changed, this, _1), gui_context());
	_session->StateSaved.connect (_session_connections, invalidator (*this), std::bind (&Editor::session_state_saved, this, _1), gui_context());
	_session->locations()->added.connect (_session_connections, invalidator (*this), std::bind (&Editor::add_new_location, this, _1), gui_context());
	_session->locations()->removed.connect (_session_connections, invalidator (*this), std::bind (&Editor::location_gone, this, _1), gui_context());
	_session->locations()->changed.connect (_session_connections, invalidator (*this), std::bind (&Editor::refresh_location_display, this), gui_context());
	_session->auto_loop_location_changed.connect (_session_connections, invalidator (*this), std::bind (&Editor::loop_location_changed, this, _1), gui_context ());
	_session->RecordPassCompleted.connect (_session_connections, invalidator (*this), std::bind (&Editor::capture_sources_changed, this, false), gui_context ());
	_session->ClearedLastCaptureSources.connect (_session_connections, invalidator (*this), std::bind (&Editor::capture_sources_changed, this, true), gui_context ());
	_session->RecordStateChanged.connect (_session_connections, invalidator (*this), std::bind (&Editor::capture_sources_changed, this, false), gui_context ());
	Location::flags_changed.connect (_session_connections, invalidator (*this), std::bind (&Editor::update_section_rects, this), gui_context ());

	_session->history().Changed.connect (_session_connections, invalidator (*this), std::bind (&Editor::history_changed, this), gui_context());

	_playhead_cursor->canvas_item().reparent ((ArdourCanvas::Item*) get_cursor_scroll_group());
	_playhead_cursor->show ();

	_snapped_cursor->canvas_item().reparent ((ArdourCanvas::Item*) get_cursor_scroll_group());
	_snapped_cursor->set_color (UIConfiguration::instance().color ("edit point"));

	std::function<void (string)> pc (std::bind (&Editor::parameter_changed, this, _1));
	Config->map_parameters (pc);
	_session->config.map_parameters (pc);

	loop_location_changed (_session->locations()->auto_loop_location ());
	capture_sources_changed (true);

	//tempo_map_changed (PropertyChange (0));
	reset_metric_marks ();

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(static_cast<TimeAxisView*>(*i))->set_samples_per_pixel (samples_per_pixel);
	}

	super_rapid_screen_update_connection = Timers::super_rapid_connect (
		sigc::mem_fun (*this, &Editor::super_rapid_screen_update)
		);

	/* register for undo history */
	_session->register_with_memento_command_factory(id(), this);
	_session->register_with_memento_command_factory(_selection_memento->id(), _selection_memento);

	LuaInstance::instance()->set_session(_session);

	start_updating_meters ();
}

void
Editor::fill_xfade_menu (Menu_Helpers::MenuList& items, bool start)
{
	using namespace Menu_Helpers;

	void (Editor::*emf)(FadeShape);
	std::map<ARDOUR::FadeShape,Gtk::Image*>* images;

	if (start) {
		images = &_xfade_in_images;
		emf = &Editor::set_fade_in_shape;
	} else {
		images = &_xfade_out_images;
		emf = &Editor::set_fade_out_shape;
	}

	items.push_back (
		ImageMenuElem (
			_("Linear (for highly correlated material)"),
			*(*images)[FadeLinear],
			sigc::bind (sigc::mem_fun (*this, emf), FadeLinear)
			)
		);

	dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

	items.push_back (
		ImageMenuElem (
			_("Constant power"),
			*(*images)[FadeConstantPower],
			sigc::bind (sigc::mem_fun (*this, emf), FadeConstantPower)
			));

	dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

	items.push_back (
		ImageMenuElem (
			_("Symmetric"),
			*(*images)[FadeSymmetric],
			sigc::bind (sigc::mem_fun (*this, emf), FadeSymmetric)
			)
		);

	dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

	items.push_back (
		ImageMenuElem (
			_("Slow"),
			*(*images)[FadeSlow],
			sigc::bind (sigc::mem_fun (*this, emf), FadeSlow)
			));

	dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();

	items.push_back (
		ImageMenuElem (
			_("Fast"),
			*(*images)[FadeFast],
			sigc::bind (sigc::mem_fun (*this, emf), FadeFast)
			));

	dynamic_cast<ImageMenuItem*>(&items.back())->set_always_show_image ();
}

/** Pop up a context menu for when the user clicks on a start crossfade */
void
Editor::popup_xfade_in_context_menu (int button, int32_t time, ArdourCanvas::Item* item, ItemType /*item_type*/)
{
	using namespace Menu_Helpers;
	AudioRegionView* arv = dynamic_cast<AudioRegionView*> ((RegionView*)item->get_data ("regionview"));
	if (!arv) {
		return;
	}

	MenuList& items (xfade_in_context_menu.items());
	items.clear ();

	if (arv->audio_region()->fade_in_active()) {
		items.push_back (MenuElem (_("Deactivate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_active), false)));
	} else {
		items.push_back (MenuElem (_("Activate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_in_active), true)));
	}

	items.push_back (SeparatorElem());
	fill_xfade_menu (items, true);

	xfade_in_context_menu.popup (button, time);
}

/** Pop up a context menu for when the user clicks on an end crossfade */
void
Editor::popup_xfade_out_context_menu (int button, int32_t time, ArdourCanvas::Item* item, ItemType /*item_type*/)
{
	using namespace Menu_Helpers;
	AudioRegionView* arv = dynamic_cast<AudioRegionView*> ((RegionView*)item->get_data ("regionview"));
	if (!arv) {
		return;
	}

	MenuList& items (xfade_out_context_menu.items());
	items.clear ();

	if (arv->audio_region()->fade_out_active()) {
		items.push_back (MenuElem (_("Deactivate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_active), false)));
	} else {
		items.push_back (MenuElem (_("Activate"), sigc::bind (sigc::mem_fun (*this, &Editor::set_fade_out_active), true)));
	}

	items.push_back (SeparatorElem());
	fill_xfade_menu (items, false);

	xfade_out_context_menu.popup (button, time);
}

void
Editor::add_section_context_items (Gtk::Menu_Helpers::MenuList& items)
{
	using namespace Menu_Helpers;

	if (Profile->get_mixbus ()) {
		items.push_back (MenuElem (_("Copy/Paste Range Section to Playhead"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), CopyPasteSection)));
		items.push_back (MenuElem (_("Cut/Paste Range Section to Playhead"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), CutPasteSection)));
	} else {
		items.push_back (MenuElem (_("Copy/Paste Range Section to Edit Point"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), CopyPasteSection)));
		items.push_back (MenuElem (_("Cut/Paste Range Section to Edit Point"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), CutPasteSection)));
	}
	items.push_back (MenuElem (_("Delete Range Section"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), DeleteSection)));

#if 0
	items.push_back (SeparatorElem());
	items.push_back (MenuElem (_("Delete all markers in Section"), sigc::bind (sigc::mem_fun (*this, &Editor::cut_copy_section), DeleteSection)));
#endif

	timepos_t start, end;
	Location* l;
	if (get_selection_extents (start, end) && NULL != (l = _session->locations ()->mark_at (start))) {
		/* add some items from build_marker_menu () */
		LocationMarkers* lm = find_location_markers (l);
		assert (lm && lm->start);
		items.push_back (SeparatorElem());
		items.push_back (MenuElem (_("Move Playhead to Marker"), sigc::bind (sigc::mem_fun(*_session, &Session::request_locate), start.samples (), false, MustStop, TRS_UI)));
		items.push_back (MenuElem (_("Edit..."), sigc::bind (sigc::mem_fun(*this, &Editor::edit_marker), lm->start, true)));
	}

	items.push_back (SeparatorElem());
	add_selection_context_items (items, true);
}

void
Editor::popup_track_context_menu (int button, int32_t time, ItemType item_type, bool with_selection)
{
	using namespace Menu_Helpers;
	Menu* (Editor::*build_menu_function)();
	Menu *menu;

	switch (item_type) {
	case RegionItem:
	case RegionViewName:
	case RegionViewNameHighlight:
	case LeftFrameHandle:
	case RightFrameHandle:
		if (with_selection) {
			build_menu_function = &Editor::build_track_selection_context_menu;
		} else {
			build_menu_function = &Editor::build_track_region_context_menu;
		}
		break;

	case SelectionItem:
		if (with_selection) {
			build_menu_function = &Editor::build_track_selection_context_menu;
		} else {
			build_menu_function = &Editor::build_track_context_menu;
		}
		break;

	case StreamItem:
		if (clicked_routeview != 0 && clicked_routeview->track()) {
			build_menu_function = &Editor::build_track_context_menu;
		} else {
			build_menu_function = &Editor::build_track_bus_context_menu;
		}
		break;

	default:
		/* probably shouldn't happen but if it does, we don't care */
		return;
	}

	menu = (this->*build_menu_function)();
	menu->set_name ("ArdourContextMenu");

	/* now handle specific situations */

	switch (item_type) {
	case RegionItem:
	case RegionViewName:
	case RegionViewNameHighlight:
	case LeftFrameHandle:
	case RightFrameHandle:
		break;

	case SelectionItem:
		break;

	case StreamItem:
		break;

	default:
		/* probably shouldn't happen but if it does, we don't care */
		return;
	}

	if (item_type != SelectionItem && clicked_routeview && clicked_routeview->audio_track()) {

		/* Bounce to disk */

		using namespace Menu_Helpers;
		MenuList& edit_items  = menu->items();

		edit_items.push_back (SeparatorElem());

		switch (clicked_routeview->audio_track()->freeze_state()) {
		case AudioTrack::NoFreeze:
			edit_items.push_back (MenuElem (_("Freeze"), sigc::mem_fun(*this, &Editor::freeze_route)));
			break;

		case AudioTrack::Frozen:
			edit_items.push_back (MenuElem (_("Unfreeze"), sigc::mem_fun(*this, &Editor::unfreeze_route)));
			break;

		case AudioTrack::UnFrozen:
			edit_items.push_back (MenuElem (_("Freeze"), sigc::mem_fun(*this, &Editor::freeze_route)));
			break;
		}

	}

	/* When the region menu is opened, we setup the actions so that they look right
	   in the menu.
	*/
	sensitize_the_right_region_actions (false);
	_last_region_menu_was_main = false;

	menu->signal_hide().connect (sigc::bind (sigc::mem_fun (*this, &Editor::sensitize_all_region_actions), true));
	menu->popup (button, time);
}

Menu*
Editor::build_track_context_menu ()
{
	using namespace Menu_Helpers;

	MenuList& edit_items = track_context_menu.items();
	edit_items.clear();

	add_dstream_context_items (edit_items);
	return &track_context_menu;
}

Menu*
Editor::build_track_bus_context_menu ()
{
	using namespace Menu_Helpers;

	MenuList& edit_items = track_context_menu.items();
	edit_items.clear();

	add_bus_context_items (edit_items);
	return &track_context_menu;
}

Menu*
Editor::build_track_region_context_menu ()
{
	using namespace Menu_Helpers;
	MenuList& edit_items  = track_region_context_menu.items();
	edit_items.clear();

	/* we've just cleared the track region context menu, so the menu that these
	   two items were on will have disappeared; stop them dangling.
	*/
	RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (clicked_axisview);

	if (rtv) {
		std::shared_ptr<Track> tr;
		std::shared_ptr<Playlist> pl;

		if ((tr = rtv->track())) {
			add_region_context_items (edit_items, tr);
		}
	}

	add_dstream_context_items (edit_items);

	return &track_region_context_menu;
}

void
Editor::loudness_analyze_region_selection ()
{
	if (!_session) {
		return;
	}
	Selection& s (PublicEditor::instance ().get_selection ());
	RegionSelection ars = s.regions;
	ARDOUR::AnalysisGraph ag (_session);
	samplecnt_t total_work = 0;

	for (RegionSelection::iterator j = ars.begin (); j != ars.end (); ++j) {
		AudioRegionView* arv = dynamic_cast<AudioRegionView*> (*j);
		if (!arv) {
			continue;
		}
		if (!std::dynamic_pointer_cast<AudioRegion> (arv->region ())) {
			continue;
		}
		assert (dynamic_cast<RouteTimeAxisView *> (&arv->get_time_axis_view ()));
		total_work += arv->region ()->length_samples ();
	}

	SimpleProgressDialog spd (_("Region Loudness Analysis"), sigc::mem_fun (ag, &AnalysisGraph::cancel));
	ScopedConnection c;
	ag.set_total_samples (total_work);
	ag.Progress.connect_same_thread (c, std::bind (&SimpleProgressDialog::update_progress, &spd, _1, _2));
	spd.show();

	for (RegionSelection::iterator j = ars.begin (); j != ars.end (); ++j) {
		AudioRegionView* arv = dynamic_cast<AudioRegionView*> (*j);
		if (!arv) {
			continue;
		}
		std::shared_ptr<AudioRegion> ar = std::dynamic_pointer_cast<AudioRegion> (arv->region ());
		if (!ar) {
			continue;
		}
		ag.analyze_region (ar);
	}
	spd.hide();
	if (!ag.canceled ()) {
		ExportReport er (_("Audio Report/Analysis"), ag.results ());
		er.run();
	}
}

void
Editor::loudness_analyze_range_selection ()
{
	if (!_session) {
		return;
	}
	Selection& s (PublicEditor::instance ().get_selection ());
	TimeSelection ts = s.time;
	ARDOUR::AnalysisGraph ag (_session);
	samplecnt_t total_work = 0;

	for (TrackSelection::iterator i = s.tracks.begin (); i != s.tracks.end (); ++i) {
		std::shared_ptr<AudioPlaylist> pl = std::dynamic_pointer_cast<AudioPlaylist> ((*i)->playlist ());
		if (!pl) {
			continue;
		}
		RouteUI *rui = dynamic_cast<RouteUI *> (*i);
		if (!pl || !rui) {
			continue;
		}
		for (std::list<TimelineRange>::iterator j = ts.begin (); j != ts.end (); ++j) {
			total_work += j->length_samples ();
		}
	}

	SimpleProgressDialog spd (_("Range Loudness Analysis"), sigc::mem_fun (ag, &AnalysisGraph::cancel));
	ScopedConnection c;
	ag.set_total_samples (total_work);
	ag.Progress.connect_same_thread (c, std::bind (&SimpleProgressDialog::update_progress, &spd, _1, _2));
	spd.show();

	for (TrackSelection::iterator i = s.tracks.begin (); i != s.tracks.end (); ++i) {
		std::shared_ptr<AudioPlaylist> pl = std::dynamic_pointer_cast<AudioPlaylist> ((*i)->playlist ());
		if (!pl) {
			continue;
		}
		RouteUI *rui = dynamic_cast<RouteUI *> (*i);
		if (!pl || !rui) {
			continue;
		}
		ag.analyze_range (rui->route (), pl, ts);
	}
	spd.hide();
	if (!ag.canceled ()) {
		ExportReport er (_("Audio Report/Analysis"), ag.results ());
		er.run();
	}
}

void
Editor::spectral_analyze_region_selection ()
{
	if (analysis_window == 0) {
		analysis_window = new AnalysisWindow();

		if (_session != 0)
			analysis_window->set_session(_session);

		analysis_window->show_all();
	}

	analysis_window->set_regionmode();
	analysis_window->analyze();

	analysis_window->present();
}

void
Editor::spectral_analyze_range_selection()
{
	if (analysis_window == 0) {
		analysis_window = new AnalysisWindow();

		if (_session != 0)
			analysis_window->set_session(_session);

		analysis_window->show_all();
	}

	analysis_window->set_rangemode();
	analysis_window->analyze();

	analysis_window->present();
}

Menu*
Editor::build_track_selection_context_menu ()
{
	using namespace Menu_Helpers;
	MenuList& edit_items  = track_selection_context_menu.items();
	edit_items.clear ();

	add_selection_context_items (edit_items);
	// edit_items.push_back (SeparatorElem());
	// add_dstream_context_items (edit_items);

	return &track_selection_context_menu;
}

void
Editor::add_region_context_items (Menu_Helpers::MenuList& edit_items, std::shared_ptr<Track> track)
{
	using namespace Menu_Helpers;

	/* OK, stick the region submenu at the top of the list, and then add
	   the standard items.
	*/

	RegionSelection rs = get_regions_from_selection_and_entered ();

	string menu_item_name = (rs.size() == 1) ? rs.front()->region()->name() : _("Selected Regions");

	if (_popup_region_menu_item == 0) {
		_popup_region_menu_item = new MenuItem (menu_item_name, false);
		_popup_region_menu_item->set_submenu (*dynamic_cast<Menu*> (ActionManager::get_widget (X_("/PopupRegionMenu"))));
		_popup_region_menu_item->show ();
	} else {
		_popup_region_menu_item->set_label (menu_item_name);
	}

	/* No layering allowed in later is higher layering model */
	RefPtr<Action> act = ActionManager::get_action (X_("EditorMenu"), X_("RegionMenuLayering"));
	if (act && Config->get_layer_model() == LaterHigher) {
		act->set_sensitive (false);
	} else if (act) {
		act->set_sensitive (true);
	}

	const timepos_t position = get_preferred_edit_position (EDIT_IGNORE_NONE, true);

	edit_items.push_back (*_popup_region_menu_item);
	if (Config->get_layer_model() == Manual && track->playlist()->count_regions_at (position) > 1 && (layering_order_editor == 0 || !layering_order_editor->get_visible ())) {
		edit_items.push_back (*manage (_region_actions->get_action ("choose-top-region-context-menu")->create_menu_item ()));
	}
	edit_items.push_back (SeparatorElem());
}

/** Add context menu items relevant to selection ranges.
 * @param edit_items List to add the items to.
 */
void
Editor::add_selection_context_items (Menu_Helpers::MenuList& edit_items, bool time_selection_only)
{
	using namespace Menu_Helpers;

	edit_items.push_back (MenuElem (_("Play Range"), sigc::mem_fun(*this, &Editor::play_selection)));
	edit_items.push_back (MenuElem (_("Loop Range"), sigc::bind (sigc::mem_fun(*this, &Editor::set_loop_from_selection), true)));

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Zoom to Range"), sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_selection), Horizontal)));

	if (!time_selection_only) {
		edit_items.push_back (SeparatorElem());
		edit_items.push_back (MenuElem (_("Loudness Analysis"), sigc::mem_fun(*this, &Editor::loudness_analyze_range_selection)));
		edit_items.push_back (MenuElem (_("Spectral Analysis"), sigc::mem_fun(*this, &Editor::spectral_analyze_range_selection)));
		edit_items.push_back (SeparatorElem());
		edit_items.push_back (MenuElem (_("Loudness Assistant..."), sigc::bind (sigc::mem_fun (*this, &Editor::loudness_assistant), true)));
		edit_items.push_back (SeparatorElem());

		edit_items.push_back (
			MenuElem (
				_("Move Range Start to Previous Region Boundary"),
				sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), false, false)
				)
			);

		edit_items.push_back (
			MenuElem (
				_("Move Range Start to Next Region Boundary"),
				sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), false, true)
				)
			);

		edit_items.push_back (
			MenuElem (
				_("Move Range End to Previous Region Boundary"),
				sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), true, false)
				)
			);

		edit_items.push_back (
			MenuElem (
				_("Move Range End to Next Region Boundary"),
				sigc::bind (sigc::mem_fun (*this, &Editor::move_range_selection_start_or_end_to_region_boundary), true, true)
				)
			);
	}

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Separate"), mem_fun(*this, &Editor::separate_region_from_selection)));
//	edit_items.push_back (MenuElem (_("Convert to Region in Region List"), sigc::mem_fun(*this, &Editor::new_region_from_selection)));

	if (!time_selection_only) {
		edit_items.push_back (SeparatorElem());
		edit_items.push_back (MenuElem (_("Select All in Range"), sigc::mem_fun(*this, &Editor::select_all_selectables_using_time_selection)));
	}

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Set Loop from Selection"), sigc::bind (sigc::mem_fun(*this, &Editor::set_loop_from_selection), false)));
	edit_items.push_back (MenuElem (_("Set Punch from Selection"), sigc::mem_fun(*this, &Editor::set_punch_from_selection)));
	edit_items.push_back (MenuElem (_("Set Session Start/End from Selection"), sigc::mem_fun(*this, &Editor::set_session_extents_from_selection)));

	edit_items.push_back (SeparatorElem());

	if (!time_selection_only) {
		edit_items.push_back (MenuElem (_("Add Range Markers"), sigc::mem_fun (*this, &Editor::add_location_from_selection)));

		edit_items.push_back (SeparatorElem());

		edit_items.push_back (MenuElem (_("Crop Region to Range"), sigc::mem_fun(*this, &Editor::crop_region_to_selection)));
		edit_items.push_back (MenuElem (_("Duplicate Range"), sigc::bind (sigc::mem_fun(*this, &Editor::duplicate_range), false)));

		edit_items.push_back (SeparatorElem());
		edit_items.push_back (MenuElem (_("Consolidate"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), ReplaceRange, false)));
		edit_items.push_back (MenuElem (_("Consolidate (with processing)"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), ReplaceRange, true)));
		edit_items.push_back (MenuElem (_("Bounce"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), NewSource, false)));
		edit_items.push_back (MenuElem (_("Bounce (with processing)"), sigc::bind (sigc::mem_fun(*this, &Editor::bounce_range_selection), NewSource, true)));
	}

	edit_items.push_back (MenuElem (_("Export Range..."), sigc::mem_fun(*this, &Editor::export_selection)));
	if (ARDOUR_UI::instance()->video_timeline->get_duration() > 0) {
		edit_items.push_back (MenuElem (_("Export Video Range..."), sigc::bind (sigc::mem_fun(*(ARDOUR_UI::instance()), &ARDOUR_UI::export_video), true)));
	}
}


void
Editor::add_dstream_context_items (Menu_Helpers::MenuList& edit_items)
{
	using namespace Menu_Helpers;

	/* Playback */

	Menu *play_menu = manage (new Menu);
	MenuList& play_items = play_menu->items();
	play_menu->set_name ("ArdourContextMenu");

	play_items.push_back (MenuElem (_("Play from Edit Point"), sigc::mem_fun(*this, &Editor::play_from_edit_point)));
	play_items.push_back (MenuElem (_("Play from Start"), sigc::mem_fun(*this, &Editor::play_from_start)));
	play_items.push_back (MenuElem (_("Play Region"), sigc::mem_fun(*this, &Editor::play_selected_region)));
	play_items.push_back (SeparatorElem());
	play_items.push_back (MenuElem (_("Loop Region"), sigc::bind (sigc::mem_fun (*this, &Editor::set_loop_from_region), true)));

	edit_items.push_back (MenuElem (_("Play"), *play_menu));

	/* Selection */

	Menu *select_menu = manage (new Menu);
	MenuList& select_items = select_menu->items();
	select_menu->set_name ("ArdourContextMenu");

	select_items.push_back (MenuElem (_("Select All in Track"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_in_track), SelectionSet)));
	select_items.push_back (MenuElem (_("Select All Objects"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_objects), SelectionSet)));
	select_items.push_back (MenuElem (_("Invert Selection in Track"), sigc::mem_fun(*this, &Editor::invert_selection_in_track)));
	select_items.push_back (MenuElem (_("Invert Selection"), sigc::mem_fun(*this, &Editor::invert_selection)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Set Range to Loop Range"), sigc::mem_fun(*this, &Editor::set_selection_from_loop)));
	select_items.push_back (MenuElem (_("Set Range to Punch Range"), sigc::mem_fun(*this, &Editor::set_selection_from_punch)));
	select_items.push_back (MenuElem (_("Set Range to Selected Regions"), sigc::mem_fun(*this, &Editor::set_selection_from_region)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true, true)));
	select_items.push_back (MenuElem (_("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false, true)));
	select_items.push_back (MenuElem (_("Select All After Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), _playhead_cursor, true)));
	select_items.push_back (MenuElem (_("Select All Before Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), _playhead_cursor, false)));
	select_items.push_back (MenuElem (_("Select All Between Playhead and Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), false)));
	select_items.push_back (MenuElem (_("Select All Within Playhead and Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_between), true)));
	select_items.push_back (MenuElem (_("Select Range Between Playhead and Edit Point"), sigc::mem_fun(*this, &Editor::select_range_between)));

	edit_items.push_back (MenuElem (_("Select"), *select_menu));

	/* Cut-n-Paste */

	Menu *cutnpaste_menu = manage (new Menu);
	MenuList& cutnpaste_items = cutnpaste_menu->items();
	cutnpaste_menu->set_name ("ArdourContextMenu");

	cutnpaste_items.push_back (MenuElem (_("Cut"), sigc::mem_fun(*this, &Editor::cut)));
	cutnpaste_items.push_back (MenuElem (_("Copy"), sigc::mem_fun(*this, &Editor::copy)));
	cutnpaste_items.push_back (MenuElem (_("Paste"), sigc::bind (sigc::mem_fun(*this, &Editor::paste), 1.0f, true)));

	cutnpaste_items.push_back (SeparatorElem());

	cutnpaste_items.push_back (MenuElem (_("Align"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions), ARDOUR::SyncPoint)));
	cutnpaste_items.push_back (MenuElem (_("Align Relative"), sigc::bind (sigc::mem_fun (*this, &Editor::align_regions_relative), ARDOUR::SyncPoint)));

	edit_items.push_back (MenuElem (_("Edit"), *cutnpaste_menu));

	/* Adding new material */

	edit_items.push_back (SeparatorElem());
	edit_items.push_back (MenuElem (_("Insert Selected Region"), sigc::bind (sigc::mem_fun(*this, &Editor::insert_source_list_selection), 1.0f)));
	if (!current_playlist () || !_sources->get_single_selection ()) {
		edit_items.back ().set_sensitive (false);
	}
	edit_items.push_back (MenuElem (_("Insert Existing Media"), sigc::bind (sigc::mem_fun(*this, &Editor::add_external_audio_action), ImportToTrack)));

	/* Nudge track */

	Menu *nudge_menu = manage (new Menu());
	MenuList& nudge_items = nudge_menu->items();
	nudge_menu->set_name ("ArdourContextMenu");

	edit_items.push_back (SeparatorElem());
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Later"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, true))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Later"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, true))));
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Earlier"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, false))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Earlier"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, false))));

	edit_items.push_back (MenuElem (_("Nudge"), *nudge_menu));
}

void
Editor::add_bus_context_items (Menu_Helpers::MenuList& edit_items)
{
	using namespace Menu_Helpers;

	/* Playback */

	Menu *play_menu = manage (new Menu);
	MenuList& play_items = play_menu->items();
	play_menu->set_name ("ArdourContextMenu");

	play_items.push_back (MenuElem (_("Play from Edit Point"), sigc::mem_fun(*this, &Editor::play_from_edit_point)));
	play_items.push_back (MenuElem (_("Play from Start"), sigc::mem_fun(*this, &Editor::play_from_start)));
	edit_items.push_back (MenuElem (_("Play"), *play_menu));

	/* Selection */

	Menu *select_menu = manage (new Menu);
	MenuList& select_items = select_menu->items();
	select_menu->set_name ("ArdourContextMenu");

	select_items.push_back (MenuElem (_("Select All in Track"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_in_track), SelectionSet)));
	select_items.push_back (MenuElem (_("Select All Objects"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_objects), SelectionSet)));
	select_items.push_back (MenuElem (_("Invert Selection in Track"), sigc::mem_fun(*this, &Editor::invert_selection_in_track)));
	select_items.push_back (MenuElem (_("Invert Selection"), sigc::mem_fun(*this, &Editor::invert_selection)));
	select_items.push_back (SeparatorElem());
	select_items.push_back (MenuElem (_("Select All After Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), true, true)));
	select_items.push_back (MenuElem (_("Select All Before Edit Point"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_edit), false, true)));
	select_items.push_back (MenuElem (_("Select All After Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), _playhead_cursor, true)));
	select_items.push_back (MenuElem (_("Select All Before Playhead"), sigc::bind (sigc::mem_fun(*this, &Editor::select_all_selectables_using_cursor), _playhead_cursor, false)));

	edit_items.push_back (MenuElem (_("Select"), *select_menu));

	/* Cut-n-Paste */
#if 0 // unused, why?
	Menu *cutnpaste_menu = manage (new Menu);
	MenuList& cutnpaste_items = cutnpaste_menu->items();
	cutnpaste_menu->set_name ("ArdourContextMenu");

	cutnpaste_items.push_back (MenuElem (_("Cut"), sigc::mem_fun(*this, &Editor::cut)));
	cutnpaste_items.push_back (MenuElem (_("Copy"), sigc::mem_fun(*this, &Editor::copy)));
	cutnpaste_items.push_back (MenuElem (_("Paste"), sigc::bind (sigc::mem_fun(*this, &Editor::paste), 1.0f, true)));
#endif

	Menu *nudge_menu = manage (new Menu());
	MenuList& nudge_items = nudge_menu->items();
	nudge_menu->set_name ("ArdourContextMenu");

	edit_items.push_back (SeparatorElem());
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Later"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, true))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Later"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, true))));
	nudge_items.push_back (MenuElem (_("Nudge Entire Track Earlier"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), false, false))));
	nudge_items.push_back (MenuElem (_("Nudge Track After Edit Point Earlier"), (sigc::bind (sigc::mem_fun(*this, &Editor::nudge_track), true, false))));

	edit_items.push_back (MenuElem (_("Nudge"), *nudge_menu));
}

void
Editor::show_rulers_for_grid ()
{
	GridType gt (grid_type());

	/* show appropriate rulers for this grid setting. */
	if (grid_musical()) {
		ruler_tempo_action->set_active(true);
		ruler_meter_action->set_active(true);
		ruler_bbt_action->set_active(true);

		if (UIConfiguration::instance().get_rulers_follow_grid()) {
			ruler_timecode_action->set_active(false);
			ruler_minsec_action->set_active(false);
			ruler_samples_action->set_active(false);
		}
	} else if (gt == GridTypeTimecode) {
		ruler_timecode_action->set_active(true);

		if (UIConfiguration::instance().get_rulers_follow_grid()) {
			ruler_tempo_action->set_active(false);
			ruler_meter_action->set_active(false);
			ruler_bbt_action->set_active(false);
			ruler_minsec_action->set_active(false);
			ruler_samples_action->set_active(false);
		}
	} else if (gt == GridTypeMinSec) {
		ruler_minsec_action->set_active(true);

		if (UIConfiguration::instance().get_rulers_follow_grid()) {
			ruler_tempo_action->set_active(false);
			ruler_meter_action->set_active(false);
			ruler_bbt_action->set_active(false);
			ruler_timecode_action->set_active(false);
			ruler_samples_action->set_active(false);
		}
	} else if (gt == GridTypeCDFrame) {
		ruler_minsec_action->set_active(true);

		if (UIConfiguration::instance().get_rulers_follow_grid()) {
			ruler_tempo_action->set_active(false);
			ruler_meter_action->set_active(false);
			ruler_bbt_action->set_active(false);
			ruler_timecode_action->set_active(false);
			ruler_samples_action->set_active(false);
		}
	}
}

void
Editor::set_edit_point_preference (EditPoint ep, bool force)
{
	if (Profile->get_mixbus()) {
		if (ep == EditAtSelectedMarker) {
			ep = EditAtPlayhead;
		}
	}

	bool changed = (_edit_point != ep);

	_edit_point = ep;

	string str = edit_point_strings[(int)ep];
	if (str != edit_point_selector.get_text ()) {
		edit_point_selector.set_text (str);
	}

	if (!force && !changed) {
		return;
	}

	const char* action=NULL;

	switch (_edit_point) {
	case EditAtPlayhead:
		action = "edit-at-playhead";
		_snapped_cursor->hide ();
		break;
	case EditAtSelectedMarker:
		action = "edit-at-selected-marker";
		_snapped_cursor->hide ();
		break;
	case EditAtMouse:
		action = "edit-at-mouse";
		break;
	}

	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Editor", action);
	tact->set_active (true);

	samplepos_t foo;
	bool in_track_canvas;

	if (!mouse_sample (foo, in_track_canvas)) {
		in_track_canvas = false;
	}

	reset_canvas_action_sensitivity (in_track_canvas);
	sensitize_the_right_region_actions (false);

	instant_save ();
}

void
Editor::focus_on_clock()
{
	_application_bar.focus_on_clock();
}

int
Editor::set_state (const XMLNode& node, int version)
{
	set_id (node);
	PBD::Unwinder<bool> nsi (no_save_instant, true);
	bool yn;

	Tabbable::set_state (node, version);

	samplepos_t ph_pos;
	if (_session && node.get_property ("playhead", ph_pos)) {
		if (ph_pos >= 0) {
			_playhead_cursor->set_position (ph_pos);
		} else {
			warning << _("Playhead position stored with a negative value - ignored (use zero instead)") << endmsg;
			_playhead_cursor->set_position (0);
		}
	} else {
		_playhead_cursor->set_position (0);
	}

	update_selection_markers ();
	update_section_box ();

	node.get_property ("mixer-width", editor_mixer_strip_width);

	ZoomFocus zf;
	if (!node.get_property ("zoom-focus", zf)) {
		zf = ZoomFocusLeft;
	}
	set_zoom_preset (zf);

	node.get_property ("marker-click-behavior", marker_click_behavior);
	marker_click_behavior_selection_done (marker_click_behavior);

	int32_t cnt;
	if (node.get_property ("visible-track-count", cnt)) {
		set_visible_track_count (cnt);
	}

	set_common_editing_state (node);

	double y_origin;
	if (node.get_property ("y-origin", y_origin)) {
		reset_y_origin (y_origin);
	}

	yn = false;
	node.get_property ("join-object-range", yn);
	{
		/* do it twice to force the change */
		smart_mode_action->set_active (!yn);
		smart_mode_action->set_active (yn);
		set_mouse_mode (current_mouse_mode(), true);
	}

	EditPoint ep;
	if (node.get_property ("edit-point", ep)) {
		set_edit_point_preference (ep, true);
	} else {
		set_edit_point_preference (_edit_point);
	}

#ifndef LIVETRAX
	yn = false;
#else
	yn = true;
#endif
	node.get_property ("follow-playhead", yn);
	set_follow_playhead (yn);

	yn = false;
	node.get_property ("stationary-playhead", yn);
	set_stationary_playhead (yn);

	yn = true;
	node.get_property ("show-editor-mixer", yn);
	/* force a change to sync action state and actual attachment visibility.
	 * Otherwise after creating a new session from a running instance
	 * the editor-mixer and bottom attachment are not visible, even though
	 * the actions are enabled.
	 */
	show_editor_mixer_action->set_active (!yn);
	show_editor_mixer_action->set_active (yn);

	yn = false;
	node.get_property ("show-editor-list", yn);
	show_editor_list_action->set_active (!yn); // ditto
	show_editor_list_action->set_active (yn);

	yn = false;
	node.get_property ("show-editor-props", yn);
	show_editor_props_action->set_active (!yn); // ditto
	show_editor_props_action->set_active (yn);

	guint index;
	if (node.get_property (X_("editor-list-btn1"), index)) {
		_notebook_tab1.set_index (index);
	}

	if (node.get_property (X_("editor-list-btn2"), index)) {
		_notebook_tab2.set_index (index);
	}

	int32_t el_page;
	if (node.get_property (X_("editor-list-page"), el_page)) {
		_the_notebook.set_current_page (el_page);
	} else {
		el_page = _the_notebook.get_current_page ();
	}
	std::string label (_the_notebook.get_tab_label_text (*_the_notebook.get_nth_page (el_page)));
	_notebook_tab1.set_active (label);
	_notebook_tab2.set_active (label);

	yn = false;
	node.get_property (X_("show-marker-lines"), yn);
	{
		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("show-marker-lines"));
		/* do it twice to force the change */
		tact->set_active (!yn);
		tact->set_active (yn);
	}

	yn = false;
	node.get_property (X_("show-touched-automation"), yn);
	{
		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("show-touched-automation"));
		/* do it twice to force the change */
		tact->set_active (!yn);
		tact->set_active (yn);
	}

	XMLNodeList children = node.children ();
	for (XMLNodeList::const_iterator i = children.begin(); i != children.end(); ++i) {
		selection->set_state (**i, Stateful::current_state_version);
		_locations->set_state (**i);
	}

	if (node.get_property ("maximised", yn)) {
		Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Common"), X_("ToggleMaximalEditor"));
		bool fs = tact->get_active();
		if (yn ^ fs) {
			ActionManager::do_action ("Common", "ToggleMaximalEditor");
		}
	}

	timecnt_t nudge_clock_value;
	if (node.get_property ("nudge-clock-value", nudge_clock_value)) {
		nudge_clock->set_duration (nudge_clock_value);
	} else {
		nudge_clock->set_mode (AudioClock::Timecode);
		nudge_clock->set_duration (timecnt_t (_session->sample_rate() * 5), true);
	}

	return 0;
}

XMLNode&
Editor::get_state () const
{
	XMLNode* node = new XMLNode (X_("Editor"));

	node->set_property ("id", id().to_s ());

	node->add_child_nocopy (Tabbable::get_state());

	node->set_property("edit-vertical-pane-pos", editor_summary_pane.get_divider());

	maybe_add_mixer_strip_width (*node);

	node->set_property ("zoom-focus", zoom_focus());

	node->set_property ("edit-point", _edit_point);
	node->set_property ("visible-track-count", _visible_track_count);
	node->set_property ("marker-click-behavior", marker_click_behavior);

	get_common_editing_state (*node);

	node->set_property ("playhead", _playhead_cursor->current_sample ());
	node->set_property ("y-origin", vertical_adjustment.get_value ());

	node->set_property ("maximised", _maximised);
	node->set_property ("follow-playhead", follow_playhead());
	node->set_property ("stationary-playhead", _stationary_playhead);
	node->set_property ("mouse-mode", current_mouse_mode());
	node->set_property ("join-object-range", smart_mode_action->get_active ());

	node->set_property (X_("show-editor-mixer"), show_editor_mixer_action->get_active());
	node->set_property (X_("show-editor-list"), show_editor_list_action->get_active());
	node->set_property (X_("show-editor-props"), show_editor_props_action->get_active());

	node->set_property (X_("editor-list-page"), _the_notebook.get_current_page ());
	node->set_property (X_("editor-list-btn1"), _notebook_tab1.index ());
	node->set_property (X_("editor-list-btn2"), _notebook_tab2.index ());

	if (button_bindings) {
		XMLNode* bb = new XMLNode (X_("Buttons"));
		button_bindings->save (*bb);
		node->add_child_nocopy (*bb);
	}

	node->set_property (X_("show-marker-lines"), _show_marker_lines);
	node->set_property (X_("show-touched-automation"), show_touched_automation());

	node->add_child_nocopy (selection->get_state ());

	node->set_property ("nudge-clock-value", nudge_clock->current_duration());

	node->add_child_nocopy (_locations->get_state ());

	return *node;
}

/** Find a TimeAxisView by y position.
 *
 *  TimeAxisView may be 0.  Layer index is the layer number if the TimeAxisView
 *  is valid and is in stacked or expanded region display mode, otherwise 0.
 *
 *  If @p trackview_relative_offset is true, then @p y is an offset into the
 *  trackview area.  Otherwise, @p y is a global canvas coordinate.  In both
 *  cases, @p y is in pixels.
 *
 *  @return The TimeAxisView that @p y is over, and the layer index.
 */
std::pair<TimeAxisView *, double>
Editor::trackview_by_y_position (double y, bool trackview_relative_offset) const
{
	if (!trackview_relative_offset) {
		y -= _trackview_group->canvas_origin().y;
	}

	if (y < 0) {
		return std::make_pair ((TimeAxisView *) 0, 0);
	}

	for (TrackViewList::const_iterator iter = track_views.begin(); iter != track_views.end(); ++iter) {

		std::pair<TimeAxisView*, double> const r = (*iter)->covers_y_position (y);

		if (r.first) {
			return r;
		}
	}

	return std::make_pair ((TimeAxisView *) 0, 0);
}

void
Editor::set_snapped_cursor_position (timepos_t const & pos)
{
	if (_edit_point == EditAtMouse) {
		_snapped_cursor->set_position (pos.samples());
		if (UIConfiguration::instance().get_show_snapped_cursor()) {
			_snapped_cursor->show ();
		}
	}
}


timepos_t
Editor::snap_to_timecode (timepos_t const & presnap, Temporal::RoundMode direction, SnapPref gpref) const
{
	timepos_t start = presnap;
	samplepos_t start_sample = presnap.samples();
	const samplepos_t one_timecode_second = (samplepos_t)(rint(_session->timecode_frames_per_second()) * _session->samples_per_timecode_frame());
	samplepos_t one_timecode_minute = (samplepos_t)(rint(_session->timecode_frames_per_second()) * _session->samples_per_timecode_frame() * 60);

	TimecodeRulerScale scale = (gpref != SnapToGrid_Unscaled) ? timecode_ruler_scale : timecode_show_samples;

	switch (scale) {
	case timecode_show_bits:
	case timecode_show_samples:
		if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
		    fmod((double)start_sample, (double)_session->samples_per_timecode_frame()) == 0) {
			/* start is already on a whole timecode frame, do nothing */
		} else if (((direction == 0) && (fmod((double)start_sample, (double)_session->samples_per_timecode_frame()) > (_session->samples_per_timecode_frame() / 2))) || (direction > 0)) {
			start_sample = (samplepos_t) (ceil ((double) start_sample / _session->samples_per_timecode_frame()) * _session->samples_per_timecode_frame());
		} else {
			start_sample = (samplepos_t) (floor ((double) start_sample / _session->samples_per_timecode_frame()) *  _session->samples_per_timecode_frame());
		}
		start = timepos_t (start_sample);
		break;

	case timecode_show_seconds:
		if (_session->config.get_timecode_offset_negative()) {
			start_sample += _session->config.get_timecode_offset ();
		} else {
			start_sample -= _session->config.get_timecode_offset ();
		}
		if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
		    (start_sample % one_timecode_second == 0)) {
			/* start is already on a whole second, do nothing */
		} else if (((direction == 0) && (start_sample % one_timecode_second > one_timecode_second / 2)) || direction > 0) {
			start_sample = (samplepos_t) ceil ((double) start_sample / one_timecode_second) * one_timecode_second;
		} else {
			start_sample = (samplepos_t) floor ((double) start_sample / one_timecode_second) * one_timecode_second;
		}

		if (_session->config.get_timecode_offset_negative()) {
			start_sample -= _session->config.get_timecode_offset ();
		} else {
			start_sample += _session->config.get_timecode_offset ();
		}
		start = timepos_t (start_sample);
		break;

	case timecode_show_minutes:
	case timecode_show_hours:
	case timecode_show_many_hours:
		if (_session->config.get_timecode_offset_negative()) {
			start_sample += _session->config.get_timecode_offset ();
		} else {
			start_sample -= _session->config.get_timecode_offset ();
		}
		if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
		    (start_sample % one_timecode_minute == 0)) {
			/* start is already on a whole minute, do nothing */
		} else if (((direction == 0) && (start_sample % one_timecode_minute > one_timecode_minute / 2)) || direction > 0) {
			start_sample = (samplepos_t) ceil ((double) start_sample / one_timecode_minute) * one_timecode_minute;
		} else {
			start_sample = (samplepos_t) floor ((double) start_sample / one_timecode_minute) * one_timecode_minute;
		}
		if (_session->config.get_timecode_offset_negative()) {
			start_sample -= _session->config.get_timecode_offset ();
		} else {
			start_sample += _session->config.get_timecode_offset ();
		}
		start = timepos_t (start_sample);
		break;
	default:
		fatal << "Editor::smpte_snap_to_internal() called with non-timecode snap type!" << endmsg;
	}

	return start;
}

timepos_t
Editor::snap_to_minsec (timepos_t const & presnap, Temporal::RoundMode direction, SnapPref gpref) const
{
	samplepos_t presnap_sample = presnap.samples ();

	const samplepos_t one_second = _session->sample_rate();
	const samplepos_t one_minute = one_second * 60;
	const samplepos_t one_hour = one_minute * 60;

	MinsecRulerScale scale = (gpref != SnapToGrid_Unscaled) ? minsec_ruler_scale : minsec_show_seconds;

	switch (scale) {
		case minsec_show_msecs:
		case minsec_show_seconds: {
			if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
				presnap_sample % one_second == 0) {
				/* start is already on a whole second, do nothing */
			} else if (((direction == 0) && (presnap_sample % one_second > one_second / 2)) || (direction > 0)) {
				presnap_sample = (samplepos_t) ceil ((double) presnap_sample / one_second) * one_second;
			} else {
				presnap_sample = (samplepos_t) floor ((double) presnap_sample / one_second) * one_second;
			}
		} break;

		case minsec_show_minutes: {
			if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
				presnap_sample % one_minute == 0) {
				/* start is already on a whole minute, do nothing */
			} else if (((direction == 0) && (presnap_sample % one_minute > one_minute / 2)) || (direction > 0)) {
				presnap_sample = (samplepos_t) ceil ((double) presnap_sample / one_minute) * one_minute;
			} else {
				presnap_sample = (samplepos_t) floor ((double) presnap_sample / one_minute) * one_minute;
			}
		} break;

		default: {
			if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
				presnap_sample % one_hour == 0) {
				/* start is already on a whole hour, do nothing */
			} else if (((direction == 0) && (presnap_sample % one_hour > one_hour / 2)) || (direction > 0)) {
				presnap_sample = (samplepos_t) ceil ((double) presnap_sample / one_hour) * one_hour;
			} else {
				presnap_sample = (samplepos_t) floor ((double) presnap_sample / one_hour) * one_hour;
			}
		} break;
	}

	return timepos_t (presnap_sample);
}

timepos_t
Editor::snap_to_cd_frames (timepos_t const & presnap, Temporal::RoundMode direction, SnapPref gpref) const
{
	if ((gpref != SnapToGrid_Unscaled) && (minsec_ruler_scale != minsec_show_msecs)) {
		return snap_to_minsec (presnap, direction, gpref);
	}

	const samplepos_t one_second = _session->sample_rate();

	samplepos_t presnap_sample = presnap.samples();

	if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundDownMaybe) &&
		presnap_sample % (one_second/75) == 0) {
		/* start is already on a whole CD sample, do nothing */
	} else if (((direction == 0) && (presnap_sample % (one_second/75) > (one_second/75) / 2)) || (direction > 0)) {
		presnap_sample = (samplepos_t) ceil ((double) presnap_sample / (one_second / 75)) * (one_second / 75);
	} else {
		presnap_sample = (samplepos_t) floor ((double) presnap_sample / (one_second / 75)) * (one_second / 75);
	}

	return timepos_t (presnap_sample);
}

timepos_t
Editor::snap_to_grid (timepos_t const & presnap, Temporal::RoundMode direction, SnapPref gpref) const
{
	timepos_t ret(presnap);

	if (grid_musical()) {
		ret = snap_to_bbt (presnap, direction, gpref);
	}

	switch (grid_type()) {
	case GridTypeTimecode:
		ret = snap_to_timecode(presnap, direction, gpref);
		break;
	case GridTypeMinSec:
		ret = snap_to_minsec(presnap, direction, gpref);
		break;
	case GridTypeCDFrame:
		ret = snap_to_cd_frames(presnap, direction, gpref);
		break;
	default:
		break;
	};

	return ret;
}

timepos_t
Editor::snap_to_marker (timepos_t const & presnap, Temporal::RoundMode direction) const
{
	timepos_t before;
	timepos_t after;
	timepos_t test;

	if (_session->locations()->list().empty()) {
		/* No marks to snap to, so just don't snap */
		return timepos_t();
	}

	_session->locations()->marks_either_side (presnap, before, after);

	if (before == timepos_t::max (before.time_domain())) {
		test = after;
	} else if (after == timepos_t::max (after.time_domain())) {
		test = before;
	} else  {
		if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundUpAlways)) {
			test = after;
		} else if ((direction == Temporal::RoundDownMaybe || direction == Temporal::RoundDownAlways)) {
			test = before;
		} else if (direction ==  0) {
			if (before.distance (presnap) < presnap.distance (after)) {
				test = before;
			} else {
				test = after;
			}
		}
	}

	return test;
}


void
Editor::setup_toolbar ()
{
	const int spc = Profile->get_mixbus() ? 0 : 2;

	HBox* mode_box = manage(new HBox);
	mode_box->set_border_width (spc);
	mode_box->set_spacing(2);

	HBox* mouse_mode_box = manage (new HBox);
	HBox* mouse_mode_hbox = manage (new HBox);
	VBox* mouse_mode_vbox = manage (new VBox);
	Alignment* mouse_mode_align = manage (new Alignment);

	Glib::RefPtr<SizeGroup> mouse_mode_size_group = SizeGroup::create (SIZE_GROUP_VERTICAL);
	mouse_mode_size_group->add_widget (smart_mode_button);
	mouse_mode_size_group->add_widget (mouse_move_button);
	mouse_mode_size_group->add_widget (mouse_cut_button);
	mouse_mode_size_group->add_widget (mouse_select_button);
	mouse_mode_size_group->add_widget (mouse_timefx_button);
	mouse_mode_size_group->add_widget (mouse_grid_button);
	mouse_mode_size_group->add_widget (mouse_draw_button);
	mouse_mode_size_group->add_widget (mouse_content_button);

	if (!Profile->get_mixbus()) {
		mouse_mode_size_group->add_widget (zoom_in_button);
		mouse_mode_size_group->add_widget (zoom_out_button);
		mouse_mode_size_group->add_widget (full_zoom_button);
		mouse_mode_size_group->add_widget (zoom_focus_selector);
		mouse_mode_size_group->add_widget (tav_shrink_button);
		mouse_mode_size_group->add_widget (tav_expand_button);
		mouse_mode_size_group->add_widget (follow_playhead_button);
		mouse_mode_size_group->add_widget (follow_edits_button);
	} else {
		mouse_mode_size_group->add_widget (zoom_preset_selector);
		mouse_mode_size_group->add_widget (visible_tracks_selector);
	}

	mouse_mode_size_group->add_widget (stretch_marker_cb);

	mouse_mode_size_group->add_widget (grid_type_selector);
	mouse_mode_size_group->add_widget (draw_length_selector);
	mouse_mode_size_group->add_widget (draw_velocity_selector);
	mouse_mode_size_group->add_widget (draw_channel_selector);
	mouse_mode_size_group->add_widget (snap_mode_button);

	mouse_mode_size_group->add_widget (edit_point_selector);
	mouse_mode_size_group->add_widget (edit_mode_selector);
	mouse_mode_size_group->add_widget (ripple_mode_selector);

	mouse_mode_size_group->add_widget (*nudge_clock);
	mouse_mode_size_group->add_widget (nudge_forward_button);
	mouse_mode_size_group->add_widget (nudge_backward_button);

	mouse_mode_hbox->set_spacing (spc);
	mouse_mode_hbox->pack_start (smart_mode_button, false, false);

	mouse_mode_hbox->pack_start (mouse_move_button, false, false);
	mouse_mode_hbox->pack_start (mouse_select_button, false, false);

	mouse_mode_hbox->pack_start (mouse_cut_button, false, false);

	mouse_mode_hbox->pack_start (mouse_timefx_button, false, false);
	mouse_mode_hbox->pack_start (mouse_grid_button, false, false);
	mouse_mode_hbox->pack_start (mouse_draw_button, false, false);
	mouse_mode_hbox->pack_start (mouse_content_button, false, false);

	mouse_mode_vbox->pack_start (*mouse_mode_hbox);

	mouse_mode_align->add (*mouse_mode_vbox);
	mouse_mode_align->set (0.5, 1.0, 0.0, 0.0);

	mouse_mode_box->pack_start (*mouse_mode_align, false, false);

	ripple_mode_selector.set_name ("mouse mode button");
	edit_mode_selector.set_name ("mouse mode button");

	mode_box->pack_start (edit_mode_selector, false, false);
	mode_box->pack_start (ripple_mode_selector, false, false);
	mode_box->pack_start (*(manage (new ArdourVSpacer ())), false, false, 3);
	mode_box->pack_start (edit_point_selector, false, false);
	mode_box->pack_start (*(manage (new ArdourVSpacer ())), false, false, 3);

	mode_box->pack_start (*mouse_mode_box, false, false);

	/* Zoom */

	_zoom_box.set_spacing (2);
	_zoom_box.set_border_width (spc);

	RefPtr<Action> act;

	zoom_preset_selector.set_name ("zoom button");
	zoom_preset_selector.set_icon (ArdourIcon::ZoomExpand);

	act = ActionManager::get_action (X_("Editor"), X_("zoom-to-session"));
	full_zoom_button.set_related_action (act);

	if (ARDOUR::Profile->get_mixbus()) {
		_zoom_box.pack_start (zoom_preset_selector, false, false);
	} else {
		_zoom_box.pack_start (zoom_out_button, false, false);
		_zoom_box.pack_start (zoom_in_button, false, false);
		_zoom_box.pack_start (full_zoom_button, false, false);
		_zoom_box.pack_start (zoom_focus_selector, false, false);
	}

	/* Track zoom buttons */
	_track_box.set_spacing (2);
	_track_box.set_border_width (spc);

	visible_tracks_selector.set_name ("zoom button");
	set_size_request_to_display_given_text (visible_tracks_selector, _("All"), 30, 2);

	tav_expand_button.set_name ("zoom button");
	tav_expand_button.set_icon (ArdourIcon::TimeAxisExpand);
	act = ActionManager::get_action (X_("Editor"), X_("expand-tracks"));
	tav_expand_button.set_related_action (act);

	tav_shrink_button.set_name ("zoom button");
	tav_shrink_button.set_icon (ArdourIcon::TimeAxisShrink);
	act = ActionManager::get_action (X_("Editor"), X_("shrink-tracks"));
	tav_shrink_button.set_related_action (act);

	if (!ARDOUR::Profile->get_mixbus()) {
		_track_box.pack_start (visible_tracks_selector);
		_track_box.pack_start (tav_shrink_button);
		_track_box.pack_start (tav_expand_button);
	}

	snap_box.set_spacing (2);
	snap_box.set_border_width (spc);

	stretch_marker_cb.set_name ("mouse mode button");

	snap_mode_button.set_name ("mouse mode button");

	edit_point_selector.set_name ("mouse mode button");

	pack_snap_box ();

	/* Nudge */

	HBox *nudge_box = manage (new HBox);
	nudge_box->set_spacing (2);
	nudge_box->set_border_width (spc);

	nudge_forward_button.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::nudge_forward_release), false);
	nudge_backward_button.signal_button_release_event().connect (sigc::mem_fun(*this, &Editor::nudge_backward_release), false);

	nudge_box->pack_start (nudge_backward_button, false, false);
	nudge_box->pack_start (*nudge_clock, false, false);
	nudge_box->pack_start (nudge_forward_button, false, false);

	stretch_marker_cb.set_label (_("Adjust Markers"));
	stretch_marker_cb.set_active (true);

	grid_box.set_spacing (2);
	grid_box.set_border_width (spc);
	grid_box.pack_start (stretch_marker_cb, false, false, 4);

	grid_type_selector.set_name ("mouse mode button");

	pack_draw_box (true);

	HBox* follow_mode_hbox = manage (new HBox);
	follow_mode_hbox->set_spacing (spc ? 2 : 1);
	follow_mode_hbox->set_border_width (spc);
	follow_mode_hbox->pack_start (follow_playhead_button, false, false);
	follow_mode_hbox->pack_start (follow_edits_button, false, false);

	/* Pack everything in... */

	toolbar_hbox.set_spacing (2);
	toolbar_hbox.set_border_width (spc ? 1 : 0);

#ifndef MIXBUS
	ArdourWidgets::ArdourDropShadow *tool_shadow = manage (new (ArdourWidgets::ArdourDropShadow));
	tool_shadow->set_size_request (4, -1);
	tool_shadow->show();

	ebox_hpacker.pack_start (*tool_shadow, false, false);
#endif
	ebox_hpacker.pack_start(ebox_vpacker, true, true);

	Gtk::EventBox* spacer = manage (new Gtk::EventBox); // extra space under the mouse toolbar, for aesthetics
	spacer->set_name("EditorWindow");
	spacer->set_size_request(-1,4);
	spacer->show();

	ebox_vpacker.pack_start(toolbar_hbox, false, false);
	ebox_vpacker.pack_start(*spacer, false, false);
	ebox_vpacker.show();

	toolbar_hbox.pack_start (*mode_box, false, false);
	toolbar_hbox.pack_start (*(manage (new ArdourVSpacer ())), false, false, 3);
	toolbar_hbox.pack_start (snap_box, false, false);
	toolbar_hbox.pack_start (*(manage (new ArdourVSpacer ())), false, false, 3);
	toolbar_hbox.pack_start (*nudge_box, false, false);
	toolbar_hbox.pack_start (_grid_box_spacer, false, false, 3);
	toolbar_hbox.pack_start (grid_box, false, false);
	toolbar_hbox.pack_start (_draw_box_spacer, false, false, 3);
	toolbar_hbox.pack_start (draw_box, false, false);
	toolbar_hbox.pack_end (_zoom_box, false, false, 2);
	toolbar_hbox.pack_end (*(manage (new ArdourVSpacer ())), false, false, 3);
	toolbar_hbox.pack_end (_track_box, false, false);
	toolbar_hbox.pack_end (*(manage (new ArdourVSpacer ())), false, false, 3);
	toolbar_hbox.pack_end (*follow_mode_hbox, false, false);

	toolbar_hbox.show_all ();
}


void
Editor::build_edit_point_menu ()
{
	using namespace Menu_Helpers;

	edit_point_selector.add_menu_elem (MenuElem (edit_point_strings[(int)EditAtPlayhead], sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_selection_done), (EditPoint) EditAtPlayhead)));
	if(!Profile->get_mixbus())
		edit_point_selector.add_menu_elem (MenuElem (edit_point_strings[(int)EditAtSelectedMarker], sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_selection_done), (EditPoint) EditAtSelectedMarker)));
	edit_point_selector.add_menu_elem (MenuElem (edit_point_strings[(int)EditAtMouse], sigc::bind (sigc::mem_fun(*this, &Editor::edit_point_selection_done), (EditPoint) EditAtMouse)));

	edit_point_selector.set_sizing_texts (edit_point_strings);
}

void
Editor::build_edit_mode_menu ()
{
	using namespace Menu_Helpers;

	edit_mode_selector.add_menu_elem (MenuElem (edit_mode_strings[(int)Slide], sigc::bind (sigc::mem_fun(*this, &Editor::edit_mode_selection_done), (EditMode) Slide)));
	edit_mode_selector.add_menu_elem (MenuElem (edit_mode_strings[(int)Ripple], sigc::bind (sigc::mem_fun(*this, &Editor::edit_mode_selection_done), (EditMode) Ripple)));
	edit_mode_selector.add_menu_elem (MenuElem (edit_mode_strings[(int)Lock], sigc::bind (sigc::mem_fun(*this, &Editor::edit_mode_selection_done), (EditMode)  Lock)));
	/* Note: Splice was removed */
	edit_mode_selector.set_sizing_texts (edit_mode_strings);

	ripple_mode_selector.add_menu_elem (MenuElem (ripple_mode_strings[(int)RippleSelected],  sigc::bind (sigc::mem_fun(*this, &Editor::ripple_mode_selection_done), (RippleMode) RippleSelected)));
	ripple_mode_selector.add_menu_elem (MenuElem (ripple_mode_strings[(int)RippleAll],       sigc::bind (sigc::mem_fun(*this, &Editor::ripple_mode_selection_done), (RippleMode) RippleAll)));
	ripple_mode_selector.add_menu_elem (MenuElem (ripple_mode_strings[(int)RippleInterview], sigc::bind (sigc::mem_fun(*this, &Editor::ripple_mode_selection_done), (RippleMode) RippleInterview)));
	ripple_mode_selector.set_sizing_texts (ripple_mode_strings);
}

void
Editor::setup_tooltips ()
{
	set_tooltip (smart_mode_button, _("Smart Mode (add range functions to Grab Mode)"));
	set_tooltip (*_group_tabs, _("Groups: click to (de)activate\nContext-click for other operations"));
	set_tooltip (nudge_forward_button, _("Nudge Region/Selection Later"));
	set_tooltip (nudge_backward_button, _("Nudge Region/Selection Earlier"));
	set_tooltip (zoom_in_button, _("Zoom In"));
	set_tooltip (zoom_out_button, _("Zoom Out"));
	set_tooltip (zoom_preset_selector, _("Zoom to Time Scale"));
	set_tooltip (full_zoom_button, _("Zoom to Session"));
	set_tooltip (tav_expand_button, _("Expand Tracks"));
	set_tooltip (tav_shrink_button, _("Shrink Tracks"));
	set_tooltip (visible_tracks_selector, _("Number of visible tracks"));
	set_tooltip (stretch_marker_cb, _("Move markers and ranges when stretching the Grid\n(this option is only available when session Time Domain is Beat Time)"));
	set_tooltip (edit_point_selector, _("Edit Point"));
	set_tooltip (edit_mode_selector, _("Edit Mode"));
	set_tooltip (nudge_clock, _("Nudge Clock\n(controls distance used to nudge regions and selections)"));
}

void
Editor::new_tempo_section ()
{
}

void
Editor::map_transport_state ()
{
	ENSURE_GUI_THREAD (*this, &Editor::map_transport_state);

	if (_session && _session->transport_stopped()) {
		have_pending_keyboard_selection = false;
	}

	update_loop_range_view ();
}

void
Editor::transport_looped ()
{
	/* reset Playhead position interpolation.
	 * see Editor::super_rapid_screen_update
	 */
	_last_update_time = 0;
}

/* UNDO/REDO */

void
Editor::begin_selection_op_history ()
{
	selection_op_cmd_depth = 0;
	selection_op_history_it = 0;

	while(!selection_op_history.empty()) {
		delete selection_op_history.front();
		selection_op_history.pop_front();
	}

	selection_undo_action->set_sensitive (false);
	selection_redo_action->set_sensitive (false);
	selection_op_history.push_front (&_selection_memento->get_state ());
}

void
Editor::begin_reversible_selection_op (string name){

	if (_session) {
		//cerr << name << endl;
		/* begin/commit pairs can be nested */
		selection_op_cmd_depth++;
	}
}

#include "pbd/stacktrace.h"

void
Editor::abort_reversible_selection_op ()
{
	PBD::stacktrace (std::cerr, 20);
	if (!_session) {
		return;
	}
	if (selection_op_cmd_depth > 0) {
		selection_op_cmd_depth--;
	}
}

void
Editor::commit_reversible_selection_op ()
{
	if (_session) {
		if (selection_op_cmd_depth == 1) {

			if (selection_op_history_it > 0 && selection_op_history_it < selection_op_history.size()) {
				/* The user has undone some selection ops and then made a new one,
				 * making anything earlier in the list invalid.
				 */

				list<XMLNode *>::iterator it = selection_op_history.begin();
				list<XMLNode *>::iterator e_it = it;
				advance (e_it, selection_op_history_it);

				for (; it != e_it; ++it) {
					delete *it;
				}
				selection_op_history.erase (selection_op_history.begin(), e_it);
			}

			selection_op_history.push_front (&_selection_memento->get_state ());
			selection_op_history_it = 0;

			selection_undo_action->set_sensitive (true);
			selection_redo_action->set_sensitive (false);
		}

		if (selection_op_cmd_depth > 0) {
			selection_op_cmd_depth--;
		}
	}
}

void
Editor::undo_selection_op ()
{
	if (_session) {
		selection_op_history_it++;
		uint32_t n = 0;
		for (std::list<XMLNode *>::iterator i = selection_op_history.begin(); i != selection_op_history.end(); ++i) {
			if (n == selection_op_history_it) {
				_selection_memento->set_state (*(*i), Stateful::current_state_version);
				selection_redo_action->set_sensitive (true);
			}
			++n;
		}
		/* is there an earlier entry? */
		if ((selection_op_history_it + 1) >= selection_op_history.size()) {
			selection_undo_action->set_sensitive (false);
		}
	}
}

void
Editor::redo_selection_op ()
{
	if (_session) {
		if (selection_op_history_it > 0) {
			selection_op_history_it--;
		}
		uint32_t n = 0;
		for (std::list<XMLNode *>::iterator i = selection_op_history.begin(); i != selection_op_history.end(); ++i) {
			if (n == selection_op_history_it) {
				_selection_memento->set_state (*(*i), Stateful::current_state_version);
				selection_undo_action->set_sensitive (true);
			}
			++n;
		}

		if (selection_op_history_it == 0) {
			selection_redo_action->set_sensitive (false);
		}
	}
}

void
Editor::history_changed ()
{
	if (!_session) {
		return;
	}

	update_undo_redo_actions (_session->undo_redo());
}

void
Editor::duplicate_range (bool with_dialog)
{
	float times = 1.0f;

	RegionSelection rs = get_regions_from_selection_and_entered ();

	if (selection->time.length() == 0 && rs.empty()) {
		return;
	}

	if (with_dialog) {

		ArdourDialog win (_("Duplicate"));
		Label label (_("Number of duplications:"));
		Adjustment adjustment (1.0, 1.0, 1000000.0, 1.0, 5.0);
		SpinButton spinner (adjustment, 0.0, 1);
		HBox hbox;

		win.get_vbox()->set_spacing (12);
		win.get_vbox()->pack_start (hbox);
		hbox.set_border_width (6);
		hbox.pack_start (label, PACK_EXPAND_PADDING, 12);

		/* dialogs have ::add_action_widget() but that puts the spinner in the wrong
		   place, visually. so do this by hand.
		*/

		hbox.pack_start (spinner, PACK_EXPAND_PADDING, 12);
		spinner.signal_activate().connect (sigc::bind (sigc::mem_fun (win, &ArdourDialog::response), RESPONSE_ACCEPT));
		spinner.grab_focus();

		hbox.show ();
		label.show ();
		spinner.show ();

		win.add_button (Stock::CANCEL, RESPONSE_CANCEL);
		win.add_button (_("Duplicate"), RESPONSE_ACCEPT);
		win.set_default_response (RESPONSE_ACCEPT);

		spinner.grab_focus ();

		switch (win.run ()) {
		case RESPONSE_ACCEPT:
			break;
		default:
			return;
		}

		times = adjustment.get_value();
	}

	if ((current_mouse_mode() == MouseRange)) {
		if (!selection->time.length().is_zero()) {
			duplicate_selection (times);
		}
	} else if (get_smart_mode()) {
		if (!selection->time.length().is_zero()) {
			duplicate_selection (times);
		} else
			duplicate_some_regions (rs, times);
	} else {
		duplicate_some_regions (rs, times);
	}
}

void
Editor::set_ripple_mode (RippleMode m) /* redundant with selection_done ? */
{
	Config->set_ripple_mode (m);
}

void
Editor::set_edit_mode (EditMode m) /* redundant with selection_done ? */
{
	Config->set_edit_mode (m);
}

void
Editor::cycle_edit_mode ()
{
	switch (Config->get_edit_mode()) {
	case Slide:
		Config->set_edit_mode (Ripple);
		break;
	case Ripple:
		Config->set_edit_mode (Lock);
		break;
	case Lock:
		Config->set_edit_mode (Slide);
		break;
	}
}

void
Editor::edit_mode_selection_done (EditMode m)
{
	Config->set_edit_mode (m);
}

void
Editor::ripple_mode_selection_done (RippleMode m)
{
	Config->set_ripple_mode (m);
}

void
Editor::cycle_edit_point (bool with_marker)
{
	if(Profile->get_mixbus())
		with_marker = false;

	switch (_edit_point) {
	case EditAtMouse:
		set_edit_point_preference (EditAtPlayhead);
		break;
	case EditAtPlayhead:
		if (with_marker) {
			set_edit_point_preference (EditAtSelectedMarker);
		} else {
			set_edit_point_preference (EditAtMouse);
		}
		break;
	case EditAtSelectedMarker:
		set_edit_point_preference (EditAtMouse);
		break;
	}
}

void
Editor::edit_point_selection_done (EditPoint ep)
{
	set_edit_point_preference (ep);
}

void
Editor::build_zoom_focus_menu ()
{
	using namespace Menu_Helpers;

	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusLeft]);
	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusRight]);
	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusCenter]);
	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusPlayhead]);
	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusMouse]);
	zoom_focus_selector.append (zoom_focus_actions[ZoomFocusEdit]);
	zoom_focus_selector.set_sizing_texts (zoom_focus_strings);
}

void
Editor::marker_click_behavior_selection_done (MarkerClickBehavior m)
{
	RefPtr<RadioAction> ract = marker_click_behavior_action (m);
	if (ract) {
		ract->set_active ();
	}
}

void
Editor::build_track_count_menu ()
{
	using namespace Menu_Helpers;

	if (!Profile->get_mixbus()) {
		visible_tracks_selector.add_menu_elem (MenuElem (X_("1"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 1)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("2"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 2)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("3"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 3)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("4"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 4)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("8"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 8)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("12"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 12)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("16"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 16)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("20"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 20)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("24"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 24)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("32"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 32)));
		visible_tracks_selector.add_menu_elem (MenuElem (X_("64"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 64)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Selection"), sigc::mem_fun(*this, &Editor::fit_selection)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("All"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 0)));
	} else {
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 1 track"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 1)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 2 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 2)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 4 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 4)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 8 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 8)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 16 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 16)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 24 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 24)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 32 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 32)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit 48 tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 48)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit All tracks"), sigc::bind (sigc::mem_fun(*this, &Editor::set_visible_track_count), 0)));
		visible_tracks_selector.add_menu_elem (MenuElem (_("Fit Selection"), sigc::mem_fun(*this, &Editor::fit_selection)));

		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 10 ms"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 100 ms"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 100)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 1 sec"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 1 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 10 sec"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 1 min"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 60 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 10 min"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 10 * 60 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 1 hour"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 60 * 60 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 8 hours"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 8 * 60 * 60 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to 24 hours"), sigc::bind (sigc::mem_fun(*this, &Editor::set_zoom_preset), 24 * 60 * 60 * 1000)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to Session"), sigc::mem_fun(*this, &Editor::temporal_zoom_session)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to Extents"), sigc::mem_fun(*this, &Editor::temporal_zoom_extents)));
		zoom_preset_selector.add_menu_elem (MenuElem (_("Zoom to Range/Region Selection"), sigc::bind (sigc::mem_fun(*this, &Editor::temporal_zoom_selection), Horizontal)));
	}
}

void
Editor::set_zoom_preset (int64_t ms)
{
	if (ms <= 0) {
		temporal_zoom_session();
		return;
	}

	ARDOUR::samplecnt_t const sample_rate = TEMPORAL_SAMPLE_RATE;
	temporal_zoom ((sample_rate * ms / 1000) / _visible_canvas_width);
}

void
Editor::set_visible_track_count (int32_t n)
{
	_visible_track_count = n;

	/* if the canvas hasn't really been allocated any size yet, just
	   record the desired number of visible tracks and return. when canvas
	   allocation happens, we will get called again and then we can do the
	   real work.
	*/

	if (_visible_canvas_height <= 1) {
		return;
	}

	int h;
	string str;
	DisplaySuspender ds;

	if (_visible_track_count > 0) {
		h = trackviews_height() / _visible_track_count;
		std::ostringstream s;
		s << _visible_track_count;
		str = s.str();
	} else if (_visible_track_count == 0) {
		uint32_t n = 0;
		for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
			if ((*i)->marked_for_display()) {
				++n;
				TimeAxisView::Children cl ((*i)->get_child_list ());
				for (TimeAxisView::Children::const_iterator j = cl.begin(); j != cl.end(); ++j) {
					if ((*j)->marked_for_display()) {
						++n;
					}
				}
			}
		}
		if (n == 0) {
			visible_tracks_selector.set_text (X_("*"));
			return;
		}
		h = trackviews_height() / n;
		str = _("All");
	} else {
		/* negative value means that the visible track count has
		   been overridden by explicit track height changes.
		*/
		visible_tracks_selector.set_text (X_("*"));
		return;
	}

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(*i)->set_height (h, TimeAxisView::HeightPerLane);
	}

	if (str != visible_tracks_selector.get_text()) {
		visible_tracks_selector.set_text (str);
	}
}

void
Editor::override_visible_track_count ()
{
	_visible_track_count = -1;
	visible_tracks_selector.set_text (_("*"));
}

bool
Editor::edit_controls_button_event (GdkEventButton* ev)
{
	if (ev->type == GDK_BUTTON_RELEASE && track_dragging()) {
		end_track_drag ();
		return true;
	}

	if ((ev->type == GDK_2BUTTON_PRESS && ev->button == 1) || (ev->type == GDK_BUTTON_RELEASE && Keyboard::is_context_menu_event (ev))) {
		ARDOUR_UI::instance()->add_route ();
	} else if (ev->button == 1 && ev->type == GDK_BUTTON_PRESS) {
		selection->clear_tracks ();
	}
	return true;
}

bool
Editor::mouse_select_button_release (GdkEventButton* ev)
{
	/* this handles just right-clicks */

	if (ev->button != 3) {
		return false;
	}

	return true;
}

void
Editor::set_zoom_focus (ZoomFocus f)
{
	zoom_focus_actions[f]->set_active (true);
}

void
Editor::set_marker_click_behavior (MarkerClickBehavior m)
{
	if (marker_click_behavior != m) {
		marker_click_behavior = m;
		marker_click_behavior_selection_done (marker_click_behavior);
		instant_save ();
	}
}

void
Editor::cycle_marker_click_behavior ()
{
	switch (marker_click_behavior) {
	case MarkerClickSelectOnly:
		set_marker_click_behavior (MarkerClickLocate);
		break;
	case MarkerClickLocate:
		set_marker_click_behavior (MarkerClickLocateWhenStopped);
		break;
	case MarkerClickLocateWhenStopped:
		set_marker_click_behavior (MarkerClickSelectOnly);
		break;
	}
}

void
Editor::toggle_stationary_playhead ()
{
	RefPtr<ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("toggle-stationary-playhead"));
	set_stationary_playhead (tact->get_active());
}

void
Editor::set_stationary_playhead (bool yn)
{
	if (_stationary_playhead != yn) {
		if ((_stationary_playhead = yn) == true) {
			/* catch up -- FIXME need a 3.0 equivalent of this 2.X call */
			// update_current_screen ();
		}
		instant_save ();
	}
}

bool
Editor::show_touched_automation() const
{
	if (!contents().get_mapped()) {
		return false;
	}

	if (!show_touched_automation_action) {
		return false;
	}

	return show_touched_automation_action->get_active ();
}

void
Editor::toggle_show_touched_automation ()
{
	set_show_touched_automation (show_touched_automation_action->get_active());
}

void
Editor::set_show_touched_automation (bool yn)
{
	show_touched_automation_action->set_active (yn);
	if (!yn) {
		RouteTimeAxisView::signal_ctrl_touched (true);
	}
	instant_save ();
}

Temporal::timecnt_t
Editor::get_paste_offset (Temporal::timepos_t const & pos, unsigned paste_count, Temporal::timecnt_t const & duration)
{
	if (paste_count == 0) {
		/* don't bother calculating an offset that will be zero anyway */
		return timecnt_t (0, timepos_t());
	}

	/* calculate basic unsnapped multi-paste offset */
	Temporal::timecnt_t offset = duration.scale (paste_count);

	/* snap offset so pos + offset is aligned to the grid */
	Temporal::timepos_t snap_pos (pos + offset);
	snap_to (snap_pos, Temporal::RoundUpMaybe);

	return pos.distance (snap_pos);
}

timecnt_t
Editor::get_nudge_distance (timepos_t const & pos, timecnt_t& next) const
{
	timecnt_t ret;

	ret = nudge_clock->current_duration (pos);
	next = ret + timepos_t::smallest_step (pos.time_domain()); /* FIX ME ... not sure this is how to compute "next" */

	return ret;
}

int
Editor::playlist_deletion_dialog (std::shared_ptr<Playlist> pl)
{
	ArdourDialog dialog (_("Playlist Deletion"));
	Label  label (string_compose (_("Playlist %1 is currently unused.\n"
					"If it is kept, its audio files will not be cleaned.\n"
					"If it is deleted, audio files used by it alone will be cleaned."),
				      pl->name()));

	dialog.set_position (WIN_POS_CENTER);
	dialog.get_vbox()->pack_start (label);

	label.show ();

	dialog.add_button (_("Delete All Unused"), RESPONSE_YES); // needs clarification. this and all remaining ones
	dialog.add_button (_("Delete Playlist"), RESPONSE_ACCEPT);
	Button* keep = dialog.add_button (_("Keep Playlist"), RESPONSE_REJECT);
	dialog.add_button (_("Keep Remaining"), RESPONSE_NO); // ditto
	dialog.add_button (_("Cancel"), RESPONSE_CANCEL);

	/* by default gtk uses the left most button */
	keep->grab_focus ();

	switch (dialog.run ()) {
	case RESPONSE_NO:
		/* keep this and all remaining ones */
		return -2;
		break;

	case RESPONSE_YES:
		/* delete this and all others */
		return 2;
		break;

	case RESPONSE_ACCEPT:
		/* delete the playlist */
		return 1;
		break;

	case RESPONSE_REJECT:
		/* keep the playlist */
		return 0;
		break;

	default:
		break;
	}

	return -1;
}

int
Editor::plugin_setup (std::shared_ptr<Route> r, std::shared_ptr<PluginInsert> pi, ARDOUR::Route::PluginSetupOptions flags)
{
	PluginSetupDialog psd (r, pi, flags);
	int rv = psd.run ();
	return rv + (psd.fan_out() ? 4 : 0);
}

bool
Editor::audio_region_selection_covers (samplepos_t where)
{
	for (RegionSelection::iterator a = selection->regions.begin(); a != selection->regions.end(); ++a) {
		if ((*a)->region()->covers (where)) {
			return true;
		}
	}

	return false;
}

void
Editor::cleanup_regions ()
{
	_regions->remove_unused_regions();
}


void
Editor::prepare_for_cleanup ()
{
	cut_buffer->clear_regions ();
	cut_buffer->clear_playlists ();

	selection->clear_regions ();
	selection->clear_playlists ();

	_regions->suspend_redisplay ();
}

void
Editor::finish_cleanup ()
{
	_regions->resume_redisplay ();
}

Location*
Editor::transport_punch_location()
{
	if (_session) {
		return _session->locations()->auto_punch_location();
	} else {
		return 0;
	}
}

bool
Editor::control_layout_scroll (GdkEventScroll* ev)
{
	/* Just forward to the normal canvas scroll method. The coordinate
	   systems are different but since the canvas is always larger than the
	   track headers, and aligned with the trackview area, this will work.

	   In the not too distant future this layout is going away anyway and
	   headers will be on the canvas.
	*/
	return canvas_scroll_event (ev, false);
}

void
Editor::session_state_saved (string)
{
	update_title ();
	_snapshots->redisplay ();
}

void
Editor::maximise_editing_space ()
{
	if (_maximised) {
		return;
	}

	Gtk::Window* toplevel = current_toplevel();

	if (toplevel) {
		toplevel->fullscreen ();
		_maximised = true;
	}
}

void
Editor::restore_editing_space ()
{
	if (!_maximised) {
		return;
	}

	Gtk::Window* toplevel = current_toplevel();

	if (toplevel) {
		toplevel->unfullscreen();
		_maximised = false;
	}
}

bool
Editor::stamp_new_playlist (string title, string &name, string &pgroup, bool copy)
{
	pgroup = Playlist::generate_pgroup_id ();

	if (name.length()==0) {
		name = _("Take.1");
		if (_session->playlists()->by_name (name)) {
			name = Playlist::bump_name (name, *_session);
		}
	}

	Prompter prompter (true);
	prompter.set_title (title);
	prompter.set_prompt (_("Name for new playlist:"));
	prompter.set_initial_text (name);
	prompter.add_button (Gtk::Stock::NEW, Gtk::RESPONSE_ACCEPT);
	prompter.set_response_sensitive (Gtk::RESPONSE_ACCEPT, true);
	prompter.show_all ();

	while (true) {
		if (prompter.run () != Gtk::RESPONSE_ACCEPT) {
			return false;
		}
		prompter.get_result (name);
		if (name.length()) {
			if (_session->playlists()->by_name (name)) {
				prompter.set_prompt (_("That name is already in use.  Use this instead?"));
				prompter.set_initial_text (Playlist::bump_name (name, *_session));
			} else {
				break;
			}
		}
	}

	return true;
}

void
Editor::mapped_clear_playlist (RouteUI& rui)
{
	rui.clear_playlist ();
}

/** Clear the current playlist for a given track and also any others that belong
 *  to the same active route group with the `select' property.
 *  @param v Track.
 */

void
Editor::clear_grouped_playlists (RouteUI* rui)
{
	begin_reversible_command (_("clear playlists"));
	vector<std::shared_ptr<ARDOUR::Playlist> > playlists;
	_session->playlists()->get (playlists);
	mapover_grouped_routes (sigc::mem_fun (*this, &Editor::mapped_clear_playlist), rui, ARDOUR::Properties::group_select.property_id);
	commit_reversible_command ();
}

void
Editor::mapped_select_playlist_matching (RouteUI& rui, std::weak_ptr<ARDOUR::Playlist> pl)
{
	rui.select_playlist_matching (pl);
}

void
Editor::mapped_use_new_playlist (RouteUI& rui, std::string name, string gid, bool copy, vector<std::shared_ptr<ARDOUR::Playlist> > const & playlists)
{
	rui.use_new_playlist (name, gid, playlists, copy);
}

void
Editor::new_playlists_for_all_tracks (bool copy)
{
	string name, gid;
	if (stamp_new_playlist(  copy ?  _("Copy Playlist for ALL Tracks") : _("New Playlist for ALL Tracks"), name,gid,copy)) {
		vector<std::shared_ptr<ARDOUR::Playlist> > playlists;
		_session->playlists()->get (playlists);
		mapover_all_routes (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_new_playlist), name, gid, copy, playlists));
	}
}

void
Editor::new_playlists_for_grouped_tracks (RouteUI* rui, bool copy)
{
	string name, gid;
	if (stamp_new_playlist(  copy ?  _("Copy Playlist for this track/group") : _("New Playlist for this track/group"), name,gid,copy)) {
		vector<std::shared_ptr<ARDOUR::Playlist> > playlists;
		_session->playlists()->get (playlists);
		mapover_grouped_routes (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_new_playlist), name, gid, copy, playlists), rui, ARDOUR::Properties::group_select.property_id);
	}
}

void
Editor::new_playlists_for_selected_tracks (bool copy)
{
	string name, gid;
	if (stamp_new_playlist(  copy ?  _("Copy Playlist for Selected Tracks") : _("New Playlist for Selected Tracks"), name,gid,copy)) {
		vector<std::shared_ptr<ARDOUR::Playlist> > playlists;
		_session->playlists()->get (playlists);
		mapover_selected_routes (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_new_playlist), name, gid, copy, playlists));
	}
}

void
Editor::new_playlists_for_armed_tracks (bool copy)
{
	string name, gid;
	if (stamp_new_playlist( copy ?  _("Copy Playlist for Armed Tracks") : _("New Playlist for Armed Tracks"), name,gid,copy)) {
		vector<std::shared_ptr<ARDOUR::Playlist> > playlists;
		_session->playlists()->get (playlists);
		mapover_armed_routes (sigc::bind (sigc::mem_fun (*this, &Editor::mapped_use_new_playlist), name, gid, copy, playlists));
	}
}

double
Editor::get_y_origin () const
{
	return vertical_adjustment.get_value ();
}


void
Editor::reposition_and_zoom (samplepos_t pos, double spp)
{
	pending_visual_change.add (VisualChange::ZoomLevel);
	pending_visual_change.samples_per_pixel = spp;

	pending_visual_change.add (VisualChange::TimeOrigin);
	pending_visual_change.time_origin = pos;

	ensure_visual_change_idle_handler ();

	if (!no_save_visual) {
		undo_visual_stack.push_back (current_visual_state(false));
	}
}

Editor::VisualState::VisualState (bool with_tracks)
	: gui_state (with_tracks ? new GUIObjectState : 0)
{
}

Editor::VisualState::~VisualState ()
{
	delete gui_state;
}

Editor::VisualState*
Editor::current_visual_state (bool with_tracks)
{
	VisualState* vs = new VisualState (with_tracks);
	vs->y_position = vertical_adjustment.get_value();
	vs->samples_per_pixel = samples_per_pixel;
	vs->_leftmost_sample = _leftmost_sample;
	vs->zoom_focus = zoom_focus();

	if (with_tracks) {
		vs->gui_state->set_state (ARDOUR_UI::instance()->gui_object_state->get_state());
	}

	return vs;
}

void
Editor::undo_visual_state ()
{
	if (undo_visual_stack.empty()) {
		return;
	}

	VisualState* vs = undo_visual_stack.back();
	undo_visual_stack.pop_back();


	redo_visual_stack.push_back (current_visual_state (vs ? vs->gui_state != 0 : false));

	if (vs) {
		use_visual_state (*vs);
	}
}

void
Editor::redo_visual_state ()
{
	if (redo_visual_stack.empty()) {
		return;
	}

	VisualState* vs = redo_visual_stack.back();
	redo_visual_stack.pop_back();

	/* XXX: can 'vs' really be 0? Is there a place that puts NULL pointers onto the stack? */
	undo_visual_stack.push_back (current_visual_state (vs ? (vs->gui_state != 0) : false));

	if (vs) {
		use_visual_state (*vs);
	}
}

void
Editor::swap_visual_state ()
{
	if (undo_visual_stack.empty()) {
		redo_visual_state ();
	} else {
		undo_visual_state ();
	}
}

void
Editor::use_visual_state (VisualState& vs)
{
	PBD::Unwinder<bool> nsv (no_save_visual, true);
	DisplaySuspender ds;

	vertical_adjustment.set_value (vs.y_position);

	set_zoom_focus (vs.zoom_focus);
	reposition_and_zoom (vs._leftmost_sample, vs.samples_per_pixel);

	if (vs.gui_state) {
		ARDOUR_UI::instance()->gui_object_state->set_state (vs.gui_state->get_state());

		for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
			(*i)->clear_property_cache();
			(*i)->reset_visual_state ();
		}
	}

	// TODO push state to PresentationInfo, force update ?
}

/** This is the core function that controls the zoom level of the canvas. It is called
 *  whenever one or more calls are made to reset_zoom().  It executes in an idle handler.
 *  @param spp new number of samples per pixel
 */
void
Editor::set_samples_per_pixel (samplecnt_t spp)
{
	if (spp < 1) {
		return;
	}

	const samplecnt_t three_days = 3 * 24 * 60 * 60 * (_session ? _session->sample_rate() : 48000);
	const samplecnt_t lots_of_pixels = 4000;

	/* if the zoom level is greater than what you'd get trying to display 3
	 * days of audio on a really big screen, then it's too big.
	 */

	if (spp * lots_of_pixels > three_days) {
		return;
	}

	samples_per_pixel = spp;
}

void
Editor::on_samples_per_pixel_changed ()
{
	bool const showing_time_selection = selection->time.length() > 0;

	if (showing_time_selection && selection->time.start_sample () != selection->time.end_sample ()) {
		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			(*i)->reshow_selection (selection->time);
		}
	}

	ZoomChanged (); /* EMIT_SIGNAL */

	ArdourCanvas::GtkCanvasViewport* c = get_canvas_viewport ();

	if (c) {
		c->canvas()->zoomed ();
	}

	if (_playhead_cursor) {
		_playhead_cursor->set_position (_playhead_cursor->current_sample ());
	}

	refresh_location_display();
	_summary->set_overlays_dirty ();

	update_section_box ();
	update_marker_labels ();

	instant_save ();
}

samplepos_t
Editor::playhead_cursor_sample () const
{
	return _playhead_cursor->current_sample();
}

void
Editor::visual_changer (const VisualChange& vc)
{
	/**
	 * Changed first so the correct horizontal canvas position is calculated in
	 * Editor::set_horizontal_position
	 */
	if (vc.pending & VisualChange::ZoomLevel) {
		set_samples_per_pixel (vc.samples_per_pixel);
	}

	if (vc.pending & VisualChange::TimeOrigin) {
		double new_time_origin = sample_to_pixel_unrounded (vc.time_origin);
		set_horizontal_position (new_time_origin);
	}

	if (vc.pending & VisualChange::YOrigin) {
		vertical_adjustment.set_value (vc.y_origin);
	}

	/**
	 * Now the canvas is in the final state before render the canvas items that
	 * support the Item::prepare_for_render interface can calculate the correct
	 * item to visible canvas intersection.
	 */
	if (vc.pending & VisualChange::ZoomLevel) {
		on_samples_per_pixel_changed ();

		compute_fixed_ruler_scale ();

		compute_bbt_ruler_scale (vc.time_origin, pending_visual_change.time_origin + current_page_samples());
		update_tempo_based_rulers ();
	}

	if (!(vc.pending & VisualChange::ZoomLevel)) {
		/* If the canvas is not being zoomed then the canvas items will not change
		 * and cause Item::prepare_for_render to be called so do it here manually.
		 * Not ideal, but I can't think of a better solution atm.
		 */
		_track_canvas->prepare_for_render();
	}

	/* If we are only scrolling vertically there is no need to update these */
	if (vc.pending != VisualChange::YOrigin) {
		update_fixed_rulers ();
		redisplay_grid (true);

		/* video frames & position need to be updated for zoom, horiz-scroll
		 * and (explicitly) VisualChange::VideoTimeline.
		 */
		update_video_timeline();
	}

	_region_peak_cursor->hide ();
	_summary->set_overlays_dirty ();
}

void
Editor::queue_visual_videotimeline_update ()
{
	pending_visual_change.add (VisualChange::VideoTimeline);
	ensure_visual_change_idle_handler ();
}

struct EditorOrderTimeAxisSorter {
    bool operator() (const TimeAxisView* a, const TimeAxisView* b) const {
	    return a->order () < b->order ();
    }
};

void
Editor::sort_track_selection (TrackViewList& sel)
{
	EditorOrderTimeAxisSorter cmp;
	sel.sort (cmp);
}

timepos_t
Editor::_get_preferred_edit_position (EditIgnoreOption ignore, bool from_context_menu, bool from_outside_canvas)
{
	bool ignored;
	timepos_t where;
	EditPoint ep = _edit_point;

	if (Profile->get_mixbus()) {
		if (ep == EditAtSelectedMarker) {
			ep = EditAtPlayhead;
		}
	}

	if (from_outside_canvas && (ep == EditAtMouse)) {
		ep = EditAtPlayhead;
	} else if (from_context_menu && (ep == EditAtMouse)) {
		return timepos_t (canvas_event_sample (&context_click_event, 0, 0));
	}

	if (entered_marker) {
		DEBUG_TRACE (DEBUG::CutNPaste, string_compose ("GPEP: use entered marker @ %1\n", entered_marker->position()));
		return entered_marker->position();
	}

	if ((ignore == EDIT_IGNORE_PHEAD) && ep == EditAtPlayhead) {
		ep = EditAtSelectedMarker;
	}

	if ((ignore == EDIT_IGNORE_MOUSE) && ep == EditAtMouse) {
		ep = EditAtPlayhead;
	}

	samplepos_t ms;

	switch (ep) {
	case EditAtPlayhead:
		if (_dragging_playhead) {
			/* NOTE: since the user is dragging with the mouse, this operation will implicitly be Snapped */
			where = timepos_t (_playhead_cursor->current_sample());
		} else {
			where = timepos_t (_session->audible_sample());
		}
		DEBUG_TRACE (DEBUG::CutNPaste, string_compose ("GPEP: use playhead @ %1\n", where));
		break;

	case EditAtSelectedMarker:
		if (!selection->markers.empty()) {
			bool is_start;
			Location* loc = find_location_from_marker (selection->markers.front(), is_start);
			if (loc) {
				if (is_start) {
					where =  loc->start();
				} else {
					where = loc->end();
				}
				DEBUG_TRACE (DEBUG::CutNPaste, string_compose ("GPEP: use selected marker @ %1\n", where));
				break;
			}
		}
		/* fallthrough */

	default:
	case EditAtMouse:
		if (!mouse_sample (ms, ignored)) {
			/* XXX not right but what can we do ? */
			return timepos_t ();
		}
		where = timepos_t (ms);
		snap_to (where);
		DEBUG_TRACE (DEBUG::CutNPaste, string_compose ("GPEP: use mouse @ %1\n", where));
		break;
	}

	return where;
}

void
Editor::set_punch_range (timepos_t const & start, timepos_t const & end, string cmd)
{
	if (!_session) return;

	begin_reversible_command (cmd);

	Location* tpl;

	if ((tpl = transport_punch_location()) == 0) {
		Location* loc = new Location (*_session, start, end, _("Punch"),  Location::IsAutoPunch);
		XMLNode &before = _session->locations()->get_state();
		_session->locations()->add (loc, true);
		_session->set_auto_punch_location (loc);
		XMLNode &after = _session->locations()->get_state();
		_session->add_command (new MementoCommand<Locations>(*(_session->locations()), &before, &after));
	} else {
		XMLNode &before = tpl->get_state();
		tpl->set_hidden (false, this);
		tpl->set (start, end);
		XMLNode &after = tpl->get_state();
		_session->add_command (new MementoCommand<Location>(*tpl, &before, &after));
	}

	commit_reversible_command ();
}

/** Find regions which exist at a given time, and optionally on a given list of tracks.
 *  @param rs List to which found regions are added.
 *  @param where Time to look at.
 *  @param ts Tracks to look on; if this is empty, all tracks are examined.
 */
void
Editor::get_regions_at (RegionSelection& rs, timepos_t const & where, const TrackViewList& ts) const
{
	const TrackViewList* tracks;

	if (ts.empty()) {
		tracks = &track_views;
	} else {
		tracks = &ts;
	}

	for (TrackViewList::const_iterator t = tracks->begin(); t != tracks->end(); ++t) {

		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(*t);

		if (rtv) {
			std::shared_ptr<Track> tr;
			std::shared_ptr<Playlist> pl;

			if ((tr = rtv->track()) && ((pl = tr->playlist()))) {

				std::shared_ptr<RegionList> regions = pl->regions_at (where);

				for (RegionList::iterator i = regions->begin(); i != regions->end(); ++i) {
					RegionView* rv = rtv->view()->find_view (*i);
					if (rv) {
						rs.add (rv);
					}
				}
			}
		}
	}
}

void
Editor::get_regions_after (RegionSelection& rs, timepos_t const & where, const TrackViewList& ts) const
{
	const TrackViewList* tracks;

	if (ts.empty()) {
		tracks = &track_views;
	} else {
		tracks = &ts;
	}

	for (TrackViewList::const_iterator t = tracks->begin(); t != tracks->end(); ++t) {
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(*t);
		if (rtv) {
			std::shared_ptr<Track> tr;
			std::shared_ptr<Playlist> pl;

			if ((tr = rtv->track()) && ((pl = tr->playlist()))) {

				std::shared_ptr<RegionList> regions = pl->regions_touched (where, timepos_t::max (where.time_domain()));

				for (RegionList::iterator i = regions->begin(); i != regions->end(); ++i) {

					RegionView* rv = rtv->view()->find_view (*i);

					if (rv) {
						rs.add (rv);
					}
				}
			}
		}
	}
}

/** Get regions using the following method:
 *
 *  Make a region list using:
 *   (a) any selected regions
 *   (b) the intersection of any selected tracks and the edit point(*)
 *   (c) if neither exists, and edit_point == mouse, then whatever region is under the mouse
 *
 *  (*) NOTE: in this case, if 'No Selection = All Tracks' is active, search all tracks
 *
 *  Note that we have forced the rule that selected regions and selected tracks are mutually exclusive
 */

RegionSelection
Editor::get_regions_from_selection_and_edit_point (EditIgnoreOption ignore, bool from_context_menu, bool from_outside_canvas)
{
	RegionSelection regions;

	if (_edit_point == EditAtMouse && entered_regionview && selection->tracks.empty() && selection->regions.empty()) {
		regions.add (entered_regionview);
	} else {
		regions = selection->regions;
	}

	if (regions.empty()) {
		TrackViewList tracks = selection->tracks;

		if (!tracks.empty()) {
			/* no region selected or entered, but some selected tracks:
			 * act on all regions on the selected tracks at the edit point
			 */
			timepos_t const where = get_preferred_edit_position (ignore, from_context_menu, from_outside_canvas);
			get_regions_at(regions, where, tracks);
		}
	}

	return regions;
}

/** Get regions using the following method:
 *
 *  Make a region list using:
 *   (a) any selected regions
 *   (b) the intersection of any selected tracks and the edit point(*)
 *   (c) if neither exists, then whatever region is under the mouse
 *
 *  (*) NOTE: in this case, if 'No Selection = All Tracks' is active, search all tracks
 *
 *  Note that we have forced the rule that selected regions and selected tracks are mutually exclusive
 */
RegionSelection
Editor::get_regions_from_selection_and_mouse (timepos_t const & pos)
{
	RegionSelection regions;

	if (entered_regionview && selection->tracks.empty() && selection->regions.empty()) {
		regions.add (entered_regionview);
	} else {
		regions = selection->regions;
	}

	if (regions.empty()) {
		TrackViewList tracks = selection->tracks;

		if (!tracks.empty()) {
			/* no region selected or entered, but some selected tracks:
			 * act on all regions on the selected tracks at the edit point
			 */
			get_regions_at (regions, pos, tracks);
		}
	}

	return regions;
}

/** Start with the selected Region(s) or TriggerSlot
 *  if neither is found, try using the entered_regionview (region under the mouse).
 */

RegionSelection
Editor::get_regions_from_selection_and_entered () const
{
	RegionSelection regions = selection->regions;

	if (regions.empty() && !selection->triggers.empty()) {
		regions = selection->trigger_regionview_proxy();
	}

	if (regions.empty() && entered_regionview) {
		regions.add (entered_regionview);
	}

	return regions;
}

void
Editor::get_regionviews_by_id (PBD::ID const id, RegionSelection & regions) const
{
	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		RouteTimeAxisView* rtav;

		if ((rtav = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {
			std::shared_ptr<Playlist> pl;
			std::vector<std::shared_ptr<Region> > results;
			std::shared_ptr<Track> tr;

			if ((tr = rtav->track()) == 0) {
				/* bus */
				continue;
			}

			if ((pl = (tr->playlist())) != 0) {
				std::shared_ptr<Region> r = pl->region_by_id (id);
				if (r) {
					RegionView* rv = rtav->view()->find_view (r);
					if (rv) {
						regions.push_back (rv);
					}
				}
			}
		}
	}
}

void
Editor::get_per_region_note_selection (list<pair<PBD::ID, set<std::shared_ptr<Evoral::Note<Temporal::Beats> > > > > &selection) const
{

	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		MidiTimeAxisView* mtav;

		if ((mtav = dynamic_cast<MidiTimeAxisView*> (*i)) != 0) {

			mtav->get_per_region_note_selection (selection);
		}
	}

}

void
Editor::get_regionview_corresponding_to (std::shared_ptr<Region> region, vector<RegionView*>& regions)
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {

		RouteTimeAxisView* tatv;

		if ((tatv = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {

			std::shared_ptr<Playlist> pl;
			RegionView* marv;
			std::shared_ptr<Track> tr;

			if ((tr = tatv->track()) == 0) {
				/* bus */
				continue;
			}

			if ((marv = tatv->view()->find_view (region)) != 0) {
				regions.push_back (marv);
			}
		}
	}
}

RegionView*
Editor::regionview_from_region (std::shared_ptr<Region> region) const
{
	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		RouteTimeAxisView* tatv;
		if ((tatv = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {
			if (!tatv->track()) {
				continue;
			}
			RegionView* marv = tatv->view()->find_view (region);
			if (marv) {
				return marv;
			}
		}
	}
	return NULL;
}

RouteTimeAxisView*
Editor::rtav_from_route (std::shared_ptr<Route> route) const
{
	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		RouteTimeAxisView* rtav;
		if ((rtav = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {
			if (rtav->route() == route) {
				return rtav;
			}
		}
	}
	return NULL;
}

void
Editor::show_rhythm_ferret ()
{
	if (rhythm_ferret == 0) {
		rhythm_ferret = new RhythmFerret(*this);
	}

	rhythm_ferret->set_session (_session);
	rhythm_ferret->show ();
	rhythm_ferret->present ();
}

void
Editor::first_idle ()
{
	ArdourMessageDialog* dialog = 0;

	if (track_views.size() > 1) {
		Timers::TimerSuspender t;
		dialog = new ArdourMessageDialog (
			string_compose (_("Please wait while %1 loads visual data."), PROGRAM_NAME),
			true
			);
		dialog->present ();
		ARDOUR_UI::instance()->flush_pending (60);
	}

	for (TrackViewList::iterator t = track_views.begin(); t != track_views.end(); ++t) {
		(*t)->first_idle();
	}

	/* now that all regionviews should exist, setup region selection */

	RegionSelection rs;

	for (list<PBD::ID>::iterator pr = selection->regions.pending.begin (); pr != selection->regions.pending.end (); ++pr) {
		/* this is cumulative: rs is NOT cleared each time */
		get_regionviews_by_id (*pr, rs);
	}

	selection->set (rs);

	/* first idle adds route children (automation tracks), so we need to redisplay here */
	redisplay_track_views ();

	delete dialog;

	if (_session->undo_depth() == 0) {
		undo_action->set_sensitive(false);
	}
	redo_action->set_sensitive(false);
	begin_selection_op_history ();

	_have_idled = true;
}

gboolean
Editor::_idle_resize (gpointer arg)
{
	return ((Editor*)arg)->idle_resize ();
}

void
Editor::add_to_idle_resize (TimeAxisView* view, int32_t h)
{
	if (resize_idle_id < 0) {
		/* https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html#G-PRIORITY-HIGH-IDLE:CAPS
		 * GTK+ uses G_PRIORITY_HIGH_IDLE + 10 for resizing operations, and G_PRIORITY_HIGH_IDLE + 20 for redrawing operations.
		 * (This is done to ensure that any pending resizes are processed before any pending redraws, so that widgets are not redrawn twice unnecessarily.)
		 */
		resize_idle_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE + 10, _idle_resize, this, NULL);
		queue_redisplay_track_views ();
		_pending_resize_amount = 0;
	}

	/* make a note of the smallest resulting height, so that we can clamp the
	   lower limit at TimeAxisView::hSmall */

	int32_t min_resulting = INT32_MAX;

	_pending_resize_amount += h;
	_pending_resize_view = view;

	min_resulting = min (min_resulting, int32_t (_pending_resize_view->current_height()) + _pending_resize_amount);

	if (selection->tracks.contains (_pending_resize_view)) {
		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			min_resulting = min (min_resulting, int32_t ((*i)->current_height()) + _pending_resize_amount);
		}
	}

	if (min_resulting < 0) {
		min_resulting = 0;
	}

	/* clamp */
	if (uint32_t (min_resulting) < TimeAxisView::preset_height (HeightSmall)) {
		_pending_resize_amount += TimeAxisView::preset_height (HeightSmall) - min_resulting;
	}
}

/** Handle pending resizing of tracks */
bool
Editor::idle_resize ()
{
	_pending_resize_view->idle_resize (_pending_resize_view->current_height() + _pending_resize_amount);

	if (dynamic_cast<AutomationTimeAxisView*> (_pending_resize_view) == 0 &&
	    selection->tracks.contains (_pending_resize_view)) {

		for (TrackViewList::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
			if (*i != _pending_resize_view) {
				(*i)->idle_resize ((*i)->current_height() + _pending_resize_amount);
			}
		}
	}

	_pending_resize_amount = 0;
	_group_tabs->set_dirty ();
	resize_idle_id = -1;

	return false;
}

void
Editor::located ()
{
	ENSURE_GUI_THREAD (*this, &Editor::located);

	if (_session) {
		_playhead_cursor->set_position (_session->audible_sample ());
		if (follow_playhead() && !_pending_initial_locate) {
			reset_x_origin_to_follow_playhead ();
		}
		update_section_box ();
	}

	_pending_locate_request = false;
	_pending_initial_locate = false;
	_last_update_time = 0;
}

void
Editor::region_view_added (RegionView * rv)
{
	MidiRegionView* mrv = dynamic_cast<MidiRegionView*> (rv);
	if (mrv) {
		list<pair<PBD::ID const, list<Evoral::event_id_t> > >::iterator rnote;
		for (rnote = selection->pending_midi_note_selection.begin(); rnote != selection->pending_midi_note_selection.end(); ++rnote) {
			if (rv->region()->id () == (*rnote).first) {
				list<Evoral::event_id_t> notes ((*rnote).second);
				selection->pending_midi_note_selection.erase(rnote);
				mrv->select_notes (notes, false); // NB. this may change the selection
				break;
			}
		}
	}

	_summary->set_background_dirty ();

	mark_region_boundary_cache_dirty ();
}

void
Editor::region_view_removed ()
{
	_summary->set_background_dirty ();

	mark_region_boundary_cache_dirty ();
}

AxisView*
Editor::axis_view_by_stripable (std::shared_ptr<Stripable> s) const
{
	for (TrackViewList::const_iterator j = track_views.begin (); j != track_views.end(); ++j) {
		if ((*j)->stripable() == s) {
			return *j;
		}
	}

	return 0;
}

AxisView*
Editor::axis_view_by_control (std::shared_ptr<AutomationControl> c) const
{
	for (TrackViewList::const_iterator j = track_views.begin (); j != track_views.end(); ++j) {
		if ((*j)->control() == c) {
			return *j;
		}

		TimeAxisView::Children kids = (*j)->get_child_list ();

		for (TimeAxisView::Children::iterator k = kids.begin(); k != kids.end(); ++k) {
			if ((*k)->control() == c) {
				return (*k).get();
			}
		}
	}

	return 0;
}

TrackViewList
Editor::axis_views_from_routes (std::shared_ptr<RouteList> r) const
{
	TrackViewList t;

	for (RouteList::const_iterator i = r->begin(); i != r->end(); ++i) {
		TimeAxisView* tv = time_axis_view_from_stripable (*i);
		if (tv) {
			t.push_back (tv);
		}
	}

	return t;
}

void
Editor::suspend_route_redisplay ()
{
	_tvl_no_redisplay = true;
}

void
Editor::queue_redisplay_track_views ()
{
	if (!_tvl_redisplay_connection.connected ()) {
		_tvl_redisplay_connection = Glib::signal_idle().connect (sigc::mem_fun (*this, &Editor::redisplay_track_views), Glib::PRIORITY_HIGH_IDLE+10);
	}
}

bool
Editor::process_redisplay_track_views ()
{
	if (_tvl_redisplay_connection.connected ()) {
		_tvl_redisplay_connection.disconnect ();
		redisplay_track_views ();
	}

	return false;
}

void
Editor::resume_route_redisplay ()
{
	_tvl_no_redisplay = false;
	if (_tvl_redisplay_on_resume) {
		queue_redisplay_track_views ();
	}
}

void
Editor::initial_display ()
{
	DisplaySuspender ds;
	StripableList s;
	_session->get_stripables (s);
	add_stripables (s);
}

void
Editor::add_vcas (VCAList& vlist)
{
	StripableList sl;

	for (VCAList::iterator v = vlist.begin(); v != vlist.end(); ++v) {
		sl.push_back (std::dynamic_pointer_cast<Stripable> (*v));
	}

	add_stripables (sl);
}

void
Editor::add_routes (RouteList& rlist)
{
	StripableList sl;

	for (RouteList::iterator r = rlist.begin(); r != rlist.end(); ++r) {
		sl.push_back (*r);
	}

	add_stripables (sl);
}

void
Editor::add_stripables (StripableList& sl)
{
	std::shared_ptr<VCA> v;
	std::shared_ptr<Route> r;
	TrackViewList new_selection;
	bool changed = false;
	bool from_scratch = (track_views.size() == 0);

	sl.sort (Stripable::Sorter());

	DisplaySuspender ds;

	for (StripableList::iterator s = sl.begin(); s != sl.end(); ++s) {

		if ((*s)->is_foldbackbus()) {
			continue;
		}

		if ((v = std::dynamic_pointer_cast<VCA> (*s)) != 0) {

			VCATimeAxisView* vtv = new VCATimeAxisView (*this, _session, *_track_canvas);
			vtv->set_vca (v);
			track_views.push_back (vtv);

			(*s)->gui_changed.connect (*this, invalidator (*this), std::bind (&Editor::handle_gui_changes, this, _1, _2), gui_context());
			changed = true;

		} else if ((r = std::dynamic_pointer_cast<Route> (*s)) != 0) {

			if (r->is_auditioner() || r->is_monitor() || r->is_surround_master ()) {
				continue;
			}

			RouteTimeAxisView* rtv;
			DataType dt = r->input()->default_type();

			if (dt == ARDOUR::DataType::AUDIO) {
				rtv = new AudioTimeAxisView (*this, _session, *_track_canvas);
				rtv->set_route (r);
			} else if (dt == ARDOUR::DataType::MIDI) {
				rtv = new MidiTimeAxisView (*this, _session, *_track_canvas);
				rtv->set_route (r);
			} else {
				throw unknown_type();
			}

			track_views.push_back (rtv);
			new_selection.push_back (rtv);

			rtv->effective_gain_display ();

			rtv->view()->RegionViewAdded.connect (sigc::mem_fun (*this, &Editor::region_view_added));
			rtv->view()->RegionViewRemoved.connect (sigc::mem_fun (*this, &Editor::region_view_removed));
			(*s)->gui_changed.connect (*this, invalidator (*this), std::bind (&Editor::handle_gui_changes, this, _1, _2), gui_context());
			changed = true;
		}
	}

	if (changed) {
		queue_redisplay_track_views ();
	}

	/* note: !new_selection.empty() means that we got some routes rather
	 * than just VCAs
	 */

	if (!from_scratch && !_no_not_select_reimported_tracks && !new_selection.empty()) {
		selection->set (new_selection);
		begin_selection_op_history();
	}

	if (show_editor_mixer_when_tracks_arrive && !new_selection.empty()) {
		show_editor_mixer (true);
	}
}

void
Editor::timeaxisview_deleted (TimeAxisView *tv)
{
	if (tv == entered_track) {
		entered_track = 0;
	}

	if (_session && _session->deletion_in_progress()) {
		/* the situation is under control */
		return;
	}

	DisplaySuspender ds;

	ENSURE_GUI_THREAD (*this, &Editor::timeaxisview_deleted, tv);

	if (dynamic_cast<AutomationTimeAxisView*> (tv)) {
		selection->remove (tv);
		return;
	}

	RouteTimeAxisView* rtav = dynamic_cast<RouteTimeAxisView*> (tv);

	TimeAxisView::Children c = tv->get_child_list ();
	for (TimeAxisView::Children::const_iterator i = c.begin(); i != c.end(); ++i) {
		if (entered_track == i->get()) {
			entered_track = 0;
		}
	}

	/* remove it from the list of track views */

	TrackViewList::iterator i;

	if ((i = find (track_views.begin(), track_views.end(), tv)) != track_views.end()) {
		i = track_views.erase (i);
	}

	/* Update the route that is shown in the editor-mixer. */
	if (!rtav) {
		return;
	}

	std::shared_ptr<Route> route = rtav->route ();
	if (current_mixer_strip && current_mixer_strip->route() == route) {

		TimeAxisView* next_tv;

		if (track_views.empty()) {
			next_tv = 0;
		} else if (i == track_views.end()) {
			next_tv = track_views.front();
		} else {
			next_tv = (*i);
		}

		// skip VCAs (cannot be selected, n/a in editor-mixer)
		if (dynamic_cast<VCATimeAxisView*> (next_tv)) {
			/* VCAs are sorted last in line -- route_sorter.h, jump to top */
			next_tv = track_views.front();
		}
		if (dynamic_cast<VCATimeAxisView*> (next_tv)) {
			/* just in case: no master, only a VCA remains */
			next_tv = 0;
		}


		if (next_tv) {
			set_selected_mixer_strip (*next_tv);
		} else {
			/* make the editor mixer strip go away setting the
			 * button to inactive (which also unticks the menu option)
			 */

			ActionManager::uncheck_toggleaction ("Editor/show-editor-mixer");
		}
	}
}

void
Editor::hide_track_in_display (TimeAxisView* tv, bool apply_to_selection)
{
	if (!tv) {
		return;
	}

	DisplaySuspender ds;
	PresentationInfo::ChangeSuspender cs;

	if (apply_to_selection) {
		for (TrackSelection::iterator i = selection->tracks.begin(); i != selection->tracks.end();) {

			TrackSelection::iterator j = i;
			++j;

			hide_track_in_display (*i, false);

			i = j;
		}
	} else {
		RouteTimeAxisView*     rtv = dynamic_cast<RouteTimeAxisView*> (tv);
		StripableTimeAxisView* stv = dynamic_cast<StripableTimeAxisView*> (tv);

		if (rtv && current_mixer_strip && (rtv->route() == current_mixer_strip->route())) {
			/* this will hide the mixer strip */
			set_selected_mixer_strip (*tv);
		}
		if (stv) {
			stv->stripable()->presentation_info().set_hidden (true);
			/* TODO also handle Routegroups IFF (rg->is_hidden() && !rg->is_selection())
			 * selection currently unconditionally hides due to above if() clause :(
			 */
		}
	}
}

void
Editor::show_track_in_display (TimeAxisView* tv, bool move_into_view)
{
	if (!tv) {
		return;
	}
	StripableTimeAxisView* stv = dynamic_cast<StripableTimeAxisView*> (tv);
	if (stv) {
		stv->stripable()->presentation_info().set_hidden (false);
#if 0 // TODO see above
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (tv);
		RouteGroup* rg = rtv->route ()->route_group ();
		if (rg && rg->is_active () && rg->is_hidden () && !rg->is_select ()) {
			std::shared_ptr<RouteList> rl (rg->route_list ());
			for (RouteList::const_iterator i = rl->begin(); i != rl->end(); ++i) {
				(*i)->presentation_info().set_hidden (false);
			}
	}
#endif
	}
	if (move_into_view) {
		ensure_time_axis_view_is_visible (*tv, false);
	}
}

struct TrackViewStripableSorter
{
  bool operator() (const TimeAxisView* tav_a, const TimeAxisView *tav_b)
  {
    StripableTimeAxisView const* stav_a = dynamic_cast<StripableTimeAxisView const*>(tav_a);
    StripableTimeAxisView const* stav_b = dynamic_cast<StripableTimeAxisView const*>(tav_b);
    assert (stav_a && stav_b);

    std::shared_ptr<ARDOUR::Stripable> const& a = stav_a->stripable ();
    std::shared_ptr<ARDOUR::Stripable> const& b = stav_b->stripable ();
    return ARDOUR::Stripable::Sorter () (a, b);
  }
};

void
Editor::maybe_move_tracks ()
{
	for (auto & tv : track_views) {

		if (!tv->marked_for_display () || (tv == track_drag->track)) {
			continue;
		}

		/* find the track the mouse pointer is within, and if
		 * we're in the upper or lower half of it (depending on
		 * drag direction, move the spacer.
		 */

		if (track_drag->current >= tv->y_position() && track_drag->current < (tv->y_position() + tv->effective_height())) {

			if (track_drag->bump_track == tv) {
				/* already bumped for this track */
				break;
			}

			if (track_drag->direction < 0) {

				/* dragging up */

				if (track_drag->current < (tv->y_position() + (tv->effective_height() / 2))) {
					/* in top half of this track, move spacer */
					track_drag->bump_track = tv;
					move_selected_tracks (true);
					track_drag->did_reorder = true;
				}

			} else if (track_drag->direction > 0) {

				/* dragging down */

				if (track_drag->current > (tv->y_position() + (tv->effective_height() / 2))) {
					track_drag->bump_track = tv;
					move_selected_tracks (false);
					track_drag->did_reorder = true;
				}
			}

			break;
		}
	}
}

bool
Editor::redisplay_track_views ()
{
	if (!_session || _session->deletion_in_progress()) {
		return false;
	}

	if (_tvl_no_redisplay) {
		_tvl_redisplay_on_resume = true;
		return false;
	}

	_tvl_redisplay_on_resume = false;

	track_views.sort (TrackViewStripableSorter ());

	if (track_drag) { //  && track_drag->spacer) {
		maybe_move_tracks ();
	}

	/* n will be the count of tracks plus children (updated by TimeAxisView::show_at),
	 * so we will use that to know where to put things.
	 */
	int n = 0;
	uint32_t position = 0;

	for (auto & tv : track_views) {

		if (tv->marked_for_display ()) {
			position += tv->show_at (position, n, &edit_controls_vbox);
		} else {
			tv->hide ();
		}

		n++;
	}

	reset_controls_layout_height (position);
	reset_controls_layout_width ();
	_full_canvas_height = position;

	if ((vertical_adjustment.get_value() + _visible_canvas_height) > vertical_adjustment.get_upper()) {
		/*
		 * We're increasing the size of the canvas while the bottom is visible.
		 * We scroll down to keep in step with the controls layout.
		 */
		vertical_adjustment.set_value (_full_canvas_height - _visible_canvas_height);
	}

	_summary->set_background_dirty();
	_group_tabs->set_dirty ();

	return false;
}

void
Editor::handle_gui_changes (string const & what, void*)
{
	if (what == "visible_tracks") {
		queue_redisplay_track_views ();
	}
}

void
Editor::foreach_time_axis_view (sigc::slot<void,TimeAxisView&> theslot)
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		theslot (**i);
	}
}

/** Find a StripableTimeAxisView by the ID of its stripable */
StripableTimeAxisView*
Editor::get_stripable_time_axis_by_id (const PBD::ID& id) const
{
	StripableTimeAxisView* v;

	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {
		if((v = dynamic_cast<StripableTimeAxisView*>(*i)) != 0) {
			if(v->stripable()->id() == id) {
				return v;
			}
		}
	}

	return 0;
}

void
Editor::fit_route_group (RouteGroup *g)
{
	TrackViewList ts = axis_views_from_routes (g->route_list ());
	fit_tracks (ts);
}

void
Editor::consider_auditioning (std::shared_ptr<Region> region)
{
	std::shared_ptr<AudioRegion> r = std::dynamic_pointer_cast<AudioRegion> (region);

	if (r == 0) {
		_session->cancel_audition ();
		return;
	}

	if (_session->is_auditioning()) {
		_session->cancel_audition ();
		if (r == last_audition_region) {
			return;
		}
	}

	_session->audition_region (r);
	last_audition_region = r;
}

void
Editor::hide_a_region (std::shared_ptr<Region> r)
{
	r->set_hidden (true);
}

void
Editor::show_a_region (std::shared_ptr<Region> r)
{
	r->set_hidden (false);
}

void
Editor::audition_region_from_region_list ()
{
	_regions->selection_mapover (sigc::mem_fun (*this, &Editor::consider_auditioning));
}

void
Editor::step_edit_status_change (bool yn)
{
	if (yn) {
		start_step_editing ();
	} else {
		stop_step_editing ();
	}
}

void
Editor::start_step_editing ()
{
	step_edit_connection = Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::check_step_edit), 20);
}

void
Editor::stop_step_editing ()
{
	step_edit_connection.disconnect ();
}

bool
Editor::check_step_edit ()
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		MidiTimeAxisView* mtv = dynamic_cast<MidiTimeAxisView*> (*i);
		if (mtv) {
			mtv->check_step_edit ();
		}
	}

	return true; // do it again, till we stop
}

bool
Editor::scroll_press (Direction dir)
{
	++_scroll_callbacks;

	if (_scroll_connection.connected() && _scroll_callbacks < 5) {
		/* delay the first auto-repeat */
		return true;
	}

	switch (dir) {
	case LEFT:
		scroll_backward (1);
		break;

	case RIGHT:
		scroll_forward (1);
		break;

	case UP:
		scroll_up_one_track ();
		break;

	case DOWN:
		scroll_down_one_track ();
		break;
	}

	/* do hacky auto-repeat */
	if (!_scroll_connection.connected ()) {

		_scroll_connection = Glib::signal_timeout().connect (
			sigc::bind (sigc::mem_fun (*this, &Editor::scroll_press), dir), 100
			);

		_scroll_callbacks = 0;
	}

	return true;
}

void
Editor::scroll_release ()
{
	_scroll_connection.disconnect ();
}

void
Editor::super_rapid_screen_update ()
{
	if (!_session || !_session->engine().running()) {
		return;
	}

	/* METERING / MIXER STRIPS */

	/* update track meters, if required */
	if (!UIConfiguration::instance().get_no_strobe() && contents().get_mapped() && meters_running) {
		RouteTimeAxisView* rtv;
		for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
			if ((rtv = dynamic_cast<RouteTimeAxisView*>(*i)) != 0) {
				rtv->fast_update ();
			}
		}
	}

	/* and any current mixer strip */
	if (!UIConfiguration::instance().get_no_strobe() && current_mixer_strip) {
		current_mixer_strip->fast_update ();
	}

	bool latent_locate = false;
	samplepos_t sample = _session->audible_sample (&latent_locate);
	const int64_t now = g_get_monotonic_time ();
	double err = 0;

	if (_session->exporting ()) {
		/* freewheel/export may be faster or slower than transport_speed() / SR.
		 * Also exporting multiple ranges locates/jumps without a _pending_locate_request.
		 */
		_last_update_time = 0;
	}

	if (!_session->transport_rolling () || _session->is_auditioning ()) {
		/* Do not interpolate the playhead position; just set it */
		_last_update_time = 0;
	}

	if (_last_update_time > 0) {
		/* interpolate and smoothen playhead position */
		const double ds =  (now - _last_update_time) * _session->transport_speed() * _session->nominal_sample_rate () * 1e-6;
		samplepos_t guess = _playhead_cursor->current_sample () + rint (ds);
		err = sample - guess;

		guess += err * .12 + _err_screen_engine; // time-constant based on 25fps (super_rapid_screen_update)
		_err_screen_engine += .0144 * (err - _err_screen_engine); // tc^2

#if 0 // DEBUG
		printf ("eng: %ld  gui:%ld (%+6.1f)  diff: %6.1f (err: %7.2f)\n",
				sample, guess, ds,
				err, _err_screen_engine);
#endif

		sample = guess;
	} else {
		_err_screen_engine = 0;
	}

	if (err > 8192 || latent_locate) {
		// in case of xruns or freewheeling
		_last_update_time = 0;
		sample = _session->audible_sample ();
	} else {
		_last_update_time = now;
	}

	/* snapped cursor stuff (the snapped_cursor shows where an operation is going to occur) */
	bool ignored;
	MusicSample where (sample, 0);
	if (!UIConfiguration::instance().get_show_snapped_cursor()) {
		_snapped_cursor->hide ();
	} else if (_edit_point == EditAtPlayhead && !_dragging_playhead) {
		/* EditAtPlayhead does not snap */
	} else if (_edit_point == EditAtSelectedMarker) {
		/* NOTE: I don't think EditAtSelectedMarker should snap. They are what they are.
		 * however, the current editing code -does- snap so I'll draw it that way for now.
		 */
		if (!selection->markers.empty()) {
			timepos_t ms (selection->markers.front()->position());
			snap_to (ms); // should use snap_to_with_modifier?
			_snapped_cursor->set_position (ms.samples());
			if (UIConfiguration::instance().get_show_snapped_cursor()) {
				_snapped_cursor->show ();
			}
		}
	} else if (_edit_point == EditAtMouse && mouse_sample (where.sample, ignored)) {
		/* cursor is in the editing canvas. show it. */
		if (!_drags->active()) {
			if (UIConfiguration::instance().get_show_snapped_cursor()) {
				_snapped_cursor->show ();
			}
		}
	} else {
		/* mouse is out of the editing canvas, or edit-point isn't mouse. Hide the snapped_cursor */
		_snapped_cursor->hide ();
	}

	/* There are a few reasons why we might not update the playhead / viewport stuff:
	 *
	 * 1.  we don't update things when there's a pending locate request, otherwise
	 *     when the editor requests a locate there is a chance that this method
	 *     will move the playhead before the locate request is processed, causing
	 *     a visual glitch.
	 * 2.  if we're not rolling, there's nothing to do here (locates are handled elsewhere).
	 * 3.  if we're still at the same frame that we were last time, there's nothing to do.
	 */
	if (_pending_locate_request) {
		_last_update_time = 0;
		return;
	}

	if (_dragging_playhead) {
		_last_update_time = 0;
		return;
	}

	if (_playhead_cursor->current_sample () == sample) {
		return;
	}

	if (!_pending_locate_request && !_session->locate_initiated()) {
		_playhead_cursor->set_position (sample);
	}

	update_section_box ();

	if (_session->requested_return_sample() >= 0) {
		_last_update_time = 0;
		return;
	}

	if (!follow_playhead() || pending_visual_change.being_handled) {
		/* We only do this if we aren't already
		 * handling a visual change (ie if
		 * pending_visual_change.being_handled is
		 * false) so that these requests don't stack
		 * up there are too many of them to handle in
		 * time.
		 */
		return;
	}

	if (!_stationary_playhead) {
		reset_x_origin_to_follow_playhead ();
	} else {
		samplepos_t const sample = _playhead_cursor->current_sample ();
		double target = ((double)sample - (double)current_page_samples() / 2.0);
		if (target <= 0.0) {
			target = 0.0;
		}
		/* compare to EditorCursor::set_position() */
		double const old_pos = sample_to_pixel_unrounded (_leftmost_sample);
		double const new_pos = sample_to_pixel_unrounded (target);
		if (rint (new_pos) != rint (old_pos)) {
			reset_x_origin (pixel_to_sample (new_pos));
		}
	}
}


void
Editor::session_going_away ()
{
	_have_idled = false;

	_session_connections.drop_connections ();

	super_rapid_screen_update_connection.disconnect ();

	selection->clear ();
	cut_buffer->clear ();

	clicked_regionview = 0;
	clicked_axisview = 0;
	clicked_routeview = 0;
	entered_regionview = 0;
	entered_track = 0;
	_last_update_time = 0;
	_drags->abort ();

	_playhead_cursor->hide ();

	/* rip everything out of the list displays */

	_routes->clear ();
	_route_groups->clear ();

	/* do this first so that deleting a track doesn't reset cms to null
	   and thus cause a leak.
	*/

	if (current_mixer_strip) {
		if (current_mixer_strip->get_parent() != 0) {
			content_att_left.remove ();
		}
		delete current_mixer_strip;
		current_mixer_strip = 0;
	}

	/* delete all trackviews */

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		delete *i;
	}
	track_views.clear ();

	/* clear tempo/meter rulers */
	remove_metric_marks ();
	clear_marker_display ();

	drop_grid ();

	stop_step_editing ();

	if (own_window()) {

		/* get rid of any existing editor mixer strip */

		WindowTitle title(Glib::get_application_name());
		title += _("Editor");

		own_window()->set_title (title.get_string());
	}

	SessionHandlePtr::session_going_away ();
}

void
Editor::trigger_script (int i)
{
	LuaInstance::instance()-> call_action (i);
}

void
Editor::change_region_layering_order (bool from_context_menu)
{
	const timepos_t position = get_preferred_edit_position (EDIT_IGNORE_NONE, from_context_menu);

	if (!clicked_routeview) {
		if (layering_order_editor) {
			layering_order_editor->hide ();
		}
		return;
	}

	std::shared_ptr<Track> track = std::dynamic_pointer_cast<Track> (clicked_routeview->route());

	if (!track) {
		return;
	}

	std::shared_ptr<Playlist> pl = track->playlist();

	if (!pl) {
		return;
	}

	if (layering_order_editor == 0) {
		layering_order_editor = new RegionLayeringOrderEditor (*this);
	}

	layering_order_editor->set_context (clicked_routeview->name(), _session, clicked_routeview, pl, position);
	layering_order_editor->maybe_present ();
}

void
Editor::update_region_layering_order_editor ()
{
	if (layering_order_editor && layering_order_editor->get_visible ()) {
		change_region_layering_order (true);
	}
}

void
Editor::setup_fade_images ()
{
	_xfade_in_images[FadeLinear] = new Gtk::Image (get_icon_path (X_("fadein-linear")));
	_xfade_in_images[FadeSymmetric] = new Gtk::Image (get_icon_path (X_("fadein-symmetric")));
	_xfade_in_images[FadeFast] = new Gtk::Image (get_icon_path (X_("fadein-fast-cut")));
	_xfade_in_images[FadeSlow] = new Gtk::Image (get_icon_path (X_("fadein-slow-cut")));
	_xfade_in_images[FadeConstantPower] = new Gtk::Image (get_icon_path (X_("fadein-constant-power")));

	_xfade_out_images[FadeLinear] = new Gtk::Image (get_icon_path (X_("fadeout-linear")));
	_xfade_out_images[FadeSymmetric] = new Gtk::Image (get_icon_path (X_("fadeout-symmetric")));
	_xfade_out_images[FadeFast] = new Gtk::Image (get_icon_path (X_("fadeout-fast-cut")));
	_xfade_out_images[FadeSlow] = new Gtk::Image (get_icon_path (X_("fadeout-slow-cut")));
	_xfade_out_images[FadeConstantPower] = new Gtk::Image (get_icon_path (X_("fadeout-constant-power")));

}

/** @return Gtk::manage()d menu item for a given action from `editor_actions' */
Gtk::MenuItem&
Editor::action_menu_item (std::string const & name)
{
	Glib::RefPtr<Action> a = editor_actions->get_action (name);
	assert (a);

	return *manage (a->create_menu_item ());
}

void
Editor::add_notebook_page (string const& label, string const& name, Gtk::Widget& widget)
{
	_the_notebook.append_page (widget, name);

	using namespace Menu_Helpers;
	_notebook_tab1.add_item (label, name, [this, &widget]() {_the_notebook.set_current_page (_the_notebook.page_num (widget)); });
	_notebook_tab2.add_item (label, name, [this, &widget]() {_the_notebook.set_current_page (_the_notebook.page_num (widget)); });
}

void
Editor::popup_control_point_context_menu (ArdourCanvas::Item* item, GdkEvent* event)
{
	using namespace Menu_Helpers;

	MenuList& items = _control_point_context_menu.items ();
	items.clear ();

	items.push_back (MenuElem (_("Edit..."), sigc::bind (sigc::mem_fun (*this, &Editor::edit_control_point), item)));
	items.push_back (MenuElem (_("Delete"), sigc::bind (sigc::mem_fun (*this, &Editor::remove_control_point), item)));
	if (!can_remove_control_point (item)) {
		items.back().set_sensitive (false);
	}

	_control_point_context_menu.popup (event->button.button, event->button.time);
}

void
Editor::zoom_vertical_modifier_released()
{
	_stepping_axis_view = 0;
}

void
Editor::ui_parameter_changed (string parameter)
{
	EditingContext::ui_parameter_changed (parameter);

	if (parameter == "icon-set") {
		_cursors->set_cursor_set (UIConfiguration::instance().get_icon_set());
		content_right_pane.set_drag_cursor (*PublicEditor::instance().cursors()->expand_left_right);
		editor_summary_pane.set_drag_cursor (*_cursors->expand_up_down);

	} else if (parameter == "sensitize-playhead") {
		if (_playhead_cursor) {
			_playhead_cursor->set_sensitive (UIConfiguration::instance().get_sensitize_playhead());
		}
	} else if (parameter == "use-note-bars-for-velocity") {
		ArdourCanvas::Note::set_show_velocity_bars (UIConfiguration::instance().get_use_note_bars_for_velocity());
		_track_canvas->request_redraw (_track_canvas->visible_area());
	} else if (parameter == "use-note-color-for-velocity") {
		/* handled individually by each MidiRegionView */
	} else if (parameter == "show-selection-marker") {
		update_ruler_visibility ();
	}
}

Gtk::Window*
Editor::use_own_window (bool and_fill_it)
{
	bool new_window = !own_window();

	Gtk::Window* win = Tabbable::use_own_window (and_fill_it);

	if (win && new_window) {
		win->set_name ("EditorWindow");

		ARDOUR_UI::instance()->setup_toplevel_window (*win, _("Editor"), this);

		// win->signal_realize().connect (*this, &Editor::on_realize);
		win->signal_event().connect (sigc::bind (sigc::ptr_fun (&Keyboard::catch_user_event_for_pre_dialog_focus), win));
		win->signal_event().connect (sigc::mem_fun (*this, &Editor::generic_event_handler));
		set_widget_bindings (*win, bindings, ARDOUR_BINDING_KEY);

		update_title ();
	}

	DisplaySuspender ds;
	contents().show_all ();

	/* XXX: this is a bit unfortunate; it would probably
	   be nicer if we could just call show () above rather
	   than needing the show_all ()
	*/

	/* re-hide stuff if necessary */
	parameter_changed ("show-summary");
	parameter_changed ("show-group-tabs");
	parameter_changed ("show-zoom-tools");

	/* now reset all audio_time_axis heights, because widgets might need
	   to be re-hidden
	*/

	TimeAxisView *tv;

	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		tv = (static_cast<TimeAxisView*>(*i));
		tv->reset_height ();
	}

	if (current_mixer_strip) {
		current_mixer_strip->hide_things ();
		current_mixer_strip->parameter_changed ("mixer-element-visibility");
	}

	return win;
}

void
Editor::start_track_drag (TimeAxisView& tav, int y, Gtk::Widget& w, bool can_change_cursor)
{
	RouteTimeAxisView* rtav = dynamic_cast<RouteTimeAxisView*> (&tav);

	/* We do not allow dragging VCA Masters */

	if (!rtav) {
		return;
	}

	track_drag = new TrackDrag (rtav, *_session);
	DEBUG_TRACE (DEBUG::TrackDrag, string_compose ("start track drag with %1\n", track_drag));

	int xo, yo;
	w.translate_coordinates (edit_controls_vbox, 0, y, xo, yo);

	if (can_change_cursor) {
		track_drag->drag_cursor = _cursors->move->gobj();
		track_drag->predrag_cursor = gdk_window_get_cursor (edit_controls_vbox.get_window()->gobj());
		gdk_window_set_cursor (edit_controls_vbox.get_toplevel()->get_window()->gobj(), track_drag->drag_cursor);
		track_drag->have_predrag_cursor = true;
	}

	track_drag->bump_track = nullptr;
	track_drag->previous = yo;
	track_drag->start = yo;
}

void
Editor::mid_track_drag (GdkEventMotion* ev, Gtk::Widget& w)
{
	int xo, yo;
	w.translate_coordinates (edit_controls_vbox, ev->x, ev->y, xo, yo);

	if (track_drag->first_move) {

		/* move threshold */

		if (abs (yo - track_drag->previous) < (int) (4 * UIConfiguration::instance().get_ui_scale())) {
			return;
		}

		if (!track_drag->track->selected()) {
			set_selected_track (*track_drag->track, SelectionSet, false);
		}

		if (!track_drag->have_predrag_cursor) {
			track_drag->drag_cursor = _cursors->move->gobj();
			track_drag->predrag_cursor = gdk_window_get_cursor (edit_controls_vbox.get_window()->gobj());
			gdk_window_set_cursor (edit_controls_vbox.get_toplevel()->get_window()->gobj(), track_drag->drag_cursor);
			track_drag->have_predrag_cursor = true;
		}

		track_drag->first_move = false;
	}

	track_drag->current = yo;

	if (track_drag->current > track_drag->previous) {
		if (track_drag->direction != 1) {
			track_drag->bump_track = nullptr;
			track_drag->direction = 1;
		}
	} else if (track_drag->current < track_drag->previous) {
		if (track_drag->direction != -1) {
			track_drag->bump_track = nullptr;
			track_drag->direction = -1;
		}
	}

	if (track_drag->current == track_drag->previous) {
		return;
	}

	redisplay_track_views ();
	track_drag->previous = yo;
}

void
Editor::end_track_drag ()
{
	if (!track_drag) {
		return;
	}

	if (track_drag->have_predrag_cursor) {
		gdk_window_set_cursor (edit_controls_vbox.get_toplevel()->get_window()->gobj(), track_drag->predrag_cursor);
	}

	DEBUG_TRACE (DEBUG::TrackDrag, string_compose ("ending track drag with %1\n", track_drag));
	delete track_drag;
	track_drag = nullptr;
}

bool
Editor::track_dragging() const
{
	return (bool) track_drag;
}

void
Editor::snap_to_internal (timepos_t& start, Temporal::RoundMode direction, SnapPref pref, bool ensure_snap) const
{
	UIConfiguration const& uic (UIConfiguration::instance ());
	const timepos_t presnap = start;


	timepos_t test = timepos_t::max (start.time_domain()); // for each snap, we'll use this value
	timepos_t dist = timepos_t::max (start.time_domain()); // this records the distance of the best snap result we've found so far
	timepos_t best = timepos_t::max (start.time_domain()); // this records the best snap-result we've found so far

	/* check Grid */
	if ( (grid_type() != GridTypeNone) && (uic.get_snap_target () != SnapTargetOther) ) {
		timepos_t pre (presnap);
		timepos_t post (snap_to_grid (pre, direction, pref));
		check_best_snap (presnap, post, dist, best);
		if (uic.get_snap_target () == SnapTargetGrid) {
			goto check_distance;
		}
	}

	/* check snap-to-marker */
	if ((pref == SnapToAny_Visual) && uic.get_snap_to_marks ()) {
		test = snap_to_marker (presnap, direction);
		check_best_snap (presnap, test, dist, best);
	}

	/* check snap-to-playhead */
	if ((pref == SnapToAny_Visual) && uic.get_snap_to_playhead () && !_session->transport_rolling ()) {
		test = timepos_t (_session->audible_sample());
		check_best_snap (presnap, test, dist, best);
	}

	/* check snap-to-region-{start/end/sync} */
	if ((pref == SnapToAny_Visual) && (uic.get_snap_to_region_start () || uic.get_snap_to_region_end () || uic.get_snap_to_region_sync ())) {

		if (!region_boundary_cache.empty ()) {

			auto prev = region_boundary_cache.begin ();
			auto next = std::upper_bound (region_boundary_cache.begin (), region_boundary_cache.end (), presnap);
			if (next != region_boundary_cache.begin ()) {
				prev = next;
				prev--;
			}
			if (next == region_boundary_cache.end ()) {
				next--;
			}

			if ((direction == Temporal::RoundUpMaybe || direction == Temporal::RoundUpAlways)) {
				test = *next;
			} else if ((direction == Temporal::RoundDownMaybe || direction == Temporal::RoundDownAlways)) {
				test = *prev;
			} else if (direction ==  0) {
				if ((*prev).distance (presnap) < presnap.distance (*next)) {
					test = *prev;
				} else {
					test = *next;
				}
			}

		}

		check_best_snap (presnap, test, dist, best);
	}

  check_distance:

	if (timepos_t::max (start.time_domain()) == best) {
		return;
	}

	/* now check "magnetic" state: is the grid within reasonable on-screen distance to trigger a snap?
	 * this also helps to avoid snapping to somewhere the user can't see.  (i.e.: I clicked on a region and it disappeared!!)
	 * ToDo: Perhaps this should only occur if EditPointMouse?
	 */
	samplecnt_t snap_threshold_s = pixel_to_sample (uic.get_snap_threshold ());

	if (!ensure_snap && ::llabs (best.distance (presnap).samples()) > snap_threshold_s) {
		return;
	}

	start = best;
}

ArdourCanvas::Duple
Editor::upper_left() const
{
	return get_trackview_group ()->canvas_origin ();
}

