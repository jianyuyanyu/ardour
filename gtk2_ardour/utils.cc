/*
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2009 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2005-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2006-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2006-2009 Sampo Savolainen <v2@iki.fi>
 * Copyright (C) 2007-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013 John Emmas <john@creativepost.co.uk>
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

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <list>
#include <sys/stat.h>

#include <boost/algorithm/string.hpp>

#include <ytk/gtkpaned.h>
#include <ytkmm/label.h>
#include <ytkmm/paned.h>
#include <ytkmm/rc.h>
#include <ytkmm/stock.h>
#include <ytkmm/window.h>

#include "pbd/basename.h"
#include "pbd/file_utils.h"

#include "ardour/audioengine.h"
#include "ardour/auditioner.h"
#include "ardour/filesystem_paths.h"
#include "ardour/search_paths.h"
#include "ardour/session.h"
#include "ardour/triggerbox.h"

#include "gtkmm2ext/colors.h"
#include "gtkmm2ext/utils.h"

#include "canvas/item.h"

#include "actions.h"
#include "ardour_dialog.h"
#include "ardour_ui.h"
#include "context_menu_helper.h"
#include "debug.h"
#include "gui_thread.h"
#include "keyboard.h"
#include "main_clock.h"
#include "public_editor.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace std;
using namespace Gtk;
using namespace Glib;
using namespace PBD;
using Gtkmm2ext::Keyboard;

namespace ARDOUR_UI_UTILS
{
	sigc::signal<void> DPIReset;
}

#ifdef PLATFORM_WINDOWS
#define random() rand ()
#endif

/** Add an element to a menu, settings its sensitivity.
 * @param m Menu to add to.
 * @param e Element to add.
 * @param s true to make sensitive, false to make insensitive
 */
void
ARDOUR_UI_UTILS::add_item_with_sensitivity (Menu_Helpers::MenuList& m, Menu_Helpers::MenuElem e, bool s)
{
	m.push_back (e);
	if (!s) {
		m.back ().set_sensitive (false);
	}
}

gint
ARDOUR_UI_UTILS::just_hide_it (GdkEventAny* /*ev*/, Gtk::Window* win)
{
	win->hide ();
	return 0;
}

static bool
idle_notify_engine_stopped ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Window", "toggle-audio-midi-setup");

	MessageDialog msg (
	    _("The current operation is not possible because of an error communicating with the audio hardware."),
	    false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);

	msg.add_button (_("Cancel"), Gtk::RESPONSE_CANCEL);

	if (tact && !tact->get_active ()) {
		msg.add_button (_("Configure Hardware"), Gtk::RESPONSE_OK);
	}

	if (msg.run () == Gtk::RESPONSE_OK) {
		tact->set_active ();
	}
	return false; /* do not call again */
}

bool
ARDOUR_UI_UTILS::engine_is_running ()
{
	if (ARDOUR::AudioEngine::instance ()->running ()) {
		return true;
	}
	Glib::signal_idle ().connect (sigc::ptr_fun (&idle_notify_engine_stopped));
	return false;
}

/* xpm2rgb copied from nixieclock, which bore the legend:

    nixieclock - a nixie desktop timepiece
    Copyright (C) 2000 Greg Ercolano, erco@3dsite.com

    and was released under the GPL.
*/

unsigned char*
ARDOUR_UI_UTILS::xpm2rgb (const char** xpm, uint32_t& w, uint32_t& h)
{
	static long    vals[256], val;
	uint32_t       t, x, y, colors, cpp;
	unsigned char  c;
	unsigned char *savergb, *rgb;

	// PARSE HEADER

	if (sscanf (xpm[0], "%u%u%u%u", &w, &h, &colors, &cpp) != 4) {
		error << string_compose (_("bad XPM header %1"), xpm[0])
		      << endmsg;
		return 0;
	}

	savergb = rgb = (unsigned char*)malloc (h * w * 3);

	// LOAD XPM COLORMAP LONG ENOUGH TO DO CONVERSION
	for (t = 0; t < colors; ++t) {
		sscanf (xpm[t + 1], "%c c #%lx", &c, &val);
		vals[c] = val;
	}

	// COLORMAP -> RGB CONVERSION
	//    Get low 3 bytes from vals[]
	//

	const char* p;
	for (y = h - 1; y > 0; --y) {
		for (p = xpm[1 + colors + (h - y - 1)], x = 0; x < w; x++, rgb += 3) {
			val        = vals[(int)*p++];
			*(rgb + 2) = val & 0xff;
			val >>= 8; // 2:B
			*(rgb + 1) = val & 0xff;
			val >>= 8;               // 1:G
			*(rgb + 0) = val & 0xff; // 0:R
		}
	}

	return (savergb);
}

unsigned char*
ARDOUR_UI_UTILS::xpm2rgba (const char** xpm, uint32_t& w, uint32_t& h)
{
	static long    vals[256], val;
	uint32_t       t, x, y, colors, cpp;
	unsigned char  c;
	unsigned char *savergb, *rgb;
	char           transparent;

	// PARSE HEADER

	if (sscanf (xpm[0], "%u%u%u%u", &w, &h, &colors, &cpp) != 4) {
		error << string_compose (_("bad XPM header %1"), xpm[0])
		      << endmsg;
		return 0;
	}

	savergb = rgb = (unsigned char*)malloc (h * w * 4);

	// LOAD XPM COLORMAP LONG ENOUGH TO DO CONVERSION

	if (strstr (xpm[1], "None")) {
		sscanf (xpm[1], "%c", &transparent);
		t = 1;
	} else {
		transparent = 0;
		t           = 0;
	}

	for (; t < colors; ++t) {
		sscanf (xpm[t + 1], "%c c #%lx", &c, &val);
		vals[c] = val;
	}

	// COLORMAP -> RGB CONVERSION
	//    Get low 3 bytes from vals[]
	//

	const char* p;
	for (y = h - 1; y > 0; --y) {
		char alpha;

		for (p = xpm[1 + colors + (h - y - 1)], x = 0; x < w; x++, rgb += 4) {
			if (transparent && (*p++ == transparent)) {
				alpha = 0;
				val   = 0;
			} else {
				alpha = 0xff;
				val   = vals[(int)*p];
			}

			*(rgb + 3) = alpha; // 3: alpha
			*(rgb + 2) = val & 0xff;
			val >>= 8; // 2:B
			*(rgb + 1) = val & 0xff;
			val >>= 8;               // 1:G
			*(rgb + 0) = val & 0xff; // 0:R
		}
	}

	return (savergb);
}

/** Returns a Pango::FontDescription given a string describing the font.
 *
 * If the returned FontDescription does not specify a family, then
 * the family is set to "Sans". This mirrors GTK's behaviour in
 * gtkstyle.c.
 *
 * Some environments will force Pango to specify the family
 * even if it was not specified in the string describing the font.
 * Such environments should be left unaffected by this function,
 * since the font family will be left alone.
 *
 * There may be other similar font specification enforcement
 * that we might add here later.
 */
Pango::FontDescription
ARDOUR_UI_UTILS::sanitized_font (std::string const& name)
{
	Pango::FontDescription fd (name);

	if (fd.get_family ().empty ()) {
		/* default: "Sans" or "ArdourSans" */
		fd.set_family (UIConfiguration::instance ().get_ui_font_family ());
	}

	return fd;
}

Pango::FontDescription
ARDOUR_UI_UTILS::ardour_font (std::string const& name)
{
	Pango::FontDescription fd (name);
	if (!fd.get_family ().empty () && fd.get_family ().find ("Mon") != std::string::npos) {
		/* matches "ArdourMono", "Monaco" */
		fd.set_family ("ArdourMono");
	} else {
		fd.set_family ("ArdourSans");
	}

	return fd;
}

Pango::FontDescription
ARDOUR_UI_UTILS::get_font_for_style (string widgetname)
{
	Gtk::Window              window (WINDOW_TOPLEVEL);
	Gtk::Label               foobar;
	Glib::RefPtr<Gtk::Style> style;

	window.add (foobar);
	foobar.set_name (widgetname);
	foobar.ensure_style ();

	style = foobar.get_style ();

	Glib::RefPtr<const Pango::Layout> layout = foobar.get_layout ();

	PangoFontDescription* pfd = const_cast<PangoFontDescription*> (pango_layout_get_font_description (const_cast<PangoLayout*> (layout->gobj ())));

	if (!pfd) {
		/* layout inherited its font description from a PangoContext */

		PangoContext* ctxt = (PangoContext*)pango_layout_get_context (const_cast<PangoLayout*> (layout->gobj ()));
		pfd                = pango_context_get_font_description (ctxt);
		return Pango::FontDescription (pfd); /* make a copy */
	}

	return Pango::FontDescription (pfd); /* make a copy */
}

bool
ARDOUR_UI_UTILS::relay_key_press (GdkEventKey* ev, Gtk::Window* win)
{
	return ARDOUR_UI::instance ()->key_event_handler (ev, win);
}

bool
ARDOUR_UI_UTILS::emulate_key_event (unsigned int keyval)
{
	GdkDisplay*   display   = gtk_widget_get_display (GTK_WIDGET (ARDOUR_UI::instance ()->main_window ().gobj ()));
	GdkKeymap*    keymap    = gdk_keymap_get_for_display (display);
	GdkKeymapKey* keymapkey = NULL;
	gint          n_keys;

	if (!gdk_keymap_get_entries_for_keyval (keymap, keyval, &keymapkey, &n_keys))
		return false;
	if (n_keys != 1) {
		g_free (keymapkey);
		return false;
	}

	Gtk::Window& main_window (ARDOUR_UI::instance ()->main_window ());

	GdkEventKey ev;
	ev.type             = GDK_KEY_PRESS;
	ev.window           = main_window.get_window ()->gobj ();
	ev.send_event       = FALSE;
	ev.time             = 0;
	ev.state            = 0;
	ev.keyval           = keyval;
	ev.length           = 0;
	ev.string           = const_cast<gchar*> ("");
	ev.hardware_keycode = keymapkey[0].keycode;
	ev.group            = keymapkey[0].group;
	g_free (keymapkey);

	relay_key_press (&ev, &main_window);
	ev.type = GDK_KEY_RELEASE;
	return relay_key_press (&ev, &main_window);
}

Glib::RefPtr<Gdk::Pixbuf>
ARDOUR_UI_UTILS::get_xpm (std::string name)
{
	if (!xpm_map[name]) {
		Searchpath spath (ARDOUR::ardour_data_search_path ());

		spath.add_subdirectory_to_paths ("pixmaps");

		std::string data_file_path;

		if (!find_file (spath, name, data_file_path)) {
			fatal << string_compose (_("cannot find XPM file for %1"), name) << endmsg;
		}

		try {
			xpm_map[name] = Gdk::Pixbuf::create_from_file (data_file_path);
		} catch (const Glib::Error& e) {
			warning << "Caught Glib::Error: " << e.what () << endmsg;
		}
	}

	return xpm_map[name];
}

void
ARDOUR_UI_UTILS::get_color_themes (map<std::string, std::string>& themes)
{
	Searchpath spath (ARDOUR::theme_search_path ());

	for (vector<string>::iterator s = spath.begin (); s != spath.end (); ++s) {
		vector<string> entries;

		find_files_matching_pattern (entries, *s, string ("*") + UIConfiguration::color_file_suffix);

		for (vector<string>::iterator e = entries.begin (); e != entries.end (); ++e) {
			XMLTree tree;

			tree.read ((*e).c_str ());
			XMLNode* root = tree.root ();

			if (!root || root->name () != X_("Ardour")) {
				continue;
			}

			XMLProperty const* prop = root->property (X_("theme-name"));

			if (!prop) {
				continue;
			}

			std::string color_name = basename_nosuffix (*e);
			size_t      sep        = color_name.find_first_of ("-");
			if (sep != string::npos) {
				color_name = color_name.substr (0, sep);
			}
			themes.insert (make_pair (prop->value (), color_name));
		}
	}
}

vector<string>
ARDOUR_UI_UTILS::get_icon_sets ()
{
	Searchpath spath (ARDOUR::ardour_data_search_path ());
	spath.add_subdirectory_to_paths ("icons");
	vector<string> r;

	r.push_back (_("default"));

	for (vector<string>::iterator s = spath.begin (); s != spath.end (); ++s) {
		vector<string> entries;

		get_paths (entries, *s, false, false);

		for (vector<string>::iterator e = entries.begin (); e != entries.end (); ++e) {
			if (Glib::file_test (*e, Glib::FILE_TEST_IS_DIR)) {
				r.push_back (Glib::filename_to_utf8 (Glib::path_get_basename (*e)));
			}
		}
	}

	return r;
}

std::string
ARDOUR_UI_UTILS::get_icon_path (const char* cname, string icon_set, bool is_image)
{
	std::string data_file_path;
	string      name = cname;

	if (is_image) {
		name += X_(".png");
	}

	Searchpath spath (ARDOUR::ardour_data_search_path ());

	if (!icon_set.empty () && icon_set != _("default")) {
		/* add "icons/icon_set" but .. not allowed to add both of these at once */
		spath.add_subdirectory_to_paths ("icons");
		spath.add_subdirectory_to_paths (icon_set);

		find_file (spath, name, data_file_path);
	} else {
		spath.add_subdirectory_to_paths ("icons");
		find_file (spath, name, data_file_path);
	}

	if (data_file_path.empty ()) {
		Searchpath rc (ARDOUR::ardour_data_search_path ());
		rc.add_subdirectory_to_paths ("resources");
		find_file (rc, name, data_file_path);
	}

	if (is_image && data_file_path.empty ()) {
		if (!icon_set.empty () && icon_set != _("default")) {
			warning << string_compose (_("icon \"%1\" not found for icon set \"%2\", fallback to default"), cname, icon_set) << endmsg;
		}

		Searchpath def (ARDOUR::ardour_data_search_path ());
		def.add_subdirectory_to_paths ("icons");

		if (!find_file (def, name, data_file_path)) {
			fatal << string_compose (_("cannot find icon image for %1 using %2"), name, spath.to_string ()) << endmsg;
			abort (); /*NOTREACHED*/
		}
	}

	return data_file_path;
}

Glib::RefPtr<Gdk::Pixbuf>
ARDOUR_UI_UTILS::get_icon (const char* cname, string icon_set)
{
	Glib::RefPtr<Gdk::Pixbuf> img;
	try {
		img = Gdk::Pixbuf::create_from_file (get_icon_path (cname, icon_set));
	} catch (const Gdk::PixbufError& e) {
		cerr << "Caught PixbufError: " << e.what () << endl;
	} catch (...) {
		error << string_compose (_("Caught exception while loading icon named %1"), cname) << endmsg;
	}

	return img;
}

namespace ARDOUR_UI_UTILS
{
Glib::RefPtr<Gdk::Pixbuf>
get_icon (const char* cname)
{
	Glib::RefPtr<Gdk::Pixbuf> img;
	try {
		img = Gdk::Pixbuf::create_from_file (get_icon_path (cname));
	} catch (const Gdk::PixbufError& e) {
		cerr << "Caught PixbufError: " << e.what () << endl;
	} catch (...) {
		error << string_compose (_("Caught exception while loading icon named %1"), cname) << endmsg;
	}

	return img;
}
} // namespace ARDOUR_UI_UTILS

string
ARDOUR_UI_UTILS::longest (vector<string>& strings)
{
	if (strings.empty ()) {
		return string ("");
	}

	vector<string>::iterator longest        = strings.begin ();
	string::size_type        longest_length = (*longest).length ();

	vector<string>::iterator i = longest;
	++i;

	while (i != strings.end ()) {
		string::size_type len = (*i).length ();

		if (len > longest_length) {
			longest        = i;
			longest_length = len;
		}

		++i;
	}

	return *longest;
}

bool
ARDOUR_UI_UTILS::key_is_legal_for_numeric_entry (guint keyval)
{
	/* we assume that this does not change over the life of the process */
	static int comma_decimal = -1;

	switch (keyval) {
		case GDK_period:
		case GDK_comma:
			if (comma_decimal < 0) {
				std::lconv* lc = std::localeconv ();
				if (strchr (lc->decimal_point, ',') != 0) {
					comma_decimal = 1;
				} else {
					comma_decimal = 0;
				}
			}
			break;
		default:
			break;
	}

	switch (keyval) {
		case GDK_decimalpoint:
		case GDK_KP_Separator:
			return true;

		case GDK_period:
			if (comma_decimal) {
				return false;
			} else {
				return true;
			}
			break;
		case GDK_comma:
			if (comma_decimal) {
				return true;
			} else {
				return false;
			}
			break;
		case GDK_minus:
		case GDK_plus:
		case GDK_0:
		case GDK_1:
		case GDK_2:
		case GDK_3:
		case GDK_4:
		case GDK_5:
		case GDK_6:
		case GDK_7:
		case GDK_8:
		case GDK_9:
		case GDK_KP_Add:
		case GDK_KP_Subtract:
		case GDK_KP_Decimal:
		case GDK_KP_0:
		case GDK_KP_1:
		case GDK_KP_2:
		case GDK_KP_3:
		case GDK_KP_4:
		case GDK_KP_5:
		case GDK_KP_6:
		case GDK_KP_7:
		case GDK_KP_8:
		case GDK_KP_9:
		case GDK_Return:
		case GDK_BackSpace:
		case GDK_Delete:
		case GDK_KP_Enter:
		case GDK_Home:
		case GDK_End:
		case GDK_Left:
		case GDK_Right:
			return true;

		default:
			break;
	}

	return false;
}

int
ARDOUR_UI_UTILS::guess_default_ui_scale ()
{
#ifdef __APPLE__
	return 100;
#else
	gint width = 0;
	gint height = 0;
	GdkScreen* screen = gdk_display_get_screen (gdk_display_get_default (), 0);
	gint n_monitors = gdk_screen_get_n_monitors (screen);

	if (!screen) {
		return 100;
	}

	for (gint i = 0; i < n_monitors; ++i) {
		GdkRectangle rect;
		gdk_screen_get_monitor_geometry (screen, i, &rect);
		width = std::max (width, rect.width);
		height = std::max (height, rect.height);
	}

	float wx = width  / 1920.f;
	float hx = height / 1080.f;
	float sx = std::min (wx, hx);

	if (sx < 1.25) {
		return 100;
	} else if (sx < 1.6) {
		return 150;
	} else if (sx < 2.1) {
		return 200;
	} else {
		return 250;
	}
#endif
}

void
ARDOUR_UI_UTILS::resize_window_to_proportion_of_monitor (Gtk::Window* window, int max_width, int max_height)
{
	Glib::RefPtr<Gdk::Screen> screen = window->get_screen ();
	Gdk::Rectangle            monitor_rect;
	screen->get_monitor_geometry (0, monitor_rect);

	int const w = std::min (int (monitor_rect.get_width () * 0.8), max_width);
	int const h = std::min (int (monitor_rect.get_height () * 0.8), max_height);

	window->resize (w, h);
}

/** Replace _ with __ in a string; for use with menu item text to make underscores displayed correctly */
string
ARDOUR_UI_UTILS::escape_underscores (string const& s)
{
	string                  o;
	string::size_type const N = s.length ();

	for (string::size_type i = 0; i < N; ++i) {
		if (s[i] == '_') {
			o += "__";
		} else {
			o += s[i];
		}
	}

	return o;
}

Gdk::Color
ARDOUR_UI_UTILS::round_robin_palette_color (bool just_peek)
{
	Gdk::Color                                newcolor;
	string                                    cp = UIConfiguration::instance ().get_stripable_color_palette ();
	Gdk::ArrayHandle_Color                    gc = ColorSelection::palette_from_string (cp);
	std::vector<Gdk::Color>                   c (gc);
	static std::vector<Gdk::Color>::size_type index = 0;

	assert (c.size() > 0);

	if (index >= c.size ()) {
		index = 0;
	}

	if(just_peek) {
		return c[index];
	}

	return c[index++];
}

Gdk::Color
ARDOUR_UI_UTILS::unique_random_color (list<Gdk::Color>& used_colors)
{
	Gdk::Color newcolor;

	while (1) {
		double h, s, v;

		h = fmod (random (), 360.0);
		s = (random () % 65535) / 65535.0;
		v = (random () % 65535) / 65535.0;

		s = min (0.5, s); /* not too saturated */
		v = max (0.9, v); /* not too bright */
		newcolor.set_hsv (h, s, v);

		if (used_colors.size () == 0) {
			used_colors.push_back (newcolor);
			return newcolor;
		}

		for (list<Gdk::Color>::iterator i = used_colors.begin (); i != used_colors.end (); ++i) {
			Gdk::Color c = *i;
			float      rdelta, bdelta, gdelta;

			rdelta = newcolor.get_red () - c.get_red ();
			bdelta = newcolor.get_blue () - c.get_blue ();
			gdelta = newcolor.get_green () - c.get_green ();

			if (sqrt (rdelta * rdelta + bdelta * bdelta + gdelta * gdelta) > 25.0) {
				/* different enough */
				used_colors.push_back (newcolor);
				return newcolor;
			}
		}

		/* XXX need throttle here to make sure we don't spin for ever */
	}
}

string
ARDOUR_UI_UTILS::rate_as_string (float r)
{
	char buf[32];
	if (fmod (r, 100.0f)) {
		snprintf (buf, sizeof (buf), "%.2f kHz", r / 1000.0);
	} else if (fmod (r, 1000.0f)) {
		snprintf (buf, sizeof (buf), "%.1f kHz", r / 1000.0);
	} else {
		snprintf (buf, sizeof (buf), "%.0f kHz", r / 1000.0);
	}
	return buf;
}

string
ARDOUR_UI_UTILS::samples_as_time_string (samplecnt_t s, float rate, bool show_samples)
{
	char buf[32];
	if (rate <= 0) {
		snprintf (buf, sizeof (buf), "--");
	} else if (s == 0) {
		snprintf (buf, sizeof (buf), "0");
	} else if (s < 1000 && show_samples) {
		/* 0 .. 999 spl */
		snprintf (buf, sizeof (buf), "%" PRId64 " spl", s);
	} else if (s < (rate / 1000.f)) {
		/* 0 .. 999 usec */
		snprintf (buf, sizeof (buf), u8"%.0f \u00B5s", s * 1e+6f / rate);
	} else if (s < (rate / 100.f)) {
		/* 1.000 .. 9.999 ms */
		snprintf (buf, sizeof (buf), "%.3f ms", s * 1e+3f / rate);
	} else if (s < (rate / 10.f)) {
		/* 1.00 .. 99.99 ms */
		snprintf (buf, sizeof (buf), "%.2f ms", s * 1e+3f / rate);
	} else if (s < rate) {
		/* 100.0 .. 999.9 ms */
		snprintf (buf, sizeof (buf), "%.1f ms", s * 1e+3f / rate);
	} else if (s < rate * 10.f) {
		/* 1.000 s .. 9.999 s */
		snprintf (buf, sizeof (buf), "%.3f s", s / rate);
	} else if (s < rate * 90.f) {
		/* 10.00 s .. 89.99 s */
		snprintf (buf, sizeof (buf), "%.2f s", s / rate);
	} else {
		/* 1m30.0 ...  */
		snprintf (buf, sizeof (buf), "'%.0fm%.1f", s / (60.f * rate), fmodf (s / rate, 60));
	}
	buf[31] = '\0';
	return buf;
}

string
ARDOUR_UI_UTILS::midi_channels_as_string (std::bitset<16> channels)
{
	if (channels.none ()) {
		return _("none");
	}

	string rv;

	for (int i = 0; i < 16; i++) {
		bool prior    = i < 1 ? false : channels.test (i - 1);
		bool current  = channels.test (i);
		bool next     = i > 14 ? false : channels.test (i + 1);
		bool nextnext = i > 13 ? false : channels.test (i + 2);
		bool future   = false;
		for (int f = i + 1; f < 16; f++) {
			if (channels.test (f)) {
				future = true;
			}
		}

		if (prior && current && next) {
			/* I'm in the middle of a consecutive chain, maybe just add a dash */
			if (!rv.empty () && (rv.rfind ("-") != rv.length () - 1)) {
				rv += "-";
			}
			continue;
		}

		if (current) {
			/* here I am! */
			rv += to_string<int> (i + 1);
		}

		if (current && future && !(next && nextnext)) {
			/* there are channels after me but they are not 3 consecutive; add a comma */
			rv += ",";
		}
	}

	return rv;
}

bool
ARDOUR_UI_UTILS::windows_overlap (Gtk::Window* a, Gtk::Window* b)
{
	if (!a || !b) {
		return false;
	}
	if (a->get_screen () == b->get_screen ()) {
		gint ex, ey, ew, eh;
		gint mx, my, mw, mh;

		a->get_position (ex, ey);
		a->get_size (ew, eh);
		b->get_position (mx, my);
		b->get_size (mw, mh);

		GdkRectangle e;
		GdkRectangle m;
		GdkRectangle r;

		e.x      = ex;
		e.y      = ey;
		e.width  = ew;
		e.height = eh;

		m.x      = mx;
		m.y      = my;
		m.width  = mw;
		m.height = mh;

		if (gdk_rectangle_intersect (&e, &m, &r)) {
			return true;
		}
	}
	return false;
}

bool
ARDOUR_UI_UTILS::overwrite_file_dialog (Gtk::Window& parent, string title, string text)
{
	ArdourDialog dialog (parent, title, true);
	Label        label (text);

	dialog.get_vbox ()->pack_start (label, true, true);
	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (_("Overwrite"), Gtk::RESPONSE_ACCEPT);
	dialog.show_all ();

	switch (dialog.run ()) {
		case RESPONSE_ACCEPT:
			return true;
		default:
			return false;
	}
}

bool
ARDOUR_UI_UTILS::running_from_source_tree ()
{
	gchar const* x = g_getenv ("ARDOUR_THEMES_PATH");
	return x && (string (x).find ("gtk2_ardour") != string::npos);
}

Gtk::Menu*
ARDOUR_UI_UTILS::shared_popup_menu ()
{
	return ARDOUR_UI::instance ()->shared_popup_menu ();
}

bool
ARDOUR_UI_UTILS::convert_drop_to_paths (vector<string>& paths, const SelectionData& data)
{
	vector<string> uris = data.get_uris ();

	if (uris.empty ()) {
		/* This is seriously fucked up. Nautilus doesn't say that its URI lists
		 * are actually URI lists. So do it by hand.
		 */
		if (data.get_target () != "text/plain") {
			return false;
		}

		/* Parse the "uri-list" format that Nautilus provides,
		 * where each pathname is delimited by \r\n.
		 *
		 * THERE MAY BE NO NULL TERMINATING CHAR!!!
		 */
		string txt = data.get_text ();

		/* copy to char* for easy char-wise checks and modification */
		char* tmp = (char*)malloc (txt.length () + 1);
		char* p   = tmp;

		txt.copy (p, txt.length (), 0);
		p[txt.length ()] = '\0';

		while (p) {
			if (*p != '#') {
				while (g_ascii_isspace (*p)) {
					p++;
				}

				const char* q = p;
				while (*q && (*q != '\n') && (*q != '\r')) {
					q++;
				}

				if (q > p) {
					--q;
					while (q > p && g_ascii_isspace (*q)) {
						--q;
					}

					if (q > p) {
						uris.push_back (string (p, q - p + 1));
					}
				}
			}
			p = strchr (p, '\n');
			if (p) {
				++p;
			}
		}

		free ((void*)tmp);

		if (uris.empty ()) {
			return false;
		}
	}

	for (vector<string>::iterator i = uris.begin (); i != uris.end (); ++i) {
		if ((*i).substr (0, 7) == "file://") {
			paths.push_back (Glib::filename_from_uri (*i));
		}
	}

	return !paths.empty ();
}

void
ARDOUR_UI_UTILS::copy_patch_changes (std::shared_ptr<ARDOUR::Auditioner> a, std::shared_ptr<ARDOUR::Trigger> t)
{
	std::shared_ptr<ARDOUR::MIDITrigger> mt = std::dynamic_pointer_cast<ARDOUR::MIDITrigger> (t);

	if (!mt || !a) {
		return;
	}
	for (uint8_t c = 0; c < 16; ++c) {
		if (a->patch_change (c).is_set ()) {
			mt->set_patch_change (a->patch_change (c));
		} else {
			mt->unset_patch_change (c);
		}
	}
}

void
ARDOUR_UI_UTILS::format_position (ARDOUR::Session* s, Temporal::timepos_t const& p, char* buf, size_t bufsize, bool onoff)
{
	Temporal::BBT_Time bbt;
	Timecode::Time     timecode;
	samplepos_t        pos (p.samples ());

	if (pos < 0) {
		error << string_compose (_("format_position: negative timecode position: %1"), pos) << endmsg;
		snprintf (buf, bufsize, "invalid");
		return;
	}

	switch (ARDOUR_UI::instance ()->primary_clock->mode ()) {
		case AudioClock::BBT:
			bbt = Temporal::TempoMap::use ()->bbt_at (p);
			if (onoff) {
				snprintf (buf, bufsize, "%03d|%02d|%04d", bbt.bars, bbt.beats, bbt.ticks);
			} else {
				snprintf (buf, bufsize, "(%03d|%02d|%04d)", bbt.bars, bbt.beats, bbt.ticks);
			}
			break;

		case AudioClock::MinSec:
			samplepos_t left;
			int         hrs;
			int         mins;
			float       secs;

			left = pos;
			hrs  = (int)floor (left / (s->sample_rate () * 60.0f * 60.0f));
			left -= (samplecnt_t)floor (hrs * s->sample_rate () * 60.0f * 60.0f);
			mins = (int)floor (left / (s->sample_rate () * 60.0f));
			left -= (samplecnt_t)floor (mins * s->sample_rate () * 60.0f);
			secs = left / (float)s->sample_rate ();
			if (onoff) {
				snprintf (buf, bufsize, "%02d:%02d:%06.3f", hrs, mins, secs);
			} else {
				snprintf (buf, bufsize, "(%02d:%02d:%06.3f)", hrs, mins, secs);
			}
			break;

		case AudioClock::Seconds:
			if (onoff) {
				snprintf (buf, bufsize, "%.1f", pos / (float)s->sample_rate ());
			} else {
				snprintf (buf, bufsize, "(%.1f)", pos / (float)s->sample_rate ());
			}
			break;

		case AudioClock::Samples:
			if (onoff) {
				snprintf (buf, bufsize, "%" PRId64, pos);
			} else {
				snprintf (buf, bufsize, "(%" PRId64 ")", pos);
			}
			break;

		case AudioClock::Timecode:
		default:
			s->timecode_time (pos, timecode);
			if (onoff) {
				snprintf (buf, bufsize, "%02d:%02d:%02d:%02d", timecode.hours, timecode.minutes, timecode.seconds, timecode.frames);
			} else {
				snprintf (buf, bufsize, "(%02d:%02d:%02d:%02d)", timecode.hours, timecode.minutes, timecode.seconds, timecode.frames);
			}
			break;
	}
}
