#.rst:
# FindOgg
# -------
# Find the Ogg library (Xiph.org).
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#   Ogg::ogg
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#   OGG_FOUND
#   OGG_INCLUDE_DIRS
#   OGG_LIBRARIES
#   OGG_VERSION
#
# Hints
# ^^^^^
# Respects pkg-config if present. Otherwise searches common locations.

include_guard(GLOBAL)

include(CMakeFindDependencyMacro)
include(FetchContent) # harmless if unused; avoids policy noise
find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_OGG QUIET ogg)
endif()

# Headers
find_path(OGG_INCLUDE_DIR
  NAMES ogg/ogg.h
  HINTS
    ${PC_OGG_INCLUDEDIR} ${PC_OGG_INCLUDE_DIRS}
  PATH_SUFFIXES include
)

# Library
find_library(OGG_LIBRARY
  NAMES ogg libogg ogg_static
  HINTS
    ${PC_OGG_LIBDIR} ${PC_OGG_LIBRARY_DIRS}
)

# Version
set(OGG_VERSION "")
if(PC_OGG_VERSION)
  set(OGG_VERSION "${PC_OGG_VERSION}")
else()
  # Try to parse from ogg/ogg.h (fallback, may be empty)
  if(OGG_INCLUDE_DIR AND EXISTS "${OGG_INCLUDE_DIR}/ogg/ogg.h")
    file(STRINGS "${OGG_INCLUDE_DIR}/ogg/ogg.h" _ogg_ver_line
         REGEX "#define[ \t]+OGG_VERSION_STRING[ \t]+\"[^\"]+\"")
    if(_ogg_ver_line)
      string(REGEX REPLACE ".*OGG_VERSION_STRING[ \t]+\"([^\"]+)\".*" "\\1" OGG_VERSION "${_ogg_ver_line}")
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg
  REQUIRED_VARS OGG_INCLUDE_DIR OGG_LIBRARY
  VERSION_VAR OGG_VERSION)

if(OGG_FOUND)
  set(OGG_INCLUDE_DIRS "${OGG_INCLUDE_DIR}")
  set(OGG_LIBRARIES "${OGG_LIBRARY}")

  if(NOT TARGET Ogg::ogg)
    add_library(Ogg::ogg UNKNOWN IMPORTED)
    set_target_properties(Ogg::ogg PROPERTIES
      IMPORTED_LOCATION "${OGG_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIRS}")
  endif()
endif()
