#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

######################################################################
# OGRE-Next BUILD SYSTEM
# Welcome to the CMake build system for OGRE-Next.
# This is the main file where we prepare the general build environment
# and provide build configuration options.
######################################################################

cmake_minimum_required(VERSION 3.13)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)
cmake_policy(SET CMP0003 NEW)

if(CMAKE_VS_PLATFORM_NAME STREQUAL "Tegra-Android")
	set(CMAKE_LINK_LIBRARY_FLAG "")
endif()

#fix Policy CMP0072 is not set
set(OpenGL_GL_PREFERENCE LEGACY)

# Use relative paths
# This is mostly to reduce path size for command-line limits on windows
if(WIN32)
  # This seems to break Xcode projects so definitely don't enable on Apple builds
  set(CMAKE_USE_RELATIVE_PATHS true)
  set(CMAKE_SUPPRESS_REGENERATION true)
endif()

if (APPLE AND NOT ANDROID)
  SET(CMAKE_SIZEOF_VOID_P 4)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_XCODE_ATTRIBUTE_USE_HEADERMAP NO)
endif ()

# This option needs to be set now
# OgreAddTargets and OgreConfigTargets make use of OGRE_NEXT and OGRE_NEXT_PREFIX
option(OGRE_USE_NEW_PROJECT_NAME
	"Libraries containing Ogre name will be called OgreNext, e.g. 'OgreNext.dll' instead of 'Ogre.dll'"
	TRUE)

if( OGRE_USE_NEW_PROJECT_NAME )
	set( OGRE_NEXT "OgreNext" )
	set( OGRE_NEXT_PREFIX "OGRE-Next")
else()
	set( OGRE_NEXT "Ogre" )
	set( OGRE_NEXT_PREFIX "OGRE" )
endif()

project(OGRE-Next)
# capture ogre location, and at the same time hide legacy cache variables
set(OGRE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(OGRE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Include necessary submodules
set(CMAKE_MODULE_PATH
  "${OGRE_SOURCE_DIR}/CMake"
  "${OGRE_SOURCE_DIR}/CMake/Utils"
  "${OGRE_SOURCE_DIR}/CMake/Packages"
)

if(ANDROID)
  set(CMAKE_FIND_ROOT_PATH ${OGRE_DEPENDENCIES_DIR} "${CMAKE_FIND_ROOT_PATH}")
endif()

include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(MacroLogFeature)
include(OgreAddTargets)
include(OgreConfigTargets)
include(PreprocessorUtils)
set(OGRE_TEMPLATES_DIR "${OGRE_SOURCE_DIR}/CMake/Templates")
set(OGRE_WORK_DIR ${OGRE_BINARY_DIR})


#####################################################################
# Set up the basic build environment
#####################################################################

if (NOT CMAKE_BUILD_TYPE)
  # CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
  # differentiation between debug and release builds.
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

# determine Ogre version numbers
include(OgreGetVersion)
ogre_get_version(${OGRE_SOURCE_DIR}/OgreMain/include/OgrePrerequisites.h)
message(STATUS "Configuring OGRE ${OGRE_VERSION}")
# Configure version file for use by scripts
configure_file("${OGRE_TEMPLATES_DIR}/version.txt.in" "${OGRE_BINARY_DIR}/version.txt" @ONLY)

# determine if we are compiling for a 32bit or 64bit system
include(CheckTypeSize)
CHECK_TYPE_SIZE("void*" OGRE_PTR_SIZE BUILTIN_TYPES_ONLY)
if (OGRE_PTR_SIZE EQUAL 8)
  set(OGRE_PLATFORM_X64 TRUE)
else ()
  set(OGRE_PLATFORM_X64 FALSE)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif ()

if (NOT APPLE)
  # Create debug libraries with _d postfix
  set(CMAKE_DEBUG_POSTFIX "_d")
endif ()

# Set compiler specific build flags
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
  if (EMSCRIPTEN OR (ANDROID AND NOT ANDROID_ABI MATCHES "x86") )
    set(OGRE_GCC_HAS_SSE FALSE)
  else()
    check_cxx_compiler_flag(-msse OGRE_GCC_HAS_SSE)

    if( UNIX AND (EXISTS /usr/bin/ld.lld OR EXISTS /usr/local/bin/ld.lld) )
      message( STATUS "Using lld linker" )
      set( CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld" )
    else()
      message( "BUILD PERFORMANCE - lld linker not found. Consider installing it for much faster link times" )
    endif()
  endif()
  if (OGRE_GCC_HAS_SSE)
    if(CMAKE_GENERATOR STREQUAL "Xcode")
      # XCode builds universal binaries, and generates too much warnings when -msse and -msse2 are passed to arm compiler
      set(CMAKE_XCODE_ATTRIBUTE_ARCH_CXXFLAGS[arch=i386] "-msse -msse2")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $(ARCH_CXXFLAGS)")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
    endif ()
  endif ()
  # This is a set of sensible warnings that provide meaningful output
  set(OGRE_WARNING_FLAGS
      "-Wall -Winit-self -Wcast-qual -Wwrite-strings -Wextra -Wno-unused-parameter -Wshadow -Wimplicit-fallthrough")

  if( CMAKE_COMPILER_IS_GNUCXX )
    # On GCC:
    # - Wsign-conversion has too many false positives. See https://stackoverflow.com/a/64542771
    # - Wconversion is too dumb. Too many false positives.
    set( OGRE_WARNING_FLAGS "${OGRE_WARNING_FLAGS} -Wsuggest-override" )
  else()
    set( OGRE_WARNING_FLAGS "${OGRE_WARNING_FLAGS} \
        -Winconsistent-missing-destructor-override -Wcomma -Wsign-conversion -Wconversion" )
  endif()

  if (EMSCRIPTEN)
      set(OGRE_WARNING_FLAGS "${OGRE_WARNING_FLAGS} -Wno-warn-absolute-paths")
  elseif (NOT APPLE AND NOT CMAKE_COMPILER_IS_CLANGXX)
      set(OGRE_WARNING_FLAGS "${OGRE_WARNING_FLAGS} -Wno-unused-but-set-parameter")
  endif ()
  set(CMAKE_CXX_FLAGS "${OGRE_WARNING_FLAGS} ${CMAKE_CXX_FLAGS}")

  set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1" )
endif ()
if (MSVC)
  if (CMAKE_BUILD_TOOL STREQUAL "nmake")
    # set variable to state that we are using nmake makefiles
	set(NMAKE TRUE)
  endif ()
  if (CMAKE_BUILD_TOOL STREQUAL "jom")
	# set variable to state that we are using jom makefiles
	set(JOM TRUE)
  endif ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /Zc:__cplusplus")
  if (NOT OGRE_PLATFORM_X64)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  endif()
  # Enable intrinsics on MSVC in debug mode
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Oi")
  if (CMAKE_CL_64)
    # Visual Studio bails out on debug builds in 64bit mode unless
	# this flag is set...
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /bigobj")
  else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
  endif ()
  if (OGRE_UNITY_BUILD)
    # object files can get large with Unity builds
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  endif ()
  option(OGRE_BUILD_MSVC_MP "Enable build with multiple processes in Visual Studio" TRUE)
  if(OGRE_BUILD_MSVC_MP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif ()
  option(OGRE_BUILD_MSVC_ZM "Add /Zm256 compiler option to Visual Studio to fix PCH errors" TRUE)

  if(OGRE_BUILD_MSVC_ZM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm256")
  endif ()
endif ()
if (MINGW)
  add_definitions(-D_WIN32_WINNT=0x0500)
  if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    # set architecture to i686, since otherwise some versions of MinGW can't link
    # the atomic primitives
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i686")
  endif ()
  # Fpermissive required to let some "non-standard" casting operations in the plugins pass
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
  # disable this optimisation because it breaks release builds (reason unknown)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-slp-vectorize")
  # Ignore some really annoying warnings which also happen in dependencies
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=cast-qual -Wno-unused-local-typedefs")
endif ()

if ((CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX) AND NOT MINGW)
  # Test for GCC visibility
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-fvisibility=hidden OGRE_GCC_VISIBILITY)
  if (OGRE_GCC_VISIBILITY)
    # determine gcc version
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
      OUTPUT_VARIABLE OGRE_GCC_VERSION)
    message(STATUS "Detected g++ ${OGRE_GCC_VERSION}")
    message(STATUS "Enabling GCC visibility flags")
    set(OGRE_GCC_VISIBILITY_FLAGS "-fvisibility=hidden")
    set(XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "YES")

    # check if we can safely add -fvisibility-inlines-hidden
    string(TOLOWER "${CMAKE_BUILD_TYPE}" OGRE_BUILD_TYPE)
    if (OGRE_BUILD_TYPE STREQUAL "debug" AND OGRE_GCC_VERSION VERSION_LESS "4.2")
      message(STATUS "Skipping -fvisibility-inlines-hidden due to possible bug in g++ < 4.2")
    else ()
      set(OGRE_GCC_VISIBILITY_FLAGS "${OGRE_GCC_VISIBILITY_FLAGS} -fvisibility-inlines-hidden")
      set(XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN "YES")
    endif()
  endif(OGRE_GCC_VISIBILITY)

  # Fix x64 issues on Linux
  if(OGRE_PLATFORM_X64 AND NOT APPLE)
    add_definitions(-fPIC)
  endif()
endif ((CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX) AND NOT MINGW)

# determine system endianess
if (MSVC OR ANDROID OR EMSCRIPTEN)
  # This doesn't work on VS 2010
  # MSVC only builds for intel anyway
  set(OGRE_TEST_BIG_ENDIAN FALSE)
else()
  include(TestBigEndian)
  test_big_endian(OGRE_TEST_BIG_ENDIAN)
endif()

# Add OgreMain include path
include_directories("${OGRE_SOURCE_DIR}/OgreMain/include")
include_directories("${OGRE_BINARY_DIR}/include")
if (APPLE)
  if (OGRE_BUILD_PLATFORM_APPLE_IOS)
    include_directories("${OGRE_SOURCE_DIR}/OgreMain/include/iOS")

    # Set static early for proper dependency detection
    set(OGRE_STATIC TRUE)
  else ()
    include_directories("${OGRE_SOURCE_DIR}/OgreMain/include/OSX")
  endif ()
endif (APPLE)

# Add platform-specific render system Metal include path
if (APPLE AND OGRE_BUILD_RENDERSYSTEM_METAL)
  if (OGRE_BUILD_PLATFORM_APPLE_IOS)
    include_directories("${OGRE_SOURCE_DIR}/RenderSystems/Metal/include/Windowing/iOS")
  else ()
    include_directories("${OGRE_SOURCE_DIR}/RenderSystems/Metal/include/Windowing/OSX")
  endif ()
endif (APPLE AND OGRE_BUILD_RENDERSYSTEM_METAL)

# Find dependencies
include(Dependencies)
# definitions for samples
set(OGRE_LIBRARIES ${OGRE_NEXT}Main)
set(OGRE_MeshLodGenerator_LIBRARIES ${OGRE_NEXT}MeshLodGenerator)
set(OGRE_Paging_LIBRARIES ${OGRE_NEXT}Paging)
set(OGRE_Terrain_LIBRARIES ${OGRE_NEXT}Terrain)
set(OGRE_Volume_LIBRARIES ${OGRE_NEXT}Volume)

if( OGRE_STATIC AND UNIX AND NOT APPLE AND NOT OGRE_CONFIG_UNIX_NO_X11 )
	set( OGRE_LIBRARIES ${OGRE_LIBRARIES} xcb xcb-randr )
endif()

# Specify build paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OGRE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OGRE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OGRE_BINARY_DIR}/bin")
if (WIN32 OR APPLE)
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # We don't want to install in default system location, install is really for the SDK, so call it that
    set(CMAKE_INSTALL_PREFIX
	  "${OGRE_BINARY_DIR}/sdk" CACHE PATH "OGRE install prefix" FORCE
    )
  endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif(WIN32 OR APPLE)

# Set up iOS overrides.
if (OGRE_BUILD_PLATFORM_APPLE_IOS)
  # Set build variables
  set(XCODE_ATTRIBUTE_SDKROOT iphoneos)
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    execute_process(COMMAND xcodebuild -version -sdk "${XCODE_ATTRIBUTE_SDKROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
    string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
  else()
    set(CMAKE_OSX_SYSROOT iphoneos)
  endif()

  set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -framework CoreGraphics -framework QuartzCore -framework UIKit")
  set( CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES )
  if( OGRE_BUILD_RENDERSYSTEM_METAL )
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Metal")
  endif()
  set(XCODE_ATTRIBUTE_GCC_UNROLL_LOOPS "YES")
  set(XCODE_ATTRIBUTE_LLVM_VECTORIZE_LOOPS "YES")
  set(XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
  set(XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES")
  #set(OGRE_BUILD_RENDERSYSTEM_GLES2 TRUE CACHE BOOL "Forcing OpenGL ES 2 RenderSystem for iOS" FORCE)
  set(OGRE_STATIC TRUE CACHE BOOL "Forcing static build for iOS" FORCE)
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.\${PRODUCT_NAME:rfc1034identifier}")
  set( CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" )

  if( NOT CMAKE_GENERATOR STREQUAL "Ninja" )
    set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_INCLUDING_64_BIT)")
  endif()
  string (REPLACE "-msse2" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string (REPLACE "-msse" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif (ANDROID)
  set(TargetPlatform "Android")
  set(OGRE_PLATFORM OGRE_PLATFORM_ANDROID)
  set(OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE TRUE CACHE BOOL "Enable viewport orientation Android" FORCE)
  option(OGRE_BUILD_ANDROID_JNI_SAMPLE "Enable JNI Sample" FALSE)

  if( NOT ANDROID_GLES_ONLY )
   set(OGRE_CONFIG_ENABLE_GLES2_VAO_SUPPORT FALSE CACHE BOOL "Disable VAO on Android" FORCE)
  else()
   set(OGRE_BUILD_COMPONENT_VOLUME FALSE CACHE BOOL "Disable volume component on Android ES1" FORCE)
   set(OGRE_BUILD_COMPONENT_PAGING FALSE CACHE BOOL "Disable paging component on Android ES1" FORCE)
  endif()

  set(OGRE_BUILD_TOOLS FALSE CACHE BOOL "Disable tools on Android" FORCE)
  set(OGRE_STATIC TRUE CACHE BOOL "Forcing static build for Android" FORCE)
  set(OGRE_SIMD FALSE CACHE BOOL "Disable SIMD on Android" FORCE)
  set(OGRE_RESTRICT_ALIASING FALSE CACHE BOOL "Disable strict aliasing on Android" FORCE)
  string (REPLACE "-msse2" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(EMSCRIPTEN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-extensions")
  add_definitions(-DEMSCRIPTEN=1 -D__EMSCRIPTEN__=1)
  set(TargetPlatform "Emscripten")
  set(OGRE_PLATFORM OGRE_PLATFORM_EMSCRIPTEN)
  set(OGRE_BUILD_RENDERSYSTEM_GLES2 TRUE CACHE BOOL "Forcing OpenGL ES 2 RenderSystem for Emscripten" FORCE)
  set(OGRE_CONFIG_ENABLE_STBI TRUE CACHE BOOL "Enable STBIImageCodec on Emscripten (Smaller Footprint)" FORCE)
  set(OGRE_CONFIG_ENABLE_FREEIMAGE FALSE CACHE BOOL "Disable Freeimage on Emscripten (Smaller Footprint)" FORCE)
  set(OGRE_BUILD_TOOLS FALSE CACHE BOOL "Disable tools on Emscripten" FORCE)
  set(OGRE_BUILD_TESTS FALSE CACHE BOOL "Disable tests on Emscripten" FORCE)
  set(OGRE_BUILD_COMPONENT_VOLUME FALSE CACHE BOOL "Disable volume component on Emscripten" FORCE)
  set(OGRE_BUILD_COMPONENT_PAGING FALSE CACHE BOOL "Disable paging component on Emscripten" FORCE)
  set(OGRE_CONFIG_ALLOCATOR 0 CACHE STRING "Use default allocator on Emscripten" FORCE)
  set(OGRE_CONFIG_CONTAINERS_USE_CUSTOM_ALLOCATOR FALSE CACHE BOOL "Containers use default allocator on Emscripten" FORCE)
elseif (APPLE AND NOT OGRE_BUILD_PLATFORM_APPLE_IOS)

  set(XCODE_ATTRIBUTE_SDKROOT macosx)
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    execute_process(COMMAND xcodebuild -version -sdk "${XCODE_ATTRIBUTE_SDKROOT}" Path | head -n 1 OUTPUT_VARIABLE CMAKE_OSX_SYSROOT)
    string(REGEX REPLACE "(\r?\n)+$" "" CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}")
  else()
    set(CMAKE_OSX_SYSROOT macosx)
  endif()

  set( CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES )

  if (NOT CMAKE_OSX_ARCHITECTURES)
    if(OGRE_BUILD_RENDERSYSTEM_GL3PLUS)
      if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
        set(CMAKE_OSX_ARCHITECTURES "${ARCHS_STANDARD_64_BIT}")
      else()
        set(CMAKE_OSX_ARCHITECTURES "${ARCHS_STANDARD_64_BIT}")
      endif()
    else()
      if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
        set(CMAKE_OSX_ARCHITECTURES "${ARCHS_STANDARD_32_64_BIT}")
      else()
        set(CMAKE_OSX_ARCHITECTURES "${ARCHS_STANDARD_32_64_BIT}")
      endif()
    endif()
  endif()

  # Make sure that the OpenGL render system is selected for non-iOS Apple builds
  set(OGRE_BUILD_RENDERSYSTEM_GLES2 FALSE)
endif ()

if(OGRE_BUILD_COMPONENT_MESHLODGENERATOR)
  set(OGRE_CONFIG_ENABLE_MESHLOD TRUE CACHE BOOL "Forcing Mesh Lod" FORCE)
endif()

if(OGRE_CONFIG_ENABLE_GLES3_SUPPORT)
  set(OGRE_CONFIG_ENABLE_GLES2_VAO_SUPPORT TRUE CACHE BOOL "Forcing VAO support for OpenGL ES 3" FORCE)
endif()

# Enable the PVRTC codec if OpenGL ES is being built
if(OGRE_BUILD_RENDERSYSTEM_GLES2)
  set(OGRE_CONFIG_ENABLE_PVRTC TRUE CACHE BOOL "Forcing PVRTC codec for OpenGL ES" FORCE)
  set(OGRE_CONFIG_ENABLE_ETC TRUE CACHE BOOL "Forcing ETC codec for OpenGL ES" FORCE)
endif()

# Enable the ETC codec if OpenGL 3+ is being built
if(OGRE_BUILD_RENDERSYSTEM_GL3PLUS)
  set(OGRE_CONFIG_ENABLE_ETC TRUE CACHE BOOL "Forcing ETC codec for OpenGL 3+" FORCE)
endif()

if(OGRE_BUILD_RENDERSYSTEM_METAL AND OGRE_BUILD_PLATFORM_APPLE_IOS)
  set(OGRE_CONFIG_ENABLE_ASTC TRUE CACHE BOOL "Forcing ASTC codec for Metal" FORCE)
endif()

######################################################################
# Provide user options to customise the build process
######################################################################

# Customise what to build
cmake_dependent_option(OGRE_STATIC "Static build" FALSE "" TRUE)
set(OGRE_SHADER_COMPILATION_THREADING_MODE 1 CACHE STRING
"Specify shader and PSO threading model:
  0 - Disable.
  1 - Use compatible API. Enable only if supported by compiler and OS.
  2 - Force-enable.

This setting is independent of OGRE_CONFIG_THREAD_PROVIDER.

Option 1 means we will use compiler-assisted TLS (Thread Local Storage) to provide a
backwards-compatible interface and ease porting to newer OgreNext versions.
However this is only supported if OGRE_STATIC is true.

Option 2 will disable the backwards-compatible interface, which means there may be compiler
errors when porting to newer OgreNext versions. However, this works on dynamic DLLs builds."
)
if( OGRE_SHADER_COMPILATION_THREADING_MODE EQUAL 0 )
	set( OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API TRUE )
	set( OGRE_SHADER_THREADING_USE_TLS FALSE )
elseif( OGRE_SHADER_COMPILATION_THREADING_MODE EQUAL 1 )
	set( OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API TRUE )
	set( OGRE_SHADER_THREADING_USE_TLS ${OGRE_STATIC} )
else()
	set( OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API FALSE )
	set( OGRE_SHADER_THREADING_USE_TLS FALSE )
endif()
cmake_dependent_option(OGRE_BUILD_RENDERSYSTEM_D3D11 "Build Direct3D11 RenderSystem" TRUE "WIN32;DirectX11_FOUND OR WINDOWS_STORE OR WINDOWS_PHONE" FALSE)
cmake_dependent_option(OGRE_BUILD_RENDERSYSTEM_GL3PLUS "Build OpenGL 3+ RenderSystem" TRUE "OPENGL_FOUND; NOT APPLE; NOT OGRE_BUILD_RENDERSYSTEM_GLES; NOT OGRE_BUILD_RENDERSYSTEM_GLES2;NOT WINDOWS_STORE;NOT WINDOWS_PHONE;NOT OGRE_BUILD_PLATFORM_APPLE_IOS" FALSE)
cmake_dependent_option(OGRE_GLSUPPORT_USE_EGL_HEADLESS "Use EGL. Required if you want to use GL3+ headless support (e.g. rendering on the Cloud without X11). If more than one GLSUPPORT is enabled, interface can be selected at runtime" FALSE "OGRE_BUILD_RENDERSYSTEM_GL3PLUS;EGL_FOUND" FALSE)
if( UNIX AND NOT APPLE AND (NOT OGRE_BUILD_RENDERSYSTEM_GL3PLUS OR OGRE_GLSUPPORT_USE_EGL_HEADLESS) )
	option(OGRE_CONFIG_UNIX_NO_X11 "Avoid X11 dependencies as much as possible. OGRE_GLSUPPORT_USE_GLX & OGRE_VULKAN_WINDOW_XCB won't be available" FALSE)
endif()
cmake_dependent_option(OGRE_GLSUPPORT_USE_GLX "Use GLX (Default / Native window). If more than one GLSUPPORT is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_GL3PLUS;UNIX;NOT APPLE;NOT OGRE_CONFIG_UNIX_NO_X11" FALSE)
cmake_dependent_option(OGRE_GLSUPPORT_USE_WGL "Use WGL (Default / Native window). If more than one GLSUPPORT is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_GL3PLUS;WIN32" FALSE)
cmake_dependent_option(OGRE_GLSUPPORT_USE_COCOA "Use Cocoa (Default / Native window). If more than one GLSUPPORT is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_GL3PLUS;APPLE" FALSE)
set( OGRE_GLSUPPORT_NUM_INTERFACES 0 )
if( OGRE_GLSUPPORT_USE_GLX OR OGRE_GLSUPPORT_USE_WGL OR OGRE_GLSUPPORT_USE_COCOA )
	math( EXPR OGRE_GLSUPPORT_NUM_INTERFACES "${OGRE_GLSUPPORT_NUM_INTERFACES} + 1" )
endif()
if( OGRE_GLSUPPORT_USE_EGL_HEADLESS )
	math( EXPR OGRE_GLSUPPORT_NUM_INTERFACES "${OGRE_GLSUPPORT_NUM_INTERFACES} + 1" )
endif()
cmake_dependent_option(OGRE_BUILD_RENDERSYSTEM_GLES2 "Build OpenGL ES 2.x RenderSystem" FALSE "OPENGLES2_FOUND;NOT WINDOWS_STORE;NOT WINDOWS_PHONE" FALSE)
cmake_dependent_option(OGRE_BUILD_RENDERSYSTEM_VULKAN "Build Vulkan RenderSystem" TRUE "Vulkan_FOUND" FALSE)
cmake_dependent_option(OGRE_VULKAN_USE_SWAPPY "Vulkan: Use Swappy Frame Pacing (Recommended)" TRUE "ANDROID;AndroidSwappy_FOUND" FALSE)
cmake_dependent_option(OGRE_VULKAN_WINDOW_NULL "Allow headless support. If more than one OGRE_VULKAN_WINDOW is enabled, interface can be selected at runtime" FALSE "OGRE_BUILD_RENDERSYSTEM_VULKAN" FALSE)
cmake_dependent_option(OGRE_VULKAN_WINDOW_WIN32 "Use Win32 (Default / Native window). If more than one OGRE_VULKAN_WINDOW is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_VULKAN;WIN32" FALSE)
cmake_dependent_option(OGRE_VULKAN_WINDOW_XCB "Use X11 / xcb (Default / Native window). If more than one OGRE_VULKAN_WINDOW is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_VULKAN;UNIX;NOT ANDROID;NOT OGRE_CONFIG_UNIX_NO_X11" FALSE)
cmake_dependent_option(OGRE_VULKAN_WINDOW_ANDROID "Use Android (Default / 'ANative' window). If more than one OGRE_VULKAN_WINDOW is enabled, interface can be selected at runtime" TRUE "OGRE_BUILD_RENDERSYSTEM_VULKAN;ANDROID" FALSE)
if( OGRE_BUILD_RENDERSYSTEM_VULKAN AND OGRE_CONFIG_UNIX_NO_X11 )
	set(OGRE_VULKAN_WINDOW_NULL ON CACHE BOOL "Forced on due to OGRE_CONFIG_UNIX_NO_X11" FORCE)
endif()
if( APPLE )
    option(OGRE_BUILD_PLATFORM_APPLE_IOS "Build For iOS (Apple only)" FALSE)
    option(OGRE_BUILD_RENDERSYSTEM_METAL "Build Metal RenderSystem (Apple only)" TRUE)
endif()
option(OGRE_BUILD_PLUGIN_PFX "Build ParticleFX plugin" TRUE)
option(OGRE_BUILD_PLUGIN_PFX2 "Build ParticleFX2 plugin" TRUE)

option(OGRE_IDSTRING_USE_128 "Ogre will use 128 for IdString to reduce chances of collisions. Use this if you have approximately >= 10.000 meshes or materials" FALSE)

option( OGRE_ADDRESS_SANITIZER_ASAN "Build with Asan (Address Sanitizer)" OFF )

option( OGRE_BUILD_COMPONENT_ATMOSPHERE "Component to use atmosphere (e.g. sky), can be used by HlmsPbs" TRUE )
cmake_dependent_option(OGRE_BUILD_COMPONENT_HLMS_PBS
"PBS Stands for Physically Based Shading and it's the default material for most entities and meshes. This is the 'desktop' version for OpenGL 3.3+ and D3D11." TRUE "" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_HLMS_UNLIT
"Unlit material system supporting texture animations, multiple diffuse texture units (aka multitexturing) with Photoshop-like blend modes. This is the
'desktop' version for OpenGL 3.3+ and D3D11." TRUE "" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_PAGING "Build Paging component" FALSE "" FALSE)
option( OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS "Component to use planar reflections, can be used by both HlmsPbs & HlmsUnlit" FALSE )
cmake_dependent_option(OGRE_BUILD_COMPONENT_MESHLODGENERATOR "Build MeshLodGenerator component" TRUE "" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_VOLUME "Build Volume component" FALSE "" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_PROPERTY "Build Property component" TRUE "" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_OVERLAY "Build Overlay component" TRUE "FREETYPE_FOUND OR OGRE_BUILD_PLATFORM_WINRT OR OGRE_BUILD_PLATFORM_WINDOWS_PHONE" FALSE)
cmake_dependent_option(OGRE_BUILD_COMPONENT_DEAR_IMGUI "Build Dear Imgui integration component" TRUE "DearImgui_FOUND" FALSE)
cmake_dependent_option(OGRE_BUILD_LIBS_AS_FRAMEWORKS "Build frameworks for libraries on OS X." TRUE "APPLE;NOT OGRE_BUILD_PLATFORM_APPLE_IOS" FALSE)

option(OGRE_BUILD_SAMPLES2 "Build Ogre new demos (2.1)" TRUE)
cmake_dependent_option(OGRE_BUILD_SAMPLES_AS_BUNDLES "Build samples as application bundles on OS X." TRUE "APPLE;OGRE_BUILD_SAMPLES2" FALSE)
cmake_dependent_option(OGRE_BUILD_TOOLS "Build the command-line tools" TRUE "NOT OGRE_BUILD_PLATFORM_APPLE_IOS;NOT WINDOWS_STORE;NOT OGRE_BUILD_PLATFORM_WINDOWS_PHONE" FALSE)
cmake_dependent_option(OGRE_BUILD_XSIEXPORTER "Build the Softimage exporter" FALSE "Softimage_FOUND" FALSE)
option(OGRE_BUILD_TESTS "Build the unit tests & PlayPen" FALSE)
option(OGRE_CONFIG_DOUBLE "Use doubles instead of floats in Ogre" FALSE)
option(OGRE_CONFIG_NODE_INHERIT_TRANSFORM "Tells the node whether it should inherit full transform from it's parent node or derived position, orientation and scale" FALSE)

set(OGRE_PLUGIN_LIB_PREFIX "" CACHE STRING "Prefix dynamically linked plugins, e.g. 'lib' (GCC and Clang only)")

if (WINDOWS_STORE OR WINDOWS_PHONE)
# WinRT can only use the standard allocator
set(OGRE_CONFIG_ALLOCATOR 0 CACHE STRING "Forcing no Allocator for WinRT" FORCE)
else ()
set(OGRE_CONFIG_ALLOCATOR 0 CACHE STRING
"Specify the memory allocator to use. Possible values:
  0 - None
  1 - Standard allocator. Supports additional debugging (see OGRE_CONFIG_MEMTRACK)
  3 - User-provided allocator
  5 - Debug allocator that will assert on most forms of memory corruption and provides deterministic memory addressing (for easier debugging w/ data breakpoints). Not indended for release or deployment. You may want to tweak OGRE_TRACK_POOL_SIZE in OgreMain/src/OgreMemoryTrackAlloc.cpp."
)
endif ()

set( OGRE_EMBED_DEBUG_MODE auto CACHE STRING
	"Specify whether OGRE_DEBUG_MODE should be embedded into OgreBuildSettings.h
	auto - Use 'never' for multi-config generators like MSVC & XCode. Use 'always' for Make & Ninja generators
	never - Compile headers will determine whether it's a debug build based on macros like _DEBUG, DEBUG & NDEBUG
	always - OGRE_DEBUG_MODE is embedded into OgreBuildSettings.h, its value is based on CMAKE_BUILD_TYPE"
)
set( OGRE_DEBUG_LEVEL_DEBUG 3 CACHE STRING
    "Specify debug level for debug builds:
    0 - None. Disabled. No checks done at all.
    1 - Low. We perform basic assert checks and other non-performance intensive checks.
    2 - Medium. We alter classes to add some debug variables for a bit more thorough checking and also perform checks that may cause some performance hit.
    3 - High. We perform intensive validation without concerns for performance."
)
set( OGRE_DEBUG_LEVEL_RELEASE 0 CACHE STRING
    "Specify debug level for Release, RelWithDebInfo and MinSizeRel builds. See OGRE_DEBUG_LEVEL_DEBUG" )

cmake_dependent_option(OGRE_CONFIG_CONTAINERS_USE_CUSTOM_ALLOCATOR "STL containers in Ogre use the custom allocator" TRUE "" FALSE)
if( OGRE_CONFIG_ALLOCATOR EQUAL 0 )
    set(OGRE_CONFIG_CONTAINERS_USE_CUSTOM_ALLOCATOR FALSE CACHE BOOL "Forced off when OGRE_CONFIG_ALLOCATOR = 0" FORCE)
endif()
option(OGRE_CONFIG_STRING_USE_CUSTOM_ALLOCATOR "Ogre String uses the custom allocator" FALSE)
option(OGRE_CONFIG_MEMTRACK_DEBUG "Enable Ogre's memory tracker in debug mode" FALSE)
option(OGRE_CONFIG_MEMTRACK_RELEASE "Enable Ogre's memory tracker in release mode" FALSE)
# determine threading options
include(PrepareThreadingOptions)
cmake_dependent_option(OGRE_CONFIG_ENABLE_FREEIMAGE "Build FreeImage codec. If you disable this option, you need to provide your own image handling codecs." TRUE "FreeImage_FOUND" FALSE)
option(OGRE_CONFIG_ENABLE_STBI "Enable STBI image codec." FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_JSON "Use rapidjson (needed by Hlms JSON materials)" TRUE "Rapidjson_FOUND" FALSE)
option(OGRE_CONFIG_ENABLE_MESHLOD "Enable Mesh Lod." TRUE)
option(OGRE_CONFIG_ENABLE_DDS "Build DDS codec." TRUE)
option(OGRE_CONFIG_ENABLE_PVRTC "Build PVRTC codec." FALSE)
option(OGRE_CONFIG_ENABLE_ETC "Build ETC codec." FALSE)
option(OGRE_CONFIG_ENABLE_ASTC "Build ASTC codec." FALSE)
option(OGRE_CONFIG_ENABLE_QUAD_BUFFER_STEREO "Enable stereoscopic 3D support" FALSE)
cmake_dependent_option(OGRE_CONFIG_AMD_AGS "Enable AMD GPU Service library for D3D vendor extensions" TRUE "OGRE_BUILD_RENDERSYSTEM_D3D11 AND AMDAGS_FOUND" FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_ZIP "Build ZIP archive support. If you disable this option, you cannot use ZIP archives resource locations. The samples won't work." TRUE "ZZip_FOUND" FALSE)
option(OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE "Include Viewport orientation mode support." FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_GLES2_GLSL_OPTIMISER "Enable GLSL optimiser use in GLES 2 render system" FALSE "OGRE_BUILD_RENDERSYSTEM_GLES2" FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_GLES2_VAO_SUPPORT "Use VertexArrayObjects if possible." FALSE "OGRE_BUILD_RENDERSYSTEM_GLES2" FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_GL_STATE_CACHE_SUPPORT "Enable OpenGL state cache management" FALSE "OGRE_BUILD_RENDERSYSTEM_GL OR OGRE_BUILD_RENDERSYSTEM_GLES OR OGRE_BUILD_RENDERSYSTEM_GLES2 OR OGRE_BUILD_RENDERSYSTEM_GL3PLUS" FALSE)
cmake_dependent_option(OGRE_CONFIG_ENABLE_GLES3_SUPPORT "Enable OpenGL ES 3.x Features" TRUE "OPENGLES3_FOUND;OGRE_BUILD_RENDERSYSTEM_GLES2" FALSE)
cmake_dependent_option(OGRE_CONFIG_RENDERDOC_INTEGRATION "Programmatically schedule captures when launched from RenderDoc" TRUE "NOT APPLE;RenderDoc_FOUND" FALSE)
option(OGRE_CONFIG_ENABLE_FINE_LIGHT_MASK_GRANULARITY "Forward+ will support setLightMask, for a fine granularity control over which lights affect which objects. Performance impact may vary (may be slower, may be faster if you filter a lot of lights)" FALSE)
option(OGRE_CONFIG_ENABLE_TBB_SCHEDULER "Enable TBB's scheduler initialisation/shutdown." TRUE)
option(OGRE_CONFIG_ENABLE_LIGHT_OBB_RESTRAINT "Enable Light OBB restraints" FALSE)

cmake_dependent_option( OGRE_BUILD_COMPONENT_SCENE_FORMAT "Component to export and import scenes (meshes, textures, items, entities)" TRUE "OGRE_CONFIG_ENABLE_JSON" FALSE )

# Customise what to install
option(OGRE_INSTALL_SAMPLES "Install Ogre demos." TRUE)
option(OGRE_INSTALL_TOOLS "Install Ogre tools." TRUE)
option(OGRE_INSTALL_DOCS "Install documentation." TRUE)
option(OGRE_INSTALL_SAMPLES_SOURCE "Install samples source files." FALSE)
cmake_dependent_option(OGRE_INSTALL_PDB "Install debug pdb files" TRUE "MSVC" FALSE)
cmake_dependent_option(OGRE_FULL_RPATH "Build executables with the full required RPATH to run from their install location." FALSE "NOT WIN32" FALSE)
option(OGRE_SIMD_SSE2 "Enable SIMD (Include SSE2 files)." TRUE)
option(OGRE_SIMD_NEON "Enable SIMD (Include NEON files)." TRUE)
option(OGRE_RESTRICT_ALIASING "Restrict aliasing." TRUE)
option(OGRE_IDSTRING_ALWAYS_READABLE "Always keep readable strings on IdString, even in Release builds." FALSE)
cmake_dependent_option(OGRE_CONFIG_STATIC_LINK_CRT "Statically link the MS CRT dlls (msvcrt)" FALSE "MSVC" FALSE)
set(OGRE_LIB_DIRECTORY "lib${LIB_SUFFIX}" CACHE STRING "Install path for libraries, e.g. 'lib64' on some 64-bit Linux distros.")
if (WIN32)
	option(OGRE_INSTALL_VSPROPS "Install Visual Studio Property Page." FALSE)
	if (OGRE_INSTALL_VSPROPS)
		configure_file(${OGRE_TEMPLATES_DIR}/OGRE.props.in ${OGRE_BINARY_DIR}/OGRE.props)
		install(FILES ${OGRE_BINARY_DIR}/OGRE.props DESTINATION "${CMAKE_INSTALL_PREFIX}")
	endif ()
endif ()

# Unity build options
# A Unity build includes all sources files in just a few actual compilation units
# to potentially speed up the compilation.
option(OGRE_UNITY_BUILD "Enable unity build for Ogre." FALSE)
set(OGRE_UNITY_FILES_PER_UNIT "50" CACHE STRING "Number of files per compilation unit in Unity build.")

option(OGRE_PROFILING_TEXTURES "Enables profiling of texture loading. This is independent from OGRE_PROFILING_PROVIDER. Overhead is low so you may want to enable it without much worrying about performance." FALSE)

set( OGRE_PROFILING_PROVIDER "none" CACHE STRING
	"Enable profiling of Ogre for performance analysis. Profiling has a performance
	overhead so you may want to ship your game with this option disabled.
	none - Profiling OFF
	internal - Use internal profiling with on-screen overlays
	remotery - Use Remotery. https://github.com/Celtoys/Remotery
	offline - Use internal profiling that generates a CSV file for offline analysis"
)
option(OGRE_PROFILING_EXHAUSTIVE "When a valid profiler provider is set, includes exhaustive information of Ogre calls to better find culprit of big slowdowns or hitches, particularly why load times are slow. Best used with 'offline' profiler provider" FALSE)

if( NOT Remotery_FOUND AND OGRE_PROFILING_PROVIDER STREQUAL "remotery" )
	message( STATUS "WARNING: Remotery not found! Forcing profiling to disable" )
	set( OGRE_PROFILING_PROVIDER "none" )
endif()

if( OGRE_ADDRESS_SANITIZER_ASAN )
	if( APPLE )
		set( CMAKE_XCODE_GENERATE_SCHEME ON )
		set( CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER ON )
		set( CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN ON )
	elseif( UNIX )
		set( CMAKE_C_FLAGS
			"${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address" )
		set( CMAKE_CXX_FLAGS
			"${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address" )
		set( CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address" )
	elseif( MSVC )
		set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} /fsanitize=address" )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address" )
	endif()
endif()

# hide advanced options
mark_as_advanced(
  OGRE_ADDRESS_SANITIZER_ASAN
  OGRE_CONFIG_DOUBLE
  OGRE_CONFIG_NODE_INHERIT_TRANSFORM
  OGRE_CONFIG_ALLOCATOR
  OGRE_CONFIG_CONTAINERS_USE_CUSTOM_ALLOCATOR
  OGRE_CONFIG_STRING_USE_CUSTOM_ALLOCATOR
  OGRE_CONFIG_MEMTRACK_DEBUG
  OGRE_CONFIG_MEMTRACK_RELEASE
  OGRE_CONFIG_ENABLE_MESHLOD
  OGRE_CONFIG_ENABLE_DDS
  OGRE_CONFIG_ENABLE_FREEIMAGE
  OGRE_CONFIG_ENABLE_PVRTC
  OGRE_CONFIG_ENABLE_ETC
  OGRE_CONFIG_ENABLE_STBI
  OGRE_CONFIG_ENABLE_ASTC
  OGRE_CONFIG_ENABLE_VIEWPORT_ORIENTATIONMODE
  OGRE_CONFIG_ENABLE_ZIP
  OGRE_CONFIG_ENABLE_GL_STATE_CACHE_SUPPORT
  OGRE_CONFIG_ENABLE_GLES2_GLSL_OPTIMISER
  OGRE_CONFIG_ENABLE_GLES2_VAO_SUPPORT
  OGRE_CONFIG_ENABLE_GLES3_SUPPORT
  OGRE_CONFIG_RENDERDOC_INTEGRATION
  OGRE_CONFIG_ENABLE_TBB_SCHEDULER
  OGRE_CONFIG_ENABLE_FINE_LIGHT_MASK_GRANULARITY
  OGRE_CONFIG_ENABLE_LIGHT_OBB_RESTRAINT
  OGRE_INSTALL_SAMPLES_SOURCE
  OGRE_FULL_RPATH
  OGRE_PLUGIN_LIB_PREFIX
  OGRE_PROFILING_PROVIDER
  OGRE_CONFIG_STATIC_LINK_CRT
  OGRE_LIB_DIRECTORY
)

###################################################################
# configure global build settings based on selected build options
###################################################################
include(ConfigureBuild)

###################################################################
# disable (useless) compiler warnings on project level
###################################################################
if(MSVC)
	add_definitions( /wd4786 /wd4503 /wd4251 /wd4275 /wd4290 /wd4661 /wd4996 /wd4127 /wd4100)
endif()

##################################################################
# Now setup targets
##################################################################

# install resource files
include(InstallResources)

# Setup OgreMain project
add_subdirectory(OgreMain)

# Setup RenderSystems
add_subdirectory(RenderSystems)

# Setup Plugins
add_subdirectory(PlugIns)

# Setup Components
add_subdirectory(Components)

# Setup tests (before samples so that PlayPen is included in browser)
if (OGRE_BUILD_TESTS AND 0)
  # enable CTest
  ENABLE_TESTING()
  INCLUDE(CTest)
  add_subdirectory(Tests)
endif ()

# Setup samples
add_subdirectory(Samples)

# Add android JNI binding
if(ANDROID AND OGRE_BUILD_ANDROID_JNI_SAMPLE)
  include(toolchain/AndroidJNI)
endif()

# Setup command-line tools
if (OGRE_BUILD_TOOLS)
  add_subdirectory(Tools)
endif ()

# Setup XSIExporter
if (OGRE_BUILD_XSIEXPORTER)
  add_subdirectory(Tools/XSIExport)
endif ()

# Install documentation
add_subdirectory(Docs)

# Install media files
if (OGRE_INSTALL_SAMPLES OR OGRE_INSTALL_SAMPLES_SOURCE)
  add_subdirectory(Samples/Media)
endif ()

# Install CMake modules
add_subdirectory(CMake)

# provide option to install dependencies on Windows
include(InstallDependencies)

# Provide CPack packaging target
include(Packaging)

# Show feature summary
include(OgreFeatureSummary)
