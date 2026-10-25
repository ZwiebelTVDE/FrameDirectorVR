#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <memory>
#include <vector>

namespace FrameDirector {

class BoardHost;
class DeltacastFeed;

/// Engine singleton "Deltacast". GDScript calls register_devices() once at
/// startup; every supported RX connector is then added to the CameraServer.
class DeltacastDevices : public godot::Object {
	GDCLASS(DeltacastDevices, godot::Object)

public:
	static DeltacastDevices *get_singleton() { return singleton; }

	DeltacastDevices();
	~DeltacastDevices();

	int register_devices();
	void unregister_devices();
	godot::String get_api_version() const;
	godot::String get_last_error() const;
	godot::Array get_feeds() const;

protected:
	static void _bind_methods();

private:
	static DeltacastDevices *singleton;
	std::vector<std::shared_ptr<BoardHost>> boards;
	godot::Array feeds;
	godot::String last_error;
	bool registered = false;
};

} // namespace FrameDirector
