project(NetworkManagerAPI)

if (${CMAKE_BUILD_TYPE} MATCHES "Deb")
	set (BUILD_TYPE "Debug")
else ()
	set (BUILD_TYPE "Release")
endif()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "64")
	set (ARCH "x64")
else()
	if (WIN32)
		set (ARCH "Win32")
	else()
		set (ARCH "x86")
	endif()
endif()

if (WIN32)
	# MYSQL
	set (MYSQL_INCLUDE_DIRS "$ENV{MYSQL_DIR}/include")
	set (MYSQL_LIB_PATH "$ENV{MYSQL_DIR}/lib")
	set (MYSQL_LIBS "${MYSQL_LIB_PATH}/libmysql.lib;${MYSQL_LIB_PATH}/mysqlclient.lib")

	# Crypto++
	set (CRYPTOPP_INCLUDE_DIRS "$ENV{CRYPTOPP_DIR}")
	set (CRYPTOPP_LIBS "$ENV{CRYPTOPP_DIR}/${ARCH}/Output/${BUILD_TYPE}/cryptlib.lib")
endif()

file(GLOB NETWORK_MANAGER_SRC_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

file(GLOB NETWORK_MANAGER_HEADER_FILES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(${PROJECT_NAME} STATIC
	"${NETWORK_MANAGER_HEADER_FILES}"
	"${NETWORK_MANAGER_SRC_FILES}"
)
target_include_directories(${PROJECT_NAME} PRIVATE
	${MYSQL_INCLUDE_DIRS}
	${CRYPTOPP_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
	${MYSQL_LIBS}
	${CRYPTOPP_LIBS}
)

add_executable(CryptoExample "${CMAKE_CURRENT_SOURCE_DIR}/apps/crypto_example.cc")
target_link_libraries(CryptoExample PRIVATE ${PROJECT_NAME})
target_include_directories(CryptoExample PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
