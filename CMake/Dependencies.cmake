#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

#######################################################################
# Find all necessary and optional OGRE dependencies
#######################################################################

# OGRE_DEPENDENCIES_DIR can be used to specify a single base
# folder where the required dependencies may be found.
set(OGRE_DEPENDENCIES_DIR "" CACHE PATH "Path to prebuilt OGRE-Next dependencies")
include(FindPkgMacros)
getenv_path(OGRE_DEPENDENCIES_DIR)
if(OGRE_BUILD_PLATFORM_EMSCRIPTEN)
  set(OGRE_DEP_SEARCH_PATH 
    ${OGRE_DEPENDENCIES_DIR}
    ${EMSCRIPTEN_ROOT_PATH}/system
    ${ENV_OGRE_DEPENDENCIES_DIR}
    "${OGRE_BINARY_DIR}/EmscriptenDependencies"
    "${OGRE_SOURCE_DIR}/EmscriptenDependencies"
    "${OGRE_BINARY_DIR}/../EmscriptenDependencies"
    "${OGRE_SOURCE_DIR}/../EmscriptenDependencies"
  )
  set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${OGRE_DEP_SEARCH_PATH})
elseif(OGRE_BUILD_PLATFORM_APPLE_IOS)
  set(OGRE_DEP_SEARCH_PATH 
    ${OGRE_DEPENDENCIES_DIR}
    ${ENV_OGRE_DEPENDENCIES_DIR}
    "${OGRE_BINARY_DIR}/iOSDependencies"
    "${OGRE_SOURCE_DIR}/iOSDependencies"
    "${OGRE_BINARY_DIR}/../iOSDependencies"
    "${OGRE_SOURCE_DIR}/../iOSDependencies"
  )
elseif(OGRE_BUILD_PLATFORM_ANDROID)
  set(OGRE_DEP_SEARCH_PATH 
    ${OGRE_DEPENDENCIES_DIR}
    ${ENV_OGRE_DEPENDENCIES_DIR}
    "${OGRE_BINARY_DIR}/DependenciesAndroid"
    "${OGRE_SOURCE_DIR}/DependenciesAndroid"
    "${OGRE_BINARY_DIR}/../DependenciesAndroid"
    "${OGRE_SOURCE_DIR}/../DependenciesAndroid"
  )
else()
  set(OGRE_DEP_SEARCH_PATH 
    ${OGRE_DEPENDENCIES_DIR}
    ${ENV_OGRE_DEPENDENCIES_DIR}
    "${OGRE_BINARY_DIR}/Dependencies"
    "${OGRE_SOURCE_DIR}/Dependencies"
    "${OGRE_BINARY_DIR}/../Dependencies"
    "${OGRE_SOURCE_DIR}/../Dependencies"
  )
endif()

message(STATUS "Search path: ${OGRE_DEP_SEARCH_PATH}")

# Set hardcoded path guesses for various platforms
if (UNIX AND NOT EMSCRIPTEN)
  set(OGRE_DEP_SEARCH_PATH ${OGRE_DEP_SEARCH_PATH} /usr/local)
  # Ubuntu 11.10 has an inconvenient path to OpenGL libraries
  set(OGRE_DEP_SEARCH_PATH ${OGRE_DEP_SEARCH_PATH} /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)
endif ()

# give guesses as hints to the find_package calls
set(CMAKE_PREFIX_PATH ${OGRE_DEP_SEARCH_PATH} ${CMAKE_PREFIX_PATH})
set(CMAKE_FRAMEWORK_PATH ${OGRE_DEP_SEARCH_PATH} ${CMAKE_FRAMEWORK_PATH})

#######################################################################
# Core dependencies
#######################################################################

# Find zlib
find_package(ZLIB)
macro_log_feature(ZLIB_FOUND "zlib" "Simple data compression library" "http://www.zlib.net" FALSE "" "")

if (ZLIB_FOUND)
  # Find zziplib
  find_package(ZZip)
  macro_log_feature(ZZip_FOUND "zziplib" "Extract data from zip archives" "http://zziplib.sourceforge.net" FALSE "" "")
endif ()

# Find FreeImage
find_package(FreeImage)
macro_log_feature(FreeImage_FOUND "freeimage" "Support for commonly used graphics image formats" "http://freeimage.sourceforge.net" FALSE "" "")

# Find FreeType
find_package(Freetype)
macro_log_feature(FREETYPE_FOUND "freetype" "Portable font engine" "http://www.freetype.org" FALSE "" "")

find_package(Vulkan)
macro_log_feature(Vulkan_FOUND "vulkan-sdk" "Vulkan SDK" "https://vulkan.lunarg.com/" FALSE "" "")

# Find X11
if (UNIX AND NOT APPLE AND NOT ANDROID AND NOT EMSCRIPTEN)
  find_package(X11)
  macro_log_feature(X11_FOUND "X11" "X Window system" "http://www.x.org" TRUE "" "")
  macro_log_feature(X11_Xt_FOUND "Xt" "X Toolkit" "http://www.x.org" TRUE "" "")
  find_library(XAW_LIBRARY NAMES Xaw Xaw7 PATHS ${OGRE_DEP_SEARCH_PATH} ${DEP_LIB_SEARCH_DIR} ${X11_LIB_SEARCH_PATH})
  macro_log_feature(XAW_LIBRARY "Xaw" "X11 Athena widget set" "http://www.x.org" TRUE "" "")
  mark_as_advanced(XAW_LIBRARY)
endif ()

# Find rapidjson
find_package(Rapidjson)
macro_log_feature(Rapidjson_FOUND "rapidjson" "C++ JSON parser" "https://rapidjson.org/" FALSE "" "")

find_package(RenderDoc)
macro_log_feature(RenderDoc_FOUND "RenderDoc" "RenderDoc Integration" "https://renderdoc.org/" FALSE "" "")


#######################################################################
# RenderSystem dependencies
#######################################################################

# Find OpenGL
if(NOT ANDROID AND NOT EMSCRIPTEN)
  find_package(OpenGL)
  macro_log_feature(OPENGL_FOUND "OpenGL" "Support for the OpenGL render system" "http://www.opengl.org/" FALSE "" "")
endif()

# Find OpenGL 3+
find_package(OpenGL)
macro_log_feature(OPENGL_FOUND "OpenGL 3+" "Support for the OpenGL 3+ render system" "http://www.opengl.org/" FALSE "" "")

if(NOT OGRE_SKIP_GLES_SEARCHING)
	# Find OpenGL ES 2.x
	find_package(OpenGLES2)
	macro_log_feature(OPENGLES2_FOUND "OpenGL ES 2.x" "Support for the OpenGL ES 2.x render system" "http://www.khronos.org/opengles/" FALSE "" "")

	# Find OpenGL ES 3.x
	find_package(OpenGLES3)
	macro_log_feature(OPENGLES3_FOUND "OpenGL ES 3.x" "Support for the OpenGL ES 2.x render system with OpenGL ES 3 support" "http://www.khronos.org/opengles/" FALSE "" "")
endif()

# Find DirectX
if(WIN32)
	find_package(DirectX11)
	macro_log_feature(DirectX11_FOUND "DirectX11" "Support for the DirectX11 render system" "http://msdn.microsoft.com/en-us/directx/" FALSE "" "")

	if(OGRE_CONFIG_ENABLE_QUAD_BUFFER_STEREO)
		# Find DirectX Stereo Driver Libraries
		find_package(NVAPI)
		macro_log_feature(NVAPI_FOUND "NVAPI" "Support NVIDIA stereo with the DirectX render system" "https://developer.nvidia.com/nvapi" FALSE "" "")

		find_package(AMDQBS)
		macro_log_feature(AMDQBS_FOUND "AMDQBS" "Support AMD stereo with the DirectX render system" "http://developer.amd.com/tools-and-sdks/graphics-development/amd-quad-buffer-sdk/" FALSE "" "")
	endif()

	find_package(AMDAGS)
	macro_log_feature(AMDAGS_FOUND "AMDAGS" "Use AMD GPU Services library to provide D3D vendor extensions" "https://gpuopen.com/gaming-product/amd-gpu-services-ags-library/" FALSE "" "")
endif()

if(ANDROID)
	find_package(AndroidSwappy)
	macro_log_feature(AndroidSwappy_FOUND "Android Swappy" "Frame Pacing Library (Swappy) for Android" "https://developer.android.com/games/sdk/frame-pacing" FALSE "" "")
endif()

find_package(OpenVR)
macro_log_feature(OpenVR_FOUND "OpenVR" "OpenVR for Virtual Reality" "https://github.com/ValveSoftware/openvr" FALSE "" "")

#######################################################################
# Additional features
#######################################################################

# Find Remotery
find_package(Remotery)
macro_log_feature(Remotery_FOUND "Remotery" "Realtime CPU/D3D/OpenGL/CUDA/Metal Profiler in a single C file with web browser viewer" "https://github.com/Celtoys/Remotery" FALSE "" "")

if( Remotery_LIBRARIES )
    if( UNIX OR WIN32 )
        set( Remotery_LIBRARIES ${Remotery_LIBRARIES} ${OPENGL_LIBRARIES} )
    endif()
    if( WIN32 )
        set( Remotery_LIBRARIES ${Remotery_LIBRARIES} ${DirectX11_LIBRARIES} )
    endif()
    if( APPLE )
        set( Remotery_LIBRARIES ${Remotery_LIBRARIES} "-framework Metal" )
    endif()
endif()

# POCO
find_package(POCO)
macro_log_feature(POCO_FOUND "POCO" "POCO framework" "http://pocoproject.org/" FALSE "" "")

# ThreadingBuildingBlocks
find_package(TBB)
macro_log_feature(TBB_FOUND "tbb" "Threading Building Blocks" "http://www.threadingbuildingblocks.org/" FALSE "" "")

# GLSL-Optimizer
find_package(GLSLOptimizer)
macro_log_feature(GLSL_Optimizer_FOUND "GLSL Optimizer" "GLSL Optimizer" "http://github.com/aras-p/glsl-optimizer/" FALSE "" "")

# HLSL2GLSL
find_package(HLSL2GLSL)
macro_log_feature(HLSL2GLSL_FOUND "HLSL2GLSL" "HLSL2GLSL" "http://hlsl2glslfork.googlecode.com/" FALSE "" "")

# Python
if (EMSCRIPTEN)
  find_package(PythonInterp)
  macro_log_feature(PYTHONINTERP_FOUND "Python" "Used to generate indices for dynamic file loading" "http://www.python.org/" FALSE "" "")
endif()

#######################################################################
# Samples dependencies
#######################################################################

# Find sdl2
find_package(SDL2)
macro_log_feature(SDL2_FOUND "SDL2" "Simple DirectMedia Library" "https://www.libsdl.org/" FALSE "" "")

#######################################################################
# Tools
#######################################################################

find_package(Doxygen)
macro_log_feature(DOXYGEN_FOUND "Doxygen" "Tool for building API documentation" "http://doxygen.org" FALSE "" "")

# Find Softimage SDK
find_package(Softimage)
macro_log_feature(Softimage_FOUND "Softimage" "Softimage SDK needed for building XSIExporter" FALSE "6.0" "")

find_package(TinyXML)
macro_log_feature(TINYXML_FOUND "TinyXML" "TinyXML needed for building OgreXMLConverter" FALSE "" "")

#######################################################################
# Tests
#######################################################################

find_package(CppUnit)
macro_log_feature(CppUnit_FOUND "CppUnit" "Library for performing unit tests" "http://cppunit.sourceforge.net" FALSE "" "")

# now see if we have a buildable Dependencies package in
# the source tree. If so, include that, and it will take care of
# setting everything up, including overriding any of the above
# findings.
set(OGREDEPS_RUNTIME_OUTPUT ${OGRE_RUNTIME_OUTPUT})
if (EXISTS "${OGRE_SOURCE_DIR}/Dependencies/CMakeLists.txt")
  add_subdirectory(Dependencies)
elseif (EXISTS "${OGRE_SOURCE_DIR}/ogredeps/CMakeLists.txt")
  add_subdirectory(ogredeps)
endif ()


# Display results, terminate if anything required is missing
MACRO_DISPLAY_FEATURE_LOG()

# Add library and include paths from the dependencies
include_directories(
  ${ZLIB_INCLUDE_DIRS}
  ${ZZip_INCLUDE_DIRS}
  ${FreeImage_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${Rapidjson_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIRS}
  ${OPENGLES_INCLUDE_DIRS}
  ${OPENGLES2_INCLUDE_DIRS}
  ${OPENGLES3_INCLUDE_DIRS}
  ${X11_INCLUDE_DIR}
  ${CppUnit_INCLUDE_DIRS}
  ${GLSL_Optimizer_INCLUDE_DIRS}
  ${HLSL2GLSL_INCLUDE_DIRS}
)

link_directories(
  ${OPENGL_LIBRARY_DIRS}
  ${OPENGLES2_LIBRARY_DIRS}
  ${OPENGLES3_LIBRARY_DIRS}
  ${X11_LIBRARY_DIRS}
  ${CppUnit_LIBRARY_DIRS}
)
