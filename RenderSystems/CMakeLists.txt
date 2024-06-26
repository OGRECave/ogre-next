#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure RenderSystem build

if (OGRE_BUILD_RENDERSYSTEM_D3D11)
  if (NOT DirectX11_FOUND)
    MESSAGE(STATUS "Could not find dependency: DirectX")
  	MESSAGE(STATUS "Skipping Direct3D11 rendersystem build")
    set(OGRE_BUILD_RENDERSYSTEM_D3D11 0)
  else ()
    add_subdirectory(Direct3D11)
  endif ()
endif ()

if (OGRE_BUILD_RENDERSYSTEM_GL3PLUS)
  if (NOT OPENGL_FOUND)
    MESSAGE(STATUS "Could not find dependency: GL3Plus")
	  MESSAGE(STATUS "Skipping GL3Plus rendersystem build")
    set(OGRE_BUILD_RENDERSYSTEM_GL3PLUS 0)
  else ()
    add_subdirectory(GL3Plus)
  endif ()
endif ()

if (OGRE_BUILD_RENDERSYSTEM_GLES2)
#  if (NOT OPENGLES2_FOUND)
#    MESSAGE(STATUS "Could not find dependency: GLES2 ")
#	  MESSAGE(STATUS "Skipping GLES2 rendersystem build")
#    set(OGRE_BUILD_RENDERSYSTEM_GLES2 0)
#  else ()
#    add_subdirectory(GLES2)
#  endif()
    message( "GLES2 RenderSystem skipped. Currently broken" )
endif()

if (OGRE_BUILD_RENDERSYSTEM_METAL)
    add_subdirectory(Metal)
endif()

if (OGRE_BUILD_RENDERSYSTEM_VULKAN)
    add_subdirectory(Vulkan)
endif()

add_subdirectory(NULL)
