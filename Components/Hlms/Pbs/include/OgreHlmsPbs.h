/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#ifndef _OgreHlmsPbs_H_
#define _OgreHlmsPbs_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreMatrix4.h"
#include "OgreHeaderPrefix.h"
#include "OgreRoot.h"

namespace Ogre
{
    class CompositorShadowNode;
    struct QueuedRenderable;
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
    class PlanarReflections;
#endif

    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

    class HlmsPbsDatablock;

    /** Physically based shading implementation specfically designed for
        OpenGL 3+, D3D11 and other RenderSystems which support uniform buffers.
    */
    class _OgreHlmsPbsExport HlmsPbs : public HlmsBufferManager, public ConstBufferPool
    {
    public:
        enum ShadowFilter
        {
            /// Standard quality. Very fast.
            PCF_2x2,

            /// Good quality. Still quite fast on most modern hardware.
            PCF_3x3,

            /// High quality. Very slow in old hardware (i.e. DX10 level hw and below)
            /// Use RSC_TEXTURE_GATHER to check whether it will be slow or not.
            PCF_4x4,

            /// High quality. Produces soft shadows. It's much more expensive but given
            /// its blurry results, you can reduce resolution and/or use less PSSM splits
            /// which gives you very competing performance with great results.
            /// ESM stands for Exponential Shadow Maps.
            ExponentialShadowMaps,

            NumShadowFilter
        };

        enum AmbientLightMode
        {
            /// Use fixed-colour ambient lighting when upper hemisphere = lower hemisphere,
            /// use hemisphere lighting when they don't match.
            /// Disables ambient lighting if the colours are black.
            AmbientAuto,

            /// Force fixed-colour ambient light. Only uses the upper hemisphere paramter.
            AmbientFixed,

            /// Force hemisphere ambient light. Useful if you plan on adjusting the colours
            /// dynamically very often and this might cause swapping shaders.
            AmbientHemisphere,

            /// Uses spherical harmonics
            AmbientSh,

            /// Uses spherical harmonics (monochrome / single channel)
            AmbientShMonochrome,

            /// Disable ambient lighting.
            AmbientNone
        };

    protected:
        typedef vector<ConstBufferPacked*>::type ConstBufferPackedVec;
        typedef vector<HlmsDatablock*>::type HlmsDatablockVec;

        struct PassData
        {
            FastArray<TextureGpu*> shadowMaps;
            FastArray<float>    vertexShaderSharedBuffer;
            FastArray<float>    pixelShaderSharedBuffer;

            Matrix4 viewMatrix;
        };

        PassData                mPreparedPass;
        ConstBufferPackedVec    mPassBuffers;
        ConstBufferPackedVec    mLight0Buffers; // lights
        ConstBufferPackedVec    mLight1Buffers; // areaApproxLights
        ConstBufferPackedVec    mLight2Buffers; // areaLtcLights
        HlmsSamplerblock const  *mShadowmapSamplerblock;    /// GL3+ only when not using depth textures
        HlmsSamplerblock const  *mShadowmapCmpSamplerblock; /// For depth textures & D3D11
        HlmsSamplerblock const  *mShadowmapEsmSamplerblock; /// For ESM.
        HlmsSamplerblock const  *mCurrentShadowmapSamplerblock;
        ParallaxCorrectedCubemapBase *mParallaxCorrectedCubemap;
        float                   mPccVctMinDistance;
        float                   mInvPccVctInvDistance;

        uint32                  mCurrentPassBuffer;     /// Resets to zero every new frame.

        TexBufferPacked         *mGridBuffer;
        TexBufferPacked         *mGlobalLightListBuffer;


        float                   mMaxSpecIblMipmap;
        uint32                  mTexUnitSlotStart;

        TextureGpuVec const     *mPrePassTextures;
        TextureGpu              *mPrePassMsaaDepthTexture;
        /// Used by techniques: SS Reflections, SS Refractions
        TextureGpu              *mDepthTexture;
        TextureGpu              *mSsrTexture;
        TextureGpu              *mDepthTextureNoMsaa;
        TextureGpu              *mRefractionsTexture;
        IrradianceVolume        *mIrradianceVolume;
        VctLighting             *mVctLighting;
        IrradianceField         *mIrradianceField;
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        //TODO: After texture refactor it should be possible to abstract this,
        //so we don't have to be aware of PlanarReflections class.
        PlanarReflections       *mPlanarReflections;
        HlmsSamplerblock const  *mPlanarReflectionsSamplerblock;
        /// Whether the current active pass can use mPlanarReflections (i.e. we can't
        /// use the reflections if they were built for a different camera angle)
        bool                    mHasPlanarReflections;
        uint8                   mLastBoundPlanarReflection;
#endif
        TextureGpu              *mAreaLightMasks;
        HlmsSamplerblock const  *mAreaLightMasksSamplerblock;
        LightArray                mAreaLights;
        bool                    mUsingAreaLightMasks;

        bool                    mSkipRequestSlotInChangeRS;

        /// LTC matrix texture also contains BRDF LUT for specular IBL.
        TextureGpu              *mLtcMatrixTexture;

        bool                    mDecalsDiffuseMergedEmissive;
        TextureGpu              *mDecalsTextures[3];
        HlmsSamplerblock const  *mDecalsSamplerblock;

        ConstBufferPool::BufferPool const *mLastBoundPool;

        bool mHasSeparateSamplers;
        DescriptorSetTexture const *mLastDescTexture;
        DescriptorSetSampler const *mLastDescSampler;
        uint8 mReservedTexSlots;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        bool mFineLightMaskGranularity;
#endif
        bool mSetupWorldMatBuf;
        bool mDebugPssmSplits;

        bool mAutoSpecIblMaxMipmap;
        bool mVctFullConeCount;

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
        bool mUseObbRestraintAreaApprox;
        bool mUseObbRestraintAreaLtc;
#endif

        bool mUseLightBuffers;

        ShadowFilter    mShadowFilter;
        uint16          mEsmK; /// K parameter for ESM.
        AmbientLightMode mAmbientLightMode;

        virtual const HlmsCache* createShaderCacheEntry( uint32 renderableHash,
                                                         const HlmsCache &passCache,
                                                         uint32 finalHash,
                                                         const QueuedRenderable &queuedRenderable );

        virtual HlmsDatablock* createDatablockImpl( IdString datablockName,
                                                    const HlmsMacroblock *macroblock,
                                                    const HlmsBlendblock *blendblock,
                                                    const HlmsParamVec &paramVec );

        void setDetailMapProperties( HlmsPbsDatablock *datablock, PiecesMap *inOutPieces );
        void setTextureProperty( const char *propertyName, HlmsPbsDatablock *datablock,
                                 PbsTextureTypes texType );
        void setDetailTextureProperty( const char *propertyName, HlmsPbsDatablock *datablock,
                                       PbsTextureTypes baseTexType, uint8 detailIdx );

        virtual void calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash );
        virtual void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces );
        virtual void calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces );

        virtual void notifyPropertiesMergedPreGenerationStep(void);

        static bool requiredPropertyByAlphaTest( IdString propertyName );

        virtual void destroyAllBuffers(void);

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache *cache,
                                           const QueuedRenderable &queuedRenderable,
                                           bool casterPass, uint32 lastCacheHash,
                                           CommandBuffer *commandBuffer, bool isV1 );

    public:
        HlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders );
        virtual ~HlmsPbs();

        virtual void _changeRenderSystem( RenderSystem *newRs );

        virtual HlmsCache preparePassHash( const Ogre::CompositorShadowNode *shadowNode,
                                           bool casterPass, bool dualParaboloid,
                                           SceneManager *sceneManager );

        virtual uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                       bool casterPass, uint32 lastCacheHash,
                                       uint32 lastTextureHash );

        virtual uint32 fillBuffersForV1( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );
        virtual uint32 fillBuffersForV2( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );

        virtual void postCommandBufferExecution( CommandBuffer *commandBuffer );
        virtual void frameEnded(void);

        /** By default we see the reflection textures' mipmaps and store the largest one we found.
            By calling resetIblSpecMipmap; you can reset this process thus if a reflection texture
            with a large number of mipmaps was removed, these textures can be reevaluated
        @param numMipmaps
            When 0; we automatically check for reflection texture.
            When non-zero, we force the number of mipmaps to the specified value
        */
        void resetIblSpecMipmap( uint8 numMipmaps );

        void _notifyIblSpecMipmap( uint8 numMipmaps );

        void loadLtcMatrix(void);

        /** Fill the provided string and string vector with all the sub-folder needed to instantiate
            an HlmsPbs object with the default distribution of the HlmsResources.
            These paths are dependent of the current RenderSystem.

            This method can only be called after a valid RenderSysttem has been chosen.

            All output parameter's content will be replaced with the new set of paths.
        @param outDataFolderPath
            Path (as a String) used for creating the "dataFolder" Archive the constructor will need
        @param outLibraryFoldersPaths
            Vector of String used for creating the ArchiveVector "libraryFolders" the constructor will need
        */
        static void getDefaultPaths( String& outDataFolderPath, StringVector& outLibraryFoldersPaths );

#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        /// Toggles whether light masks will be obeyed per object by doing:
        /// if( movableObject->getLightMask() & light->getLightMask() )
        ///     doLighting( movableObject light );
        /// Note this toggle only affects forward lights
        /// (i.e. Directional lights + shadow casting lights).
        /// You may want to see ForwardPlusBase::setFineLightMaskGranularity
        /// for control over Forward+ lights.
        void setFineLightMaskGranularity( bool useFineGranularity )
                                                    { mFineLightMaskGranularity = useFineGranularity; }
        bool getFineLightMaskGranularity(void) const{ return mFineLightMaskGranularity; }
#endif

        void setDebugPssmSplits( bool bDebug );
        bool getDebugPssmSplits(void) const                 { return mDebugPssmSplits; }

        void setShadowSettings( ShadowFilter filter );
        ShadowFilter getShadowFilter(void) const            { return mShadowFilter; }

        /** Sets the 'K' parameter of ESM filter. Defaults to 600.
            Small values will give weak shadows, and light bleeding (specially if the
            caster is close to the receiver; particularly noticeable at contact points).
            It also gives the chance of over darkening to appear (the shadow of a small
            caster in front of a large caster looks darker; thus the large caster appers
            like if it were made of glass instead of being solid).

            Large values give strong, dark shadows; but the higher the value, the more
            you push floating point limits.
            This value is related to K in MiscUtils::setGaussianLogFilterParams. You don't
            have to set them to the same value; but you'll notice that if you change this
            value here, you'll likely have to change the log filter's too.
        @param K
            In range (0; infinite).
        */
        void setEsmK( uint16 K );
        uint16 getEsmK(void) const                          { return mEsmK; }

        void setAmbientLightMode( AmbientLightMode mode );
        AmbientLightMode getAmbientLightMode(void) const    { return mAmbientLightMode; }

        /** Sets PCC
        @remarks
            PCC is a crude approximation of reflections that works best when the shape
            of the objects in the scene resemble the rectangle shape set by the PCC probe(s).

            But when they do not, severe distortion could happen.

            When both VCT and PCC are active, VCT can be used to determine whether the PCC
            approximation is accurate. If it is, it uses the PCC result. If it's not, it uses
            the VCT one (which is accurate but more 'blocky' due to voxelization)

            This setting determines how much deviation is allowed and smoothly transition
            between the VCT and PCC results.

            This value *only* affects specular reflections.
            For diffuse GI, VCT is always preferred over the cubemap's.
        @param pcc
            Pointer to PCC
        @param pccVctMinDistance
            Value in units (e.g. meters, centimeters, whatever your engine uses)
            Any discrepancy between PCC and VCT below this threshold means PCC
            reflections will be used.

            The assertion pccVctMinDistance < pccVctMaxDistance must be true.
            Use a very high pccVctMinDistance to always use PCC
        @param pccVctMaxDistance
            Value in units (e.g. meters, centimeters, whatever your engine uses)
            Any discrepancy between PCC and VCT above this threshold means VCT
            reflections will be used.

            Errors between pccVctMinDistance & pccVctMaxDistance will be faded smoothly
            Use negative pccVctMaxDistance to always use VCT
        */
        void setParallaxCorrectedCubemap( ParallaxCorrectedCubemapBase *pcc,
                                          float pccVctMinDistance = 1.0f,
                                          float pccVctMaxDistance = 2.0f );
        ParallaxCorrectedCubemapBase* getParallaxCorrectedCubemap(void) const
                                                            { return mParallaxCorrectedCubemap; }

        void setIrradianceVolume( IrradianceVolume *irradianceVolume )
                                                    { mIrradianceVolume = irradianceVolume; }
        IrradianceVolume* getIrradianceVolume(void) const  { return mIrradianceVolume; }

        void setVctLighting( VctLighting *vctLighting )     { mVctLighting = vctLighting; }
        VctLighting* getVctLighting(void)                   { return mVctLighting; }

        void setIrradianceField( IrradianceField *irradianceField )
                                                    { mIrradianceField = irradianceField; }
        IrradianceField *getIrradianceField( void ) { return mIrradianceField; }

        /** When false, we will use 4 cones for diffuse VCT.
            When true, we will use 6 cones instead. This is higher quality but consumes more
            performance and is usually overkill (benefit / cost ratio).

            Default value is false
        @param vctFullConeCount
        */
        void setVctFullConeCount( bool vctFullConeCount )   { mVctFullConeCount = vctFullConeCount; }
        bool getVctFullConeCount(void) const                { return mVctFullConeCount; }

        void setAreaLightMasks( TextureGpu *areaLightMask );
        TextureGpu* getAreaLightMasks(void) const           { return mAreaLightMasks; }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        void setPlanarReflections( PlanarReflections *planarReflections );
        PlanarReflections* getPlanarReflections(void) const;
#endif

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
        void setUseObbRestraints( bool areaApprox, bool areaLtc );
        bool getUseObbRestraintsAreaApprox(void) const      { return mUseObbRestraintAreaApprox; }
        bool getUseObbRestraintsAreaLtc(void) const         { return mUseObbRestraintAreaLtc; }
#endif

        void setUseLightBuffers(bool b);
        bool getUseLightBuffers() { return mUseLightBuffers; }

#if !OGRE_NO_JSON
        /// @copydoc Hlms::_loadJson
        virtual void _loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                                HlmsDatablock *datablock, const String &resourceGroup,
                                HlmsJsonListener *listener, const String &additionalTextureExtension ) const;
        /// @copydoc Hlms::_saveJson
        virtual void _saveJson( const HlmsDatablock *datablock, String &outString,
                                HlmsJsonListener *listener,
                                const String &additionalTextureExtension ) const;

        /// @copydoc Hlms::_collectSamplerblocks
        virtual void _collectSamplerblocks( set<const HlmsSamplerblock*>::type &outSamplerblocks,
                                            const HlmsDatablock *datablock ) const;
#endif
    };

    struct _OgreHlmsPbsExport PbsProperty
    {
        static const IdString useLightBuffers;
    
        static const IdString HwGammaRead;
        static const IdString HwGammaWrite;
        static const IdString MaterialsPerBuffer;
        static const IdString LowerGpuOverhead;
        static const IdString DebugPssmSplits;
        static const IdString HasPlanarReflections;

        static const IdString NumTextures;
        static const IdString NumSamplers;
        static const char *DiffuseMap;
        static const char *NormalMapTex;
        static const char *SpecularMap;
        static const char *RoughnessMap;
        static const char *EmissiveMap;
        static const char *EnvProbeMap;
        static const char *DetailWeightMap;
        static const char *DetailMapN;
        static const char *DetailMapNmN;

        static const IdString DetailMap0;
        static const IdString DetailMap1;
        static const IdString DetailMap2;
        static const IdString DetailMap3;

        static const IdString NormalMap;

        static const IdString FresnelScalar;
        static const IdString UseTextureAlpha;
        static const IdString TransparentMode;
        static const IdString FresnelWorkflow;
        static const IdString MetallicWorkflow;
        static const IdString TwoSidedLighting;
        static const IdString ReceiveShadows;
        static const IdString UsePlanarReflections;

        static const IdString NormalSamplingFormat;
        static const IdString NormalLa;
        static const IdString NormalRgUnorm;
        static const IdString NormalRgSnorm;

        static const IdString NormalWeight;
        static const IdString NormalWeightTex;
        static const IdString NormalWeightDetail0;
        static const IdString NormalWeightDetail1;
        static const IdString NormalWeightDetail2;
        static const IdString NormalWeightDetail3;

        static const IdString DetailWeights;
        static const IdString DetailOffsets0;
        static const IdString DetailOffsets1;
        static const IdString DetailOffsets2;
        static const IdString DetailOffsets3;

        static const IdString UvDiffuse;
        static const IdString UvNormal;
        static const IdString UvSpecular;
        static const IdString UvRoughness;
        static const IdString UvDetailWeight;

        static const IdString UvDetail0;
        static const IdString UvDetail1;
        static const IdString UvDetail2;
        static const IdString UvDetail3;

        static const IdString UvDetailNm0;
        static const IdString UvDetailNm1;
        static const IdString UvDetailNm2;
        static const IdString UvDetailNm3;

        static const IdString UvEmissive;
        static const IdString EmissiveConstant;
        static const IdString EmissiveAsLightmap;
        static const IdString DetailMapsDiffuse;
        static const IdString DetailMapsNormal;
        static const IdString FirstValidDetailMapNm;

        static const IdString BlendModeIndex0;
        static const IdString BlendModeIndex1;
        static const IdString BlendModeIndex2;
        static const IdString BlendModeIndex3;

        static const IdString Pcf3x3;
        static const IdString Pcf4x4;
        static const IdString PcfIterations;
        static const IdString ExponentialShadowMaps;

        static const IdString AmbientHemisphere;
        static const IdString AmbientSh;
        static const IdString AmbientShMonochrome;
        static const IdString LtcTextureAvailable;
        static const IdString EnvMapScale;
        static const IdString AmbientFixed;
        static const IdString TargetEnvprobeMap;
        static const IdString ParallaxCorrectCubemaps;
        static const IdString UseParallaxCorrectCubemaps;
        static const IdString EnableCubemapsAuto;
        static const IdString CubemapsUseDpm;
        static const IdString CubemapsAsDiffuseGi;
        static const IdString IrradianceVolumes;
        static const IdString VctNumProbes;
        static const IdString VctConeDirs;
        static const IdString VctDisableDiffuse;
        static const IdString VctDisableSpecular;
        static const IdString VctAnisotropic;
        static const IdString VctEnableSpecularSdfQuality;
        static const IdString VctAmbientSphere;
        static const IdString IrradianceField;
        static const IdString ObbRestraintApprox;
        static const IdString ObbRestraintLtc;

        static const IdString BrdfDefault;
        static const IdString BrdfCookTorrance;
        static const IdString BrdfBlinnPhong;
        static const IdString FresnelSeparateDiffuse;
        static const IdString GgxHeightCorrelated;
        static const IdString LegacyMathBrdf;
        static const IdString RoughnessIsShininess;

        static const IdString UseEnvProbeMap;
        static const IdString NeedsViewDir;
        static const IdString NeedsReflDir;
        static const IdString NeedsEnvBrdf;

        static const IdString *UvSourcePtrs[NUM_PBSM_SOURCES];
        static const IdString *BlendModes[4];
        static const IdString *DetailNormalWeights[4];
        static const IdString *DetailOffsetsPtrs[4];
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
