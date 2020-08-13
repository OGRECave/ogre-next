#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Vulkan RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/Vao/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/SPIRV-Reflect/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Vao/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/SPIRV-Reflect/*.c")

if( LINUX )
	file( GLOB PLATFORM_SPECIFIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/X11/*.h" )
	file( GLOB PLATFORM_SPECIFIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/X11/*.cpp" )
elseif( WIN32 )
	file( GLOB PLATFORM_SPECIFIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/win32/*.h" )
	file( GLOB PLATFORM_SPECIFIC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/win32/*.cpp" )
endif()

set( HEADER_FILES ${HEADER_FILES} ${PLATFORM_SPECIFIC_HEADER} )
set( SOURCE_FILES ${SOURCE_FILES} ${PLATFORM_SPECIFIC_SRC} )

include_directories(
  BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# TODO These paths are hardcoded
if( WIN32 )
	include_directories(
		C:/VulkanSDK/1.2.131.2/Include
		C:/VulkanSDK/1.2.131.2/glslang
		C:/VulkanSDK/1.2.131.2/Include/glslang
	)
else()
	include_directories(
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/include
	)
endif()

ogre_add_library(RenderSystem_Vulkan ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RenderSystem_Vulkan OgreMain)

if( WIN32 )
	link_directories( C:/VulkanSDK/1.2.131.2/lib )
	target_link_libraries(RenderSystem_Vulkan vulkan
		C:/VulkanSDK/1.2.131.2/Lib/shaderc_combined.lib
		C:/VulkanSDK/1.2.131.2/Lib/VkLayer_utils.lib
		C:/VulkanSDK/1.2.131.2/Lib/vulkan-1.lib
	)
else()
	link_directories( /home/matias/Projects/vulkan/1.1.121.1/x86_64/lib )
	target_link_libraries(RenderSystem_Vulkan vulkan
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/lib/libglslang.a
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/lib/libOSDependent.a
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/lib/libOGLCompiler.a
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/lib/libHLSL.a
		/home/matias/Projects/vulkan/1.1.121.1/x86_64/lib/libSPIRV.a )
endif()

if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_Vulkan ${OGRE_THREAD_LIBRARIES})
endif ()

ogre_config_framework(RenderSystem_Vulkan)

ogre_config_plugin(RenderSystem_Vulkan)
install(FILES ${HEADER_FILES} DESTINATION include/OGRE/RenderSystems/Vulkan)
install(FILES ${PLATFORM_HEADERS} DESTINATION include/OGRE/RenderSystems/Vulkan/${PLATFORM_HEADER_INSTALL})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION include/OGRE/RenderSystems/Vulkan)
