cmake_minimum_required(VERSION 3.8)

if (WIN32)
    project(RtspClient LANGUAGES CXX)
elseif(UNIX)
    project(RtspClient)
endif()

if (${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_SOURCE_DIR})

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "" FORCE)

    if (WIN32)
            set(CMAKE_MODULE_PATH "$ENV{OPENCV_DIR};${CMAKE_MODULE_PATH}") #apend opencv cmake module
            set(CMAKE_MODULE_PATH "$ENV{FFMPEG_DIR};${CMAKE_MODULE_PATH}") #apend ffmpeg cmake module
    endif()

    find_package(OpenCV REQUIRED)
    find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE REQUIRED)

    include_directories("${OpenCV_INCLUDE_DIRS}")
    include_directories("${FFMPEG_INCLUDE_DIRS}")

    link_libraries("${OpenCV_LIBS}")
    link_libraries("${FFMPEG_LIBRARIES}")

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../H264StreamingAPI" "${CMAKE_BINARY_DIR}/H264StreamingAPI")

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../H264StreamingAPI")
    link_libraries(H264StreamingAPI)

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../SocketNetworking" "${CMAKE_BINARY_DIR}/SocketNetworking")

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../SocketNetworking/socket_net/include")
    link_libraries(socket_net)

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../NetworkManagerAPI" "${CMAKE_BINARY_DIR}/NetworkManagerAPI")

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../NetworkManagerAPI")
    link_libraries(NetworkManagerAPI)

    find_package(Threads REQUIRED)
    link_libraries(Threads::Threads)

endif (${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_SOURCE_DIR})

#======================= INCLUSION OF Qt =======================#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_PREFIX_PATH $ENV{QTDIR})

find_package(Qt6Core REQUIRED)
find_package(Qt6Widgets REQUIRED)

#=================== INCLUSION OF Project Files ====================#
set(FORMS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/forms")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE SOURCES
    "${FORMS_DIR}/*.ui"
    "${FORMS_DIR}/*.qrc"
    "${INCLUDE_DIR}/*.h"
    "${INCLUDE_DIR}/*.hpp"
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.cc"
)

#=================== SETUP EXECTUABLE ====================#
# Enable debug logging on RELWITHDEBINFO configuration
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:RELWITHDEBINFO>:QT_MESSAGELOGCONTEXT>
)

# Add the forms directory to the AUTOUIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${FORMS_DIR})

# Add the executable
if (WIN32) 
    add_executable(RtspClient WIN32 ${SOURCES})
elseif(UNIX)
    add_executable(RtspClient  ${SOURCES})
endif()

# Add the target includes for MY_PROJECT 
target_include_directories(RtspClient  PRIVATE ${FORMS_DIR})
target_include_directories(RtspClient  PRIVATE ${INCLUDE_DIR})
target_include_directories(RtspClient  PRIVATE ${SOURCE_DIR})

#===================== LINKING LIBRARIES =======================#
target_link_libraries(RtspClient  PRIVATE Qt6::Widgets)
