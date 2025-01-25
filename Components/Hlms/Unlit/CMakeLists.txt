#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Ogre HLMS User Interface (2D) for OpenGL 3.3+ and D3D11 build


file(
    GLOB HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Common/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Common/include/*.inl"
)
file(
    GLOB SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Common/src/*.cpp"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common/include)

add_definitions( -DOgreHlmsUnlit_EXPORTS )

ogre_add_library(${OGRE_NEXT}HlmsUnlit ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${OGRE_NEXT}HlmsUnlit PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}HlmsUnlit ${OGRE_NEXT}Main)

ogre_config_framework(${OGRE_NEXT}HlmsUnlit)
ogre_config_component(${OGRE_NEXT}HlmsUnlit)

install (DIRECTORY include/
        DESTINATION include/${OGRE_NEXT_PREFIX}/Hlms/Unlit
        FILES_MATCHING PATTERN "*.h"
        )

# also install HLMS Common folder
install (DIRECTORY ../Common/include/
        DESTINATION include/${OGRE_NEXT_PREFIX}/Hlms/Common
        FILES_MATCHING PATTERN "*.h" PATTERN "*.inl"
        )
