#include "mf_devices.h"
#include "../common/frame_pump.h"
#include "com_ptr.h"
#include "mf_feed.h"

#include <godot_cpp/classes/camera_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <mfapi.h>
#include <mfidl.h>

#include <chrono>
#include <map>

using namespace godot;
using namespace std::chrono_literals;

namespace FrameDirector {

namespace {

String to_godot(const std::wstring &p_text) {
	// wchar_t is UTF-16 on Windows.
	static_assert(sizeof(wchar_t) == sizeof(char16_t), "Media Foundation backend is Windows-only");
	return String::utf16(reinterpret_cast<const char16_t *>(p_text.c_str()), int(p_text.size()));
}

std::wstring allocated_string(IMFActivate *p_device, const GUID &p_key) {
	WCHAR *value = nullptr;
	UINT32 length = 0;
	std::wstring out;
	if (SUCCEEDED(p_device->GetAllocatedString(p_key, &value, &length)) && value) {
		out.assign(value, length);
		CoTaskMemFree(value);
	}
	return out;
}

std::vector<MediaFoundationDevices::DeviceInfo> enumerate_devices() {
	std::vector<MediaFoundationDevices::DeviceInfo> devices;
	ComPtr<IMFAttributes> attributes;
	if (FAILED(MFCreateAttributes(attributes.put(), 1))) {
		return devices;
	}
	attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	IMFActivate **list = nullptr;
	UINT32 count = 0;
	if (FAILED(MFEnumDeviceSources(attributes.get(), &list, &count))) {
		return devices;
	}
	for (UINT32 i = 0; i < count; i++) {
		MediaFoundationDevices::DeviceInfo info;
		info.name = allocated_string(list[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
		info.link = allocated_string(list[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
		if (!info.link.empty()) {
			devices.push_back(info);
		}
		list[i]->Release();
	}
	CoTaskMemFree(list);
	return devices;
}

} // namespace

void MediaFoundationDevices::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_devices"), &MediaFoundationDevices::register_devices);
	ClassDB::bind_method(D_METHOD("unregister_devices"), &MediaFoundationDevices::unregister_devices);
	ClassDB::bind_method(D_METHOD("get_feeds"), &MediaFoundationDevices::get_feeds);
	ClassDB::bind_method(D_METHOD("get_last_error"), &MediaFoundationDevices::get_last_error);
}

MediaFoundationDevices::MediaFoundationDevices() {}

MediaFoundationDevices::~MediaFoundationDevices() {
	unregister_devices();
}

Array MediaFoundationDevices::get_feeds() const {
	return feeds.duplicate();
}

String MediaFoundationDevices::get_last_error() const {
	return last_error;
}

int MediaFoundationDevices::register_devices() {
	if (registered) {
		return feeds.size();
	}
	registered = true;
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	(void)hr; // the main thread is usually initialised by Godot already
	if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
		last_error = "Media Foundation is not available on this system";
		UtilityFunctions::push_warning(last_error);
		return 0;
	}
	mf_started = true;
	_sync(enumerate_devices());

	SceneTree *tree = FramePump::scene_tree();
	if (tree) {
		tree->connect("process_frame", callable_mp(this, &MediaFoundationDevices::_apply_device_list));
		process_frame_connected = true;
	}
	stop_watching = false;
	watcher = std::thread(&MediaFoundationDevices::_watch_loop, this);
	return feeds.size();
}

void MediaFoundationDevices::unregister_devices() {
	stop_watching = true;
	if (watcher.joinable()) {
		watcher.join();
	}
	if (process_frame_connected) {
		SceneTree *tree = FramePump::scene_tree();
		Callable apply = callable_mp(this, &MediaFoundationDevices::_apply_device_list);
		if (tree && tree->is_connected("process_frame", apply)) {
			tree->disconnect("process_frame", apply);
		}
		process_frame_connected = false;
	}
	_sync({});
	if (mf_started) {
		MFShutdown();
		mf_started = false;
	}
	registered = false;
}

// Enumerating devices can take a few milliseconds, so it runs here and the
// result is applied on the main thread, without a hitch in VR.
void MediaFoundationDevices::_watch_loop() {
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool owns_com = SUCCEEDED(hr);
	MFStartup(MF_VERSION, MFSTARTUP_LITE);
	while (!stop_watching) {
		for (int i = 0; i < 20 && !stop_watching; i++) {
			std::this_thread::sleep_for(100ms);
		}
		if (stop_watching) {
			break;
		}
		std::vector<DeviceInfo> devices = enumerate_devices();
		std::lock_guard<std::mutex> lock(pending_mutex);
		pending = std::move(devices);
		pending_valid = true;
	}
	MFShutdown();
	if (owns_com) {
		CoUninitialize();
	}
}

void MediaFoundationDevices::_apply_device_list() {
	std::vector<DeviceInfo> devices;
	{
		std::lock_guard<std::mutex> lock(pending_mutex);
		if (!pending_valid) {
			return;
		}
		devices = std::move(pending);
		pending_valid = false;
	}
	_sync(devices);
}

void MediaFoundationDevices::_sync(const std::vector<DeviceInfo> &p_devices) {
	CameraServer *camera_server = CameraServer::get_singleton();
	if (!camera_server) {
		return;
	}
	// Remove feeds whose device is gone.
	for (int i = feeds.size() - 1; i >= 0; i--) {
		Ref<MediaFoundationFeed> feed = feeds[i];
		bool still_there = false;
		for (const DeviceInfo &device : p_devices) {
			if (device.link == feed->symbolic_link()) {
				still_there = true;
				break;
			}
		}
		if (!still_there) {
			feed->shutdown();
			camera_server->remove_feed(feed);
			feeds.remove_at(i);
		}
	}
	// Add new devices. Identical models get " (2)", " (3)"... so names stay
	// unique (the app remembers devices by name).
	std::map<std::wstring, int> name_count;
	for (const DeviceInfo &device : p_devices) {
		int n = ++name_count[device.name];
		bool known = false;
		for (int i = 0; i < feeds.size(); i++) {
			Ref<MediaFoundationFeed> feed = feeds[i];
			if (feed->symbolic_link() == device.link) {
				known = true;
				break;
			}
		}
		if (known) {
			continue;
		}
		String name = device.name.empty() ? String("Video capture device") : to_godot(device.name);
		if (n > 1) {
			name += vformat(" (%d)", n);
		}
		Ref<MediaFoundationFeed> feed;
		feed.instantiate();
		feed->setup(device.link, name);
		camera_server->add_feed(feed);
		feeds.push_back(feed);
	}
}

} // namespace FrameDirector
