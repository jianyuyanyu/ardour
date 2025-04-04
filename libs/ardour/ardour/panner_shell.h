/*
 * Copyright (C) 2011-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2011 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013-2014 Robin Gareus <robin@gareus.org>
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

#include <cmath>
#include <cassert>
#include <vector>
#include <string>
#include <iostream>

#include "evoral/Parameter.h"
#include "pbd/cartesian.h"
#include "temporal/domain_provider.h"

#include "ardour/libardour_visibility.h"
#include "ardour/types.h"
#include "ardour/session_object.h"

namespace ARDOUR {

class Session;
class Route;
class Panner;
class BufferSet;
class AudioBuffer;
class Speakers;
class Pannable;

/** Class to manage panning by instantiating and controlling
 *  an appropriate Panner object for a given in/out configuration.
 */
class LIBARDOUR_API PannerShell : public SessionObject
{
public:
	PannerShell (std::string name, Session&, std::shared_ptr<Pannable>, Temporal::TimeDomainProvider const &, bool is_send = false);
	virtual ~PannerShell ();

	std::string describe_parameter (Evoral::Parameter param);

	bool can_support_io_configuration (const ChanCount& /*in*/, ChanCount& /*out*/) { return true; };
	void configure_io (ChanCount in, ChanCount out);

	/// The fundamental Panner function
	void run (BufferSet& src, BufferSet& dest, samplepos_t start_sample, samplepos_t end_samples, pframes_t nframes);

	XMLNode& get_state () const;
	int      set_state (const XMLNode&, int version);

	PBD::Signal<void()> PannableChanged; /* Pannable changed -- l*/
	PBD::Signal<void()> Changed; /* panner and/or outputs count and/or bypass state changed */

	std::shared_ptr<Panner> panner() const { return _panner; }
	std::shared_ptr<Pannable> pannable() const { return _panlinked ? _pannable_route : _pannable_internal; }
	std::shared_ptr<Pannable> unlinked_pannable () const { return _pannable_internal; }

	bool bypassed () const;
	void set_bypassed (bool);

	bool is_send () const { return (_is_send); }
	bool is_linked_to_route () const { return (_is_send && _panlinked); }
	/* this function takes the process lock: */
	void set_linked_to_route (bool);

	std::string current_panner_uri() const { return _current_panner_uri; }
	std::string user_selected_panner_uri() const { return _user_selected_panner_uri; }
	std::string panner_gui_uri() const { return _panner_gui_uri; }

	/* this function takes the process lock: */
	bool select_panner_by_uri (std::string const uri);

  private:
	void distribute_no_automation (BufferSet& src, BufferSet& dest, pframes_t nframes, gain_t gain_coeff);
	bool set_user_selected_panner_uri (std::string const uri);

	std::shared_ptr<Panner> _panner;

	std::shared_ptr<Pannable> _pannable_internal;
	std::shared_ptr<Pannable> _pannable_route;
	bool _is_send;
	bool _panlinked;
	bool _bypassed;

	std::string _current_panner_uri;
	std::string _user_selected_panner_uri;
	std::string _panner_gui_uri;
	bool _force_reselect;
};

} // namespace ARDOUR

