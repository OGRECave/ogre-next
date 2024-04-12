#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Vulkan RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/Vao/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/SPIRV-Reflect/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Vao/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SPIRV-Reflect/*.c")

if( OGRE_VULKAN_WINDOW_ANDROID )
	file( GLOB PLATFORM_SPECIFIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/Android/*.h" )
	file( GLOB PLATFORM_SPECIFIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/Android/*.cpp" )
endif()
if( OGRE_VULKAN_WINDOW_XCB )
	file( GLOB PLATFORM_SPECIFIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/X11/*.h" )
	file( GLOB PLATFORM_SPECIFIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/X11/*.cpp" )
endif()
if( OGRE_VULKAN_WINDOW_WIN32 )
	file( GLOB PLATFORM_SPECIFIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/win32/*.h" )
	file( GLOB PLATFORM_SPECIFIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/win32/*.cpp" )
endif()

set( HEADER_FILES ${HEADER_FILES} ${PLATFORM_SPECIFIC_HEADER} )
set( SOURCE_FILES ${SOURCE_FILES} ${PLATFORM_SPECIFIC_SRC} )

include_directories(
  BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories( ${Vulkan_INCLUDE_DIRS} )
if( OGRE_VULKAN_USE_SWAPPY )
	include_directories( ${AndroidSwappy_INCLUDE_DIRS} )
endif()

ogre_add_library(RenderSystem_Vulkan ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RenderSystem_Vulkan ${OGRE_NEXT}Main ${Vulkan_LIBRARIES})

if( OGRE_VULKAN_WINDOW_XCB )
	target_link_libraries(RenderSystem_Vulkan xcb-randr X11-xcb)
endif()

if( OGRE_VULKAN_USE_SWAPPY )
	target_link_libraries(RenderSystem_Vulkan ${AndroidSwappy_LIBRARY})
endif()

if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_Vulkan ${OGRE_THREAD_LIBRARIES})
endif ()

ogre_config_framework(RenderSystem_Vulkan)

ogre_config_plugin(RenderSystem_Vulkan)
install(FILES ${HEADER_FILES} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/Vulkan)
install(FILES ${PLATFORM_HEADERS} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/Vulkan/${PLATFORM_HEADER_INSTALL})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/Vulkan)
