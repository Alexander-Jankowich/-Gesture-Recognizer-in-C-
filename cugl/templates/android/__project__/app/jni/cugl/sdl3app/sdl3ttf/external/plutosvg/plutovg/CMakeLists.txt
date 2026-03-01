cmake_minimum_required(VERSION 3.15)

set(PLUTOVG_VERSION_MAJOR 1)
set(PLUTOVG_VERSION_MINOR 1)
set(PLUTOVG_VERSION_MICRO 0)

project(plutovg LANGUAGES C VERSION ${PLUTOVG_VERSION_MAJOR}.${PLUTOVG_VERSION_MINOR}.${PLUTOVG_VERSION_MICRO})

# Only allow this makefile as a subdirectory
if(NOT DEFINED PLUTOVG_DIR)
	message(FATAL_ERROR, "Makefile for plutovg must be called as a subdirectory")
endif()

# To keep things getting out of control path-wise
set(CMAKE_OBJECT_PATH_MAX 512)

# Resume the normal SDL CMake project

set(plutovg_sources
    ${PLUTOVG_DIR}/source/plutovg-blend.c
    ${PLUTOVG_DIR}/source/plutovg-canvas.c
    ${PLUTOVG_DIR}/source/plutovg-font.c
    ${PLUTOVG_DIR}/source/plutovg-matrix.c
    ${PLUTOVG_DIR}/source/plutovg-paint.c
    ${PLUTOVG_DIR}/source/plutovg-path.c
    ${PLUTOVG_DIR}/source/plutovg-rasterize.c
    ${PLUTOVG_DIR}/source/plutovg-surface.c
    ${PLUTOVG_DIR}/source/plutovg-ft-math.c
    ${PLUTOVG_DIR}/source/plutovg-ft-raster.c
    ${PLUTOVG_DIR}/source/plutovg-ft-stroker.c
)

set(plutovg_headers
    ${PLUTOVG_DIR}/include/plutovg.h
    ${PLUTOVG_DIR}/source/plutovg-private.h
    ${PLUTOVG_DIR}/source/plutovg-utils.h
    ${PLUTOVG_DIR}/source/plutovg-ft-math.h
    ${PLUTOVG_DIR}/source/plutovg-ft-raster.h
    ${PLUTOVG_DIR}/source/plutovg-ft-stroker.h
    ${PLUTOVG_DIR}/source/plutovg-ft-types.h
    ${PLUTOVG_DIR}/source/plutovg-stb-image-write.h
    ${PLUTOVG_DIR}/source/plutovg-stb-image.h
    ${PLUTOVG_DIR}/source/plutovg-stb-truetype.h
)

add_library(plutovg ${plutovg_sources} ${plutovg_headers})
add_library(plutovg::plutovg ALIAS plutovg)

set_target_properties(plutovg PROPERTIES
    SOVERSION ${PLUTOVG_VERSION_MAJOR}
    C_VISIBILITY_PRESET hidden
    C_STANDARD_REQUIRED ON
    C_STANDARD 99
)

target_include_directories(plutovg PRIVATE
    ${PLUTOVG_DIR}/include
    ${PLUTOVG_DIR}/source
)

target_include_directories(plutovg PUBLIC
    $<BUILD_INTERFACE:${PLUTOVG_DIR}/include>
    $<INSTALL_INTERFACE:include/plutovg>
)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    target_link_libraries(plutovg PRIVATE m)
endif()

target_compile_definitions(plutovg PRIVATE PLUTOVG_BUILD)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(plutovg PUBLIC PLUTOVG_BUILD_STATIC)
endif()

include(GNUInstallDirs)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/plutovgConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plutovgConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plutovg
)

write_basic_package_version_file(plutovgConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PLUTOVG_DIR}/include/plutovg.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/plutovg
)

install(TARGETS plutovg
    EXPORT plutovgTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT plutovgTargets
    FILE plutovgTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plutovg
    NAMESPACE plutovg::
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/plutovgConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/plutovgConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/plutovg
)

export(EXPORT plutovgTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/plutovgTargets.cmake
    NAMESPACE plutovg::
)

file(RELATIVE_PATH plutovg_pc_prefix_relative
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    "${CMAKE_INSTALL_PREFIX}"
)

set(plutovg_pc_cflags "")
set(plutovg_pc_libs_private "")

if(MATH_LIBRARY)
    string(APPEND plutovg_pc_libs_private " -lm")
endif()

if(NOT BUILD_SHARED_LIBS)
    string(APPEND plutovg_pc_cflags " -DPLUTOVG_BUILD_STATIC")
endif()

string(CONFIGURE [[
prefix=${pcfiledir}/@plutovg_pc_prefix_relative@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@

Name: PlutoVG
Description: Tiny 2D vector graphics library in C
Version: @PROJECT_VERSION@

Cflags: -I${includedir}/plutovg@plutovg_pc_cflags@
Libs: -L${libdir} -lplutovg
Libs.private:@plutovg_pc_libs_private@
]] plutovg_pc @ONLY)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plutovg.pc" "${plutovg_pc}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/plutovg.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

option(PLUTOVG_BUILD_EXAMPLES "Build examples" OFF)
if(PLUTOVG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
