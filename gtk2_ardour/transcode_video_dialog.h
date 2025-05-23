/*
 * Copyright (C) 2013-2017 Robin Gareus <robin@gareus.org>
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

#include <string>

#include <ytkmm/adjustment.h>
#include <ytkmm/box.h>
#include <ytkmm/button.h>
#include <ytkmm/checkbutton.h>
#include <ytkmm/comboboxtext.h>
#include <ytkmm/entry.h>
#include <ytkmm/label.h>
#include <ytkmm/progressbar.h>
#include <ytkmm/spinbutton.h>

#include "ardour/template_utils.h"
#include "ardour/types.h"
#include "ardour_dialog.h"

#include "transcode_ffmpeg.h"

enum VtlTranscodeOption {
	VTL_IMPORT_REFERENCE  = 0,
	VTL_IMPORT_TRANSCODED = 1,
	VTL_IMPORT_NO_VIDEO   = 2
};

/** @class TranscodeVideoDialog
 *  @brief dialog-box and controller for importing video-files
 */
class TranscodeVideoDialog : public ArdourDialog, public PBD::ScopedConnectionList
{
public:
	TranscodeVideoDialog (ARDOUR::Session*, std::string);
	~TranscodeVideoDialog ();

	VtlTranscodeOption import_option ();

	std::string get_filename ()
	{
		return path_entry.get_text ();
	}

	std::string get_audiofile ()
	{
		return audiofile;
	}

	bool detect_ltc ()
	{
		return ltc_detect.get_active ();
	}

	void on_response (int response_id)
	{
		Gtk::Dialog::on_response (response_id);
	}

private:
	void on_show ();
	void open_browse_dialog ();
	void abort_clicked ();
	void scale_combo_changed ();
	void audio_combo_changed ();
	void video_combo_changed ();
	void aspect_checkbox_toggled ();
	void bitrate_checkbox_toggled ();
	void update_bitrate ();
	void launch_audioonly ();
	void launch_transcode ();
	void launch_extract ();
	void dialog_progress_mode ();
	void finished (int);
	void update_progress (ARDOUR::samplecnt_t, ARDOUR::samplecnt_t);

	bool        aborted;
	bool        pending_audio_extract;
	std::string audiofile;
	std::string infn;
	double      m_aspect;

	PBD::Signal<void()> StartNextStage;

	TranscodeFfmpeg* transcoder;

	Gtk::Label  path_label;
	Gtk::Entry  path_entry;
	Gtk::Button browse_button;
	Gtk::Button transcode_button;

	Gtk::VBox*   vbox;
	Gtk::Button* cancel_button;
	Gtk::Button  abort_button;

	Gtk::VBox*       progress_box;
	Gtk::Label       progress_label;
	Gtk::ProgressBar pbar;

	Gtk::ComboBoxText video_combo;
	Gtk::ComboBoxText scale_combo;
	Gtk::CheckButton  aspect_checkbox;
	Gtk::Adjustment   height_adjustment;
	Gtk::SpinButton   height_spinner;
	Gtk::ComboBoxText audio_combo;
	Gtk::CheckButton  ltc_detect;
	Gtk::CheckButton  bitrate_checkbox;
	Gtk::Adjustment   bitrate_adjustment;
	Gtk::SpinButton   bitrate_spinner;
	Gtk::CheckButton  debug_checkbox;
};

