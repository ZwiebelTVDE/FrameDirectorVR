#include "frame_pump.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstring>

using namespace godot;

namespace FrameDirector {

SceneTree *FramePump::scene_tree() {
	Engine *engine = Engine::get_singleton();
	return engine ? Object::cast_to<SceneTree>(engine->get_main_loop()) : nullptr;
}

void FramePump::store(const uint8_t *p_src, ptrdiff_t p_pitch, unsigned int p_width, unsigned int p_height, Layout p_layout, bool p_uyvy, bool p_bgrx) {
	if (!p_src || p_width < 2 || p_height == 0) {
		return;
	}
	const size_t row_bytes = size_t(p_width) * (p_layout == Layout::YUYV ? 2 : 4);
	std::lock_guard<std::mutex> lock(mutex);
	frame.resize(row_bytes * p_height);
	for (unsigned int y = 0; y < p_height; y++) {
		const uint8_t *src = p_src + ptrdiff_t(y) * p_pitch;
		uint8_t *dst = frame.data() + row_bytes * y;
		if (p_uyvy) {
			for (size_t i = 0; i + 3 < row_bytes; i += 4) {
				dst[i + 0] = src[i + 1];
				dst[i + 1] = src[i + 0];
				dst[i + 2] = src[i + 3];
				dst[i + 3] = src[i + 2];
			}
		} else if (p_bgrx) {
			for (size_t i = 0; i + 3 < row_bytes; i += 4) {
				dst[i + 0] = src[i + 2];
				dst[i + 1] = src[i + 1];
				dst[i + 2] = src[i + 0];
				dst[i + 3] = 255;
			}
		} else {
			std::memcpy(dst, src, row_bytes);
		}
	}
	layout = p_layout;
	frame_width = p_width;
	frame_height = p_height;
	frame_serial++;
}

void FramePump::reset() {
	std::lock_guard<std::mutex> lock(mutex);
	frame_serial = 0;
	pushed_serial = 0;
	frame_width = 0;
	frame_height = 0;
}

void FramePump::push(CameraFeed *p_feed) {
	PackedByteArray bytes;
	int tex_width = 0;
	int tex_height = 0;
	Layout frame_layout;
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (frame_serial == pushed_serial || frame.empty()) {
			return;
		}
		pushed_serial = frame_serial;
		frame_layout = layout;
		tex_width = int(frame_layout == Layout::YUYV ? frame_width / 2 : frame_width);
		tex_height = int(frame_height);
		bytes.resize(int64_t(frame.size()));
		std::memcpy(bytes.ptrw(), frame.data(), frame.size());
	}
	Ref<Image> image = Image::create_from_data(tex_width, tex_height, false, Image::FORMAT_RGBA8, bytes);
	if (image.is_null()) {
		return;
	}
	if (frame_layout == Layout::YUYV) {
		p_feed->set_ycbcr_image(image);
	} else {
		p_feed->set_rgb_image(image);
	}
}

} // namespace FrameDirector
