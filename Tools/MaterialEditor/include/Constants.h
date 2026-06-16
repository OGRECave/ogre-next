//----------------------------------------------------------------------
//	Copyright (C) 2025-present Matias N. Goldberg ("dark_sylinc")
//  Under MIT License.
//----------------------------------------------------------------------

#pragma once

#include <OgreString.h>

// Resources in group "InternalMeshGroup" get erased and reinitialized all the time (to load the mesh).
// Resources in group "InternalPermanentMeshGroup" are loaded once and used by the application.
static const Ogre::String c_InternMeshGroup = "InternalMeshGroup";
static const Ogre::String c_InterMeshPermGroup = "InternalPermanentMeshGroup";

static const Ogre::String c_userSettingsFile = "UserSettings.cfg";
static const Ogre::String c_layoutSettingsFile = "GUI_Layout.cfg";
