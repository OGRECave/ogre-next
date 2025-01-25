#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# Overlay optional component
############################################################


# define header and source files for the library
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Add needed definitions 
add_definitions(-DOGRE_OVERLAY_EXPORTS -D_MT -D_USRDLL)

# include headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/OgreMain/include)

ogre_add_component_include_dir(Hlms/Unlit)
#ogre_add_component_include_dir(Hlms/UnlitMobile)
ogre_add_component_include_dir(Hlms/Common)

# setup target
ogre_add_library(${OGRE_NEXT}Overlay ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES} ${PLATFORM_HEADER_FILES} ${PLATFORM_SOURCE_FILES})
set_target_properties(${OGRE_NEXT}Overlay PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(${OGRE_NEXT}Overlay ${OGRE_NEXT}Main ${FREETYPE_LIBRARIES} ${OGRE_NEXT}HlmsUnlit )
if (OGRE_CONFIG_THREADS)
  target_link_libraries(${OGRE_NEXT}Overlay ${OGRE_THREAD_LIBRARIES})
endif ()

# install 
ogre_config_framework(${OGRE_NEXT}Overlay)
ogre_config_component(${OGRE_NEXT}Overlay)

install(FILES ${HEADER_FILES}
  DESTINATION include/${OGRE_NEXT_PREFIX}/Overlay
)

