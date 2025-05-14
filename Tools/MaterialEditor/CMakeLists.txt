#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

project( MaterialEditor )

cmake_minimum_required(VERSION 3.10...4.0)

include( CMake/Dependencies/wxWidgets.cmake )

if( APPLE )
	macro( add_recursive dir retVal )
		file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c ${dir}/*.mm ${dir}/*.m )
	endmacro()
else()
	macro( add_recursive dir retVal )
		file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c )
	endmacro()
endif()

setupWxWidgets( wxWidgets_LIBRARIES )

add_recursive( ./src SOURCES )
add_recursive( ./include HEADERS )

# Add Visual Studio filters.
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )

# Shamelessly include a single file from the Samples.
list( APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../Samples/2.0/Common/src/Utils/HdrUtils.cpp )
list( APPEND HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../../Samples/2.0/Common/include/Utils/HdrUtils.h )

add_executable( ${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} )
target_link_libraries( ${PROJECT_NAME} ${OGRE_NEXT}Main ${OGRE_NEXT}HlmsPbs ${OGRE_NEXT}HlmsUnlit ${wxWidgets_LIBRARIES} )

target_include_directories( ${PROJECT_NAME} PRIVATE "./include"
	"${CMAKE_SOURCE_DIR}/Components/Hlms/Common/include"
	"${CMAKE_SOURCE_DIR}/Components/Hlms/Pbs/include"
	"${CMAKE_SOURCE_DIR}/Components/Hlms/Unlit/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../Samples/2.0/Common/include"
)

if( UNIX )
	target_link_libraries( ${PROJECT_NAME} ${GTK3_LIBRARIES} )
endif()

