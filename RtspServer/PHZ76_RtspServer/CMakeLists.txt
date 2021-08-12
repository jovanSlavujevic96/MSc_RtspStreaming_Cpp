project(PHZ76_RtspServer)

### md5 header files

file(GLOB MD5_HEADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/3rdpart/md5/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/3rdpart/md5/*.h"
)

### net source and header files

file(GLOB NET_SRC_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.cc"
)

file(GLOB NET_HEADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/net/*.h"
)

### xop source and header files

file(GLOB XOP_SRC_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/xop/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/xop/*.cc"
)

file(GLOB XOP_HEADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/xop/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/xop/*.h"
)

### create PHZ76_RtspServer static library

add_library(${PROJECT_NAME} STATIC
	"${MD5_HEADER_FILES}"
	"${NET_SRC_FILES}"
	"${NET_HEADER_FILES}"
	"${XOP_SRC_FILES}"
	"${XOP_HEADER_FILES}"
)

### create example apps

add_executable(Rtsp_H264_File "${CMAKE_CURRENT_SOURCE_DIR}/example/rtsp_h264_file.cpp")
target_link_libraries(Rtsp_H264_File PRIVATE "${PROJECT_NAME}")
target_compile_definitions(Rtsp_H264_File PRIVATE -DVIDEO_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test.h264")

add_executable(Rtsp_Pusher "${CMAKE_CURRENT_SOURCE_DIR}/example/rtsp_pusher.cpp")
target_link_libraries(Rtsp_Pusher PRIVATE "${PROJECT_NAME}")

add_executable(Rtsp_Server "${CMAKE_CURRENT_SOURCE_DIR}/example/rtsp_server.cpp")
target_link_libraries(Rtsp_Server PRIVATE "${PROJECT_NAME}")

if (UNIX)
    set_target_properties(Rtsp_H264_File PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
	set_target_properties(Rtsp_Pusher PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
	set_target_properties(Rtsp_Server PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
endif ()
