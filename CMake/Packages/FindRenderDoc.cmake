#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find RenderDoc
# Once done, this will define
#
#  RenderDoc_FOUND - system has RenderDoc
#  RenderDoc_INCLUDE_DIRS - the RenderDoc include directories

include(FindPackageHandleStandardArgs)
include(FindPkgMacros)
findpkg_begin(RenderDoc)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(RenderDoc_HOME)
getenv_path(OGRE_SDK)
getenv_path(OGRE_HOME)
getenv_path(OGRE_SOURCE)
getenv_path(OGRE_DEPENDENCIES_DIR)

# construct search paths
set(RenderDoc_PREFIX_PATH ${RenderDoc_HOME} ${ENV_RenderDoc_HOME}
  ${OGRE_DEPENDENCIES_DIR} ${ENV_OGRE_DEPENDENCIES_DIR}
  ${OGRE_SOURCE}/iOSDependencies ${ENV_OGRE_SOURCE}/iOSDependencies
  ${OGRE_SOURCE}/Dependencies )
create_search_paths(RenderDoc)
# redo search if prefix path changed
clear_if_changed(RenderDoc_PREFIX_PATH
  RenderDoc_INCLUDE_DIR
)

use_pkgconfig(RenderDoc_PKGC RenderDoc)

# For RenderDoc, prefer static library over framework (important when referencing RenderDoc source build)
set(CMAKE_FIND_FRAMEWORK "LAST")

findpkg_framework(RenderDoc)

find_path(RenderDoc_INCLUDE_DIR NAMES renderdoc/renderdoc_app.h
	HINTS ${RenderDoc_INC_SEARCH_PATH} ${RenderDoc_PKGC_INCLUDE_DIRS})

find_package_handle_standard_args( RenderDoc DEFAULT_MSG RenderDoc_INCLUDE_DIR )

if( RenderDoc_INCLUDE_DIR )
	set( RenderDoc_FOUND TRUE )
	set( RenderDoc_INCLUDE_DIRS ${RenderDoc_INCLUDE_DIR} )
endif()

findpkg_finish(RenderDoc)

# add parent of RenderDoc folder to support RenderDoc/RenderDoc.h
#add_parent_dir(RenderDoc_INCLUDE_DIRS RenderDoc_INCLUDE_DIR)

# Reset framework finding
set(CMAKE_FIND_FRAMEWORK "FIRST")
