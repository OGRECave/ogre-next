#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find AMDAGS
# Once done, this will define
#
#  AMDAGS_FOUND - system has AMDAGS
#  AMDAGS_INCLUDE_DIRS - the AMDAGS include directories
#  AMDAGS_LIBRARIES - link these to use AMDAGS

include(FindPkgMacros)
findpkg_begin(AMDAGS)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_HOME)

# construct search paths
set(AMDAGS_PREFIX_PATH ${OGRE_SOURCE}/Dependencies ${ENV_OGRE_SOURCE}/Dependencies ${OGRE_HOME} ${ENV_OGRE_HOME})

create_search_paths(AMDAGS)

# redo search if prefix path changed
clear_if_changed(AMDAGS_PREFIX_PATH
  AMDAGS_LIBRARY_FWK
  AMDAGS_LIBRARY_REL
  AMDAGS_LIBRARY_DBG
  AMDAGS_INCLUDE_DIR
)

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set(AMDAGS_LIBRARY_NAMES amd_ags_x64)
    set(AMDAGS_BINARY_NAMES amd_ags_x64)
else()
    set(AMDAGS_LIBRARY_NAMES amd_ags_x86)
    set(AMDAGS_BINARY_NAMES amd_ags_x86)
endif()
get_debug_names(AMDAGS_LIBRARY_NAMES)

use_pkgconfig(AMDAGS_PKGC AMDAGS)

findpkg_framework(AMDAGS)

find_path(AMDAGS_INCLUDE_DIR NAMES amd_ags.h HINTS ${AMDAGS_FRAMEWORK_INCLUDES} ${AMDAGS_INC_SEARCH_PATH} ${AMDAGS_PKGC_INCLUDE_DIRS} PATH_SUFFIXES AMDAGS)
find_library(AMDAGS_LIBRARY_REL NAMES ${AMDAGS_LIBRARY_NAMES} HINTS ${AMDAGS_LIB_SEARCH_PATH} ${AMDAGS_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
find_library(AMDAGS_LIBRARY_DBG NAMES ${AMDAGS_LIBRARY_NAMES_DBG} HINTS ${AMDAGS_LIB_SEARCH_PATH} ${AMDAGS_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
make_library_set(AMDAGS_LIBRARY)

set(AMDAGS_BIN_SEARCH_PATH ${OGRE_DEPENDENCIES_DIR}/bin ${CMAKE_SOURCE_DIR}/Dependencies/bin
	${ENV_OGRE_DEPENDENCIES_DIR}/bin
	${OGRE_SOURCE}/Dependencies/bin ${ENV_OGRE_SOURCE}/Dependencies/bin
	${OGRE_SDK}/bin ${ENV_OGRE_SDK}/bin
	${OGRE_HOME}/bin ${ENV_OGRE_HOME}/bin)
find_file(AMDAGS_BINARY_REL NAMES "${AMDAGS_BINARY_NAMES}.dll" HINTS ${AMDAGS_BIN_SEARCH_PATH} ${AMDAGS_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" Release RelWithDebInfo MinSizeRel Debug )
set(AMDAGS_BINARY_DBG ${AMDAGS_BINARY_REL})

make_library_set(AMDAGS_BINARY)

findpkg_finish(AMDAGS)
add_parent_dir(AMDAGS_INCLUDE_DIRS AMDAGS_INCLUDE_DIR)
