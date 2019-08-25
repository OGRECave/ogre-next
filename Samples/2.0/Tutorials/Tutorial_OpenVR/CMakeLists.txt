#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
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

include_directories(${CMAKE_SOURCE_DIR}/Components/Hlms/Common/include)
ogre_add_component_include_dir(Hlms/Pbs)

add_recursive( ./ SOURCE_FILES )

if( UNIX )
	include_directories( /home/matias/openvr/headers )
	link_directories( /home/matias/openvr/lib/linux64 )
else()
	include_directories( C:/Projects/SDK/openvr/headers )
	link_directories( C:/Projects/SDK/openvr/lib/win64 )
endif()

ogre_add_executable(Tutorial_OpenVR WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${SAMPLE_COMMON_RESOURCES})

target_link_libraries(Tutorial_OpenVR ${OGRE_LIBRARIES} ${OGRE_SAMPLES_LIBRARIES} openvr_api)
ogre_config_sample_lib(Tutorial_OpenVR)

if( APPLE )
	set_target_properties( Tutorial_OpenVR PROPERTIES RESOURCE "${SAMPLE_COMMON_RESOURCES}" )
	set_target_properties( Tutorial_OpenVR PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${SAMPLES_PLIST} )
endif()
