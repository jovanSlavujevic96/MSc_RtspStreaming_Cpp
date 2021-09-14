#include "AvH264Encoder.h"

AvH264Encoder::AvH264Encoder()
{
    mFrameSize = 0;
    mPts = 0;
    mCdc = NULL;
    mCdcCtxt = NULL;
    mAvFrame = NULL;
    mAvPacket = NULL;
}

AvH264Encoder::~AvH264Encoder()
{
    AvH264Encoder::close();
}

int AvH264Encoder::open(const AvH264EncConfig& h264_config)
{
    mCdc = ::avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!mCdc)
    {
        std::cout << "!cdc_\n";
        return -1;
    }
    mCdcCtxt = avcodec_alloc_context3(mCdc);
    if (!mCdcCtxt)
    {
        std::cout << "!cdc_ctx_\n";
        return -1;
    }
    mCdcCtxt->bit_rate = h264_config.bit_rate;
    mCdcCtxt->width = h264_config.width;
    mCdcCtxt->height = h264_config.height;
    mCdcCtxt->time_base = { 1, h264_config.frame_rate };
    mCdcCtxt->framerate = { h264_config.frame_rate, 1 };
    mCdcCtxt->gop_size = h264_config.gop_size;
    mCdcCtxt->max_b_frames = h264_config.max_b_frames;
    mCdcCtxt->pix_fmt = AV_PIX_FMT_YUV420P;
    mCdcCtxt->codec_id = AV_CODEC_ID_H264;
    mCdcCtxt->codec_type = AVMEDIA_TYPE_VIDEO;
    mCdcCtxt->qmin = 10; /*10*/
    mCdcCtxt->qmax = 20; /*51*/
    //mCdcCtxt->qcompress = 0.6f;

    AVDictionary* dict = 0;
    av_dict_set(&dict, "preset", "slow", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
    av_dict_set(&dict, "profile", "main", 0);
    av_dict_set(&dict, "rtsp_transport", "udp", 0);

    mFrameSize = mCdcCtxt->width * mCdcCtxt->height;

    mCvSize = cv::Size(mCdcCtxt->width, mCdcCtxt->height);
    return avcodec_open2(mCdcCtxt, mCdc, &dict);
}

int AvH264Encoder::allocNewAvFrame()
{
    mAvFrame = av_frame_alloc();
    if (!mAvFrame)
    {
        return -1;
    }
    mAvPacket = av_packet_alloc();
    if (!mAvPacket)
    {
        av_frame_free(&mAvFrame);
        mAvFrame = NULL;
        return -1;
    }

    mAvFrame->format = mCdcCtxt->pix_fmt;
    mAvFrame->width = mCdcCtxt->width;
    mAvFrame->height = mCdcCtxt->height;

    // alloc memory
    if ((av_frame_get_buffer(mAvFrame, 0) < 0) ||
        (av_frame_make_writable(mAvFrame) < 0))
    {
        av_frame_free(&mAvFrame);
        av_packet_free(&mAvPacket);
        mAvPacket = NULL;
        mAvFrame = NULL;
        return -1;
    }
    return 0;
}

void AvH264Encoder::close()
{
    if (mCdcCtxt)
    {
        avcodec_free_context(&mCdcCtxt);
        mCdcCtxt = NULL;
    }
    if (mAvFrame)
    {
        av_frame_free(&mAvFrame);
        mAvFrame = NULL;
    }
    if (mAvPacket)
    {
        av_packet_free(&mAvPacket);
        mAvPacket = NULL;
    }
}

const AVPacket* AvH264Encoder::encode(const cv::Mat& mat)
{
    if (mat.empty())
    {
        return NULL;
    }
    cv::resize(mat, mCvFrameEdited, mCvSize);
    cv::cvtColor(mCvFrameEdited, mCvFrameEdited, cv::COLOR_BGR2YUV_I420);

    mAvFrame->data[0] = mCvFrameEdited.data;
    mAvFrame->data[1] = mCvFrameEdited.data + mFrameSize;
    mAvFrame->data[2] = mCvFrameEdited.data + mFrameSize * 5 / 4;
    mAvFrame->pts = mPts++;

    if ((avcodec_send_frame(mCdcCtxt, mAvFrame) >= 0) &&
        (avcodec_receive_packet(mCdcCtxt, mAvPacket) == 0))
    {
        mAvPacket->stream_index = 0;
    }
    return mAvPacket;
}
