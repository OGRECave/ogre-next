#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find DearImgui
# Once done, this will define
#
#  DearImgui_FOUND - system has DearImgui
#  DearImgui_INCLUDE_DIRS - the DearImgui include directories

include(FindPackageHandleStandardArgs)
include(FindPkgMacros)
findpkg_begin(DearImgui)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(DearImgui_HOME)
getenv_path(OGRE_SDK)
getenv_path(OGRE_HOME)
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_DEPENDENCIES_DIR)

# construct search paths
set(DearImgui_PREFIX_PATH ${DearImgui_HOME} ${ENV_DearImgui_HOME}
  ${OGRE_DEPENDENCIES_DIR} ${ENV_OGRE_DEPENDENCIES_DIR}
  ${OGRE_SOURCE}/iOSDependencies ${ENV_OGRE_SOURCE}/iOSDependencies
  ${OGRE_SOURCE}/Dependencies )
create_search_paths(DearImgui)
# redo search if prefix path changed
clear_if_changed(DearImgui_PREFIX_PATH
  DearImgui_INCLUDE_DIR
)

use_pkgconfig(DearImgui_PKGC DearImgui)

# For DearImgui, prefer static library over framework (important when referencing DearImgui source build)
set(CMAKE_FIND_FRAMEWORK "LAST")

findpkg_framework(DearImgui)

find_path(DearImgui_INCLUDE_DIR NAMES imgui.h
  HINTS ${DearImgui_INC_SEARCH_PATH} ${DearImgui_PKGC_INCLUDE_DIRS}
  PATH_SUFFIXES imgui
)

find_package_handle_standard_args( DearImgui DEFAULT_MSG DearImgui_INCLUDE_DIR )

if( DearImgui_INCLUDE_DIR )
    set( DearImgui_FOUND TRUE )
    set( DearImgui_INCLUDE_DIRS ${DearImgui_INCLUDE_DIR} )
endif()

findpkg_finish(DearImgui)

# Reset framework finding
set(CMAKE_FIND_FRAMEWORK "FIRST")
