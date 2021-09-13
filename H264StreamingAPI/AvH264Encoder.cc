#include "AvH264Encoder.h"

AvH264Encoder::AvH264Encoder()
{
    frame_size_ = 0;
    pts_ = 0;
    cdc_ = NULL;
    cdc_ctx_ = NULL;
    avf_ = NULL;
    avp_ = NULL;
}

AvH264Encoder::~AvH264Encoder()
{
    AvH264Encoder::close();
}

int AvH264Encoder::open(const AvH264EncConfig& h264_config)
{
    cdc_ = ::avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!cdc_)
    {
        std::cout << "!cdc_\n";
        return -1;
    }
    cdc_ctx_ = avcodec_alloc_context3(cdc_);
    if (!cdc_ctx_)
    {
        std::cout << "!cdc_ctx_\n";
        return -1;
    }
    cdc_ctx_->bit_rate = h264_config.bit_rate;
    cdc_ctx_->width = h264_config.width;
    cdc_ctx_->height = h264_config.height;
    cdc_ctx_->time_base = { 1, h264_config.frame_rate };
    cdc_ctx_->framerate = { h264_config.frame_rate, 1 };
    cdc_ctx_->gop_size = h264_config.gop_size;
    cdc_ctx_->max_b_frames = h264_config.max_b_frames;
    cdc_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    cdc_ctx_->codec_id = AV_CODEC_ID_H264;
    cdc_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    cdc_ctx_->qmin = 10; /*10*/
    cdc_ctx_->qmax = 20; /*51*/
    //cdc_ctx_->qcompress = 0.6f;

    AVDictionary* dict = 0;
    av_dict_set(&dict, "preset", "slow", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
    av_dict_set(&dict, "profile", "main", 0);
    av_dict_set(&dict, "rtsp_transport", "udp", 0);

    avf_ = av_frame_alloc();
    avp_ = av_packet_alloc();
    if (!avf_ || !avp_)
    {
        return -1;
    }

    frame_size_ = cdc_ctx_->width * cdc_ctx_->height;
    avf_->format = cdc_ctx_->pix_fmt;
    avf_->width = cdc_ctx_->width;
    avf_->height = cdc_ctx_->height;

    // alloc memory
    if ((av_frame_get_buffer(avf_, 0) < 0) ||
        (av_frame_make_writable(avf_) < 0))
    {
        return -1;
    }

    size = cv::Size(cdc_ctx_->width, cdc_ctx_->height);
    return avcodec_open2(cdc_ctx_, cdc_, &dict);
}

void AvH264Encoder::close()
{
    if (cdc_ctx_)
    {
        avcodec_free_context(&cdc_ctx_);
        cdc_ctx_ = NULL;
    }
    if (avf_)
    {
        av_frame_free(&avf_);
        avf_ = NULL;
    }
    if (avp_)
    {
        av_packet_free(&avp_);
        avp_ = NULL;
    }
}

AVPacket* AvH264Encoder::encode(const cv::Mat& mat) noexcept
{
    if (mat.empty())
    {
        avp_ = NULL;
        return avp_;
    }
    cv::Mat mat_edit;
    cv::resize(mat, mat_edit, size);
    cv::cvtColor(mat, mat_edit, cv::COLOR_BGR2YUV_I420);

    avf_->data[0] = mat_edit.data;
    avf_->data[1] = mat_edit.data + frame_size_;
    avf_->data[2] = mat_edit.data + frame_size_ * 5 / 4;
    avf_->pts = pts_++;

    if ((avcodec_send_frame(cdc_ctx_, avf_) >= 0) &&
        (avcodec_receive_packet(cdc_ctx_, avp_) == 0))
    {
        avp_->stream_index = 0;
    }
    return avp_;
}
