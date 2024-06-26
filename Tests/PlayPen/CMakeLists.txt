#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure PlayPen build

set(HEADER_FILES 
	include/PlayPen.h
	include/PlayPenSamples.h)
set(SOURCE_FILES 
    src/PlayPen.cpp
    src/PlayPenSamples.cpp)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OGRE_SOURCE_DIR}/Samples/Common/include)
ogre_add_component_include_dir(Terrain)
ogre_add_component_include_dir(Overlay)
ogre_add_component_include_dir(MeshLodGenerator)
ogre_add_component_include_dir(Paging)
ogre_add_component_include_dir(RTShaderSystem)

add_library(PlayPen ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES} )

target_link_libraries(PlayPen ${OGRE_LIBRARIES} ${OIS_LIBRARIES} OgreOverlay)
ogre_config_sample_lib(PlayPen)

if (APPLE AND NOT OGRE_BUILD_PLATFORM_APPLE_IOS)
    # Set the INSTALL_PATH so that Samples can be installed in the application package
    set_target_properties(PlayPen
       PROPERTIES BUILD_WITH_INSTALL_RPATH 1
       INSTALL_NAME_DIR "@executable_path/../Plugins"
    )
endif()

