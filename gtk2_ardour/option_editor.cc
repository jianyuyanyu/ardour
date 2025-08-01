/*
 * Copyright (C) 2005-2006 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2008-2014 David Robillard <d@drobilla.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2015 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2016 Julien "_FrnchFrgg_" RIVAUD <frnchfrgg@free.fr>
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

#include <ctype.h>
#include <algorithm>

#include <boost/tokenizer.hpp>

#include <ytkmm/box.h>
#include <ytkmm/alignment.h>
#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/colors.h"

#include "ardour/dB.h"
#include "ardour/rc_configuration.h"
#include "ardour/session.h"
#include "ardour/types.h"
#include "ardour/utils.h"

#include "pbd/configuration.h"
#include "pbd/replace_all.h"
#include "pbd/openuri.h"
#include "pbd/strsplit.h"

#include "widgets/frame.h"
#include "widgets/slider_controller.h"

#include "gui_thread.h"
#include "option_editor.h"
#include "public_editor.h"
#include "ui_config.h"
#include "utils.h"
#include "pbd/i18n.h"

using namespace std;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace ARDOUR;

void
OptionEditorComponent::add_widget_to_page (OptionEditorPage* p, Gtk::Widget* w)
{
	int const n = p->table.property_n_rows();
	int m = n + 1;
	if (!_note.empty ()) {
		++m;
	}
	_frame = manage (new ArdourWidgets::Frame);
	_frame->add (*w);
	_frame->set_draw (false);
	_frame->set_edge_color (UIConfiguration::instance().color (X_("preference highlight")));

	p->table.resize (m, 3);
	p->table.attach (*_frame, 1, 3, n, n + 1, FILL | EXPAND);

	maybe_add_note (p, n + 1);
}

void
OptionEditorComponent::add_widgets_to_page (OptionEditorPage* p, Gtk::Widget* wa, Gtk::Widget* wb, bool /*notused*/)
{
	int const n = p->table.property_n_rows();
	int m = n + 1;
	if (!_note.empty ()) {
		++m;
	}

	_frame = manage (new ArdourWidgets::Frame);
	_frame->add (*wa);
	_frame->set_draw (false);
	_frame->set_edge_color (UIConfiguration::instance().color (X_("preference highlight")));

	p->table.resize (m, 3);
	p->table.attach (*_frame, 1, 2, n, n + 1, FILL);

	Alignment* a = manage (new Alignment (0, 0.5, 0, 1.0));
	a->add (*wb);
	p->table.attach (*a, 2, 3, n, n + 1, FILL | EXPAND);

	maybe_add_note (p, n + 1);
}

void
OptionEditorComponent::maybe_add_note (OptionEditorPage* p, int n)
{
	if (!_note.empty ()) {
		Gtk::Label* l = manage (left_aligned_label (string_compose (X_("<i>%1</i>"), _note)));
		l->set_use_markup (true);
		l->set_line_wrap (true);
		p->table.attach (*l, 1, 3, n, n + 1, FILL | EXPAND);
		if (_note.find ("<a href=") != _note.npos) {
			l->property_track_visited_links() = false;
			l->signal_activate_link().connect ([](std::string const& url) { return PBD::open_uri (url); });
		}
	}
}

void
OptionEditorComponent::set_note (string const & n)
{
	_note = n;
}

void
OptionEditorComponent::highlight ()
{
	if (_frame) {
		_frame->set_draw (true);
	}
}

void
OptionEditorComponent::end_highlight ()
{
	if (_frame) {
		_frame->set_draw (false);
	}
}

PBD::Configuration::Metadata const *
OptionEditorComponent::get_metadata () const
{
	return _metadata;
}

void
OptionEditorComponent::set_metadata (PBD::Configuration::Metadata const & m)
{
	_metadata = &m;
}

/*--------------------------*/

OptionEditorHeading::OptionEditorHeading (string const & h)
{
	std::stringstream s;
	s << "<b>" << h << "</b>";
	_label = manage (left_aligned_label (s.str()));
	_label->set_use_markup (true);
}

/*--------------------------*/

void
OptionEditorHeading::add_to_page (OptionEditorPage* p)
{
	int const n = p->table.property_n_rows();
	if (!_note.empty ()) {
		p->table.resize (n + 3, 3);
	} else {
		p->table.resize (n + 2, 3);
	}

	p->table.attach (*manage (new Label ("")), 0, 3, n, n + 1, FILL | EXPAND);
	p->table.attach (*_label, 0, 3, n + 1, n + 2, FILL | EXPAND);
	maybe_add_note (p, n + 2);
}

/*--------------------------*/

void
OptionEditorBlank::add_to_page (OptionEditorPage* p)
{
	int const n = p->table.property_n_rows();
	p->table.resize (n + 1, 3);
	p->table.attach (_dummy, 2, 3, n, n + 1, FILL | EXPAND, SHRINK, 0, 0);
	_dummy.set_size_request (-1, 1);
	_dummy.show ();
}

/*--------------------------*/

RcConfigDisplay::RcConfigDisplay (string const & i, string const & n, sigc::slot<string> g, char s)
	: _get (g)
	, _id (i)
	, _sep (s)
{
	_label = manage (right_aligned_label (n));
	_info = manage (new Label);
	_info-> set_line_wrap (true);
	set_state_from_config ();
}

void
RcConfigDisplay::set_state_from_config ()
{
	string p = _get();
	if (_sep) {
		std::replace (p.begin(), p.end(), _sep, '\n');
	}
	_info->set_text (p);
}

void
RcConfigDisplay::parameter_changed (std::string const & p)
{
	if (p == _id) {
		set_state_from_config ();
	}
}

void
RcConfigDisplay::add_to_page (OptionEditorPage *p)
{
	int const n = p->table.property_n_rows();
	int m = n + 1;
	p->table.resize (m, 3);
	p->table.attach (*_label, 1, 2, n, n + 1, FILL | EXPAND);
	p->table.attach (*_info,  2, 3, n, n + 1, FILL | EXPAND);
}

/*--------------------------*/

RcActionButton::RcActionButton (std::string const & t, const Glib::SignalProxy0< void >::SlotType & slot, std::string const & l)
	: _label (NULL)
{
	_button = manage (new Button (t));
	_button->signal_clicked().connect (slot);
	if (!l.empty ()) {
		_label = manage (right_aligned_label (l));
	}
}

void
RcActionButton::add_to_page (OptionEditorPage *p)
{
	int const n = p->table.property_n_rows();
	int m = n + 1;
	if (!_note.empty ()) {
		++m;
	}
	p->table.resize (m, 3);
	Alignment* a = manage (new Alignment (0, 0.5, 0, 1.0));
	a->add (*_button);

	if (_label) {
		p->table.attach (*_label,  1, 2, n, n + 1);
		p->table.attach (*a, 2, 3, n, n + 1, FILL|EXPAND);
	} else {
		p->table.attach (*a, 1, 3, n, n + 1, FILL|EXPAND);
	}
	maybe_add_note (p, n + 1);
}

/*--------------------------*/

CheckOption::CheckOption (string const & i, string const & n, Glib::RefPtr<Gtk::Action> act)
{
	_button = manage (new CheckButton);
	_label = manage (new Label);
	_label->set_markup (n);
	_button->add (*_label);
	_button->signal_toggled().connect (sigc::mem_fun (*this, &CheckOption::toggled));

	Gtkmm2ext::Activatable::set_related_action (act);
	assert (_action);

	action_sensitivity_changed ();

	Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (_action);
	if (tact) {
		action_toggled ();
		tact->signal_toggled().connect (sigc::mem_fun (*this, &CheckOption::action_toggled));
	}

	_action->connect_property_changed ("sensitive", sigc::mem_fun (*this, &CheckOption::action_sensitivity_changed));
}

void
CheckOption::action_toggled ()
{
	Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (_action);
	if (tact) {
		_button->set_active (tact->get_active());
	}
}

void
CheckOption::add_to_page (OptionEditorPage* p)
{
	add_widget_to_page (p, _button);
}

void
CheckOption::toggled ()
{
	Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic (_action);

	tact->set_active (_button->get_active ());
}


/*--------------------------*/

BoolOption::BoolOption (string const & i, string const & n, sigc::slot<bool> g, sigc::slot<bool, bool> s)
	: Option (i, n),
	  _get (g),
	  _set (s)
{
	_button = manage (new CheckButton);
	_label = manage (new Label);
	_label->set_markup (n);
	_button->add (*_label);
	_button->set_active (_get ());
	_button->signal_toggled().connect (sigc::mem_fun (*this, &BoolOption::toggled));
}

void
BoolOption::add_to_page (OptionEditorPage* p)
{
	add_widget_to_page (p, _button);
}

void
BoolOption::set_state_from_config ()
{
	_button->set_active (_get ());
}

void
BoolOption::toggled ()
{
	if (!_set (_button->get_active ())) {
		_button->set_active (_get ());
	}
}

/*--------------------------*/

RouteDisplayBoolOption::RouteDisplayBoolOption (string const & i, string const & n, sigc::slot<bool> g, sigc::slot<bool, bool> s)
	: BoolOption (i, n, g, s)
{
}

void
RouteDisplayBoolOption::toggled ()
{
	DisplaySuspender ds;
	BoolOption::toggled ();
}

/*--------------------------*/

EntryOption::EntryOption (string const & i, string const & n, sigc::slot<string> g, sigc::slot<bool, string> s)
	: Option (i, n),
	  _get (g),
	  _set (s)
{
	_label = manage (left_aligned_label (n + ":"));
	_entry = manage (new Entry);
	_entry->signal_activate().connect (sigc::mem_fun (*this, &EntryOption::activated));
	_entry->signal_focus_out_event().connect (sigc::mem_fun (*this, &EntryOption::focus_out));
	_entry->signal_insert_text().connect (sigc::mem_fun (*this, &EntryOption::filter_text));
}

void
EntryOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, _entry);
}

void
EntryOption::set_state_from_config ()
{
	_entry->set_text (_get ());
}

void
EntryOption::set_sensitive (bool s)
{
	_entry->set_sensitive (s);
}

void
EntryOption::filter_text (const Glib::ustring&, int*)
{
	std::string text = _entry->get_text ();

	if (!_valid.empty()) {
		for (std::string::const_iterator t = text.begin(); t != text.end(); ) {
			if (_valid.find_first_of (*t) == std::string::npos) {
				t = text.erase (t);
			} else {
				++t;
			}
		}
	}

	for (size_t i = 0; i < _invalid.length(); ++i) {
		text.erase (std::remove(text.begin(), text.end(), _invalid.at(i)), text.end());
	}

	if (text != _entry->get_text ()) {
		_entry->set_text (text);
	}
}

void
EntryOption::activated ()
{
	_set (_entry->get_text ());
}

bool
EntryOption::focus_out (GdkEventFocus*)
{
	_set (_entry->get_text ());
	return true;
}

/*--------------------------*/
HSliderOption::HSliderOption (
		std::string const& i,
		std::string const& n,
		sigc::slot<float> g,
		sigc::slot<bool, float> s,
		double lower, double upper,
		double step_increment,
		double page_increment,
		double mult,
		bool logarithmic
		)
	: Option (i, n)
	, _get (g)
	, _set (s)
	, _adj (lower, lower, upper, step_increment, page_increment, 0)
	, _hscale (_adj)
	, _mult (mult)
	, _log (logarithmic)
{
	_label = manage (left_aligned_label (n + ":"));
	_label->set_name ("OptionsLabel");
	_adj.set_value (_get());
	_adj.signal_value_changed().connect (sigc::mem_fun (*this, &HSliderOption::changed));
	_hscale.set_update_policy (Gtk::UPDATE_DISCONTINUOUS);

	/* make the slider be a fixed, font-relative width */

	_hscale.ensure_style ();
	int width, height;
	_hscale.create_pango_layout (X_("a piece of text that is as wide sliders should be"))->get_pixel_size (width, height);
	_hscale.set_size_request (width, -1);
}

void
HSliderOption::set_state_from_config ()
{
	if (_log) {
		_adj.set_value (log10(_get()) / _mult);
	} else {
		_adj.set_value (_get() / _mult);
	}
}

void
HSliderOption::changed ()
{
	if (_log) {
		_set (pow (10, _adj.get_value () * _mult));
	} else {
		_set (_adj.get_value () * _mult);
	}
}

void
HSliderOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, &_hscale);
}

void
HSliderOption::set_sensitive (bool yn)
{
	_hscale.set_sensitive (yn);
}

/*--------------------------*/

ComboStringOption::ComboStringOption (
		std::string const & i,
		std::string const & n,
		sigc::slot<std::string> g,
		sigc::slot<bool, std::string> s
		)
	: Option (i, n)
	, _get (g)
	, _set (s)
{
	_label = Gtk::manage (new Gtk::Label (n + ":"));
	_label->set_alignment (0, 0.5);
	_combo = Gtk::manage (new Gtk::ComboBoxText);
	_combo->signal_changed().connect (sigc::mem_fun (*this, &ComboStringOption::changed));
}

void
ComboStringOption::set_state_from_config () {
	_combo->set_active_text (_get());
}

void
ComboStringOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, _combo);
}

/** Set the allowed strings for this option
 *  @param strings a vector of allowed strings
 */
void
ComboStringOption::set_popdown_strings (const std::vector<std::string>& strings) {
	_combo->remove_all ();
	for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); ++i) {
		_combo->append (*i);
	}
}

void
ComboStringOption::clear () {
	_combo->remove_all();
}

void
ComboStringOption::changed () {
	_set (_combo->get_active_text ());
}

void
ComboStringOption::set_sensitive (bool yn) {
	_combo->set_sensitive (yn);
}

/*--------------------------*/

/** Construct a BoolComboOption.
 *  @param i id
 *  @param n User-visible name.
 *  @param t Text to give for the variable being true.
 *  @param f Text to give for the variable being false.
 *  @param g Slot to get the variable's value.
 *  @param s Slot to set the variable's value.
 */
BoolComboOption::BoolComboOption (
	string const & i, string const & n, string const & t, string const & f,
	sigc::slot<bool> g, sigc::slot<bool, bool> s
	)
	: Option (i, n)
	, _get (g)
	, _set (s)
{
	_label = manage (new Label (n + ":"));
	_label->set_alignment (0, 0.5);
	_combo = manage (new ComboBoxText);

	/* option 0 is the false option */
	_combo->append (f);
	/* and option 1 is the true */
	_combo->append (t);

	_combo->signal_changed().connect (sigc::mem_fun (*this, &BoolComboOption::changed));
}

void
BoolComboOption::set_state_from_config ()
{
	_combo->set_active (_get() ? 1 : 0);
}

void
BoolComboOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, _combo);
}

void
BoolComboOption::changed ()
{
	_set (_combo->get_active_row_number () == 0 ? false : true);
}

void
BoolComboOption::set_sensitive (bool yn)
{
	_combo->set_sensitive (yn);
}

/*--------------------------*/

FaderOption::FaderOption (string const & i, string const & n, sigc::slot<gain_t> g, sigc::slot<bool, gain_t> s)
	: Option (i, n)
	, _db_adjustment (gain_to_slider_position_with_max (1.0, Config->get_max_gain()), 0, 1, 0.01, 0.1)
	, _get (g)
	, _set (s)
{
	_db_slider = manage (new ArdourWidgets::HSliderController (&_db_adjustment, std::shared_ptr<PBD::Controllable>(), 220, 18));

	_label = manage (left_aligned_label (n + ":"));
	_label->set_name (X_("OptionsLabel"));

	_fader_centering_box.pack_start (*_db_slider, true, false);

	_box.set_spacing (4);
	_box.set_homogeneous (false);
	_box.pack_start (_fader_centering_box, false, false);
	_box.pack_start (_db_display, false, false);
	_box.pack_start (*manage (new Label ("dB")), false, false);
	_box.show_all ();

	set_size_request_to_display_given_text (_db_display, "-99.00", 12, 0);

	_db_adjustment.signal_value_changed().connect (sigc::mem_fun (*this, &FaderOption::db_changed));
	_db_display.signal_activate().connect (sigc::mem_fun (*this, &FaderOption::on_activate));
	_db_display.signal_key_press_event().connect (sigc::mem_fun (*this, &FaderOption::on_key_press), false);
}

void
FaderOption::set_state_from_config ()
{
	gain_t const val = _get ();
	_db_adjustment.set_value (gain_to_slider_position_with_max (val, Config->get_max_gain ()));

	char buf[16];

	if (val == 0.0) {
		snprintf (buf, sizeof (buf), "-inf");
	} else {
		snprintf (buf, sizeof (buf), "%.2f", accurate_coefficient_to_dB (val));
	}

	_db_display.set_text (buf);
}

void
FaderOption::db_changed ()
{
	_set (slider_position_to_gain_with_max (_db_adjustment.get_value (), Config->get_max_gain()));
}

void
FaderOption::on_activate ()
{
	float db_val = atof (_db_display.get_text ().c_str ());
	gain_t coeff_val = dB_to_coefficient (db_val);

	_db_adjustment.set_value (gain_to_slider_position_with_max (coeff_val, Config->get_max_gain ()));
}

bool
FaderOption::on_key_press (GdkEventKey* ev)
{
	if (ARDOUR_UI_UTILS::key_is_legal_for_numeric_entry (ev->keyval)) {
		/* drop through to normal handling */
		return false;
	}
	/* illegal key for gain entry */
	return true;
}

void
FaderOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, &_box);
}

Gtk::Widget&
FaderOption::tip_widget() {
	return *_db_slider;
}

/*--------------------------*/

ClockOption::ClockOption (string const & i, string const & n, sigc::slot<std::string> g, sigc::slot<bool, std::string> s)
	: Option (i, n)
	, _clock (X_("timecode-offset"), true, X_(""), true, false, true, false)
	, _get (g)
	, _set (s)
{
	_label = manage (left_aligned_label (n + ":"));
	_label->set_name (X_("OptionsLabel"));
	_clock.ValueChanged.connect (sigc::mem_fun (*this, &ClockOption::save_clock_time));
}

void
ClockOption::set_state_from_config ()
{
	Timecode::Time TC;
	samplepos_t when;
	if (!Timecode::parse_timecode_format(_get(), TC)) {
		_clock.set (timepos_t (0), true);
	}
	TC.rate = _session->samples_per_timecode_frame();
	TC.drop = _session->timecode_drop_frames();
	_session->timecode_to_sample(TC, when, false, false);
	if (TC.negative) { when=-when; }
	_clock.set (timepos_t (when), true);
}

void
ClockOption::save_clock_time ()
{
	Timecode::Time TC;
	_session->sample_to_timecode (_clock.last_when().samples(), TC, false, false);
	_set (Timecode::timecode_format_time(TC));
}

void
ClockOption::add_to_page (OptionEditorPage* p)
{
	add_widgets_to_page (p, _label, &_clock);
}

void
ClockOption::set_session (Session* s)
{
	_session = s;
	if (s) {
		_clock.set_session (s);
	}
}

/*--------------------------*/

WidgetOption::WidgetOption (string const & i, string const & n, Gtk::Widget& w)
	: Option (i, n)
	, _widget (&w)
{
}

void
WidgetOption::add_to_page (OptionEditorPage* p)
{
	add_widget_to_page (p, _widget);
}

/*--------------------------*/

OptionEditorPage::OptionEditorPage ()
	: table (1, 3)
{
	init ();
}

OptionEditorPage::OptionEditorPage (Gtk::Notebook& n, std::string const & t)
	: table (1, 3)
{
	init ();
	box.pack_start (table, false, false);
	box.set_border_width (4);
	n.append_page (box, t);
}

void
OptionEditorPage::init ()
{
	table.set_spacings (4);
	table.set_col_spacing (0, 32);
}

/*--------------------------*/

void
OptionEditorMiniPage::add_to_page (OptionEditorPage* p)
{
	int const n = p->table.property_n_rows();
	int m = n + 1;
	if (!_note.empty ()) {
		++m;
	}
	p->table.resize (m, 3);
	p->table.attach (box, 0, 3, n, n + 1, FILL | EXPAND, SHRINK, 0, 0);
	maybe_add_note (p, n + 1);
}

/*--------------------------*/

/** Construct an OptionEditor.
 *  @param o Configuration to edit.
 *  @param t Title for the dialog.
 */
OptionEditor::OptionEditor (PBD::Configuration* c)
	: _config (c)
	, option_tree (TreeStore::create (option_columns))
	, search_results (0)
	, search_current_highlight (0)
	, search_not_found_count (0)
	, option_treeview (option_tree)
{
	using namespace Notebook_Helpers;

	_notebook.set_show_tabs (false);
	_notebook.set_show_border (true);
	_notebook.set_name ("OptionsNotebook");

	option_treeview.append_column ("", option_columns.name);
	option_treeview.set_enable_search(true);
	option_treeview.set_search_column(0);
	option_treeview.set_name ("OptionsTreeView");
	option_treeview.set_headers_visible (false);

	option_treeview.get_selection()->set_mode (Gtk::SELECTION_SINGLE);
	option_treeview.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &OptionEditor::treeview_row_selected));

	/* Watch out for changes to parameters */
	_config->ParameterChanged.connect (config_connection, invalidator (*this), std::bind (&OptionEditor::parameter_changed, this, _1), gui_context());

	search_entry.show ();
	search_entry.set_text (_("Search here..."));
	search_entry.set_name (X_("ShadedEntry"));
	set_size_request_to_display_given_text (search_entry, X_("a long enough search string"), 2, 2);
	search_packer.pack_start (search_entry, true, true);
	search_packer.show ();

	search_entry.signal_activate().connect (sigc::mem_fun (*this, &OptionEditor::search));
	search_entry.signal_key_press_event().connect (sigc::mem_fun (*this, &OptionEditor::search_key_press), false);
	search_entry.signal_focus_in_event().connect (sigc::mem_fun (*this, &OptionEditor::search_key_focus), false);
	search_entry.signal_focus_out_event().connect (sigc::mem_fun (*this, &OptionEditor::search_key_focus), false);
}

OptionEditor::~OptionEditor ()
{
	for (std::map<std::string, OptionEditorPage*>::iterator i = _pages.begin(); i != _pages.end(); ++i) {
		for (std::list<OptionEditorComponent*>::iterator j = i->second->components.begin(); j != i->second->components.end(); ++j) {
			delete *j;
		}
		delete i->second;
	}
}

bool
OptionEditor::search_key_focus (GdkEventFocus* ev)
{
	if (ev->in) {
		if (search_entry.get_name () == X_("ShadedEntry")) {
			search_entry.set_text ("");
			search_entry.set_name (X_("GtkEntry"));
		}
	} else {
		if (search_entry.get_text().empty() && search_entry.get_name () != X_("ShadedEntry")) {
			search_entry.set_text (_("Search here..."));
			search_entry.set_name (X_("ShadedEntry"));
		}
		if (search_current_highlight) {
			search_current_highlight->end_highlight ();
			search_current_highlight = 0;
		}

	}
	return false;
}

bool
OptionEditor::search_key_press (GdkEventKey* ev)
{
	if (search_entry.get_name () == X_("ShadedEntry")) {
		search_entry.set_text ("");
		search_entry.set_name (X_("GtkEntry"));
	}

	/* any key press should remove the current highlight, since something
	 * is changing.
	 */

	if (search_current_highlight) {
		search_current_highlight->end_highlight ();
		search_current_highlight = 0;
	}

	return false;
}

void
OptionEditor::search ()
{
	string search_for = search_entry.get_text();

	not_found_callback ();

	if (search_for.empty()) {
		return;
	}

	if (!search_results || search_for != last_search_string) {

		boost::char_separator<char> sep (" ");
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		tokenizer t (search_for, sep);

		search_targets.clear ();
		for (tokenizer::iterator ti = t.begin (); ti != t.end (); ++ti) {
			string word = *ti;
			transform (word.begin (), word.end (), word.begin (), ::toupper);
			search_targets.push_back (word);
		}

		/* (re)build search results */

		delete search_results;
		search_results = new std::vector<SearchResult>;

		for (auto p : pages()) {
			for (auto oc : p.second->components) {
				PBD::Configuration::Metadata const * metadata (oc->get_metadata());

				if (!metadata) {
					continue;
				}

				std::vector<SearchResult>::size_type found_cnt = 0;

				for (auto const & s : search_targets) {
					for (auto const & m : *metadata) {
						if (m.find (s) != string::npos) {
							found_cnt++;
							break;
						}
					}
				}

				if (found_cnt == search_targets.size()) {
					search_results->push_back (SearchResult (p.first, *oc));
				}
			}
		}

		if (search_results->empty()) {
			not_found ();
			delete search_results;
			search_results = 0;
			return;
		}

		last_search_string = search_for;
		search_iterator = search_results->begin ();

	} else {

		/* have results and still searching for the same string. End
		 * highlight of previous find (if not at end) and move on to
		 * the next if we can.
		 */

		if (search_current_highlight) {
			search_current_highlight->end_highlight ();
			search_current_highlight = 0;
		}

		if (search_iterator != search_results->end()) {
			++search_iterator;
		}

		if (search_iterator == search_results->end()) {

			search_iterator = search_results->begin();
			search_highlight ((*search_iterator).page_title, (*search_iterator).component);
			search_iterator++;

			return;
		}
	}

	/* Move to next result, and highlight it */

	search_highlight ((*search_iterator).page_title, (*search_iterator).component);
}

void
OptionEditor::not_found ()
{
	search_entry.set_sensitive (false);
	not_found_timeout = Glib::signal_timeout().connect (mem_fun (*this, &OptionEditor::not_found_callback), 250);
	search_not_found_count++;
}

bool
OptionEditor::not_found_callback ()
{
	search_entry.set_sensitive (true);
	search_entry.grab_focus ();
	search_not_found_count = 0;
	return false;
}

void
OptionEditor::search_highlight (std::string const & page_title, OptionEditorComponent& component)
{
	if (current_page() != page_title) {
		set_current_page (page_title);
	}
	search_current_highlight = &component;
	search_current_highlight->highlight ();
}


/** Called when a configuration parameter has been changed.
 *  @param p Parameter name.
 */
void
OptionEditor::parameter_changed (std::string const & p)
{
	ENSURE_GUI_THREAD (*this, &OptionEditor::parameter_changed, p)

	for (std::map<std::string, OptionEditorPage*>::iterator i = _pages.begin(); i != _pages.end(); ++i) {
		for (std::list<OptionEditorComponent*>::iterator j = i->second->components.begin(); j != i->second->components.end(); ++j) {
			(*j)->parameter_changed (p);
		}
	}
}

std::string
OptionEditor::current_page ()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = option_treeview.get_selection();
	TreeModel::const_iterator iter = selection->get_selected();

	if (iter) {
		TreeModel::Row row = *iter;
		return row[option_columns.name];
	}

	return string();
}

void
OptionEditor::treeview_row_selected ()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = option_treeview.get_selection();
	TreeModel::iterator iter = selection->get_selected();

	if (iter) {
		TreeModel::Row row = *iter;
		Gtk::Widget* w = row[option_columns.widget];
		if (w) {
			_notebook.set_current_page (_notebook.page_num (*w));
		}
	}
}

TreeModel::iterator
OptionEditor::find_path_in_treemodel (std::string const & pn, bool create_missing)
{
	/* split page name, which is actually a path, into each component */

	std::vector<std::string> components;
	split (pn, components, '/');

	/* start with top level children */

	TreeModel::Children children = option_tree->children();
	TreeModel::iterator iter;

	/* foreach path component ... */

	for (std::vector<std::string>::const_iterator s = components.begin(); s != components.end(); ++s) {

		for (iter = children.begin(); iter != children.end(); ++iter) {
			TreeModel::Row row = *iter;
			const std::string row_name = row[option_columns.name];
			if (row_name == (*s)) {
				break;
			}
		}

		if (iter == children.end()) {
			/* the current component is missing; bail out or create it */
			if (!create_missing) {
				return option_tree->get_iter(TreeModel::Path(""));
			} else {
				iter = option_tree->append (children);
				TreeModel::Row row = *iter;
				row[option_columns.name] = *s;
				row[option_columns.widget] = 0;
			}
		}

		/* from now on, iter points to a valid row, either the one we found or a new one */
		/* set children to the row's children to continue searching */
		children = (*iter)->children ();

	}

	return iter;
}

void
OptionEditor::add_path_to_treeview (std::string const & pn, Gtk::Widget& widget)
{
	option_treeview.set_model (Glib::RefPtr<TreeStore>());

	TreeModel::iterator row_iter = find_path_in_treemodel(pn, true);

	assert(row_iter);

	TreeModel::Row row = *row_iter;
	row[option_columns.widget] = &widget;

	option_treeview.set_model (option_tree);
	option_treeview.expand_all ();
}

/** Add a component to a given page.
 *  @param page_name Page name (will be created if it doesn't already exist)
 *  @param o Component.
 */
void
OptionEditor::add_option (std::string const & page_name, OptionEditorComponent* o)
{
	if (_pages.find (page_name) == _pages.end()) {
		OptionEditorPage* oep = new OptionEditorPage (_notebook, page_name);
		_pages[page_name] = oep;

		add_path_to_treeview (page_name, oep->box);
	}

	OptionEditorPage* p = _pages[page_name];
	p->components.push_back (o);

	o->add_to_page (p);
	o->set_state_from_config ();
}

/** Add a new page
 *  @param pn Page name (will be created if it doesn't already exist)
 *  @param w widget that fills the page
 */
void
OptionEditor::add_page (std::string const & pn, Gtk::Widget& w)
{
	if (_pages.find (pn) == _pages.end()) {
		OptionEditorPage* oep = new OptionEditorPage (_notebook, pn);
		_pages[pn] = oep;
		add_path_to_treeview (pn, oep->box);
	}

	OptionEditorPage* p = _pages[pn];
	p->box.pack_start (w, true, true);
}

void
OptionEditor::set_current_page (string const & p)
{
	TreeModel::iterator row_iter = find_path_in_treemodel(p);

	if (row_iter) {
		option_treeview.get_selection()->select(row_iter);
	}

}

/*--------------------------*/

DirectoryOption::DirectoryOption (string const & i, string const & n, sigc::slot<string> g, sigc::slot<bool, string> s)
	: Option (i, n)
	, _get (g)
	, _set (s)
{
	Gtkmm2ext::add_volume_shortcuts (_file_chooser);
	_file_chooser.set_action (Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	_changed_connection = _file_chooser.signal_selection_changed().connect (sigc::mem_fun (*this, &DirectoryOption::selection_changed));
}

void
DirectoryOption::set_state_from_config ()
{
	_changed_connection.block ();
	_file_chooser.set_filename (poor_mans_glob(_get ()));
	_changed_connection.unblock ();
}

void
DirectoryOption::add_to_page (OptionEditorPage* p)
{
	Gtk::Label *label = manage (new Label (_name));
	label->set_alignment (0, 0.5);
	label->set_name (X_("OptionsLabel"));
	add_widgets_to_page (p, label, &_file_chooser);
}

void
DirectoryOption::selection_changed ()
{
	_set (poor_mans_glob(_file_chooser.get_filename ()));
}

/*--------------------------*/

OptionEditorContainer::OptionEditorContainer (PBD::Configuration* c)
	: OptionEditor (c)
{
	set_border_width (4);
	Frame* f = manage (new Frame ());
	f->add (treeview());
	f->set_shadow_type (Gtk::SHADOW_OUT);
	f->set_border_width (0);
	treeview_packer.pack_start (*f, true, true);

	hpacker.pack_start (treeview_packer, false, false, 4);
	hpacker.pack_start (notebook(), false, false);
	pack_start (hpacker, true, true);

	show_all ();
}

OptionEditorWindow::OptionEditorWindow (PBD::Configuration* c, string const& str)
	: OptionEditor (c)
	, ArdourWindow (str)
{
	hpacker.set_border_width (4);
	Frame* f = manage (new Frame ());

	f->add (treeview());
	f->set_shadow_type (Gtk::SHADOW_OUT);
	f->set_border_width (0);
	vpacker.pack_start (*f, true, true);

	hpacker.pack_start (vpacker, false, false);
	hpacker.pack_start (notebook(), true, true, 4);

	hpacker.show_all ();
	vpacker.show ();

	add (hpacker);
}
