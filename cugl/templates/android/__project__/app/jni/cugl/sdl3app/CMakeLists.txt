# Primary CMake file for SDL_app
# This makefile is less structured than SDL_atk because it is not meant to be
# a traditional SDL add-on. This file is VERY specific our build system.
cmake_minimum_required(VERSION 3.16...3.28)

# Make option() cache overrides in subdirs stick
cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(cmake_build_type_undefined 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 2)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.2.6)

project(SDL3_app
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

# Only allow this makefile as a subdirectory
if(NOT DEFINED SDL3_APP_DIR)
	message(FATAL_ERROR, "Makefile for SDL3_App must be called as a subdirectory")
endif()

set(SDL3_APP_SRC   "${SDL3_APP_DIR}/src/app")
set(SDL3_APP_INC   "${SDL3_APP_DIR}/include")
set(SDL3_BASE_DIR  "${SDL3_APP_DIR}")
set(SDL3_IMAGE_DIR "${SDL3_APP_DIR}")
set(SDL3_TTF_DIR   "${SDL3_APP_DIR}")
set(SDL3_ATK_DIR   "${SDL3_APP_DIR}")

# Get the platform
include("${CMAKE_CURRENT_LIST_DIR}/modules/sdlplatform.cmake")
SDL_DetectCMakePlatform()

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ON)
option(SDL3APP_BUILD_SHARED_LIBS  "Build all components as a shared library" ON)
set(BUILD_SHARED_LIBS ${SDL3APP_BUILD_SHARED_LIBS})

set(COMPONENT_LIBS)

if(SDL3APP_BUILD_SHARED_LIBS)
    set(sdl3_app_target_name SDL3_app-shared)
    set(APP_TYPE SHARED)
else()
    set(sdl3_app_target_name SDL3_app-static)
    set(APP_TYPE STATIC)
endif()

# New code for SDL_app
set(APP_SOURCES
    ${SDL3_APP_SRC}/APP_version.c
    ${SDL3_APP_SRC}/device/APP_device.c
    ${SDL3_APP_SRC}/display/APP_display.c
)

if(ANDROID)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/android/APP_sysdevice.c)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/android/APP_sysdisplay.c)
elseif(MACOS)
    enable_language(OBJC)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/appkit/APP_sysdevice.m)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/appkit/APP_sysinternals.m)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/appkit/APP_sysdisplay.m)
elseif(IOS OR TVOS OR VISIONOS OR WATCHOS)
    enable_language(OBJC)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/uikit/APP_sysdevice.m)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/uikit/APP_sysinternals.m)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/uikit/APP_sysdisplay.c)
elseif(WIN32)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/windows/APP_sysdevice.cpp)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/dummy/APP_sysdisplay.c)
elseif(LINUX)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/posix/APP_sysdevice.cpp)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/dummy/APP_sysdisplay.c)
else()
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/device/dummy/APP_sysdevice.c)
    list(APPEND APP_SOURCES ${SDL3_APP_SRC}/display/dummy/APP_sysdisplay.c)
endif()

add_library(${sdl3_app_target_name} ${APP_TYPE} ${APP_SOURCES})
target_compile_features(${sdl3_app_target_name} PRIVATE c_std_99)
file(GLOB APP_PUBLIC_HEADERS
    ${SDL3_APP_INC}/SDL3/*.h
)
if(CMAKE_VERSION VERSION_LESS 3.23)
    message("INCLUDING ${APP_PUBLIC_HEADERS}")
    target_sources(${sdl3_app_target_name} PUBLIC ${APP_PUBLIC_HEADERS})
    target_include_directories(${sdl3_app_target_name}
        PUBLIC
            $<BUILD_INTERFACE:${SDL3_APP_INC}>
            $<INSTALL_INTERFACE:include>
    )
    set_target_properties(${sdl3_app_target_name}
        PROPERTIES PUBLIC_HEADER "${APP_PUBLIC_HEADERS}"
    )
else()
    target_sources(${sdl3_app_target_name}
        PUBLIC
            FILE_SET public_headers
                TYPE HEADERS
                BASE_DIRS ${SDL3_APP_INC}
                FILES ${APP_PUBLIC_HEADERS}
    )
endif()

target_include_directories(${sdl3_app_target_name} PRIVATE "${SDL3_BASE_DIR}/src")
target_include_directories(${sdl3_app_target_name} PRIVATE "${SDL3_BASE_DIR}/src/video/khronos")
target_include_directories(${sdl3_app_target_name} PRIVATE "${SDL3_BASE_DIR}/include/build_config")

if(APPLE)
  target_compile_options(${sdl3_app_target_name} PRIVATE
    -Wno-deprecated-declarations
    -Wno-objc-property-no-attribute
  )
endif()

# Aliases
add_library(SDL3_app::${sdl3_app_target_name} ALIAS ${sdl3_app_target_name})
if(NOT TARGET SDL3_app::SDL3_app)
    add_library(SDL3_app::SDL3_app ALIAS ${sdl3_app_target_name})
endif()

# Pull in the libraries

# SDL3 wants these toggles (build static, not shared)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TESTS  OFF CACHE BOOL "" FORCE)

if (ANDROID) 
    set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "Disable libusb backend on Android" FORCE)
    set(SDL_PTHREADS ON CACHE BOOL "Enable pthreads on Android" FORCE)
endif()
add_subdirectory(sdl3)
list(APPEND COMPONENT_LIBS SDL3::SDL3-static)

# The other three subprojects rely on BUILD_SHARED_LIBS
unset(BUILD_SHARED_LIBS)                 # clear any normal var
unset(BUILD_SHARED_LIBS CACHE)           # clear any stale cache
set(BUILD_SHARED_LIBS OFF) 
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(sdl3image)
list(APPEND COMPONENT_LIBS SDL3_image::SDL3_image-static)

add_subdirectory(sdl3ttf)
list(APPEND COMPONENT_LIBS SDL3_ttf::SDL3_ttf-static)

add_subdirectory(sdl3atk)
list(APPEND COMPONENT_LIBS SDL3_atk::SDL3_atk-static)

# Restore the settings
set(BUILD_SHARED_LIBS "${SDL3APP_BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS "${SDL3APP_BUILD_SHARED_LIBS}" CACHE BOOL "Build libs shared" FORCE)

set(APP_LINK_SCOPE PUBLIC)
if (SDL3APP_BUILD_SHARED_LIBS)
    set(APP_LINK_SCOPE PRIVATE)
endif()

# Link libraries, exposing all symbols from SDL and its add-ons
if(SDL3APP_BUILD_SHARED_LIBS)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        # Portable, modern way
        target_link_libraries(${sdl3_app_target_name} ${APP_LINK_SCOPE}
            $<LINK_LIBRARY:WHOLE_ARCHIVE,SDL3::SDL3-static>
            $<LINK_LIBRARY:WHOLE_ARCHIVE,SDL3_image::SDL3_image-static>
            $<LINK_LIBRARY:WHOLE_ARCHIVE,SDL3_ttf::SDL3_ttf-static>
            $<LINK_LIBRARY:WHOLE_ARCHIVE,SDL3_atk::SDL3_atk-static>
        )
    else()
        # Back-compat per-platform fallbacks
        if(MSVC)
            foreach(tgt IN LISTS COMPONENT_LIBS)
                # /WHOLEARCHIVE accepts a lib path; use generator expression for the file
                target_link_libraries(${sdl3_app_target_name} ${APP_LINK_SCOPE} ${COMPONENT_LIBS})
                target_link_options(${sdl3_app_target_name} ${APP_LINK_SCOPE} "/WHOLEARCHIVE:$<TARGET_FILE:${tgt}>")
            endforeach()
        elseif(APPLE)
            foreach(tgt IN LISTS COMPONENT_LIBS)
                target_link_options(${sdl3_app_target_name} ${APP_LINK_SCOPE} "-Wl,-force_load,$<TARGET_FILE:${tgt}>")
            endforeach()
        else()
            # GNU ld / lld: wrap an extra PRIVATE pass of the libs with whole-archive
            target_link_options(${sdl3_app_target_name} ${APP_LINK_SCOPE} -Wl,--whole-archive)
            target_link_libraries(${sdl3_app_target_name} ${APP_LINK_SCOPE} ${COMPONENT_LIBS})
            target_link_options(${sdl3_app_target_name} ${APP_LINK_SCOPE} -Wl,--no-whole-archive)
        endif()
    endif()

    # If you globally force hidden visibility, re-expose this target's symbols:
    set_target_properties(${sdl3_app_target_name}  PROPERTIES C_VISIBILITY_PRESET default)

    # On Windows, if you truly want to export all symbols from app_extras.dll:
    if (WINDOWS)
        set_target_properties(${sdl3_app_target_name}  PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
else()
    target_link_libraries(${sdl3_app_target_name} ${APP_LINK_SCOPE} ${COMPONENT_LIBS})
endif()
