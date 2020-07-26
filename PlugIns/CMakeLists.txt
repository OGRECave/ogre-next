#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Plugins build

if (OGRE_BUILD_PLUGIN_PFX)
  add_subdirectory(ParticleFX)
endif (OGRE_BUILD_PLUGIN_PFX)
