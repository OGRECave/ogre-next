set(OGRE_SAMPLES_COMMON_LIBRARY "OgreSamplesCommon")

# Sources
macro( add_recursive dir retVal )
	file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c )
endmacro()

if( ANDROID )
	# We depend on native_app_glue, but it is the appliaction which has to build it
	set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
	include_directories(${APP_GLUE_DIR})
endif()

file( GLOB SOURCE_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/System/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/System/Android/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Threading/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/*.cpp" )

if( OGRE_BUILD_PLATFORM_APPLE_IOS )
	file( GLOB IOS_SOURCE_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/System/iOS/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/System/iOS/*.mm" )
	set( SOURCE_FILES ${SOURCE_FILES} ${IOS_SOURCE_FILES} )
elseif( APPLE )
    file( GLOB OSX_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/System/Desktop/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/System/OSX/*.mm" )
	set( SOURCE_FILES ${SOURCE_FILES} ${OSX_SOURCE_FILES} )
elseif( NOT ANDROID )
	file( GLOB DESKTOP_SOURCE_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/System/Desktop/*.cpp" )
	set( SOURCE_FILES ${SOURCE_FILES} ${DESKTOP_SOURCE_FILES} )
endif()

add_recursive( ./include HEADER_FILES )

ogre_add_component_include_dir(Hlms/Pbs)
ogre_add_component_include_dir(Hlms/Unlit)
ogre_add_component_include_dir(Hlms/Common)

if( OGRE_BUILD_COMPONENT_ATMOSPHERE )
	ogre_add_component_include_dir(Atmosphere)
endif()

if( OGRE_STATIC )
	# Static linking means we need to directly use plugins
	include_directories( ${PROJECT_SOURCE_DIR}/PlugIns/ParticleFX/include )
	include_directories( ${PROJECT_SOURCE_DIR}/PlugIns/ParticleFX2/include )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/Direct3D11/include )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/GL3Plus/include )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/GL3Plus/include/GLSL )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/GLES2/include )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/Metal/include )
	include_directories( ${PROJECT_SOURCE_DIR}/RenderSystems/Vulkan/include )
endif()

add_library(${OGRE_SAMPLES_COMMON_LIBRARY} STATIC ${SOURCE_FILES} ${HEADER_FILES})

link_directories(${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${OGRE_SAMPLES_COMMON_LIBRARY} ${SDL2_LIBRARY} ${OGRE_NEXT}HlmsPbs ${OGRE_NEXT}HlmsUnlit)
if( OGRE_BUILD_COMPONENT_ATMOSPHERE )
	target_link_libraries(${OGRE_SAMPLES_COMMON_LIBRARY} ${OGRE_NEXT}Atmosphere)
endif()
