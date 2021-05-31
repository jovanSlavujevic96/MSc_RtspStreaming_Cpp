######  Simple example

add_executable(RtspH264_VideoStreaming
	"${CMAKE_CURRENT_SOURCE_DIR}/h264_video_streaming.cc"
)
target_compile_definitions(RtspH264_VideoStreaming PRIVATE 
	-DVIDEO_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test.h264"
)
if (UNIX)
    set_target_properties(RtspH264_VideoStreaming PROPERTIES
        LINK_FLAGS "-Wl,--whole-archive"
    )
endif ()
target_link_libraries(RtspH264_VideoStreaming PUBLIC	
	MyRtsp
	Threads::Threads
)

###### MSc Project

add_executable(RtspH264_CameraStreaming
	"${CMAKE_CURRENT_SOURCE_DIR}/camera_streaming.cc"
)
if (UNIX)
    set_target_properties(RtspH264_CameraStreaming PROPERTIES
        LINK_FLAGS "-Wl,--whole-archive"
    )
endif ()
target_link_libraries(RtspH264_CameraStreaming PUBLIC	
	MyRtsp
	Threads::Threads
)
