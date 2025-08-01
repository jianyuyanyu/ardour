/*
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2007-2011 David Robillard <d@drobilla.net>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013-2018 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2016-2018 Ben Loftis <ben@harrisonconsoles.com>
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

/* this file exists solely to break compilation dependencies that
   would connect changes to the mixer or editor objects.
*/

#include <cstdio>

#include "pbd/error.h"

#include "ardour/session.h"
#include "ardour/lv2_plugin.h"

#include "gtkmm2ext/bindings.h"

#include "actions.h"
#include "ardour_message.h"
#include "ardour_ui.h"
#include "audio_clip_editor.h"
#include "public_editor.h"
#include "meterbridge.h"
#include "luainstance.h"
#include "luawindow.h"
#include "mixer_ui.h"
#include "recorder_ui.h"
#include "trigger_page.h"
#include "keyboard.h"
#include "keyeditor.h"
#include "rc_option_editor.h"
#include "region_editor.h"
#include "rta_manager.h"
#include "route_params_ui.h"
#include "trigger_ui.h"
#include "step_entry.h"
#include "opts.h"

#ifdef GDK_WINDOWING_X11
#include <ydk/gdkx.h>
#endif

#include "pbd/i18n.h"

using namespace Gtk;
using namespace PBD;

namespace ARDOUR {
	class Session;
	class Route;
}

using namespace ARDOUR;
using namespace Gtkmm2ext;

void
ARDOUR_UI::we_have_dependents ()
{
	install_dependent_actions ();

	/* The monitor section relies on at least 1 action defined by us. Since that
	 * action now exists, give it a chance to use it.
	 */
	mixer->monitor_section().use_others_actions ();

	StepEntry::setup_actions_and_bindings ();
	RegionEditor::setup_actions_and_bindings ();

	setup_action_tooltips ();

	/* Global, editor, mixer, processor box actions are defined now. Link
	   them with any bindings, so that GTK does not get a chance to define
	   the GTK accel map entries first when we ask the GtkUIManager to
	   create menus/widgets.

	   If GTK adds the actions to its accel map before we do, we lose our
	   freedom to use any keys. More precisely, we can use any keys, but
	   ones that GTK considers illegal as accelerators will not show up in
	   menus.

	   There are other dynamic actions that can be created by a monitor
	   section, by step entry dialogs. These need to be handled
	   separately. They don't tend to use GTK-illegal bindings and more
	   importantly they don't have menus showing the bindings, so it is
	   less of an issue.
	*/

	Gtkmm2ext::Bindings::associate_all ();

	editor->UpdateAllTransportClocks.connect (sigc::mem_fun (*this, &ARDOUR_UI::update_transport_clocks));

	/* all actions are defined */

	ActionManager::load_menus (ARDOUR_COMMAND_LINE::menus_file);

	/* catch up on parameters */

	std::function<void (std::string)> pc (std::bind (&ARDOUR_UI::parameter_changed, this, _1));
	Config->map_parameters (pc);
}

void
ARDOUR_UI::connect_dependents_to_session (ARDOUR::Session *s)
{
	DisplaySuspender ds;
	BootMessage (_("Setup Editor"));
	editor->set_session (s);
	BootMessage (_("Setup Mixer"));
	mixer->set_session (s);
	recorder->set_session (s);
	trigger_page->set_session (s);
	meterbridge->set_session (s);

	RTAManager::instance ()->set_session (s);

	/* its safe to do this now */

	BootMessage (_("Reload Session History"));
	s->restore_history ("");
}

/** The main editor window has been closed */
gint
ARDOUR_UI::exit_on_main_window_close (GdkEventAny * /*ev*/)
{
#ifdef __APPLE__
	/* just hide the window, and return - the top menu stays up */
	editor->hide ();
	return TRUE;
#else
	/* time to get out of here */
	finish();
	return TRUE;
#endif
}

GtkNotebook*
ARDOUR_UI::tab_window_root_drop (GtkNotebook* src,
				 GtkWidget* w,
				 gint x,
				 gint y,
				 gpointer)
{
	using namespace std;
	Gtk::Notebook* nb = 0;
	Gtk::Window* win = 0;
	ArdourWidgets::Tabbable* tabbable = 0;


	if (w == GTK_WIDGET(editor->contents().gobj())) {
		tabbable = editor;
	} else if (w == GTK_WIDGET(mixer->contents().gobj())) {
		tabbable = mixer;
	} else if (w == GTK_WIDGET(rc_option_editor->contents().gobj())) {
		tabbable = rc_option_editor;
	} else if (w == GTK_WIDGET(recorder->contents().gobj())) {
		tabbable = recorder;
	} else if (w == GTK_WIDGET(trigger_page->contents().gobj())) {
		tabbable = trigger_page;
	} else {
		return 0;
	}

	nb = tabbable->tab_root_drop ();
	win = tabbable->own_window ();

	if (nb) {
		win->move (x, y);
		win->show_all ();
		win->present ();
		return nb->gobj();
	}

	return 0; /* what was that? */
}

bool
ARDOUR_UI::idle_ask_about_quit ()
{
	const auto ask_before_closing = UIConfiguration::instance ().get_ask_before_closing_last_window ();

	if ((_session && _session->dirty ()) || !ask_before_closing) {
		finish ();
	} else {
		/* no session or session not dirty, but still ask anyway */

		ArdourMessageDialog msg (string_compose (_("Quit %1?"), PROGRAM_NAME),
		                         false, /* no markup */
		                         Gtk::MESSAGE_INFO,
		                         Gtk::BUTTONS_YES_NO,
		                         true); /* modal */
		msg.set_default_response (Gtk::RESPONSE_YES);
		msg.set_position (WIN_POS_MOUSE);

		if (msg.run () == Gtk::RESPONSE_YES) {
			finish ();
		}
	}

	/* not reached but keep the compiler happy */

	return false;
}

bool
ARDOUR_UI::main_window_delete_event (GdkEventAny* ev)
{
	/* quit the application as soon as we go idle. If we call this here,
	 * the window manager/desktop can think we're taking too longer to
	 * handle the "delete" event
	 */

	Glib::signal_idle().connect (sigc::mem_fun (*this, &ARDOUR_UI::idle_ask_about_quit));

	return true;
}

static GtkNotebook*
tab_window_root_drop (GtkNotebook* src,
                      GtkWidget* w,
                      gint x,
                      gint y,
                      gpointer user_data)
{
	return ARDOUR_UI::instance()->tab_window_root_drop (src, w, x, y, user_data);
}

int
ARDOUR_UI::setup_windows ()
{
	_tabs.set_show_border(false);
	_tabs.signal_switch_page().connect (sigc::mem_fun (*this, &ARDOUR_UI::tabs_switch));
	_tabs.signal_page_added().connect (sigc::mem_fun (*this, &ARDOUR_UI::tabs_page_added));
	_tabs.signal_page_removed().connect (sigc::mem_fun (*this, &ARDOUR_UI::tabs_page_removed));

	rc_option_editor = new RCOptionEditor;
	rc_option_editor->StateChange.connect (sigc::mem_fun (*this, &ARDOUR_UI::tabbable_state_change));

	if (create_editor ()) {
		error << _("UI: cannot setup editor") << endmsg;
		return -1;
	}

	if (create_mixer ()) {
		error << _("UI: cannot setup mixer") << endmsg;
		return -1;
	}

	if (create_recorder ()) {
		error << _("UI: cannot setup recorder") << endmsg;
		return -1;
	}

	if (create_trigger_page ()) {
		error << _("UI: cannot setup trigger") << endmsg;
		return -1;
	}

	if (create_meterbridge ()) {
		error << _("UI: cannot setup meterbridge") << endmsg;
		return -1;
	}

	/* all other dialogs are created conditionally */

	we_have_dependents ();

	/* order of addition affects order seen in initial window display */

	rc_option_editor->add_to_notebook (_tabs);
	mixer->add_to_notebook (_tabs);
	editor->add_to_notebook (_tabs);
	recorder->add_to_notebook (_tabs);
	trigger_page->add_to_notebook (_tabs);

	top_packer.pack_start (menu_bar_base, false, false);

	main_vpacker.pack_start (top_packer, false, false);

	ArdourWidgets::ArdourDropShadow *spacer = manage (new (ArdourWidgets::ArdourDropShadow));
	spacer->set_size_request( -1, 4 );
	spacer->show();

	/* now add the transport sample to the top of main window */

	main_vpacker.pack_start ( *spacer, false, false);
	main_vpacker.pack_start (_tabs, true, true);

	setup_transport();
	build_menu_bar ();
	setup_tooltips ();

	/* set DPI before realizing widgets */
	UIConfiguration::instance().reset_dpi ();

	ActionsReady (); // EMIT SIGNAL

	_main_window.signal_delete_event().connect (sigc::mem_fun (*this, &ARDOUR_UI::main_window_delete_event));

	/* pack the main vpacker into the main window and show everything
	 */

	_main_window.add (main_vpacker);

	apply_window_settings (true);

	setup_toplevel_window (_main_window, "", this);
	_main_window.show_all ();

	_tabs.set_show_tabs (false);

	/* It would be nice if Gtkmm had wrapped this rather than just
	 * deprecating the old set_window_creation_hook() method, but oh well...
	 */
	g_signal_connect (_tabs.gobj(), "create-window", (GCallback) ::tab_window_root_drop, this);

#ifdef GDK_WINDOWING_X11
	/* allow externalUIs to be transient, on top of the main window */
	LV2Plugin::set_main_window_id (GDK_DRAWABLE_XID(_main_window.get_window()->gobj()));
#endif

	return 0;
}

void
ARDOUR_UI::apply_window_settings (bool with_size)
{
	const XMLNode* mnode = main_window_settings ();

	if (!mnode) {
		return;
	}

	XMLProperty const* prop;

	if (with_size) {
		gint x = -1;
		gint y = -1;
		gint w = -1;
		gint h = -1;

		if ((prop = mnode->property (X_("x"))) != 0) {
			x = atoi (prop->value());
		}

		if ((prop = mnode->property (X_("y"))) != 0) {
			y = atoi (prop->value());
		}

		if ((prop = mnode->property (X_("w"))) != 0) {
			w = atoi (prop->value());
		}

		if ((prop = mnode->property (X_("h"))) != 0) {
			h = atoi (prop->value());
		}

		if (x >= 0 && y >= 0 && w >= 0 && h >= 0) {
			_main_window.set_position (Gtk::WIN_POS_NONE);
		}

		if (x >= 0 && y >= 0) {
			_main_window.move (x, y);
		}

		if (w > 0 && h > 0) {
			_main_window.set_default_size (w, h);
		}
	}

	std::string current_tab;

	if ((prop = mnode->property (X_("current-tab"))) != 0) {
		current_tab = prop->value();
	} else {
		current_tab = "editor";
	}

	if (mixer && current_tab == "mixer") {
		_tabs.set_current_page (_tabs.page_num (mixer->contents()));
	} else if (rc_option_editor && current_tab == "preferences") {
		_tabs.set_current_page (_tabs.page_num (rc_option_editor->contents()));
	} else if (recorder && current_tab == "recorder") {
		_tabs.set_current_page (_tabs.page_num (recorder->contents()));
	} else if (trigger_page && current_tab == "trigger") {
		_tabs.set_current_page (_tabs.page_num (trigger_page->contents()));
	} else if (editor) {
		_tabs.set_current_page (_tabs.page_num (editor->contents()));
	}
	return;
}
