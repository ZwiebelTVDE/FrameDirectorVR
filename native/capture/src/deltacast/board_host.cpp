#include "board_host.h"

using namespace Deltacast::Wrapper;

namespace FrameDirector {

BoardHost::BoardHost(unsigned int p_index) :
		board_index(p_index) {
	// Same as the DELTACAST sample: the callback restores the loopback (input
	// passed straight through to the output) when the board is closed.
	// Constructed from the prvalue directly, so Board doesn't need to be movable.
	board.reset(new Board(Board::open(p_index, [](Board &b) {
		for (unsigned int i = 0; i < b.number_of_rx(); i++) {
			VideoMaster::enable_loopback(b, i);
		}
	})));
	for (unsigned int i = 0; i < board->number_of_rx(); i++) {
		VideoMaster::disable_loopback(*board, i);
	}
}

BoardHost::~BoardHost() = default;

std::string BoardHost::name() {
	std::lock_guard<std::mutex> lock(mutex);
	return board->name();
}

unsigned int BoardHost::number_of_rx() {
	std::lock_guard<std::mutex> lock(mutex);
	return board->number_of_rx();
}

bool BoardHost::rx_supported(unsigned int p_rx_index) {
	std::lock_guard<std::mutex> lock(mutex);
	return VideoMaster::is_supported_channel(board->rx(p_rx_index).type());
}

bool BoardHost::rx_is_sdi(unsigned int p_rx_index) {
	std::lock_guard<std::mutex> lock(mutex);
	return VideoMaster::is_sdi_channel(board->rx(p_rx_index).type());
}

bool BoardHost::signal_present(unsigned int p_rx_index) {
	std::lock_guard<std::mutex> lock(mutex);
	return board->rx(p_rx_index).signal_present();
}

VideoMaster::TechStream BoardHost::open_stream(unsigned int p_rx_index) {
	std::lock_guard<std::mutex> lock(mutex);
	return VideoMaster::open_stream(*board, p_rx_index);
}

} // namespace FrameDirector
