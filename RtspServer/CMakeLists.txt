project(RtspServer)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/src/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/src/net/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/src/xop/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/src/3rdpart/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/src/3rdpart/md5/")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/PHZ76_RtspServer/") #3rd party repo

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/EnterpriseEmitter/")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/EnterpriseEmitter/")

###### MSc Project - RTSP Server application

add_executable(${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/rtsp_server_main.cc"
)
target_link_libraries(${PROJECT_NAME} PRIVATE PHZ76_RtspServer)
target_link_libraries(${PROJECT_NAME} PRIVATE EnterpriseEmitter)
if (UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--whole-archive")
endif ()
if (STREAMING_VIDEOS_DIR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DOUTPUT_VIDEO_DIR="${STREAMING_VIDEOS_DIR}")
endif ()
