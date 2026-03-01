#.rst:
# FindVorbis
# ----------
# Find the Vorbis libraries (Xiph.org).
#
# Components:
#   vorbis (core) [default], vorbisfile, vorbisenc
#
# Imported Targets
#   Vorbis::vorbis
#   Vorbis::vorbisfile (if component found/asked)
#   Vorbis::vorbisenc  (if component found/asked)
#
# Result Variables
#   VORBIS_FOUND
#   VORBIS_INCLUDE_DIRS
#   VORBIS_LIBRARIES
#   VORBIS_VERSION

include_guard(GLOBAL)
find_package(PkgConfig QUIET)

# Determine components
set(_vorbis_comps vorbis)
if(Vorbis_FIND_COMPONENTS)
  list(APPEND _vorbis_comps ${Vorbis_FIND_COMPONENTS})
else()
  # default: core only
endif()
list(REMOVE_DUPLICATES _vorbis_comps)

# Try pkg-config (separately so we can get include/lib dirs)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_VORBIS QUIET vorbis)
  pkg_check_modules(PC_VORBISFILE QUIET vorbisfile)
  pkg_check_modules(PC_VORBISENC QUIET vorbisenc)
endif()

# Common include dir (codec.h)
find_path(VORBIS_INCLUDE_DIR
  NAMES vorbis/codec.h
  HINTS ${PC_VORBIS_INCLUDEDIR} ${PC_VORBIS_INCLUDE_DIRS} ${PC_VORBISFILE_INCLUDE_DIRS} ${PC_VORBISENC_INCLUDE_DIRS}
)

# Core lib
find_library(VORBIS_VORBIS_LIBRARY
  NAMES vorbis libvorbis vorbis_static
  HINTS ${PC_VORBIS_LIBDIR} ${PC_VORBIS_LIBRARY_DIRS}
)

# vorbisfile (optional)
if("vorbisfile" IN_LIST _vorbis_comps)
  find_library(VORBIS_VORBISFILE_LIBRARY
    NAMES vorbisfile libvorbisfile vorbisfile_static
    HINTS ${PC_VORBISFILE_LIBDIR} ${PC_VORBISFILE_LIBRARY_DIRS} ${PC_VORBIS_LIBDIR} ${PC_VORBIS_LIBRARY_DIRS}
  )
endif()

# vorbisenc (optional)
if("vorbisenc" IN_LIST _vorbis_comps)
  find_library(VORBIS_VORBISENC_LIBRARY
    NAMES vorbisenc libvorbisenc vorbisenc_static
    HINTS ${PC_VORBISENC_LIBDIR} ${PC_VORBISENC_LIBRARY_DIRS} ${PC_VORBIS_LIBDIR} ${PC_VORBIS_LIBRARY_DIRS}
  )
endif()

# Version (prefer pkg-config)
set(VORBIS_VERSION "")
foreach(_pcver IN ITEMS ${PC_VORBIS_VERSION} ${PC_VORBISFILE_VERSION} ${PC_VORBISENC_VERSION})
  if(_pcver AND NOT VORBIS_VERSION)
    set(VORBIS_VERSION "${_pcver}")
  endif()
endforeach()

include(FindPackageHandleStandardArgs)
# Required vars depend on requested components
set(_vorbis_required VORBIS_INCLUDE_DIR VORBIS_VORBIS_LIBRARY)
if("vorbisfile" IN_LIST _vorbis_comps)
  list(APPEND _vorbis_required VORBIS_VORBISFILE_LIBRARY)
endif()
if("vorbisenc" IN_LIST _vorbis_comps)
  list(APPEND _vorbis_required VORBIS_VORBISENC_LIBRARY)
endif()

find_package_handle_standard_args(Vorbis
  REQUIRED_VARS ${_vorbis_required}
  VERSION_VAR VORBIS_VERSION)

if(VORBIS_FOUND)
  set(VORBIS_INCLUDE_DIRS "${VORBIS_INCLUDE_DIR}")
  set(VORBIS_LIBRARIES "${VORBIS_VORBIS_LIBRARY}")

  if(NOT TARGET Vorbis::vorbis)
    add_library(Vorbis::vorbis UNKNOWN IMPORTED)
    set_target_properties(Vorbis::vorbis PROPERTIES
      IMPORTED_LOCATION "${VORBIS_VORBIS_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIRS}")
  endif()

  if(VORBIS_VORBISFILE_LIBRARY)
    if(NOT TARGET Vorbis::vorbisfile)
      add_library(Vorbis::vorbisfile UNKNOWN IMPORTED)
      set_target_properties(Vorbis::vorbisfile PROPERTIES
        IMPORTED_LOCATION "${VORBIS_VORBISFILE_LIBRARY}"
        INTERFACE_LINK_LIBRARIES Vorbis::vorbis
        INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIRS}")
    endif()
    list(APPEND VORBIS_LIBRARIES "${VORBIS_VORBISFILE_LIBRARY}")
  endif()

  if(VORBIS_VORBISENC_LIBRARY)
    if(NOT TARGET Vorbis::vorbisenc)
      add_library(Vorbis::vorbisenc UNKNOWN IMPORTED)
      set_target_properties(Vorbis::vorbisenc PROPERTIES
        IMPORTED_LOCATION "${VORBIS_VORBISENC_LIBRARY}"
        INTERFACE_LINK_LIBRARIES Vorbis::vorbis
        INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIRS}")
    endif()
    list(APPEND VORBIS_LIBRARIES "${VORBIS_VORBISENC_LIBRARY}")
  endif()
endif()