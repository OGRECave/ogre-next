#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Vulkan RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/Vao/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Vao/*.cpp")

file( GLOB X11_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/X11/*.h" )
file( GLOB X11_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/X11/*.cpp" )

set( HEADER_FILES ${HEADER_FILES} ${X11_HEADER} )
set( SOURCE_FILES ${SOURCE_FILES} ${X11_SRC} )

include_directories(
  BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

ogre_add_library(RenderSystem_Vulkan ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
#target_link_libraries(RenderSystem_Vulkan OgreMain)

if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_Vulkan ${OGRE_THREAD_LIBRARIES})
endif ()

ogre_config_framework(RenderSystem_Vulkan)

ogre_config_plugin(RenderSystem_Vulkan)
install(FILES ${HEADER_FILES} DESTINATION include/OGRE/RenderSystems/Vulkan)
install(FILES ${PLATFORM_HEADERS} DESTINATION include/OGRE/RenderSystems/Vulkan/${PLATFORM_HEADER_INSTALL})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION include/OGRE/RenderSystems/Vulkan)
