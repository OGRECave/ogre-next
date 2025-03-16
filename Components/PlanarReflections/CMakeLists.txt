#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------


PROJECT(${OGRE_NEXT}PlanarReflections)

file(
	GLOB HEADER_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(
	GLOB SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_definitions( -DOgrePlanarReflections_EXPORTS )

ogre_add_library(${OGRE_NEXT}PlanarReflections ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}PlanarReflections PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}PlanarReflections ${OGRE_NEXT}Main)
target_include_directories(${OGRE_NEXT}PlanarReflections PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

ogre_config_framework(${OGRE_NEXT}PlanarReflections)
ogre_config_component(${OGRE_NEXT}PlanarReflections)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/PlanarReflections
)
