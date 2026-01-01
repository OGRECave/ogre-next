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
ogre_add_component_include_dir(Hlms/Unlit)
ogre_add_component_include_dir(Overlays)

add_recursive( ./ SOURCE_FILES )

ogre_add_executable(Sample_ShadowMapDebugging WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Sample_ShadowMapDebugging ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES})
ogre_config_sample_lib(Sample_ShadowMapDebugging)
ogre_config_sample_pkg(Sample_ShadowMapDebugging)
