#include "deltacast_feed.h"
#include "board_host.h"
#include "videomaster_helper.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <VideoMasterCppApi/exception.hpp>

#include <chrono>

using namespace godot;
using namespace std::chrono_literals;
using namespace Deltacast::Wrapper;

namespace FrameDirector {

void DeltacastFeed::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_status"), &DeltacastFeed::get_status);
	ClassDB::bind_method(D_METHOD("get_board_index"), &DeltacastFeed::get_board_index);
	ClassDB::bind_method(D_METHOD("get_rx_index"), &DeltacastFeed::get_rx_index);
	ClassDB::bind_method(D_METHOD("get_signal_size"), &DeltacastFeed::get_signal_size);
}

DeltacastFeed::DeltacastFeed() {}

DeltacastFeed::~DeltacastFeed() {
	shutdown();
}

void DeltacastFeed::setup(const std::shared_ptr<BoardHost> &p_board, unsigned int p_rx_index, const String &p_name) {
	board = p_board;
	rx_index = p_rx_index;
	set_name(p_name);
}

void DeltacastFeed::shutdown() {
	stop_requested = true;
	if (worker.joinable()) {
		worker.join();
	}
	if (process_frame_connected) {
		SceneTree *tree = FramePump::scene_tree();
		Callable push = callable_mp(this, &DeltacastFeed::_push_frame);
		if (tree && tree->is_connected("process_frame", push)) {
			tree->disconnect("process_frame", push);
		}
		process_frame_connected = false;
	}
}

bool DeltacastFeed::_activate_feed() {
	if (!board) {
		return false;
	}
	shutdown();
	stop_requested = false;
	pump.reset();
	// Frames are handed to Godot on the main thread, once per frame.
	SceneTree *tree = FramePump::scene_tree();
	if (!tree) {
		return false;
	}
	tree->connect("process_frame", callable_mp(this, &DeltacastFeed::_push_frame));
	process_frame_connected = true;
	worker = std::thread(&DeltacastFeed::_capture_loop, this);
	return true;
}

void DeltacastFeed::_deactivate_feed() {
	shutdown();
	status.set("Idle");
}

String DeltacastFeed::get_status() const {
	return String::utf8(status.get().c_str());
}

int DeltacastFeed::get_board_index() const {
	return board ? int(board->index()) : -1;
}

int DeltacastFeed::get_rx_index() const {
	return int(rx_index);
}

Vector2i DeltacastFeed::get_signal_size() const {
	return Vector2i(int(pump.width()), int(pump.height()));
}

bool DeltacastFeed::_signal_lost() {
	try {
		return !board->signal_present(rx_index);
	} catch (const std::exception &) {
		return false;
	}
}

// Mirrors the receive loop of DELTACAST's videomaster-video-monitor sample:
// wait for a signal, detect it, configure and start the stream, then pop slots
// until the signal changes or disappears, and start over.
void DeltacastFeed::_capture_loop() {
	while (!stop_requested) {
		try {
			status.set("Waiting for signal");
			while (!stop_requested && !board->signal_present(rx_index)) {
				std::this_thread::sleep_for(100ms);
			}
			if (stop_requested) {
				break;
			}

			auto rx_tech_stream = board->open_stream(rx_index);
			auto &rx_stream = VideoMaster::to_base_stream(rx_tech_stream);

			auto signal_information = VideoMaster::detect_information(rx_tech_stream);
			auto video_characteristics = VideoMaster::get_video_characteristics(signal_information);
			const unsigned int width = video_characteristics.width;
			const unsigned int height = video_characteristics.height;

			rx_stream.buffer_queue().set_depth(8);
			rx_stream.set_buffer_packing(VHD_BUFPACK_VIDEO_YUV422_8);
			VideoMaster::configure_stream(rx_tech_stream, signal_information);
			rx_stream.start();
			status.set(VideoMaster::describe(signal_information));

			while (!stop_requested) {
				if (!board->signal_present(rx_index)) {
					status.set("Signal lost");
					break;
				}
				if (VideoMaster::detect_information(rx_tech_stream) != signal_information) {
					break; // format changed: reopen with the new settings
				}
				auto slot = rx_stream.pop_slot();
				auto [buffer, buffer_size] = slot->video().buffer();
				// VideoMaster's 8-bit 4:2:2 packing is UYVY (Cb Y0 Cr Y1); the
				// pump reorders it to the YUYV layout Godot's packed path expects.
				if (buffer && width >= 2 && size_t(buffer_size) >= size_t(width) * height * 2) {
					pump.store(reinterpret_cast<const uint8_t *>(buffer), ptrdiff_t(width) * 2, width, height, FramePump::Layout::YUYV, true);
				}
			}
			// rx_tech_stream goes out of scope here, which stops and closes it.
		} catch (const ApiException &e) {
			// A timeout while the cable is pulled is expected, not an error.
			if (_signal_lost()) {
				status.set("Signal lost");
			} else {
				status.set(std::string("VideoMaster error: ") + e.what());
			}
			std::this_thread::sleep_for(1s);
		} catch (const std::exception &e) {
			status.set(std::string("Error: ") + e.what());
			std::this_thread::sleep_for(1s);
		}
	}
}

// Runs on the main thread once per frame (SceneTree.process_frame).
void DeltacastFeed::_push_frame() {
	pump.push(this);
}

} // namespace FrameDirector
