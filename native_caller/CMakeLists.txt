cmake_minimum_required(VERSION 3.16)

project(webrtc-sample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_MESSAGE LAZY)

find_library(WebRTC NAMES libwebrtc.a PATHS ${CMAKE_CURRENT_LIST_DIR}/webrtc/lib REQUIRED)

set(WEBRTC_ROOT_DIR CACHE STRING "")

#[[ 
file(
  INSTALL ${WEBRTC_ROOT_DIR}/api
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/call
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/common_audio
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/common_video
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/logging
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/video
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/media
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/modules
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/p2p
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/pc
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/rtc_base
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/system_wrappers
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include
  FILES_MATCHING
  PATTERN "*.h"
)

file(
  INSTALL ${WEBRTC_ROOT_DIR}/third_party/abseil-cpp/absl 
  DESTINATION ${CMAKE_CURRENT_LIST_DIR}/webrtc/include/third_party
  FILES_MATCHING
  PATTERN "*.h"
) 
]]

# Force WebSocket++ to use 100% C++11 mode, so that it doesn't try to look for Boost
# (Note that under MinGW, Boost.Thread is still required, due to a bug in MinGW that prevents the C++11 version from being used)
add_definitions(-D_WEBSOCKETPP_CPP11_STRICT_)

# Add the third-party headers directory to the search path
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/external")

# Compile jsoncpp from source
add_library(jsoncpp STATIC ${PROJECT_SOURCE_DIR}/external/jsoncpp.cpp)

add_executable(webrtc-sample main.cpp default_client.cpp websocket_server.cpp)

find_package(Threads REQUIRED)
target_link_libraries(webrtc-sample PRIVATE ${WebRTC} ${CMAKE_DL_LIBS} Threads::Threads jsoncpp)

target_compile_definitions(webrtc-sample PRIVATE WEBRTC_POSIX)
target_include_directories(webrtc-sample PRIVATE ${CMAKE_CURRENT_LIST_DIR}/webrtc/include)
target_include_directories(webrtc-sample PRIVATE ${CMAKE_CURRENT_LIST_DIR}/webrtc/include/third_party)
