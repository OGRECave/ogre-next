#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE-Next
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Plugins build

if (OGRE_BUILD_PLUGIN_PFX)
  add_subdirectory(ParticleFX)
endif()

if (OGRE_BUILD_PLUGIN_PFX2)
  add_subdirectory(ParticleFX2)
endif()
