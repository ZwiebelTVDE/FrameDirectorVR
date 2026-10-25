#pragma once

#include "../common/frame_pump.h"

#include <godot_cpp/classes/camera_feed.hpp>
#include <godot_cpp/variant/string.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace FrameDirector {

class BoardHost;

/// One RX connector of a DELTACAST board, exposed to Godot as a CameraFeed.
/// Capture runs on a worker thread while the feed is active; frames are handed
/// to Godot on the main thread (SceneTree.process_frame).
class DeltacastFeed : public godot::CameraFeed {
	GDCLASS(DeltacastFeed, godot::CameraFeed)

public:
	DeltacastFeed();
	~DeltacastFeed();

	void setup(const std::shared_ptr<BoardHost> &p_board, unsigned int p_rx_index, const godot::String &p_name);
	void shutdown();

	bool _activate_feed() override;
	void _deactivate_feed() override;

	godot::String get_status() const;
	int get_board_index() const;
	int get_rx_index() const;
	godot::Vector2i get_signal_size() const;

protected:
	static void _bind_methods();

private:
	void _capture_loop();
	void _push_frame();
	bool _signal_lost();

	std::shared_ptr<BoardHost> board;
	unsigned int rx_index = 0;

	std::thread worker;
	std::atomic<bool> stop_requested{ false };

	StatusText status;
	FramePump pump;
	bool process_frame_connected = false;
};

} // namespace FrameDirector
