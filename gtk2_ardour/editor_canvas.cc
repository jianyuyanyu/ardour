/*
 * Copyright (C) 2005-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2019 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015-2017 Nick Mainsbridge <mainsbridge@gmail.com>
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

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

#include "gtkmm2ext/utils.h"

#include "ardour/profile.h"
#include "ardour/rc_configuration.h"
#include "ardour/smf_source.h"

#include "pbd/error.h"

#include "canvas/arc.h"
#include "canvas/canvas.h"
#include "canvas/rectangle.h"
#include "canvas/pixbuf.h"
#include "canvas/scroll_group.h"
#include "canvas/text.h"
#include "canvas/debug.h"

#include "ardour_ui.h"
#include "automation_time_axis.h"
#include "control_point.h"
#include "editor.h"
#include "editing.h"
#include "rgb_macros.h"
#include "utils.h"
#include "audio_time_axis.h"
#include "editor_drag.h"
#include "region_view.h"
#include "editor_group_tabs.h"
#include "editor_section_box.h"
#include "editor_summary.h"
#include "video_timeline.h"
#include "keyboard.h"
#include "editor_cursors.h"
#include "mouse_cursors.h"
#include "note_base.h"
#include "region_peak_cursor.h"
#include "ui_config.h"
#include "verbose_cursor.h"

#include "pbd/i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace PBD;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace Editing;

void
Editor::initialize_canvas ()
{
	_track_canvas_viewport = new ArdourCanvas::GtkCanvasViewport (horizontal_adjustment, vertical_adjustment);
	_track_canvas = _track_canvas_viewport->canvas ();

	_track_canvas->set_background_color (UIConfiguration::instance().color ("arrange base"));
	_track_canvas->use_nsglview (UIConfiguration::instance().get_nsgl_view_mode () == NSGLHiRes);
#ifdef __APPLE__
	// as of april 12 2024 on X Window and Windows, setting this to false
	// causes redraw errors, but not on macOS as far as we can tell
	_track_canvas->set_single_exposure (false);
#endif

	/* scroll group for items that should not automatically scroll
	 *  (e.g verbose cursor). It shares the canvas coordinate space.
	*/
	no_scroll_group = new ArdourCanvas::Container (_track_canvas->root());

	_verbose_cursor = new VerboseCursor (*this);

	ArdourCanvas::ScrollGroup* hsg;
	ArdourCanvas::ScrollGroup* hg;
	ArdourCanvas::ScrollGroup* cg;

	h_scroll_group = hg = new ArdourCanvas::ScrollGroup (_track_canvas->root(), ArdourCanvas::ScrollGroup::ScrollsHorizontally);
	CANVAS_DEBUG_NAME (h_scroll_group, "canvas h scroll");
	_track_canvas->add_scroller (*hg);

	hv_scroll_group = hsg = new ArdourCanvas::ScrollGroup (_track_canvas->root(),
							       ArdourCanvas::ScrollGroup::ScrollSensitivity (ArdourCanvas::ScrollGroup::ScrollsVertically|
													     ArdourCanvas::ScrollGroup::ScrollsHorizontally));
	CANVAS_DEBUG_NAME (hv_scroll_group, "canvas hv scroll");
	_track_canvas->add_scroller (*hsg);

	cursor_scroll_group = cg = new ArdourCanvas::ScrollGroup (_track_canvas->root(), ArdourCanvas::ScrollGroup::ScrollsHorizontally);
	CANVAS_DEBUG_NAME (cursor_scroll_group, "canvas cursor scroll");
	_track_canvas->add_scroller (*cg);

	_region_peak_cursor = new RegionPeakCursor (get_noscroll_group ());

	/*a group to hold global rects like punch/loop indicators */
	global_rect_group = new ArdourCanvas::Container (hv_scroll_group);
	CANVAS_DEBUG_NAME (global_rect_group, "global rect group");

        transport_loop_range_rect = new ArdourCanvas::Rectangle (global_rect_group, ArdourCanvas::Rect (0.0, 0.0, 0.0, ArdourCanvas::COORD_MAX));
	CANVAS_DEBUG_NAME (transport_loop_range_rect, "loop rect");
	transport_loop_range_rect->hide();

	transport_punch_range_rect = new ArdourCanvas::Rectangle (global_rect_group, ArdourCanvas::Rect (0.0, 0.0, 0.0, ArdourCanvas::COORD_MAX));
	CANVAS_DEBUG_NAME (transport_punch_range_rect, "punch rect");
	transport_punch_range_rect->hide();

	/*a group to hold time (measure) lines */
	time_line_group = new ArdourCanvas::Container (h_scroll_group);
	CANVAS_DEBUG_NAME (time_line_group, "time line group");

	_trackview_group = new ArdourCanvas::Container (hv_scroll_group);
	CANVAS_DEBUG_NAME (_trackview_group, "Canvas TrackViews");

	// used as rubberband rect
	rubberband_rect = new ArdourCanvas::Rectangle (hv_scroll_group, ArdourCanvas::Rect (0.0, 0.0, 0.0, 0.0));
	rubberband_rect->hide();

	/* a group to hold stuff while it gets dragged around. Must be the
	 * uppermost (last) group with hv_scroll_group as a parent
	 */
	_drag_motion_group = new ArdourCanvas::Container (hv_scroll_group);
	CANVAS_DEBUG_NAME (_drag_motion_group, "Canvas Drag Motion");

	/* TIME BAR CANVAS */

	_time_markers_group = new ArdourCanvas::Container (h_scroll_group);
	CANVAS_DEBUG_NAME (_time_markers_group, "time bars");

	/* Note that because of ascending-y-axis coordinates, this order is
	 * bottom-to-top. But further note that the actual order is set in
	 * ::update_ruler_visibility()
	 */

	/* the video ruler is temporarily placed a the same location as the
	   previous marker group, but is moved later.
	*/
	videotl_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple(0.0, 0.0));
	CANVAS_DEBUG_NAME (videotl_group, "videotl group");
	marker_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple (0.0, timebar_height + 1.0));
	CANVAS_DEBUG_NAME (marker_group, "marker group");
	range_marker_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple (0.0, (timebar_height * 3.0) + 1.0));
	CANVAS_DEBUG_NAME (range_marker_group, "range marker group");
	tempo_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple (0.0, (timebar_height * 4.0) + 1.0));
	CANVAS_DEBUG_NAME (tempo_group, "tempo group");
	section_marker_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple (0.0, (timebar_height * 5.0) + 1.0));
	CANVAS_DEBUG_NAME (section_marker_group, "Arranger marker group");
	meter_group = new ArdourCanvas::Container (_time_markers_group, ArdourCanvas::Duple (0.0, (timebar_height * 5.0) + 1.0));
	CANVAS_DEBUG_NAME (meter_group, "meter group");

	meter_bar = new ArdourCanvas::Rectangle (meter_group, ArdourCanvas::Rect (0.0, 0., ArdourCanvas::COORD_MAX, timebar_height));
	CANVAS_DEBUG_NAME (meter_bar, "meter Bar");
	meter_bar->set_outline(false);

	tempo_bar = new ArdourCanvas::Rectangle (tempo_group, ArdourCanvas::Rect (0.0, 0.0, ArdourCanvas::COORD_MAX, timebar_height));
	CANVAS_DEBUG_NAME (tempo_bar, "Tempo Bar");
	tempo_bar->set_fill(true);
	tempo_bar->set_outline(false);
	tempo_bar->set_outline_what(ArdourCanvas::Rectangle::BOTTOM);

	range_marker_bar = new ArdourCanvas::Rectangle (range_marker_group, ArdourCanvas::Rect (0.0, 0, ArdourCanvas::COORD_MAX, timebar_height));
	range_marker_bar->set_outline_what(ArdourCanvas::Rectangle::BOTTOM);
	CANVAS_DEBUG_NAME (range_marker_bar, "Range Marker Bar");

	marker_bar = new ArdourCanvas::Rectangle (marker_group, ArdourCanvas::Rect (0.0, 0, ArdourCanvas::COORD_MAX, timebar_height));
	marker_bar->set_outline_what(ArdourCanvas::Rectangle::BOTTOM);
	CANVAS_DEBUG_NAME (marker_bar, "Marker Bar");

	section_marker_bar = new ArdourCanvas::Rectangle (section_marker_group, ArdourCanvas::Rect (0.0, 0, ArdourCanvas::COORD_MAX, timebar_height));
	section_marker_bar->set_outline_what(ArdourCanvas::Rectangle::BOTTOM);
	CANVAS_DEBUG_NAME (section_marker_bar, "Arranger Marker Bar");

	ruler_separator = new ArdourCanvas::Line(_time_markers_group);
	CANVAS_DEBUG_NAME (ruler_separator, "separator between ruler and main canvas");
	ruler_separator->set (ArdourCanvas::Duple(0.0, 0.0), ArdourCanvas::Duple(ArdourCanvas::COORD_MAX, 0.0));
	ruler_separator->set_outline_color(Gtkmm2ext::rgba_to_color (0, 0, 0, 1.0));
	ruler_separator->set_outline_width(1.0);
	ruler_separator->show();

	ARDOUR_UI::instance()->video_timeline = new VideoTimeLine(this, videotl_group, (timebar_height * videotl_bar_height));

	range_bar_drag_rect = new ArdourCanvas::Rectangle (range_marker_group, ArdourCanvas::Rect (0.0, 0.0, 100, timebar_height));
	CANVAS_DEBUG_NAME (range_bar_drag_rect, "range drag");
	range_bar_drag_rect->set_outline (false);
	range_bar_drag_rect->hide ();

	transport_punchin_line = new ArdourCanvas::Line (hv_scroll_group);
	transport_punchin_line->set_x0 (0);
	transport_punchin_line->set_y0 (0);
	transport_punchin_line->set_x1 (0);
	transport_punchin_line->set_y1 (ArdourCanvas::COORD_MAX);
	transport_punchin_line->hide ();

	transport_punchout_line  = new ArdourCanvas::Line (hv_scroll_group);
	transport_punchout_line->set_x0 (0);
	transport_punchout_line->set_y0 (0);
	transport_punchout_line->set_x1 (0);
	transport_punchout_line->set_y1 (ArdourCanvas::COORD_MAX);
	transport_punchout_line->hide();

	tempo_bar->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_ruler_bar_event), tempo_bar, TempoBarItem, "tempo bar"));
	meter_bar->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_ruler_bar_event), meter_bar, MeterBarItem, "meter bar"));
	marker_bar->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_ruler_bar_event), marker_bar, MarkerBarItem, "marker bar"));
	section_marker_bar->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_ruler_bar_event), section_marker_bar, SectionMarkerBarItem, "arrangement marker bar"));
	videotl_group->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_videotl_bar_event), videotl_group));
	range_marker_bar->Event.connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_ruler_bar_event), range_marker_bar, RangeMarkerBarItem, "range marker bar"));

	_playhead_cursor = new EditorCursor (*this, &EditingContext::canvas_playhead_cursor_event, X_("playhead"));
	_playhead_cursor->set_sensitive (UIConfiguration::instance().get_sensitize_playhead());

	_snapped_cursor = new EditorCursor (*this, X_("snapped"));

	_canvas_drop_zone = new ArdourCanvas::Rectangle (hv_scroll_group, ArdourCanvas::Rect (0.0, 0.0, ArdourCanvas::COORD_MAX, 0.0));
	/* this thing is transparent */
	_canvas_drop_zone->set_fill (false);
	_canvas_drop_zone->set_outline (false);
	_canvas_drop_zone->Event.connect (sigc::mem_fun (*this, &Editor::canvas_drop_zone_event));

	_canvas_grid_zone = new ArdourCanvas::Rectangle (hv_scroll_group, ArdourCanvas::Rect (0.0, 0.0, ArdourCanvas::COORD_MAX, ArdourCanvas::COORD_MAX));
	/* this thing is transparent */
	_canvas_grid_zone->set_fill (false);
	_canvas_grid_zone->set_outline (false);
	_canvas_grid_zone->Event.connect (sigc::mem_fun (*this, &Editor::canvas_grid_zone_event));
	_canvas_grid_zone->set_ignore_events (true);

	/* and now the timeline-selection rectangle which is controlled by the markers */
	_section_box = new SectionBox (*this, cursor_scroll_group);
	_section_box->Event.connect (sigc::mem_fun (*this, &Editor::canvas_section_box_event));

	/* group above rulers, to show selection triangles */
	_selection_marker_group = new ArdourCanvas::Container (cursor_scroll_group);
	CANVAS_DEBUG_NAME (_selection_marker_group, "Canvas Selection Ruler");
	_selection_marker->start = new SelectionMarker (*this, *_selection_marker_group, "selection", ArdourMarker::SelectionStart);
	_selection_marker->end = new SelectionMarker (*this, *_selection_marker_group, "selection", ArdourMarker::SelectionEnd);
	_selection_marker_group->raise_to_top ();

	/* these signals will initially be delivered to the canvas itself, but if they end up remaining unhandled,
	 * they are passed to Editor-level handlers.
	 */

	_track_canvas->signal_scroll_event().connect (sigc::bind (sigc::mem_fun (*this, &Editor::canvas_scroll_event), true));
	_track_canvas->signal_motion_notify_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_motion_notify_event));
	_track_canvas->signal_button_press_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_button_press_event));
	_track_canvas->signal_button_release_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_button_release_event));
	_track_canvas->signal_drag_motion().connect (sigc::mem_fun (*this, &Editor::track_canvas_drag_motion));
	_track_canvas->signal_key_press_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_key_press));
	_track_canvas->signal_key_release_event().connect (sigc::mem_fun (*this, &Editor::track_canvas_key_release));

	_track_canvas->set_name ("EditorMainCanvas");
	_track_canvas->add_events (Gdk::POINTER_MOTION_HINT_MASK | Gdk::SCROLL_MASK | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK);
	_track_canvas->signal_leave_notify_event().connect (sigc::mem_fun(*this, &Editor::left_track_canvas), false);
	_track_canvas->signal_enter_notify_event().connect (sigc::mem_fun(*this, &Editor::entered_track_canvas), false);
	_track_canvas->set_can_focus ();

	_track_canvas->PreRender.connect (sigc::mem_fun(*this, &EditingContext::pre_render));

	/* set up drag-n-drop */

	vector<TargetEntry> target_table;

	target_table.push_back (TargetEntry ("x-ardour/region.pbdid", TARGET_SAME_APP));
	target_table.push_back (TargetEntry ("text/uri-list"));
	target_table.push_back (TargetEntry ("text/plain"));
	target_table.push_back (TargetEntry ("application/x-rootwin-drop"));

	_track_canvas->drag_dest_set (target_table);
	_track_canvas->signal_drag_data_received().connect (sigc::mem_fun(*this, &Editor::track_canvas_drag_data_received));

	_track_canvas_viewport->signal_size_allocate().connect (sigc::mem_fun(*this, &Editor::track_canvas_viewport_allocate));

	initialize_rulers ();

	UIConfiguration::instance().ColorsChanged.connect (sigc::mem_fun (*this, &Editor::color_handler));
	UIConfiguration::instance().DPIReset.connect (sigc::mem_fun (*this, &Editor::dpi_reset));
	color_handler();
}

void
Editor::track_canvas_viewport_allocate (Gtk::Allocation alloc)
{
	_canvas_viewport_allocation = alloc;
	track_canvas_viewport_size_allocated ();
}

void
Editor::track_canvas_viewport_size_allocated ()
{
	bool height_changed = _visible_canvas_height != _canvas_viewport_allocation.get_height();

	_visible_canvas_width  = _canvas_viewport_allocation.get_width ();
	_visible_canvas_height = _canvas_viewport_allocation.get_height ();
	_track_canvas_width = _visible_canvas_width;

	_canvas_drop_zone->set_y1 (_canvas_drop_zone->y0() + (_visible_canvas_height - 20.0));

	// SHOWTRACKS

	if (height_changed) {

		vertical_adjustment.set_page_size (_visible_canvas_height);
		if ((vertical_adjustment.get_value() + _visible_canvas_height) >= vertical_adjustment.get_upper()) {
			/*
			   We're increasing the size of the canvas while the bottom is visible.
			   We scroll down to keep in step with the controls layout.
			*/
			vertical_adjustment.set_value (_full_canvas_height - _visible_canvas_height);
		}

		set_visible_track_count (_visible_track_count);
	}

	update_fixed_rulers();
	update_tempo_based_rulers ();
	redisplay_grid (false);
	redisplay_track_views ();
	_summary->set_overlays_dirty ();
}

void
Editor::reset_controls_layout_width ()
{
	GtkRequisition req = { 0, 0 };
	gint w;

	req = edit_controls_vbox.size_request ();
	w = req.width;

	/* the controls layout has no horizontal scrolling, its visible
	   width is always equal to the total width of its contents.
	*/

	controls_layout.property_width() = w;
	controls_layout.property_width_request() = w;
}

void
Editor::reset_controls_layout_height (int32_t h)
{
	/* ensure that the rect that represents the "bottom" of the canvas
	 * (the drag-n-drop zone) is, in fact, at the bottom.
	 */

	_canvas_drop_zone->set_position (ArdourCanvas::Duple (0, h));

	/* track controls layout must span the full height of "h" (all tracks)
	 * plus the bottom rect.
	 */

	h += _canvas_drop_zone->height ();

	/* set the height of the scrollable area (i.e. the sum of all contained widgets)
	 * for the controls layout. The size request is set elsewhere.
	 */

	controls_layout.property_height() = h;

	_group_tabs->set_extent (h);
	controls_layout.queue_draw ();
}

bool
Editor::track_canvas_map_handler (GdkEventAny* /*ev*/)
{
	set_canvas_cursor (get_canvas_cursor());
	return false;
}

/** This is called when something is dropped onto the track canvas */
void
Editor::track_canvas_drag_data_received (const RefPtr<Gdk::DragContext>& context,
					 int x, int y,
					 const SelectionData& data,
					 guint info, guint time)
{
	if (!ARDOUR_UI_UTILS::engine_is_running ()) {
		return;
	}
	if (data.get_target() == "x-ardour/region.pbdid") {
		drop_regions (context, x, y, data, info, time);
	} else {
		drop_paths (context, x, y, data, info, time);
	}
}

bool
Editor::idle_drop_paths (vector<string> paths, timepos_t pos, double ypos, bool copy)
{
	drop_paths_part_two (paths, pos, ypos, copy);
	return false;
}

void
Editor::drop_paths_part_two (const vector<string>& paths, timepos_t const & p, double ypos, bool copy)
{
	RouteTimeAxisView* tv;
	timepos_t pos (p);

	/* MIDI files must always be imported, because we consider them
	 * writable. So split paths into two vectors, and follow the import
	 * path on the MIDI part.
	 */

	vector<string> midi_paths;
	vector<string> audio_paths;

	for (vector<string>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
		if (SMFSource::safe_midi_file_extension (*i)) {
			midi_paths.push_back (*i);
		} else {
			audio_paths.push_back (*i);
		}
	}

	std::pair<TimeAxisView*, int> const tvp = trackview_by_y_position (ypos, false);
	if (tvp.first == 0) {

		/* drop onto canvas background: create new tracks */

		InstrumentSelector is(InstrumentSelector::ForTrackDefault); // instantiation builds instrument-list and sets default.
	        do_import (midi_paths, Editing::ImportDistinctFiles, ImportAsTrack, SrcBest, SMFFileAndTrackName, SMFTempoIgnore, pos, is.selected_instrument());

		if (UIConfiguration::instance().get_only_copy_imported_files() || copy) {
			do_import (audio_paths, Editing::ImportDistinctFiles, Editing::ImportAsTrack,
			           SrcBest, SMFFileAndTrackName, SMFTempoIgnore, pos);
		} else {
			do_embed (audio_paths, Editing::ImportDistinctFiles, ImportAsTrack, pos);
		}

	} else if ((tv = dynamic_cast<RouteTimeAxisView*> (tvp.first)) != 0) {

		/* check that its a track, not a bus */

		if (tv->track()) {
			do_import (midi_paths, Editing::ImportSerializeFiles, ImportToTrack,
				   SrcBest, SMFFileAndTrackName, SMFTempoIgnore, pos, std::shared_ptr<ARDOUR::PluginInfo>(), tv->track ());

			if (UIConfiguration::instance().get_only_copy_imported_files() || copy) {
				do_import (audio_paths, Editing::ImportSerializeFiles, Editing::ImportToTrack,
					   SrcBest, SMFFileAndTrackName, SMFTempoIgnore, pos, std::shared_ptr<PluginInfo>(), tv->track ());
			} else {
				do_embed (audio_paths, Editing::ImportSerializeFiles, ImportToTrack, pos, std::shared_ptr<ARDOUR::PluginInfo>(), tv->track ());
			}
		}
	}
}

void
Editor::drop_paths (const RefPtr<Gdk::DragContext>& context,
		    int x, int y,
		    const SelectionData& data,
		    guint info, guint time)
{
	vector<string> paths;
	GdkEvent ev;
	double cy;

	if (_session && convert_drop_to_paths (paths, data)) {

		/* D-n-D coordinates are window-relative, so convert to canvas coordinates */

		ev.type = GDK_BUTTON_RELEASE;
		ev.button.x = x;
		ev.button.y = y;

		timepos_t when (window_event_sample (&ev, 0, &cy));
		snap_to (when);

		bool copy = ((context->get_actions() & (Gdk::ACTION_COPY | Gdk::ACTION_LINK | Gdk::ACTION_MOVE)) == Gdk::ACTION_COPY);
#ifdef __APPLE__
		/* We are not allowed to call recursive main event loops from within
		   the main event loop with GTK/Quartz. Since import/embed wants
		   to push up a progress dialog, defer all this till we go idle.
		*/
		Glib::signal_idle().connect (sigc::bind (sigc::mem_fun (*this, &Editor::idle_drop_paths), paths, when, cy, copy));
#else
		drop_paths_part_two (paths, when, cy, copy);
#endif
	}

	context->drag_finish (true, false, time);
}

/** @param allow_horiz true to allow horizontal autoscroll, otherwise false.
 *
 *  @param allow_vert true to allow vertical autoscroll, otherwise false.
 *
 */
void
Editor::maybe_autoscroll (bool allow_horiz, bool allow_vert, bool from_headers)
{
	Gtk::Window* toplevel = dynamic_cast<Gtk::Window*>(contents().get_toplevel());

	if (!toplevel) {
		return;
	}

	if (!UIConfiguration::instance().get_autoscroll_editor () || autoscroll_active ()) {
		return;
	}

	/* define a rectangular boundary for scrolling. If the mouse moves
	 * outside of this area and/or continue to be outside of this area,
	 * then we will continuously auto-scroll the canvas in the appropriate
	 * direction(s)
	 *
	 * the boundary is defined in coordinates relative to the toplevel
	 * window since that is what we're going to call ::get_pointer() on
	 * during autoscrolling to determine if we're still outside the
	 * boundary or not.
	 */

	ArdourCanvas::Rect scrolling_boundary;
	Gtk::Allocation alloc;

	if (from_headers) {
		alloc = controls_layout.get_allocation ();

		int wx, wy;

		controls_layout.get_parent()->translate_coordinates (*toplevel,
		                                                     alloc.get_x(), alloc.get_y(),
		                                                     wx, wy);

		scrolling_boundary = ArdourCanvas::Rect (wx, wy, wx + alloc.get_width(), wy + alloc.get_height());


	} else {
		alloc = _track_canvas_viewport->get_allocation ();

		/* reduce height by the height of the timebars, which happens
		   to correspond to the position of the hv_scroll_group.
		*/

		alloc.set_height (alloc.get_height() - hv_scroll_group->position().y);
		alloc.set_y (alloc.get_y() + hv_scroll_group->position().y);

		/* now reduce it again so that we start autoscrolling before we
		 * move off the top or bottom of the canvas
		 */

		alloc.set_height (alloc.get_height() - 20);
		alloc.set_y (alloc.get_y() + 10);

		/* the effective width of the autoscroll boundary so
		   that we start scrolling before we hit the edge.

		   this helps when the window is slammed up against the
		   right edge of the screen, making it hard to scroll
		   effectively.
		*/

		if (alloc.get_width() > 20) {
			alloc.set_width (alloc.get_width() - 20);
			alloc.set_x (alloc.get_x() + 10);
		}

		int wx, wy;

		_track_canvas_viewport->get_parent()->translate_coordinates (*toplevel,
		                                                             alloc.get_x(), alloc.get_y(),
			                                                     wx, wy);

		scrolling_boundary = ArdourCanvas::Rect (wx, wy, wx + alloc.get_width(), wy + alloc.get_height());
	}

	int x, y;
	Gdk::ModifierType mask;

	toplevel->get_window()->get_pointer (x, y, mask);

	if ((allow_horiz && ((x < scrolling_boundary.x0 && _leftmost_sample > 0) || x >= scrolling_boundary.x1)) ||
	    (allow_vert && ((y < scrolling_boundary.y0 && vertical_adjustment.get_value() > 0)|| y >= scrolling_boundary.y1))) {
		start_canvas_autoscroll (allow_horiz, allow_vert, scrolling_boundary);
	}
}

bool
Editor::autoscroll_active () const
{
	return autoscroll_connection.connected ();
}

std::pair <timepos_t,timepos_t>
Editor::session_gui_extents (bool use_extra) const
{
	if (!_session) {
		return std::make_pair (timepos_t::max (Temporal::AudioTime), timepos_t (Temporal::AudioTime));
	}

	timepos_t session_extent_start (_session->current_start_sample());
	timepos_t session_extent_end (_session->current_end_sample());

	/* calculate the extents of all regions in every playlist
	 * NOTE: we should listen to playlists, and cache these values so we don't calculate them every time.
	 */
	{
		std::shared_ptr<RouteList const> rl = _session->get_routes();
		for (auto const& r : *rl) {
			std::shared_ptr<Track> tr = std::dynamic_pointer_cast<Track> (r);

			if (!tr) {
				continue;
			}
			if (tr->presentation_info ().hidden ()) {
				continue;
			}
			pair<timepos_t, timepos_t> e = tr->playlist()->get_extent ();
			if (e.first == e.second) {
				/* no regions present */
				continue;
			}
			session_extent_start = std::min (session_extent_start, e.first);
			session_extent_end   = std::max (session_extent_end, e.second);
		}
	}

	/* ToDo: also incorporate automation regions (in case the session has no audio/midi but is just used for automating plugins or the like) */

	/* add additional time to the ui extents (user-defined in config) */
	if (use_extra) {
		timecnt_t const extra ((samplepos_t) (UIConfiguration::instance().get_extra_ui_extents_time() * 60 * _session->nominal_sample_rate()));
		session_extent_end += timepos_t (extra);
		session_extent_start.shift_earlier (extra);
	}

	/* range-check */
	if (session_extent_end >= timepos_t::max (Temporal::AudioTime)) {
		session_extent_end = timepos_t::max (Temporal::AudioTime);
	}
	if (session_extent_start.is_negative()) {
		session_extent_start = timepos_t (0);
	}

	return std::make_pair (session_extent_start, session_extent_end);
}

bool
Editor::autoscroll_canvas ()
{
	int x, y;
	Gdk::ModifierType mask;
	sampleoffset_t dx = 0;
	bool no_stop = false;
	Gtk::Window* toplevel = dynamic_cast<Gtk::Window*>(contents().get_toplevel());

	if (!toplevel) {
		return false;
	}

	toplevel->get_window()->get_pointer (x, y, mask);

	VisualChange vc;
	bool vertical_motion = false;

	if (autoscroll_horizontal_allowed) {

		samplepos_t new_sample = _leftmost_sample;

		/* horizontal */

		if (x > autoscroll_boundary.x1) {

			/* bring it back into view */
			dx = x - autoscroll_boundary.x1;
			dx += 10 + (2 * (autoscroll_cnt/2));

			dx = pixel_to_sample (dx);

			dx *= UIConfiguration::instance().get_draggable_playhead_speed();

			if (_leftmost_sample < max_samplepos - dx) {
				new_sample = _leftmost_sample + dx;
			} else {
				new_sample = max_samplepos;
			}

			no_stop = true;

		} else if (x < autoscroll_boundary.x0) {

			dx = autoscroll_boundary.x0 - x;
			dx += 10 + (2 * (autoscroll_cnt/2));

			dx = pixel_to_sample (dx);

			dx *= UIConfiguration::instance().get_draggable_playhead_speed();

			if (_leftmost_sample >= dx) {
				new_sample = _leftmost_sample - dx;
			} else {
				new_sample = 0;
			}

			no_stop = true;
		}

		if (new_sample != _leftmost_sample) {
			vc.time_origin = new_sample;
			vc.add (VisualChange::TimeOrigin);
		}
	}

	if (autoscroll_vertical_allowed) {

		// const double vertical_pos = vertical_adjustment.get_value();
		const int speed_factor = 10;

		/* vertical */

		if (y < autoscroll_boundary.y0) {

			/* scroll to make higher tracks visible */

			if (autoscroll_cnt && (autoscroll_cnt % speed_factor == 0)) {
				scroll_up_one_track ();
				vertical_motion = true;
			}
			no_stop = true;

		} else if (y > autoscroll_boundary.y1) {

			if (autoscroll_cnt && (autoscroll_cnt % speed_factor == 0)) {
				scroll_down_one_track ();
				vertical_motion = true;
			}
			no_stop = true;
		}

	}

	if (vc.pending || vertical_motion) {

		/* change horizontal first */

		if (vc.pending) {
			visual_changer (vc);
		}

		/* now send a motion event to notify anyone who cares
		   that we have moved to a new location (because we scrolled)
		*/

		GdkEventMotion ev;

		ev.type = GDK_MOTION_NOTIFY;
		ev.state = Gdk::BUTTON1_MASK;

		/* the motion handler expects events in canvas coordinate space */

		/* we asked for the mouse position above (::get_pointer()) via
		 * our own top level window (we being the Editor). Convert into
		 * coordinates within the canvas window.
		 */

		int cx;
		int cy;

		toplevel->translate_coordinates (*_track_canvas, x, y, cx, cy);

		/* clamp x and y to remain within the autoscroll boundary,
		 * which is defined in window coordinates
		 */

		x = min (max ((ArdourCanvas::Coord) cx, autoscroll_boundary.x0), autoscroll_boundary.x1);
		y = min (max ((ArdourCanvas::Coord) cy, autoscroll_boundary.y0), autoscroll_boundary.y1);

		/* now convert from Editor window coordinates to canvas
		 * window coordinates
		 */

		ArdourCanvas::Duple d = _track_canvas->window_to_canvas (ArdourCanvas::Duple (cx, cy));
		ev.x = d.x;
		ev.y = d.y;
		ev.state = mask;

		motion_handler (0, (GdkEvent*) &ev, true);

	} else if (no_stop) {

		/* not changing visual state but pointer is outside the scrolling boundary
		 * so we still need to deliver a fake motion event
		 */

		GdkEventMotion ev;

		ev.type = GDK_MOTION_NOTIFY;
		ev.state = Gdk::BUTTON1_MASK;

		/* the motion handler expects events in canvas coordinate space */

		/* first convert from Editor window coordinates to canvas
		 * window coordinates
		 */

		int cx;
		int cy;

		/* clamp x and y to remain within the visible area. except
		 * .. if horizontal scrolling is allowed, always allow us to
		 * move back to zero
		 */

		if (autoscroll_horizontal_allowed) {
			x = min (max ((ArdourCanvas::Coord) x, 0.0), autoscroll_boundary.x1);
		} else {
			x = min (max ((ArdourCanvas::Coord) x, autoscroll_boundary.x0), autoscroll_boundary.x1);
		}
		y = min (max ((ArdourCanvas::Coord) y, autoscroll_boundary.y0), autoscroll_boundary.y1);

		toplevel->translate_coordinates (*_track_canvas_viewport, x, y, cx, cy);

		ArdourCanvas::Duple d = _track_canvas->window_to_canvas (ArdourCanvas::Duple (cx, cy));
		ev.x = d.x;
		ev.y = d.y;
		ev.state = mask;

		motion_handler (0, (GdkEvent*) &ev, true);

	} else {
		stop_canvas_autoscroll ();
		return false;
	}

	autoscroll_cnt++;

	return true; /* call me again */
}

void
Editor::start_canvas_autoscroll (bool allow_horiz, bool allow_vert, const ArdourCanvas::Rect& boundary)
{
	if (!_session) {
		return;
	}

	stop_canvas_autoscroll ();

	autoscroll_horizontal_allowed = allow_horiz;
	autoscroll_vertical_allowed = allow_vert;
	autoscroll_boundary = boundary;

	/* do the first scroll right now
	*/

	autoscroll_canvas ();

	/* scroll again at very very roughly 30FPS */

	autoscroll_connection = Glib::signal_timeout().connect (sigc::mem_fun (*this, &Editor::autoscroll_canvas), 30);
}

void
Editor::stop_canvas_autoscroll ()
{
	autoscroll_connection.disconnect ();
	autoscroll_cnt = 0;
}

bool
Editor::left_track_canvas (GdkEventCrossing* ev)
{
	const bool was_within = within_track_canvas;
	DropDownKeys ();
	within_track_canvas = false;
	set_entered_track (0);
	set_entered_regionview (0);
	reset_canvas_action_sensitivity (false);

	if (was_within) {
		if (ev->detail == GDK_NOTIFY_NONLINEAR ||
		    ev->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL) {
			/* context menu or something similar */
			sensitize_the_right_region_actions (false);
		} else {
			sensitize_the_right_region_actions (true);
		}
	}

	return false;
}

bool
Editor::entered_track_canvas (GdkEventCrossing* ev)
{
	const bool was_within = within_track_canvas;
	within_track_canvas = true;
	reset_canvas_action_sensitivity (true);

	if (!was_within) {

		_track_canvas->grab_focus ();

		if (ev->detail == GDK_NOTIFY_NONLINEAR ||
		    ev->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL) {
			/* context menu or something similar */
			sensitize_the_right_region_actions (false);
		} else {
			sensitize_the_right_region_actions (true);
		}
	}

	return false;
}

void
Editor::ensure_time_axis_view_is_visible (TimeAxisView const & track, bool at_top)
{
	if (track.hidden()) {
		return;
	}

	/* apply any pending [height] changes */
	(void) process_redisplay_track_views ();

	/* compute visible area of trackview group, as offsets from top of
	 * trackview group.
	 */

	double const current_view_min_y = vertical_adjustment.get_value();
	double const current_view_max_y = current_view_min_y + vertical_adjustment.get_page_size();

	double const track_min_y = track.y_position ();
	double const track_max_y = track.y_position () + track.effective_height ();

	if (!at_top &&
	    (track_min_y >= current_view_min_y &&
	     track_max_y < current_view_max_y)) {
		/* already visible, and caller did not ask to place it at the
		 * top of the track canvas
		 */
		return;
	}

	double new_value;

	if (at_top) {
		new_value = track_min_y;
	} else {
		if (track_min_y < current_view_min_y) {
			// Track is above the current view
			new_value = track_min_y;
		} else if (track_max_y > current_view_max_y) {
			// Track is below the current view
			new_value = track.y_position () + track.effective_height() - vertical_adjustment.get_page_size();
		} else {
			new_value = track_min_y;
		}
	}

	vertical_adjustment.set_value(new_value);
}

/** Called when the main vertical_adjustment has changed */
void
Editor::tie_vertical_scrolling ()
{
	if (pending_visual_change.idle_handler_id < 0) {
		_region_peak_cursor->hide ();
		_summary->set_overlays_dirty ();
	}
	_group_tabs->set_offset (vertical_adjustment.get_value ());
	controls_layout.queue_draw ();
}

void
Editor::color_handler()
{
	Gtkmm2ext::Color base = UIConfiguration::instance().color ("ruler base");
	Gtkmm2ext::Color text = UIConfiguration::instance().color ("ruler text");
	timecode_ruler->set_fill_color (base);
	timecode_ruler->set_outline_color (text);
	minsec_ruler->set_fill_color (base);
	minsec_ruler->set_outline_color (text);
	samples_ruler->set_fill_color (base);
	samples_ruler->set_outline_color (text);
	bbt_ruler->set_fill_color (base);
	bbt_ruler->set_outline_color (text);

	_section_box->set_fill_color (UIConfiguration::instance().color_mod ("selection", "selection rect"));
	_section_box->set_outline_color (UIConfiguration::instance().color ("selection"));

	_playhead_cursor->set_color (UIConfiguration::instance().color ("play head"));

	meter_bar->set_fill_color (UIConfiguration::instance().color_mod ("meter bar", "marker bar"));
	meter_bar->set_outline_color (UIConfiguration::instance().color ("marker bar separator"));

	tempo_bar->set_fill_color (UIConfiguration::instance().color_mod ("tempo bar", "marker bar"));

	marker_bar->set_fill_color (UIConfiguration::instance().color_mod ("marker bar", "marker bar"));
	marker_bar->set_outline_color (UIConfiguration::instance().color ("marker bar separator"));

	section_marker_bar->set_fill_color (UIConfiguration::instance().color_mod ("arrangement marker bar", "marker bar"));
	section_marker_bar->set_outline_color (UIConfiguration::instance().color ("marker bar separator"));

	range_marker_bar->set_fill_color (UIConfiguration::instance().color_mod ("range marker bar", "marker bar"));
	range_marker_bar->set_outline_color (UIConfiguration::instance().color ("marker bar separator"));

	range_bar_drag_rect->set_fill_color (UIConfiguration::instance().color ("range drag bar rect"));
	range_bar_drag_rect->set_outline_color (UIConfiguration::instance().color ("range drag bar rect"));

	transport_loop_range_rect->set_fill_color (UIConfiguration::instance().color_mod ("transport loop rect", "loop rectangle"));
	transport_loop_range_rect->set_outline_color (UIConfiguration::instance().color ("transport loop rect"));

	transport_punch_range_rect->set_fill_color (UIConfiguration::instance().color ("transport punch rect"));
	transport_punch_range_rect->set_outline_color (UIConfiguration::instance().color ("transport punch rect"));

	transport_punchin_line->set_outline_color (UIConfiguration::instance().color ("punch line"));
	transport_punchout_line->set_outline_color (UIConfiguration::instance().color ("punch line"));

	rubberband_rect->set_outline_color (UIConfiguration::instance().color ("rubber band rect"));
	rubberband_rect->set_fill_color (UIConfiguration::instance().color_mod ("rubber band rect", "selection rect"));

	refresh_location_display ();
	update_section_rects ();

	NoteBase::set_colors ();

	/* redraw the whole thing */
	_track_canvas->set_background_color (UIConfiguration::instance().color ("arrange base"));
	_track_canvas->queue_draw ();

/*
	redisplay_grid (true);

	if (_session)
	      _session->tempo_map().apply_with_metrics (*this, &Editor::draw_metric_marks); // redraw metric markers
*/
}

ArdourCanvas::GtkCanvasViewport*
Editor::get_canvas_viewport() const
{
	return _track_canvas_viewport;
}

ArdourCanvas::GtkCanvas*
Editor::get_canvas() const
{
	return _track_canvas_viewport->canvas();
}

bool
Editor::track_canvas_key_press (GdkEventKey*)
{
	return false;
}

bool
Editor::track_canvas_key_release (GdkEventKey*)
{
	return false;
}

double
Editor::clamp_verbose_cursor_x (double x)
{
	if (x < 0) {
		x = 0;
	} else {
		x = min (_visible_canvas_width - 200.0, x);
	}
	return x;
}

double
Editor::clamp_verbose_cursor_y (double y)
{
	y = max (0.0, y);
	y = min (_visible_canvas_height - 50, y);
	return y;
}

Gdk::Cursor*
Editor::which_trim_cursor (bool left) const
{
	if (!entered_regionview) {
		return 0;
	}

	Trimmable::CanTrim ct = entered_regionview->region()->can_trim ();

	if (left) {
		if (ct & Trimmable::FrontTrimEarlier) {
			return _cursors->left_side_trim;
		} else {
			return _cursors->left_side_trim_right_only;
		}
	} else {
		if (ct & Trimmable::EndTrimLater) {
			return _cursors->right_side_trim;
		} else {
			return _cursors->right_side_trim_left_only;
		}
	}
}

Gdk::Cursor*
Editor::which_mode_cursor () const
{
	Gdk::Cursor* mode_cursor = MouseCursors::invalid_cursor ();

	switch (current_mouse_mode()) {
	case MouseRange:
		mode_cursor = _cursors->selector;
		break;

	case MouseCut:
		mode_cursor = _cursors->scissors;
		break;

	case MouseGrid:
	case MouseObject:
	case MouseContent:
		/* don't use mode cursor, pick a grabber cursor based on the item */
		break;

	case MouseDraw:
		mode_cursor = _cursors->midi_pencil;
		break;

	case MouseTimeFX:
		mode_cursor = _cursors->time_fx; // just use playhead
		break;
	}

	/* up-down cursor as a cue that automation can be dragged up and down when in join object/range mode */
	if (get_smart_mode()) {

		double x, y;
		get_pointer_position (x, y);

		if (x >= 0 && y >= 0) {

			vector<ArdourCanvas::Item const *> items;

			/* Note how we choose a specific scroll group to get
			 * items from. This could be problematic.
			 */

			hv_scroll_group->add_items_at_point (ArdourCanvas::Duple (x,y), items);

			// first item will be the upper most

			if (!items.empty()) {
				const ArdourCanvas::Item* i = items.front();

				if (i && i->parent() && i->parent()->get_data (X_("timeselection"))) {
					pair<TimeAxisView*, int> tvp = trackview_by_y_position (_last_motion_y);
					if (dynamic_cast<AutomationTimeAxisView*> (tvp.first)) {
						mode_cursor = _cursors->up_down;
					}
				}
			}
		}
	}

	return mode_cursor;
}

Gdk::Cursor*
Editor::which_track_cursor () const
{
	Gdk::Cursor* cursor = MouseCursors::invalid_cursor();

	switch (_join_object_range_state) {
	case JOIN_OBJECT_RANGE_NONE:
	case JOIN_OBJECT_RANGE_OBJECT:
		cursor = _cursors->grabber;
		break;
	case JOIN_OBJECT_RANGE_RANGE:
		cursor = _cursors->selector;
		break;
	}

	return cursor;
}

double
Editor::trackviews_height() const
{
	if (!_trackview_group) {
		return 0;
	}

	return _visible_canvas_height - _trackview_group->canvas_origin().y;
}

Gdk::Cursor*
Editor::which_canvas_cursor(ItemType type) const
{
	Gdk::Cursor* cursor = which_mode_cursor ();
	auto mouse_mode = current_mouse_mode();

	if (mouse_mode == MouseRange) {
		switch (type) {
		case StartSelectionTrimItem:
			cursor = _cursors->left_side_trim;
			break;
		case EndSelectionTrimItem:
			cursor = _cursors->right_side_trim;
			break;
		default:
			break;
		}
	}

	if ((mouse_mode == MouseObject || get_smart_mode ()) ||
	    mouse_mode == MouseContent) {

		/* find correct cursor to use in object/smart mode */
		switch (type) {
		case RegionItem:
		case WaveItem:
		case StreamItem:
		case AutomationTrackItem:
			cursor = which_track_cursor ();
			break;
		case PlayheadCursorItem:
			cursor = _cursors->grabber;
			break;
		case SelectionItem:
			cursor = _cursors->selector;
			break;
		case ControlPointItem:
			cursor = _cursors->fader;
			break;
		case GainLineItem:
			cursor = _cursors->cross_hair;
			break;
		case EditorAutomationLineItem:
			cursor = _cursors->cross_hair;
			break;
		case StartSelectionTrimItem:
			cursor = _cursors->left_side_trim;
			break;
		case EndSelectionTrimItem:
			cursor = _cursors->right_side_trim;
			break;
		case FadeInItem:
			cursor = _cursors->fade_in;
			break;
		case FadeInHandleItem:
			cursor = _cursors->fade_in;
			break;
		case FadeInTrimHandleItem:
			cursor = _cursors->fade_in;
			break;
		case FadeOutItem:
			cursor = _cursors->fade_out;
			break;
		case FadeOutHandleItem:
			cursor = _cursors->fade_out;
			break;
		case FadeOutTrimHandleItem:
			cursor = _cursors->fade_out;
			break;
		case FeatureLineItem:
			cursor = _cursors->cross_hair;
			break;
		case LeftFrameHandle:
			if (effective_mouse_mode() == MouseObject) {// (smart mode): if the user is in the btm half, show the trim cursor
				cursor = which_trim_cursor (true);
			} else {
				cursor = _cursors->selector; // (smart mode): in the top half, just show the selection (range) cursor
			}
			break;
		case RightFrameHandle:
			if (effective_mouse_mode() == MouseObject) // see above
				cursor = which_trim_cursor (false);
			else
				cursor = _cursors->selector;
			break;
		case RegionViewName:
		case RegionViewNameHighlight:
			/* the trim bar is used for trimming, but we have to determine if we are on the left or right side of the region */
			cursor = MouseCursors::invalid_cursor ();
			if (entered_regionview) {
				samplepos_t where;
				bool in_track_canvas;
				if (mouse_sample (where, in_track_canvas)) {
					samplepos_t start = entered_regionview->region()->first_sample();
					samplepos_t end = entered_regionview->region()->last_sample();
					cursor = which_trim_cursor ((where - start) < (end - where));
				}
			}
			break;
		case StartCrossFadeItem:
			cursor = _cursors->fade_in;
			break;
		case EndCrossFadeItem:
			cursor = _cursors->fade_out;
			break;
		case CrossfadeViewItem:
			cursor = _cursors->cross_hair;
			break;
		case NoteItem:
			cursor = _cursors->grabber_note;
		default:
			break;
		}

	} else if (mouse_mode == MouseDraw) {

		/* ControlPointItem is not really specific to region gain mode
		   but it is the same cursor so don't worry about this for now.
		   The result is that we'll see the fader cursor if we enter
		   non-region-gain-line control points while in MouseDraw
		   mode, even though we can't edit them in this mode.
		*/

		switch (type) {
		case GainLineItem:
		case ControlPointItem:
			cursor = _cursors->fader;
			break;
		case NoteItem:
			cursor = _cursors->grabber_note;
		default:
			break;
		}
	}

	switch (type) {
		/* These items use the timebar cursor at all times */
	case TimecodeRulerItem:
	case MinsecRulerItem:
	case BBTRulerItem:
	case SamplesRulerItem:
		cursor = _cursors->timebar;
		break;

		/* These items use the grabber cursor at all times */
	case MeterMarkerItem:
	case BBTMarkerItem:
	case TempoMarkerItem:
	case MeterBarItem:
	case TempoBarItem:
	case MarkerItem:
	case MarkerBarItem:
	case RangeMarkerBarItem:
	case SectionMarkerBarItem:
	case VideoBarItem:
	case DropZoneItem:
	case GridZoneItem:
	case SelectionMarkerItem:
		cursor = _cursors->grabber;
		break;

	default:
		break;
	}

	return cursor;
}

bool
Editor::enter_handler (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	ControlPoint* cp;
	ArdourMarker * marker;
	MeterMarker* m_marker = 0;
	TempoMarker* t_marker = 0;
	double fraction;
	bool ret = true;
	auto mouse_mode = current_mouse_mode();

	/* by the time we reach here, entered_regionview and entered trackview
	 * will have already been set as appropriate. Things are done this
	 * way because this method isn't passed a pointer to a variable type of
	 * thing that is entered (which may or may not be canvas item).
	 * (e.g. the actual entered regionview)
	 */

	choose_canvas_cursor_on_entry (item_type);

	switch (item_type) {
	case GridZoneItem:
		break;

	case ControlPointItem:
		if (mouse_mode == MouseDraw || mouse_mode == MouseObject || mouse_mode == MouseContent) {
			cp = static_cast<ControlPoint*>(item->get_data ("control_point"));
			cp->show ();

			fraction = 1.0 - (cp->get_y() / cp->line().height());

			_verbose_cursor->set (cp->line().get_verbose_cursor_string (fraction));
			_verbose_cursor->show ();
		}
		break;

	case GainLineItem:
		if (mouse_mode == MouseDraw) {
			ArdourCanvas::Line *line = dynamic_cast<ArdourCanvas::Line *> (item);
			if (line) {
				line->set_outline_color (UIConfiguration::instance().color ("entered gain line"));
			}
		}
		break;

	case EditorAutomationLineItem:
		if (mouse_mode == MouseDraw || mouse_mode == MouseObject) {
			ArdourCanvas::Line *line = dynamic_cast<ArdourCanvas::Line *> (item);
			if (line) {
				line->set_outline_color (UIConfiguration::instance().color ("entered automation line"));
			}
		}
		break;

	case AutomationTrackItem:
		AutomationTimeAxisView* atv;
		if ((atv = static_cast<AutomationTimeAxisView*>(item->get_data ("trackview"))) != 0) {
			clear_entered_track = false;
			set_entered_track (atv);
		}
		break;

	case MarkerItem:
		if ((marker = static_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		entered_marker = marker;
		marker->set_entered (true);
		break;

	case MeterMarkerItem:
		if ((m_marker = static_cast<MeterMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		entered_marker = m_marker;
		/* "music" currently serves as a stand-in for "entered". */
		m_marker->set_color ("meter marker music");
		break;

	case TempoMarkerItem:
		if ((t_marker = static_cast<TempoMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		entered_marker = t_marker;
		/* "music" currently serves as a stand-in for "entered". */
		t_marker->set_color ("tempo marker music");
		break;

	case FadeInHandleItem:
	case FadeInTrimHandleItem:
		if (mouse_mode == MouseObject) {
			ArdourCanvas::Rectangle *rect = dynamic_cast<ArdourCanvas::Rectangle *> (item);
			if (rect) {
				RegionView* rv = static_cast<RegionView*>(item->get_data ("regionview"));
				rect->set_fill_color (rv->get_fill_color());
			}
		}
		break;

	case FadeOutHandleItem:
	case FadeOutTrimHandleItem:
		if (mouse_mode == MouseObject) {
			ArdourCanvas::Rectangle *rect = dynamic_cast<ArdourCanvas::Rectangle *> (item);
			if (rect) {
				RegionView* rv = static_cast<RegionView*>(item->get_data ("regionview"));
				rect->set_fill_color (rv->get_fill_color ());
			}
		}
		break;

	case FeatureLineItem:
	{
		ArdourCanvas::Line *line = dynamic_cast<ArdourCanvas::Line *> (item);
		line->set_outline_color (0xFF0000FF);
	}
	break;

	case SelectionItem:
		break;

	case WaveItem:
	{
		if (entered_regionview) {
			entered_regionview->entered();
		}
	}
	break;

	default:
		break;
	}

	/* third pass to handle entered track status in a comprehensible way.
	 */

	switch (item_type) {
	case GainLineItem:
	case EditorAutomationLineItem:
	case ControlPointItem:
		/* these do not affect the current entered track state */
		clear_entered_track = false;
		break;

	case AutomationTrackItem:
		/* handled above already */
		break;

	default:

		break;
	}

	return ret;
}

bool
Editor::leave_handler (ArdourCanvas::Item* item, GdkEvent*, ItemType item_type)
{
	EditorAutomationLine* al;
	ArdourMarker *marker;
	TempoMarker *t_marker;
	MeterMarker *m_marker;
	bool ret = true;

	switch (item_type) {
	case GridZoneItem:
		break;

	case ControlPointItem:
		_verbose_cursor->hide ();
		break;

	case GainLineItem:
	case EditorAutomationLineItem:
		al = reinterpret_cast<EditorAutomationLine*> (item->get_data ("line"));
		{
			ArdourCanvas::Line *line = dynamic_cast<ArdourCanvas::Line *> (item);
			if (line) {
				line->set_outline_color (al->get_line_color());
			}
		}
		break;

	case MarkerItem:
		if ((marker = static_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		entered_marker = 0;
		marker->set_entered (false);
		break;

	case MeterMarkerItem:
		if ((m_marker = static_cast<MeterMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		m_marker->set_color ("meter marker");
		entered_marker = 0;
		break;

	case TempoMarkerItem:
		if ((t_marker = static_cast<TempoMarker *> (item->get_data ("marker"))) == 0) {
			break;
		}
		t_marker->set_color ("tempo marker");
		entered_marker = 0;
		break;

	case FadeInTrimHandleItem:
	case FadeOutTrimHandleItem:
	case FadeInHandleItem:
	case FadeOutHandleItem:
	{
		ArdourCanvas::Rectangle *rect = dynamic_cast<ArdourCanvas::Rectangle *> (item);
		if (rect) {
			rect->set_fill_color (UIConfiguration::instance().color ("inactive fade handle"));
		}
	}
	break;

	case AutomationTrackItem:
		break;

	case FeatureLineItem:
	{
		ArdourCanvas::Line *line = dynamic_cast<ArdourCanvas::Line *> (item);
		line->set_outline_color (UIConfiguration::instance().color ("zero line"));
	}
	break;

	default:
		_region_peak_cursor->hide ();
		break;
	}

	return ret;
}
