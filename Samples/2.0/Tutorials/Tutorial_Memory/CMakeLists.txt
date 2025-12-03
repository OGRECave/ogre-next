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
ogre_add_component_include_dir(SceneFormat)

add_recursive( ./ SOURCE_FILES )

ogre_add_executable(Sample_Tutorial_Memory WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Sample_Tutorial_Memory ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES} ${OGRE_NEXT}SceneFormat)
ogre_config_sample_lib(Sample_Tutorial_Memory)
ogre_config_sample_pkg(Sample_Tutorial_Memory)
