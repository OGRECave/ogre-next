#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# Paging optional component
############################################################

PROJECT(${OGRE_NEXT}Property)

# define header and source files for the library
set (HEADER_FILES
	include/OgreProperty.h
	include/OgrePropertyPrerequisites.h
)

set (SOURCE_FILES
	src/OgreProperty.cpp
)


# Add needed definitions 
add_definitions(-DOGRE_PROPERTY_EXPORTS -D_MT -D_USRDLL)

# include headers
include_directories(${OGRE_SOURCE_DIR}/OgreMain/include)

# setup target
ogre_add_library(${OGRE_NEXT}Property ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES} ${PLATFORM_HEADER_FILES} ${PLATFORM_SOURCE_FILES})
set_target_properties(${OGRE_NEXT}Property PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}Property ${OGRE_NEXT}Main)
target_include_directories(${OGRE_NEXT}Property PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (OGRE_CONFIG_THREADS)
  target_link_libraries(${OGRE_NEXT}Property ${OGRE_THREAD_LIBRARIES})
endif ()

# install 
ogre_config_framework(${OGRE_NEXT}Property)
ogre_config_component(${OGRE_NEXT}Property)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/Property
)

