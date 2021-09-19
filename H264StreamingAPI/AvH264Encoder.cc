#include "AvH264Encoder.h"

AvH264Encoder::AvH264Encoder()
{
    mFrameSize = 0;
    mPts = 0;
    mCodec = NULL;
    mCodecCtxt = NULL;
    mAvFrame = NULL;
    mAvPacket = NULL;
}

AvH264Encoder::AvH264Encoder(const AvH264Encoder& encoder)
{
    AvH264Encoder* encoder_ = const_cast<AvH264Encoder*>(&encoder);

    mFrameSize = encoder_->mFrameSize;
    mPts = encoder_->mPts;
    mCodec = encoder_->mCodec;
    mCodecCtxt = encoder_->mCodecCtxt;
    mAvFrame = encoder_->mAvFrame;
    mAvPacket = encoder_->mAvPacket;
    mCvSize = encoder_->mCvSize;

    encoder_->mCodec = NULL;
    encoder_->mCodecCtxt = NULL;
    encoder_->mAvFrame = NULL;
    encoder_->mAvPacket = NULL;
}

AvH264Encoder::~AvH264Encoder()
{
    AvH264Encoder::close();
}

int AvH264Encoder::open(const AvH264EncConfig& h264_config)
{
    mCodec = ::avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!mCodec)
    {
        std::cout << "!cdc_\n";
        return -1;
    }
    mCodecCtxt = avcodec_alloc_context3(mCodec);
    if (!mCodecCtxt)
    {
        std::cout << "!cdc_ctx_\n";
        return -1;
    }
    mCodecCtxt->bit_rate = h264_config.bit_rate;
    mCodecCtxt->width = h264_config.width;
    mCodecCtxt->height = h264_config.height;
    mCodecCtxt->time_base = { 1, h264_config.frame_rate };
    mCodecCtxt->framerate = { h264_config.frame_rate, 1 };
    mCodecCtxt->gop_size = h264_config.gop_size;
    mCodecCtxt->max_b_frames = h264_config.max_b_frames;
    mCodecCtxt->pix_fmt = AV_PIX_FMT_YUV420P;
    mCodecCtxt->codec_id = AV_CODEC_ID_H264;
    mCodecCtxt->codec_type = AVMEDIA_TYPE_VIDEO;
    mCodecCtxt->qmin = 10; /*10*/
    mCodecCtxt->qmax = 19; /*51*/
    //mCdcCtxt->qcompress = 0.6f;

    AVDictionary* dict = 0;
    av_dict_set(&dict, "preset", "slow", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
    av_dict_set(&dict, "profile", "main", 0);
    av_dict_set(&dict, "rtsp_transport", "udp", 0);

    mFrameSize = mCodecCtxt->width * mCodecCtxt->height;

    mCvSize = cv::Size(mCodecCtxt->width, mCodecCtxt->height);
    return avcodec_open2(mCodecCtxt, mCodec, &dict);
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

    mAvFrame->format = mCodecCtxt->pix_fmt;
    mAvFrame->width = mCodecCtxt->width;
    mAvFrame->height = mCodecCtxt->height;

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
    if (mCodecCtxt)
    {
        avcodec_free_context(&mCodecCtxt);
        mCodecCtxt = NULL;
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

AVPacket* AvH264Encoder::encode(cv::Mat& mat)
{
    if (mat.empty())
    {
        return NULL;
    }
    cv::resize(mat, mat, mCvSize);
    cv::cvtColor(mat, mat, cv::COLOR_BGR2YUV_I420);

    mAvFrame->data[0] = mat.data;
    mAvFrame->data[1] = mat.data + mFrameSize;
    mAvFrame->data[2] = mat.data + mFrameSize * 5 / 4;
    mAvFrame->pts = mPts++;

    if ((avcodec_send_frame(mCodecCtxt, mAvFrame) >= 0) &&
        (avcodec_receive_packet(mCodecCtxt, mAvPacket) == 0))
    {
        mAvPacket->stream_index = 0;
        return mAvPacket;
    }
    return NULL;
}
