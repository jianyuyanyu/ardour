/*
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2005 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2006-2011 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2008 Hans Baier <hansfbaier@googlemail.com>
 * Copyright (C) 2012-2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2013-2015 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2013-2016 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2017 Johannes Mueller <github@johannes-mueller.org>
 * Copyright (C) 2018 Len Ovens <len@ovenwerks.net>
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

#include <time.h>

/* need _BSD_SOURCE to get timersub macros */

#ifdef _BSD_SOURCE
#include <sys/time.h>
#else
#define _BSD_SOURCE
#include <sys/time.h>
#undef _BSD_SOURCE
#endif

#include <list>
#include <cmath>

#include "pbd/xml++.h"
#include <ytkmm/box.h>
#include <ytkmm/frame.h>
#include <ytkmm/label.h>
#include <ytkmm/table.h>
#include <ytkmm/fixed.h>
#include <ytkmm/drawingarea.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/menu.h>
#include <ytkmm/menuitem.h>
#include <ytkmm/messagedialog.h>
#include <ytkmm/notebook.h>
#include <ytkmm/button.h>
#include <ytkmm/togglebutton.h>
#include <ytkmm/sizegroup.h>
#include <ytkmm/treeview.h>
#include <ytkmm/menubar.h>
#include <ytkmm/textbuffer.h>
#include <ytkmm/adjustment.h>

#include "gtkmm2ext/gtk_ui.h"
#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/visibility_tracker.h"

#include "ardour/ardour.h"
#include "ardour/types.h"
#include "ardour/utils.h"
#include "ardour/plugin.h"
#include "ardour/session_handle.h"
#include "ardour/system_exec.h"

#include "video_timeline.h"

#include "widgets/ardour_button.h"
#include "widgets/ardour_dropdown.h"
#include "widgets/ardour_spacer.h"

#include "add_route_dialog.h"
#include "ardour_dialog.h"
#include "ardour_window.h"
#include "editing.h"
#include "enums.h"
#include "region_ui_settings.h"
#include "shuttle_control.h"
#include "startup_fsm.h"
#include "transport_control.h"
#include "transport_control_ui.h"
#include "visibility_group.h"
#include "window_manager.h"

#ifdef COMPILER_MSVC
#include "about.h"
#include "add_video_dialog.h"
#include "big_clock_window.h"
#include "big_transport_window.h"
#include "bundle_manager.h"
#include "dsp_stats_window.h"
#include "engine_dialog.h"
#include "export_video_dialog.h"
#include "global_port_matrix.h"
#include "idleometer.h"
#include "io_plugin_window.h"
#include "keyeditor.h"
#include "location_ui.h"
#include "lua_script_manager.h"
#include "luawindow.h"
#include "plugin_dspload_window.h"
#include "plugin_manager_ui.h"
#include "rc_option_editor.h"
#include "route_dialogs.h"
#include "route_params_ui.h"
#include "rta_window.h"
#include "session_option_editor.h"
#include "speaker_dialog.h"
#include "transport_masters_dialog.h"
#include "virtual_keyboard_window.h"
#include "library_download_dialog.h"
#else
class About;
class AddRouteDialog;
class AddVideoDialog;
class BigClockWindow;
class BigTransportWindow;
class BundleManager;
class EngineControl;
class ExportVideoDialog;
class KeyEditor;
class LocationUIWindow;
class LuaScriptManager;
class LuaWindow;
class RCOptionEditor;
class RouteParams_UI;
class SessionOptionEditor;
class SpeakerDialog;
class GlobalPortMatrixWindow;
class IdleOMeter;
class IOPluginWindow;
class PluginDSPLoadWindow;
class PluginManagerUI;
class RTAWindow;
class DspStatisticsWindow;
class TransportMastersWindow;
class VirtualKeyboardWindow;
class LibraryDownloadDialog;
#endif

class VideoTimeLine;
class ArdourKeyboard;
class AudioClock;
class ConnectionEditor;
class DuplicateRouteDialog;
class MainClock;
class Mixer_UI;
class PublicEditor;
class RecorderUI;
class TriggerPage;
class SaveAsDialog;
class SaveTemplateDialog;
class SessionDialog;
class SessionOptionEditorWindow;
class Splash;
class TimeInfoBox;
class ApplicationBar;
class Meterbridge;
class LuaWindow;
class MidiTracer;
class PianorollWindow;
class NSM_Client;
class LevelMeterHBox;
class GUIObjectState;
class BasicUI;

namespace ARDOUR {
	class ControlProtocolInfo;
	class IO;
	class Port;
	class Route;
	class RouteGroup;
	class Location;
	class ProcessThread;
}

namespace Gtk {
	class ProgressBar;
}

namespace ArdourWidgets {
	class Prompter;
	class Tabbable;
}

#define MAX_LUA_ACTION_SCRIPTS 32

class ARDOUR_UI : public Gtkmm2ext::UI, public ARDOUR::SessionHandlePtr, public TransportControlProvider
{
public:
	ARDOUR_UI (int *argcp, char **argvp[], const char* localedir);
	~ARDOUR_UI();

	bool run_startup (bool should_be_new, std::string load_template);

	void hide_splash ();

	void launch_chat ();
	void launch_tutorial ();
	void launch_reference ();
	void launch_tracker ();
	void launch_subscribe ();
	void launch_website ();
	void launch_website_dev ();
	void launch_forums ();
	void launch_howto_report ();
	void show_about ();
	void hide_about ();

	void load_from_application_api (const std::string& path);
	void finish();

	int copy_demo_sessions ();
	int load_session (const std::string& path, const std::string& snapshot, std::string mix_template = std::string());
	bool session_load_in_progress;
	int build_session (std::string const& path, std::string const& snapshot, std::string const& session_template, ARDOUR::BusProfile const&, bool from_startup_fsm, bool unnamed, Temporal::TimeDomain domain);
	int build_session_stage_two (std::string const& path, std::string const& snapshot, std::string const& session_template, ARDOUR::BusProfile const&, bool unnamed, Temporal::TimeDomain domain, ARDOUR::samplecnt_t samplerate = 0);

	bool session_is_new() const { return _session_is_new; }

	ARDOUR::Session* the_session() { return _session; }

	bool get_smart_mode () const;

	RCOptionEditor* get_rc_option_editor() { return rc_option_editor; }
	void show_tabbable (ArdourWidgets::Tabbable*);
	bool tabbable_visibility_button_press (GdkEventButton* ev, std::string const& tabbable_name);

	enum ArdourLogLevel {
		LogLevelNone = 0,
		LogLevelInfo,
		LogLevelWarning,
		LogLevelError
	};

	ArdourLogLevel log_not_acknowledged () const { return _log_not_acknowledged; }
	void set_log_not_acknowledged (const ArdourLogLevel lvl) { _log_not_acknowledged =lvl; }

	void start_session_load (bool create_new);
	void session_dialog_response_handler (int response, SessionDialog* session_dialog);
	void build_session_from_dialog (SessionDialog&, std::string const& session_name, std::string const& session_path, std::string const& session_template, Temporal::TimeDomain domain);
	bool ask_about_loading_existing_session (const std::string& session_path);
	int load_session_from_startup_fsm ();

	/// @return true if session was successfully unloaded.
	int unload_session (bool hide_stuff = false, bool force_unload = false);
	void close_session();

	int  save_state_canfail (std::string state_name = "", bool switch_to_it = false);
	void save_state (const std::string & state_name = "", bool switch_to_it = false);

	int new_session_from_aaf (std::string const&, std::string const&, std::string&, std::string&);

	static ARDOUR_UI *instance () { return theArdourUI; }

	/* signal emitted when all windows (editor, mixer/monitor, recorder and cues)
	   have published their actions to the ActionManager
	*/
	PBD::Signal<void()> ActionsReady;

	/* signal emitted when escape key is pressed. All UI components that
	   need to respond to Escape in some way (e.g. break drag, clear
	   selection, etc) should connect to and handle this.
	*/
	PBD::Signal<void()> Escape;

	PublicEditor&	  the_editor() { return *editor;}
	Mixer_UI* the_mixer() { return mixer; }

	Gtk::Menu* shared_popup_menu ();

	void new_midi_tracer_window ();
	void toggle_editing_space();
	void toggle_mixer_space();
	void toggle_keep_tearoffs();
	void show_plugin_manager();
	void show_lua_window();
	void show_realtime_analyzer();

	void reset_focus (Gtk::Widget*);

	/** Emitted frequently with the audible sample, false, and the edit point as
	 *  parameters respectively.
	 *
	 *  (either RapidScreenUpdate || SuperRapidScreenUpdate - user-config)
	 */
	static sigc::signal<void, Temporal::timepos_t> Clock;
	static unsigned int clock_signal_interval ();

	static void close_all_dialogs () { CloseAllDialogs(); }
	static sigc::signal<void> CloseAllDialogs;

	XMLNode* main_window_settings() const;
	XMLNode* editor_settings() const;
	XMLNode* preferences_settings() const;
	XMLNode* mixer_settings () const;
	XMLNode* trigger_page_settings () const;
	XMLNode* recorder_settings () const;
	XMLNode* keyboard_settings () const;
	XMLNode* clock_mode_settings () const;
	XMLNode* tearoff_settings (const char*) const;

	void trigger_slot (int c, int r);
	void trigger_cue_row (int r);
	void stop_all_cues (bool immediately);
	void stop_cues (int c, bool immediately);

	void save_ardour_state ();
	gboolean configure_handler (GdkEventConfigure* conf);

	void halt_on_xrun_message ();
	void xrun_handler (samplepos_t);
	void create_xrun_marker (samplepos_t);

	Glib::RefPtr<Gtk::SizeGroup> button_height_size_group;

	GUIObjectState* gui_object_state;

	MainClock* primary_clock;    //this clock is the canonical PrimaryClock, whose mode is reflected to all other clocks with PrimaryClock disposition.
	MainClock* secondary_clock;  //this clock is the canonical SecondaryClock, whose mode is reflected to all other clocks with PrimaryClock disposition
	AudioClock*   big_clock;

	void focus_on_clock ();

	VideoTimeLine *video_timeline;

	void reset_main_clocks ();

	void synchronize_sync_source_and_video_pullup ();

	void add_route ();
	void add_route_dialog_response (int);

	void add_routes_part_two ();
	void add_routes_thread ();

	void start_duplicate_routes ();

	void add_video (Gtk::Window* float_window);
	void remove_video ();
	void start_video_server_menu (Gtk::Window* float_window);
	bool start_video_server (Gtk::Window* float_window, bool popup_msg);
	void stop_video_server (bool ask_confirm=false);
	void flush_videotimeline_cache (bool localcacheonly=false);
	void export_video (bool range = false);

	void session_add_audio_route (bool, int32_t, int32_t, ARDOUR::TrackMode, ARDOUR::RouteGroup *,
	                              uint32_t, std::string const &, bool, ARDOUR::PresentationInfo::order_t order, bool trigger_visibility);

	void session_add_midi_route (bool, ARDOUR::RouteGroup *, uint32_t, std::string const &, bool,
	                             ARDOUR::PluginInfoPtr, ARDOUR::Plugin::PresetRecord*,
	                             ARDOUR::PresentationInfo::order_t order, bool trigger_visibility);

	void session_add_foldback_bus (int32_t, uint32_t, std::string const &);

	void display_insufficient_ports_message ();

	void attach_to_engine ();
	void post_engine ();

	gint exit_on_main_window_close (GdkEventAny *);

	void maximise_editing_space ();
	void restore_editing_space ();

	void show_ui_prefs ();
	void show_mixer_prefs ();
	void show_plugin_prefs ();

	bool check_audioengine(Gtk::Window&);

	void setup_profile ();
	void setup_tooltips ();

	void set_shuttle_fract (double);

	void get_process_buffers ();
	void drop_process_buffers ();

	void show_library_download_window();

	const std::string& announce_string() const { return _announce_string; }

	void hide_application ();

	Gtk::Notebook& tabs();
	Gtk::Window& main_window () { return _main_window; }

	void setup_toplevel_window (Gtk::Window&, const std::string& name, void* owner);

	/* called from a static C function */

	GtkNotebook* tab_window_root_drop (GtkNotebook* src,
	                                   GtkWidget* w,
	                                   gint x,
	                                   gint y,
	                                   gpointer user_data);

	bool tabbed_window_state_event_handler (GdkEventWindowState*, void* object);
	bool key_event_handler (GdkEventKey*, Gtk::Window* window);

	ARDOUR::PresentationInfo::order_t translate_order (RouteDialogs::InsertAt);

	std::map<std::string, std::string> route_setup_info (const std::string& script_path);

	void gui_idle_handler ();

	RegionUISettingsManager region_ui_settings_manager;

protected:
	friend class PublicEditor;

	void toggle_use_monitor_section ();
	void monitor_dim_all ();
	void monitor_cut_all ();
	void monitor_mono ();

	void toggle_auto_play ();
	void toggle_auto_input ();
	void toggle_punch ();
	void unset_dual_punch ();
	bool ignore_dual_punch;
	void toggle_punch_in ();
	void toggle_punch_out ();
	void toggle_session_monitoring_in ();
	void toggle_session_monitoring_disk ();
	void show_loop_punch_ruler_and_disallow_hide ();
	void reenable_hide_loop_punch_ruler_if_appropriate ();
	void toggle_auto_return ();
	void toggle_click ();
	void toggle_click_on_rec ();
	void toggle_session_auto_loop ();
	void toggle_rc_options_window ();
	void toggle_session_options_window ();

private:

	Gtk::Window   _main_window;
	Gtkmm2ext::VisibilityTracker* main_window_visibility;
	Gtk::VBox      main_vpacker;
	Gtk::HBox      status_bar_hpacker;
	Gtk::Notebook _tabs;
	PublicEditor*  editor;
	Mixer_UI*      mixer;
	RecorderUI*    recorder;
	TriggerPage*   trigger_page;
	Gtk::Tooltips _tooltips;
	NSM_Client*    nsm;
	bool          _was_dirty;
	bool          _mixer_on_top;

	Gtk::Menu*    _shared_popup_menu;

	BasicUI*      _basic_ui;
	void hide_tabbable (ArdourWidgets::Tabbable*);
	void detach_tabbable (ArdourWidgets::Tabbable*);
	void attach_tabbable (ArdourWidgets::Tabbable*);
	void button_change_tabbable_visibility (ArdourWidgets::Tabbable*);
	void key_change_tabbable_visibility (ArdourWidgets::Tabbable*);
	void toggle_editor_and_mixer ();

	void tabbable_state_change (ArdourWidgets::Tabbable&);

	void toggle_meterbridge ();

	int  setup_windows ();
	void apply_window_settings (bool);
	void setup_transport ();
	void setup_clock ();

	static ARDOUR_UI *theArdourUI;
	SessionDialog *_session_dialog;

	StartupFSM* startup_fsm;

	int starting ();
	int nsm_init ();
	void startup_done ();
	void sfsm_response (StartupFSM::Result);

	int ask_about_saving_session (const std::vector<std::string>& actions);

	void audio_midi_setup_reconfigure_done (int response, std::string path, std::string snapshot, std::string mix_template);
	int  load_session_stage_two (const std::string& path, const std::string& snapshot, std::string mix_template = std::string());
	void audio_midi_setup_for_new_session_done (int response, std::string path, std::string snapshot, std::string session_template, ARDOUR::BusProfile const&, bool unnamed, Temporal::TimeDomain domain);
	sigc::connection _engine_dialog_connection;

	void save_session_at_its_request (std::string);
	/* periodic safety backup, to be precise */
	gint autosave_session();
	void update_autosave();
	sigc::connection _autosave_connection;

	void session_dirty_changed ();
	void update_title ();

	void map_transport_state ();
	int32_t do_engine_start ();

	void engine_halted (const char* reason, bool free_reason);
	void engine_stopped ();
	void engine_running (uint32_t cnt);

	void use_config ();

	void about_signal_response(int response);

	Gtk::VBox    top_packer;

	sigc::connection clock_signal_connection;
	void         update_clocks ();
	void         start_clocking ();
	void         stop_clocking ();

	void update_transport_clocks (samplepos_t pos);
	void record_state_changed ();

	std::list<MidiTracer*> _midi_tracer_windows;

	/* Transport Control */

	Gtk::Table               tabbables_table;

	void toggle_follow_edits ();

	void set_transport_controllable_state (const XMLNode&);
	XMLNode& get_transport_controllable_state ();

	void toggle_external_sync ();
	void toggle_time_master ();
	void toggle_video_sync ();

	ArdourWidgets::ArdourButton error_alert_button;

	void blink_handler (bool);
	sigc::connection blink_connection;

	void cancel_solo ();
	void error_blink (bool);

	void set_flat_buttons();

	bool error_alert_press (GdkEventButton *);
	/* menu bar and associated stuff */

	Gtk::MenuBar* menu_bar;
	Gtk::EventBox menu_bar_base;
	Gtk::HBox     menu_hbox;

	void use_menubar_as_top_menubar ();
	void build_menu_bar ();

	Gtk::Label   wall_clock_label;
	gint update_wall_clock ();

	Gtk::Label  disk_space_label;
	void update_disk_space ();
	void format_disk_space_label (float);

	Gtk::Label   timecode_format_label;
	void update_timecode_format ();

	Gtk::Label   latency_info_label;
	Gtk::Label   pdc_info_label;
	void session_latency_updated (bool);

	Gtk::Label  dsp_load_label;
	void update_cpu_load ();

	Gtk::Label   peak_thread_work_label;
	void update_peak_thread_work ();

	Gtk::Label   sample_rate_label;
	void update_sample_rate ();

	Gtk::Label    format_label;
	void update_format ();

	Gtk::Label session_path_label;
	void update_path_label ();

	Gtk::Label snapshot_name_label;

	void every_second ();
	void every_point_one_seconds ();

	sigc::connection second_connection;
	sigc::connection point_one_second_connection;
	sigc::connection fps_connection;

	void set_fps_timeout_connection ();

	void open_session ();
	void open_recent_session ();
	void save_template_dialog_response (int response, SaveTemplateDialog* d);
	void save_template ();
	void manage_templates ();

	void meta_session_setup (const std::string& script_path);
	void meta_route_setup (const std::string& script_path);

	void edit_metadata ();
	void import_metadata ();

	void set_punch_sensitivity ();

	//stuff for ProTools-style numpad
	void transport_numpad_event (int num);
	void transport_numpad_decimal ();
	bool _numpad_locate_happening;
	int _pending_locate_num;
	gint transport_numpad_timeout ();
	sigc::connection _numpad_timeout_connection;

	void transport_goto_nth_marker (int nth);
	void transport_goto_zero ();
	void transport_goto_start ();
	void transport_goto_end ();
	void transport_goto_wallclock ();
	void transport_stop ();
	void transport_record (bool roll);
	void transport_roll ();
	void transport_play_selection();
	void transport_play_preroll();
	void transport_rec_preroll();
	void transport_rec_count_in();
	void transport_forward ();
	void transport_rewind ();
	void transport_ffwd_rewind (bool fwd);
	void transport_loop ();
	void toggle_roll (bool with_abort, bool roll_out_of_bounded_mode);
	void spacebar_action (bool with_abort, bool roll_out_of_bounded_mode);
	bool trx_record_enable_all_tracks ();

	bool _session_is_new;
	void set_session (ARDOUR::Session *);
	void connect_dependents_to_session (ARDOUR::Session *);
	void we_have_dependents ();
	void setup_action_tooltips ();

	void setup_session_options ();

	guint32  last_key_press_time;

	bool process_snapshot_session_prompter (ArdourWidgets::Prompter& prompter, bool switch_to_it);
	void snapshot_session (bool switch_to_it);

	void quick_snapshot_session (bool switch_to_it);  //does not prompt for name, just makes a timestamped file

	SaveAsDialog* save_as_dialog;

	bool save_as_progress_update (float fraction, int64_t cnt, int64_t total, Gtk::Label* label, Gtk::ProgressBar* bar);
	void save_session_as ();
	void archive_session ();
	void rename_session (bool for_unnamed);

	int         create_mixer ();
	int         create_editor ();
	int         create_meterbridge ();
	int         create_masters ();
	int         create_recorder ();
	int         create_trigger_page ();

	Meterbridge  *meterbridge;

	/* Dialogs that can be created via new<T> */

	RCOptionEditor* rc_option_editor;
	Gtk::HBox rc_option_editor_placeholder;

	WM::Proxy<SpeakerDialog> speaker_config_window;
	WM::Proxy<AddRouteDialog> add_route_dialog;
	WM::Proxy<About> about;
	WM::Proxy<LocationUIWindow> location_ui;
	WM::Proxy<RouteParams_UI> route_params;
	WM::Proxy<EngineControl> audio_midi_setup;
	WM::Proxy<ExportVideoDialog> export_video_dialog;
	WM::Proxy<LuaScriptManager> lua_script_window;
	WM::Proxy<IdleOMeter> idleometer;
	WM::Proxy<IOPluginWindow> io_plugin_window;
	WM::Proxy<PluginManagerUI> plugin_manager_ui;
	WM::Proxy<PluginDSPLoadWindow> plugin_dsp_load_window;
	WM::Proxy<DspStatisticsWindow> dsp_statistics_window;
	WM::Proxy<TransportMastersWindow> transport_masters_window;

	/* Windows/Dialogs that require a creator method */

	WM::ProxyWithConstructor<SessionOptionEditor> session_option_editor;
	WM::ProxyWithConstructor<AddVideoDialog> add_video_dialog;
	WM::ProxyWithConstructor<BundleManager> bundle_manager;
	WM::ProxyWithConstructor<BigClockWindow> big_clock_window;
	WM::ProxyWithConstructor<BigTransportWindow> big_transport_window;
	WM::ProxyWithConstructor<VirtualKeyboardWindow> virtual_keyboard_window;
	WM::ProxyWithConstructor<LibraryDownloadDialog> library_download_window;
	WM::ProxyWithConstructor<GlobalPortMatrixWindow> audio_port_matrix;
	WM::ProxyWithConstructor<GlobalPortMatrixWindow> midi_port_matrix;
	WM::ProxyWithConstructor<KeyEditor> key_editor;
	WM::ProxyWithConstructor<LuaWindow> luawindow;
	WM::ProxyWithConstructor<RTAWindow> rtawindow;

	/* creator methods */

	SessionOptionEditor*    create_session_option_editor ();
	BundleManager*          create_bundle_manager ();
	AddVideoDialog*         create_add_video_dialog ();
	BigClockWindow*         create_big_clock_window();
	BigTransportWindow*     create_big_transport_window();
	VirtualKeyboardWindow*  create_virtual_keyboard_window();
	LibraryDownloadDialog*  create_library_download_window();
	GlobalPortMatrixWindow* create_global_port_matrix (ARDOUR::DataType);
	KeyEditor*              create_key_editor ();
	LuaWindow*              create_luawindow ();
	RTAWindow*              create_rtawindow ();

	ARDOUR::SystemExec *video_server_process;

	void handle_locations_change (ARDOUR::Location*);

	/* Keyboard Handling */

	ArdourKeyboard* keyboard;

	/* Keymap handling */

	void install_actions ();
	void install_dependent_actions ();

	void toggle_record_enable (uint16_t);

	uint32_t rec_enabled_streams;
	void count_recenabled_streams (ARDOUR::Route&);

	/* cleanup */

	Gtk::MenuItem *cleanup_item;

	void display_cleanup_results (ARDOUR::CleanupReport const& rep, const gchar* list_title, const bool msg_delete);
	void cleanup ();
	void cleanup_peakfiles ();
	void flush_trash ();

	bool have_configure_timeout;
	PBD::microseconds_t last_configure_time;
	gint configure_timeout ();

	PBD::microseconds_t last_peak_grab;
	PBD::microseconds_t last_shuttle_request;

	bool have_disk_speed_dialog_displayed;
	void disk_speed_dialog_gone (int ignored_response, Gtk::MessageDialog*);
	void disk_overrun_handler ();
	void disk_underrun_handler ();

	void session_format_mismatch (std::string, std::string);

	void session_dialog (std::string);
	int pending_state_dialog ();
	int sr_mismatch_dialog (ARDOUR::samplecnt_t, ARDOUR::samplecnt_t);
	void sr_mismatch_message (ARDOUR::samplecnt_t, ARDOUR::samplecnt_t);

	Gtk::MenuItem* jack_disconnect_item;
	Gtk::MenuItem* jack_reconnect_item;
	Gtk::Menu*     jack_bufsize_menu;

	Glib::RefPtr<Gtk::ActionGroup> common_actions;

	void editor_realized ();

	void toggle_use_mmc ();
	void toggle_send_mmc ();
	void toggle_send_mtc ();
	void toggle_send_midi_clock ();

	void toggle_use_osc ();

	void parameter_changed (std::string);
	void session_parameter_changed (std::string);

	bool first_idle ();

	void check_memory_locking ();

	void audioengine_setup ();

	void display_message (const char* prefix, gint prefix_len, Glib::RefPtr<Gtk::TextBuffer::Tag>, Glib::RefPtr<Gtk::TextBuffer::Tag>, const char* msg);
	Gtk::Label status_bar_label;
	bool status_bar_button_press (GdkEventButton*);

	PBD::ScopedConnectionList forever_connections;
	PBD::ScopedConnection halt_connection;
	PBD::ScopedConnection editor_meter_connection;

	/* these are used only in response to a platform-specific "ShouldQuit" signal */
	bool idle_finish ();
	void queue_finish ();
	void fontconfig_dialog ();

	int missing_file (ARDOUR::Session*s, std::string str, ARDOUR::DataType type);
	int ambiguous_file (std::string file, std::vector<std::string> hits);

	bool click_button_clicked (GdkEventButton *);

	VisibilityGroup _status_bar_visibility;

	/** A ProcessThread so that we have some thread-local buffers for use by
	 *  PluginEqGui::impulse_analysis ().
	 */
	ARDOUR::ProcessThread* _process_thread;

	void toggle_latency_switch ();
	void latency_switch_changed ();

	ArdourWidgets::ArdourButton             midi_panic_button;
	void                     midi_panic ();

	ArdourLogLevel _log_not_acknowledged;

	void on_theme_changed ();

	bool path_button_press (GdkEventButton* ev);
	bool audio_button_press (GdkEventButton* ev);
	bool format_button_press (GdkEventButton* ev);
	bool timecode_button_press (GdkEventButton* ev);
	bool xrun_button_press (GdkEventButton* ev);
	bool xrun_button_release (GdkEventButton* ev);

	std::string _announce_string;
	void check_announcements ();

	void audioengine_became_silent ();

	DuplicateRouteDialog* duplicate_routes_dialog;

	void grab_focus_after_dialog ();

	void tabs_switch (GtkNotebookPage*, guint page_number);
	void tabs_page_added (Gtk::Widget*, guint);
	void tabs_page_removed (Gtk::Widget*, guint);
	ArdourWidgets::ArdourButton editor_visibility_button;
	ArdourWidgets::ArdourButton mixer_visibility_button;
	ArdourWidgets::ArdourButton prefs_visibility_button;
	ArdourWidgets::ArdourButton recorder_visibility_button;
	ArdourWidgets::ArdourButton trigger_page_visibility_button;

	bool key_press_focus_accelerator_handler (Gtk::Window& window, GdkEventKey* ev, Gtkmm2ext::BindingSet*);
	bool try_gtk_accel_binding (GtkWindow* win, GdkEventKey* ev, bool translate, GdkModifierType modifier);

	bool main_window_delete_event (GdkEventAny*);
	bool idle_ask_about_quit ();

	void step_up_through_tabs ();
	void step_down_through_tabs ();

	void escape ();
	void close_current_dialog ();

	void ask_about_scratch_deletion ();
	bool nsm_first_session_opened;

	PBD::ScopedConnectionList clock_state_connection;
};

