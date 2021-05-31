project(rtsp)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/MediaSource/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/MediaSource")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/MediaConverter/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/MediaConverter")

file(GLOB RTSP_SRC_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

file(GLOB RTSP_HEADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library("${PROJECT_NAME}" STATIC
	"${RTSP_SRC_FILES}"
	"${RTSP_HEADER_FILES}"
)
target_link_libraries("${PROJECT_NAME}" PUBLIC
	MediaSource
	MediaConverter
)
