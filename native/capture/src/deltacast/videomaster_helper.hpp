/*
 * Adapted from DELTACAST's videomaster-video-monitor sample (src/helper.hpp),
 * https://github.com/deltacasttv/videomaster-video-monitor
 * SPDX-FileCopyrightText: Copyright (c) DELTACAST.TV. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Only the parts needed for capture are kept. Logic is unchanged.
 */
#pragma once

#include <atomic>
#include <variant>

#include <VideoMasterCppApi/helper/video.hpp>
#include <VideoMasterCppApi/board/board.hpp>
#include <VideoMasterCppApi/stream/sdi/sdi_stream.hpp>
#include <VideoMasterCppApi/stream/dv/dv_stream.hpp>

namespace FrameDirector::VideoMaster
{
    void enable_loopback(Deltacast::Wrapper::Board& board, unsigned int channel_index);
    void disable_loopback(Deltacast::Wrapper::Board& board, unsigned int channel_index);

    VHD_STREAMTYPE rx_index_to_streamtype(unsigned int rx_index);
    bool is_supported_channel(VHD_CHANNELTYPE channel_type);
    bool is_sdi_channel(VHD_CHANNELTYPE channel_type);

    using TechStream = std::variant<Deltacast::Wrapper::SdiStream, Deltacast::Wrapper::DvStream>;
    TechStream open_stream(Deltacast::Wrapper::Board& board, unsigned int rx_index);
    Deltacast::Wrapper::Stream& to_base_stream(TechStream& stream);

    struct SdiSignalInformation
    {
        VHD_VIDEOSTANDARD video_standard;
        VHD_CLOCKDIVISOR clock_divisor;
        VHD_INTERFACE video_interface;

        bool operator==(const SdiSignalInformation& other) const
        {
            return video_standard == other.video_standard
                    && clock_divisor == other.clock_divisor
                    && video_interface == other.video_interface;
        }
        bool operator!=(const SdiSignalInformation& other) const { return !(*this == other); }
    };
    struct DvSignalInformation
    {
        unsigned int width;
        unsigned int height;
        bool progressive;
        unsigned int framerate;
        VHD_DV_CS cable_color_space;
        VHD_DV_SAMPLING cable_sampling;

        bool operator==(const DvSignalInformation& other) const
        {
            return width == other.width
                    && height == other.height
                    && framerate == other.framerate
                    && progressive == other.progressive
                    && cable_color_space == other.cable_color_space
                    && cable_sampling == other.cable_sampling;
        }
        bool operator!=(const DvSignalInformation& other) const { return !(*this == other); }
    };
    using SignalInformation = std::variant<SdiSignalInformation, DvSignalInformation>;

    void configure_stream(TechStream& stream, const SignalInformation& signal_information);
    SignalInformation detect_information(TechStream& stream);
    std::string describe(const SignalInformation& signal_information);

    Deltacast::Wrapper::Helper::VideoCharacteristics get_video_characteristics(const SignalInformation& signal_information);
}
