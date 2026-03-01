cmake_minimum_required(VERSION 3.1...3.5)

# extract version out of configure.ac
file(READ "configure.ac" CONFIGURE_AC)
string(REGEX MATCH "AC_INIT\\(\\[libjpeg\\], \\[([0-9.]+)\\]\\)" ver_found "${CONFIGURE_AC}")
if(NOT ver_found)
   message(FATAL_ERROR "Could not extract version from configure.ac")
endif()
set(LIBJPEG_VERSION "${CMAKE_MATCH_1}")

project(jpeg LANGUAGES C VERSION ${LIBJPEG_VERSION})

# Only allow this makefile as a subdirectory
if(NOT DEFINED JPEG_DIR)
	message(FATAL_ERROR, "Makefile for JPEG must be called as a subdirectory")
endif()

# To keep things getting out of control path-wise
set(CMAKE_OBJECT_PATH_MAX 512)

option(BUILD_SHARED_LIBS "Build shared jpeg library" OFF)

# Resume the normal SDL CMake project
include(CheckSymbolExists)
include(CheckCSourceCompiles)
include(GNUInstallDirs)

check_symbol_exists(setmode io.h USE_SETMODE)
check_c_source_compiles("
  int x[(char)0xff]; /* x[-1] if signed */
  int main(void) {return 0;}" CHAR_IS_UNSIGNED
)
check_c_source_compiles("
  int x[((signed char)0xff) >> 1]; /* x[-1] if signed */
  int main(void) {return 0;}" RIGHT_SHIFT_IS_UNSIGNED
)

set(jpeg_headers
    ${JPEG_DIR}/jerror.h
    ${JPEG_DIR}/jmorecfg.h
    ${JPEG_DIR}/jpeglib.h
)
set(jpeg_sources
    ${JPEG_DIR}/jaricom.c 
    ${JPEG_DIR}/jcapimin.c 
    ${JPEG_DIR}/jcapistd.c 
    ${JPEG_DIR}/jcarith.c 
    ${JPEG_DIR}/jccoefct.c 
    ${JPEG_DIR}/jccolor.c
    ${JPEG_DIR}/jcdctmgr.c 
    ${JPEG_DIR}/jchuff.c 
    ${JPEG_DIR}/jcinit.c 
    ${JPEG_DIR}/jcmainct.c 
    ${JPEG_DIR}/jcmarker.c 
    ${JPEG_DIR}/jcmaster.c
    ${JPEG_DIR}/jcomapi.c 
    ${JPEG_DIR}/jcparam.c 
    ${JPEG_DIR}/jcprepct.c 
    ${JPEG_DIR}/jcsample.c 
    ${JPEG_DIR}/jctrans.c 
    ${JPEG_DIR}/jdapimin.c
    ${JPEG_DIR}/jdapistd.c 
    ${JPEG_DIR}/jdarith.c 
    ${JPEG_DIR}/jdatadst.c 
    ${JPEG_DIR}/jdatasrc.c 
    ${JPEG_DIR}/jdcoefct.c 
    ${JPEG_DIR}/jdcolor.c
    ${JPEG_DIR}/jddctmgr.c 
    ${JPEG_DIR}/jdhuff.c 
    ${JPEG_DIR}/jdinput.c 
    ${JPEG_DIR}/jdmainct.c 
    ${JPEG_DIR}/jdmarker.c 
    ${JPEG_DIR}/jdmaster.c
    ${JPEG_DIR}/jdmerge.c 
    ${JPEG_DIR}/jdpostct.c 
    ${JPEG_DIR}/jdsample.c 
    ${JPEG_DIR}/jdtrans.c 
    ${JPEG_DIR}/jerror.c 
    ${JPEG_DIR}/jfdctflt.c
    ${JPEG_DIR}/jfdctfst.c 
    ${JPEG_DIR}/jfdctint.c 
    ${JPEG_DIR}/jidctflt.c 
    ${JPEG_DIR}/jquant1.c
    ${JPEG_DIR}/jquant2.c 
    ${JPEG_DIR}/jutils.c 
    ${JPEG_DIR}/jmemmgr.c
)

if(BUILD_SHARED_LIBS AND WIN32)
  set(CMAKE_SHARED_LIBRARY_NAME_WITH_VERSION 1)
endif()

add_library(jpeg ${jpeg_sources})
add_library(JPEG::JPEG ALIAS jpeg)

if(ANDROID)
  target_sources(jpeg PRIVATE ${JPEG_DIR}/jmem-android.c)
  # Android.mk defines this:
  target_compile_definitions(jpeg PRIVATE AVOID_TABLES)
else()
  target_sources(jpeg PRIVATE ${JPEG_DIR}/jmemansi.c)
endif()
target_sources(jpeg PRIVATE ${JPEG_DIR}/jidctint.c ${JPEG_DIR}/jidctfst.c)
# FIXME : include asm for ARM
# target_sources(jpeg PRIVATE ${JPEG_DIR}/jidctint.c ${JPEG_DIR}/jidctfst.S)

set_property(TARGET jpeg PROPERTY DEFINE_SYMBOL JPEG_SHARED)
set_target_properties(jpeg PROPERTIES
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  VERSION "${PROJECT_VERSION}"
)
if(BUILD_SHARED_LIBS AND MSVC)
  set_target_properties(jpeg PROPERTIES
    PREFIX "lib"
  )
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/jconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfig.h)
list(APPEND jpeg_headers ${CMAKE_CURRENT_BINARY_DIR}/jconfig.h)
target_include_directories(jpeg PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS jpeg
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES ${jpeg_headers}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
