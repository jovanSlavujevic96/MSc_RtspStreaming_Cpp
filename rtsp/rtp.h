#pragma once

#include <memory>
#include <cstdint>

#define RTP_HEADER_SIZE   	   12
#define MAX_RTP_PAYLOAD_SIZE   1460 // 1460 <=> 1500-20-12-8
#define RTP_VERSION			   2
#define RTP_TCP_HEAD_SIZE	   4

namespace rtsp
{

enum TransportMode
{
	RTP_OVER_TCP = 1,
	RTP_OVER_UDP = 2,
	RTP_OVER_MULTICAST = 3,
};

struct RtpHeader
{
	uint8_t csrc:4;
	uint8_t extension:1;
	uint8_t padding:1;
	uint8_t version:2;
	uint8_t payload:7;
	uint8_t marker:1;

	uint16_t seq;
	uint32_t ts;
	uint32_t ssrc;
};

struct MediaChannelInfo
{
	RtpHeader rtp_header;

	// tcp
	uint16_t rtp_channel;
	uint16_t rtcp_channel;

	// udp
	uint16_t rtp_port;
	uint16_t rtcp_port;
	uint16_t packet_seq;
	uint32_t clock_rate;

	// rtcp
	uint64_t packet_count;
	uint64_t octet_count;
	uint64_t last_rtcp_ntp_time;

	bool is_setup;
	bool is_play;
	bool is_record;
};

struct RtpPacket
{
	std::unique_ptr<uint8_t[]> data;
	uint32_t size;
	uint32_t timestamp;
	uint8_t  type;
	uint8_t  last;

	RtpPacket() :
		data{ std::make_unique<uint8_t[]>(1600) }
	{
		size = 0;
		timestamp = 0;
		type = 0;
		last = 0;
	}
};

}
