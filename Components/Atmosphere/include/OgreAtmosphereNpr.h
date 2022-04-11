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
#ifndef _OgreAtmosphereNpr_H_
#define _OgreAtmosphereNpr_H_

#include "OgreAtmospherePrerequisites.h"

#include "OgreAtmosphereComponent.h"
#include "OgreColourValue.h"
#include "OgreSharedPtr.h"
#include "OgreVector3.h"

#include <map>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Component
     *  @{
     */
    /** \addtogroup Material
     *  @{
     */

    OGRE_ASSUME_NONNULL_BEGIN

    /** Non-physically-based-render (NPR) Atmosphere solution

        This solution is also integrated into HlmsPbs.

        It may seem counter-intuitive to include an NPR solution in a PBR solution (HlmsPbs)
        However it makes sense because it looks nice and is efficient for most cases.

        A PBR solution is iterative and requires more resources.
    */
    class _OgreAtmosphereExport AtmosphereNpr final : public AtmosphereComponent
    {
    public:
        struct Preset
        {
            float         densityCoeff;
            Ogre::Vector3 skyColour;

            Preset() : densityCoeff( 0.27f ), skyColour( 0.334f, 0.57f, 1.0f ) {}
        };

    protected:
        Preset  mPreset;
        Vector3 mSunDir;
        /// In range [0; 1] where
        float  mNormalizedTimeOfDay;
        Light *mLinkedLight;

        MaterialPtr         mMaterial;
        Pass *ogre_nullable mPass;

        std::map<Ogre::SceneManager *, Rectangle2D *> mSkies;

        // We must clone the material in case there's more than one
        // AtmosphereNpr with different settings.
        void createMaterial();

        void setPackedParams();

    public:
        AtmosphereNpr();
        ~AtmosphereNpr() override;

        void setSky( Ogre::SceneManager *sceneManager, bool bEnabled );
        void destroySky( Ogre::SceneManager *sceneManager );

        /** Links an existing directional to be updated using the Atmosphere's parameters
        @param light
            Light to associate with.
            Can be nullptr to unlink it.
            Light pointer must stay alive while linked (i.e. no dangling pointers)
        */
        void setLight( Light *ogre_nullable light );

        /** Sets the time of day.
        @remarks
            Assumes Y is up
        @param sunAltitude
            Altitude of the sun.
            At 90° the sun is facing downards (i.e. 12pm)
            *Must* be in range [0; pi)
        @param azimuth
            Rotation around Y axis. In radians.
        */
        void setSunDir( const Radian sunAltitude, const Ogre::Radian azimuth = Radian( 0.0f ) );

        /** More direct approach on setting time of day.
        @param sunDir
            Direction. Will be normalized.
        @param normalizedTimeOfDay
            In range [0; 1] where 0 is when the sun goes out and 1 when it's gone.
        */
        void setSunDir( const Ogre::Vector3 &sunDir, const float normalizedTimeOfDay );

        void setPreset( const Preset &preset );

        void _update( SceneManager *sceneManager, Camera *camera ) override;
    };

    OGRE_ASSUME_NONNULL_END

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
