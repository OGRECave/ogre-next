#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Direct3D11 RenderSystem build

file( GLOB HEADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Vao/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/VendorExtensions/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Windowing/WIN32/*.h"
)
file( GLOB SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vao/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/VendorExtensions/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windowing/WIN32/*.cpp"
)

if(OGRE_CONFIG_ENABLE_QUAD_BUFFER_STEREO)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${DirectX11_INCLUDE_DIR} ${AMDQBS_INCLUDE_DIR} ${NVAPI_INCLUDE_DIR})
  link_directories(${DirectX11_LIBRARY_DIR} ${NVAPI_LIBRARY_DIR})
else()
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${DirectX11_INCLUDE_DIR})
  link_directories(${DirectX11_LIBRARY_DIR})
endif()

if( OGRE_CONFIG_AMD_AGS )
  include_directories( ${AMDAGS_INCLUDE_DIR} )
  link_directories( ${AMDAGS_LIBRARY_DIR} )
endif()

add_definitions(-D_USRDLL)

ogre_add_library(RenderSystem_Direct3D11 ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RenderSystem_Direct3D11
  ${OGRE_NEXT}Main
  ${DirectX11_LIBRARIES}
)

if (NOT OGRE_STATIC)
  set_target_properties(RenderSystem_Direct3D11 PROPERTIES
    COMPILE_DEFINITIONS OGRED3DENGINEDLL_EXPORTS
  )
endif ()
if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_Direct3D11 ${OGRE_THREAD_LIBRARIES})
endif ()
if(OGRE_CONFIG_ENABLE_QUAD_BUFFER_STEREO)
  target_link_libraries(RenderSystem_Direct3D11 ${NVAPI_LIBRARIES})
endif()
if( OGRE_CONFIG_AMD_AGS )
  target_link_libraries(RenderSystem_Direct3D11 ${AMDAGS_LIBRARIES})
endif()

ogre_config_plugin(RenderSystem_Direct3D11)
install(FILES ${HEADER_FILES} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/Direct3D11)
