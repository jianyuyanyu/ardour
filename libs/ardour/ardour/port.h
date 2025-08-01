/*
 * Copyright (C) 2006-2011 David Robillard <d@drobilla.net>
 * Copyright (C) 2006-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2015-2019 Robin Gareus <robin@gareus.org>
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

#ifdef WAF_BUILD
#include "libardour-config.h"
#endif

#include <set>
#include <string>
#include <vector>
#include "pbd/signals.h"

#include "ardour/data_type.h"
#include "ardour/port_engine.h"
#include "ardour/libardour_visibility.h"
#include "ardour/types.h"

namespace ARDOUR {

class AudioEngine;
class Buffer;

class LIBARDOUR_API Port
{
public:
	Port (const Port&) = delete;
	Port& operator= (const Port&) = delete;
	virtual ~Port ();

	static void set_connecting_blocked( bool yn ) {
		_connecting_blocked = yn;
	}
	static bool connecting_blocked() {
		return _connecting_blocked;
	}

	/** @return Port short name */
	std::string name () const {
		return _name;
	}

	/** @return Port human readable name */
	std::string pretty_name (bool fallback_to_name = false) const;
	bool set_pretty_name (const std::string&);

	int set_name (std::string const &);

	/** @return flags */
	PortFlags flags () const {
		return _flags;
	}

	/** @return true if this Port receives input, otherwise false */
	bool receives_input () const {
		return _flags & IsInput;
	}

	/** @return true if this Port sends output, otherwise false */
	bool sends_output () const {
		return _flags & IsOutput;
	}

	bool connected () const;
	int disconnect_all ();
	int get_connections (std::vector<std::string> &) const;

	/* connection by name */
	bool connected_to (std::string const &) const;
	int connect (std::string const &);
	int disconnect (std::string const &);

	/* connection by Port* */
	bool connected_to (Port *) const;
	virtual int connect (Port *);
	int disconnect (Port *);

	void request_input_monitoring (bool);
	void ensure_input_monitoring (bool);
	bool monitoring_input () const;
	int reestablish ();
	int reconnect ();

	bool last_monitor() const { return _last_monitor; }
	void set_last_monitor (bool yn) { _last_monitor = yn; }

	PortEngine::PortHandle port_handle() { return _port_handle; }

	void get_connected_latency_range (LatencyRange& range, bool playback) const;

	void collect_latency_from_backend (LatencyRange& range, bool playback) const;

	void set_private_latency_range (LatencyRange& range, bool playback);
	const LatencyRange&  private_latency_range (bool playback) const;

	void set_public_latency_range (LatencyRange const& range, bool playback) const;
	LatencyRange public_latency_range (bool playback) const;

	virtual void reset ();

	virtual DataType type () const = 0;
	virtual void cycle_start (pframes_t);
	virtual void cycle_end (pframes_t);
	virtual void cycle_split () = 0;
	virtual void reinit (bool) {}
	virtual Buffer& get_buffer (pframes_t nframes) = 0;
	virtual void flush_buffers (pframes_t /*nframes*/) {}
	virtual void transport_stopped () {}
	virtual void realtime_locate (bool for_loop_end) {}
	virtual void set_buffer_size (pframes_t) {}

	bool has_ext_connection () const;
	bool physically_connected () const;
	bool in_cycle () const { return _in_cycle; }

	uint32_t externally_connected () const { return _externally_connected; }
	uint32_t internally_connected () const { return _internally_connected; }

	void rename_connected_port (std::string const&, std::string const&);

	void increment_external_connections ();
	void decrement_external_connections ();

	void increment_internal_connections ();
	void decrement_internal_connections ();


	PBD::Signal<void(bool)> MonitorInputChanged;
	PBD::Signal<void(std::shared_ptr<Port>,std::shared_ptr<Port>, bool )> ConnectedOrDisconnected;

	static PBD::Signal<void()> PortDrop;
	static PBD::Signal<void()> PortSignalDrop;
	static PBD::Signal<void()> ResamplerQualityChanged;

	static void set_varispeed_ratio (double s); //< varispeed playback
	static bool set_engine_ratio (double session, double engine); //< SR mismatch
	static void set_cycle_samplecnt (pframes_t n);

	static samplecnt_t port_offset() { return _global_port_buffer_offset; }
	static void set_global_port_buffer_offset (pframes_t off) {
		_global_port_buffer_offset = off;
	}
	static void increment_global_port_buffer_offset (pframes_t n) {
		_global_port_buffer_offset += n;
	}

	virtual XMLNode& get_state () const;
	virtual int set_state (const XMLNode&, int version);

	static std::string state_node_name;

	static pframes_t cycle_nframes () { return _cycle_nframes; }
	static double speed_ratio () { return _speed_ratio; }
	static double engine_ratio () { return _engine_ratio; }
	static double resample_ratio () { return _resample_ratio; } // == _speed_ratio * _engine_ratio

	static uint32_t resampler_quality () { return _resampler_quality; }
	static uint32_t resampler_latency () { return _resampler_latency; }
	static bool     can_varispeed ()     { return _resampler_latency > 0; }
	static bool     setup_resampler (uint32_t q = 17);

protected:

	Port (std::string const &, DataType, PortFlags);

	PortEngine::PortPtr _port_handle;

	static bool       _connecting_blocked;
	static pframes_t  _cycle_nframes; /* access only from process() tree */

	static pframes_t  _global_port_buffer_offset; /* access only from process() tree */

	LatencyRange _private_playback_latency;
	LatencyRange _private_capture_latency;

	static double _speed_ratio;
	static double _engine_ratio;
	static double _resample_ratio; // = _speed_ratio * _engine_ratio (cached)

private:
	std::string _name;  ///< port short name
	PortFlags   _flags; ///< flags
	bool        _last_monitor;
	bool        _in_cycle;
	uint32_t    _externally_connected;
	uint32_t    _internally_connected;

	typedef std::set<std::string> ConnectionSet;
	/* ports that we are connected to, kept so that we can
	 * reconnect to the backend when required
	 */
	mutable Glib::Threads::RWLock        _connections_lock;
	ConnectionSet                        _int_connections;
	std::map<std::string, ConnectionSet> _ext_connections;

	static uint32_t _resampler_quality; // 8 <= q <= 96
	static uint32_t _resampler_latency; // = _resampler_quality - 1

	void port_connected_or_disconnected (std::weak_ptr<Port>, std::string, std::weak_ptr<Port>, std::string, bool);

	int  connect_internal (std::string const &);
	void insert_connection (std::string const&);
	void erase_connection (std::string const&);

	void signal_drop ();
	void session_global_drop ();
	void drop ();
	PBD::ScopedConnectionList drop_connection;
	PBD::ScopedConnection engine_connection;
};

}

