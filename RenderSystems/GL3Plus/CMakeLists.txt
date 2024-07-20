#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure OpenGL 3+ RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/Vao/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Vao/*.cpp")

file(GLOB GLSL_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/GLSL/*.h")
file(GLOB GLSL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/GLSL/*.cpp")

# Add system specific settings
if( OGRE_GLSUPPORT_USE_WGL )
	file(GLOB PLATFORM_HEADERS "include/windowing/win32/*.h" "src/windowing/win32/*.h")
	file(GLOB PLATFORM_SOURCES "src/windowing/win32/*.cpp")

	include_directories( include/windowing/win32 )
	if( OGRE_GLSUPPORT_NUM_INTERFACES EQUAL 1 )
		include_directories( src/windowing/win32 )
	endif()

	source_group(GLSL FILES ${GLSL_SOURCE} ${GLSL_HEADERS})
endif()

if( OGRE_GLSUPPORT_USE_COCOA )
	file(GLOB PLATFORM_HEADERS "include/windowing/OSX/*.h" "src/windowing/OSX/*.h")
	file(GLOB PLATFORM_SOURCES "src/windowing/OSX/*.mm")
	set( PLATFORM_HEADERS_OSX ${PLATFORM_HEADERS} )

	include_directories( include/windowing/OSX "${OGRE_SOURCE_DIR}/OgreMain/include/OSX" )
	if( OGRE_GLSUPPORT_NUM_INTERFACES EQUAL 1 )
		include_directories( src/windowing/OSX )
	endif()

	set(PLATFORM_LIBS ${Cocoa_LIBRARY_FWK})
endif()

if( OGRE_GLSUPPORT_USE_GLX )
	file(GLOB PLATFORM_HEADERS "include/windowing/GLX/*.h" "src/windowing/GLX/*.h" )
	file(GLOB PLATFORM_SOURCES "src/windowing/GLX/*.cpp")
	set( PLATFORM_HEADERS_GLX ${PLATFORM_HEADERS} )

	include_directories( include/windowing/GLX )
	if( OGRE_GLSUPPORT_NUM_INTERFACES EQUAL 1 )
		include_directories( src/windowing/GLX )
	endif()

	set(PLATFORM_LIBS ${X11_LIBRARIES} ${X11_Xrandr_LIB})
endif()

if( OGRE_GLSUPPORT_USE_EGL_HEADLESS )
	file(GLOB PLATFORM_HEADERS_TMP "include/windowing/EGL/PBuffer/*.h")
	file(GLOB PLATFORM_SOURCES_TMP "src/windowing/EGL/PBuffer/*.cpp")
	list( APPEND PLATFORM_HEADERS ${PLATFORM_HEADERS_TMP} )
	list( APPEND PLATFORM_SOURCES ${PLATFORM_SOURCES_TMP} )
	set( PLATFORM_HEADERS_EGL ${PLATFORM_HEADERS_TMP} )
	include_directories( include/windowing/EGL )

	if( OGRE_GLSUPPORT_NUM_INTERFACES EQUAL 1 )
		include_directories( src/windowing/EGL )
	endif()

	list( APPEND PLATFORM_LIBS ${EGL_LIBRARIES} )
endif()

if( OGRE_GLSUPPORT_NUM_INTERFACES GREATER 1 )
	list( APPEND SOURCE_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/windowing/OgreGlSwitchableSupport.cpp" )
	list( APPEND HEADER_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/include/windowing/OgreGlSwitchableSupport.h" )

	include_directories( src/windowing )
endif()

set(HEADER_FILES ${HEADER_FILES} ${GLSL_HEADERS})

include_directories(
  BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/GLSL
)

if(NOT APPLE)
  include_directories(
    ${OPENGL_INCLUDE_DIR}
  )
  include_directories(
	include/GL
  )
endif()

if( OGRE_GLSUPPORT_USE_GLX )
  # Separate GLSupport and Window out to fix Unity builds
  ogre_add_library(RenderSystem_GL3Plus
	  ${OGRE_LIB_TYPE} ${HEADER_FILES} ${GLSL_SOURCE} ${PLATFORM_HEADERS} ${PLATFORM_SOURCES}
	  ${SOURCE_FILES}
	  SEPARATE "${CMAKE_CURRENT_SOURCE_DIR}/src/windowing/GLX/OgreGLXContext.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/windowing/GLX/OgreGLXWindow.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/windowing/GLX/OgreGLXGLSupport.cpp")
else()
  ogre_add_library(RenderSystem_GL3Plus
	  ${OGRE_LIB_TYPE} ${HEADER_FILES} ${GLSL_SOURCE} ${PLATFORM_HEADERS}
	  ${PLATFORM_SOURCES} ${SOURCE_FILES})
endif()
target_link_libraries(RenderSystem_GL3Plus ${OGRE_NEXT}Main ${OPENGL_LIBRARIES} ${PLATFORM_LIBS})

if (NOT OGRE_STATIC)
  set_target_properties(RenderSystem_GL3Plus PROPERTIES
    COMPILE_DEFINITIONS OGRE_GLPLUGIN_EXPORTS
  )
endif ()
if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_GL3Plus ${OGRE_THREAD_LIBRARIES})
endif ()

if (APPLE)
    set_target_properties(RenderSystem_GL3Plus PROPERTIES
        LINK_FLAGS "-framework Cocoa -framework OpenGL")
endif ()

ogre_config_framework(RenderSystem_GL3Plus)

ogre_config_plugin(RenderSystem_GL3Plus)
install(FILES ${HEADER_FILES} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus)
if( OGRE_GLSUPPORT_USE_GLX )
	install(FILES ${PLATFORM_HEADERS_GLX} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus/GLX)
endif()
if( OGRE_GLSUPPORT_USE_COCOA )
	install(FILES ${PLATFORM_HEADERS_OSX} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus/OSX)
endif()
if( OGRE_GLSUPPORT_USE_EGL_HEADLESS )
	install(FILES ${PLATFORM_HEADERS_EGL} DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus/EGL)
endif()
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/GLSL/ DESTINATION include/${OGRE_NEXT_PREFIX}/RenderSystems/GL3Plus)
