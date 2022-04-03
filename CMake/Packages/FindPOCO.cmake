#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find POCO libraries
# Once done, this will define
#
#  POCO_FOUND - system has POCO
#  POCO_INCLUDE_DIRS - the POCO include directories 
#  POCO_LIBRARIES - link these to use POCO

include(FindPkgMacros)
findpkg_begin(POCO)

# Get path, convert backslashes as ${ENV_${var}}
getenv_path(POCO_HOME)
getenv_path(POCO_ROOT)
getenv_path(POCO_BASE)

# construct search paths
set(POCO_PREFIX_PATH 
  ${POCO_HOME} ${ENV_POCO_HOME} 
  ${POCO_ROOT} ${ENV_POCO_ROOT}
  ${POCO_BASE} ${ENV_POCO_BASE}
)
# redo search if prefix path changed
clear_if_changed(POCO_PREFIX_PATH
  POCO_LIBRARY_FWK
  POCO_LIBRARY_REL
  POCO_LIBRARY_DBG
  POCO_INCLUDE_DIR
)

create_search_paths(POCO)
set(POCO_INC_SEARCH_PATH ${POCO_INC_SEARCH_PATH} ${POCO_PREFIX_PATH})


set(POCO_LIBRARY_NAMES PocoFoundation PocoFoundationmt)
get_debug_names(POCO_LIBRARY_NAMES)

use_pkgconfig(POCO_PKGC POCO)

findpkg_framework(POCO)

find_path(POCO_INCLUDE_DIR NAMES Poco/Foundation.h HINTS ${POCO_INC_SEARCH_PATH} ${POCO_PKGC_INCLUDE_DIRS} PATH_SUFFIXES Foundation/include)
find_library(POCO_LIBRARY_REL NAMES ${POCO_LIBRARY_NAMES} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
find_library(POCO_LIBRARY_DBG NAMES ${POCO_LIBRARY_NAMES_DBG} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
make_library_set(POCO_LIBRARY)

findpkg_finish(POCO)

if (NOT POCO_FOUND)
  return()
endif ()


# Look for Poco's Util package
findpkg_begin(POCO_Util)
set(POCO_Util_LIBRARY_NAMES PocoUtil PocoUtilmt)
get_debug_names(POCO_Util_LIBRARY_NAMES)
find_path(POCO_Util_INCLUDE_DIR NAMES Poco/Util/Util.h HINTS ${POCO_INCLUDE_DIR} ${POCO_INC_SEARCH_PATH} ${POCO_PKGC_INCLUDE_DIRS} PATH_SUFFIXES Util/include)
find_library(POCO_Util_LIBRARY_REL NAMES ${POCO_Util_LIBRARY_NAMES} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
find_library(POCO_Util_LIBRARY_DBG NAMES ${POCO_Util_LIBRARY_NAMES_DBG} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
make_library_set(POCO_Util_LIBRARY)
findpkg_finish(POCO_Util)

# Look for Poco's Net package
findpkg_begin(POCO_Net)
set(POCO_Net_LIBRARY_NAMES PocoNet PocoNetmt)
get_debug_names(POCO_Net_LIBRARY_NAMES)
find_path(POCO_Net_INCLUDE_DIR NAMES Poco/Net/Net.h HINTS ${POCO_INCLUDE_DIR} ${POCO_INC_SEARCH_PATH} ${POCO_PKGC_INCLUDE_DIRS} PATH_SUFFIXES Net/include)
find_library(POCO_Net_LIBRARY_REL NAMES ${POCO_Net_LIBRARY_NAMES} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
find_library(POCO_Net_LIBRARY_DBG NAMES ${POCO_Net_LIBRARY_NAMES_DBG} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
make_library_set(POCO_Net_LIBRARY)
findpkg_finish(POCO_Net)

# Look for Poco's NetSSL package
findpkg_begin(POCO_NetSSL)
set(POCO_NetSSL_LIBRARY_NAMES PocoNetSSL PocoNetSSLmt)
get_debug_names(POCO_NetSSL_LIBRARY_NAMES)
find_path(POCO_NetSSL_INCLUDE_DIR NAMES Poco/Net/NetSSL.h HINTS ${POCO_INCLUDE_DIR} ${POCO_INC_SEARCH_PATH} ${POCO_PKGC_INCLUDE_DIRS} PATH_SUFFIXES NetSSL/include)
find_library(POCO_NetSSL_LIBRARY_REL NAMES ${POCO_NetSSL_LIBRARY_NAMES} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
find_library(POCO_NetSSL_LIBRARY_DBG NAMES ${POCO_NetSSL_LIBRARY_NAMES_DBG} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
make_library_set(POCO_NetSSL_LIBRARY)
findpkg_finish(POCO_NetSSL)

# Look for Poco's XML package
findpkg_begin(POCO_XML)
set(POCO_XML_LIBRARY_NAMES PocoXML PocoXMLmt)
get_debug_names(POCO_XML_LIBRARY_NAMES)
find_path(POCO_XML_INCLUDE_DIR NAMES Poco/XML/XML.h HINTS ${POCO_INCLUDE_DIR} ${POCO_INC_SEARCH_PATH} ${POCO_PKGC_INCLUDE_DIRS} PATH_SUFFIXES XML/include)
find_library(POCO_XML_LIBRARY_REL NAMES ${POCO_XML_LIBRARY_NAMES} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
find_library(POCO_XML_LIBRARY_DBG NAMES ${POCO_XML_LIBRARY_NAMES_DBG} HINTS ${POCO_LIB_SEARCH_PATH} ${POCO_PKGC_LIBRARY_DIRS} PATH_SUFFIXES Linux/i686)
make_library_set(POCO_XML_LIBRARY)
findpkg_finish(POCO_XML)

