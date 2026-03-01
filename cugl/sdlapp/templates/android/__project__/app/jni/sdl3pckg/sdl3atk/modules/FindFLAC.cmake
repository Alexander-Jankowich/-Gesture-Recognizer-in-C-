#.rst:
# FindFLAC
# --------
# Find the FLAC library (Xiph.org).
#
# Components:
#   FLAC (C library) [default], FLACPP (C++ bindings)
#
# Imported Targets
#   FLAC::FLAC
#   FLAC::FLACPP (if component found/asked)
#
# Result Variables
#   FLAC_FOUND
#   FLAC_INCLUDE_DIRS
#   FLAC_LIBRARIES
#   FLAC_VERSION

include_guard(GLOBAL)
find_package(PkgConfig QUIET)

set(_flac_comps FLAC)
if(FLAC_FIND_COMPONENTS)
  list(APPEND _flac_comps ${FLAC_FIND_COMPONENTS})
endif()
list(REMOVE_DUPLICATES _flac_comps)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_FLAC QUIET flac)
  pkg_check_modules(PC_FLACPP QUIET flac++)
endif()

# Headers (C)
find_path(FLAC_INCLUDE_DIR
  NAMES FLAC/stream_decoder.h FLAC/format.h
  HINTS ${PC_FLAC_INCLUDEDIR} ${PC_FLAC_INCLUDE_DIRS}
)

# Library (C)
find_library(FLAC_FLAC_LIBRARY
  NAMES FLAC libFLAC FLAC_static
  HINTS ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS}
)

# C++ headers & lib (optional)
if("FLACPP" IN_LIST _flac_comps)
  find_path(FLACPP_INCLUDE_DIR
    NAMES FLAC++/decoder.h
    HINTS ${PC_FLACPP_INCLUDEDIR} ${PC_FLACPP_INCLUDE_DIRS} ${FLAC_INCLUDE_DIR} # sometimes co-located
  )
  find_library(FLAC_FLACPP_LIBRARY
    NAMES FLAC++ libFLAC++ FLAC++_static
    HINTS ${PC_FLACPP_LIBDIR} ${PC_FLACPP_LIBRARY_DIRS} ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS}
  )
endif()

# Version
set(FLAC_VERSION "")
if(PC_FLAC_VERSION)
  set(FLAC_VERSION "${PC_FLAC_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
set(_flac_required FLAC_INCLUDE_DIR FLAC_FLAC_LIBRARY)
if("FLACPP" IN_LIST _flac_comps)
  list(APPEND _flac_required FLACPP_INCLUDE_DIR FLAC_FLACPP_LIBRARY)
endif()

find_package_handle_standard_args(FLAC
  REQUIRED_VARS ${_flac_required}
  VERSION_VAR FLAC_VERSION)

if(FLAC_FOUND)
  set(FLAC_INCLUDE_DIRS "${FLAC_INCLUDE_DIR}")
  set(FLAC_LIBRARIES "${FLAC_FLAC_LIBRARY}")

  if(NOT TARGET FLAC::FLAC)
    add_library(FLAC::FLAC UNKNOWN IMPORTED)
    set_target_properties(FLAC::FLAC PROPERTIES
      IMPORTED_LOCATION "${FLAC_FLAC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIRS}")
  endif()

  if(FLAC_FLACPP_LIBRARY)
    if(NOT TARGET FLAC::FLACPP)
      add_library(FLAC::FLACPP UNKNOWN IMPORTED)
      set_target_properties(FLAC::FLACPP PROPERTIES
        IMPORTED_LOCATION "${FLAC_FLACPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FLACPP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES FLAC::FLAC)
    endif()
    list(APPEND FLAC_LIBRARIES "${FLAC_FLACPP_LIBRARY}")
  endif()
endif()