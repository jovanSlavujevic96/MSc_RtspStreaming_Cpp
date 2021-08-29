#include "AvH264Decoder.h"

/// <summary>
/// AvH264_Decoder
/// </summary>

int AvH264Decoder::avframe2cvmat(AVFrame* frame, cv::Mat& res)
{
    int width = frame->width, height = frame->height;
    res.create(height * 3 / 2, width, CV_8UC1);
    memcpy(res.data, frame->data[0], width * height);
    memcpy(res.data + width * height, frame->data[1], width * height / 4);
    memcpy(res.data + width * height * 5 / 4, frame->data[2], width * height / 4);

    cv::cvtColor(res, res, cv::COLOR_YUV2BGR_I420);
    return 0;
}

AVFrame* AvH264Decoder::cvmat2avframe(const cv::Mat& mat)
{
    AVFrame* avframe = av_frame_alloc();
    if (avframe && !mat.empty())
    {
        avframe->format = AV_PIX_FMT_YUV420P;
        avframe->width = mat.cols;
        avframe->height = mat.rows;
        av_frame_get_buffer(avframe, 0);
        av_frame_make_writable(avframe);
        cv::Mat yuv;
        cv::cvtColor(mat, yuv, cv::COLOR_BGR2YUV_I420);
        // calc frame size
        int frame_size = mat.cols * mat.rows;
        unsigned char* pdata = yuv.data;
        // fill yuv420
        avframe->data[0] = pdata;
        avframe->data[1] = pdata + frame_size;
        avframe->data[2] = pdata + frame_size * 5 / 4;
    }
    return avframe;
}

AvH264Decoder::~AvH264Decoder()
{
    AvH264Decoder::close();
}

int AvH264Decoder::init()
{
    av_init_packet(&packet);

    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        std::cerr << "avcodec_find_decoder failed!\n";
        return -1;
    }

    context = avcodec_alloc_context3(codec);
    if (!context)
    {
        std::cerr << "avcodec_alloc_context3 failed!\n";
        return -2;
    }

    context->codec_type = AVMEDIA_TYPE_VIDEO;
    context->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(context, codec, NULL) < 0)
    {
        std::cerr << "avcodec_open2 failed!\n";
        return -3;
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        return -4;
    }
    return 0;
}

int AvH264Decoder::decode(uint8_t* pDataIn, int nInSize, cv::Mat& res)
{
    packet.size = nInSize;
    packet.data = pDataIn;
    int ret;
    if (packet.size > 0)
    {
        int got_picture = 0;
        ret = avcodec_send_packet(context, &packet);
        if (0 == ret)
        {
            got_picture = avcodec_receive_frame(context, frame);
        }
        else if (ret < 0)
        {
            std::cerr << "avcodec_send_packet failed\n";
            return -2;
        }

        if (!got_picture)
        {
            ret = avframe2cvmat(frame, res);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        std::cerr << "No data to decode!\n";
        return -1;
    }
    return ret;
}

void AvH264Decoder::close()
{
    if (context)
    {
        avcodec_close(context);
        av_free(context);
        context = NULL;
    }
    if (frame)
    {
        av_frame_free(&frame);
        frame = NULL;
    }
}
