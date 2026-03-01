# SDL_atk CMAKE file
# We have attempted to make this consistent with SDL_image, SDL_ttf when possible
cmake_minimum_required(VERSION 3.16...3.28)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(cmake_build_type_undefined 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# See docs/release_checklist.md
set(MAJOR_VERSION 3)
set(MINOR_VERSION 2)
set(MICRO_VERSION 0)
set(SDL_REQUIRED_VERSION 3.2.6)

project(SDL3_atk
    LANGUAGES C
    VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
)

include("${CMAKE_CURRENT_LIST_DIR}/modules/GetGitRevisionDescription.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/modules/PkgConfigHelper.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/modules/PrivateSdlFunctions.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/modules/sdlcpu.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/modules/sdlplatform.cmake")
sdl_calculate_derived_version_variables(${MAJOR_VERSION} ${MINOR_VERSION} ${MICRO_VERSION})

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# Only allow this makefile as a subdirectory
if(NOT DEFINED SDL3_ATK_DIR)
	message(FATAL_ERROR, "Makefile for SDL3_atk must be called as a subdirectory")
endif()

set(SDL3_ATK_SRC "${SDL3_ATK_DIR}/src/atk")
set(SDL3_ATK_INC "${SDL3_ATK_DIR}/include")
set(OGG_DIR  "${SDL3_ATK_DIR}/external/ogg")
set(FLAC_DIR "${SDL3_ATK_DIR}/external/flac")
set(VORB_DIR "${SDL3_ATK_DIR}/external/vorbis")
set(MMP3_DIR "${SDL3_ATK_DIR}/external/minimp3")
set(KISS_DIR "${SDL3_ATK_DIR}/external/kissfft")

# To keep things getting out of control path-wise
set(CMAKE_OBJECT_PATH_MAX 512)

# Resume the normal SDL CMake project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(SDLATK_ROOTPROJECT ON)
else()
    set(SDLATK_ROOTPROJECT OFF)
endif()

# By default, configure in RelWithDebInfo configuration
if(SDLATK_ROOTPROJECT)
  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    # The first item in CMAKE_CONFIGURATION_TYPES is the default configuration
    if(DEFINED CMAKE_CONFIGURATION_TYPES AND "RelWithDebInfo" IN_LIST CMAKE_CONFIGURATION_TYPES)
      list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES "RelWithDebInfo")
      list(INSERT CMAKE_CONFIGURATION_TYPES 0 "RelWithDebInfo")
      set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "CMake configuration types" FORCE)
    endif()
  else()
    if(cmake_build_type_undefined)
      set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "CMake build type" FORCE)
    endif()
  endif()
endif()

# Assume MSVC projects don't have a package manager and need vendored dependencies (by default).
# Most other platforms have some kind of package manager.
# FIXME: consider a package manager such as conan/vcpkg instead of vendoring
set(vendored_default TRUE)

set(sdl3atk_install_enableable ON)
if ((TARGET SDL3-shared OR TARGET SDL3-static) AND SDL_DISABLE_INSTALL)
    # Cannot install SDL3_atk when SDL3 is built in same built, and is not installed.
    set(sdl3atk_install_enableable OFF)
endif()

include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CheckSymbolExists)

set(PLATFORM_SUPPORTS_SHARED ON)
if(EMSCRIPTEN OR VITA OR PSP OR PS2 OR N3DS OR RISCOS)
  set(PLATFORM_SUPPORTS_SHARED OFF)
endif()

option(CMAKE_POSITION_INDEPENDENT_CODE "Build static libraries with -fPIC" ${PLATFORM_SUPPORTS_SHARED})
cmake_dependent_option(BUILD_SHARED_LIBS "Build the library as a shared library" ON PLATFORM_SUPPORTS_SHARED OFF)

cmake_dependent_option(SDLATK_INSTALL "Enable SDL3_atk install target" ${SDLATK_ROOTPROJECT} "${sdl3atk_install_enableable}" OFF)
cmake_dependent_option(SDLATK_INSTALL_CPACK "Create binary SDL3_atk archive using CPack" ${SDLATK_ROOTPROJECT} "SDLATK_INSTALL" OFF)
cmake_dependent_option(SDLATK_RELOCATABLE "Create relocatable SDL_atk package" "${MSVC}" SDLATK_INSTALL OFF)
# NOTE: I do not know how to generate MAN pages and do not want to learn
# So that option has been removed

option(SDLATK_VENDORED "Use vendored third-party libraries" ON)
option(SDLATK_WERROR "Treat warnings as errors" OFF)

option(SDLATK_STRICT "Fail when a dependency could not be found" OFF)
set(find_package_strict_arg )
set(fatal_error "STATUS")
if(SDLATK_STRICT)
    set(find_package_strict_arg "REQUIRED")
    set(fatal_error "FATAL_ERROR")
endif()

option(SDLATK_TESTS "Build unit tests?" OFF)
option(SDLATK_TESTS_INSTALL "Install unit tests?" OFF)

option(SDLATK_WAV  "Support loading WAV audio" ON)
option(SDLATK_MP3  "Support loading MP3 audio" ON)
option(SDLATK_VORB "Support loading OGG Vorbis audio" ON)
option(SDLATK_FLAC "Support loading Xiph FLAC audio" ON)

cmake_dependent_option(SDLATK_WAV_SAVE  "Add WAV save support"  ON SDLATK_WAV OFF)
cmake_dependent_option(SDLATK_VORB_SAVE "Add VORB save support" ON SDLATK_VORB OFF)
cmake_dependent_option(SDLATK_FLAC_SAVE "Add FLAC save support" ON SDLATK_FLAC OFF)

set(LIBOGG_MINIMUM_VERSION "1.3.2")
set(SDLATK_OGG_VENDORED OFF)

set(LIBFLAC_MINIMUM_VERSION "1.3.2")
if(SDLATK_VENDORED AND SDLATK_FLAC)
    set(SDLATK_FLAC_VENDORED ON)
    set(SDLATK_OGG_VENDORED  ON)
else()
    set(SDLATK_FLAC_VENDORED OFF)
endif()

set(LIBVORBIS_MINIMUM_VERSION "1.3.4")
if(SDLATK_VENDORED AND SDLATK_VORB)
    set(SDLATK_VORB_VENDORED ON)
    set(SDLATK_OGG_VENDORED  ON)
else()
    set(SDLATK_VORB_VENDORED OFF)
endif()

set(LIBKISSFFT_MINIMUM_VERSION "131.1.0")
if(SDLATK_VENDORED)
    set(SDLATK_KISSFFT_VENDORED ON)
else()
    set(SDLATK_KISSFFT_VENDORED OFF)
endif()

# Save BUILD_SHARED_LIBS variable
set(SDLATK_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

set(sdl_required_components Headers)

if(SDLATK_BUILD_SHARED_LIBS)
    set(sdl3_atk_target_name SDL3_atk-shared)
    set(sdl3_target_name SDL3::SDL3-shared)

    list(APPEND sdl_required_components SDL3-shared)
else()
    set(sdl3_atk_target_name SDL3_atk-static)
    set(sdl3_target_name SDL3::SDL3)
endif()

if(NOT TARGET SDL3::Headers OR NOT TARGET ${sdl3_target_name})
    find_package(SDL3 ${SDL_REQUIRED_VERSION} REQUIRED COMPONENTS ${sdl_required_components})
endif()

SDL_DetectTargetCPUArchitectures(SDL_CPU_NAMES)
SDL_DetectCMakePlatform()

# Enable large file support on 32-bit glibc, so that the vendored libraries
# can access files with large inode numbers
check_symbol_exists("__GLIBC__" "stdlib.h" LIBC_IS_GLIBC)
if (LIBC_IS_GLIBC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(_FILE_OFFSET_BITS=64)
endif()


file(GLOB ATK_SOURCES
    ${SDL3_ATK_SRC}/audio/*.c
    ${SDL3_ATK_SRC}/codec/*.c
    ${SDL3_ATK_SRC}/dsp/*.c
    ${SDL3_ATK_SRC}/file/*.c
    ${SDL3_ATK_SRC}/math/*.c
    ${SDL3_ATK_SRC}/rand/*.c
)

add_library(${sdl3_atk_target_name} ${ATK_SOURCES})
add_library(SDL3_atk::${sdl3_atk_target_name} ALIAS ${sdl3_atk_target_name})
if(NOT TARGET SDL3_atk::SDL3_atk)
    add_library(SDL3_atk::SDL3_atk ALIAS ${sdl3_atk_target_name})
endif()
target_include_directories(${sdl3_atk_target_name}
    PUBLIC
        "$<BUILD_INTERFACE:${SDL3_ATK_INC}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_compile_definitions(${sdl3_atk_target_name} PRIVATE
    BUILD_SDL
    SDL_BUILD_MAJOR_VERSION=${MAJOR_VERSION}
    SDL_BUILD_MINOR_VERSION=${MINOR_VERSION}
    SDL_BUILD_MICRO_VERSION=${MICRO_VERSION}
)
target_link_libraries(${sdl3_atk_target_name} PUBLIC SDL3::Headers)
if(SDLATK_BUILD_SHARED_LIBS)
    target_link_libraries(${sdl3_atk_target_name} PRIVATE SDL3::SDL3-shared)
endif()

# Use SDL warnings, but disable the more problematic ones
sdl_add_warning_options(${sdl3_atk_target_name} WARNING_AS_ERROR ${SDLATK_WERROR})
if(MSVC)
    target_compile_options(${sdl3_atk_target_name}  PRIVATE /wd4100) # 'unreferenced formal parameter'
else()
    target_compile_options(${sdl3_atk_target_name}  PRIVATE -Wno-unused-parameter -Wno-unused-variable -Wno-unknown-pragmas)
endif()

if ("c_std_99" IN_LIST CMAKE_C_COMPILE_FEATURES)
    target_compile_features(${sdl3_atk_target_name} PRIVATE c_std_99)
else()
    message(WARNING "target_compile_features does not know c_std_99 for C compiler")
endif()
if(WIN32 AND SDLATK_BUILD_SHARED_LIBS)
    target_sources(${sdl3_atk_target_name} PRIVATE
        ${SDL_ATK_SRC}/version.rc
    )
    if(MINGW)
        target_link_options(${sdl3_atk_target_name} PRIVATE -static-libgcc)
    endif()
endif()
set_target_properties(${sdl3_atk_target_name} PROPERTIES
    OUTPUT_NAME SDL3_atk
    DEFINE_SYMBOL DLL_EXPORT
    EXPORT_NAME ${sdl3_atk_target_name}
    C_VISIBILITY_PRESET "hidden"
)
if(NOT ANDROID)
    set_target_properties(${sdl3_atk_target_name} PROPERTIES
        SOVERSION "${SO_VERSION_MAJOR}"
        VERSION "${SO_VERSION}"
    )
    if(APPLE)
        cmake_minimum_required(VERSION 3.17...3.28)
        set_target_properties(${sdl3_atk_target_name} PROPERTIES
            MACHO_COMPATIBILITY_VERSION "${DYLIB_COMPAT_VERSION}"
            MACHO_CURRENT_VERSION "${DYLIB_CURRENT_VERSION}"
        )
    endif()
endif()
if(SDLATK_BUILD_SHARED_LIBS)
    if(WIN32)
        set_target_properties(${sdl3_atk_target_name} PROPERTIES
            PREFIX ""
        )
    endif()
else()
    if(MSVC)
        set_target_properties(${sdl3_atk_target_name} PROPERTIES
            OUTPUT_NAME "SDL3_atk-static"
        )
    endif()
endif()

if(SDLATK_BUILD_SHARED_LIBS)
    # Use `Compatible Interface Properties` to ensure a shared SDL3_atk is linked to a shared SDL3 library
    set_property(TARGET ${sdl3_atk_target_name} PROPERTY INTERFACE_SDL3_SHARED ${SDLATK_BUILD_SHARED_LIBS})
    set_property(TARGET ${sdl3_atk_target_name} APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
endif()

if(SDLATK_BUILD_SHARED_LIBS)
    sdl_target_link_options_no_undefined(${sdl3_atk_target_name})
endif()

sdl_target_link_option_version_file(${sdl3_atk_target_name} "${SDL3_ATK_SRC}/SDL_atk.sym")

if(SDLATK_BUILD_SHARED_LIBS)
    # Make sure static library dependencies are built with -fPIC when building a shared SDL3_atk
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(INSTALL_EXTRA_TARGETS)
set(PC_LIBS)
set(PC_REQUIRES)
set(SDLIMAGE_BACKENDS)

# Build all other libraries as a static library
set(BUILD_SHARED_LIBS OFF)

if(SDLATK_FLAC OR SDLATK_VORB)
    if(SDLATK_OGG_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libogg")
        sdl_check_project_in_subfolder(external/ogg ogg SDL2ATK_VENDORED)

		add_subdirectory(external/ogg EXCLUDE_FROM_ALL)

		list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:ogg>)
    else()
        message(STATUS "${PROJECT_NAME}: Using system libogg")
        find_package(Ogg ${LIBOGG_MINIMUM_VERSION} REQUIRED)
        list(APPEND PC_REQUIRES libogg)
    endif()
	target_link_libraries(${sdl3_atk_target_name} PRIVATE Ogg::ogg)
endif()

list(APPEND SDLATK_BACKENDS OGG)
list(APPEND SDLATK_BACKENDS FLAC)
if(SDLATK_FLAC)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE LOAD_FLAC)
    if(SDLATK_FLAC_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libFLAC")
        sdl_check_project_in_subfolder(external/flac FLAC SDL2ATK_VENDORED)

        option(BUILD_CXXLIBS "Build libFLAC++" OFF)
		option(BUILD_PROGRAMS "Build and install programs" OFF)
		option(BUILD_EXAMPLES "Build and install examples" OFF)
		option(BUILD_TESTING "Build tests" OFF)
		option(BUILD_DOCS "Build and install doxygen documents" OFF)
		option(WITH_STACK_PROTECTOR "Enable GNU GCC stack smash protection" OFF)
        option(WITH_FORTIFY_SOURCE "Enable protection against buffer overflows" OFF)
		option(INSTALL_MANPAGES "Install MAN pages" OFF)
		option(INSTALL_PKGCONFIG_MODULES "Install PkgConfig modules" OFF)
        add_subdirectory(external/flac EXCLUDE_FROM_ALL)

        list(APPEND INSTALL_EXTRA_TARGETS FLAC)
		list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:FLAC>)
    else()
        message(STATUS "${PROJECT_NAME}: Using system libFLAC")
        find_package(FLAC ${LIBFLAC_MINIMUM_VERSION} REQUIRED)
        list(APPEND PC_REQUIRES libFLAC)
    endif()
	target_link_libraries(${sdl3_atk_target_name} PRIVATE FLAC)
    set(SDLATK_OGG_ENABLED TRUE)
    set(SDLATK_FLAC_ENABLED TRUE)
endif()

if(SDLATK_FLAC_SAVE)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE SAVE_FLAC)
endif()

list(APPEND SDLATK_BACKENDS VORB)
if(SDLATK_VORB)
	set(CMAKE_POLICY_DEFAULT_CMP0048 OLD)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE LOAD_VORB)
    if(SDLATK_VORB_VENDORED)
        message(STATUS "${PROJECT_NAME}: Using vendored libvorbis")
        sdl_check_project_in_subfolder(external/vorbis vorbis SDL2ATK_VENDORED)

        add_subdirectory(external/vorbis EXCLUDE_FROM_ALL)
        list(APPEND INSTALL_EXTRA_TARGETS vorbis)
        list(APPEND INSTALL_EXTRA_TARGETS vorbisenc)
        list(APPEND INSTALL_EXTRA_TARGETS vorbisfile)
		list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:vorbis>)
		list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:vorbisenc>)
		list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:vorbisfile>)
    else()
        message(STATUS "${PROJECT_NAME}: Using system libvorbis")
        find_package(Vorbis ${LIBVORBIS_MINIMUM_VERSION} REQUIRED  COMPONENTS vorbis vorbisfile vorbisenc)
        list(APPEND PC_REQUIRES libvorbis)
        list(APPEND PC_REQUIRES libvorbisenc)
        list(APPEND PC_REQUIRES libvorbisencfile)
    endif()
	target_link_libraries(${sdl3_atk_target_name} PRIVATE vorbis)
	target_link_libraries(${sdl3_atk_target_name} PRIVATE vorbisenc)
	target_link_libraries(${sdl3_atk_target_name} PRIVATE vorbisfile)
    set(SDLATK_OGG_ENABLED TRUE)
    set(SDLATK_VORB_ENABLED TRUE)
endif()

if(SDLATK_VORB_SAVE)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE SAVE_VORB)
endif()

list(APPEND SDLATK_BACKENDS MP3)
if(SDLATK_MP3)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE LOAD_MP3)
    target_include_directories(${sdl3_atk_target_name} PUBLIC
    	"$<BUILD_INTERFACE:${MMP3_DIR}>"
	    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>"
	)
    set(SDLATK_MP3_ENABLED TRUE)
endif()

if(SDLATK_WAV)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE LOAD_WAV)
endif()

if(SDLATK_WAV_SAVE)
    target_compile_definitions(${sdl3_atk_target_name} PRIVATE SAVE_WAV)
endif()

if(SDLATK_KISSFFT_VENDORED)
	message(STATUS "${PROJECT_NAME}: Using vendored libkissfft")
	sdl_check_project_in_subfolder(external/kissfft kissfft SDL2ATK_VENDORED)

	option(KISSFFT_TEST "Build kissfft tests" OFF)
	option(KISSFFT_TOOLS "Build kissfft command-line tools" OFF)
	option(KISSFFT_STATIC "Build kissfft as static (ON) or shared library (OFF)" ON)
	add_subdirectory(external/kissfft EXCLUDE_FROM_ALL)
	list(APPEND INSTALL_EXTRA_TARGETS kissfft)
	list(APPEND PC_LIBS -l$<TARGET_FILE_BASE_NAME:kissfft>)
else()
	message(STATUS "${PROJECT_NAME}: Using system libkissfft")
	find_package(KissFFT REQUIRED)
	list(APPEND PC_REQUIRES libkissfft)
endif()
target_link_libraries(${sdl3_atk_target_name} PRIVATE kissfft)

# Remove this annoying warning
if(APPLE)
	target_link_options(${sdl3_atk_target_name} PRIVATE "-Wl,-no_warn_duplicate_libraries")
  	get_target_property(_opts ${sdl3_atk_target_name} LINK_OPTIONS)
  	if(_opts)
    	list(FILTER _opts EXCLUDE REGEX "^-Wl,-undefined,error$")
  		set_target_properties(${sdl3_atk_target_name} PROPERTIES LINK_OPTIONS "${_opts}")
	endif()
endif()

# Restore BUILD_SHARED_LIBS variable
set(BUILD_SHARED_LIBS ${SDLATK_BUILD_SHARED_LIBS})

if(SDLATK_INSTALL)
    install(
        TARGETS ${sdl3_atk_target_name}
        EXPORT SDL3_atkTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT devel
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT library
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT library
    )
    install(FILES
        "${SDL_ATK_INC}/SDL3_ttf/SDL_atk.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_audio.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_codec.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_dsp.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_error.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_file.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_math.h"
        "${SDL_ATK_INC}/SDL3_ttf/ATK_rand.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL3_atk" COMPONENT devel
    )
    if(BUILD_SHARED_LIBS)
        set(pdbdir "${CMAKE_INSTALL_BINDIR}")
    else()
        set(pdbdir "${CMAKE_INSTALL_LIBDIR}")
    endif()
    if(MSVC)
        SDL_install_pdb("${sdl3_atk_target_name}" "${pdbdir}")
    endif()

    if(WIN32 AND NOT MINGW)
        set(SDLATK_INSTALL_CMAKEDIR_DEFAULT "cmake")
    else()
        set(SDATK_INSTALL_CMAKEDIR_DEFAULT "${CMAKE_INSTALL_LIBDIR}/cmake")
    endif()
    set(SDLATK_INSTALL_CMAKEDIR_ROOT "${SDLATK_INSTALL_CMAKEDIR_DEFAULT}" CACHE STRING "Location where to install SDL3_atkConfig.cmake")
    set(SDLATK_PKGCONFIG_INSTALLDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

    if(WIN32 AND NOT MINGW)
        set(SDLATK_INSTALL_CMAKEDIR "${SDLATK_INSTALL_CMAKEDIR_ROOT}")
        set(LICENSES_PREFIX "licenses/SDL3_atk")
    else()
        set(SDLATK_INSTALL_CMAKEDIR "${SDLATK_INSTALL_CMAKEDIR_ROOT}/SDL3_atk")
        set(LICENSES_PREFIX "${CMAKE_INSTALL_DATAROOTDIR}/licenses/SDL3_atk")
    endif()

    configure_package_config_file(modules/SDL3_atkConfig.cmake.in SDL3_atkConfig.cmake
        NO_SET_AND_CHECK_MACRO
        INSTALL_DESTINATION "${SDLATK_INSTALL_CMAKEDIR}"
    )
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/SDL3_atkConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_atkConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/SDL3_atkConfigVersion.cmake"
        DESTINATION ${SDLATK_INSTALL_CMAKEDIR}
        COMPONENT devel
    )
    if(NOT SDLATK_VENDORED)
      install(
          FILES
              modules/PkgConfigHelper.cmake
              modules/FindFlac.cmake
              modules/FindOgg.cmake
              modules/FindVorbis.cmake
              modules/FindKissFFT.cmake
          DESTINATION "${SDLATK_INSTALL_CMAKEDIR}"
          COMPONENT devel
      )
    endif()
    install(EXPORT SDL3_atkTargets
        FILE ${sdl3_atk_target_name}-targets.cmake
        NAMESPACE SDL3_atk::
        DESTINATION "${SDLATK_INSTALL_CMAKEDIR}"
        COMPONENT devel
    )

    export(TARGETS ${sdl3_atk_target_name} NAMESPACE "SDL3_atk::" FILE "${sdl3_atk_target_name}-targets.cmake")

    if(SDLATK_RELOCATABLE)
      file(RELATIVE_PATH SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${CMAKE_INSTALL_PREFIX}/${SDLATK_PKGCONFIG_INSTALLDIR}" "${CMAKE_INSTALL_PREFIX}")
      string(REGEX REPLACE "[/]+$" "" SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG "${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
      set(SDL_PKGCONFIG_PREFIX "\${pcfiledir}/${SDL_PATH_PREFIX_RELATIVE_TO_PKGCONFIG}")
    else()
      set(SDL_PKGCONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif()

    if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
      set(INCLUDEDIR_FOR_PKG_CONFIG "${CMAKE_INSTALL_INCLUDEDIR}")
    else()
      set(INCLUDEDIR_FOR_PKG_CONFIG "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif()
    if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
      set(LIBDIR_FOR_PKG_CONFIG "${CMAKE_INSTALL_LIBDIR}")
    else()
      set(LIBDIR_FOR_PKG_CONFIG "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    string(JOIN " " PC_REQUIRES ${PC_REQUIRES})
    string(JOIN " " PC_LIBS ${PC_LIBS})
    configure_file(modules/sdl3-atk.pc.in sdl3-atk.pc @ONLY)

    # Always install sdl3-atk.pc file: libraries might be different between config modes
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdl3-atk.pc"
        DESTINATION "${SDLATK_PKGCONFIG_INSTALLDIR}" COMPONENT devel)

    install(FILES "LICENSE.txt"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
        COMPONENT library
    )

    if(SDLATK_INSTALL_CPACK)
        if(MSVC)
            set(CPACK_GENERATOR "ZIP")
        else()
            set(CPACK_GENERATOR "TGZ")
        endif()
        configure_file(modules/CPackProjectConfig.cmake.in CPackProjectConfig.cmake @ONLY)
        set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/CPackProjectConfig.cmake")
        # CPACK_SOURCE_PACKAGE_FILE_NAME must end with "-src" (so we can block creating a source archive)
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-src")
        set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
        include(CPack)
    endif()

endif()

if(SDLATK_TESTS)
	set(SDL_ATK_SRC_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
    enable_testing()
    add_subdirectory(test)
endif()

set(available_deps)
set(unavailable_deps)
foreach(dep IN LISTS SDLATK_BACKENDS)
  set(var SDLATK_${dep}_ENABLED)
  if(NOT DEFINED ${var})
    message(AUTHOR_WARNING "${var} not defined")
  endif()
  if(${var})
    list(APPEND available_deps ${dep})
  else()
    list(APPEND unavailable_deps ${dep})
  endif()
endforeach()
string(JOIN " " avail_str ${available_deps})
string(TOLOWER "${avail_str}" avail_str)
string(JOIN " " unavail_str ${unavailable_deps})
string(TOLOWER "${unavail_str}" unavail_str)
message(STATUS "SDL3_atk backends:")
message(STATUS "- enabled:  ${avail_str}")
message(STATUS "- disabled: ${unavail_str}")
