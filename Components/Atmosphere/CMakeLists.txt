#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------



file(
	GLOB HEADER_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)
file(
	GLOB SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions( -DOgreAtmosphere_EXPORTS )

ogre_add_library(${OGRE_NEXT}Atmosphere ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}Atmosphere PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}Atmosphere ${OGRE_NEXT}Main)

ogre_config_framework(${OGRE_NEXT}Atmosphere)
ogre_config_component(${OGRE_NEXT}Atmosphere)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/Atmosphere
)
