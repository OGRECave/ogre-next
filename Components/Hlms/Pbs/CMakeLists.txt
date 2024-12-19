#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Ogre HLMS Physically Based Shading for OpenGL 3.3+ and D3D11 build


file(
	GLOB HEADER_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Cubemaps/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/InstantRadiosity/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/IrradianceField/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/LightProfiles/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/Vct/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Common/include/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Common/include/*.inl"
)
file(
	GLOB SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Cubemaps/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/InstantRadiosity/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/IrradianceField/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/LightProfiles/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Vct/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../Common/src/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/include)

if( OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS )
	ogre_add_component_include_dir( PlanarReflections )
endif()

add_definitions( -DOgreHlmsPbs_EXPORTS )

ogre_add_library(${OGRE_NEXT}HlmsPbs ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}HlmsPbs PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}HlmsPbs ${OGRE_NEXT}Main)

if( OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS )
	target_link_libraries( ${OGRE_NEXT}HlmsPbs ${OGRE_NEXT}PlanarReflections )
endif()

ogre_config_framework(${OGRE_NEXT}HlmsPbs)
ogre_config_component(${OGRE_NEXT}HlmsPbs)

message(STATUS "OGRE_NEXT_PREFIX: ${OGRE_NEXT_PREFIX}")

install (DIRECTORY include/
        DESTINATION include/${OGRE_NEXT_PREFIX}/Hlms/Pbs
        FILES_MATCHING PATTERN "*.h"
        )

# also install HLMS Common folder
install (DIRECTORY ../Common/include/
        DESTINATION include/${OGRE_NEXT_PREFIX}/Hlms/Common
        FILES_MATCHING PATTERN "*.h" PATTERN "*.inl"
        )
