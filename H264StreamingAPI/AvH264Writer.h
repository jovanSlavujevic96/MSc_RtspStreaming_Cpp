#pragma once

#include <string>
#include <memory>
#include <condition_variable>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
}
#include <opencv2/opencv.hpp>

#include "AvH264EncConfig.h"

#define DEFAULT_MAX_VIDEOS_PER_WRITER 2u

class AvH264Writer
{
public:
	explicit AvH264Writer(const char* path, uint16_t camera_id, uint16_t starting_counter = 0, uint16_t max_number_of_videos = DEFAULT_MAX_VIDEOS_PER_WRITER);
	AvH264Writer(const AvH264Writer& writer);
	~AvH264Writer();

	const std::string& getCurrentFileName() const;
	const std::string& getRecordedFileName() const;

	void initWriter() noexcept(false);
	void closeWriter() noexcept(false);
	void writeFrame(AVPacket* packet);

	void bindEncodingConfiguration(AvH264EncConfig* configuration);
	void bindNetworkManagerNotifier(std::condition_variable* condition);
	void bindStreamingNotifier(std::shared_ptr<std::condition_variable> condition);
private: //methods
	void preUpdateFileNames();
	void postUpdateFileNames();
private: //fields
	static inline constexpr const char* cFileNameSuffix = "h264";

	std::string mPathToFile;
	std::string mCurrentFileName;
	std::string mRecordedFileName;
	uint16_t mVideoCounter;
	uint16_t mMaxVideos;
	uint16_t mCameraId;

	AvH264EncConfig* mEncConfig;
	AVFormatContext* mOutContext;
	AVStream* mAvStream;
	AVFrame* mAvFrame;
	int64_t mFramePts;

	std::shared_ptr<std::condition_variable> mStreamingNotifier;
	std::condition_variable* mNetworkManagerNotififer;
};
