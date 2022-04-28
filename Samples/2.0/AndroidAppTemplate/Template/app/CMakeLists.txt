cmake_minimum_required(VERSION 3.4.1)

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

# The order in which libs are linked MATTERS
set( OGRE_LIBS
	optimized ${OGRE_BINARIES}/lib/lib%%prefixSampleName%%.a
	debug ${OGRE_BINARIES}/lib/lib%%prefixSampleName%%_d.a

	optimized ${OGRE_BINARIES}/lib/libOgreSamplesCommon.a
	debug ${OGRE_BINARIES}/lib/libOgreSamplesCommon_d.a

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

target_link_libraries( %%sampleName%% app-glue log android vulkan ${OGRE_LIBS} cpufeatures )
