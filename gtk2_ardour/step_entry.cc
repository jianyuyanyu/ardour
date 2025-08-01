/*
 * Copyright (C) 2010-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2010-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2012-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2012-2022 Robin Gareus <robin@gareus.org>
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

#include <iostream>

#include "pbd/file_utils.h"

#include "gtkmm2ext/actions.h"
#include "gtkmm2ext/keyboard.h"

#include "widgets/tooltips.h"

#include "ardour/filesystem_paths.h"

#include "midi_channel_selector.h"
#include "midi_time_axis.h"
#include "step_editor.h"
#include "step_entry.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace std;
using namespace Gtk;
using namespace Glib;
using namespace Gtkmm2ext;
using namespace PBD;
using namespace ARDOUR;
using namespace ARDOUR_UI_UTILS;
using namespace ArdourWidgets;

Gtkmm2ext::Bindings* StepEntry::bindings = 0;
StepEntry* StepEntry::_instance = 0;

StepEntry&
StepEntry::instance()
{
	if (!_instance) {
		_instance = new StepEntry;
	}

	return *_instance;
}

StepEntry::StepEntry ()
	: ArdourWindow (string())
	, _current_note_length (1, 0)
	, _current_note_velocity (64)
	, triplet_button ("3")
	, dot_adjustment (0.0, 0.0, 3.0, 1.0, 1.0)
	, beat_resync_button (_(">beat"))
	, bar_resync_button (_(">bar"))
	, resync_button (_(">EP"))
	, sustain_button (_("sustain"))
	, rest_button (_("rest"))
	, grid_rest_button (_("g-rest"))
	, back_button (_("back"))
	, channel_adjustment (1, 1, 16, 1, 4)
	, channel_spinner (channel_adjustment)
	, octave_adjustment (4, 0, 10, 1, 4) // start in octave 4
	, octave_spinner (octave_adjustment)
	, length_divisor_adjustment (4.0, 1.0, 128, 1.0, 4.0)
	, length_divisor_spinner (length_divisor_adjustment)
	, velocity_adjustment (64.0, 0.0, 127.0, 1.0, 4.0)
	, velocity_spinner (velocity_adjustment)
	, bank_adjustment (0, 0.0, 127.0, 1.0, 4.0)
	, bank_spinner (bank_adjustment)
	, bank_button (_("+"))
	, program_adjustment (0, 0.0, 127.0, 1.0, 4.0)
	, program_spinner (program_adjustment)
	, program_button (_("+"))
	, se (0)
{
	set_widget_bindings (*this, *bindings, ARDOUR_BINDING_KEY);

	Pango::FontDescription font (ARDOUR_UI_UTILS::sanitized_font ("ArdourSans 24"));
	length_1_button.set_layout_font (font);
	length_2_button.set_layout_font (font);
	length_4_button.set_layout_font (font);
	length_8_button.set_layout_font (font);
	length_16_button.set_layout_font (font);
	length_32_button.set_layout_font (font);
	length_64_button.set_layout_font (font);

	length_1_button.set_width_padding (.2);
	length_2_button.set_width_padding (.2);
	length_4_button.set_width_padding (.2);
	length_8_button.set_width_padding (.2);
	length_16_button.set_width_padding (.2);
	length_32_button.set_width_padding (.2);
	length_64_button.set_width_padding (.2);

	chord_button.set_layout_font (font);
	chord_button.set_width_padding (.2);

	Pango::FontDescription font2 (ARDOUR_UI_UTILS::sanitized_font ("ArdourSans 12"));
	velocity_ppp_button.set_layout_font (font2);
	velocity_pp_button.set_layout_font (font2);
	velocity_p_button.set_layout_font (font2);
	velocity_mp_button.set_layout_font (font2);
	velocity_mf_button.set_layout_font (font2);
	velocity_f_button.set_layout_font (font2);
	velocity_ff_button.set_layout_font (font2);
	velocity_fff_button.set_layout_font (font2);

	RefPtr<Action> act;

	act = ActionManager::get_action ("StepEditing/note-length-whole");
	length_1_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-half");
	length_2_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-quarter");
	length_4_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-eighth");
	length_8_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-sixteenth");
	length_16_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-thirtysecond");
	length_32_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-length-sixtyfourth");
	length_64_button.set_related_action (act);

	note_length_box.pack_start (length_1_button, false, false);
	note_length_box.pack_start (length_2_button, false, false);
	note_length_box.pack_start (length_4_button, false, false);
	note_length_box.pack_start (length_8_button, false, false);
	note_length_box.pack_start (length_16_button, false, false);
	note_length_box.pack_start (length_32_button, false, false);
	note_length_box.pack_start (length_64_button, false, false);

	set_tooltip (&length_1_button, _("Set note length to a whole note"), "");
	set_tooltip (&length_2_button, _("Set note length to a half note"), "");
	set_tooltip (&length_4_button, _("Set note length to a quarter note"), "");
	set_tooltip (&length_8_button, _("Set note length to a eighth note"), "");
	set_tooltip (&length_16_button, _("Set note length to a sixteenth note"), "");
	set_tooltip (&length_32_button, _("Set note length to a thirty-second note"), "");
	set_tooltip (&length_64_button, _("Set note length to a sixty-fourth note"), "");

	act = ActionManager::get_action ("StepEditing/note-velocity-ppp");
	velocity_ppp_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-pp");
	velocity_pp_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-p");
	velocity_p_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-mp");
	velocity_mp_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-mf");
	velocity_mf_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-f");
	velocity_f_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-ff");
	velocity_ff_button.set_related_action (act);
	act = ActionManager::get_action ("StepEditing/note-velocity-fff");
	velocity_fff_button.set_related_action (act);

	set_tooltip (&velocity_ppp_button, _("Set volume (velocity) to pianississimo"), "");
	set_tooltip (&velocity_pp_button, _("Set volume (velocity) to pianissimo"), "");
	set_tooltip (&velocity_p_button, _("Set volume (velocity) to piano"), "");
	set_tooltip (&velocity_mp_button, _("Set volume (velocity) to mezzo-piano"), "");
	set_tooltip (&velocity_mf_button, _("Set volume (velocity) to mezzo-forte"), "");
	set_tooltip (&velocity_f_button, _("Set volume (velocity) to forte"), "");
	set_tooltip (&velocity_ff_button, _("Set volume (velocity) to fortissimo"), "");
	set_tooltip (&velocity_fff_button, _("Set volume (velocity) to fortississimo"), "");

	note_velocity_box.pack_start (velocity_ppp_button, false, false);
	note_velocity_box.pack_start (velocity_pp_button, false, false);
	note_velocity_box.pack_start (velocity_p_button, false, false);
	note_velocity_box.pack_start (velocity_mp_button, false, false);
	note_velocity_box.pack_start (velocity_mf_button, false, false);
	note_velocity_box.pack_start (velocity_f_button, false, false);
	note_velocity_box.pack_start (velocity_ff_button, false, false);
	note_velocity_box.pack_start (velocity_fff_button, false, false);

	/* https://www.unicode.org/charts/PDF/U1D100.pdf */
	velocity_ppp_button.set_text (u8"\U0001D18F\U0001D18F\U0001D18F"); //MUSICAL SYMBOL PIANO (U+1D18F)
	velocity_pp_button.set_text (u8"\U0001D18F\U0001D18F");
	velocity_p_button.set_text (u8"\U0001D18F");
	velocity_mp_button.set_text (u8"\U0001D190\U0001D18F"); //MUSICAL SYMBOL MEZZO (U+1D190)
	velocity_mf_button.set_text (u8"\U0001D190\U0001D191");
	velocity_f_button.set_text (u8"\U0001D191"); // MUSICAL SYMBOL FORTE (U+1D191)
	velocity_ff_button.set_text (u8"\U0001D191\U0001D191");
	velocity_fff_button.set_text (u8"\U0001D191\U0001D191\U0001D191");

	length_1_button.set_text (u8"\U0001D15D"); // MUSICAL SYMBOL WHOLE NOTE
	length_2_button.set_text (u8"\U0001D15E"); // MUSICAL SYMBOL HALF NOTE
	length_4_button.set_text (u8"\U0001D15F"); // MUSICAL SYMBOL QUARTER NOTE
	length_8_button.set_text (u8"\U0001D160"); // MUSICAL SYMBOL EIGHTH NOTE
	length_16_button.set_text (u8"\U0001D161"); // MUSICAL SYMBOL SIXTEENTH NOTE
	length_32_button.set_text (u8"\U0001D162"); // MUSICAL SYMBOL THIRTY-SECOND NOTE
	length_64_button.set_text (u8"\U0001D163"); // MUSICAL SYMBOL SIXTY-FOURTH NOTE

	chord_button.set_text (u8"\U0001D1D6"); // MUSICAL SYMBOL SCANDICUS (customized in ArdourSans)

	Label* l = manage (new Label);
	l->set_markup ("<b><big>-</big></b>");
	l->show ();
	dot0_button.add (*l);

	l = manage (new Label);
	l->set_markup ("<b><big>.</big></b>");
	l->show ();
	dot1_button.add (*l);

	l = manage (new Label);
	l->set_markup ("<b><big>..</big></b>");
	l->show ();
	dot2_button.add (*l);

	l = manage (new Label);
	l->set_markup ("<b><big>...</big></b>");
	l->show ();
	dot3_button.add (*l);

	dot_box1.pack_start (dot0_button, true, false);
	dot_box1.pack_start (dot1_button, true, false);
	dot_box2.pack_start (dot2_button, true, false);
	dot_box2.pack_start (dot3_button, true, false);

	rest_box.pack_start (rest_button, true, false);
	rest_box.pack_start (grid_rest_button, true, false);
	rest_box.pack_start (back_button, true, false);

	resync_box.pack_start (beat_resync_button, true, false);
	resync_box.pack_start (bar_resync_button, true, false);
	resync_box.pack_start (resync_button, true, false);

	set_tooltip (&chord_button, _("Stack inserted notes to form a chord"), "");
	set_tooltip (&sustain_button, _("Extend selected notes by note length"), "");
	set_tooltip (&dot0_button, _("Use undotted note lengths"), "");
	set_tooltip (&dot1_button, _("Use dotted (* 1.5) note lengths"), "");
	set_tooltip (&dot2_button, _("Use double-dotted (* 1.75) note lengths"), "");
	set_tooltip (&dot3_button, _("Use triple-dotted (* 1.875) note lengths"), "");
	set_tooltip (&rest_button, _("Insert a note-length's rest"), "");
	set_tooltip (&grid_rest_button, _("Insert a grid-unit's rest"), "");
	set_tooltip (&beat_resync_button, _("Insert a rest until the next beat"), "");
	set_tooltip (&bar_resync_button, _("Insert a rest until the next bar"), "");
	set_tooltip (&bank_button, _("Insert a bank change message"), "");
	set_tooltip (&program_button, _("Insert a program change message"), "");
	set_tooltip (&back_button, _("Move Insert Position Back by Note Length"), "");
	set_tooltip (&resync_button, _("Move Insert Position to Edit Point"), "");

	act = ActionManager::get_action ("StepEditing/back");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (back_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (back_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/sync-to-edit-point");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (resync_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (resync_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/toggle-triplet");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (triplet_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (triplet_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/no-dotted");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (dot0_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (dot0_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/toggle-dotted");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (dot1_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (dot1_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/toggle-double-dotted");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (dot2_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (dot2_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/toggle-triple-dotted");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (dot3_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (dot3_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/insert-rest");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (rest_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (rest_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/insert-snap-rest");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (grid_rest_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (grid_rest_button.gobj()), act->gobj());
	act = ActionManager::get_action ("StepEditing/sustain");
	gtk_activatable_set_use_action_appearance (GTK_ACTIVATABLE (sustain_button.gobj()), false);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (sustain_button.gobj()), act->gobj());

	act = ActionManager::get_action ("StepEditing/toggle-chord");
	chord_button.set_related_action (act);

	upper_box.set_spacing (6);
	upper_box.pack_start (chord_button, false, false);
	upper_box.pack_start (note_length_box, false, false, 12);
	upper_box.pack_start (triplet_button, false, false);
	upper_box.pack_start (dot_box1, false, false);
	upper_box.pack_start (dot_box2, false, false);
	upper_box.pack_start (sustain_button, false, false);
	upper_box.pack_start (rest_box, false, false);
	upper_box.pack_start (resync_box, false, false);
	upper_box.pack_start (note_velocity_box, false, false, 12);

	VBox* v;

	v = manage (new VBox);
	l = manage (new Label (_("Channel")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (channel_spinner, false, false);
	upper_box.pack_start (*v, false, false);

	v = manage (new VBox);
	l = manage (new Label (_("1/Note")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (length_divisor_spinner, false, false);
	upper_box.pack_start (*v, false, false);

	v = manage (new VBox);
	l = manage (new Label (_("Velocity")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (velocity_spinner, false, false);
	upper_box.pack_start (*v, false, false);

	v = manage (new VBox);
	l = manage (new Label (_("Octave")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (octave_spinner, false, false);
	upper_box.pack_start (*v, false, false);

#if 0 // not implemented in StepEditor
	v = manage (new VBox);
	l = manage (new Label (_("Bank")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (bank_spinner, false, false);
	v->pack_start (bank_button, false, false);
	upper_box.pack_start (*v, false, false);

	v = manage (new VBox);
	l = manage (new Label (_("Program")));
	v->set_spacing (6);
	v->pack_start (*l, false, false);
	v->pack_start (program_spinner, false, false);
	v->pack_start (program_button, false, false);
	upper_box.pack_start (*v, false, false);
#endif

	velocity_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &StepEntry::velocity_value_change));
	length_divisor_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &StepEntry::length_value_change));
	dot_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &StepEntry::dot_value_change));

	_piano.set_can_focus ();

	_piano.NoteOff.connect (sigc::mem_fun (*this, &StepEntry::note_off_event_handler));
	_piano.Rest.connect (sigc::mem_fun (*this, &StepEntry::rest_event_handler));

	program_button.signal_clicked().connect (sigc::mem_fun (*this, &StepEntry::program_click));
	bank_button.signal_clicked().connect (sigc::mem_fun (*this, &StepEntry::bank_click));
	beat_resync_button.signal_clicked().connect (sigc::mem_fun (*this, &StepEntry::beat_resync_click));
	bar_resync_button.signal_clicked().connect (sigc::mem_fun (*this, &StepEntry::bar_resync_click));

	packer.set_spacing (6);
	packer.pack_start (upper_box, false, false);
	packer.pack_start (_piano, false, false);
	packer.show_all ();

	add (packer);

	/* initial settings: quarter note and mezzo forte */
	ActionManager::get_radio_action ("StepEditing/note-length-quarter")->set_active (true);
	ActionManager::get_radio_action ("StepEditing/note-velocity-mf")->set_active (true);
	length_value_change ();
	velocity_value_change ();
}

StepEntry::~StepEntry()
{
}

void
StepEntry::set_step_editor (StepEditor* seditor)
{
	if (se && se != seditor) {
		se->step_entry_done ();
	}

	se = seditor;

	if (se) {
		set_title (string_compose (_("Step Entry: %1"), se->name()));
#if 0
		/* set channel selector to first selected channel. if none
		   are selected, it will remain at the value set in its
		   constructor, above (1)
		*/

		uint16_t chn_mask = se->channel_selector().get_selected_channels();

		for (uint32_t i = 0; i < 16; ++i) {
			if (chn_mask & (1<<i)) {
				channel_adjustment.set_value (i+1);
				break;
			}
		}

#endif
	} else {
		hide ();
	}
}


bool
StepEntry::on_key_press_event (GdkEventKey* ev)
{
	/* focus widget gets first shot, then bindings, otherwise
	   forward to main window
	*/

	if (gtk_window_propagate_key_event (GTK_WINDOW(gobj()), ev)) {
		return true;
	}

	return relay_key_press (ev, this);
}

bool
StepEntry::on_key_release_event (GdkEventKey* ev)
{
	if (gtk_window_propagate_key_event (GTK_WINDOW(gobj()), ev)) {
		return true;
	}

	/* don't forward releases */

	return true;
}

void
StepEntry::rest_event_handler ()
{
	if (se) {
		se->step_edit_rest (Temporal::Beats());
	}
}

Temporal::Beats
StepEntry::note_length ()
{
	double base_time = 4.0 / (double) length_divisor_adjustment.get_value();

	RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("StepEditing/toggle-triplet");
	bool triplets = tact->get_active ();

	if (triplets) {
		base_time *= (2.0/3.0);
	}

	double dots = dot_adjustment.get_value ();

	if (dots > 0) {
		dots = pow (2.0, dots);
		base_time *= 1 + ((dots - 1.0)/dots);
	}

	return Temporal::Beats::from_double (base_time);
}

uint8_t
StepEntry::note_velocity () const
{
	return velocity_adjustment.get_value();
}

uint8_t
StepEntry::note_channel() const
{
	return channel_adjustment.get_value() - 1;
}

void
StepEntry::note_off_event_handler (int note)
{
	insert_note (note);
}


void
StepEntry::on_show ()
{
	ArdourWindow::on_show ();
	//_piano->grab_focus ();
}

void
StepEntry::beat_resync_click ()
{
	if (se) {
		se->step_edit_beat_sync ();
	}
}

void
StepEntry::bar_resync_click ()
{
	if (se) {
		se->step_edit_bar_sync ();
	}
}

void
StepEntry::register_actions ()
{
	/* add named actions for the step editor */

	Glib::RefPtr<ActionGroup> group = ActionManager::create_action_group (bindings, X_("StepEditing"));

	ActionManager::register_action (group, "insert-a", _("Insert Note A"), sigc::ptr_fun (&StepEntry::se_insert_a));
	ActionManager::register_action (group, "insert-asharp", _("Insert Note A-sharp"), sigc::ptr_fun (&StepEntry::se_insert_asharp));
	ActionManager::register_action (group, "insert-b", _("Insert Note B"), sigc::ptr_fun (&StepEntry::se_insert_b));
	ActionManager::register_action (group, "insert-c", _("Insert Note C"), sigc::ptr_fun (&StepEntry::se_insert_c));
	ActionManager::register_action (group, "insert-csharp", _("Insert Note C-sharp"), sigc::ptr_fun (&StepEntry::se_insert_csharp));
	ActionManager::register_action (group, "insert-d", _("Insert Note D"), sigc::ptr_fun (&StepEntry::se_insert_d));
	ActionManager::register_action (group, "insert-dsharp", _("Insert Note D-sharp"), sigc::ptr_fun (&StepEntry::se_insert_dsharp));
	ActionManager::register_action (group, "insert-e", _("Insert Note E"), sigc::ptr_fun (&StepEntry::se_insert_e));
	ActionManager::register_action (group, "insert-f", _("Insert Note F"), sigc::ptr_fun (&StepEntry::se_insert_f));
	ActionManager::register_action (group, "insert-fsharp", _("Insert Note F-sharp"), sigc::ptr_fun (&StepEntry::se_insert_fsharp));
	ActionManager::register_action (group, "insert-g", _("Insert Note G"), sigc::ptr_fun (&StepEntry::se_insert_g));
	ActionManager::register_action (group, "insert-gsharp", _("Insert Note G-sharp"), sigc::ptr_fun (&StepEntry::se_insert_gsharp));

	ActionManager::register_action (group, "insert-rest", _("Insert a Note-length Rest"), sigc::ptr_fun (&StepEntry::se_insert_rest));
	ActionManager::register_action (group, "insert-snap-rest", _("Insert a Snap-length Rest"), sigc::ptr_fun (&StepEntry::se_insert_grid_rest));

	ActionManager::register_action (group, "next-octave", _("Move to next octave"), sigc::ptr_fun (&StepEntry::se_next_octave));
	ActionManager::register_action (group, "prev-octave", _("Move to next octave"), sigc::ptr_fun (&StepEntry::se_prev_octave));

	ActionManager::register_action (group, "next-note-length", _("Move to Next Note Length"), sigc::ptr_fun (&StepEntry::se_next_note_length));
	ActionManager::register_action (group, "prev-note-length", _("Move to Previous Note Length"), sigc::ptr_fun (&StepEntry::se_prev_note_length));

	ActionManager::register_action (group, "inc-note-length", _("Increase Note Length"), sigc::ptr_fun (&StepEntry::se_inc_note_length));
	ActionManager::register_action (group, "dec-note-length", _("Decrease Note Length"), sigc::ptr_fun (&StepEntry::se_dec_note_length));

	ActionManager::register_action (group, "next-note-velocity", _("Move to Next Note Velocity"), sigc::ptr_fun (&StepEntry::se_next_note_velocity));
	ActionManager::register_action (group, "prev-note-velocity", _("Move to Previous Note Velocity"), sigc::ptr_fun (&StepEntry::se_prev_note_velocity));

	ActionManager::register_action (group, "inc-note-velocity", _("Increase Note Velocity"), sigc::ptr_fun (&StepEntry::se_inc_note_velocity));
	ActionManager::register_action (group, "dec-note-velocity", _("Decrease Note Velocity"), sigc::ptr_fun (&StepEntry::se_dec_note_velocity));

	ActionManager::register_action (group, "octave-0", _("Switch to the 1st octave"), sigc::ptr_fun (&StepEntry::se_octave_0));
	ActionManager::register_action (group, "octave-1", _("Switch to the 2nd octave"), sigc::ptr_fun (&StepEntry::se_octave_1));
	ActionManager::register_action (group, "octave-2", _("Switch to the 3rd octave"), sigc::ptr_fun (&StepEntry::se_octave_2));
	ActionManager::register_action (group, "octave-3", _("Switch to the 4th octave"), sigc::ptr_fun (&StepEntry::se_octave_3));
	ActionManager::register_action (group, "octave-4", _("Switch to the 5th octave"), sigc::ptr_fun (&StepEntry::se_octave_4));
	ActionManager::register_action (group, "octave-5", _("Switch to the 6th octave"), sigc::ptr_fun (&StepEntry::se_octave_5));
	ActionManager::register_action (group, "octave-6", _("Switch to the 7th octave"), sigc::ptr_fun (&StepEntry::se_octave_6));
	ActionManager::register_action (group, "octave-7", _("Switch to the 8th octave"), sigc::ptr_fun (&StepEntry::se_octave_7));
	ActionManager::register_action (group, "octave-8", _("Switch to the 9th octave"), sigc::ptr_fun (&StepEntry::se_octave_8));
	ActionManager::register_action (group, "octave-9", _("Switch to the 10th octave"), sigc::ptr_fun (&StepEntry::se_octave_9));
	ActionManager::register_action (group, "octave-10", _("Switch to the 11th octave"), sigc::ptr_fun (&StepEntry::se_octave_10));

	ActionManager::register_toggle_action (group, "toggle-triplet", _("Toggle Triple Notes"), sigc::ptr_fun (&StepEntry::se_toggle_triplet));

	ActionManager::register_toggle_action (group, "toggle-chord", _("Toggle Chord Entry"), sigc::ptr_fun (&StepEntry::se_toggle_chord));
	ActionManager::register_action (group, "sustain", _("Sustain Selected Notes by Note Length"), sigc::ptr_fun (&StepEntry::se_do_sustain));

	ActionManager::register_action (group, "sync-to-edit-point", _("Move Insert Position to Edit Point"), sigc::ptr_fun (&StepEntry::se_sync_to_edit_point));
	ActionManager::register_action (group, "back", _("Move Insert Position Back by Note Length"), sigc::ptr_fun (&StepEntry::se_back));

	RadioAction::Group note_length_group;

	ActionManager::register_radio_action (group, note_length_group, "note-length-whole",
	                                 _("Set Note Length to Whole"), sigc::ptr_fun (&StepEntry::se_note_length_change), 1);
	ActionManager::register_radio_action (group, note_length_group, "note-length-half",
	                                 _("Set Note Length to 1/2"), sigc::ptr_fun (&StepEntry::se_note_length_change), 2);
	ActionManager::register_radio_action (group, note_length_group, "note-length-third",
	                                 _("Set Note Length to 1/3"), sigc::ptr_fun (&StepEntry::se_note_length_change), 3);
	ActionManager::register_radio_action (group, note_length_group, "note-length-quarter",
	                                 _("Set Note Length to 1/4"), sigc::ptr_fun (&StepEntry::se_note_length_change), 4);
	ActionManager::register_radio_action (group, note_length_group, "note-length-eighth",
	                                 _("Set Note Length to 1/8"), sigc::ptr_fun (&StepEntry::se_note_length_change), 8);
	ActionManager::register_radio_action (group, note_length_group, "note-length-sixteenth",
	                                 _("Set Note Length to 1/16"), sigc::ptr_fun (&StepEntry::se_note_length_change), 16);
	ActionManager::register_radio_action (group, note_length_group, "note-length-thirtysecond",
	                                 _("Set Note Length to 1/32"), sigc::ptr_fun (&StepEntry::se_note_length_change), 32);
	ActionManager::register_radio_action (group, note_length_group, "note-length-sixtyfourth",
	                                 _("Set Note Length to 1/64"), sigc::ptr_fun (&StepEntry::se_note_length_change), 64);

	RadioAction::Group note_velocity_group;

	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-ppp",
	                                 _("Set Note Velocity to Pianississimo"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 1);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-pp",
	                                 _("Set Note Velocity to Pianissimo"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 16);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-p",
	                                 _("Set Note Velocity to Piano"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 32);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-mp",
	                                 _("Set Note Velocity to Mezzo-Piano"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 64);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-mf",
	                                 _("Set Note Velocity to Mezzo-Forte"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 80);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-f",
	                                 _("Set Note Velocity to Forte"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 96);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-ff",
	                                 _("Set Note Velocity to Fortississimo"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 112);
	ActionManager::register_radio_action (group, note_velocity_group, "note-velocity-fff",
	                                 _("Set Note Velocity to Fortississimo"), sigc::ptr_fun (&StepEntry::se_note_velocity_change), 127);


	RadioAction::Group dot_group;

	ActionManager::register_radio_action (group, dot_group, "no-dotted", _("No Dotted Notes"), sigc::ptr_fun (&StepEntry::se_dot_change), 0);
	ActionManager::register_radio_action (group, dot_group, "toggle-dotted", _("Toggled Dotted Notes"), sigc::ptr_fun (&StepEntry::se_dot_change), 1);
	ActionManager::register_radio_action (group, dot_group, "toggle-double-dotted", _("Toggled Double-Dotted Notes"), sigc::ptr_fun (&StepEntry::se_dot_change), 2);
	ActionManager::register_radio_action (group, dot_group, "toggle-triple-dotted", _("Toggled Triple-Dotted Notes"), sigc::ptr_fun (&StepEntry::se_dot_change), 3);
}

void
StepEntry::setup_actions_and_bindings ()
{
	load_bindings ();
	register_actions ();
}

void
StepEntry::load_bindings ()
{
	bindings = Bindings::get_bindings (X_("Step Editing"));
}

void
StepEntry::toggle_triplet ()
{
	if (se) {
		se->set_step_edit_cursor_width (note_length());
	}
}

void
StepEntry::toggle_chord ()
{
	if (se) {
		se->step_edit_toggle_chord ();
	}
}

void
StepEntry::dot_change (GtkRadioAction* act)
{
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(act))) {
		gint v = gtk_radio_action_get_current_value (act);
		dot_adjustment.set_value (v);
	}
}

void
StepEntry::dot_value_change ()
{
	RefPtr<Action> act;
	RefPtr<RadioAction> ract;
	double val = dot_adjustment.get_value();
	bool inconsistent = true;
	vector<const char*> dot_actions;

	dot_actions.push_back ("StepEditing/no-dotted");
	dot_actions.push_back ("StepEditing/toggle-dotted");
	dot_actions.push_back ("StepEditing/toggle-double-dotted");
	dot_actions.push_back ("StepEditing/toggle-triple-dotted");

	for (vector<const char*>::iterator i = dot_actions.begin(); i != dot_actions.end(); ++i) {

		act = ActionManager::get_action (*i);

		if (act) {
			ract = RefPtr<RadioAction>::cast_dynamic (act);

			if (ract) {
				if (ract->property_value() == val) {
					ract->set_active (true);
					inconsistent = false;
					break;
				}
			}
		}
	}

	dot1_button.set_inconsistent (inconsistent);
	dot2_button.set_inconsistent (inconsistent);
	dot3_button.set_inconsistent (inconsistent);

	if (se) {
		se->set_step_edit_cursor_width (note_length());
	}
}

void
StepEntry::program_click ()
{
	if (se) {
		se->step_add_program_change (note_channel(), (int8_t) floor (program_adjustment.get_value()));
	}
}

void
StepEntry::bank_click ()
{
	if (se) {
		se->step_add_bank_change (note_channel(), (int8_t) floor (bank_adjustment.get_value()));
	}
}

void
StepEntry::insert_rest ()
{
	if (se) {
		se->step_edit_rest (note_length());
	}
}

void
StepEntry::insert_grid_rest ()
{
	if (se) {
		se->step_edit_rest (Temporal::Beats());
	}
}

void
StepEntry::insert_note (uint8_t note)
{
	if (note > 127) {
		return;
	}

	if (se) {
		se->step_add_note (note_channel(), note, note_velocity(), note_length());
	}
}
void
StepEntry::insert_c ()
{
	insert_note (0 + (current_octave() * 12));
}
void
StepEntry::insert_csharp ()
{
	insert_note (1 + (current_octave() * 12));
}
void
StepEntry::insert_d ()
{
	insert_note (2 + (current_octave() * 12));
}
void
StepEntry::insert_dsharp ()
{
	insert_note (3 + (current_octave() * 12));
}
void
StepEntry::insert_e ()
{
	insert_note (4 + (current_octave() * 12));
}
void
StepEntry::insert_f ()
{
	insert_note (5 + (current_octave() * 12));
}
void
StepEntry::insert_fsharp ()
{
	insert_note (6 + (current_octave() * 12));
}
void
StepEntry::insert_g ()
{
	insert_note (7 + (current_octave() * 12));
}
void
StepEntry::insert_gsharp ()
{
	insert_note (8 + (current_octave() * 12));
}

void
StepEntry::insert_a ()
{
	insert_note (9 + (current_octave() * 12));
}

void
StepEntry::insert_asharp ()
{
	insert_note (10 + (current_octave() * 12));
}
void
StepEntry::insert_b ()
{
	insert_note (11 + (current_octave() * 12));
}

void
StepEntry::note_length_change (GtkRadioAction* act)
{
	/* it doesn't matter which note length action we look up - we are interested
	   in the current_value which is global across the whole group of note length
	   actions. this method is called twice for every user operation,
	   once for the action that became "inactive" and once for the action that
	   becaome "active". so ... only bother to actually change the value when this
	   is called for the "active" action.
	*/

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(act))) {
		gint v = gtk_radio_action_get_current_value (act);
		length_divisor_adjustment.set_value (v);
	}
}

void
StepEntry::note_velocity_change (GtkRadioAction* act)
{
	/* it doesn't matter which note velocity action we look up - we are interested
	   in the current_value which is global across the whole group of note velocity
	   actions. this method is called twice for every user operation,
	   once for the action that became "inactive" and once for the action that
	   becaome "active". so ... only bother to actually change the value when this
	   is called for the "active" action.
	*/

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(act))) {
		gint v = gtk_radio_action_get_current_value (act);
		velocity_adjustment.set_value (v);
	}
}

void
StepEntry::velocity_value_change ()
{
	RefPtr<Action> act;
	RefPtr<RadioAction> ract;
	double val = velocity_adjustment.get_value();
	bool inconsistent = true;
	vector<const char*> velocity_actions;

	velocity_actions.push_back ("StepEditing/note-velocity-ppp");
	velocity_actions.push_back ("StepEditing/note-velocity-pp");
	velocity_actions.push_back ("StepEditing/note-velocity-p");
	velocity_actions.push_back ("StepEditing/note-velocity-mp");
	velocity_actions.push_back ("StepEditing/note-velocity-mf");
	velocity_actions.push_back ("StepEditing/note-velocity-f");
	velocity_actions.push_back ("StepEditing/note-velocity-ff");
	velocity_actions.push_back ("StepEditing/note-velocity-fff");

	for (vector<const char*>::iterator i = velocity_actions.begin(); i != velocity_actions.end(); ++i) {

		act = ActionManager::get_action (*i);

		if (act) {
			ract = RefPtr<RadioAction>::cast_dynamic (act);

			if (ract) {
				if (ract->property_value() == val) {
					ract->set_active (false);
					ract->set_active (true);
					inconsistent = false;
					break;
				}
			}
		}
	}

	if (inconsistent) {
		velocity_ppp_button.unset_active_state ();
		velocity_pp_button.unset_active_state ();
		velocity_p_button.unset_active_state ();
		velocity_mp_button.unset_active_state ();
		velocity_mf_button.unset_active_state ();
		velocity_f_button.unset_active_state ();
		velocity_ff_button.unset_active_state ();
		velocity_fff_button.unset_active_state ();
	}
}

void
StepEntry::length_value_change ()
{
	RefPtr<Action> act;
	RefPtr<RadioAction> ract;
	double val = length_divisor_adjustment.get_value();
	bool inconsistent = true;
	vector<const char*> length_actions;

	length_actions.push_back ("StepEditing/note-length-whole");
	length_actions.push_back ("StepEditing/note-length-half");
	length_actions.push_back ("StepEditing/note-length-quarter");
	length_actions.push_back ("StepEditing/note-length-eighth");
	length_actions.push_back ("StepEditing/note-length-sixteenth");
	length_actions.push_back ("StepEditing/note-length-thirtysecond");
	length_actions.push_back ("StepEditing/note-length-sixtyfourth");

	for (vector<const char*>::iterator i = length_actions.begin(); i != length_actions.end(); ++i) {

		Glib::RefPtr<RadioAction> ract = ActionManager::get_radio_action (*i);
		if (ract) {
			if (ract->property_value() == val) {
				ract->set_active (false);
				ract->set_active (true);
				inconsistent = false;
				break;
			}
		}
	}

	if (inconsistent) {
		length_1_button.unset_active_state ();
		length_2_button.unset_active_state ();
		length_4_button.unset_active_state ();
		length_8_button.unset_active_state ();
		length_16_button.unset_active_state ();
		length_32_button.unset_active_state ();
		length_64_button.unset_active_state ();
	}

	if (se) {
		se->set_step_edit_cursor_width (note_length());
	}
}

void
StepEntry::next_octave ()
{
	octave_adjustment.set_value (octave_adjustment.get_value() + 1.0);
}

void
StepEntry::prev_octave ()
{
	octave_adjustment.set_value (octave_adjustment.get_value() - 1.0);
}

void
StepEntry::inc_note_length ()
{
	length_divisor_adjustment.set_value (length_divisor_adjustment.get_value() - 1.0);
}

void
StepEntry::dec_note_length ()
{
	length_divisor_adjustment.set_value (length_divisor_adjustment.get_value() + 1.0);
}

void
StepEntry::prev_note_length ()
{
	double l = length_divisor_adjustment.get_value();
	int il = (int) lrintf (l); // round to nearest integer
	il = (il/2) * 2; // round to power of 2

	if (il == 0) {
		il = 1;
	}

	il *= 2; // double

	length_divisor_adjustment.set_value (il);
}

void
StepEntry::next_note_length ()
{
	double l = length_divisor_adjustment.get_value();
	int il = (int) lrintf (l); // round to nearest integer
	il = (il/2) * 2; // round to power of 2

	if (il == 0) {
		il = 1;
	}

	il /= 2; // half

	if (il > 0) {
		length_divisor_adjustment.set_value (il);
	}
}

void
StepEntry::inc_note_velocity ()
{
	velocity_adjustment.set_value (velocity_adjustment.get_value() + 1.0);
}

void
StepEntry::dec_note_velocity ()
{
	velocity_adjustment.set_value (velocity_adjustment.get_value() - 1.0);
}

void
StepEntry::next_note_velocity ()
{
	double l = velocity_adjustment.get_value ();

	if (l < 16) {
		l = 16;
	} else if (l < 32) {
		l = 32;
	} else if (l < 48) {
		l = 48;
	} else if (l < 64) {
		l = 64;
	} else if (l < 80) {
		l = 80;
	} else if (l < 96) {
		l = 96;
	} else if (l < 112) {
		l = 112;
	} else if (l < 127) {
		l = 127;
	}

	velocity_adjustment.set_value (l);
}

void
StepEntry::prev_note_velocity ()
{
	double l = velocity_adjustment.get_value ();

	if (l > 112) {
		l = 112;
	} else if (l > 96) {
		l = 96;
	} else if (l > 80) {
		l = 80;
	} else if (l > 64) {
		l = 64;
	} else if (l > 48) {
		l = 48;
	} else if (l > 32) {
		l = 32;
	} else if (l > 16) {
		l = 16;
	} else {
		l = 1;
	}

	velocity_adjustment.set_value (l);
}

void
StepEntry::octave_n (int n)
{
	octave_adjustment.set_value (n);
}

void
StepEntry::do_sustain ()
{
	if (se) {
		se->step_edit_sustain (note_length());
	}
}

void
StepEntry::back ()
{
	if (se) {
		se->move_step_edit_beat_pos (-note_length());
	}
}

void
StepEntry::sync_to_edit_point ()
{
	if (se) {
		se->resync_step_edit_to_edit_point ();
	}
}
