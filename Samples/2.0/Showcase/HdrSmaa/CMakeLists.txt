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

if( OGRE_BUILD_COMPONENT_ATMOSPHERE )
	ogre_add_component_include_dir(Atmosphere)
endif()
include_directories(${CMAKE_SOURCE_DIR}/Components/Hlms/Common/include)
ogre_add_component_include_dir(Hlms/Pbs)

add_recursive( ./ SOURCE_FILES )

ogre_add_executable(Sample_HdrSmaa WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Sample_HdrSmaa ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES})
if( OGRE_BUILD_COMPONENT_ATMOSPHERE )
	target_link_libraries(Sample_HdrSmaa ${OGRE_NEXT}Atmosphere)
endif()
ogre_config_sample_lib(Sample_HdrSmaa)
ogre_config_sample_pkg(Sample_HdrSmaa)
