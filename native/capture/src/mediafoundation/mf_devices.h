#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace FrameDirector {

/// Engine singleton "MediaFoundation". register_devices() adds every Windows
/// video capture device to the CameraServer and keeps the list current as
/// devices are plugged in or removed.
class MediaFoundationDevices : public godot::Object {
	GDCLASS(MediaFoundationDevices, godot::Object)

public:
	struct DeviceInfo {
		std::wstring link;
		std::wstring name;
	};

	MediaFoundationDevices();
	~MediaFoundationDevices();

	int register_devices();
	void unregister_devices();
	godot::Array get_feeds() const;
	godot::String get_last_error() const;

protected:
	static void _bind_methods();

private:
	void _watch_loop();
	void _apply_device_list();
	void _sync(const std::vector<DeviceInfo> &p_devices);

	godot::Array feeds;
	godot::String last_error;
	bool registered = false;
	bool mf_started = false;
	bool process_frame_connected = false;

	std::thread watcher;
	std::atomic<bool> stop_watching{ false };
	std::mutex pending_mutex;
	std::vector<DeviceInfo> pending;
	bool pending_valid = false;
};

} // namespace FrameDirector
