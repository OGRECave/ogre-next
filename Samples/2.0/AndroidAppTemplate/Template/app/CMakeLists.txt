cmake_minimum_required(VERSION 3.5.1)

# build native_app_glue as a static lib
set( APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue )
include_directories( ${APP_GLUE_DIR} )
add_library( app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c )

# build cpufeatures as well
set( ANDROID_CPU_FEATURES_DIR ${ANDROID_NDK}/sources/android/cpufeatures )
include_directories( ${ANDROID_CPU_FEATURES_DIR} )
add_library( cpufeatures STATIC ${ANDROID_CPU_FEATURES_DIR}/cpu-features.c )

set( OGRE_SOURCE %%ogreSource%% )
set( OGRE_BINARIES %%ogreBinaries%%/${CMAKE_BUILD_TYPE} )

if( EXISTS ${OGRE_BINARIES}/lib/libOgreNextMainStatic.a OR
	EXISTS ${OGRE_BINARIES}/lib/libOgreNextMainStatic_d.a )
	set( OGRE_NEXT "OgreNext" )
else()
	set( OGRE_NEXT "Ogre" )
endif()

# Read OgreBuildSettings.h to link against optional libraries (only if set)
file( READ "${OGRE_BINARIES}/include/OgreBuildSettings.h" OGRE_BUILD_SETTINGS_STR )
string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_COMPONENT_ATMOSPHERE" OGRE_BUILD_COMPONENT_ATMOSPHERE )
string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_PLUGIN_PFX" OGRE_BUILD_PLUGIN_PFX )
string( FIND "${OGRE_BUILD_SETTINGS_STR}" "#define OGRE_BUILD_PLUGIN_PFX2" OGRE_BUILD_PLUGIN_PFX2 )

# Read OgreVulkanBuildSettings.h to link against optional libraries (only if set)
file( READ "${OGRE_BINARIES}/include/OgreVulkanBuildSettings.h" OGRE_VK_BUILD_SETTINGS_STR )
string( FIND "${OGRE_VK_BUILD_SETTINGS_STR}" "#define OGRE_VULKAN_USE_SWAPPY" OGRE_VULKAN_USE_SWAPPY )

include_directories( ${OGRE_SOURCE}/OgreMain/include )
include_directories( ${OGRE_SOURCE}/RenderSystems/Vulkan/include )
include_directories( ${OGRE_SOURCE}/Components/Overlay/include )
include_directories( ${OGRE_SOURCE}/Components/Hlms/Common/include )
include_directories( ${OGRE_SOURCE}/Components/Hlms/Pbs/include )
include_directories( ${OGRE_SOURCE}/Components/Hlms/Unlit/include )
include_directories( ${OGRE_BINARIES}/include )
include_directories( ${OGRE_SOURCE}/DependenciesAndroid/include )
include_directories( ${OGRE_SOURCE}/Samples/2.0/Common/include )

# build vulkan app
set( SRC_DIR src/main/jni )

add_library( %%sampleName%% SHARED
			${SRC_DIR}/AndroidMain.cpp )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror  \
					 -Wno-error=unused-variable -Wno-error=delete-non-abstract-non-virtual-dtor \
					 -DVK_USE_PLATFORM_ANDROID_KHR" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 -D_DEBUG=1" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate" )

set( OGRE_OPTIONAL_LIBS "" )

if( NOT OGRE_BUILD_COMPONENT_ATMOSPHERE EQUAL -1 )
	set( OGRE_OPTIONAL_LIBS ${OGRE_OPTIONAL_LIBS}
		optimized ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}AtmosphereStatic.a
		debug ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}AtmosphereStatic_d.a
	)
endif()

if( NOT OGRE_BUILD_PLUGIN_PFX EQUAL -1 )
	set( OGRE_OPTIONAL_LIBS ${OGRE_OPTIONAL_LIBS}
		optimized ${OGRE_BINARIES}/lib/libPlugin_ParticleFXStatic.a
		debug ${OGRE_BINARIES}/lib/libPlugin_ParticleFXStatic_d.a
	)
endif()

if( NOT OGRE_BUILD_PLUGIN_PFX2 EQUAL -1 )
	set( OGRE_OPTIONAL_LIBS ${OGRE_OPTIONAL_LIBS}
		optimized ${OGRE_BINARIES}/lib/libPlugin_ParticleFX2Static.a
		debug ${OGRE_BINARIES}/lib/libPlugin_ParticleFX2Static_d.a
	)
endif()

# The order in which libs are linked MATTERS
set( OGRE_LIBS
	optimized ${OGRE_BINARIES}/lib/lib%%prefixSampleName%%.a
	debug ${OGRE_BINARIES}/lib/lib%%prefixSampleName%%_d.a

	optimized ${OGRE_BINARIES}/lib/libOgreSamplesCommon.a
	debug ${OGRE_BINARIES}/lib/libOgreSamplesCommon_d.a

	${OGRE_OPTIONAL_LIBS}

	optimized ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}HlmsPbsStatic.a
	optimized ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}HlmsUnlitStatic.a
	optimized ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}OverlayStatic.a
	optimized ${OGRE_BINARIES}/lib/libRenderSystem_VulkanStatic.a
	optimized ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}MainStatic.a

	debug ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}HlmsPbsStatic_d.a
	debug ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}HlmsUnlitStatic_d.a
	debug ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}OverlayStatic_d.a
	debug ${OGRE_BINARIES}/lib/libRenderSystem_VulkanStatic_d.a
	debug ${OGRE_BINARIES}/lib/lib${OGRE_NEXT}MainStatic_d.a

	${OGRE_SOURCE}/DependenciesAndroid/lib/libshaderc_combined.a
	${OGRE_SOURCE}/DependenciesAndroid/lib/libFreeImage.a
	${OGRE_SOURCE}/DependenciesAndroid/lib/libfreetype.a
	${OGRE_SOURCE}/DependenciesAndroid/lib/libzlib.a
	${OGRE_SOURCE}/DependenciesAndroid/lib/libzziplib.a )

if( OGRE_VULKAN_USE_SWAPPY )
	list( APPEND OGRE_LIBS ${OGRE_SOURCE}/DependenciesAndroid/lib/libswappy_static.a )
endif()

target_link_libraries( %%sampleName%% app-glue log android vulkan ${OGRE_LIBS} cpufeatures )
