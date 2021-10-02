#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "AvH264Writer.h"

AvH264Writer::~AvH264Writer()
{
    AvH264Writer::close();
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (disable : 4996)
#endif

int AvH264Writer::open(const std::string& video) noexcept
{
    int ret = 0;
    const int dst_width = mEncConfig->width;
    const int dst_height = mEncConfig->height;
    const AVRational dst_fps = { mEncConfig->bit_rate, 1 };

    // open output format context
    ret = avformat_alloc_output_context2(&mOutContext, NULL, AvH264Writer::cFileNameSuffix, NULL);
    if (ret < 0)
    {
        return 1;
    }

    // open output IO context
    ret = avio_open2(&mOutContext->pb, video.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0)
    {
        return 2;
    }

    // create new video stream
    AVCodec* vcodec = avcodec_find_encoder(mOutContext->oformat->video_codec);
    mAvStream = avformat_new_stream(mOutContext, vcodec);
    if (!mAvStream)
    {
        return 3;
    }

    // get context
    avcodec_get_context_defaults3(mAvStream->codec, vcodec);
    mAvStream->codec->width = dst_width;
    mAvStream->codec->height = dst_height;
    mAvStream->codec->pix_fmt = vcodec->pix_fmts[0];
    mAvStream->codec->time_base = av_inv_q(dst_fps);
    mAvStream->time_base = av_inv_q(dst_fps);
    mAvStream->r_frame_rate = dst_fps;
    mAvStream->avg_frame_rate = dst_fps;
    if (mOutContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
        mAvStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // open video encoder
    ret = avcodec_open2(mAvStream->codec, vcodec, nullptr);
    if (ret < 0)
    {
        return 4;
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

    return 0;
}

void AvH264Writer::close() noexcept
{
    if (!mAvFrame || !mOutContext || !mAvStream)
    {
        goto free;
    }
    int ret = 0;
    int got_pkt = 0;

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

free:
    if (mAvFrame)
    {
        av_frame_free(&mAvFrame);
        mAvFrame = NULL;
    }
    if (mAvStream)
    {
        avcodec_close(mAvStream->codec);
        mAvStream = NULL;
    }
    if (mOutContext)
    {
        avio_close(mOutContext->pb);
        avformat_free_context(mOutContext);
        mOutContext = NULL;
    }
}

void AvH264Writer::write(AVPacket* packet)
{
    // write packet
    av_write_frame(mOutContext, packet);
}

void AvH264Writer::bind(std::shared_ptr<AvH264EncConfig> configuration)
{
    mEncConfig = std::move(configuration);
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (default : 4996)
#endif
