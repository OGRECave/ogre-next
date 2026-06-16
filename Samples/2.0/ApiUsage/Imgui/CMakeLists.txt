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
include_directories(${DearImgui_INCLUDE_DIRS})

ogre_add_component_include_dir(Imgui)

add_recursive( ./ SOURCE_FILES )

ogre_add_executable(Sample_Imgui WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Sample_Imgui ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES} "${OGRE_NEXT}Imgui")
ogre_config_sample_lib(Sample_Imgui)
ogre_config_sample_pkg(Sample_Imgui)
