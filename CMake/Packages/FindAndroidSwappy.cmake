#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find AndroidSwappy
# Once done, this will define
#
#  AndroidSwappy_FOUND - system has AndroidSwappy
#  AndroidSwappy_INCLUDE_DIRS - the AndroidSwappy include directories
#  AndroidSwappy_LIBRARIES - link these to use AndroidSwappy

include(FindPkgMacros)
findpkg_begin(AndroidSwappy)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_HOME)

# construct search paths
set(AndroidSwappy_PREFIX_PATH ${OGRE_SOURCE}/Dependencies ${ENV_OGRE_SOURCE}/Dependencies ${OGRE_HOME} ${ENV_OGRE_HOME})

create_search_paths(AndroidSwappy)

# redo search if prefix path changed
clear_if_changed(AndroidSwappy_PREFIX_PATH
  AndroidSwappy_LIBRARY_FWK
  AndroidSwappy_LIBRARY_REL
  AndroidSwappy_LIBRARY_DBG
  AndroidSwappy_INCLUDE_DIR
)

get_debug_names(AndroidSwappy_LIBRARY_NAMES)

use_pkgconfig(AndroidSwappy_PKGC AndroidSwappy)

findpkg_framework(AndroidSwappy)

find_path(AndroidSwappy_INCLUDE_DIR NAMES swappyVk.h HINTS ${AndroidSwappy_FRAMEWORK_INCLUDES} ${AndroidSwappy_INC_SEARCH_PATH} ${AndroidSwappy_PKGC_INCLUDE_DIRS} PATH_SUFFIXES "swappy" "swappy/swappy")
find_library(AndroidSwappy_LIBRARY NAMES swappy_static HINTS ${AndroidSwappy_LIB_SEARCH_PATH} ${AndroidSwappy_PKGC_LIBRARY_DIRS} PATH_SUFFIXES "" release relwithdebinfo minsizerel debug)

findpkg_finish(AndroidSwappy)
add_parent_dir(AndroidSwappy_INCLUDE_DIRS AndroidSwappy_INCLUDE_DIR)
