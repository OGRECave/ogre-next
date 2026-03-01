#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

macro( add_recursive dir retVal )
	file( GLOB_RECURSE ${retVal} ${dir}/*.h ${dir}/*.cpp ${dir}/*.c )
endmacro()

include_directories(${CMAKE_SOURCE_DIR}/Components/Hlms/Common/include)
ogre_add_component_include_dir(Hlms/Pbs)

add_recursive( ./ SOURCE_FILES )

include_directories( ${OpenVR_INCLUDE_DIRS} )

set( ${SPECIFIC_RESOURCES} "" )
if( APPLE )
	set( ${SPECIFIC_RESOURCES} ${CMAKE_BINARY_DIR}/bin/HiddenAreaMeshVr.cfg )
endif()

ogre_add_executable(Tutorial_OpenVR WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES}
	${SPECIFIC_RESOURCES} )

target_link_libraries(Tutorial_OpenVR ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES} ${OpenVR_LIBRARIES})
ogre_config_sample_lib(Tutorial_OpenVR)
ogre_config_sample_pkg(Tutorial_OpenVR)
