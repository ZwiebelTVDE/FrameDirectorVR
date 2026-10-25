// Stand-in for the parts of DELTACAST's VideoMaster C++ API used by the plugin.
// Only for building and testing without the SDK (-DDELTACAST_MOCK=ON). It
// simulates a 4-input 3G-SDI board: In 1 = 1080p60, In 2 = 720p50,
// In 3 = signal comes and goes every 4 s, In 4 = no signal.
// Real builds use the headers from the VideoMaster SDK instead.
#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

typedef unsigned char BYTE;
typedef unsigned long ULONG;

enum VHD_STREAMTYPE { VHD_ST_RX0, VHD_ST_RX1, VHD_ST_RX2, VHD_ST_RX3, VHD_ST_RX4, VHD_ST_RX5, VHD_ST_RX6, VHD_ST_RX7, VHD_ST_RX8, VHD_ST_RX9, VHD_ST_RX10, VHD_ST_RX11 };
enum VHD_CHANNELTYPE { VHD_CHNTYPE_DISABLE, VHD_CHNTYPE_HDSDI, VHD_CHNTYPE_3GSDI, VHD_CHNTYPE_12GSDI, VHD_CHNTYPE_HDMI_TMDS, VHD_CHNTYPE_HDMI_FRL3, VHD_CHNTYPE_HDMI_FRL4, VHD_CHNTYPE_HDMI_FRL5, VHD_CHNTYPE_HDMI_FRL6, VHD_CHNTYPE_DISPLAYPORT };
enum VHD_VIDEOSTANDARD { VHD_VIDEOSTD_S274M_1080p_60Hz, VHD_VIDEOSTD_S296M_720p_50Hz };
enum VHD_CLOCKDIVISOR { VHD_CLOCKDIV_1, VHD_CLOCKDIV_1001 };
enum VHD_INTERFACE { VHD_INTERFACE_3G_A_425_1, VHD_INTERFACE_HD_292_1 };
enum VHD_DV_CS { VHD_DV_CS_YUV422 };
enum VHD_DV_SAMPLING { VHD_DV_SAMPLING_4_2_2_8BITS };
enum VHD_BUFFERPACKING { VHD_BUFPACK_VIDEO_YUV422_8 };
enum VHD_SDI_STREAMPROCMODE { VHD_SDI_STPROC_DISJOINED_VIDEO };
enum VHD_DV_STREAMPROCMODE { VHD_DV_STPROC_DISJOINED_VIDEO };

namespace Deltacast::Wrapper {

class ApiException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
	std::string logs() const { return ""; }
};
class UnavailableResource : public ApiException {
public:
	using ApiException::ApiException;
};

inline std::string api_version() { return "mock 6.30"; }
inline std::string to_pretty_string(VHD_VIDEOSTANDARD s) { return s == VHD_VIDEOSTD_S274M_1080p_60Hz ? "1920x1080p60" : "1280x720p50"; }
inline std::string to_pretty_string(VHD_CLOCKDIVISOR d) { return d == VHD_CLOCKDIV_1 ? "1" : "1.001"; }

namespace Helper {
struct VideoCharacteristics {
	unsigned int width;
	unsigned int height;
	bool interlaced;
	unsigned int framerate;
};
namespace Sdi {
inline VideoCharacteristics video_standard_to_characteristics(VHD_VIDEOSTANDARD s) {
	return s == VHD_VIDEOSTD_S274M_1080p_60Hz ? VideoCharacteristics{ 1920, 1080, false, 60 } : VideoCharacteristics{ 1280, 720, false, 50 };
}
} // namespace Sdi
} // namespace Helper

namespace mock {
inline double seconds() {
	using namespace std::chrono;
	static const auto start = steady_clock::now();
	return duration<double>(steady_clock::now() - start).count();
}
inline bool signal(unsigned int rx) {
	if (rx == 0 || rx == 1) return true;
	if (rx == 2) return int(seconds() / 4.0) % 2 == 0;
	return false;
}
} // namespace mock

class VideoSlotPart {
public:
	std::vector<BYTE> *data;
	std::pair<BYTE *, ULONG> buffer() { return { data->data(), ULONG(data->size()) }; }
};
class Slot {
public:
	std::vector<BYTE> data;
	VideoSlotPart part{ &data };
	VideoSlotPart &video() { return part; }
};

class BufferQueue {
public:
	void set_depth(int) {}
	int slots_count() const { return 0; }
	int slots_dropped() const { return 0; }
};

class Stream {
public:
	Stream(unsigned int rx, unsigned int w, unsigned int h, int fps) :
			rx(rx), w(w), h(h), fps(fps) {}
	Stream(Stream &&) = default;
	virtual ~Stream() = default;
	BufferQueue &buffer_queue() { return queue; }
	void set_buffer_packing(VHD_BUFFERPACKING) {}
	void start() { started = true; }
	std::unique_ptr<Slot> pop_slot() {
		if (!started) throw ApiException("stream not started");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
		if (!mock::signal(rx)) throw ApiException("timeout");
		auto slot = std::make_unique<Slot>();
		slot->data.resize(size_t(w) * h * 2);
		fill(slot->data.data(), frame++);
		return slot;
	}

protected:
	// UYVY colour bars with a moving white bar, plus a per-input tint so inputs
	// can be told apart.
	void fill(BYTE *p, int n) {
		static const BYTE bars[7][3] = { { 180, 128, 128 }, { 168, 44, 136 }, { 145, 147, 44 }, { 133, 63, 52 }, { 63, 193, 204 }, { 51, 109, 212 }, { 28, 212, 120 } };
		unsigned int sweep = (unsigned int)(n * 8) % w;
		for (unsigned int y = 0; y < h; y++) {
			for (unsigned int x = 0; x < w; x += 2) {
				const BYTE *c = bars[(x * 7) / w];
				BYTE Y = c[0], cb = c[1], cr = c[2];
				if (y > h * 3 / 4) { Y = BYTE(40 + rx * 40); cb = 128; cr = 128; }
				if (x / 16 == sweep / 16) { Y = 235; cb = 128; cr = 128; }
				BYTE *q = p + (size_t(y) * w + x) * 2;
				q[0] = cb; q[1] = Y; q[2] = cr; q[3] = Y;
			}
		}
	}
	unsigned int rx, w, h;
	int fps;
	int frame = 0;
	bool started = false;
	BufferQueue queue;
};

class SdiStream : public Stream {
public:
	explicit SdiStream(unsigned int rx) :
			Stream(rx, rx == 1 ? 1280 : 1920, rx == 1 ? 720 : 1080, rx == 1 ? 50 : 60) {}
	VHD_VIDEOSTANDARD video_standard() { return rx == 1 ? VHD_VIDEOSTD_S296M_720p_50Hz : VHD_VIDEOSTD_S274M_1080p_60Hz; }
	VHD_CLOCKDIVISOR clock_divisor() { return VHD_CLOCKDIV_1; }
	VHD_INTERFACE interface() { return rx == 1 ? VHD_INTERFACE_HD_292_1 : VHD_INTERFACE_3G_A_425_1; }
	void set_video_standard(VHD_VIDEOSTANDARD) {}
	void set_interface(VHD_INTERFACE) {}
};

class DvStream : public Stream {
public:
	explicit DvStream(unsigned int rx) :
			Stream(rx, 1920, 1080, 60) {}
	unsigned int active_width() { return 1920; }
	unsigned int active_height() { return 1080; }
	bool interlaced() { return false; }
	unsigned int frame_rate() { return 60; }
	VHD_DV_CS cable_color_space() { return VHD_DV_CS_YUV422; }
	VHD_DV_SAMPLING cable_sampling() { return VHD_DV_SAMPLING_4_2_2_8BITS; }
	void set_active_width(unsigned int) {}
	void set_active_height(unsigned int) {}
	void set_progressive() {}
	void set_interlaced() {}
	void set_frame_rate(unsigned int) {}
	void set_cable_color_space(VHD_DV_CS) {}
};

namespace BoardComponents {
class RxConnector {
public:
	unsigned int index;
	VHD_CHANNELTYPE type() const { return VHD_CHNTYPE_3GSDI; }
	bool signal_present() const { return mock::signal(index); }
};
class Loopback {
public:
	void enable() {}
	void disable() {}
};
} // namespace BoardComponents

class Board;
class SdiFactory {
public:
	SdiStream open_stream(VHD_STREAMTYPE t, VHD_SDI_STREAMPROCMODE) { return SdiStream(unsigned(t)); }
};
class DvFactory {
public:
	DvStream open_stream(VHD_STREAMTYPE t, VHD_DV_STREAMPROCMODE) { return DvStream(unsigned(t)); }
};

class Board {
public:
	static unsigned int count() { return 1; }
	static Board open(unsigned int index, std::function<void(Board &)> on_close) { return Board(index, std::move(on_close)); }
	Board(const Board &) = delete;
	~Board() {
		if (on_close) on_close(*this);
	}
	std::string name() const { return "DELTA-3G-elp-d 40 (mock)"; }
	unsigned int index() const { return idx; }
	unsigned int number_of_rx() const { return 4; }
	unsigned int number_of_tx() const { return 0; }
	BoardComponents::RxConnector &rx(unsigned int i) {
		if (i >= 4) throw std::invalid_argument("rx");
		return rxs[i];
	}
	BoardComponents::Loopback &firmware_loopback(unsigned int) { throw UnavailableResource("no loopback"); }
	BoardComponents::Loopback &active_loopback(unsigned int) { throw UnavailableResource("no loopback"); }
	BoardComponents::Loopback &passive_loopback(unsigned int) { throw UnavailableResource("no loopback"); }
	SdiFactory &sdi() { return sdi_factory; }
	DvFactory &dv() { return dv_factory; }

private:
	// Non-movable on purpose, to prove the plugin doesn't need Board to move.
	Board(unsigned int index, std::function<void(Board &)> cb) :
			idx(index), on_close(std::move(cb)) {
		for (unsigned int i = 0; i < 4; i++) rxs[i].index = i;
	}
	unsigned int idx;
	std::function<void(Board &)> on_close;
	BoardComponents::RxConnector rxs[4];
	SdiFactory sdi_factory;
	DvFactory dv_factory;
};

} // namespace Deltacast::Wrapper
