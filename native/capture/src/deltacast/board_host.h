#pragma once

#include "videomaster_helper.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace FrameDirector {

/// Owns one opened DELTACAST board. Board-level calls are serialized because
/// several inputs on the same board capture from different threads.
class BoardHost {
public:
	explicit BoardHost(unsigned int p_index);
	~BoardHost();

	unsigned int index() const { return board_index; }
	std::string name();
	unsigned int number_of_rx();
	bool rx_supported(unsigned int p_rx_index);
	bool rx_is_sdi(unsigned int p_rx_index);
	bool signal_present(unsigned int p_rx_index);
	VideoMaster::TechStream open_stream(unsigned int p_rx_index);

private:
	unsigned int board_index;
	std::mutex mutex;
	std::unique_ptr<Deltacast::Wrapper::Board> board;
};

} // namespace FrameDirector
