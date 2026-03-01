#.rst:
# FindKissFFT
# -----------
# Find the KissFFT library (https://github.com/mborgerding/kissfft).
#
# Components:
#   float  (default)
#   double
#
# Imported Targets:
#   KissFFT::kissfft         (float)
#   KissFFT::kissfft_double  (double)
#
# Result Variables:
#   KISSFFT_FOUND
#   KISSFFT_INCLUDE_DIRS
#   KISSFFT_LIBRARIES
#   KISSFFT_VERSION
#
# Notes:
# - Tries pkg-config modules: kissfft, kissfft-float, kissfft-double.
# - Handles both header layouts: "kissfft/kiss_fft.h" and plain "kiss_fft.h".
# - On Windows, works with both static and import libraries.

include_guard(GLOBAL)
cmake_policy(PUSH)
# No special policies required; reserved for future compatibility.
cmake_policy(POP)

include(FindPackageHandleStandardArgs)
find_package(PkgConfig QUIET)

# Normalize requested components
set(_kiss_components float)
if(KissFFT_FIND_COMPONENTS)
  set(_kiss_components ${KissFFT_FIND_COMPONENTS})
endif()
string(TOLOWER "${_kiss_components}" _kiss_components)
list(REMOVE_DUPLICATES _kiss_components)

# --- pkg-config probes -------------------------------------------------------
if(PKG_CONFIG_FOUND)
  # generic
  pkg_check_modules(PC_KISSFFT      QUIET kissfft)
  # split variants seen on some distros
  pkg_check_modules(PC_KISSFFT_F    QUIET kissfft-float)
  pkg_check_modules(PC_KISSFFT_D    QUIET kissfft-double)
endif()

# Collect common include hints
set(_KISS_INCLUDE_HINTS
  ${PC_KISSFFT_INCLUDEDIR} ${PC_KISSFFT_INCLUDE_DIRS}
  ${PC_KISSFFT_F_INCLUDEDIR} ${PC_KISSFFT_F_INCLUDE_DIRS}
  ${PC_KISSFFT_D_INCLUDEDIR} ${PC_KISSFFT_D_INCLUDE_DIRS}
)

# Headers: accept both layouts
find_path(KISSFFT_INCLUDE_DIR
  NAMES kissfft/kiss_fft.h kiss_fft.h
  HINTS ${_KISS_INCLUDE_HINTS}
)

# Determine version, if any
set(KISSFFT_VERSION "")
foreach(_v IN ITEMS
  ${PC_KISSFFT_VERSION} ${PC_KISSFFT_F_VERSION} ${PC_KISSFFT_D_VERSION})
  if(_v AND NOT KISSFFT_VERSION)
    set(KISSFFT_VERSION "${_v}")
  endif()
endforeach()

# Helper to locate a lib with many possible names
function(_kiss_find_lib outvar)
  set(names ${ARGN})
  find_library(${outvar}
    NAMES ${names}
    HINTS
      ${PC_KISSFFT_LIBDIR} ${PC_KISSFFT_LIBRARY_DIRS}
      ${PC_KISSFFT_F_LIBDIR} ${PC_KISSFFT_F_LIBRARY_DIRS}
      ${PC_KISSFFT_D_LIBDIR} ${PC_KISSFFT_D_LIBRARY_DIRS}
  )
endfunction()

# Library name variants seen across platforms/packagers
set(_kiss_names_float
  kissfft-float kissfft_float kissfft
  libkissfft-float libkissfft_float libkissfft
)
set(_kiss_names_double
  kissfft-double kissfft_double
  libkissfft-double libkissfft_double
)

# If only a unified "kissfft" exists, treat it as float unless double explicitly found.
_kiss_find_lib(KISSFFT_FLOAT_LIBRARY  ${_kiss_names_float})
_kiss_find_lib(KISSFFT_DOUBLE_LIBRARY ${_kiss_names_double})

# Handle component requirements
set(_kiss_required KISSFFT_INCLUDE_DIR)
foreach(_c IN LISTS _kiss_components)
  if(_c STREQUAL "float")
    list(APPEND _kiss_required KISSFFT_FLOAT_LIBRARY)
  elseif(_c STREQUAL "double")
    list(APPEND _kiss_required KISSFFT_DOUBLE_LIBRARY)
  else()
    message(VERBOSE "FindKissFFT: unknown component '${_c}' ignored")
  endif()
endforeach()

# If user asked for double but only generic "kissfft" is present and DOUBLE not found,
# keep it missing (better to be explicit). If user asked only for float and generic lib is present, it's fine.

find_package_handle_standard_args(KissFFT
  REQUIRED_VARS ${_kiss_required}
  VERSION_VAR KISSFFT_VERSION)

if(KISSFFT_FOUND)
  set(KISSFFT_INCLUDE_DIRS "${KISSFFT_INCLUDE_DIR}")
  set(KISSFFT_LIBRARIES "")

  # Float target
  if(KISSFFT_FLOAT_LIBRARY)
    if(NOT TARGET KissFFT::kissfft)
      add_library(KissFFT::kissfft UNKNOWN IMPORTED)
      set_target_properties(KissFFT::kissfft PROPERTIES
        IMPORTED_LOCATION "${KISSFFT_FLOAT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${KISSFFT_INCLUDE_DIRS}")
    endif()
    if("float" IN_LIST _kiss_components OR NOT KissFFT_FIND_COMPONENTS)
      list(APPEND KISSFFT_LIBRARIES "${KissFFT_FLOAT_LIBRARY}")
    endif()
  endif()

  # Double target
  if(KISSFFT_DOUBLE_LIBRARY)
    if(NOT TARGET KissFFT::kissfft_double)
      add_library(KissFFT::kissfft_double UNKNOWN IMPORTED)
      set_target_properties(KissFFT::kissfft_double PROPERTIES
        IMPORTED_LOCATION "${KISSFFT_DOUBLE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${KISSFFT_INCLUDE_DIRS}")
    endif()
    if("double" IN_LIST _kiss_components)
      list(APPEND KISSFFT_LIBRARIES "${KISSFFT_DOUBLE_LIBRARY}")
    endif()
  endif()
endif()