#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------


PROJECT(${OGRE_NEXT}Imgui)

file(
	GLOB HEADER_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(
	GLOB SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${DearImgui_INCLUDE_DIRS})

file(GLOB IMGUI_SOURCES
	"${DearImgui_INCLUDE_DIRS}*.cpp"
)

if( NOT ANDROID AND NOT OGRE_BUILD_PLATFORM_APPLE_IOS AND SDL2_FOUND )
	list( APPEND IMGUI_SOURCES
		"${DearImgui_INCLUDE_DIRS}backends/imgui_impl_sdl2.cpp"
	)

	set( BROKEN_FILES_IN_UNITY_BUILD "${DearImgui_INCLUDE_DIRS}backends/imgui_impl_sdl2.cpp" )

	include_directories( ${SDL2_INCLUDE_DIR} )
else()
	set( BROKEN_FILES_IN_UNITY_BUILD "" )
endif()

add_definitions( -DOgreImgui_EXPORTS )
if( WIN32 )
	add_definitions( "-DIMGUI_API=__declspec( dllexport )" )
else()
	add_definitions( "-DIMGUI_API=__attribute__( ( visibility( \"default\" ) ) )" )
endif()

ogre_add_library(${OGRE_NEXT}Imgui ${OGRE_COMP_LIB_TYPE} ${IMGUI_SOURCES} ${HEADER_FILES} ${SOURCE_FILES}
	SEPARATE ${BROKEN_FILES_IN_UNITY_BUILD})
set_target_properties(${OGRE_NEXT}Imgui PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}Imgui ${OGRE_NEXT}Main)

if( NOT ANDROID AND NOT OGRE_BUILD_PLATFORM_APPLE_IOS AND SDL2_FOUND )
	target_link_libraries(${OGRE_NEXT}Imgui ${SDL2_LIBRARY})
endif()

ogre_config_framework(${OGRE_NEXT}Imgui)
ogre_config_component(${OGRE_NEXT}Imgui)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/Imgui
)
