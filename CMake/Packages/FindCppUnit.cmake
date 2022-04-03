#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find CppUnit
# Once done, this will define
#
#  CppUnit_FOUND - system has CppUnit
#  CppUnit_INCLUDE_DIRS - the CppUnit include directories 
#  CppUnit_LIBRARIES - link these to use CppUnit

include(FindPkgMacros)
findpkg_begin(CppUnit)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(CPPUNIT_HOME)

# construct search paths
set(CppUnit_PREFIX_PATH ${CPPUNIT_HOME} ${ENV_CPPUNIT_HOME})
create_search_paths(CppUnit)
# redo search if prefix path changed
clear_if_changed(CppUnit_PREFIX_PATH
  CppUnit_LIBRARY_FWK
  CppUnit_LIBRARY_REL
  CppUnit_LIBRARY_DBG
  CppUnit_INCLUDE_DIR
)

set(CppUnit_LIBRARY_NAMES cppunit)
get_debug_names(CppUnit_LIBRARY_NAMES)

use_pkgconfig(CppUnit_PKGC cppunit)

findpkg_framework(CppUnit)

find_path(CppUnit_INCLUDE_DIR NAMES cppunit/Test.h HINTS ${CppUnit_INC_SEARCH_PATH} ${CppUnit_PKGC_INCLUDE_DIRS})
find_library(CppUnit_LIBRARY_REL NAMES ${CppUnit_LIBRARY_NAMES} HINTS ${CppUnit_LIB_SEARCH_PATH} ${CppUnit_PKGC_LIBRARY_DIRS})
find_library(CppUnit_LIBRARY_DBG NAMES ${CppUnit_LIBRARY_NAMES_DBG} HINTS ${CppUnit_LIB_SEARCH_PATH} ${CppUnit_PKGC_LIBRARY_DIRS})
make_library_set(CppUnit_LIBRARY)

findpkg_finish(CppUnit)

