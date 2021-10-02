#pragma once

#include <string>
#include <memory>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}

#include "AvH264EncConfig.h"

class AvH264Writer
{
public:
	explicit AvH264Writer() = default;
	~AvH264Writer();

	int open(const std::string& video) noexcept;
	void close() noexcept;
	void write(AVPacket* packet);

	void bind(std::shared_ptr<AvH264EncConfig> configuration);
private:
	static inline constexpr const char* cFileNameSuffix = "h264";

	std::shared_ptr<AvH264EncConfig> mEncConfig = nullptr;
	AVFormatContext* mOutContext = NULL;
	AVStream* mAvStream = NULL;
	AVFrame* mAvFrame = NULL;
	int64_t mFramePts = 0;
};
