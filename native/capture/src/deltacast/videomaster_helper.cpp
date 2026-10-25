/*
 * Adapted from DELTACAST's videomaster-video-monitor sample (src/helper.cpp),
 * https://github.com/deltacasttv/videomaster-video-monitor
 * SPDX-FileCopyrightText: Copyright (c) DELTACAST.TV. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "videomaster_helper.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>
#include <VideoMasterCppApi/to_string.hpp>
#include <VideoMasterCppApi/exception.hpp>
#include <VideoMasterCppApi/helper/sdi.hpp>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

using namespace Deltacast::Wrapper;
using namespace Deltacast::Wrapper::Helper;

namespace FrameDirector::VideoMaster
{
    static std::optional<std::reference_wrapper<BoardComponents::Loopback>> get_loopback(Board& board, unsigned int channel_index)
    {
        try { return board.firmware_loopback(channel_index); } catch (const UnavailableResource&) { }
        try { return board.active_loopback(channel_index); } catch (const UnavailableResource&) { }
        try { return board.passive_loopback(channel_index); } catch (const UnavailableResource&) { }
        return std::nullopt;
    }

    void enable_loopback(Board& board, unsigned int channel_index)
    {
        auto optional_loopback = get_loopback(board, channel_index);
        if (optional_loopback)
            optional_loopback.value().get().enable();
    }

    void disable_loopback(Board& board, unsigned int channel_index)
    {
        auto optional_loopback = get_loopback(board, channel_index);
        if (optional_loopback)
            optional_loopback.value().get().disable();
    }

    VHD_STREAMTYPE rx_index_to_streamtype(unsigned int rx_index)
    {
        switch (rx_index)
        {
        case 0: return VHD_ST_RX0;
        case 1: return VHD_ST_RX1;
        case 2: return VHD_ST_RX2;
        case 3: return VHD_ST_RX3;
        case 4: return VHD_ST_RX4;
        case 5: return VHD_ST_RX5;
        case 6: return VHD_ST_RX6;
        case 7: return VHD_ST_RX7;
        case 8: return VHD_ST_RX8;
        case 9: return VHD_ST_RX9;
        case 10: return VHD_ST_RX10;
        case 11: return VHD_ST_RX11;
        default:
            throw std::invalid_argument("Invalid RX index");
        }
    }

    bool is_sdi_channel(VHD_CHANNELTYPE channel_type)
    {
        return channel_type == VHD_CHNTYPE_HDSDI
            || channel_type == VHD_CHNTYPE_3GSDI
            || channel_type == VHD_CHNTYPE_12GSDI;
    }

    bool is_supported_channel(VHD_CHANNELTYPE channel_type)
    {
        switch (channel_type)
        {
            case VHD_CHNTYPE_HDSDI:
            case VHD_CHNTYPE_3GSDI:
            case VHD_CHNTYPE_12GSDI:
            case VHD_CHNTYPE_HDMI_TMDS:
            case VHD_CHNTYPE_HDMI_FRL3:
            case VHD_CHNTYPE_HDMI_FRL4:
            case VHD_CHNTYPE_HDMI_FRL5:
            case VHD_CHNTYPE_HDMI_FRL6:
            case VHD_CHNTYPE_DISPLAYPORT:
                return true;
            default:
                return false;
        }
    }

    TechStream open_stream(Board& board, unsigned int rx_index)
    {
        auto stream_type = rx_index_to_streamtype(rx_index);
        auto channel_type = board.rx(rx_index).type();
        if (is_sdi_channel(channel_type))
            return std::move(board.sdi().open_stream(stream_type, VHD_SDI_STPROC_DISJOINED_VIDEO));
        if (is_supported_channel(channel_type))
            return std::move(board.dv().open_stream(stream_type, VHD_DV_STPROC_DISJOINED_VIDEO));
        throw std::invalid_argument("Invalid channel type");
    }

    Stream& to_base_stream(TechStream& stream)
    {
        return std::visit(overloaded{
            [](SdiStream& sdi_stream) -> Stream& { return sdi_stream; },
            [](DvStream& dv_stream) -> Stream& { return dv_stream; }
        }, stream);
    }

    void configure_stream(TechStream& stream, const SignalInformation& signal_information)
    {
        std::visit(overloaded{
            [&signal_information](SdiStream& sdi_stream)
            {
                const SdiSignalInformation& sdi_signal_information = std::get<SdiSignalInformation>(signal_information);
                sdi_stream.set_video_standard(sdi_signal_information.video_standard);
                sdi_stream.set_interface(sdi_signal_information.video_interface);
            },
            [&signal_information](DvStream& dv_stream)
            {
                const DvSignalInformation& dv_signal_information = std::get<DvSignalInformation>(signal_information);
                dv_stream.set_active_width(dv_signal_information.width);
                dv_stream.set_active_height(dv_signal_information.height);
                dv_signal_information.progressive ? dv_stream.set_progressive() : dv_stream.set_interlaced();
                dv_stream.set_frame_rate(dv_signal_information.framerate);
                dv_stream.set_cable_color_space(dv_signal_information.cable_color_space);
            }
        }, stream);
    }

    std::string describe(const SignalInformation& signal_information)
    {
        std::ostringstream out;
        std::visit(overloaded{
            [&out](const SdiSignalInformation& info)
            {
                out << to_pretty_string(info.video_standard) << ", " << to_pretty_string(info.clock_divisor);
            },
            [&out](const DvSignalInformation& info)
            {
                out << info.width << "x" << info.height << (info.progressive ? "p" : "i") << info.framerate;
            }
        }, signal_information);
        return out.str();
    }

    SignalInformation detect_information(TechStream& stream)
    {
        return std::visit(overloaded{
            [](SdiStream& sdi_stream) -> SignalInformation
            {
                return SdiSignalInformation{sdi_stream.video_standard(), sdi_stream.clock_divisor(), sdi_stream.interface()};
            },
            [](DvStream& dv_stream) -> SignalInformation
            {
                return DvSignalInformation{dv_stream.active_width(), dv_stream.active_height(), !dv_stream.interlaced(), dv_stream.frame_rate()
                                            , dv_stream.cable_color_space(), dv_stream.cable_sampling()};
            }
        }, stream);
    }

    VideoCharacteristics get_video_characteristics(const SignalInformation& signal_information)
    {
        return std::visit(overloaded{
            [](const SdiSignalInformation& sdi_signal_info) -> VideoCharacteristics
            {
                return Sdi::video_standard_to_characteristics(sdi_signal_info.video_standard);
            },
            [](const DvSignalInformation& dv_signal_info) -> VideoCharacteristics
            {
                return { dv_signal_info.width, dv_signal_info.height, !dv_signal_info.progressive, dv_signal_info.framerate };
            }
        }, signal_information);
    }
}
