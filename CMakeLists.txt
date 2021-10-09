cmake_minimum_required(VERSION 3.8)
project(MSc_RtspStreaming)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo;Debug" CACHE STRING "" FORCE)

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

if (WIN32)
	set(CMAKE_MODULE_PATH "$ENV{OPENCV_DIR};${CMAKE_MODULE_PATH}") #apend opencv cmake module
	set(CMAKE_MODULE_PATH "$ENV{FFMPEG_DIR};${CMAKE_MODULE_PATH}") #apend ffmpeg cmake module
endif()

set(STREAMING_VIDEOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/streaming_videos")
if(NOT EXISTS ${STREAMING_VIDEOS_DIR})
	file(MAKE_DIRECTORY ${STREAMING_VIDEOS_DIR})
endif()

find_package(OpenCV REQUIRED)
find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE REQUIRED)

find_package(Threads REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${OpenCV_INCLUDE_DIRS}")
include_directories("${FFMPEG_INCLUDE_DIRS}")

link_libraries(Threads::Threads)
link_libraries("${OpenCV_LIBS}")
link_libraries("${FFMPEG_LIBRARIES}")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/SocketNetworking/")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SocketNetworking/socket_net/include")
link_libraries(socket_net)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/H264StreamingAPI/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/NetworkManagerAPI/")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/H264StreamingAPI/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/NetworkManagerAPI/")

link_libraries(H264StreamingAPI)
link_libraries(NetworkManagerAPI)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/RtspServer/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/RtspClient/")
