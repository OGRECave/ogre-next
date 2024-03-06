/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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
#ifndef _OgreForwardPlusBase_H_
#define _OgreForwardPlusBase_H_

#include "OgrePrerequisites.h"

#include "OgreCommon.h"
#include "OgreLight.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    class CompositorShadowNode;

    static const size_t c_ForwardPlusNumFloat4PerLight = 6u;
    static const size_t c_ForwardPlusNumFloat4PerDecal = 4u;
    static const size_t c_ForwardPlusNumFloat4PerCubemapProbe = 8u;

    /** ForwardPlusBase */
    class _OgreExport ForwardPlusBase : public OgreAllocatedObj
    {
    public:
        enum ForwardPlusMethods
        {
            MethodForward3D,
            MethodForwardClustered,
            NumForwardPlusMethods
        };

        struct CachedGridBuffer
        {
            /// We use a TexBufferPacked instead of ReadOnlyBufferPacked because there is
            /// a considerable slowdown when trying to address a R16_UNORM buffer using
            /// bit unpacking, i.e. via:
            ///     (idx & 0x01) != 0u ? (bufferVar[idx >> 1u] >> 16u) : (bufferVar[idx >> 1u] & 0xFFFFu)
            ///
            /// RenderSystems which natively support ushort in UAVs wouldn't be affected
            /// but it's a lot of work maintaining so many paths and a major RenderSystem
            /// doesn't support it.
            ///
            /// Alternatively R32_UNORM could be used w/ ReadOnlyBufferPacked,
            /// but that's twice the memory (and less cache friendly).
            /// Only Android really could use ReadOnlyBufferPacked, but if you need to
            /// exceed the 65535 texel limit, it's likely too slow for phones anyway
            TexBufferPacked      *gridBuffer;
            ReadOnlyBufferPacked *globalLightListBuffer;
            CachedGridBuffer() : gridBuffer( 0 ), globalLightListBuffer( 0 ) {}
        };

        typedef vector<CachedGridBuffer>::type CachedGridBufferVec;

        static const uint32 MinDecalRq;  // Inclusive
        static const uint32 MaxDecalRq;  // Inclusive

        static const uint32 MinCubemapProbeRq;  // Inclusive
        static const uint32 MaxCubemapProbeRq;  // Inclusive

    protected:
        static const size_t NumBytesPerLight;
        static const size_t NumBytesPerDecal;
        static const size_t NumBytesPerCubemapProbe;

        struct CachedGrid
        {
            Camera    *camera;
            Vector3    lastPos;
            Quaternion lastRot;
            /// Cameras used for reflection have a different view proj matrix
            bool reflection;
            /// Cameras can change their AR depending on the RTT they're rendering to.
            Real   aspectRatio;
            uint32 visibilityMask;
            /// Cameras w/out shadows have a different light list from cameras that do.
            CompositorShadowNode const *shadowNode;
            /// Last frame this cache was updated.
            uint32 lastFrame;

            uint32              currentBufIdx;
            CachedGridBufferVec gridBuffers;
        };

        enum ObjTypes
        {
            ObjType_Decal = 0,
            ObjType_CubemapProbe,
            NumObjTypes
        };

        struct LightCount
        {
            // We use LT_DIRECTIONAL (index = 0) to contain the total light count.
            uint32 lightCount[Light::MAX_FORWARD_PLUS_LIGHTS];
            uint32 objCount[NumObjTypes];
            LightCount()
            {
                memset( lightCount, 0, sizeof( lightCount ) );
                memset( objCount, 0, sizeof( objCount ) );
            }
        };

        typedef vector<CachedGrid>::type CachedGridVec;
        CachedGridVec                    mCachedGrid;
        LightArray                       mCurrentLightList;

        FastArray<LightCount> mLightCountInCell;

        // Used to save and restore visibility of shadow casting lights. Lives here
        // to reuse memory, otherwise on stack it keeps constantly reallocating memory
        FastArray<bool> mShadowCastingLightVisibility;

        VaoManager   *mVaoManager;
        SceneManager *mSceneManager;

        bool mDebugMode;
        bool mFadeAttenuationRange;
        /// VPLs = Virtual Point Lights. Used by InstantRadiosity.
        bool mEnableVpls;
        bool mDecalsEnabled;
        bool mCubemapProbesEnabled;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        bool mFineLightMaskGranularity;
#endif
        // How many float4 to skip before Decals / CubemapProbes start in globalLightListBuffer
        uint16 mDecalFloat4Offset;
        uint16 mCubemapProbeFloat4Offset;

        static size_t calculateBytesNeeded( size_t numLights, size_t numDecals,
                                            size_t numCubemapProbes );

        void fillGlobalLightListBuffer( Camera *camera, TexBufferPacked *globalLightListBuffer );

        /** Finds a grid already cached in mCachedGrid that can be used for the given camera.
            If the cache does not exist, we create a new entry.
        @param camera
            The camera for which we'll find a cached entry.
        @param outCachedGrid
            The CachedGrid being retrieved. May be new or an existing one.
            This pointer may be invalidated on the next call to getCachedGridFor
        @return
            True if the cache is up to date. False if the cache needs to be updated.
        */
        bool getCachedGridFor( Camera *camera, CachedGrid **outCachedGrid );

        /// The const version will not create a new cache if not found, and
        /// output a null pointer instead (also returns false in that case).
        bool getCachedGridFor( const Camera *camera, const CachedGrid **outCachedGrid ) const;

        /// Check if some of the caches are really old and delete them
        void deleteOldGridBuffers();

    public:
        ForwardPlusBase( SceneManager *sceneManager, bool decalsEnabled, bool cubemapProbesEnabled );
        virtual ~ForwardPlusBase();

        virtual ForwardPlusMethods getForwardPlusMethod() const = 0;

        void _releaseManualHardwareResources();

        void _changeRenderSystem( RenderSystem *newRs );

        virtual void collectLights( Camera *camera ) = 0;

        bool isCacheDirty( const Camera *camera ) const;

        /// Cache the return value as internally we perform an O(N) search
        TexBufferPacked *getGridBuffer( const Camera *camera ) const;
        /// Cache the return value as internally we perform an O(N) search
        ReadOnlyBufferPacked *getGlobalLightListBuffer( const Camera *camera ) const;

        /// Returns the amount of bytes that fillConstBufferData is going to fill.
        virtual size_t getConstBufferSize() const = 0;

        /** Fills 'passBufferPtr' with the necessary data for ForwardPlusBase rendering.
            @see getConstBufferSize
        @remarks
            Assumes 'passBufferPtr' is aligned to a vec4/float4 boundary.
        */
        virtual void fillConstBufferData( Viewport *viewport, bool bRequiresTextureFlipping,
                                          uint32 renderTargetHeight, IdString shaderSyntax,
                                          bool                  instancedStereo,
                                          float *RESTRICT_ALIAS passBufferPtr ) const = 0;

        virtual void setHlmsPassProperties( const size_t tid, Hlms *hlms );

        /// Turns on visualization of light cell occupancy
        void setDebugMode( bool debugMode ) { mDebugMode = debugMode; }
        bool getDebugMode() const { return mDebugMode; }

        /// Attenuates the light by the attenuation range, causing smooth endings when
        /// at the end of the light range instead of a sudden sharp termination. This
        /// isn't physically based (light's range is infinite), but looks very well,
        /// and makes more intuitive to manipulate a light by controlling its range
        /// instead of controlling its radius. @see Light::setAttenuationBasedOnRadius
        /// and @see setAttenuation.
        /// And even when controlling the light by its radius, you don't have to worry
        /// so much about the threshold's value being accurate.
        /// It has a tendency to make lights dimmer though. That's the price to pay
        /// for this optimization and having more intuitive controls.
        /// Enabled by default.
        ///
        /// In math:
        ///     atten *= max( (attenRange - fDistance) / attenRange, 0.0f );
        void setFadeAttenuationRange( bool fade ) { mFadeAttenuationRange = fade; }
        bool getFadeAttenuationRange() const { return mFadeAttenuationRange; }

        void setEnableVpls( bool enable ) { mEnableVpls = enable; }
        bool getEnableVpls() const { return mEnableVpls; }

        bool getDecalsEnabled() const { return mDecalsEnabled; }

#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        /// Toggles whether light masks will be obeyed per object & per light by doing:
        /// if( movableObject->getLightMask() & light->getLightMask() )
        ///     doLighting( movableObject light );
        /// Note this toggle only affects Forward+ lights.
        /// You may want to see HlmsPbs::setFineLightMaskGranularity
        /// for control over Forward lights.
        /// If you only need coarse granularity control over Forward+ lights, you
        /// may get higher performance via CompositorPassSceneDef::mLightVisibilityMask
        /// (light_visibility_mask keyword in scripts).
        void setFineLightMaskGranularity( bool useFineGranularity )
        {
            mFineLightMaskGranularity = useFineGranularity;
        }
        bool getFineLightMaskGranularity() const { return mFineLightMaskGranularity; }
#endif
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
