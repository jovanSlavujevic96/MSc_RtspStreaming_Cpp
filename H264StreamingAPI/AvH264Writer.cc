#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "AvH264Writer.h"

AvH264Writer::AvH264Writer(const char* path, uint16_t camera_id, uint16_t starting_counter, uint16_t max_number_of_videos) :
    mCameraId{ camera_id },
    mVideoCounter{ starting_counter },
    mMaxVideos{ max_number_of_videos }
{
    mPathToFile = (path != NULL) ? path : "";
    mVideoCounter = starting_counter % max_number_of_videos;
    mCurrentFileName = "";
    mRecordedFileName = "";
    mEncConfig = NULL;
    mOutContext = NULL;
    mAvStream = NULL;
    mAvFrame = NULL;
    mFramePts = 0;
    mStreamingNotifier = nullptr;
    mNetworkManagerNotififer = NULL;
}

AvH264Writer::AvH264Writer(const AvH264Writer& writer)
{
    AvH264Writer* writer_ = const_cast<AvH264Writer*>(&writer);
    mCameraId = writer_->mCameraId;
    mMaxVideos = writer_->mMaxVideos;
    mVideoCounter = writer_->mVideoCounter;
    mPathToFile = std::move(writer_->mPathToFile);
    mVideoCounter = writer_->mVideoCounter;
    mCurrentFileName = std::move(writer_->mCurrentFileName);
    mRecordedFileName = std::move(writer_->mRecordedFileName);
    mEncConfig = writer_->mEncConfig;
    mOutContext = writer_->mOutContext;
    mAvStream = writer_->mAvStream;
    mAvFrame = writer_->mAvFrame;
    mFramePts = writer_->mFramePts;
    mStreamingNotifier = writer_->mStreamingNotifier;
    mNetworkManagerNotififer = writer_->mNetworkManagerNotififer;

    writer_->mOutContext = NULL;
    writer_->mAvStream = NULL;
    writer_->mAvFrame = NULL;
    writer_->mStreamingNotifier = nullptr;
    writer_->mNetworkManagerNotififer = NULL;
}

AvH264Writer::~AvH264Writer()
{
    AvH264Writer::closeWriter();
}

const std::string& AvH264Writer::getCurrentFileName() const
{
    return mCurrentFileName;
}

const std::string& AvH264Writer::getRecordedFileName() const
{
    return mRecordedFileName;
}

void AvH264Writer::preUpdateFileNames()
{
    if (mPathToFile != "")
    {
        mCurrentFileName = mPathToFile + "/";
    }
    if (mVideoCounter && mRecordedFileName == "")
    {
        mRecordedFileName = mCurrentFileName;
        mRecordedFileName += "recorded_camera_" + std::to_string(mCameraId) + "_" + std::to_string(mVideoCounter - 1) + "." + cFileNameSuffix;
        if (mNetworkManagerNotififer)
        {
            mNetworkManagerNotififer->notify_all();
        }
        if (mStreamingNotifier)
        {
            mStreamingNotifier->notify_one();
        }
    }
    mCurrentFileName += "recorded_camera_" + std::to_string(mCameraId) + "_" + std::to_string(mVideoCounter) + "." + cFileNameSuffix;
    mVideoCounter = (mVideoCounter + 1) % mMaxVideos;
}

void AvH264Writer::postUpdateFileNames()
{
    mRecordedFileName = mCurrentFileName;
    if (mNetworkManagerNotififer)
    {
        mNetworkManagerNotififer->notify_all();
    }
    if (mStreamingNotifier)
    {
        mStreamingNotifier->notify_one();
    }
    mCurrentFileName = "";
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (disable : 4996)
#endif

void AvH264Writer::initWriter() noexcept(false)
{
    if (!mEncConfig)
    {
        throw std::runtime_error("AvH264Writer::initWriter : mEncConfig is NULL. You must bindEncodingConfiguration.");
    }
    int ret = 0;
    const int dst_width = mEncConfig->width;
    const int dst_height = mEncConfig->height;
    const AVRational dst_fps = { (int)mEncConfig->bit_rate, 1 };

    AvH264Writer::preUpdateFileNames();

    // open output format context
    ret = avformat_alloc_output_context2(&mOutContext, NULL, cFileNameSuffix, NULL);
    if (ret < 0) 
    {
        std::stringstream ss;
        ss << "AvH264Writer::initWriter : fail to avformat_alloc_output_context2(" << mCurrentFileName << "): ret=" << ret;
        throw std::runtime_error(ss.str());
    }

    // open output IO context
    ret = avio_open2(&mOutContext->pb, mCurrentFileName.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
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
    if (!mAvFrame || !mOutContext || !mAvStream)
    {
        return;
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

    av_frame_free(&mAvFrame);
    avcodec_close(mAvStream->codec);
    avio_close(mOutContext->pb);
    avformat_free_context(mOutContext);

    AvH264Writer::postUpdateFileNames();

    mAvFrame = NULL;
    mOutContext = NULL;
    mAvStream = NULL;
}

void AvH264Writer::writeFrame(AVPacket* packet)
{
    // write packet
    av_write_frame(mOutContext, packet);
}

void AvH264Writer::bindEncodingConfiguration(AvH264EncConfig* configuration)
{
    mEncConfig = configuration;
}

void AvH264Writer::bindNetworkManagerNotifier(std::condition_variable* condition)
{
    mNetworkManagerNotififer = condition;
}

void AvH264Writer::bindStreamingNotifier(std::shared_ptr<std::condition_variable> condition)
{
    mStreamingNotifier = condition;
}

#if defined(WIN32) || defined(_WIN32)
#pragma warning (default : 4996)
#endif
