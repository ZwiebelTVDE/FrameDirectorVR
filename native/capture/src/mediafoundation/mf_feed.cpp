#include "mf_feed.h"
#include "com_ptr.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace godot;
using namespace std::chrono_literals;

namespace FrameDirector {

namespace {

constexpr DWORD VIDEO_STREAM = DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM);

struct Mode {
	DWORD index = 0;
	UINT32 width = 0;
	UINT32 height = 0;
	double fps = 0.0;
	GUID subtype = GUID_NULL;
};

std::string fourcc(const GUID &p_subtype) {
	if (p_subtype == MFVideoFormat_RGB32) return "RGB32";
	if (p_subtype == MFVideoFormat_ARGB32) return "ARGB32";
	if (p_subtype == MFVideoFormat_RGB24) return "RGB24";
	char text[5] = {};
	for (int i = 0; i < 4; i++) {
		char c = char((p_subtype.Data1 >> (8 * i)) & 0xff);
		text[i] = (c >= 32 && c < 127) ? c : '?';
	}
	return text;
}

std::string hresult_text(HRESULT p_hr) {
	char text[32];
	std::snprintf(text, sizeof(text), "0x%08lX", static_cast<unsigned long>(p_hr));
	return text;
}

/// COM must be initialised on every thread that touches Media Foundation.
class ComScope {
public:
	ComScope() {
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		owns = SUCCEEDED(hr); // RPC_E_CHANGED_MODE: already initialised differently, fine
		mf_ok = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
	}
	~ComScope() {
		if (mf_ok) MFShutdown();
		if (owns) CoUninitialize();
	}

private:
	bool owns = false;
	bool mf_ok = false;
};

HRESULT create_source(const std::wstring &p_link, ComPtr<IMFMediaSource> &r_source) {
	ComPtr<IMFAttributes> attributes;
	HRESULT hr = MFCreateAttributes(attributes.put(), 2);
	if (SUCCEEDED(hr)) hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (SUCCEEDED(hr)) hr = attributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, p_link.c_str());
	if (SUCCEEDED(hr)) hr = MFCreateDeviceSource(attributes.get(), r_source.put());
	return hr;
}

HRESULT create_reader(IMFMediaSource *p_source, ComPtr<IMFSourceReader> &r_reader) {
	ComPtr<IMFAttributes> attributes;
	HRESULT hr = MFCreateAttributes(attributes.put(), 2);
	// Lets the reader decode MJPG/H264 and convert NV12 etc. to the format we ask for.
	if (SUCCEEDED(hr)) hr = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
	if (SUCCEEDED(hr)) hr = attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
	if (SUCCEEDED(hr)) hr = MFCreateSourceReaderFromMediaSource(p_source, attributes.get(), r_reader.put());
	return hr;
}

std::vector<Mode> native_modes(IMFSourceReader *p_reader) {
	std::vector<Mode> modes;
	for (DWORD i = 0;; i++) {
		ComPtr<IMFMediaType> type;
		if (FAILED(p_reader->GetNativeMediaType(VIDEO_STREAM, i, type.put()))) {
			break; // MF_E_NO_MORE_TYPES
		}
		Mode mode;
		mode.index = i;
		MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &mode.width, &mode.height);
		UINT32 num = 0, den = 0;
		if (SUCCEEDED(MFGetAttributeRatio(type.get(), MF_MT_FRAME_RATE, &num, &den)) && den != 0) {
			mode.fps = double(num) / double(den);
		}
		type->GetGUID(MF_MT_SUBTYPE, &mode.subtype);
		if (mode.width > 0 && mode.height > 0) {
			modes.push_back(mode);
		}
	}
	return modes;
}

/// Closest resolution first, then framerate. Uncompressed formats win ties
/// because they add no decoding delay.
const Mode *pick_mode(const std::vector<Mode> &p_modes, int p_width, int p_height, int p_fps) {
	const Mode *best = nullptr;
	double best_score = 1e30;
	for (const Mode &mode : p_modes) {
		double score = std::abs(double(mode.width) - p_width) * 10.0 + std::abs(double(mode.height) - p_height) * 10.0;
		score += std::abs(mode.fps - p_fps) * 2.0;
		if (mode.subtype == MFVideoFormat_MJPG) score += 1.0;
		else if (mode.subtype == MFVideoFormat_H264) score += 5.0;
		if (score < best_score) {
			best_score = score;
			best = &mode;
		}
	}
	return best;
}

HRESULT set_output(IMFSourceReader *p_reader, const GUID &p_subtype) {
	ComPtr<IMFMediaType> type;
	HRESULT hr = MFCreateMediaType(type.put());
	if (SUCCEEDED(hr)) hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (SUCCEEDED(hr)) hr = type->SetGUID(MF_MT_SUBTYPE, p_subtype);
	if (SUCCEEDED(hr)) hr = p_reader->SetCurrentMediaType(VIDEO_STREAM, nullptr, type.get());
	return hr;
}

struct OutputFormat {
	UINT32 width = 0;
	UINT32 height = 0;
	LONG default_stride = 0;
	FramePump::Layout layout = FramePump::Layout::YUYV;
	bool uyvy = false;
	bool bgrx = false;
	std::string name;
};

HRESULT read_output(IMFSourceReader *p_reader, OutputFormat &r_format) {
	ComPtr<IMFMediaType> type;
	HRESULT hr = p_reader->GetCurrentMediaType(VIDEO_STREAM, type.put());
	if (FAILED(hr)) return hr;
	hr = MFGetAttributeSize(type.get(), MF_MT_FRAME_SIZE, &r_format.width, &r_format.height);
	if (FAILED(hr)) return hr;
	const LONG bytes_per_pixel = r_format.layout == FramePump::Layout::YUYV ? 2 : 4;
	UINT32 stride = 0;
	if (SUCCEEDED(type->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride))) {
		r_format.default_stride = LONG(INT32(stride));
	} else {
		r_format.default_stride = LONG(r_format.width) * bytes_per_pixel;
	}
	return S_OK;
}

} // namespace

void MediaFoundationFeed::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_requested_format", "width", "height", "fps"), &MediaFoundationFeed::set_requested_format);
	ClassDB::bind_method(D_METHOD("get_status"), &MediaFoundationFeed::get_status);
	ClassDB::bind_method(D_METHOD("get_signal_size"), &MediaFoundationFeed::get_signal_size);
	ClassDB::bind_method(D_METHOD("list_modes"), &MediaFoundationFeed::list_modes);
}

MediaFoundationFeed::MediaFoundationFeed() {}

MediaFoundationFeed::~MediaFoundationFeed() {
	shutdown();
}

void MediaFoundationFeed::setup(const std::wstring &p_symbolic_link, const String &p_name) {
	link = p_symbolic_link;
	set_name(p_name);
}

void MediaFoundationFeed::set_requested_format(int p_width, int p_height, int p_fps) {
	requested_width = p_width;
	requested_height = p_height;
	requested_fps = p_fps;
}

String MediaFoundationFeed::get_status() const {
	return String::utf8(status.get().c_str());
}

Vector2i MediaFoundationFeed::get_signal_size() const {
	return Vector2i(int(pump.width()), int(pump.height()));
}

void MediaFoundationFeed::_shutdown_source() {
	// Shutting the source down makes a blocking ReadSample() return at once,
	// so the worker can't hang if a device stops delivering frames.
	std::lock_guard<std::mutex> lock(source_mutex);
	if (active_source) {
		active_source->Shutdown();
	}
}

void MediaFoundationFeed::shutdown() {
	stop_requested = true;
	_shutdown_source();
	if (worker.joinable()) {
		worker.join();
	}
	if (process_frame_connected) {
		SceneTree *tree = FramePump::scene_tree();
		Callable push = callable_mp(this, &MediaFoundationFeed::_push_frame);
		if (tree && tree->is_connected("process_frame", push)) {
			tree->disconnect("process_frame", push);
		}
		process_frame_connected = false;
	}
}

bool MediaFoundationFeed::_activate_feed() {
	shutdown();
	SceneTree *tree = FramePump::scene_tree();
	if (!tree) {
		return false;
	}
	stop_requested = false;
	pump.reset();
	tree->connect("process_frame", callable_mp(this, &MediaFoundationFeed::_push_frame));
	process_frame_connected = true;
	worker = std::thread(&MediaFoundationFeed::_capture_loop, this);
	return true;
}

void MediaFoundationFeed::_deactivate_feed() {
	shutdown();
	status.set("Idle");
}

void MediaFoundationFeed::_push_frame() {
	pump.push(this);
}

PackedStringArray MediaFoundationFeed::list_modes() {
	PackedStringArray out;
	ComScope com;
	ComPtr<IMFMediaSource> source;
	ComPtr<IMFSourceReader> reader;
	if (FAILED(create_source(link, source)) || FAILED(create_reader(source.get(), reader))) {
		return out;
	}
	for (const Mode &mode : native_modes(reader.get())) {
		char text[96];
		std::snprintf(text, sizeof(text), "%ux%u @ %.2f (%s)", mode.width, mode.height, mode.fps, fourcc(mode.subtype).c_str());
		out.push_back(String(text));
	}
	reader.reset();
	source->Shutdown();
	return out;
}

void MediaFoundationFeed::_capture_loop() {
	ComScope com;
	while (!stop_requested) {
		ComPtr<IMFMediaSource> source;
		HRESULT hr = create_source(link, source);
		if (FAILED(hr)) {
			status.set("Not available (unplugged, or in use by another program) " + hresult_text(hr));
			std::this_thread::sleep_for(1s);
			continue;
		}
		{
			std::lock_guard<std::mutex> lock(source_mutex);
			active_source = source.get();
		}

		ComPtr<IMFSourceReader> reader;
		OutputFormat format;
		hr = create_reader(source.get(), reader);
		if (SUCCEEDED(hr)) {
			reader->SetStreamSelection(DWORD(MF_SOURCE_READER_ALL_STREAMS), FALSE);
			reader->SetStreamSelection(VIDEO_STREAM, TRUE);

			std::vector<Mode> modes = native_modes(reader.get());
			const Mode *mode = pick_mode(modes, requested_width, requested_height, requested_fps);
			if (mode) {
				ComPtr<IMFMediaType> native;
				if (SUCCEEDED(reader->GetNativeMediaType(VIDEO_STREAM, mode->index, native.put()))) {
					reader->SetCurrentMediaType(VIDEO_STREAM, nullptr, native.get());
				}
			}

			// Packed 4:2:2 goes to the GPU as-is (half the bytes of RGB). UYVY
			// sources are reordered while copying; everything else is converted
			// to YUY2 by Media Foundation, with RGB32 as the last resort.
			const GUID native_subtype = mode ? mode->subtype : GUID_NULL;
			if (native_subtype == MFVideoFormat_UYVY && SUCCEEDED(set_output(reader.get(), MFVideoFormat_UYVY))) {
				format.uyvy = true;
				format.name = "UYVY";
			} else if (SUCCEEDED(set_output(reader.get(), MFVideoFormat_YUY2))) {
				format.name = "YUY2";
			} else if (SUCCEEDED(hr = set_output(reader.get(), MFVideoFormat_RGB32))) {
				format.layout = FramePump::Layout::RGBA;
				format.bgrx = true;
				format.name = "RGB32";
			}
			if (format.name.empty()) {
				status.set("No usable video format " + hresult_text(hr));
				hr = E_FAIL;
			} else {
				hr = read_output(reader.get(), format);
			}
			if (SUCCEEDED(hr) && mode) {
				char text[128];
				std::snprintf(text, sizeof(text), "%ux%u @ %.0f fps (%s%s%s)", format.width, format.height, mode->fps,
						fourcc(mode->subtype).c_str(), format.name == fourcc(mode->subtype) ? "" : " > ",
						format.name == fourcc(mode->subtype) ? "" : format.name.c_str());
				status.set(text);
			}
		} else {
			status.set("Could not open device " + hresult_text(hr));
		}

		while (SUCCEEDED(hr) && !stop_requested) {
			DWORD stream_index = 0;
			DWORD flags = 0;
			LONGLONG timestamp = 0;
			ComPtr<IMFSample> sample;
			hr = reader->ReadSample(VIDEO_STREAM, 0, &stream_index, &flags, &timestamp, sample.put());
			if (FAILED(hr) || (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_ENDOFSTREAM))) {
				if (!stop_requested) {
					status.set("Device disconnected " + hresult_text(hr));
				}
				hr = E_FAIL;
				break;
			}
			if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) {
				read_output(reader.get(), format);
			}
			if (!sample) {
				continue; // stream tick / gap
			}
			ComPtr<IMFMediaBuffer> buffer;
			if (FAILED(sample->GetBufferByIndex(0, buffer.put()))) {
				continue;
			}
			ComPtr<IMF2DBuffer> buffer_2d;
			if (SUCCEEDED(buffer.as(buffer_2d))) {
				BYTE *scanline0 = nullptr;
				LONG pitch = 0;
				if (SUCCEEDED(buffer_2d->Lock2D(&scanline0, &pitch))) {
					pump.store(scanline0, ptrdiff_t(pitch), format.width, format.height, format.layout, format.uyvy, format.bgrx);
					buffer_2d->Unlock2D();
				}
			} else {
				BYTE *data = nullptr;
				DWORD max_length = 0;
				DWORD length = 0;
				if (SUCCEEDED(buffer->Lock(&data, &max_length, &length))) {
					const LONG stride = format.default_stride;
					const size_t needed = size_t(std::abs(stride)) * format.height;
					if (stride != 0 && length >= needed) {
						// Negative stride = bottom-up image: the top row is last in memory.
						const BYTE *top = stride > 0 ? data : data + size_t(-stride) * (format.height - 1);
						pump.store(top, ptrdiff_t(stride), format.width, format.height, format.layout, format.uyvy, format.bgrx);
					}
					buffer->Unlock();
				}
			}
		}

		{
			std::lock_guard<std::mutex> lock(source_mutex);
			active_source = nullptr;
		}
		reader.reset();
		source->Shutdown();
		if (!stop_requested) {
			std::this_thread::sleep_for(1s); // then try to reopen
		}
	}
}

} // namespace FrameDirector
