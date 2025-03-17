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
#include "OgreRenderSystem.h"
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
    class _OgreAtmosphereExport AtmosphereNpr final : public AtmosphereComponent,
                                                      public RenderSystem::Listener
    {
    public:
        struct Preset
        {
            /// Must be in range [-1; 1] where range [-1; 0] is night and [0; 1] is day
            float time;

            float         densityCoeff;
            float         densityDiffusion;
            float         horizonLimit;  // Most relevant in sunsets and sunrises
            float         sunPower;      // For HDR (affects the sun on the sky)
            float         skyPower;      // For HDR (affects skyColour)
            Ogre::Vector3 skyColour;     // In range [0; 1]
            /// Affects objects' fog (not sky)
            float fogDensity;
            /// Very bright objects (i.e. reflect lots of light)
            /// manage to "breakthrough" the fog.
            ///
            /// A value of fogBreakMinBrightness = 3 means that pixels
            /// that have a luminance of >= 3 (i.e. in HDR) will start
            /// becoming more visible
            ///
            /// Range: [0; inf)
            float fogBreakMinBrightness;
            /// How fast bright objects appear in fog.
            /// Small values make objects appear very slowly after luminance > fogBreakMinBrightness
            /// Large values make objects appear very quickly
            ///
            /// Range: (0; inf)
            float fogBreakFalloff;

            /// Power scale for the linked light
            float linkedLightPower;
            /// Power scale for the upper hemisphere ambient light
            float linkedSceneAmbientUpperPower;
            /// Power scale for the lower hemisphere ambient light
            float linkedSceneAmbientLowerPower;
            /// Value to send to SceneManager::setAmbientLight
            float envmapScale;

            Preset() :
                time( 0.0f ),
                // densityCoeff( 0.27f ),
                // densityDiffusion( 0.75f ),
                densityCoeff( 0.47f ),
                densityDiffusion( 2.0f ),
                horizonLimit( 0.025f ),
                sunPower( 1.0f ),
                skyPower( 1.0f ),
                skyColour( 0.334f, 0.57f, 1.0f ),
                fogDensity( 0.0001f ),
                fogBreakMinBrightness( 0.25f ),
                fogBreakFalloff( 0.1f ),
                linkedLightPower( Math::PI ),
                linkedSceneAmbientUpperPower( 0.1f * Math::PI ),
                linkedSceneAmbientLowerPower( 0.01f * Math::PI ),
                envmapScale( 1.0f )
            {
            }

            /// Sets this = lerp( a, b, w );
            /// where:
            ///     a = lerp( a, b, 0 );
            ///     b = lerp( a, b, 1 );
            ///
            /// 'time' is interpolated in a special manner to wrap around
            /// in range [-1; 1]. Thus if a.time > b.time, then we wrap around
            /// so that:
            ///     time = lerp( 0.8, -0.8, 0.25 ) =  0.9
            ///     time = lerp( 0.8, -0.8, 0.50 ) =  Either 1.0 or -1.0
            ///     time = lerp( 0.8, -0.8, 0.75 ) = -0.9
            void lerp( const Preset &a, const Preset &b, const float w );

            bool operator()( const Preset &a, const Preset &b ) const { return a.time < b.time; }
            bool operator()( const float a, const Preset &b ) const { return a < b.time; }
            bool operator()( const Preset &a, const float b ) const { return a.time < b; }
        };

        typedef FastArray<Preset> PresetArray;

        enum AxisConvention
        {
            Xup,
            Yup,
            Zup,
            NegationFlag = ( 1u << 2u ),
            NegXup = Xup | NegationFlag,
            NegYup = Yup | NegationFlag,
            NegZup = Zup | NegationFlag,

        };

    protected:
        PresetArray mPresets;

        Preset  mPreset;
        Vector3 mSunDir;
        /// In range [0; 1] where
        float  mNormalizedTimeOfDay;
        Light *mLinkedLight;

    public:
        /// PUBLIC VARIABLE. This variable can be altered directly.
        /// Changes are reflected immediately.
        AxisConvention mConvention;
        /// When camera's height == mAtmosphereSeaLevel, the camera is considered to be at the ground
        /// i.e. camera_height - mAtmosphereSeaLevel = 0
        ///
        /// Regarding the unit of measurement, see mAtmosphereHeight
        float mAtmosphereSeaLevel;
        /// How big is the "atmosphere". Earth's thermosphere is at about 110 km.
        ///
        /// This value must be in "units". If your engine stores the camera in millimeters,
        /// this value must be in millimeters. If your engine uses meters, this value must
        /// be in meters.
        ///
        /// Thus the visible atmosphere is in range
        /// [mAtmosphereSeaLevel; mAtmosphereSeaLevel + mAtmosphereHeight)
        float mAtmosphereHeight;

    protected:
        MaterialPtr         mMaterial;
        Pass *ogre_nullable mPass;

        /// Contains all settings in a GPU buffer for Hlms to consume
        ConstBufferPacked *mHlmsBuffer;
        VaoManager        *mVaoManager;

        std::map<Ogre::SceneManager *, Rectangle2D *> mSkies;

        // We must clone the material in case there's more than one
        // AtmosphereNpr with different settings.
        void createMaterial();

        void setPackedParams();

        void syncToLight();

    public:
        AtmosphereNpr( VaoManager *vaoManager );
        ~AtmosphereNpr() override;

        /// @see RenderSystem::Listener
        void eventOccurred( const String &eventName, const NameValuePairList *parameters ) override;

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
            Sun's light direction (or moon). Will be normalized.
        @param normalizedTimeOfDay
            In range [0; 1] where 0 is when the sun goes out and 1 when it's gone.
        */
        void setSunDir( const Ogre::Vector3 &sunDir, const float normalizedTimeOfDay );

        /// Sets multiple presets at different times for interpolation
        /// We will sort the array.
        ///
        /// You must call AtmosphereNpr::updatePreset to take effect
        void setPresets( const PresetArray &presets );

        const PresetArray &getPresets() const { return mPresets; }

        /** After having called AtmosphereNpr::setPresets, this function will interpolate
            between mPresets[fTime] and mPresets[fTime+1].
        @param sunDir
            Sun's light direction (or moon). Will be normalized.
        @param fTime
            Param. Should be in range [-1; 1] where [-1; 0] is meant
            to be night and [0; 1] is day
        */
        void updatePreset( const Ogre::Vector3 &sunDir, const float fTime );

        /// Sets a specific preset as current.
        void setPreset( const Preset &preset );

        const Preset &getPreset() const { return mPreset; }

        void _update( SceneManager *sceneManager, Camera *camera ) override;

        uint32 preparePassHash( Hlms *hlms, size_t constBufferSlot ) override;

        uint32 getNumConstBuffersSlots() const override;

        uint32 bindConstBuffers( CommandBuffer *commandBuffer, size_t slotIdx ) override;

        /**
        @remarks
            We assume camera displacement is 0.
        @param cameraDir
            Point to look at
        @param bSkipSun
            When true, the sun disk is skipped
        @return
            The value at that camera direction, given the current parameters
        */
        Vector3 getAtmosphereAt( const Vector3 cameraDir, bool bSkipSun = false );
    };

    OGRE_ASSUME_NONNULL_END

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
