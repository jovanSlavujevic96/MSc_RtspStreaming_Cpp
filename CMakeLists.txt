cmake_minimum_required(VERSION 3.10.2)
project(RtspServer_Cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

if (MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	add_definitions(-DBUILD_DLL)
endif()

find_package(Threads REQUIRED)

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/net/"
	"${CMAKE_CURRENT_SOURCE_DIR}/rtsp/"
	"${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/"
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/net")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/rtsp")

add_library(MyRtsp STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/dummy.cc"
)
target_link_libraries(MyRtsp
	net
	rtsp
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/apps")
