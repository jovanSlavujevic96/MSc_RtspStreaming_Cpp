#pragma once

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class AACSource : public MediaSource
{
public:
    static AACSource* CreateNew(uint32_t samplerate=44100, uint32_t channels=2, bool has_adts=true);
    ~AACSource() = default;

    uint32_t GetSamplerate() const;
    uint32_t GetChannels() const;

    virtual std::string GetMediaDescription(uint16_t port=0);
    virtual std::string GetAttribute();

    bool HandleFrame(MediaChannelId channel_id, AVFrame& frame);
    static constexpr uint32_t GetTimestamp(uint32_t samplerate = 44100);

private:
    AACSource(uint32_t samplerate, uint32_t channels, bool has_adts);

    uint32_t samplerate_ = 44100;  
    uint32_t channels_ = 2;         
    bool has_adts_ = true;

    static constexpr int ADTS_SIZE = 7;
    static constexpr int AU_SIZE   = 4;
};

}
