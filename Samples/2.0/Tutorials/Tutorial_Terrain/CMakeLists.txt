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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_recursive( ./src SOURCE_FILES )
add_recursive( ./include HEADER_FILES )
set( SOURCE_FILES ${SOURCE_FILES} ${HEADER_FILES} )

include_directories( ./include )
include_directories( ./include/Hlms )
ogre_add_component_include_dir(Hlms/Common)
ogre_add_component_include_dir(Hlms/Pbs)
if( OGRE_BUILD_COMPONENT_ATMOSPHERE )
	ogre_add_component_include_dir(Atmosphere)
endif()
if( OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS )
    ogre_add_component_include_dir( PlanarReflections )
endif()

ogre_add_executable(Sample_Tutorial_Terrain WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Sample_Tutorial_Terrain ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES} ${OGRE_NEXT}HlmsPbs)
if( OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS )
    target_link_libraries( Sample_Tutorial_Terrain ${OGRE_NEXT}PlanarReflections )
endif()
ogre_config_sample_lib(Sample_Tutorial_Terrain)
ogre_config_sample_pkg(Sample_Tutorial_Terrain)
