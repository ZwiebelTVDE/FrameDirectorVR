#pragma once

#include <godot_cpp/classes/camera_feed.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace godot {
class SceneTree;
}

namespace FrameDirector {

/// Hands frames from a capture thread to a Godot CameraFeed.
/// The capture thread calls store_*(); the feed calls push() on the main thread
/// once per frame (SceneTree.process_frame), which uploads the newest frame.
/// Older frames are dropped, so a slow app never builds up latency.
class FramePump {
public:
	enum class Layout {
		YUYV, // packed 4:2:2, Y0 Cb Y1 Cr. Sent via set_ycbcr_image (one RGBA8 texel = 2 pixels)
		RGBA, // 8-bit RGBA. Sent via set_rgb_image
	};

	/// Copies rows from p_src (p_pitch bytes apart, may be negative for
	/// bottom-up images). p_row_bytes is the number of bytes to copy per row.
	/// If p_uyvy is set, UYVY input is reordered to YUYV while copying; if
	/// p_bgrx is set, BGRX input is reordered to RGBA (alpha forced to 255).
	void store(const uint8_t *p_src, ptrdiff_t p_pitch, unsigned int p_width, unsigned int p_height, Layout p_layout, bool p_uyvy = false, bool p_bgrx = false);

	void push(godot::CameraFeed *p_feed);
	void reset();

	unsigned int width() const { return frame_width; }
	unsigned int height() const { return frame_height; }

	static godot::SceneTree *scene_tree();

private:
	std::mutex mutex;
	std::vector<uint8_t> frame;
	Layout layout = Layout::YUYV;
	unsigned int frame_width = 0;
	unsigned int frame_height = 0;
	uint64_t frame_serial = 0;
	uint64_t pushed_serial = 0;
};

/// Thread-safe status text shown in the app ("Waiting for signal", ...).
class StatusText {
public:
	void set(const std::string &p_text) {
		std::lock_guard<std::mutex> lock(mutex);
		text = p_text;
	}
	std::string get() const {
		std::lock_guard<std::mutex> lock(mutex);
		return text;
	}

private:
	mutable std::mutex mutex;
	std::string text = "Idle";
};

} // namespace FrameDirector
