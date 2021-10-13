/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef _OgreHiddenAreaMesh_H_
#define _OgreHiddenAreaMesh_H_

#include "OgrePrerequisites.h"

#include "OgreVector2.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    class ConfigFile;

    struct HiddenAreaVrSettings
    {
        Ogre::Vector2 leftEyeCenter;
        Ogre::Vector2 leftEyeRadius;

        Ogre::Vector2 leftNoseCenter;
        Ogre::Vector2 leftNoseRadius;

        Ogre::Vector2 rightEyeCenter;
        Ogre::Vector2 rightEyeRadius;

        Ogre::Vector2 rightNoseCenter;
        Ogre::Vector2 rightNoseRadius;

        Ogre::uint32 tessellation;
    };

    class _OgreExport HiddenAreaMeshVrGenerator
    {
        static float *generate( Vector2 circleCenter, Vector2 radius, uint32 tessellation,
                                float circleDir, float fillDir, float eyeIdx,
                                float *RESTRICT_ALIAS vertexBuffer );

    public:
        static void generate( const String &meshName, const HiddenAreaVrSettings &setting );

        /** Fills HiddenAreaVrSettings from a config file
        @param deviceName
            Name of the device. Partial string matching will be carried against all sections
            in the config file.
        @param configFile
            Config file to read data from
        @return
            Forces tessellation to 0 if enabled = false
        */
        static HiddenAreaVrSettings loadSettings( const String &deviceName, ConfigFile &configFile );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
