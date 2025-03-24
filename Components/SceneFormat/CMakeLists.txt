#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------


PROJECT(${OGRE_NEXT}SceneFormat)

file(
	GLOB HEADER_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(
	GLOB SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
message("SCENEFORMAT")
include_directories(${OGRE_SOURCE_DIR}/Components/Hlms/Common/include)
ogre_add_component_include_dir(Hlms/Pbs)

add_definitions( -DOgreSceneFormat_EXPORTS )

ogre_add_library(${OGRE_NEXT}SceneFormat ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}SceneFormat PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}SceneFormat ${OGRE_NEXT}Main ${OGRE_NEXT}HlmsPbs)
target_include_directories(${OGRE_NEXT}SceneFormat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

ogre_config_framework(${OGRE_NEXT}SceneFormat)
ogre_config_component(${OGRE_NEXT}SceneFormat)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/SceneFormat
)
