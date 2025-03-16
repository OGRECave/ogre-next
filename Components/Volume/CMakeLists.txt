#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# Volume optional component
############################################################

PROJECT(${OGRE_NEXT}Volume)

# define header and source files for the library
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Add needed definitions 
add_definitions(-DOGRE_VOLUME_EXPORTS -D_MT -D_USRDLL)

# include headers
include_directories(${OGRE_SOURCE_DIR}/OgreMain/include)

# setup target
ogre_add_library(${OGRE_NEXT}Volume ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES} ${PLATFORM_HEADER_FILES} ${PLATFORM_SOURCE_FILES})
set_target_properties(${OGRE_NEXT}Volume PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}Volume ${OGRE_NEXT}Main)
target_include_directories(${OGRE_NEXT}Volume PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (OGRE_CONFIG_THREADS)
  target_link_libraries(${OGRE_NEXT}Volume ${OGRE_THREAD_LIBRARIES})
endif ()


# install 
ogre_config_framework(${OGRE_NEXT}Volume)
ogre_config_component(${OGRE_NEXT}Volume)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/Volume
)

