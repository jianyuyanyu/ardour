/*
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2016 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2006 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2013-2014 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2017 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
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

#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <cmath>
#include <set>
#include <string>
#include <algorithm>
#include <bitset>

#include "pbd/error.h"
#include "pbd/enumwriter.h"
#include "pbd/memento_command.h"
#include "pbd/basename.h"
#include "pbd/stateful_diff_command.h"

#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/utils.h"

#include "canvas/canvas.h"

#include "ardour/audioplaylist.h"
#include "ardour/audioregion.h"
#include "ardour/operations.h"
#include "ardour/playlist.h"
#include "ardour/profile.h"
#include "ardour/region_factory.h"
#include "ardour/route.h"
#include "ardour/session.h"
#include "ardour/types.h"

#include "widgets/prompter.h"

#include "actions.h"
#include "ardour_ui.h"
#include "editor.h"
#include "time_axis_view.h"
#include "audio_time_axis.h"
#include "audio_region_view.h"
#include "midi_region_view.h"
#include "midi_view.h"
#include "marker.h"
#include "streamview.h"
#include "region_gain_line.h"
#include "rc_option_editor.h"
#include "automation_time_axis.h"
#include "control_point.h"
#include "selection.h"
#include "keyboard.h"
#include "editing.h"
#include "rgb_macros.h"
#include "control_point_dialog.h"
#include "editor_drag.h"
#include "automation_region_view.h"
#include "edit_note_dialog.h"
#include "mouse_cursors.h"
#include "editor_cursors.h"
#include "region_peak_cursor.h"
#include "velocity_ghost_region.h"
#include "verbose_cursor.h"
#include "note.h"

#include "pbd/i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Editing;
using namespace Temporal;

using Gtkmm2ext::Keyboard;

void
Editor::set_current_trimmable (std::shared_ptr<Trimmable> t)
{
	std::shared_ptr<Trimmable> st = _trimmable.lock();

	if (!st || st == t) {
		_trimmable = t;
	}
}

void
Editor::set_current_movable (std::shared_ptr<Movable> m)
{
	std::shared_ptr<Movable> sm = _movable.lock();

	if (!sm || sm != m) {
		_movable = m;
	}
}

void
Editor::mouse_mode_object_range_toggled()
{
	set_mouse_mode (current_mouse_mode(), true); /* updates set-mouse-mode-range */
}

void
Editor::mouse_mode_chosen (MouseMode m)
{
	if (!mouse_mode_actions[m]->get_active()) {
		/* this was just the notification that the old mode has been
		 * left. we'll get called again with the new mode active in a
		 * jiffy.
		 */
		return;
	}

	const bool was_internal = internal_editing();

	/* Ben ToDo:  once we have a dedicated 'region edit panel', we can store
	 * one snap mode in the editor canvas and another one in the editor,
	 * relieving the complexity here */

	/* Switch snap type/mode if we're moving to/from an internal tool.  Note
	   this must toggle the actions and not call set_snap_*() directly,
	   otherwise things get out of sync and the combo box stops working. */
	if (!UIConfiguration::instance().get_grid_follows_internal()) {
		grid_actions[pre_internal_grid_type]->set_active(true);
		snap_mode_actions[pre_internal_snap_mode]->set_active(true);
	} else if (!was_internal && internal_editing()) {
		grid_actions[internal_grid_type]->set_active(true);
		snap_mode_actions[internal_snap_mode]->set_active(true);
	} else if (was_internal && !internal_editing()) {
		grid_actions[pre_internal_grid_type]->set_active(true);
		snap_mode_actions[pre_internal_snap_mode]->set_active(true);
	}

	instant_save ();

	/* this should generate a new enter event which will
	   trigger the appropriate cursor.
	*/

	if (_track_canvas) {
		_track_canvas->re_enter ();
	}

	set_gain_envelope_visibility ();

	update_time_selection_display ();

	auto mouse_mode = current_mouse_mode ();

	if (mouse_mode == MouseDraw) {
		draw_box.show();
		_draw_box_spacer.show();
	} else {
		draw_box.hide();
		_draw_box_spacer.hide();
	}

	if (mouse_mode == MouseGrid) {
		grid_box.show();
		_grid_box_spacer.show ();
		_canvas_grid_zone->set_ignore_events (false);
	} else {
		grid_box.hide ();
		_grid_box_spacer.hide ();
		_canvas_grid_zone->set_ignore_events (true);
	}

	if (internal_editing()) {

		/* reinstate any existing MIDI note (and by extension, MIDI
		 * region) selection for internal edit mode. This allows a user
		 * to enter/exit/enter this mode without losing a selection of
		 * notes.
		 */

		catch_up_on_midi_selection ();

		/* ensure that the track canvas has focus, so that key events
		   will get directed to the correct place.
		*/
		_track_canvas->grab_focus ();

		/* enable MIDI editing actions, which in turns enables their
		   bindings
		*/
		ActionManager::set_sensitive (_midi_actions, true);

	} else {
		/* undo some of the above actions, since we're not in internal
		   edit mode.
		*/
		ActionManager::set_sensitive (_midi_actions, false);
	}

	if (was_internal && !internal_editing()) {
		/* drop any selected regions so that they in turn
		 * redraw any selected notes. This essentially the
		 * opposite of ::catch_up_on_midi_selection() called
		 * above.
		 */
		get_selection().clear_regions ();
	}

	MouseModeChanged (); /* EMIT SIGNAL */

	if ((was_internal && !internal_editing()) ||
	    (!was_internal && internal_editing())) {
		queue_redisplay_track_views ();
	}
}

bool
Editor::internal_editing() const
{
	auto mouse_mode = current_mouse_mode ();
	return mouse_mode == Editing::MouseContent || mouse_mode == Editing::MouseDraw;
}

void
Editor::update_time_selection_display ()
{
	auto mouse_mode = current_mouse_mode ();
	switch (mouse_mode) {
	case MouseRange:
		selection->clear_objects ();
		selection->clear_midi_notes ();
		break;
	case MouseObject:
		selection->clear_time ();
		selection->clear_midi_notes ();
		break;
	case MouseDraw:
		/* Clear regions, but not time or tracks, since that
		 * would destroy the range selection rectangle, which we need to stick
		 * around for AutomationRangeDrag. */
		selection->clear_regions ();
		selection->clear_playlists ();
		/* .. unless there is no track selection (i.e. arrangement section
		 * selection). In which case time is cleared too.
		 */
		if (selection->tracks.empty ()) {
			selection->clear_time ();
		}
		break;
	case MouseContent:
		/* This handles internal edit.
		   Clear everything except points and notes.
		*/
		selection->clear_regions();
		selection->clear_lines();
		selection->clear_playlists ();

		selection->clear_time ();
		selection->clear_tracks ();
		break;

	case MouseTimeFX:
		/* We probably want to keep region selection */
		selection->clear_points ();
		selection->clear_lines();
		selection->clear_playlists ();

		selection->clear_time ();
		selection->clear_tracks ();
		break;

	case MouseGrid:
	default:
		/*Clear everything */
		selection->clear_objects();
		selection->clear_time ();
		selection->clear_tracks ();
		break;
	}
}

void
Editor::step_mouse_mode (bool next)
{
	const int n_mouse_modes = (int)MouseContent + 1;
	int       current       = (int)current_mouse_mode();
	if (next) {
		set_mouse_mode((MouseMode)((current + 1) % n_mouse_modes));
	} else {
		set_mouse_mode((MouseMode)((current + n_mouse_modes - 1) % n_mouse_modes));
	}
}

void
Editor::button_selection (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	/* in object/audition/timefx/gain-automation mode,
	 * any button press sets the selection if the object
	 * can be selected. this is a bit of hack, because
	 * we want to avoid this if the mouse operation is a
	 * region alignment.
	 *
	 * note: not dbl-click or triple-click
	 *
	 * Also note that there is no region selection in internal edit mode, otherwise
	 * for operations operating on the selection (e.g. cut) it is not obvious whether
	 * to cut notes or regions.
	 */

	MouseMode eff_mouse_mode = effective_mouse_mode ();
	auto mouse_mode = current_mouse_mode ();

	if (eff_mouse_mode == MouseCut) {
		/* never change selection in cut mode */
		return;
	}

	if (get_smart_mode() && eff_mouse_mode == MouseRange && event->button.button == 3 && item_type == RegionItem) {
		/* context clicks are always about object properties, even if
		 * we're in range mode within smart mode.
		 */
		eff_mouse_mode = MouseObject;
	}

	/* special case: allow drag of region fade in/out in object mode with join object/range enabled */
	if (get_smart_mode()) {
		switch (item_type) {
			case FadeInHandleItem:
			case FadeInTrimHandleItem:
			case FadeOutHandleItem:
			case FadeOutTrimHandleItem:
				eff_mouse_mode = MouseObject;
				break;
			default:
				break;
		}
	}

	if (((mouse_mode != MouseObject) &&
	     (mouse_mode != MouseTimeFX || item_type != RegionItem) &&
	     (mouse_mode != MouseDraw) &&
	     (mouse_mode != MouseContent || item_type == RegionItem)) ||
	    ((event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE) || event->button.button > 3)) {
		return;
	}

	if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) {

		if ((event->button.state & Keyboard::RelevantModifierKeyMask) && event->button.button != 1) {

			/* almost no selection action on modified button-2 or button-3 events */

			if ((item_type != RegionItem && event->button.button != 2)
			    /* for selection of control points prior to delete (shift-right click) */
			    && !(item_type == ControlPointItem && event->button.button == 3 && event->type == GDK_BUTTON_PRESS)) {
				return;
			}
		}
	}

	SelectionOperation op = ArdourKeyboard::selection_type (event->button.state);
	bool press = (event->type == GDK_BUTTON_PRESS);

	if (press) {
		_mouse_changed_selection = false;
	}

	switch (item_type) {
		case RegionItem:
			if (eff_mouse_mode == MouseDraw) {
				break;
			}
			if (press) {
				if (eff_mouse_mode != MouseRange) {
					_mouse_changed_selection = set_selected_regionview_from_click (press, op);
				} else {
					/* don't change the selection unless the
					 * clicked track is not currently selected. if
					 * so, "collapse" the selection to just this track
					 */
					if (!selection->selected (clicked_axisview)) {
						set_selected_track_as_side_effect (SelectionSet);
					}
				}
			} else {
				if (eff_mouse_mode != MouseRange) {
					_mouse_changed_selection |= set_selected_regionview_from_click (press, op);
				}
			}
			break;

		case RegionViewNameHighlight:
		case RegionViewName:
		case LeftFrameHandle:
		case RightFrameHandle:
		case FadeInHandleItem:
		case FadeInTrimHandleItem:
		case FadeInItem:
		case FadeOutHandleItem:
		case FadeOutTrimHandleItem:
		case FadeOutItem:
		case StartCrossFadeItem:
		case EndCrossFadeItem:
			if (get_smart_mode() || eff_mouse_mode != MouseRange) {
				_mouse_changed_selection |= set_selected_regionview_from_click (press, op);
			} else if (event->type == GDK_BUTTON_PRESS) {
				set_selected_track_as_side_effect (op);
			}
			break;

		case ControlPointItem:
			/* for object/track exclusivity, we don't call set_selected_track_as_side_effect (op); */

			if (eff_mouse_mode != MouseRange) {
				if (event->button.button != 3) {
					_mouse_changed_selection |= set_selected_control_point_from_click (press, op);
				} else {
					_mouse_changed_selection |= set_selected_control_point_from_click (press, SelectionSet);
				}
			}
			break;

		case GainLineItem:
			if (eff_mouse_mode != MouseRange) {
				EditorAutomationLine* argl = reinterpret_cast<EditorAutomationLine*> (item->get_data ("line"));

				std::list<Selectable*> selectables;
				uint32_t before, after;
				samplecnt_t const  where = (samplecnt_t) floor (event->button.x * samples_per_pixel) - clicked_regionview->region ()->position_sample ();

				if (!argl || !argl->control_points_adjacent (where, before, after)) {
					break;
				}

				selectables.push_back (argl->nth (before));
				selectables.push_back (argl->nth (after));

				switch (op) {
				case SelectionSet:
					if (press) {
						selection->set (selectables);
						_mouse_changed_selection = true;
					}
					break;
				case SelectionAdd:
					if (press) {
						selection->add (selectables);
						_mouse_changed_selection = true;
					}
					break;
				case SelectionToggle:
					if (press) {
						selection->toggle (selectables);
						_mouse_changed_selection = true;
					}
					break;

				case SelectionExtend:
					/* XXX */
					break;
				case SelectionRemove:
					/* XXX */
					break;
				}
			}
			break;

		case EditorAutomationLineItem:
			if (eff_mouse_mode != MouseRange && eff_mouse_mode != MouseDraw) {
				select_automation_line (&event->button, item, op);
			}
			break;

		case StreamItem:
			/* for context click, select track */
			if (event->button.button == 3) {
				selection->clear_tracks ();
				set_selected_track_as_side_effect (op);

				/* We won't get a release.*/
				begin_reversible_selection_op (X_("Button 3 Menu Select"));
				commit_reversible_selection_op ();
			}
			break;

		case AutomationTrackItem:
			if (eff_mouse_mode != MouseDraw && op == SelectionSet) {
				set_selected_track_as_side_effect (op);
			}
			break;

		case NoteItem:
			if (press && event->button.button == 3) {
				NoteBase* cnote = reinterpret_cast<NoteBase*> (item->get_data ("notebase"));
				assert (cnote);
				if (cnote->midi_view().selection_size() == 0 || !cnote->selected()) {
					selection->clear_points();
					cnote->midi_view().unique_select (cnote);
					/* we won't get the release, so store the selection change now */
					begin_reversible_selection_op (X_("Button 3 Note Selection"));
					commit_reversible_selection_op ();
				}
			}
			break;

		default:
			break;
	}

	if ((!press) && _mouse_changed_selection) {
		begin_reversible_selection_op (X_("Button Selection"));
		commit_reversible_selection_op ();
		_mouse_changed_selection = false;
	}
}

bool
Editor::button_press_handler_1 (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	/* single mouse clicks on any of these item types operate
	 * independent of mouse mode, mostly because they are
	 * not on the main track canvas or because we want
	 * them to be modeless.
	 */

	NoteBase* note = NULL;

	switch (item_type) {
	case PlayheadCursorItem:
		_drags->set (new CursorDrag (*this, *_playhead_cursor, true), event);
		return true;

	case MarkerItem:
		if (Keyboard::modifier_state_equals (event->button.state, Keyboard::ModifierMask(Keyboard::PrimaryModifier|Keyboard::TertiaryModifier))) {
			hide_marker (item, event);
		} else {
			ArdourMarker* marker = static_cast<ArdourMarker*> (item->get_data ("marker"));
			if (marker->type() == ArdourMarker::RegionCue) {
				_drags->set (new RegionMarkerDrag (*this, marker->region_view(), item), event);
			} else {
				_drags->set (new MarkerDrag (*this, item), event);
			}
		}
		return true;
	case SceneMarkerItem:
		_drags->set (new MarkerDrag (*this, item), event);
		return true;

	case TempoMarkerItem:
		if (ArdourKeyboard::indicates_constraint (event->button.state)) {
			_drags->set (new TempoEndDrag (*this, item), event);
		} else {
			_drags->set (new TempoMarkerDrag (*this, item), event);
		}

		return true;

	case BBTMarkerItem:
		_drags->set (new BBTMarkerDrag (*this, item), event);
		return true;

	case SelectionMarkerItem:
		_drags->set (new SelectionMarkerDrag (*this, item), event);
		return true;

	case MeterMarkerItem:
		_drags->set (
			new MeterMarkerDrag (*this, item, ArdourKeyboard::indicates_copy (event->button.state)), event);
		return true;

	case VideoBarItem:
		_drags->set (new VideoTimeLineDrag (*this, item), event);
		return true;
		break;

	case TempoBarItem:
	case TempoCurveItem:
		if (!Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)
		    && !ArdourKeyboard::indicates_constraint (event->button.state)) {
			_drags->set (new CursorDrag (*this, *_playhead_cursor, false), event);
		} else if (Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)) {
			_drags->set (new TempoCurveDrag (*this, item), event);
			return true;
		}
		return true;

	case MeterBarItem:
		if (!Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)
		    && !ArdourKeyboard::indicates_constraint (event->button.state)) {
			_drags->set (new CursorDrag (*this, *_playhead_cursor, false), event);
		}
		return true;

	case BBTRulerItem:
		if (!Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)
		    && !ArdourKeyboard::indicates_constraint (event->button.state)) {
			_drags->set (new CursorDrag (*this, *_playhead_cursor, false), event);
		}
		return true;

	case TimecodeRulerItem:
	case SamplesRulerItem:
	case MinsecRulerItem:
	case MarkerBarItem:
	case SectionMarkerBarItem:
		if (!Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)
		    && !ArdourKeyboard::indicates_constraint (event->button.state)) {
			_drags->set (new CursorDrag (*this, *_playhead_cursor, false), event);
		}
		return true;

	case RangeMarkerBarItem:
		if (Keyboard::modifier_state_contains (event->button.state, Keyboard::TertiaryModifier)) {
			_drags->set (new RangeMarkerBarDrag (*this, item, RangeMarkerBarDrag::CreateSkipMarker), event);
		} else if (Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)) {
			_drags->set (new RangeMarkerBarDrag (*this, item, RangeMarkerBarDrag::CreateRangeMarker), event);
		} else {
			_drags->set (new CursorDrag (*this, *_playhead_cursor, false), event);
		}
		return true;
		break;

	case VelocityItem:
		_drags->set (new LollipopDrag (*this, item), event);
		return true;
		break;

	case VelocityBaseItem:
		{
			VelocityDisplay* vd = static_cast<VelocityDisplay*> (item->get_data ("ghostregionview"));
			VelocityGhostRegion* grv = dynamic_cast<VelocityGhostRegion*> (vd);
			if (grv) {
				_drags->set (new VelocityLineDrag (*this, grv->base_item(), true, Temporal::BeatTime), event);
			}
		}
		return true;
		break;

	default:
		break;
	}

	if (_join_object_range_state == JOIN_OBJECT_RANGE_OBJECT) {
		/* special case: allow trim of range selections in joined object mode;
		 * in theory eff should equal MouseRange in this case, but it doesn't
		 * because entering the range selection canvas item results in entered_regionview
		 * being set to 0, so update_join_object_range_location acts as if we aren't
		 * over a region.
		 */
		if (item_type == StartSelectionTrimItem) {
			_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionStartTrim), event);
		} else if (item_type == EndSelectionTrimItem) {
			_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionEndTrim), event);
		}
	}

	Editing::MouseMode eff = effective_mouse_mode ();

	/* special case: allow drag of region fade in/out in object mode with join object/range enabled */
	if (get_smart_mode()) {
		switch (item_type) {
		  case FadeInHandleItem:
		  case FadeInTrimHandleItem:
		  case FadeOutHandleItem:
		  case FadeOutTrimHandleItem:
			eff = MouseObject;
			break;
		default:
			break;
		}
	}

	switch (eff) {
	case MouseRange:
		switch (item_type) {
		case StartSelectionTrimItem:
			_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionStartTrim), event);
			break;

		case EndSelectionTrimItem:
			_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionEndTrim), event);
			break;

		case SelectionItem:
			if (Keyboard::modifier_state_contains (event->button.state, Keyboard::ModifierMask(Keyboard::PrimaryModifier|Keyboard::SecondaryModifier))) {
				start_selection_grab (item, event);
				return true;
			} else if (Keyboard::modifier_state_equals (event->button.state, Keyboard::SecondaryModifier)) {
				/* grab selection for moving */
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionMove), event);
			} else {
				/* this was debated, but decided the more common action was to make a new selection */
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::CreateSelection), event);
			}
			break;

		case StreamItem:
			if (Keyboard::modifier_state_equals (event->button.state, Keyboard::RangeSelectModifier) && !selection->time.empty()) {
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionExtend), event);
			} else {
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::CreateSelection), event);
			}
			return true;
			break;

		case RegionViewNameHighlight:
			if (!clicked_regionview->region()->locked()) {
				_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
				return true;
			}
			break;

		default:
			if (Keyboard::modifier_state_equals (event->button.state, Keyboard::RangeSelectModifier) && !selection->time.empty()) {
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::SelectionExtend), event);
			} else {
				_drags->set (new SelectionDrag (*this, item, SelectionDrag::CreateSelection), event);
			}
		}
		return true;
		break;

	case MouseCut:
		switch (item_type) {
		case RegionItem:
		case FadeInHandleItem:
		case FadeOutHandleItem:
		case LeftFrameHandle:
		case RightFrameHandle:
		case FeatureLineItem:
		case RegionViewNameHighlight:
		case RegionViewName:
		case StreamItem:
		case AutomationTrackItem:
			_drags->set (new RegionCutDrag (*this, item, canvas_event_sample (event)), event, get_canvas_cursor());
			return true;
			break;
		default:
			break;
		}
		break;

	case MouseContent:
		switch (item_type) {
		case NoteItem:
			/* Existing note: allow trimming/motion */
			if ((note = reinterpret_cast<NoteBase*> (item->get_data ("notebase")))) {
				if (note->big_enough_to_trim() && note->mouse_near_ends()) {
					_drags->set (new NoteResizeDrag (*this, item), event, get_canvas_cursor());
				} else {
					_drags->set (new NoteDrag (*this, item), event);
				}
			}
			return true;

		case GainLineItem:
			_drags->set (new LineDrag (*this, item, [&](GdkEvent* ev,timepos_t const & pos, double y) { line_drag_click (ev, pos, y); }), event);
			return true;
			break;

		case ControlPointItem:
			_drags->set (new ControlPointDrag (*this, item), event);
			return true;
			break;

		case EditorAutomationLineItem:
			_drags->set (new LineDrag (*this, item, [&](GdkEvent* ev,timepos_t const & pos, double y) { line_drag_click (ev, pos, y); }), event);
			return true;
			break;

		case StreamItem:
			/* in the past, we created a new midi region here, but perhaps that is best left to the Draw mode */
			/* .. now we allow for rubberband selection (region gain) */
			_drags->set (new RubberbandSelectDrag (*this, item, [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);
			return true;
			break;

		case AutomationTrackItem:
			/* rubberband drag to select automation points */
			_drags->set (new RubberbandSelectDrag (*this, item, [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);
			return true;
			break;

		case RegionItem:
			/* rubberband drag to select region gain points */
			_drags->set (new RubberbandSelectDrag (*this, item, [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);
			return true;
			break;

		default:
			break;
		}
		break;

	case MouseObject:
		if (Keyboard::modifier_state_contains (event->button.state, Keyboard::ModifierMask(Keyboard::PrimaryModifier|Keyboard::SecondaryModifier)) &&
		    event->type == GDK_BUTTON_PRESS) {

			_drags->set (new RubberbandSelectDrag (*this, get_trackview_group(), [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);

		} else if (event->type == GDK_BUTTON_PRESS) {

			switch (item_type) {
			case FadeInHandleItem:
			{
				RegionView* rv = reinterpret_cast<RegionView*> (item->get_data("regionview"));
				assert (rv);
				_drags->set (new FadeInDrag (*this, item, rv, selection->regions, drag_time_domain (rv->region())), event, _cursors->fade_in);
				return true;
			}

			case FadeOutHandleItem:
			{
				RegionView* rv = reinterpret_cast<RegionView*> (item->get_data("regionview"));
				assert (rv);
				_drags->set (new FadeOutDrag (*this, item, rv, selection->regions,  drag_time_domain (rv->region())), event, _cursors->fade_out);
				return true;
			}

			case StartCrossFadeItem:
			case EndCrossFadeItem:
			/* we might allow user to grab inside the fade to trim a region with preserve_fade_anchor.
			 * For not this is not fully implemented */
#if 0
			if (!clicked_regionview->region()->locked()) {
				_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region()), true), event);
				return true;
			}
#endif
				break;

			case FeatureLineItem:
			{
				if (Keyboard::modifier_state_contains (event->button.state, Keyboard::TertiaryModifier)) {
					remove_transient(item);
					return true;
				}

				_drags->set (new FeatureLineDrag (*this, item), event);
				return true;
				break;
			}

			case RegionItem:
				if (dynamic_cast<AutomationRegionView*> (clicked_regionview)) {
					/* click on an automation region view; do nothing here and let the ARV's signal handler
					   sort it out.
					*/
					break;
				}

				/* click on a normal region view */

				if (_drags->active ()) {
					return true;
				}

				if (Keyboard::modifier_state_equals (event->button.state, ArdourKeyboard::slip_contents_modifier ())) {
					if (!clicked_regionview->region()->locked() && (Config->get_edit_mode() != Lock)) {
						_drags->add (new RegionSlipContentsDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())));
					}
				} else if (ArdourKeyboard::indicates_copy (event->button.state)) {
					add_region_drag (item, event, clicked_regionview, true);
				} else if (Keyboard::the_keyboard().key_is_down (GDK_b)) {
					add_region_brush_drag (item, event, clicked_regionview);
				} else {
					add_region_drag (item, event, clicked_regionview, false);
				}


				_drags->start_grab (event);
				return true;
				break;

			case RegionViewNameHighlight:
			case LeftFrameHandle:
			case RightFrameHandle:
				if (!clicked_regionview->region()->locked()) {
					_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region()), false), event);
					return true;
				}
				break;

			case FadeInTrimHandleItem:
			case FadeOutTrimHandleItem:
				if (!clicked_regionview->region()->locked()) {
					_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region()), true), event);
					return true;
				}
				break;

			case RegionViewName:
			{
				/* rename happens on edit clicks */
				if (clicked_regionview->get_name_highlight()) {
					_drags->set (new TrimDrag (*this, clicked_regionview->get_name_highlight(), clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
					return true;
				}
				break;
			}

			case ControlPointItem:
				_drags->set (new ControlPointDrag (*this, item), event);
				return true;
				break;

			case EditorAutomationLineItem:
				_drags->set (new LineDrag (*this, item, [&](GdkEvent* ev,timepos_t const & pos, double y) { line_drag_click (ev, pos, y); }), event);
				return true;
				break;

			case StreamItem:
				_drags->set (new RubberbandSelectDrag (*this, item, [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);
				return true;

			case AutomationTrackItem:
			{
				TimeAxisView* parent = clicked_axisview->get_parent ();
				AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (clicked_axisview);
				assert (atv);
				if (parent && dynamic_cast<MidiTimeAxisView*> (parent) && atv->show_regions ()) {

					RouteTimeAxisView* p = dynamic_cast<RouteTimeAxisView*> (parent);
					assert (p);
					std::shared_ptr<Playlist> pl = p->track()->playlist ();
					if (pl->n_regions() == 0) {
						/* Parent has no regions; create one so that we have somewhere to put automation */
						_drags->set (new RegionCreateDrag (*this, item, parent), event);
					} else {
						/* See if there's a region before the click that we can extend, and extend it if so */
						timepos_t const t (canvas_event_sample (event));
						std::shared_ptr<Region> prev = pl->find_next_region (t, End, -1);
						if (!prev) {
							_drags->set (new RegionCreateDrag (*this, item, parent), event);
						} else {
							prev->set_length (prev->position ().distance (t));
						}
					}
				} else {
					/* rubberband drag to select automation points */
					_drags->set (new RubberbandSelectDrag (*this, item, [&](GdkEvent* ev, timepos_t const & pos) { return this->rb_click (ev, pos); }), event);
				}
				break;
			}

			case SelectionItem:
			{
				break;
			}

			case MarkerBarItem:

				break;

			default:
				break;
			}
		}
		return true;
		break;

	case MouseGrid:
		/* MouseGrid clicks are handled by _canvas_grid_zone */
		assert (0);
		abort(); /*NOTREACHED*/
		break;

	case MouseDraw:
		switch (item_type) {
		case GainLineItem:
			_drags->set (new LineDrag (*this, item, [&](GdkEvent* ev,timepos_t const & pos, double y) { line_drag_click (ev, pos, y); }), event);
			return true;

		case ControlPointItem:
			_drags->set (new ControlPointDrag (*this, item), event);
			return true;
			break;

		case SelectionItem:
			{
				if (selection->time.empty ()) {
					/* nothing to do */
					return true;
				}
				pair<TimeAxisView*, int> tvp = trackview_by_y_position (event->button.y, false);
				if (!tvp.first) {
					/* clicked outside of a track */
					return true;
				}
				/* handle automation lanes first */
				AutomationTimeAxisView* atv = dynamic_cast<AutomationTimeAxisView*> (tvp.first);
				if (atv) {
					/* if there's no line yet, AutomationRangeDrag will need to be told what the initial value of this control is */
					float init_value = atv->control()->get_value();

					_drags->set (new AutomationRangeDrag (*this, atv, init_value, selection->time), event, _cursors->up_down);
					return true;
				}
				if (dynamic_cast<AutomationRegionView*>(clicked_regionview)) {
					/* MIDI CC or similar -- TODO handle multiple? */
					list<RegionView*> rvl;
					rvl.push_back (clicked_regionview);
					_drags->set (new AutomationRangeDrag (*this, rvl, selection->time,
								clicked_regionview->get_time_axis_view().y_position(),
								clicked_regionview->get_time_axis_view().current_height()),
							event, _cursors->up_down);
					return true;
				}

				/* shift+drag: only apply to clicked_regionview (if any) */
				if (Keyboard::modifier_state_contains (event->button.state, Keyboard::TertiaryModifier)) {
					if (dynamic_cast<AudioRegionView*>(clicked_regionview) == 0) {
						return true;
					}
					list<RegionView*> rvl;
					rvl.push_back (clicked_regionview);
					// TODO: handle layer_display() == Stacked
					_drags->set (new AutomationRangeDrag (*this, rvl, selection->time,
								clicked_regionview->get_time_axis_view().y_position(),
								clicked_regionview->get_time_axis_view().current_height()),
							event, _cursors->up_down);
					return true;
				}

				/* collect all audio regions-views in the given range selection */
				list<RegionView*> rvl;
				TrackViewList ts = selection->tracks.filter_to_unique_playlists ();
				for (TrackViewList::iterator i = ts.begin(); i != ts.end(); ++i) {
					RouteTimeAxisView* tatv;
					std::shared_ptr<Playlist> playlist;
					if ((tatv = dynamic_cast<RouteTimeAxisView*> (*i)) == 0) {
						continue;
					}
					if ((playlist = (*i)->playlist()) == 0) {
						continue;
					}
					if (std::dynamic_pointer_cast<AudioPlaylist> (playlist) == 0) {
						continue;
					}
					for (list<TimelineRange>::const_iterator j = selection->time.begin(); j != selection->time.end(); ++j) {
						std::shared_ptr<RegionList> rl = playlist->regions_touched (j->start(), j->end());
						for (RegionList::iterator ir = rl->begin(); ir != rl->end(); ++ir) {
							RegionView* rv;
							if ((rv = tatv->view()->find_view (*ir)) != 0) {
								rvl.push_back (rv);
							}
						}
					}
				}
				/* region-gain drag */
				if (!rvl.empty ()) {
					double y_pos = tvp.first->y_position();
					double height = tvp.first->current_height();
					StreamView* cv = tvp.first->view ();
					if (cv->layer_display() == Stacked && cv->layers() > 1) {
						height /= cv->layers();
						double yy = event->button.y - _trackview_group->canvas_origin().y;
						y_pos += floor ((yy - y_pos) / height) * height;
					}
					_drags->set (new AutomationRangeDrag (*this, rvl, selection->time, y_pos, height),
							event, _cursors->up_down);
				}
				return true;
				break;
			}

		case EditorAutomationLineItem:
			/* fallthrough */
		case AutomationTrackItem:
			{
				AutomationTimeAxisView* atv = static_cast<AutomationTimeAxisView*> (item->get_data ("trackview"));
				if (atv) {
					_drags->set (new AutomationDrawDrag (*this, nullptr, atv->base_item(), false, Temporal::AudioTime), event);
				}
			}
			break;

		case NoteItem:
			if ((note = reinterpret_cast<NoteBase*>(item->get_data ("notebase")))) {
				if (note->big_enough_to_trim() && note->mouse_near_ends()) {
					/* Note is big and pointer is near the end, trim */
					_drags->set (new NoteResizeDrag (*this, item), event, get_canvas_cursor());
				} else {
					/* Drag note */
					_drags->set (new NoteDrag (*this, item), event);
				}
				return true;
			}
			return true;

		case StreamItem:
			if (dynamic_cast<MidiTimeAxisView*> (clicked_axisview)) {
				_drags->set (new RegionCreateDrag (*this, item, clicked_axisview), event);
			}
			return true;
		case RegionItem: {
			RegionView* rv;
			if ((rv = dynamic_cast<RegionView*> (clicked_regionview))) {
				ArdourCanvas::Rectangle* r = dynamic_cast<ArdourCanvas::Rectangle*> (rv->get_canvas_frame());
				_drags->set (new AutomationDrawDrag (*this, rv->get_canvas_group(), *r, true, Temporal::AudioTime), event);
			}
		}
			break;

		default:
			break;
		}
		return true;
		break;

	case MouseTimeFX:
		if (item_type == NoteItem) {
			/* resize-drag notes */
			if ((note = reinterpret_cast<NoteBase*>(item->get_data ("notebase")))) {
				if (note->big_enough_to_trim()) {
					_drags->set (new NoteResizeDrag (*this, item), event, get_canvas_cursor());
				}
			}
			return true;
		} else if (clicked_regionview) {
			/* do time-FX  */
			_drags->set (new TimeFXDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

bool
Editor::button_press_handler_2 (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	Editing::MouseMode const eff = effective_mouse_mode ();
	switch (eff) {
	case MouseObject:
		if (_drags->active ()) {
			return true;
		}
		switch (item_type) {
		case RegionItem:
			if (ArdourKeyboard::indicates_copy (event->button.state)) {
				add_region_drag (item, event, clicked_regionview, true);
			} else {
				add_region_drag (item, event, clicked_regionview, false);
			}
			_drags->start_grab (event);
			return true;
			break;
		case ControlPointItem:
			_drags->set (new ControlPointDrag (*this, item), event);
			return true;
			break;

		default:
			break;
		}

		switch (item_type) {
		case RegionViewNameHighlight:
			_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
			return true;
			break;

		case LeftFrameHandle:
		case RightFrameHandle:
			_drags->set (new TrimDrag (*this, item, clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
			return true;
			break;

		case RegionViewName:
			_drags->set (new TrimDrag (*this, clicked_regionview->get_name_highlight(), clicked_regionview, selection->regions.by_layer(), drag_time_domain (clicked_regionview->region())), event);
			return true;
			break;

		default:
			break;
		}

		break;

	case MouseDraw:
		return false;

	case MouseRange:
		/* relax till release */
		return true;
		break;

	default:
		break;
	}

	return false;
}

bool
Editor::button_press_handler (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	if (event->type == GDK_2BUTTON_PRESS) {
		_drags->mark_double_click ();
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		return true;
	}

	if (event->type != GDK_BUTTON_PRESS) {
		return false;
	}

	_track_canvas->grab_focus();

	if (_session && _session->actively_recording()) {
		return true;
	}

	button_selection (item, event, item_type);

	if (!_drags->active () &&
	    (Keyboard::is_delete_event (&event->button) ||
	     Keyboard::is_context_menu_event (&event->button) ||
	     Keyboard::is_edit_event (&event->button))) {

		/* handled by button release */
		return true;
	}

	/* not rolling, effectively in range mode, follow edits enabled (likely
	 * to start range drag), not in a fade handle (since that means we are
	 * not starting a range drag): locate the PH here
	 */

	if ((item_type != FadeInHandleItem) &&
	    (item_type != FadeOutHandleItem) &&
	    !_drags->active () &&
	    _session &&
	    !_session->transport_rolling() &&
	    (effective_mouse_mode() == MouseRange) &&
	    UIConfiguration::instance().get_follow_edits() &&
	    !_session->config.get_external_sync()) {

		timepos_t where (canvas_event_sample (event));
		snap_to (where);
		_session->request_locate (where.samples(), false, MustStop);
	}

	switch (event->button.button) {
	case 1:
		return button_press_handler_1 (item, event, item_type);
		break;

	case 2:
		return button_press_handler_2 (item, event, item_type);
		break;

	case 3:
		break;

	default:
		return button_press_dispatch (&event->button);
		break;

	}

	return false;
}

bool
Editor::button_press_dispatch (GdkEventButton* ev)
{
	/* this function is intended only for buttons 4 and above. */

	Gtkmm2ext::MouseButton b (ev->state, ev->button);
	return button_bindings->activate (b, Gtkmm2ext::Bindings::Press);
}

bool
Editor::button_release_dispatch (GdkEventButton* ev)
{
	/* this function is intended only for buttons 4 and above. */

	Gtkmm2ext::MouseButton b (ev->state, ev->button);
	return button_bindings->activate (b, Gtkmm2ext::Bindings::Release);
}

bool
Editor::button_release_handler (ArdourCanvas::Item* item, GdkEvent* event, ItemType item_type)
{
	timepos_t where (canvas_event_time (event));
	AutomationTimeAxisView* atv = 0;

	/* no action if we're recording */

	if (_session && _session->actively_recording()) {
		return true;
	}

	bool were_dragging = false;

	if (!Keyboard::is_context_menu_event (&event->button)) {

		/* see if we're finishing a drag */

		if (_drags->active ()) {

			bool const r = _drags->end_grab (event);
			if (r) {
				/* grab dragged, so do nothing else */
				return true;
			}

			were_dragging = true;
		}

		update_region_layering_order_editor ();
	}

	/* edit events get handled here */

	if (!_drags->active () && Keyboard::is_edit_event (&event->button)) {
		switch (item_type) {
		case RegionItem:
			show_region_properties ();
			break;
		case TempoMarkerItem: {
			ArdourMarker* marker;
			TempoMarker* tempo_marker;

			if ((marker = reinterpret_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
				fatal << _("programming error: tempo marker canvas item has no marker object pointer!") << endmsg;
				abort(); /*NOTREACHED*/
			}

			if ((tempo_marker = dynamic_cast<TempoMarker*> (marker)) == 0) {
				fatal << _("programming error: marker for tempo is not a tempo marker!") << endmsg;
				abort(); /*NOTREACHED*/
			}

			edit_tempo_marker (*tempo_marker);
			break;
		}
		case BBTMarkerItem: {
			ArdourMarker* marker;
			BBTMarker* bbt_marker;

			if ((marker = reinterpret_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
				fatal << _("programming error: bbt marker canvas item has no marker object pointer!") << endmsg;
				abort(); /*NOTREACHED*/
			}

			if ((bbt_marker = dynamic_cast<BBTMarker*> (marker)) == 0) {
				fatal << _("programming error: marker for bbt is not a bbt marker!") << endmsg;
				abort(); /*NOTREACHED*/
			}

			edit_bbt_marker (*bbt_marker);
			break;
		}

		case MeterMarkerItem: {
			ArdourMarker* marker;
			MeterMarker* meter_marker;

			if ((marker = reinterpret_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
				fatal << _("programming error: tempo marker canvas item has no marker object pointer!") << endmsg;
				abort(); /*NOTREACHED*/
			}

			if ((meter_marker = dynamic_cast<MeterMarker*> (marker)) == 0) {
				fatal << _("programming error: marker for meter is not a meter marker!") << endmsg;
				abort(); /*NOTREACHED*/
			}
			edit_meter_marker (*meter_marker);
			break;
		}

		case RegionViewName:
			if (clicked_regionview->name_active()) {
				return mouse_rename_region (item, event);
			}
			break;

		case ControlPointItem:
			edit_control_point (item);
			break;

		default:
			break;
		}
		return true;
	}

	/* context menu events get handled here */
	if (Keyboard::is_context_menu_event (&event->button)) {

		context_click_event = *event;

		if (!_drags->active ()) {

			/* no matter which button pops up the context menu, tell the menu
			   widget to use button 1 to drive menu selection.
			*/

			switch (item_type) {
			case FadeInItem:
			case FadeInHandleItem:
			case FadeInTrimHandleItem:
			case StartCrossFadeItem:
				popup_xfade_in_context_menu (1, event->button.time, item, item_type);
				break;

			case FadeOutItem:
			case FadeOutHandleItem:
			case FadeOutTrimHandleItem:
			case EndCrossFadeItem:
				popup_xfade_out_context_menu (1, event->button.time, item, item_type);
				break;

			case LeftFrameHandle:
			case RightFrameHandle:
				break;

			case StreamItem:
				popup_track_context_menu (1, event->button.time, item_type, false);
				break;

			case RegionItem:
			case RegionViewNameHighlight:
			case RegionViewName:
				popup_track_context_menu (1, event->button.time, item_type, false);
				break;

			case SelectionItem:
				popup_track_context_menu (1, event->button.time, item_type, true);
				break;

			case AutomationTrackItem:
				popup_track_context_menu (1, event->button.time, item_type, false);
				break;

			case MarkerBarItem:
			case RangeMarkerBarItem:
			case SectionMarkerBarItem:
			case TempoBarItem:
			case TempoCurveItem:
			case MeterBarItem:
			case VideoBarItem:
			case TimecodeRulerItem:
			case SamplesRulerItem:
			case MinsecRulerItem:
			case BBTRulerItem:
			case SelectionMarkerItem:
				snap_to (where, Temporal::RoundNearest, SnapToGrid_Scaled, false);
				popup_ruler_menu (where, item_type);
				break;

			case MarkerItem:
				marker_context_menu (&event->button, item);
				break;

			case SceneMarkerItem:
				marker_context_menu (&event->button, item);
				break;

			case TempoMarkerItem:
			case MeterMarkerItem:
			case BBTMarkerItem:
				tempo_map_marker_context_menu (&event->button, item);
				break;

			case CrossfadeViewItem:
				popup_track_context_menu (1, event->button.time, item_type, false);
				break;

			case ControlPointItem:
				popup_control_point_context_menu (item, event);
				break;

			case NoteItem:
				if (internal_editing()) {
					popup_note_context_menu (item, event);
				}
				break;

			case GridZoneItem:
			default:
				break;
			}

			return true;
		}
	}

	/* delete events get handled here */

	Editing::MouseMode const eff = effective_mouse_mode ();

	if (!_drags->active () && Keyboard::is_delete_event (&event->button)) {

		switch (item_type) {
		case TempoMarkerItem:
			remove_tempo_marker (item);
			break;

		case BBTMarkerItem:
			remove_bbt_marker (item);
			break;

		case MeterMarkerItem:
			remove_meter_marker (item);
			break;

		case MarkerItem:
			remove_marker (*item);
			break;

		case RegionItem:
			if (eff == MouseObject) {
				remove_clicked_region ();
			}
			break;

		case ControlPointItem:
			remove_control_point (item);
			break;

		case NoteItem:
			remove_midi_note (item, event);
			break;

		default:
			break;
		}
		return true;
	}

	switch (event->button.button) {
	case 1:

		switch (item_type) {
		/* see comments in button_press_handler */
		case PlayheadCursorItem:
		case MarkerItem:
		case GainLineItem:
		case EditorAutomationLineItem:
		case StartSelectionTrimItem:
		case EndSelectionTrimItem:
		case SelectionMarkerItem:
			return true;

		case MarkerBarItem:
			if (!_dragging_playhead) {
				snap_to_with_modifier (where, event, Temporal::RoundNearest, SnapToGrid_Scaled);
				add_location_mark (where);
			}
			return true;

		case SectionMarkerBarItem:
			if (!_dragging_playhead && Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)) {
				snap_to_with_modifier (where, event, Temporal::RoundNearest, SnapToGrid_Scaled);
				add_location_mark (where, Location::Flags (Location::IsMark | Location::IsSection));
			}
			return true;

		case GridZoneItem:
			return true;

		case TempoBarItem:
		case TempoCurveItem:
			if (!_dragging_playhead && Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)) {
				snap_to_with_modifier (where, event);
				mouse_add_new_tempo_event (where);
			}
			return true;

		case MeterBarItem:
			if (!_dragging_playhead && Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier)) {
				mouse_add_new_meter_event (timepos_t (pixel_to_sample (event->button.x)));
			}
			return true;
			break;

		case TimecodeRulerItem:
		case SamplesRulerItem:
		case MinsecRulerItem:
		case BBTRulerItem:
			return true;
			break;

		default:
			break;
		}

		switch (eff) {
		case MouseDraw:
			switch (item_type) {
			case RegionItem:
			{
				/* since we have FreehandLineDrag we can only get here after a drag, when no movement has happened.
				 * Except when a drag was aborted by pressing Esc.
				 */
				if (!were_dragging) {
					return true;
				}

				AudioRegionView*      arv = dynamic_cast<AudioRegionView*> (clicked_regionview);
				AutomationRegionView* atv = dynamic_cast<AutomationRegionView*> (clicked_regionview);

				if (arv) {
					bool with_guard_points = Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier);
					arv->add_gain_point_event (item, event, with_guard_points);
				} else if (atv) {
					atv->add_automation_event (event);
				}

				return true;
				break;
			}

			case AutomationTrackItem: {
				bool with_guard_points = Keyboard::modifier_state_equals (event->button.state, Keyboard::PrimaryModifier);
				atv = dynamic_cast<AutomationTimeAxisView*>(clicked_axisview);
				if (atv) {
					atv->add_automation_event (event, where, event->button.y, with_guard_points);
				}
				return true;
				break;
			}
			default:
				break;
			}
			break;

		case MouseGrid:
			/* MouseGrid clicks are handled by _canvas_grid_zone ,
			 * We can still get here by single-click on a Tempo, Meter or BBT Marker in the ruler.
			 */
			return true;
			break;

		default:
			break;

		}

		/* do any (de)selection operations that should occur on button release */
		button_selection (item, event, item_type);

		return true;
		break;


	case 2:
		switch (eff) {

		case MouseObject:
			switch (item_type) {
			case RegionItem:
				if (Keyboard::modifier_state_equals (event->button.state, Keyboard::TertiaryModifier)) {
					raise_region ();
				} else if (Keyboard::modifier_state_equals (event->button.state, Keyboard::ModifierMask (Keyboard::TertiaryModifier|Keyboard::SecondaryModifier))) {
					lower_region ();
				} else {
					/* Button2 click is unused */
				}
				return true;

				break;

			default:
				break;
			}
			break;

		case MouseDraw:
			if (item_type == NoteItem) {
				remove_midi_note (item, event);
			}
			return true;

		case MouseContent:
			if (item_type == NoteItem) {
				remove_midi_note (item, event);
				return true;
			}
			break;

		case MouseRange:
			// x_style_paste (where, 1.0);
			return true;
			break;

		default:
			break;
		}
		break;

	case 3:
		break;

	default:
		break;
	}

	return false;
}


GridType
Editor::determine_mapping_grid_snap (timepos_t t)
{
	timepos_t snapped          = snap_to_bbt_via_grid (t, RoundNearest, SnapToGrid_Unscaled, GridTypeBeat);
	timepos_t snapped_to_bar   = snap_to_bbt_via_grid (t, RoundNearest, SnapToGrid_Unscaled, GridTypeBar);
	const double unsnapped_pos = time_to_pixel_unrounded (t);
	const double snapped_pos   = time_to_pixel_unrounded (snapped);

	double ruler_line_granularity = UIConfiguration::instance().get_ruler_granularity () * UIConfiguration::instance().get_ui_scale(); // in pixels

	if (std::abs (snapped_pos - unsnapped_pos) < ruler_line_granularity) {
		if (snapped == snapped_to_bar) {
			return GridTypeBar;
		} else {
			return GridTypeBeat;
		}
	} else {
		return GridTypeNone;
	}
}

bool
Editor::motion_handler (ArdourCanvas::Item* item, GdkEvent* event, bool from_autoscroll)
{
	auto mouse_mode = current_mouse_mode ();

	_last_motion_y = event->motion.y;

	if (event->motion.is_hint) {
		gint x, y;

		/* We call this so that MOTION_NOTIFY events continue to be
		 * delivered to the canvas. We need to do this because we set
		 * Gdk::POINTER_MOTION_HINT_MASK on the canvas. This reduces
		 * the density of the events, at the expense of a round-trip
		 * to the server. Given that this will mostly occur on cases
		 * where DISPLAY = :0.0, and given the cost of what the motion
		 * event might do, its a good tradeoff.
		 */

		_track_canvas->get_pointer (x, y);
	}

	if (current_stepping_trackview) {
		/* don't keep the persistent stepped trackview if the mouse moves */
		current_stepping_trackview = 0;
		step_timeout.disconnect ();
	}

	if (_session && _session->actively_recording()) {
		/* Sorry. no dragging stuff around while we record */
		return true;
	}

	update_join_object_range_location (event->motion.y);

	if (_drags->active ()) {
		_region_peak_cursor->hide ();
		//drags change the snapped_cursor location, because we are snapping the thing being dragged, not the actual mouse cursor
		return _drags->motion_handler (event, from_autoscroll);
	} else {

		bool ignored;
		bool peaks_visible = false;
		samplepos_t where;

		if (mouse_sample (where, ignored)) {

			/* display peaks */
			if (mouse_mode == MouseContent || ArdourKeyboard::indicates_snap (event->motion.state)) {
				AudioRegionView* arv = dynamic_cast<AudioRegionView*>(entered_regionview);
				if (arv) {
					_region_peak_cursor->set (arv, where, samples_per_pixel);
					peaks_visible = true;
				}
			}

			/* show snapped cursor */
			timepos_t t (where);
			bool move_snapped_cursor = true;
			if (move_snapped_cursor) {
				snap_to_with_modifier (t, event);
				set_snapped_cursor_position (t);
			}

			/* if tempo-mapping, set a cursor to indicate whether we are close to a bar line, beat line, or neither */
			if (mouse_mode == MouseGrid && item == _canvas_grid_zone) {
				GridType gt = determine_mapping_grid_snap (t);
				if (gt == GridTypeBar) {
					set_canvas_cursor (cursors()->time_fx);
				} else if (gt == GridTypeBeat) {
					set_canvas_cursor (cursors()->expand_left_right);
				} else {
					set_canvas_cursor (cursors()->grabber);
				}
			}
		}

		if (!peaks_visible) {
			_region_peak_cursor->hide ();
		}
	}

	return false;
}

bool
Editor::can_remove_control_point (ArdourCanvas::Item* item)
{
	ControlPoint* control_point;

	if ((control_point = reinterpret_cast<ControlPoint *> (item->get_data ("control_point"))) == 0) {
		fatal << _("programming error: control point canvas item has no control point object pointer!") << endmsg;
		abort(); /*NOTREACHED*/
	}

	AutomationLine& line (control_point->line());
	if (dynamic_cast<RegionFxLine*> (&line)) {
		/* we shouldn't remove the first or last gain point in region gain lines */
		if (line.is_last_point(*control_point) || line.is_first_point(*control_point)) {
			return false;
		}
	}

	return true;
}

void
Editor::remove_control_point (ArdourCanvas::Item* item)
{
	if (!can_remove_control_point (item)) {
		return;
	}

	ControlPoint* control_point;

	if ((control_point = reinterpret_cast<ControlPoint *> (item->get_data ("control_point"))) == 0) {
		fatal << _("programming error: control point canvas item has no control point object pointer!") << endmsg;
		abort(); /*NOTREACHED*/
	}

	control_point->line().remove_point (*control_point);
}

void
Editor::edit_control_point (ArdourCanvas::Item* item)
{
	ControlPoint* p = reinterpret_cast<ControlPoint *> (item->get_data ("control_point"));

	if (p == 0) {
		fatal << _("programming error: control point canvas item has no control point object pointer!") << endmsg;
		abort(); /*NOTREACHED*/
	}

	std::vector<ControlPoint*> cps;

	for (auto const& cp : selection->points) {
		if (&cp->line() == &p->line ()) {
			cps.push_back (cp);
		}
	}

	assert (cps.size() > 0);

	ControlPointDialog d (p, cps.size() > 1);

	if (d.run () != RESPONSE_ACCEPT) {
		return;
	}

	if (d.all_selected_points ()) {
		p->line().modify_points_y (cps, d.get_y_fraction ());
	} else {
		cps.clear ();
		cps.push_back (p);
		p->line().modify_points_y (cps, d.get_y_fraction ());
	}
}

void
Editor::edit_region (RegionView* rv)
{
	if (UIConfiguration::instance().get_use_double_click_to_zoom_to_selection()) {
		temporal_zoom_selection (Both);
	} else {
		rv->show_region_editor ();
	}
}

void
Editor::visible_order_range (int* low, int* high) const
{
	*low = TimeAxisView::max_order ();
	*high = 0;

	for (TrackViewList::const_iterator i = track_views.begin(); i != track_views.end(); ++i) {

		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (*i);

		if (rtv && !rtv->hidden()) {

			if (*high < rtv->order()) {
				*high = rtv->order ();
			}

			if (*low > rtv->order()) {
				*low = rtv->order ();
			}
		}
	}
}

void
Editor::region_view_item_click (AudioRegionView& rv, GdkEventButton* event)
{
	/* Either add to or set the set the region selection, unless
	 * this is an alignment click (control used)
	 */

	if (Keyboard::modifier_state_contains (event->state, Keyboard::PrimaryModifier)) {

		timepos_t where = get_preferred_edit_position();

		if (where.is_negative()) {
			return;
		}

		if (Keyboard::modifier_state_equals (event->state, Keyboard::ModifierMask (Keyboard::PrimaryModifier|Keyboard::SecondaryModifier))) {

			align_region (rv.region(), SyncPoint, where);

		} else if (Keyboard::modifier_state_equals (event->state, Keyboard::ModifierMask (Keyboard::PrimaryModifier|Keyboard::TertiaryModifier))) {

			align_region (rv.region(), End, where);

		} else {

			align_region (rv.region(), Start, where);
		}
	}
}

void
Editor::collect_new_region_view (RegionView* rv)
{
	latest_regionviews.push_back (rv);
}

void
Editor::collect_and_select_new_region_view (RegionView* rv)
{
	selection->add(rv);
	latest_regionviews.push_back (rv);
}

void
Editor::cancel_selection ()
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(*i)->hide_selection ();
	}

	selection->clear ();
	clicked_selection = 0;
}

void
Editor::cancel_time_selection ()
{
	for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
		(*i)->hide_selection ();
	}
	selection->time.clear ();
	clicked_selection = 0;
}

void
Editor::point_trim (GdkEvent* event, timepos_t const & new_bound)
{
	RegionView* rv = clicked_regionview;

	/* Choose action dependent on which button was pressed */
	switch (event->button.button) {
	case 1:
		begin_reversible_command (_("start point trim"));

		if (selection->selected (rv)) {
			for (list<RegionView*>::const_iterator i = selection->regions.by_layer().begin();
			     i != selection->regions.by_layer().end(); ++i)
			{
				if (!(*i)->region()->locked()) {
					(*i)->region()->clear_changes ();
					(*i)->region()->trim_front (new_bound);
					_session->add_command(new StatefulDiffCommand ((*i)->region()));
				}
			}

		} else {
			if (!rv->region()->locked()) {
				rv->region()->clear_changes ();
				rv->region()->trim_front (new_bound);
				_session->add_command(new StatefulDiffCommand (rv->region()));
			}
		}

		commit_reversible_command();

		break;
	case 2:
		begin_reversible_command (_("end point trim"));

		if (selection->selected (rv)) {

			for (list<RegionView*>::const_iterator i = selection->regions.by_layer().begin(); i != selection->regions.by_layer().end(); ++i)
			{
				if (!(*i)->region()->locked()) {
					(*i)->region()->clear_changes();
					(*i)->region()->trim_end (new_bound);
					_session->add_command(new StatefulDiffCommand ((*i)->region()));
				}
			}

		} else {

			if (!rv->region()->locked()) {
				rv->region()->clear_changes ();
				rv->region()->trim_end (new_bound);
				_session->add_command (new StatefulDiffCommand (rv->region()));
			}
		}

		commit_reversible_command();

		break;
	default:
		break;
	}
}

void
Editor::hide_marker (ArdourCanvas::Item* item, GdkEvent* /*event*/)
{
	ArdourMarker* marker;
	bool is_start;

	if ((marker = static_cast<ArdourMarker *> (item->get_data ("marker"))) == 0) {
		fatal << _("programming error: marker canvas item has no marker object pointer!") << endmsg;
		abort(); /*NOTREACHED*/
	}

	Location* location = find_location_from_marker (marker, is_start);
	location->set_hidden (true, this);
}

gint
Editor::mouse_rename_region (ArdourCanvas::Item* /*item*/, GdkEvent* /*event*/)
{
	using namespace Gtkmm2ext;

	ArdourWidgets::Prompter prompter (false);

	prompter.set_prompt (_("Name for region:"));
	prompter.set_initial_text (clicked_regionview->region()->name());
	prompter.add_button (_("Rename"), Gtk::RESPONSE_ACCEPT);
	prompter.set_response_sensitive (Gtk::RESPONSE_ACCEPT, false);
	prompter.show_all ();
	switch (prompter.run ()) {
	case Gtk::RESPONSE_ACCEPT:
		string str;
		prompter.get_result(str);
		if (str.length()) {
			clicked_regionview->region()->set_name (str);
		}
		break;
	}
	return true;
}


void
Editor::mouse_brush_insert_region (RegionView* rv, timepos_t const & pos)
{
	/* no brushing without a useful quantize setting */
	if (grid_type() == GridTypeNone)
		return;

	/* don't brush a copy over the original */

	if (pos == rv->region()->position()) {
		return;
	}

	RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*>(&rv->get_time_axis_view());

	if (!rtv || !rtv->is_track()) {
		return;
	}

	std::shared_ptr<Playlist> playlist = rtv->playlist();

	playlist->clear_changes ();
	std::shared_ptr<Region> new_region (RegionFactory::create (rv->region(), true));
	playlist->add_region (new_region, pos);
	_session->add_command (new StatefulDiffCommand (playlist));

	/* playlist is frozen, so we have to update manually XXX this is disgusting */

	//playlist->RegionAdded (new_region); /* EMIT SIGNAL */
}

gint
Editor::track_height_step_timeout ()
{
	if (get_microseconds() - last_track_height_step_timestamp < 250000) {
		current_stepping_trackview = 0;
		return false;
	}
	return true;
}

void
Editor::add_region_drag (ArdourCanvas::Item* item, GdkEvent*, RegionView* region_view, bool copy)
{
	assert (region_view);

	if (!region_view->region()->playlist()) {
		return;
	}

	assert (!_drags->active ());

	_drags->add (new RegionMoveDrag (*this, item, region_view, selection->regions.by_layer(), copy, drag_time_domain (region_view->region())));
}

void
Editor::add_region_brush_drag (ArdourCanvas::Item* item, GdkEvent*, RegionView* region_view)
{
	assert (region_view);

	if (!region_view->region()->playlist()) {
		return;
	}

	assert (!_drags->active ());

	if (should_ripple()) {
		return;
	}

	std::list<RegionView*> empty;
	_drags->add (new RegionBrushDrag (*this, item, region_view, empty, drag_time_domain (region_view->region())));
}

/** Start a grab where a time range is selected, track(s) are selected, and the
 *  user clicks and drags a region with a modifier in order to create a new region containing
 *  the section of the clicked region that lies within the time range.
 */
void
Editor::start_selection_grab (ArdourCanvas::Item* /*item*/, GdkEvent* event)
{
	if (clicked_regionview == 0) {
		return;
	}

	/* lets try to create new Region for the selection */

	vector<std::shared_ptr<Region> > new_regions;
	create_region_from_selection (new_regions);

	if (new_regions.empty()) {
		return;
	}

	/* XXX fix me one day to use all new regions */

	std::shared_ptr<Region> region (new_regions.front());

	/* add it to the current stream/playlist.
	 *
	 * tricky: the streamview for the track will add a new regionview. we will
	 * catch the signal it sends when it creates the regionview to
	 * set the regionview we want to then drag.
	 */

	latest_regionviews.clear();
	sigc::connection c = clicked_routeview->view()->RegionViewAdded.connect (sigc::mem_fun(*this, &Editor::collect_new_region_view));

	/* A selection grab currently creates two undo/redo operations, one for
	 * creating the new region and another for moving it.
	 */
	begin_reversible_command (Operations::selection_grab);

	std::shared_ptr<Playlist> playlist = clicked_axisview->playlist();

	playlist->clear_changes ();
	clicked_routeview->playlist()->add_region (region, selection->time[clicked_selection].start());
	_session->add_command(new StatefulDiffCommand (playlist));

	c.disconnect ();

	if (latest_regionviews.empty()) {
		/* something went wrong */
		abort_reversible_command ();
		return;
	}

	/* we need to deselect all other regionviews, and select this one
	 * i'm ignoring undo stuff, because the region creation will take care of it
	 */

	selection->set (latest_regionviews);

	commit_reversible_command ();

	_drags->set (new RegionMoveDrag (*this, latest_regionviews.front()->get_canvas_group(), latest_regionviews.front(), latest_regionviews, false, drag_time_domain (latest_regionviews.front()->region())), event);
}

void
Editor::escape ()
{
	if (_drags->active ()) {
		_drags->abort ();
	} else if (_session) {

		midi_action (&MidiRegionView::clear_note_selection);

		selection->clear ();

		/* if session is playing a range, cancel that */
		if (_session->get_play_range()) {
			_session->request_cancel_play_range();
		}

		if (_session->solo_selection_active()) {
			StripableList sl;
			_session->solo_selection (sl, false);
		}
	}

	ARDOUR_UI::instance()->reset_focus (&contents());
}

/** Update _join_object_range_state which indicate whether we are over the top
 *  or bottom half of a route view, used by the `join object/range' tool
 *  mode. Coordinates in canvas space.
 */
void
Editor::update_join_object_range_location (double y)
{
	auto mouse_mode = current_mouse_mode ();

	if (!get_smart_mode()) {
		_join_object_range_state = JOIN_OBJECT_RANGE_NONE;
		return;
	}

	JoinObjectRangeState const old = _join_object_range_state;

	if (mouse_mode == MouseObject) {
		_join_object_range_state = JOIN_OBJECT_RANGE_OBJECT;
	} else if (mouse_mode == MouseRange) {
		_join_object_range_state = JOIN_OBJECT_RANGE_RANGE;
	}

	if (entered_regionview) {

		if (dynamic_cast<AutomationRegionView*> (entered_regionview)) {
			_join_object_range_state = JOIN_OBJECT_RANGE_NONE;
			return;
		}

		/* TODO: there is currently a bug here(?)
		 * when we are inside a region fade handle, it acts as though we are in range mode because it is in the top half of the region
		 * can it be fixed here?
		 */

		ArdourCanvas::Duple const item_space = entered_regionview->get_canvas_group()->canvas_to_item (ArdourCanvas::Duple (0, y));
		double const c = item_space.y / entered_regionview->height();

		_join_object_range_state = c <= 0.5 ? JOIN_OBJECT_RANGE_RANGE : JOIN_OBJECT_RANGE_OBJECT;

		if (_join_object_range_state != old) {
			set_canvas_cursor (which_track_cursor());
		}

	} else if (entered_track) {

		RouteTimeAxisView* entered_route_view = dynamic_cast<RouteTimeAxisView*> (entered_track);

		if (entered_route_view) {

			double cx = 0;
			double cy = y;

			entered_route_view->canvas_display()->canvas_to_item (cx, cy);

			double track_height = entered_route_view->view()->child_height();
			if (UIConfiguration::instance().get_show_name_highlight()) {
				track_height -= TimeAxisViewItem::NAME_HIGHLIGHT_SIZE;
			}
			double const c = cy / track_height;


			if (c <= 0.5) {
				_join_object_range_state = JOIN_OBJECT_RANGE_RANGE;
			} else {
				_join_object_range_state = JOIN_OBJECT_RANGE_OBJECT;
			}

		} else {
			/* Other kinds of tracks use object mode */
			_join_object_range_state = JOIN_OBJECT_RANGE_OBJECT;
		}

		if (_join_object_range_state != old) {
			set_canvas_cursor (which_track_cursor());
		}
	}
}

Editing::MouseMode
Editor::effective_mouse_mode () const
{
	if (_join_object_range_state == JOIN_OBJECT_RANGE_OBJECT) {
		return MouseObject;
	} else if (_join_object_range_state == JOIN_OBJECT_RANGE_RANGE) {
		return MouseRange;
	}

	return current_mouse_mode ();
}

void
Editor::use_appropriate_mouse_mode_for_sections ()
{
	Glib::RefPtr<ToggleAction> tact;

	switch (current_mouse_mode ()) {
		case Editing::MouseRange:
			/* OK, no need to change mouse mode */
			break;
		case Editing::MouseObject:
			/* "object-range" mode is not a distinct mouse mode, so
			   we cannot use get_mouse_mode_action() here
			*/
			tact = ActionManager::get_toggle_action (X_("Editor"), "set-mouse-mode-object-range");
			if (!tact) {
				/* missing action */
				fatal << X_("programming error: missing mouse-mode-object-range action") << endmsg;
				/*NOTREACHED*/
				break;
			}
			if (tact->get_active()) {
				/* smart mode; OK, leave things as they are */
				break;
			}
			/*fallthrough*/
		default:
			/* switch to range mode */
			mouse_mode_actions[Editing::MouseRange]->set_active (true);
			break;
	}
}

void
Editor::remove_midi_note (ArdourCanvas::Item* item, GdkEvent *)
{
	NoteBase* e = reinterpret_cast<NoteBase*> (item->get_data ("notebase"));
	assert (e);

	e->midi_view().delete_note (e->note ());
}

/** Obtain the pointer position in canvas coordinates */
void
Editor::get_pointer_position (double& x, double& y) const
{
	int px, py;
	_track_canvas->get_pointer (px, py);
	_track_canvas->window_to_canvas (px, py, x, y);
}

void
Editor::choose_mapping_drag (ArdourCanvas::Item* item, GdkEvent* event)
{

	/* In a departure from convention, this event is not handled by a widget
	 * 'on' the ruler-bar, like a tempo marker, but is instead handled by the
	 * whole canvas. The intent is for the user to feel that they
	 * are manipulating the 'beat and bar grid' which may or may not have tempo
	 * markers already assigned at the point under the mouse.
	 */

	bool ignored;
	samplepos_t where;

	if (!mouse_sample (where, ignored)) {
		return;
	}

	/* if tempo-mapping, set a cursor to indicate whether we are close to a bar line, beat line, or neither */
	bool ramped = false;
	auto mouse_mode = current_mouse_mode ();

	if (mouse_mode == MouseGrid && item ==_canvas_grid_zone) {
		GridType gt = determine_mapping_grid_snap (timepos_t (where));
		if (gt == GridTypeBar) {
			ramped = false;
		} else if (gt == GridTypeBeat) {
			ramped = true;
		} else {
			return; // neither a bar nor a beat; don't start a drag
		}
	}

	/* The reversible command starts here, must be ended/aborted in drag */
	begin_reversible_command ("");
	domain_bounce_info = new Temporal::DomainBounceInfo (Temporal::BeatTime, Temporal::AudioTime, should_stretch_markers());
	Temporal::TempoMap::WritableSharedPtr map = begin_tempo_mapping (*domain_bounce_info);

	/* Decide between a mid-twist, which we do if the
	 * pointer is between two tempo markers, and an end-stretch,
	 * which we do if the pointer is after the last tempo
	 * marker before the end of the map or a BBT Marker.
	 */

	timepos_t pointer_time (canvas_event_sample (event, nullptr, nullptr));
	Temporal::TempoPoint& tempo = const_cast<Temporal::TempoPoint&>(map->tempo_at (pointer_time));

	TempoPoint* before = const_cast<TempoPoint*> (map->previous_tempo (tempo));
	TempoPoint* after = const_cast<TempoPoint*> (map->next_tempo (tempo));

	XMLNode* before_state = &map->get_state();
	TempoPoint* focus;

	bool at_end = false;
	if (!after || dynamic_cast<MusicTimePoint*>(after)) {
		at_end = true;
	}

	BBT_Argument bbt = map->bbt_at (pointer_time);
	bbt = BBT_Argument (bbt.reference(), bbt.round_to_beat ());

	/* BBT_Argument is meter-agnostic so we need to use the map's meter to resolve bar boundaries */
	const Meter& m = map->meter_at (pointer_time);
	if (bbt.beats > m.divisions_per_bar()){
		bbt.beats = 1;
		bbt.bars++;
	}

	/* Create a new marker, or use the one under the mouse */

	if (tempo.bbt() == bbt) {

		/* special case 1: we are on the right side of the FIRST marker: do not allow the user to manipulate the very first (session global) tempo */
		if (!before) {
			abort_tempo_mapping ();
			abort_reversible_command ();
			return;
		}

		focus = &tempo;

		/* special case 2: if we are on the right side of the LAST marker: behave as if we clicked the marker prior*/
		if (at_end) {
			focus = before;
		}

	} else if ((after && after->bbt() == bbt )) {
		before = const_cast<TempoPoint*> (&tempo);
		focus = after;

		/* special case 3: if we are on the left side of the LAST marker: behave as if we clicked the marker prior*/
		TempoPoint* after_after = const_cast<TempoPoint*> (map->next_tempo (*focus));
		if (after_after) {
			after = after_after;
		} else {
			at_end = true;
		}

	} else if (ramped) {

		/* User is dragging on a BEAT line (not a bar line) ... try to implement a tempo twist on the prior marker */
		focus = &tempo;

	} else {
		/* Add a new tempo marker at the nearest beat point
		   (essentially the snapped grab point for the drag), so that
		   it becomes the middle one of three used by the twist tempo
		   operation.
		*/

		before = const_cast<TempoPoint*> (&tempo);
		Tempo copied_no_ramp (map->tempo_at (bbt));
		TempoPoint& added = const_cast<TempoPoint&> (map->set_tempo (copied_no_ramp, bbt));
		focus = &added;
		reset_tempo_marks ();
		reset_bbt_marks ();
	}

	/* Reversible commands get named here, now that we understand what we're doing */

	if (at_end) {
		_session->current_reversible_command()->set_name (_("tempo mapping: end-stretch"));
		_drags->set (new MappingEndDrag (*this, item, map, tempo, *focus, *before_state), event);
		return;
	}

	if (before && focus && after) {
		_session->current_reversible_command()->set_name (_("tempo mapping: mid-twist"));
		_drags->set (new MappingTwistDrag (*this, item, map, *before, *focus, *after, *before_state, ramped), event);
	} else if (ramped && focus && after) {
		/* special case 4: user is manipulating a beat line after the INITIAL tempo marker, so there is no prior marker*/
		_session->current_reversible_command()->set_name (_("tempo mapping: mid-twist"));
		before = focus; /* this is unused in MappingTwistDrag, when ramped is true, but let's not pass in garbage */
		_drags->set (new MappingTwistDrag (*this, item, map, *before, *focus, *after, *before_state, ramped), event);
	} else {
		abort_tempo_mapping ();  /* NOTREACHED */
	}
}

bool
Editor::rb_click (GdkEvent* event, timepos_t const & where)
{
	bool                    do_deselect = true;
	MidiTimeAxisView*       mtv;
	AutomationTimeAxisView* atv;

	if ((mtv = dynamic_cast<MidiTimeAxisView*> (clicked_axisview)) != 0) {
		/* MIDI track */
		if (get_selection().empty () && current_mouse_mode() == MouseDraw) {

			/* nothing selected */

			const timepos_t pos (where.beats ());
			const timecnt_t len = pos.distance (max (timepos_t::zero (Temporal::BeatTime), timepos_t (pos.beats () + Beats (1, 0))));
			mtv->add_region (pos, len, true);
			do_deselect = false;
		}
	} else if ((atv = dynamic_cast<AutomationTimeAxisView*> (clicked_axisview)) != 0) {
		atv->add_automation_event (event, where, event->button.y, false);
		do_deselect = false;
	}

	return do_deselect;
}

void
Editor::line_drag_click (GdkEvent* event, timepos_t const & where, double y)
{
	AutomationTimeAxisView* atv;

	if ((atv = dynamic_cast<AutomationTimeAxisView*> (clicked_axisview)) != 0) {

		atv->add_automation_event (event, where, y, false);

	} else if (dynamic_cast<AudioTimeAxisView*> (clicked_axisview) != 0) {

		AudioRegionView* arv;

		if ((arv = dynamic_cast<AudioRegionView*> (clicked_regionview)) != 0) {
			arv->add_gain_point_event (&arv->fx_line ()->grab_item (), event, false);
		}
	}
}
