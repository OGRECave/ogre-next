# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindVulkan
----------

.. versionadded:: 3.7

Find Vulkan, which is a low-overhead, cross-platform 3D graphics
and computing API.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``Vulkan::Vulkan``, if
Vulkan has been found.

This module defines :prop_tgt:`IMPORTED` target ``Vulkan::glslc``, if
Vulkan and the GLSLC SPIR-V compiler has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables::

  Vulkan_FOUND          - "True" if Vulkan was found
  Vulkan_INCLUDE_DIRS   - include directories for Vulkan
  Vulkan_LIBRARIES      - link against this library to use Vulkan

The module will also define three cache variables::

  Vulkan_INCLUDE_DIR        - the Vulkan include directory
  Vulkan_LIBRARY            - the path to the Vulkan library
  Vulkan_GLSLC_EXECUTABLE   - the path to the GLSL SPIR-V compiler

Hints
^^^^^

The ``VULKAN_SDK`` environment variable optionally specifies the
location of the Vulkan SDK root directory for the given
architecture. It is typically set by sourcing the toplevel
``setup-env.sh`` script of the Vulkan SDK directory into the shell
environment.

#]=======================================================================]

set( OGRE_VULKAN_SDK "" CACHE STRING "Path to Vulkan SDK e.g. C:\\VulkanSDK\\1.2.148.1 or /home/user/vulkansdk" )

if(WIN32)
	find_path(Vulkan_INCLUDE_DIR
		NAMES vulkan/vulkan.h
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/include" "${ENV_OGRE_DEPENDENCIES_DIR}/include"
			"${OGRE_SOURCE}/Dependencies/include"
			"${OGRE_VULKAN_SDK}/Include"
			"$ENV{VULKAN_SDK}/Include"
		)

	find_path(Vulkan_SHADERC_INCLUDE_DIR
		NAMES shaderc/shaderc.h
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/include" "${ENV_OGRE_DEPENDENCIES_DIR}/include"
			"${OGRE_SOURCE}/Dependencies/include"
			"${OGRE_VULKAN_SDK}/Include"
			"$ENV{VULKAN_SDK}/Include"
		)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set( VK_BIN "Bin" )
		set( VK_LIB "Lib" )
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set( VK_BIN "Bin32" )
		set( VK_LIB "Lib32" )
	endif()

	find_library(Vulkan_LIBRARY
		NAMES vulkan-1
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_LIB}"
			"${OGRE_VULKAN_SDK}/${VK_BIN}"
			"$ENV{VULKAN_SDK}/${VK_LIB}"
			"$ENV{VULKAN_SDK}/${VK_BIN}"
		PATH_SUFFIXES "" Release RelWithDebInfo MinSizeRel Debug
		)
	find_library(Vulkan_SHADERC_LIB_REL
		NAMES shaderc_combined
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_LIB}"
			"${OGRE_VULKAN_SDK}/${VK_BIN}"
			"$ENV{VULKAN_SDK}/${VK_LIB}"
			"$ENV{VULKAN_SDK}/${VK_BIN}"
		PATH_SUFFIXES "" Release RelWithDebInfo MinSizeRel
		)
	find_library(Vulkan_SHADERC_LIB_DBG
		NAMES shaderc_combined
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
		PATH_SUFFIXES "" Debug
		)
	find_program(Vulkan_GLSLC_EXECUTABLE
		NAMES glslc
		HINTS
			"${OGRE_VULKAN_SDK}/${VK_BIN}"
			"$ENV{VULKAN_SDK}/${VK_BIN}"
		)
else()
	set( VK_ARCH "x86_64" )
	find_path(Vulkan_INCLUDE_DIR
		NAMES vulkan/vulkan.h
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_ARCH}/include"
			"$ENV{VULKAN_SDK}/include"
		)
	find_path(Vulkan_SHADERC_INCLUDE_DIR
		NAMES shaderc/shaderc.h
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_ARCH}/include"
			"$ENV{VULKAN_SDK}/include"
		)
	find_library(Vulkan_LIBRARY
		NAMES vulkan
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_ARCH}/lib"
			"$ENV{VULKAN_SDK}/lib"
		)
	find_library(Vulkan_SHADERC_LIB_REL
		NAMES shaderc_combined
		HINTS
			"${OGRE_DEPENDENCIES_DIR}/lib" "${ENV_OGRE_DEPENDENCIES_DIR}/lib"
			"${OGRE_SOURCE}/Dependencies/lib"
			"${OGRE_VULKAN_SDK}/${VK_ARCH}/lib"
			"$ENV{VULKAN_SDK}/lib"
		)
	set( Vulkan_SHADERC_LIB_DBG ${Vulkan_SHADERC_LIB_REL} )
	find_program(Vulkan_GLSLC_EXECUTABLE
		NAMES glslc
		HINTS
			"${OGRE_VULKAN_SDK}/${VK_ARCH}/bin"
			"$ENV{VULKAN_SDK}/bin"
		)
endif()

set(Vulkan_LIBRARIES ${Vulkan_LIBRARY} optimized ${Vulkan_SHADERC_LIB_REL} debug ${Vulkan_SHADERC_LIB_DBG})
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR} ${Vulkan_SHADERC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan
  DEFAULT_MSG
  Vulkan_LIBRARY Vulkan_SHADERC_LIB_REL Vulkan_SHADERC_LIB_DBG Vulkan_INCLUDE_DIR)

mark_as_advanced(Vulkan_INCLUDE_DIR Vulkan_LIBRARY Vulkan_SHADERC_INCLUDE_DIR Vulkan_SHADERC_LIB_REL Vulkan_SHADERC_LIB_DBG Vulkan_GLSLC_EXECUTABLE)

if(Vulkan_FOUND AND NOT TARGET Vulkan::Vulkan)
  add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
  set_target_properties(Vulkan::Vulkan PROPERTIES
    IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
endif()

if(Vulkan_FOUND AND Vulkan_GLSLC_EXECUTABLE AND NOT TARGET Vulkan::glslc)
  add_executable(Vulkan::glslc IMPORTED)
  set_property(TARGET Vulkan::glslc PROPERTY IMPORTED_LOCATION "${Vulkan_GLSLC_EXECUTABLE}")
endif()
