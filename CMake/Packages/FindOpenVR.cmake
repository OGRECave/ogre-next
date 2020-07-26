#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenVR
# Once done, this will define
#
#  OpenVR_FOUND - system has OpenVR
#  OpenVR_INCLUDE_DIRS - the OpenVR include directories
#  OpenVR_LIBRARIES - link these to use OpenVR

include(FindPkgMacros)
findpkg_begin(OpenVR)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_HOME)

# construct search paths
set(OpenVR_PREFIX_PATH ${OGRE_SOURCE}/Dependencies ${ENV_OGRE_SOURCE}/Dependencies ${OGRE_HOME} ${ENV_OGRE_HOME})

create_search_paths(OpenVR)

# redo search if prefix path changed
clear_if_changed(OpenVR_PREFIX_PATH
  OpenVR_LIBRARY_FWK
  OpenVR_LIBRARY_REL
  OpenVR_LIBRARY_DBG
  OpenVR_INCLUDE_DIR
)

set(OpenVR_LIBRARY_NAMES openvr_api)
set(OpenVR_LIBRARY_NAMES_DBG ${OpenVR_LIBRARY_NAMES})

if(WIN32)
	set(OpenVR_BINARY_NAMES openvr_api.dll)
	set(OpenVR_BINARY_NAMES_DBG ${OpenVR_BINARY_NAMES})
else()
	set(OpenVR_BINARY_NAMES openvr_api)
	set(OpenVR_BINARY_NAMES_DBG ${OpenVR_BINARY_NAMES})
endif()

use_pkgconfig(OpenVR_PKGC OpenVR)

findpkg_framework(OpenVR)

find_path(OpenVR_INCLUDE_DIR NAMES openvr.h HINTS ${OpenVR_FRAMEWORK_INCLUDES} ${OpenVR_INC_SEARCH_PATH} ${OpenVR_PKGC_INCLUDE_DIRS} PATH_SUFFIXES OpenVR)
find_library(OpenVR_LIBRARY_REL NAMES ${OpenVR_LIBRARY_NAMES} HINTS ${OpenVR_LIB_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
find_library(OpenVR_LIBRARY_DBG NAMES ${OpenVR_LIBRARY_NAMES_DBG} HINTS ${OpenVR_LIB_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
make_library_set(OpenVR_LIBRARY)

if(WIN32)
	set(OpenVR_BIN_SEARCH_PATH ${OGRE_DEPENDENCIES_DIR}/bin ${CMAKE_SOURCE_DIR}/Dependencies/bin ${OPENVR_HOME}/dll
		${ENV_OPENVR_HOME}/dll ${ENV_OGRE_DEPENDENCIES_DIR}/bin
		${OGRE_SOURCE}/Dependencies/bin ${ENV_OGRE_SOURCE}/Dependencies/bin
		${OGRE_SDK}/bin ${ENV_OGRE_SDK}/bin
		${OGRE_HOME}/bin ${ENV_OGRE_HOME}/bin)

	find_file(OpenVR_BINARY_REL NAMES ${OpenVR_BINARY_NAMES} HINTS ${OpenVR_BIN_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
	find_file(OpenVR_BINARY_DBG NAMES ${OpenVR_BINARY_NAMES} HINTS ${OpenVR_BIN_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
else()
	find_library(OpenVR_BINARY_REL NAMES ${OpenVR_BINARY_NAMES} HINTS ${OpenVR_LIB_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel)
	find_library(OpenVR_BINARY_DBG NAMES ${OpenVR_BINARY_NAMES_DBG} HINTS ${OpenVR_LIB_SEARCH_PATH} ${OpenVR_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" debug)
endif()

make_library_set(OpenVR_BINARY)

findpkg_finish(OpenVR)
add_parent_dir(OpenVR_INCLUDE_DIRS OpenVR_INCLUDE_DIR)
