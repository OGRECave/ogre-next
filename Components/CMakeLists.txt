#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Components build

# Use same build type for components as main build
set(OGRE_COMP_LIB_TYPE ${OGRE_LIB_TYPE})

if (OGRE_BUILD_COMPONENT_ATMOSPHERE)
    add_subdirectory(Atmosphere)
endif ()

if (OGRE_BUILD_COMPONENT_HLMS_PBS)
	add_subdirectory(Hlms/Pbs)
endif ()

if (OGRE_BUILD_COMPONENT_HLMS_UNLIT)
	add_subdirectory(Hlms/Unlit)
endif ()

if (OGRE_BUILD_COMPONENT_PAGING)
  add_subdirectory(Paging)
endif ()

if (OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS)
  add_subdirectory(PlanarReflections)
endif ()

if (OGRE_BUILD_COMPONENT_SCENE_FORMAT)
  add_subdirectory(SceneFormat)
endif ()

if (OGRE_BUILD_COMPONENT_MESHLODGENERATOR)
  add_subdirectory(MeshLodGenerator)
endif ()

if (OGRE_BUILD_COMPONENT_PROPERTY)
  add_subdirectory(Property)
endif ()

if (OGRE_BUILD_COMPONENT_VOLUME)
	add_subdirectory(Volume)
endif ()

if (OGRE_BUILD_COMPONENT_OVERLAY)
	add_subdirectory(Overlay)
endif ()

if (OGRE_BUILD_COMPONENT_DEAR_IMGUI)
    add_subdirectory(Imgui)
endif ()
