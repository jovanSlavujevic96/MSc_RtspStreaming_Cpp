#include <stdexcept>

#include "AvH264.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavdevice/avdevice.h>
}

class AvH264::avh264_impl
{
public:
    AVCodec* cdc_;
    AVCodecContext* cdc_ctx_;
    AVFrame* avf_;
    AVPacket* avp_;
    cv::Size size;
    int32_t frame_size_;
    int32_t pts_;

    avh264_impl();
    ~avh264_impl() = default;

    int open(const AvH264EncConfig& h264_config);
    void close();
    void encode(cv::Mat& mat);
};

AvH264::avh264_impl::avh264_impl()
{
    frame_size_ = 0;
    pts_ = 0;
    cdc_ = NULL;
    cdc_ctx_ = NULL;
    avf_ = NULL;
    avp_ = NULL;
}

int AvH264::avh264_impl::open(const AvH264EncConfig& h264_config)
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

void AvH264::avh264_impl::close()
{
    if (cdc_ctx_)
        avcodec_free_context(&cdc_ctx_);
    if (avf_)
        av_frame_free(&avf_);
    if (avp_)
        av_packet_free(&avp_);
}

void AvH264::avh264_impl::encode(cv::Mat& mat)
{
    if (mat.empty())
    {
        avp_ = NULL;
        return;
    }
    cv::resize(mat, mat, size);
    cv::cvtColor(mat, mat, cv::COLOR_BGR2YUV_I420);

    avf_->data[0] = mat.data;
    avf_->data[1] = mat.data + frame_size_;
    avf_->data[2] = mat.data + frame_size_ * 5 / 4;
    avf_->pts = pts_++;

    if ((avcodec_send_frame(cdc_ctx_, avf_) >= 0) &&
        (avcodec_receive_packet(cdc_ctx_, avp_) == 0))
    {
        avp_->stream_index = 0;
    }
}

AvH264::AvH264() :
    avh264_pimpl{ std::make_unique<avh264_impl>() }
{
    
}

AvH264::~AvH264() = default;

int AvH264::open(const AvH264EncConfig& h264_config)
{
    return avh264_pimpl->open(h264_config);
}

void AvH264::close()
{
    avh264_pimpl->close();
}

void AvH264::encode(cv::Mat& mat, rtsp::AVFrame& frame) noexcept(false)
{
    avh264_pimpl->encode(mat);
    if (!avh264_pimpl->avp_)
    {
        throw std::runtime_error("Encode failed");
    }
    frame.size = avh264_pimpl->avp_->size;
    frame.buffer = avh264_pimpl->avp_->data;
}
