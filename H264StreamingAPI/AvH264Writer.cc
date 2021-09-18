#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "AvH264Writer.h"

AvH264Writer::AvH264Writer(const char* file_name) :
    mFileName{ file_name }
{
    mOutContext = NULL;
    //mSwsContext = NULL;
    mAvStream = NULL;
    mAvFrame = NULL;
    mFramePts = 0;
}

AvH264Writer::~AvH264Writer()
{
    AvH264Writer::closeWriter();
}

constexpr const std::string& AvH264Writer::getFileName() const
{
    return mFileName;
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (disable : 4996)
#endif

void AvH264Writer::initWriter(const AvH264EncConfig& h264_config) noexcept(false)
{
    int ret = 0;
    const int dst_width = h264_config.width;
    const int dst_height = h264_config.height;
    const AVRational dst_fps = { (int)h264_config.bit_rate, 1 };

    // open output format context
    ret = avformat_alloc_output_context2(&mOutContext, NULL, NULL, mFileName.c_str());
    if (ret < 0) 
    {
        std::stringstream ss;
        ss << "AvH264Writer::initWriter : fail to avformat_alloc_output_context2(" << mFileName << "): ret=" << ret;
        throw std::runtime_error(ss.str());
    }

    // open output IO context
    ret = avio_open2(&mOutContext->pb, mFileName.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0)
    {
        std::stringstream ss;
        ss << "AvH264Writer::initWriter : fail to avio_open2: ret=" << ret;
        throw std::runtime_error(ss.str());
    }

    // create new video stream
    AVCodec* vcodec = avcodec_find_encoder(mOutContext->oformat->video_codec);
    mAvStream = avformat_new_stream(mOutContext, vcodec);
    if (!mAvStream)
    {
        std::stringstream ss;
        ss << "AvH264Writer::initWriter : fail to avformat_new_stream";
        throw std::runtime_error(ss.str());
    }
    avcodec_get_context_defaults3(mAvStream->codec, vcodec);
    mAvStream->codec->width = dst_width;
    mAvStream->codec->height = dst_height;
    mAvStream->codec->pix_fmt = vcodec->pix_fmts[0];
    mAvStream->codec->time_base = mAvStream->time_base = av_inv_q(dst_fps);
    mAvStream->r_frame_rate = mAvStream->avg_frame_rate = dst_fps;
    if (mOutContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
        mAvStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // open video encoder
    ret = avcodec_open2(mAvStream->codec, vcodec, nullptr);
    if (ret < 0)
    {
        std::stringstream ss;
        ss << "AvH264Writer::initWriter : fail to avcodec_open2: ret=" << ret;
        throw std::runtime_error(ss.str());
    }

    // allocate frame buffer for encoding
    mAvFrame = av_frame_alloc();
    std::vector<uint8_t> framebuf(av_image_get_buffer_size(mAvStream->codec->pix_fmt, dst_width, dst_height, 0));
    av_image_fill_arrays(mAvFrame->data, mAvFrame->linesize, framebuf.data(), mAvStream->codec->pix_fmt, dst_width, dst_height, 0);
    mAvFrame->width = dst_width;
    mAvFrame->height = dst_height;
    mAvFrame->format = static_cast<int>(mAvStream->codec->pix_fmt);

    // encoding loop
    avformat_write_header(mOutContext, NULL);
}

void AvH264Writer::closeWriter() noexcept(false)
{
    int ret = 0;
    int got_pkt = 0;
    mFileName = "";

    AVPacket pkt;
    pkt.data = NULL;
    pkt.size = 0;
    av_init_packet(&pkt);

    if ((avcodec_send_frame(mAvStream->codec, NULL) >= 0) &&
        (avcodec_receive_packet(mAvStream->codec, &pkt) == 0))
    {
        // rescale packet timestamp
        pkt.duration = 0;
        av_packet_rescale_ts(&pkt, mAvStream->codec->time_base, mAvStream->time_base);
        // write packet
        av_write_frame(mOutContext, &pkt);
    }
    
    av_packet_unref(&pkt);
    av_write_trailer(mOutContext);

    av_frame_free(&mAvFrame);
    avcodec_close(mAvStream->codec);
    avio_close(mOutContext->pb);
    avformat_free_context(mOutContext);
}

void AvH264Writer::writeFrame(AVPacket* packet)
{
    // write packet
    av_write_frame(mOutContext, packet);
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (default : 4996)
#endif
