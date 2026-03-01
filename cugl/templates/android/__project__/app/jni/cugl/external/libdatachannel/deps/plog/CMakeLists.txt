cmake_minimum_required(VERSION 3.6)

project(plog VERSION 1.1.10 LANGUAGES CXX)

# check if building as a stand-alone project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(IS_TOPLEVEL_PROJECT TRUE)
else()
    set(IS_TOPLEVEL_PROJECT FALSE)
endif()

# Only allow this makefile as a subdirectory
if(NOT DEFINED DATACHANNEL_PATH)
	message(FATAL_ERROR, "Makefile for WebRTC must be called as a subdirectory")
endif()

set(PLOG_PATH "${DATACHANNEL_PATH}/deps/plog")

# Make sure install paths work on all platforms.
include(GNUInstallDirs)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${PLOG_PATH}/include>
)

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} INTERFACE log)
endif()