#pragma once

#include "../common/frame_pump.h"

#include <godot_cpp/classes/camera_feed.hpp>
#include <godot_cpp/variant/string.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

struct IMFMediaSource;

namespace FrameDirector {

/// A Windows video capture device (webcam, Elgato and other UVC/AVStream
/// capture cards) read through Media Foundation, exposed as a CameraFeed.
class MediaFoundationFeed : public godot::CameraFeed {
	GDCLASS(MediaFoundationFeed, godot::CameraFeed)

public:
	MediaFoundationFeed();
	~MediaFoundationFeed();

	void setup(const std::wstring &p_symbolic_link, const godot::String &p_name);
	const std::wstring &symbolic_link() const { return link; }
	void shutdown();

	bool _activate_feed() override;
	void _deactivate_feed() override;

	/// Preferred mode; the closest one the device offers is used. Call before
	/// activating (the app does this from the channel's resolution/framerate).
	void set_requested_format(int p_width, int p_height, int p_fps);
	godot::String get_status() const;
	godot::Vector2i get_signal_size() const;
	/// Modes the device offers, as "1920x1080 @ 60 (YUY2)" strings. Opens the
	/// device briefly, so only call it from the UI on request.
	godot::PackedStringArray list_modes();

protected:
	static void _bind_methods();

private:
	void _capture_loop();
	void _push_frame();
	void _shutdown_source();

	std::wstring link;
	std::atomic<int> requested_width{ 1920 };
	std::atomic<int> requested_height{ 1080 };
	std::atomic<int> requested_fps{ 60 };

	std::thread worker;
	std::atomic<bool> stop_requested{ false };
	std::mutex source_mutex;
	IMFMediaSource *active_source = nullptr; // owned by the worker; kept so shutdown() can unblock ReadSample

	StatusText status;
	FramePump pump;
	bool process_frame_connected = false;
};

} // namespace FrameDirector
