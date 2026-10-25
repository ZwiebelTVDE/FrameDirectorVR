#include "deltacast_devices.h"
#include "board_host.h"
#include "deltacast_feed.h"

#include <godot_cpp/classes/camera_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <VideoMasterCppApi/api.hpp>
#include <VideoMasterCppApi/exception.hpp>

using namespace godot;
using namespace Deltacast::Wrapper;

namespace FrameDirector {

DeltacastDevices *DeltacastDevices::singleton = nullptr;

void DeltacastDevices::_bind_methods() {
	ClassDB::bind_method(D_METHOD("register_devices"), &DeltacastDevices::register_devices);
	ClassDB::bind_method(D_METHOD("unregister_devices"), &DeltacastDevices::unregister_devices);
	ClassDB::bind_method(D_METHOD("get_api_version"), &DeltacastDevices::get_api_version);
	ClassDB::bind_method(D_METHOD("get_last_error"), &DeltacastDevices::get_last_error);
	ClassDB::bind_method(D_METHOD("get_feeds"), &DeltacastDevices::get_feeds);
}

DeltacastDevices::DeltacastDevices() {
	singleton = this;
}

DeltacastDevices::~DeltacastDevices() {
	unregister_devices();
	if (singleton == this) {
		singleton = nullptr;
	}
}

String DeltacastDevices::get_api_version() const {
	try {
		return String::utf8(api_version().c_str());
	} catch (const std::exception &e) {
		return String();
	}
}

String DeltacastDevices::get_last_error() const {
	return last_error;
}

Array DeltacastDevices::get_feeds() const {
	return feeds.duplicate();
}

int DeltacastDevices::register_devices() {
	if (registered) {
		return feeds.size();
	}
	registered = true;
	CameraServer *camera_server = CameraServer::get_singleton();
	if (!camera_server) {
		last_error = "CameraServer not available";
		return 0;
	}
	unsigned int board_count = 0;
	try {
		board_count = Board::count();
	} catch (const std::exception &e) {
		last_error = String("Could not enumerate DELTACAST boards: ") + String::utf8(e.what());
		UtilityFunctions::push_warning(last_error);
		return 0;
	}

	for (unsigned int b = 0; b < board_count; b++) {
		std::shared_ptr<BoardHost> host;
		std::string board_name;
		unsigned int rx_count = 0;
		try {
			host = std::make_shared<BoardHost>(b);
			board_name = host->name();
			rx_count = host->number_of_rx();
		} catch (const std::exception &e) {
			last_error = vformat("Could not open DELTACAST board %d: %s", int(b), String::utf8(e.what()));
			UtilityFunctions::push_warning(last_error);
			continue;
		}
		boards.push_back(host);

		for (unsigned int rx = 0; rx < rx_count; rx++) {
			bool supported = false;
			bool sdi = false;
			try {
				supported = host->rx_supported(rx);
				sdi = host->rx_is_sdi(rx);
			} catch (const std::exception &) {
			}
			if (!supported) {
				continue;
			}
			// Name includes the board index so two identical cards stay distinct,
			// and stays stable between runs (the app matches devices by name).
			String name = vformat(String::utf8("DELTACAST %s #%d · %s In %d"), String::utf8(board_name.c_str()), int(b), sdi ? "SDI" : "HDMI/DP", int(rx + 1));
			Ref<DeltacastFeed> feed;
			feed.instantiate();
			feed->setup(host, rx, name);
			camera_server->add_feed(feed);
			feeds.push_back(feed);
		}
	}
	UtilityFunctions::print_verbose(vformat("DELTACAST: %d board(s), %d input(s) registered", int(board_count), feeds.size()));
	return feeds.size();
}

void DeltacastDevices::unregister_devices() {
	CameraServer *camera_server = CameraServer::get_singleton();
	for (int i = 0; i < feeds.size(); i++) {
		Ref<DeltacastFeed> feed = feeds[i];
		if (feed.is_null()) {
			continue;
		}
		feed->shutdown();
		if (camera_server) {
			camera_server->remove_feed(feed);
		}
	}
	feeds.clear();
	boards.clear(); // closes the boards once no feed holds them any more
	registered = false;
}

} // namespace FrameDirector
