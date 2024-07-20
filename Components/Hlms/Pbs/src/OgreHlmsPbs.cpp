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

#include "OgreStableHeaders.h"

#include "OgreHlmsPbs.h"

#include "OgreHlmsListener.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreLwString.h"

#if !OGRE_NO_JSON
#    include "OgreHlmsJsonPbs.h"
#endif

#include "Animation/OgreSkeletonInstance.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "IrradianceField/OgreIrradianceField.h"
#include "OgreAtmosphereComponent.h"
#include "OgreCamera.h"
#include "OgreForward3D.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreIrradianceVolume.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderQueue.h"
#include "OgreRootLayout.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"
#include "OgreViewport.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vct/OgreVctLighting.h"

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
#    include "OgrePlanarReflections.h"
#endif

#include "OgreLogManager.h"
#include "OgreProfiler.h"
#include "OgreStackVector.h"

#define TODO_irradianceField_samplerblock

namespace Ogre
{
    static const TextureGpuVec c_emptyTextureContainer;

    const IdString PbsProperty::useLightBuffers = IdString( "use_light_buffers" );

    const IdString PbsProperty::HwGammaRead = IdString( "hw_gamma_read" );
    const IdString PbsProperty::HwGammaWrite = IdString( "hw_gamma_write" );
    const IdString PbsProperty::MaterialsPerBuffer = IdString( "materials_per_buffer" );
    const IdString PbsProperty::LowerGpuOverhead = IdString( "lower_gpu_overhead" );
    const IdString PbsProperty::DebugPssmSplits = IdString( "debug_pssm_splits" );
    const IdString PbsProperty::IndustryCompatible = IdString( "industry_compatible" );
    const IdString PbsProperty::PerceptualRoughness = IdString( "perceptual_roughness" );
    const IdString PbsProperty::HasPlanarReflections = IdString( "has_planar_reflections" );

    const IdString PbsProperty::NumPassConstBuffers = IdString( "num_pass_const_buffers" );

    const IdString PbsProperty::Set0TextureSlotEnd = IdString( "set0_texture_slot_end" );
    const IdString PbsProperty::Set1TextureSlotEnd = IdString( "set1_texture_slot_end" );
    const IdString PbsProperty::NumTextures = IdString( "num_textures" );
    const IdString PbsProperty::NumSamplers = IdString( "num_samplers" );
    const IdString PbsProperty::DiffuseMapGrayscale = IdString( "diffuse_map_grayscale" );
    const IdString PbsProperty::EmissiveMapGrayscale = IdString( "emissive_map_grayscale" );
    const char *PbsProperty::DiffuseMap = "diffuse_map";
    const char *PbsProperty::NormalMapTex = "normal_map_tex";
    const char *PbsProperty::SpecularMap = "specular_map";
    const char *PbsProperty::RoughnessMap = "roughness_map";
    const char *PbsProperty::EmissiveMap = "emissive_map";
    const char *PbsProperty::EnvProbeMap = "envprobe_map";
    const char *PbsProperty::DetailWeightMap = "detail_weight_map";
    const char *PbsProperty::DetailMapN = "detail_map";       // detail_map0-4
    const char *PbsProperty::DetailMapNmN = "detail_map_nm";  // detail_map_nm0-4

    const IdString PbsProperty::DetailMap0 = "detail_map0";
    const IdString PbsProperty::DetailMap1 = "detail_map1";
    const IdString PbsProperty::DetailMap2 = "detail_map2";
    const IdString PbsProperty::DetailMap3 = "detail_map3";

    const IdString PbsProperty::NormalMap = IdString( "normal_map" );

    const IdString PbsProperty::FresnelScalar = IdString( "fresnel_scalar" );
    const IdString PbsProperty::UseTextureAlpha = IdString( "use_texture_alpha" );
    const IdString PbsProperty::TransparentMode = IdString( "transparent_mode" );
    const IdString PbsProperty::FresnelWorkflow = IdString( "fresnel_workflow" );
    const IdString PbsProperty::MetallicWorkflow = IdString( "metallic_workflow" );
    const IdString PbsProperty::TwoSidedLighting = IdString( "two_sided_lighting" );
    const IdString PbsProperty::ReceiveShadows = IdString( "receive_shadows" );
    const IdString PbsProperty::UsePlanarReflections = IdString( "use_planar_reflections" );

    const IdString PbsProperty::NormalSamplingFormat = IdString( "normal_sampling_format" );
    const IdString PbsProperty::NormalLa = IdString( "normal_la" );
    const IdString PbsProperty::NormalRgUnorm = IdString( "normal_rg_unorm" );
    const IdString PbsProperty::NormalRgSnorm = IdString( "normal_rg_snorm" );
    const IdString PbsProperty::NormalBc3Unorm = IdString( "normal_bc3_unorm" );

    const IdString PbsProperty::NormalWeight = IdString( "normal_weight" );
    const IdString PbsProperty::NormalWeightTex = IdString( "normal_weight_tex" );
    const IdString PbsProperty::NormalWeightDetail0 = IdString( "normal_weight_detail0" );
    const IdString PbsProperty::NormalWeightDetail1 = IdString( "normal_weight_detail1" );
    const IdString PbsProperty::NormalWeightDetail2 = IdString( "normal_weight_detail2" );
    const IdString PbsProperty::NormalWeightDetail3 = IdString( "normal_weight_detail3" );

    const IdString PbsProperty::DetailWeights = IdString( "detail_weights" );
    const IdString PbsProperty::DetailOffsets0 = IdString( "detail_offsets0" );
    const IdString PbsProperty::DetailOffsets1 = IdString( "detail_offsets1" );
    const IdString PbsProperty::DetailOffsets2 = IdString( "detail_offsets2" );
    const IdString PbsProperty::DetailOffsets3 = IdString( "detail_offsets3" );

    const IdString PbsProperty::UvDiffuse = IdString( "uv_diffuse" );
    const IdString PbsProperty::UvNormal = IdString( "uv_normal" );
    const IdString PbsProperty::UvSpecular = IdString( "uv_specular" );
    const IdString PbsProperty::UvRoughness = IdString( "uv_roughness" );
    const IdString PbsProperty::UvDetailWeight = IdString( "uv_detail_weight" );
    const IdString PbsProperty::UvDetail0 = IdString( "uv_detail0" );
    const IdString PbsProperty::UvDetail1 = IdString( "uv_detail1" );
    const IdString PbsProperty::UvDetail2 = IdString( "uv_detail2" );
    const IdString PbsProperty::UvDetail3 = IdString( "uv_detail3" );
    const IdString PbsProperty::UvDetailNm0 = IdString( "uv_detail_nm0" );
    const IdString PbsProperty::UvDetailNm1 = IdString( "uv_detail_nm1" );
    const IdString PbsProperty::UvDetailNm2 = IdString( "uv_detail_nm2" );
    const IdString PbsProperty::UvDetailNm3 = IdString( "uv_detail_nm3" );
    const IdString PbsProperty::UvEmissive = IdString( "uv_emissive" );

    const IdString PbsProperty::BlendModeIndex0 = IdString( "blend_mode_idx0" );
    const IdString PbsProperty::BlendModeIndex1 = IdString( "blend_mode_idx1" );
    const IdString PbsProperty::BlendModeIndex2 = IdString( "blend_mode_idx2" );
    const IdString PbsProperty::BlendModeIndex3 = IdString( "blend_mode_idx3" );

    const IdString PbsProperty::DetailMapsDiffuse = IdString( "detail_maps_diffuse" );
    const IdString PbsProperty::DetailMapsNormal = IdString( "detail_maps_normal" );
    const IdString PbsProperty::FirstValidDetailMapNm = IdString( "first_valid_detail_map_nm" );
    const IdString PbsProperty::EmissiveConstant = IdString( "emissive_constant" );
    const IdString PbsProperty::EmissiveAsLightmap = IdString( "emissive_as_lightmap" );

    const IdString PbsProperty::Pcf = IdString( "pcf" );
    const IdString PbsProperty::PcfIterations = IdString( "pcf_iterations" );
    const IdString PbsProperty::ShadowsReceiveOnPs = IdString( "shadows_receive_on_ps" );
    const IdString PbsProperty::ExponentialShadowMaps = IdString( "exponential_shadow_maps" );

    const IdString PbsProperty::EnvMapScale = IdString( "envmap_scale" );
    const IdString PbsProperty::LightProfilesTexture = IdString( "light_profiles_texture" );
    const IdString PbsProperty::LtcTextureAvailable = IdString( "ltc_texture_available" );
    const IdString PbsProperty::AmbientFixed = IdString( "ambient_fixed" );
    const IdString PbsProperty::AmbientHemisphere = IdString( "ambient_hemisphere" );
    const IdString PbsProperty::AmbientHemisphereInverted = IdString( "ambient_hemisphere_inverted" );
    const IdString PbsProperty::AmbientSh = IdString( "ambient_sh" );
    const IdString PbsProperty::AmbientShMonochrome = IdString( "ambient_sh_monochrome" );
    const IdString PbsProperty::TargetEnvprobeMap = IdString( "target_envprobe_map" );
    const IdString PbsProperty::ParallaxCorrectCubemaps = IdString( "parallax_correct_cubemaps" );
    const IdString PbsProperty::UseParallaxCorrectCubemaps = IdString( "use_parallax_correct_cubemaps" );
    const IdString PbsProperty::EnableCubemapsAuto = IdString( "hlms_enable_cubemaps_auto" );
    const IdString PbsProperty::CubemapsUseDpm = IdString( "hlms_cubemaps_use_dpm" );
    const IdString PbsProperty::CubemapsAsDiffuseGi = IdString( "cubemaps_as_diffuse_gi" );
    const IdString PbsProperty::IrradianceVolumes = IdString( "irradiance_volumes" );
    const IdString PbsProperty::VctNumProbes = IdString( "vct_num_probes" );
    const IdString PbsProperty::VctConeDirs = IdString( "vct_cone_dirs" );
    const IdString PbsProperty::VctDisableDiffuse = IdString( "vct_disable_diffuse" );
    const IdString PbsProperty::VctDisableSpecular = IdString( "vct_disable_specular" );
    const IdString PbsProperty::VctAnisotropic = IdString( "vct_anisotropic" );
    const IdString PbsProperty::VctEnableSpecularSdfQuality =
        IdString( "vct_enable_specular_sdf_quality" );
    const IdString PbsProperty::VctAmbientSphere = IdString( "vct_ambient_hemisphere" );
    const IdString PbsProperty::IrradianceField = IdString( "irradiance_field" );
    const IdString PbsProperty::ObbRestraintApprox = IdString( "obb_restraint_approx" );

    const IdString PbsProperty::ObbRestraintLtc = IdString( "obb_restraint_ltc" );

    const IdString PbsProperty::BrdfDefault = IdString( "BRDF_Default" );
    const IdString PbsProperty::BrdfCookTorrance = IdString( "BRDF_CookTorrance" );
    const IdString PbsProperty::BrdfBlinnPhong = IdString( "BRDF_BlinnPhong" );
    const IdString PbsProperty::FresnelHasDiffuse = IdString( "fresnel_has_diffuse" );
    const IdString PbsProperty::FresnelSeparateDiffuse = IdString( "fresnel_separate_diffuse" );
    const IdString PbsProperty::GgxHeightCorrelated = IdString( "GGX_height_correlated" );
    const IdString PbsProperty::ClearCoat = IdString( "clear_coat" );
    const IdString PbsProperty::LegacyMathBrdf = IdString( "legacy_math_brdf" );
    const IdString PbsProperty::RoughnessIsShininess = IdString( "roughness_is_shininess" );

    const IdString PbsProperty::UseEnvProbeMap = IdString( "use_envprobe_map" );
    const IdString PbsProperty::NeedsViewDir = IdString( "needs_view_dir" );
    const IdString PbsProperty::NeedsReflDir = IdString( "needs_refl_dir" );
    const IdString PbsProperty::NeedsEnvBrdf = IdString( "needs_env_brdf" );

    // clang-format off
    const IdString *PbsProperty::UvSourcePtrs[NUM_PBSM_SOURCES] =
    {
        &PbsProperty::UvDiffuse,
        &PbsProperty::UvNormal,
        &PbsProperty::UvSpecular,
        &PbsProperty::UvRoughness,
        &PbsProperty::UvDetailWeight,
        &PbsProperty::UvDetail0,
        &PbsProperty::UvDetail1,
        &PbsProperty::UvDetail2,
        &PbsProperty::UvDetail3,
        &PbsProperty::UvDetailNm0,
        &PbsProperty::UvDetailNm1,
        &PbsProperty::UvDetailNm2,
        &PbsProperty::UvDetailNm3,
        &PbsProperty::UvEmissive,
    };

    const IdString *PbsProperty::DetailNormalWeights[4] =
    {
        &PbsProperty::NormalWeightDetail0,
        &PbsProperty::NormalWeightDetail1,
        &PbsProperty::NormalWeightDetail2,
        &PbsProperty::NormalWeightDetail3
    };

    const IdString *PbsProperty::DetailOffsetsPtrs[4] =
    {
        &PbsProperty::DetailOffsets0,
        &PbsProperty::DetailOffsets1,
        &PbsProperty::DetailOffsets2,
        &PbsProperty::DetailOffsets3
    };

    const IdString *PbsProperty::BlendModes[4] =
    {
        &PbsProperty::BlendModeIndex0,
        &PbsProperty::BlendModeIndex1,
        &PbsProperty::BlendModeIndex2,
        &PbsProperty::BlendModeIndex3
    };
    // clang-format on

    extern const String c_pbsBlendModes[];

    HlmsPbs::HlmsPbs( Archive *dataFolder, ArchiveVec *libraryFolders ) :
        HlmsBufferManager( HLMS_PBS, "pbs", dataFolder, libraryFolders ),
        ConstBufferPool( HlmsPbsDatablock::MaterialSizeInGpuAligned,
                         ConstBufferPool::ExtraBufferParams() ),
        mShadowmapSamplerblock( 0 ),
        mShadowmapCmpSamplerblock( 0 ),
        mShadowmapEsmSamplerblock( 0 ),
        mCurrentShadowmapSamplerblock( 0 ),
        mParallaxCorrectedCubemap( 0 ),
        mPccVctMinDistance( 1.0f ),
        mInvPccVctInvDistance( 1.0f ),
        mCurrentPassBuffer( 0 ),
        mGridBuffer( 0 ),
        mGlobalLightListBuffer( 0 ),
        mMaxSpecIblMipmap( 1.0f ),
        mTexBufUnitSlotEnd( 0u ),
        mTexUnitSlotStart( 0 ),
        mPrePassTextures( 0 ),
        mDepthTexture( 0 ),
        mSsrTexture( 0 ),
        mDepthTextureNoMsaa( 0 ),
        mRefractionsTexture( 0 ),
        mIrradianceVolume( 0 ),
        mVctLighting( 0 ),
        mIrradianceField( 0 ),
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        mPlanarReflections( 0 ),
        mPlanarReflectionsSamplerblock( 0 ),
        mHasPlanarReflections( false ),
        mLastBoundPlanarReflection( 0u ),
        mPlanarReflectionSlotIdx( 0u ),
#endif
        mAreaLightMasks( 0 ),
        mAreaLightMasksSamplerblock( 0 ),
        mUsingAreaLightMasks( false ),
        mNumPassConstBuffers( 0u ),
        mAtmosphere( 0 ),
        mLightProfilesTexture( 0 ),
        mSkipRequestSlotInChangeRS( false ),
        mLtcMatrixTexture( 0 ),
        mDecalsDiffuseMergedEmissive( false ),
        mDecalsSamplerblock( 0 ),
        mLastBoundPool( 0 ),
        mConstantBiasScale( 0.1f ),
        mHasSeparateSamplers( 0 ),
        mLastDescTexture( 0 ),
        mLastDescSampler( 0 ),
        mReservedTexBufferSlots( 1u ),  // Vertex shader consumes 1 slot with its tbuffer.
        mReservedTexSlots( 0u ),
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        mFineLightMaskGranularity( true ),
#endif
        mSetupWorldMatBuf( true ),
        mDebugPssmSplits( false ),
        mShadowReceiversInPixelShader( false ),
        mPerceptualRoughness( true ),
        mAutoSpecIblMaxMipmap( true ),
        mVctFullConeCount( false ),
#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
        mUseObbRestraintAreaApprox( false ),
        mUseObbRestraintAreaLtc( false ),
#endif
        mUseLightBuffers( false ),
        mIndustryCompatible( false ),
        mDefaultBrdfWithDiffuseFresnel( false ),
        mShadowFilter( PCF_3x3 ),
        mEsmK( 600u ),
        mAmbientLightMode( AmbientAutoNormal )
    {
        memset( mDecalsTextures, 0, sizeof( mDecalsTextures ) );

        // Override defaults
        mLightGatheringMode = LightGatherForwardPlus;

        // It is always 0.
        // (0 is world matrix, we don't need it for particles. 1 is animation matrix)
        mParticleSystemSlot = 0u;
    }
    //-----------------------------------------------------------------------------------
    HlmsPbs::~HlmsPbs() { destroyAllBuffers(); }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::_changeRenderSystem( RenderSystem *newRs )
    {
        ConstBufferPool::_changeRenderSystem( newRs );
        HlmsBufferManager::_changeRenderSystem( newRs );

        if( newRs )
        {
            if( !mSkipRequestSlotInChangeRS )
            {
                HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
                HlmsDatablockMap::const_iterator end = mDatablocks.end();

                while( itor != end )
                {
                    assert( dynamic_cast<HlmsPbsDatablock *>( itor->second.datablock ) );
                    HlmsPbsDatablock *datablock =
                        static_cast<HlmsPbsDatablock *>( itor->second.datablock );

                    requestSlot( datablock->mTextureHash, datablock, false );
                    ++itor;
                }
            }

            const ColourValue maxValBorder =
                ColourValue( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );
            const ColourValue pitchBlackBorder = ColourValue( 0, 0, 0, 0 );

            HlmsSamplerblock samplerblock;
            samplerblock.mU = TAM_BORDER;
            samplerblock.mV = TAM_BORDER;
            samplerblock.mW = TAM_CLAMP;
            if( !mRenderSystem->isReverseDepth() )
                samplerblock.mBorderColour = maxValBorder;
            else
                samplerblock.mBorderColour = pitchBlackBorder;

            if( mShaderProfile != "hlsl" && mShaderProfile != "hlslvk" )
            {
                samplerblock.mMinFilter = FO_POINT;
                samplerblock.mMagFilter = FO_POINT;
                samplerblock.mMipFilter = FO_NONE;

                if( !mShadowmapSamplerblock )
                    mShadowmapSamplerblock = mHlmsManager->getSamplerblock( samplerblock );
            }

            if( !mShadowmapEsmSamplerblock )
            {
                samplerblock.mMinFilter = FO_LINEAR;
                samplerblock.mMagFilter = FO_LINEAR;
                samplerblock.mMipFilter = FO_NONE;

                // ESM uses standard linear Z in range [0; 1], thus we need a different border colour
                const ColourValue oldValue = samplerblock.mBorderColour;
                samplerblock.mBorderColour = maxValBorder;

                mShadowmapEsmSamplerblock = mHlmsManager->getSamplerblock( samplerblock );

                // Restore border colour
                samplerblock.mBorderColour = oldValue;
            }

            samplerblock.mMinFilter = FO_LINEAR;
            samplerblock.mMagFilter = FO_LINEAR;
            samplerblock.mMipFilter = FO_NONE;
            samplerblock.mCompareFunction = CMPF_LESS_EQUAL;
            if( mRenderSystem->isReverseDepth() )
            {
                samplerblock.mCompareFunction =
                    RenderSystem::reverseCompareFunction( samplerblock.mCompareFunction );
            }

            if( !mShadowmapCmpSamplerblock )
                mShadowmapCmpSamplerblock = mHlmsManager->getSamplerblock( samplerblock );

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( !mPlanarReflectionsSamplerblock )
            {
                samplerblock.mMinFilter = FO_LINEAR;
                samplerblock.mMagFilter = FO_LINEAR;
                samplerblock.mMipFilter = FO_LINEAR;
                samplerblock.mCompareFunction = NUM_COMPARE_FUNCTIONS;

                samplerblock.mU = TAM_CLAMP;
                samplerblock.mV = TAM_CLAMP;
                samplerblock.mW = TAM_CLAMP;

                mPlanarReflectionsSamplerblock = mHlmsManager->getSamplerblock( samplerblock );
            }
#endif
            if( !mAreaLightMasksSamplerblock )
            {
                samplerblock.mMinFilter = FO_LINEAR;
                samplerblock.mMagFilter = FO_LINEAR;
                samplerblock.mMipFilter = FO_LINEAR;
                samplerblock.mCompareFunction = NUM_COMPARE_FUNCTIONS;

                samplerblock.mU = TAM_CLAMP;
                samplerblock.mV = TAM_CLAMP;
                samplerblock.mW = TAM_CLAMP;

                mAreaLightMasksSamplerblock = mHlmsManager->getSamplerblock( samplerblock );
            }

            if( !mDecalsSamplerblock )
            {
                samplerblock.mMinFilter = FO_LINEAR;
                samplerblock.mMagFilter = FO_LINEAR;
                samplerblock.mMipFilter = FO_LINEAR;
                samplerblock.mCompareFunction = NUM_COMPARE_FUNCTIONS;

                samplerblock.mU = TAM_CLAMP;
                samplerblock.mV = TAM_CLAMP;
                samplerblock.mW = TAM_CLAMP;

                mDecalsSamplerblock = mHlmsManager->getSamplerblock( samplerblock );
            }

            const RenderSystemCapabilities *caps = newRs->getCapabilities();
            mHasSeparateSamplers = caps->hasCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );
        }
    }

    //-----------------------------------------------------------------------------------
    void HlmsPbs::setupRootLayout( RootLayout &rootLayout, const size_t tid )
    {
        DescBindingRange *descBindingRanges = rootLayout.mDescBindingRanges[0];

        descBindingRanges[DescBindingTypes::ConstBuffer].end =
            static_cast<uint16>( getProperty( tid, PbsProperty::NumPassConstBuffers ) );

        // PccManualProbeDecl piece uses more conditionals to whether use an extra const buffer
        // However we don't need to test them all. It's ok to have a few false positives
        // (it just wastes a little bit more memory)
        if( getProperty( tid, PbsProperty::UseParallaxCorrectCubemaps ) &&
            !getProperty( tid, PbsProperty::EnableCubemapsAuto ) )
        {
            ++descBindingRanges[DescBindingTypes::ConstBuffer].end;
        }

        if( mSetupWorldMatBuf )
        {
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].start = 0u;
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 1u;
        }
        else
        {
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].start = 0u;
            descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 0u;
        }

        // if( getProperty( HlmsBaseProp::Pose ) )
        // descBindingRanges[DescBindingTypes::TexBuffer].end = 4u;

        // else
        {
            if( getProperty( tid, HlmsBaseProp::ForwardPlus ) )
            {
                descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end =
                    (uint16)getProperty( tid, "f3dLightList" ) + 1u;

                descBindingRanges[DescBindingTypes::TexBuffer].start =
                    (uint16)getProperty( tid, "f3dGrid" );
                descBindingRanges[DescBindingTypes::TexBuffer].end =
                    descBindingRanges[DescBindingTypes::TexBuffer].start + 1u;
            }
        }

        // It's not a typo: we start Texture where max( ReadOnlyBuffer, TexBuffer ) left off
        // because we treat ReadOnly buffers numbering as if they all were texbuffer slots
        // (in terms of contiguity)
        if( descBindingRanges[DescBindingTypes::ReadOnlyBuffer].isInUse() )
        {
            descBindingRanges[DescBindingTypes::Texture].start =
                descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end;
        }
        if( descBindingRanges[DescBindingTypes::TexBuffer].isInUse() )
        {
            descBindingRanges[DescBindingTypes::Texture].start =
                std::max( descBindingRanges[DescBindingTypes::Texture].end,
                          descBindingRanges[DescBindingTypes::TexBuffer].end );
        }
        descBindingRanges[DescBindingTypes::Texture].end =
            (uint16)getProperty( tid, PbsProperty::Set0TextureSlotEnd );

        descBindingRanges[DescBindingTypes::Sampler].start =
            descBindingRanges[DescBindingTypes::Texture].start;
        descBindingRanges[DescBindingTypes::Sampler].end =
            (uint16)( getProperty( tid, "samplerStateStart" ) );

        rootLayout.mBaked[1] = true;
        DescBindingRange *bakedRanges = rootLayout.mDescBindingRanges[1];

        bakedRanges[DescBindingTypes::Texture].start = descBindingRanges[DescBindingTypes::Texture].end;
        bakedRanges[DescBindingTypes::Texture].end =
            (uint16)getProperty( tid, PbsProperty::Set1TextureSlotEnd );

        bakedRanges[DescBindingTypes::Sampler].start = descBindingRanges[DescBindingTypes::Sampler].end;
        bakedRanges[DescBindingTypes::Sampler].end =
            bakedRanges[DescBindingTypes::Sampler].start +
            (uint16)getProperty( tid, PbsProperty::NumSamplers );

        int32 poseBufReg = getProperty( tid, "poseBuf", -1 );
        if( poseBufReg >= 0 )
        {
            DescBindingRange *poseRanges = rootLayout.mDescBindingRanges[2];
            if( !bakedRanges[DescBindingTypes::Texture].isInUse() &&
                !bakedRanges[DescBindingTypes::Sampler].isInUse() )
            {
                poseRanges = rootLayout.mDescBindingRanges[1];
                rootLayout.mBaked[1] = false;
            }
            poseRanges[DescBindingTypes::TexBuffer].start = static_cast<uint16>( poseBufReg );
            poseRanges[DescBindingTypes::TexBuffer].end = static_cast<uint16>( poseBufReg + 1 );
        }

        const int32 numVctProbes = getProperty( tid, PbsProperty::VctNumProbes );

        if( numVctProbes > 1 )
        {
            int32 vctProbeIdx = getProperty( tid, "vctProbes" );

            rootLayout.addArrayBinding( DescBindingTypes::Texture,
                                        RootLayout::ArrayDesc( static_cast<uint16>( vctProbeIdx ),
                                                               static_cast<uint16>( numVctProbes ) ) );
            if( getProperty( tid, PbsProperty::VctAnisotropic ) )
            {
                for( int32 i = 0; i < 3; ++i )
                {
                    vctProbeIdx += numVctProbes;
                    rootLayout.addArrayBinding(
                        DescBindingTypes::Texture,
                        RootLayout::ArrayDesc( static_cast<uint16>( vctProbeIdx ),
                                               static_cast<uint16>( numVctProbes ) ) );
                }
            }
        }

        mListener->setupRootLayout( rootLayout, mT[tid].setProperties, tid );
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *HlmsPbs::createShaderCacheEntry( uint32 renderableHash, const HlmsCache &passCache,
                                                      uint32 finalHash,
                                                      const QueuedRenderable &queuedRenderable,
                                                      HlmsCache *reservedStubEntry, const size_t tid )
    {
        OgreProfileExhaustive( "HlmsPbs::createShaderCacheEntry" );

        const HlmsCache *retVal = Hlms::createShaderCacheEntry(
            renderableHash, passCache, finalHash, queuedRenderable, reservedStubEntry, tid );

        if( mShaderProfile != "glsl" )
        {
            mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache, mT[tid].setProperties,
                                                queuedRenderable, tid );
            return retVal;  // D3D embeds the texture slots in the shader.
        }

        GpuProgramParametersSharedPtr vsParams = retVal->pso.vertexShader->getDefaultParameters();
        if( mSetupWorldMatBuf && mVaoManager->readOnlyIsTexBuffer() )
            vsParams->setNamedConstant( "worldMatBuf", 0 );

        mListener->shaderCacheEntryCreated( mShaderProfile, retVal, passCache, mT[tid].setProperties,
                                            queuedRenderable, tid );

        mRenderSystem->_setPipelineStateObject( &retVal->pso );

        mRenderSystem->bindGpuProgramParameters( GPT_VERTEX_PROGRAM, vsParams, GPV_ALL );
        if( retVal->pso.pixelShader )
        {
            GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();
            mRenderSystem->bindGpuProgramParameters( GPT_FRAGMENT_PROGRAM, psParams, GPV_ALL );
        }

        if( !mRenderSystem->getCapabilities()->hasCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER ) )
        {
            // Setting it to the vertex shader will set it to the PSO actually.
            retVal->pso.vertexShader->setUniformBlockBinding( "PassBuffer", 0 );
            retVal->pso.vertexShader->setUniformBlockBinding( "MaterialBuf", 1 );
            retVal->pso.vertexShader->setUniformBlockBinding( "InstanceBuffer", 2 );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setDetailMapProperties( const size_t tid, HlmsPbsDatablock *datablock,
                                          PiecesMap *inOutPieces, const bool bCasterPass )
    {
        int32 minNormalMap = 4;
        bool hasDiffuseMaps = false;
        bool hasNormalMaps = false;
        bool anyDetailWeight = false;
        for( int32 i = 0; i < 4; ++i )
        {
            uint8 blendMode = datablock->mBlendModes[i];

            const uint8 idx = static_cast<uint8>( i );

            setDetailTextureProperty( tid, PbsProperty::DetailMapN, datablock, PBSM_DETAIL0, idx );
            if( !bCasterPass )
            {
                setDetailTextureProperty( tid, PbsProperty::DetailMapNmN, datablock, PBSM_DETAIL0_NM,
                                          idx );
            }

            if( datablock->getTexture( PBSM_DETAIL0 + idx ) )
            {
                inOutPieces[PixelShader][*PbsProperty::BlendModes[i]] =
                    "@insertpiece( " + c_pbsBlendModes[blendMode] + ")";
                hasDiffuseMaps = true;
            }

            if( !bCasterPass && datablock->getTexture( PBSM_DETAIL0_NM + idx ) )
            {
                minNormalMap = std::min<int32>( minNormalMap, i );
                hasNormalMaps = true;
            }

            if( datablock->getDetailMapOffsetScale( idx ) != Vector4( 0, 0, 1, 1 ) )
                setProperty( tid, *PbsProperty::DetailOffsetsPtrs[i], 1 );

            if( datablock->mDetailWeight[i] != 1.0f &&
                ( datablock->getTexture( static_cast<uint8>( PBSM_DETAIL0 + i ) ) ||
                  ( !bCasterPass &&
                    datablock->getTexture( static_cast<uint8>( PBSM_DETAIL0_NM + i ) ) ) ) )
            {
                anyDetailWeight = true;
            }
        }

        if( hasDiffuseMaps )
            setProperty( tid, PbsProperty::DetailMapsDiffuse, 4 );

        if( hasNormalMaps )
            setProperty( tid, PbsProperty::DetailMapsNormal, 4 );

        setProperty( tid, PbsProperty::FirstValidDetailMapNm, minNormalMap );

        if( anyDetailWeight )
            setProperty( tid, PbsProperty::DetailWeights, 1 );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setTextureProperty( const size_t tid, const char *propertyName,
                                      HlmsPbsDatablock *datablock, PbsTextureTypes texType )
    {
        uint8 idx = datablock->getIndexToDescriptorTexture( texType );
        if( idx != NUM_PBSM_TEXTURE_TYPES )
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );

            propName = propertyName;  // diffuse_map

            const size_t basePropSize = propName.size();  // diffuse_map

            // In the template the we subtract the "+1" for the index.
            // We need to increment it now otherwise @property( diffuse_map )
            // can translate to @property( 0 ) which is not what we want.
            setProperty( tid, propertyName, idx + 1 );

            propName.resize( basePropSize );
            propName.a( "_idx" );  // diffuse_map_idx
            setProperty( tid, propName.c_str(), idx );

            if( mHasSeparateSamplers )
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler( texType );
                propName.resize( basePropSize );
                propName.a( "_sampler" );  // diffuse_map_sampler
                setProperty( tid, propName.c_str(), samplerIdx );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setDetailTextureProperty( const size_t tid, const char *propertyName,
                                            HlmsPbsDatablock *datablock, PbsTextureTypes baseTexType,
                                            uint8 detailIdx )
    {
        const PbsTextureTypes texType = static_cast<PbsTextureTypes>( baseTexType + detailIdx );
        char tmpData[32];
        LwString propName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );
        propName.a( propertyName, detailIdx );  // detail_map0
        setTextureProperty( tid, propName.c_str(), datablock, texType );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash )
    {
        assert( dynamic_cast<HlmsPbsDatablock *>( renderable->getDatablock() ) );
        HlmsPbsDatablock *datablock = static_cast<HlmsPbsDatablock *>( renderable->getDatablock() );

        if( datablock->getDirtyFlags() & ( DirtyTextures | DirtySamplers ) )
        {
            // Delay hash generation for later, when we have the final (or temporary) descriptor sets.
            outHash = 0;
            outCasterHash = 0;
        }
        else
        {
            Hlms::calculateHashFor( renderable, outHash, outCasterHash );
        }

        datablock->loadAllTextures();
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces )
    {
        assert( dynamic_cast<HlmsPbsDatablock *>( renderable->getDatablock() ) );
        HlmsPbsDatablock *datablock = static_cast<HlmsPbsDatablock *>( renderable->getDatablock() );

        const bool metallicWorkflow = datablock->getWorkflow() == HlmsPbsDatablock::MetallicWorkflow;
        const bool fresnelWorkflow =
            datablock->getWorkflow() == HlmsPbsDatablock::SpecularAsFresnelWorkflow;

        setProperty( kNoTid, PbsProperty::FresnelScalar,
                     datablock->hasSeparateFresnel() || metallicWorkflow );
        setProperty( kNoTid, PbsProperty::FresnelWorkflow, fresnelWorkflow );
        setProperty( kNoTid, PbsProperty::MetallicWorkflow, metallicWorkflow );

        if( datablock->getTwoSidedLighting() )
            setProperty( kNoTid, PbsProperty::TwoSidedLighting, 1 );

        if( datablock->getReceiveShadows() )
            setProperty( kNoTid, PbsProperty::ReceiveShadows, 1 );

        if( datablock->mTransparencyMode == HlmsPbsDatablock::Refractive )
        {
            setProperty( kNoTid, HlmsBaseProp::ScreenSpaceRefractions, 1 );
            setProperty( kNoTid, HlmsBaseProp::VPos, 1 );
            setProperty( kNoTid, HlmsBaseProp::ScreenPosInt, 1 );
            setProperty( kNoTid, HlmsBaseProp::ScreenPosUv, 1 );
        }

        if( !getProperty( kNoTid, HlmsBaseProp::QTangent ) &&
            !getProperty( kNoTid, HlmsBaseProp::Normal ) )
        {
            // No normals means we can't use normal offset bias.
            // Without normals, PBS is probably a very poor fit for this object
            // but at least don't crash
            setProperty( kNoTid, "skip_normal_offset_bias_vs", 1 );
        }

        uint32 brdf = datablock->getBrdf();
        if( ( brdf & PbsBrdf::BRDF_MASK ) == PbsBrdf::Default )
        {
            setProperty( kNoTid, PbsProperty::BrdfDefault, 1 );

            if( !( brdf & PbsBrdf::FLAG_UNCORRELATED ) )
                setProperty( kNoTid, PbsProperty::GgxHeightCorrelated, 1 );

            setProperty( kNoTid, PbsProperty::ClearCoat, datablock->mClearCoat != 0.0f );
        }
        else if( ( brdf & PbsBrdf::BRDF_MASK ) == PbsBrdf::CookTorrance )
            setProperty( kNoTid, PbsProperty::BrdfCookTorrance, 1 );
        else if( ( brdf & PbsBrdf::BRDF_MASK ) == PbsBrdf::BlinnPhong )
            setProperty( kNoTid, PbsProperty::BrdfBlinnPhong, 1 );

        if( brdf & PbsBrdf::FLAG_HAS_DIFFUSE_FRESNEL )
        {
            setProperty( kNoTid, PbsProperty::FresnelHasDiffuse, 1 );
            if( brdf & PbsBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL )
                setProperty( kNoTid, PbsProperty::FresnelSeparateDiffuse, 1 );
        }

        if( brdf & PbsBrdf::FLAG_LEGACY_MATH )
            setProperty( kNoTid, PbsProperty::LegacyMathBrdf, 1 );
        if( brdf & PbsBrdf::FLAG_FULL_LEGACY )
            setProperty( kNoTid, PbsProperty::RoughnessIsShininess, 1 );

        for( size_t i = 0u; i < PBSM_REFLECTION; ++i )
        {
            uint8 uvSource = datablock->mUvSource[i];
            setProperty( kNoTid, *PbsProperty::UvSourcePtrs[i], uvSource );

            if( datablock->getTexture( static_cast<uint8>( i ) ) &&
                getProperty( kNoTid, *HlmsBaseProp::UvCountPtrs[uvSource] ) < 2 )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Renderable needs at least 2 coordinates in UV set #" +
                                 StringConverter::toString( uvSource ) +
                                 ". Either change the mesh, or change the UV source settings",
                             "HlmsPbs::calculateHashForPreCreate" );
            }
        }

        int numNormalWeights = 0;
        if( datablock->getNormalMapWeight() != 1.0f && datablock->getTexture( PBSM_NORMAL ) )
        {
            setProperty( kNoTid, PbsProperty::NormalWeightTex, 1 );
            ++numNormalWeights;
        }

        for( size_t i = 0u; i < 4u; ++i )
        {
            if( datablock->getTexture( uint8( PBSM_DETAIL0_NM + i ) ) )
            {
                if( datablock->getDetailNormalWeight( (uint8)i ) != 1.0f )
                {
                    setProperty( kNoTid, *PbsProperty::DetailNormalWeights[i], 1 );
                    ++numNormalWeights;
                }
            }
        }

        setProperty( kNoTid, PbsProperty::NormalWeight, numNormalWeights );

        if( datablock->mTexturesDescSet )
            setDetailMapProperties( kNoTid, datablock, inOutPieces, false );
        else
            setProperty( kNoTid, PbsProperty::FirstValidDetailMapNm, 4 );

        if( datablock->mSamplersDescSet )
        {
            setProperty( kNoTid, PbsProperty::NumSamplers,
                         (int32)datablock->mSamplersDescSet->mSamplers.size() );
        }

        if( datablock->mTexturesDescSet )
        {
            bool envMap = datablock->getTexture( PBSM_REFLECTION ) != 0;
            setProperty( kNoTid, PbsProperty::NumTextures,
                         int32( datablock->mTexturesDescSet->mTextures.size() - envMap ) );

            setTextureProperty( kNoTid, PbsProperty::DiffuseMap, datablock, PBSM_DIFFUSE );
            setTextureProperty( kNoTid, PbsProperty::NormalMapTex, datablock, PBSM_NORMAL );
            setTextureProperty( kNoTid, PbsProperty::SpecularMap, datablock, PBSM_SPECULAR );
            setTextureProperty( kNoTid, PbsProperty::RoughnessMap, datablock, PBSM_ROUGHNESS );
            setTextureProperty( kNoTid, PbsProperty::EmissiveMap, datablock, PBSM_EMISSIVE );
            setTextureProperty( kNoTid, PbsProperty::EnvProbeMap, datablock, PBSM_REFLECTION );
            setTextureProperty( kNoTid, PbsProperty::DetailWeightMap, datablock, PBSM_DETAIL_WEIGHT );

            if( getProperty( kNoTid, PbsProperty::DiffuseMap ) )
            {
                if( datablock->getUseDiffuseMapAsGrayscale() )
                    setProperty( kNoTid, PbsProperty::DiffuseMapGrayscale, 1 );
            }

            if( datablock->getTexture( PBSM_EMISSIVE ) &&
                getProperty( kNoTid, PbsProperty::EmissiveMap ) )
            {
                TextureGpu *emissiveTexture = datablock->getTexture( PBSM_EMISSIVE );
                if( PixelFormatGpuUtils::getNumberOfComponents( emissiveTexture->getPixelFormat() ) ==
                    1u )
                {
                    setProperty( kNoTid, PbsProperty::EmissiveMapGrayscale, 1 );
                }
            }

            // Save the name of the cubemap for hazard prevention
            //(don't sample the cubemap and render to it at the same time).
            const TextureGpu *reflectionTexture = datablock->getTexture( PBSM_REFLECTION );
            if( reflectionTexture )
            {
                // Manual reflection texture
                if( datablock->getCubemapProbe() )
                    setProperty( kNoTid, PbsProperty::UseParallaxCorrectCubemaps, 1 );
                setProperty( kNoTid, PbsProperty::EnvProbeMap,
                             static_cast<int32>( reflectionTexture->getName().getU32Value() ) );
            }
        }

        if( datablock->_hasEmissive() )
        {
            if( datablock->hasEmissiveConstant() )
                setProperty( kNoTid, PbsProperty::EmissiveConstant, 1 );

            if( datablock->getTexture( PBSM_EMISSIVE ) )
            {
                if( datablock->getUseEmissiveAsLightmap() )
                    setProperty( kNoTid, PbsProperty::EmissiveAsLightmap, 1 );
            }
        }

        // normal maps used in this datablock
        StackVector<TextureGpu *, 5u> datablockNormalMaps;

        // normal maps that are being used and have also been loaded
        // NB We could just use the loadedNormalMaps for the logic below
        // however other parts of the code may make assumptions based on
        // the fact a normal map texture is registered with datablock but not loaded.
        StackVector<TextureGpu *, 5u> loadedNormalMaps;
        TextureGpu *tempTex;
        if( ( tempTex = datablock->getTexture( PBSM_NORMAL ) ) != 0 )
        {
            datablockNormalMaps.push_back( tempTex );
            if( tempTex->isMetadataReady() )
                loadedNormalMaps.push_back( tempTex );
        }

        for( size_t i = PBSM_DETAIL0_NM; i <= PBSM_DETAIL3_NM; ++i )
        {
            if( ( tempTex = datablock->getTexture( (uint8)i ) ) != 0 )
            {
                datablockNormalMaps.push_back( tempTex );
                if( tempTex->isMetadataReady() )
                    loadedNormalMaps.push_back( tempTex );
            }
        }

        setProperty( kNoTid, PbsProperty::NormalMap, !datablockNormalMaps.empty() );

        /*setProperty(  kNoTid,HlmsBaseProp::, (bool)datablock->getTexture( PBSM_DETAIL0 ) );
        setProperty(  kNoTid,HlmsBaseProp::DiffuseMap, (bool)datablock->getTexture( PBSM_DETAIL1 ) );*/
        bool normalMapCanBeSupported = ( getProperty( kNoTid, HlmsBaseProp::Normal ) &&
                                         getProperty( kNoTid, HlmsBaseProp::Tangent ) ) ||
                                       getProperty( kNoTid, HlmsBaseProp::QTangent ) ||
                                       getProperty( kNoTid, HlmsBaseProp::ParticleSystem );

        if( !normalMapCanBeSupported && !datablockNormalMaps.empty() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Renderable can't use normal maps but datablock wants normal maps. "
                         "Generate Tangents for this mesh to fix the problem or use a "
                         "datablock without normal maps.",
                         "HlmsPbs::calculateHashForPreCreate" );
        }

        if( !datablockNormalMaps.empty() )
        {
            // NB if texture has not loaded yet, getPixelFormat will return PFG_UNKNOWN and so
            // isSigned may be incorrect. However calculateHasFor will be called again when it
            // has loaded, so just assume default for now.
            PixelFormatGpu nmPixelFormat = datablockNormalMaps[0]->getPixelFormat();
            const bool isSigned = PixelFormatGpuUtils::isSigned( nmPixelFormat );
            if( isSigned )
            {
                setProperty( kNoTid, PbsProperty::NormalSamplingFormat,
                             static_cast<int32>( PbsProperty::NormalRgSnorm.getU32Value() ) );
                setProperty( kNoTid, PbsProperty::NormalRgSnorm,
                             static_cast<int32>( PbsProperty::NormalRgSnorm.getU32Value() ) );
            }
            else
            {
                if( nmPixelFormat != PFG_BC3_UNORM )
                {
                    setProperty( kNoTid, PbsProperty::NormalSamplingFormat,
                                 static_cast<int32>( PbsProperty::NormalRgUnorm.getU32Value() ) );
                    setProperty( kNoTid, PbsProperty::NormalRgUnorm,
                                 static_cast<int32>( PbsProperty::NormalRgUnorm.getU32Value() ) );
                }
                else
                {
                    setProperty( kNoTid, PbsProperty::NormalSamplingFormat,
                                 static_cast<int32>( PbsProperty::NormalBc3Unorm.getU32Value() ) );
                    setProperty( kNoTid, PbsProperty::NormalBc3Unorm,
                                 static_cast<int32>( PbsProperty::NormalBc3Unorm.getU32Value() ) );
                }
            }
        }

        // check and ensure all normal maps use the same signed convention
        if( !loadedNormalMaps.empty() )
        {
            const bool isSigned = PixelFormatGpuUtils::isSigned( loadedNormalMaps[0]->getPixelFormat() );

            // check normal maps all use the same one convention
            for( size_t i = 1u; i < loadedNormalMaps.size(); ++i )
            {
                const bool isSignedOther =
                    PixelFormatGpuUtils::isSigned( loadedNormalMaps[i]->getPixelFormat() );
                if( isSignedOther != isSigned )
                {
                    OGRE_EXCEPT(
                        Exception::ERR_INVALID_STATE,
                        "Renderable can't use normal maps and detailed normal maps which have "
                        "different signed conventions in the same datablock. e.g. SNORM vs UNORM ",
                        "HlmsPbs::calculateHashForPreCreate" );
                }
            }
        }

        if( datablock->mUseAlphaFromTextures &&
            ( datablock->getAlphaTest() != CMPF_ALWAYS_PASS ||
              datablock->mBlendblock[0]->isAutoTransparent() ||
              datablock->mBlendblock[0]->mAlphaToCoverage != HlmsBlendblock::A2cDisabled ||  //
              datablock->mAlphaHashing ||
              datablock->mTransparencyMode == HlmsPbsDatablock::Refractive ) &&
            ( getProperty( kNoTid, PbsProperty::DiffuseMap ) ||  //
              getProperty( kNoTid, PbsProperty::DetailMapsDiffuse ) ) )
        {
            setProperty( kNoTid, PbsProperty::UseTextureAlpha, 1 );

            // When we don't use the alpha in the texture, transparency still works but
            // only at material level (i.e. what datablock->mTransparency says). The
            // alpha from the texture will be ignored.
            if( datablock->mTransparencyMode == HlmsPbsDatablock::Transparent )
                setProperty( kNoTid, PbsProperty::TransparentMode, 1 );
        }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        if( mPlanarReflections && mPlanarReflections->hasPlanarReflections( renderable ) )
        {
            if( !mPlanarReflections->_isUpdatingRenderablesHlms() )
                mPlanarReflections->_notifyRenderableFlushedHlmsDatablock( renderable );
            else
                setProperty( kNoTid, PbsProperty::UsePlanarReflections, 1 );
        }
#endif

        if( mFastShaderBuildHack )
            setProperty( kNoTid, PbsProperty::MaterialsPerBuffer, static_cast<int>( 2 ) );
        else
            setProperty( kNoTid, PbsProperty::MaterialsPerBuffer, static_cast<int>( mSlotsPerPool ) );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::calculateHashForPreCaster( Renderable *renderable, PiecesMap *inOutPieces,
                                             const PiecesMap * )
    {
        HlmsPbsDatablock *datablock = static_cast<HlmsPbsDatablock *>( renderable->getDatablock() );
        const bool hasAlphaTestOrHash =
            datablock->getAlphaTest() != CMPF_ALWAYS_PASS || datablock->getAlphaHashing();

        HlmsPropertyVec::iterator itor = mT[kNoTid].setProperties.begin();
        HlmsPropertyVec::iterator endt = mT[kNoTid].setProperties.end();

        while( itor != endt )
        {
            if( itor->keyName == PbsProperty::FirstValidDetailMapNm )
            {
                itor->value = 0;
                ++itor;
            }
            else if( itor->keyName != PbsProperty::HwGammaRead &&
                     itor->keyName != PbsProperty::UvDiffuse &&
                     itor->keyName != HlmsPsoProp::InputLayoutId &&
                     itor->keyName != HlmsBaseProp::Skeleton && itor->keyName != HlmsBaseProp::Pose &&
                     itor->keyName != HlmsBaseProp::PoseHalfPrecision &&
                     itor->keyName != HlmsBaseProp::PoseNormals &&
                     itor->keyName != HlmsBaseProp::BonesPerVertex &&
                     itor->keyName != HlmsBaseProp::DualParaboloidMapping &&
                     ( !hasAlphaTestOrHash || !requiredPropertyByAlphaTest( itor->keyName ) ) )
            {
                itor = mT[kNoTid].setProperties.erase( itor );
                endt = mT[kNoTid].setProperties.end();
            }
            else
            {
                ++itor;
            }
        }

        setupSharedBasicProperties( renderable );

        if( hasAlphaTestOrHash && getProperty( kNoTid, PbsProperty::UseTextureAlpha ) )
        {
            if( datablock->mTexturesDescSet )
                setDetailMapProperties( kNoTid, datablock, inOutPieces, true );

            if( datablock->mSamplersDescSet )
            {
                setProperty( kNoTid, PbsProperty::NumSamplers,
                             (int32)datablock->mSamplersDescSet->mSamplers.size() );
            }

            if( datablock->mTexturesDescSet )
            {
                const bool envMap = datablock->getTexture( PBSM_REFLECTION ) != 0;
                setProperty( kNoTid, PbsProperty::NumTextures,
                             int32( datablock->mTexturesDescSet->mTextures.size() - envMap ) );

                setTextureProperty( kNoTid, PbsProperty::DiffuseMap, datablock, PBSM_DIFFUSE );
                setTextureProperty( kNoTid, PbsProperty::DetailWeightMap, datablock,
                                    PBSM_DETAIL_WEIGHT );

                if( getProperty( kNoTid, PbsProperty::DiffuseMap ) )
                {
                    if( datablock->getUseDiffuseMapAsGrayscale() )
                        setProperty( kNoTid, PbsProperty::DiffuseMapGrayscale, 1 );
                }
            }
        }

        if( mFastShaderBuildHack )
            setProperty( kNoTid, PbsProperty::MaterialsPerBuffer, static_cast<int>( 2 ) );
        else
            setProperty( kNoTid, PbsProperty::MaterialsPerBuffer, static_cast<int>( mSlotsPerPool ) );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::notifyPropertiesMergedPreGenerationStep( const size_t tid )
    {
        Hlms::notifyPropertiesMergedPreGenerationStep( tid );

        const int32 numVctProbes = getProperty( tid, PbsProperty::VctNumProbes );
        const bool hasVct = numVctProbes > 0;
        if( getProperty( tid, HlmsBaseProp::DecalsNormals ) || hasVct )
        {
            // If decals normals are enabled or VCT is used, we need to generate the TBN matrix.
            bool normalMapCanBeSupported = ( getProperty( tid, HlmsBaseProp::Normal ) &&
                                             getProperty( tid, HlmsBaseProp::Tangent ) ) ||
                                           getProperty( tid, HlmsBaseProp::QTangent );

            setProperty( tid, PbsProperty::NormalMap, normalMapCanBeSupported );
        }

        // If the pass does not have planar reflections, then the object cannot use it
        if( !getProperty( tid, PbsProperty::HasPlanarReflections ) )
            setProperty( tid, PbsProperty::UsePlanarReflections, 0 );

        if( getProperty( tid, HlmsBaseProp::Pose ) > 0 )
            setProperty( tid, HlmsBaseProp::VertexId, 1 );

        const int32 envProbeMapVal = getProperty( tid, PbsProperty::EnvProbeMap );
        const bool canUseManualProbe =
            envProbeMapVal && envProbeMapVal != getProperty( tid, PbsProperty::TargetEnvprobeMap );
        if( canUseManualProbe || getProperty( tid, PbsProperty::ParallaxCorrectCubemaps ) )
        {
            setProperty( tid, PbsProperty::UseEnvProbeMap, 1 );

            if( !canUseManualProbe )
            {
                /// "No cubemap"? Then we're in auto mode or...
                /// We're rendering to the cubemap probe we're using as manual.
                /// Use the auto mode as fallback.
                setProperty( tid, PbsProperty::UseParallaxCorrectCubemaps, 1 );
            }
        }

        OGRE_ASSERT_MEDIUM( ( !getProperty( tid, PbsProperty::UseParallaxCorrectCubemaps ) ||
                              getProperty( tid, PbsProperty::ParallaxCorrectCubemaps ) ) &&
                            "Object with manual cubemap probe but "
                            "setParallaxCorrectedCubemap() was not called!" );

        const bool hasIrradianceField = getProperty( tid, PbsProperty::IrradianceField ) != 0;

        bool bNeedsEnvBrdf = false;

        if( getProperty( tid, HlmsBaseProp::UseSsr ) ||
            getProperty( tid, PbsProperty::UseEnvProbeMap ) ||
            getProperty( tid, PbsProperty::UsePlanarReflections ) ||
            getProperty( tid, PbsProperty::AmbientHemisphere ) ||
            getProperty( tid, PbsProperty::AmbientSh ) ||
            getProperty( tid, PbsProperty::EnableCubemapsAuto ) || hasIrradianceField || hasVct )
        {
            setProperty( tid, PbsProperty::NeedsReflDir, 1 );
            setProperty( tid, PbsProperty::NeedsEnvBrdf, 1 );
            bNeedsEnvBrdf = true;
        }

        if( getProperty( tid, HlmsBaseProp::LightsSpot ) ||
            getProperty( tid, HlmsBaseProp::ForwardPlus ) ||
            getProperty( tid, HlmsBaseProp::LightsAreaApprox ) ||
            getProperty( tid, HlmsBaseProp::LightsAreaLtc ) || bNeedsEnvBrdf || hasIrradianceField ||
            hasVct )
        {
            setProperty( tid, PbsProperty::NeedsViewDir, 1 );
        }

        int32 texUnit = mReservedTexBufferSlots;

        if( getProperty( tid, HlmsBaseProp::ForwardPlus ) )
        {
            if( mVaoManager->readOnlyIsTexBuffer() )
                setTextureReg( tid, PixelShader, "f3dLightList", texUnit++ );
            else
                setProperty( tid, "f3dLightList", texUnit++ );

            setTextureReg( tid, PixelShader, "f3dGrid", texUnit++ );
        }

        texUnit += mReservedTexSlots;

        bool depthTextureDefined = false;

        if( getProperty( tid, HlmsBaseProp::UsePrePass ) )
        {
            setTextureReg( tid, PixelShader, "gBuf_normals", texUnit++ );
            setTextureReg( tid, PixelShader, "gBuf_shadowRoughness", texUnit++ );

            if( getProperty( tid, HlmsBaseProp::UsePrePassMsaa ) )
            {
                setTextureReg( tid, PixelShader, "gBuf_depthTexture", texUnit++ );
                depthTextureDefined = true;
            }
            else
            {
                ++texUnit;
            }

            if( getProperty( tid, HlmsBaseProp::UseSsr ) )
                setTextureReg( tid, PixelShader, "ssrTexture", texUnit++ );
        }

        const bool refractionsAvailable = getProperty( tid, HlmsBaseProp::SsRefractionsAvailable );
        if( refractionsAvailable )
        {
            if( !depthTextureDefined && !getProperty( tid, HlmsBaseProp::UsePrePassMsaa ) )
            {
                setTextureReg( tid, PixelShader, "gBuf_depthTexture", texUnit++ );
                depthTextureDefined = true;
            }
            else
            {
                setTextureReg( tid, PixelShader, "depthTextureNoMsaa", texUnit++ );
            }
            setTextureReg( tid, PixelShader, "refractionMap", texUnit++ );
        }

        OGRE_ASSERT_HIGH(
            ( refractionsAvailable || getProperty( tid, HlmsBaseProp::ScreenSpaceRefractions ) == 0 ) &&
            "A material that uses refractions is used in a pass where refractions are unavailable! See "
            "Samples/2.0/ApiUsage/Refractions for which pass refractions must be rendered in" );

        const bool casterPass = getProperty( tid, HlmsBaseProp::ShadowCaster ) != 0;

        if( !casterPass && getProperty( tid, PbsProperty::IrradianceVolumes ) )
            setTextureReg( tid, PixelShader, "irradianceVolume", texUnit++ );

        if( numVctProbes > 0 )
        {
            setTextureReg( tid, PixelShader, "vctProbes", texUnit, numVctProbes );
            texUnit += numVctProbes;
            if( getProperty( tid, PbsProperty::VctAnisotropic ) )
            {
                setTextureReg( tid, PixelShader, "vctProbeX", texUnit, numVctProbes );
                texUnit += numVctProbes;
                setTextureReg( tid, PixelShader, "vctProbeY", texUnit, numVctProbes );
                texUnit += numVctProbes;
                setTextureReg( tid, PixelShader, "vctProbeZ", texUnit, numVctProbes );
                texUnit += numVctProbes;
            }
        }

        if( getProperty( tid, PbsProperty::IrradianceField ) )
        {
            setTextureReg( tid, PixelShader, "ifdColour", texUnit++ );
            setTextureReg( tid, PixelShader, "ifdDepth", texUnit++ );
        }

        if( getProperty( tid, HlmsBaseProp::LightsAreaTexMask ) > 0 )
            setTextureReg( tid, PixelShader, "areaLightMasks", texUnit++ );

        if( getProperty( tid, PbsProperty::LightProfilesTexture ) > 0 )
            setTextureReg( tid, PixelShader, "lightProfiles", texUnit++ );

        if( getProperty( tid, PbsProperty::LtcTextureAvailable ) )
        {
            if( bNeedsEnvBrdf || getProperty( tid, HlmsBaseProp::LightsAreaLtc ) > 0 )
                setTextureReg( tid, PixelShader, "ltcMatrix", texUnit++ );
            else
            {
                // Always occupy the texture unit
                ++texUnit;
            }
        }

        if( getProperty( tid, HlmsBaseProp::EnableDecals ) )
        {
            const int32 decalsDiffuseProp = getProperty( tid, HlmsBaseProp::DecalsDiffuse );
            // This is a regular property!
            setProperty( tid, "decalsSampler", texUnit );

            if( decalsDiffuseProp )
                setTextureReg( tid, PixelShader, "decalsDiffuseTex", texUnit++ );
            if( getProperty( tid, HlmsBaseProp::DecalsNormals ) )
                setTextureReg( tid, PixelShader, "decalsNormalsTex", texUnit++ );
            const int32 decalsEmissiveProp = getProperty( tid, HlmsBaseProp::DecalsEmissive );
            if( decalsEmissiveProp && decalsEmissiveProp != decalsDiffuseProp )
                setTextureReg( tid, PixelShader, "decalsEmissiveTex", texUnit++ );
        }

        const int32 numShadowMaps = getProperty( tid, HlmsBaseProp::NumShadowMapTextures );
        if( numShadowMaps )
        {
            if( getProperty( tid, HlmsBaseProp::NumShadowMapLights ) != 0 )
            {
                char tmpData[32];
                LwString texName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );
                texName = "texShadowMap";
                const size_t baseTexSize = texName.size();

                for( int32 i = 0; i < numShadowMaps; ++i )
                {
                    texName.resize( baseTexSize );
                    texName.a( i );  // texShadowMap0
                    setTextureReg( tid, PixelShader, texName.c_str(), texUnit++ );
                }
            }
            else
            {
                // No visible lights casting shadow maps.
                texUnit += numShadowMaps;
            }
        }

        int cubemapTexUnit = 0;
        const int32 parallaxCorrectCubemaps = getProperty( tid, PbsProperty::ParallaxCorrectCubemaps );
        if( parallaxCorrectCubemaps )
            cubemapTexUnit = texUnit++;

        if( getProperty( tid, HlmsBaseProp::BlueNoise ) )
            setTextureReg( tid, PixelShader, "blueNoiseTex", texUnit++ );

        const int32 hasPlanarReflections = getProperty( tid, PbsProperty::HasPlanarReflections );
        if( hasPlanarReflections )
        {
            const bool usesPlanarReflections =
                getProperty( tid, PbsProperty::UsePlanarReflections ) != 0;
            if( usesPlanarReflections )
                setTextureReg( tid, PixelShader, "planarReflectionTex", texUnit );
            ++texUnit;
        }

        texUnit += mListener->getNumExtraPassTextures( mT[tid].setProperties, casterPass );

        setProperty( tid, PbsProperty::Set0TextureSlotEnd, texUnit );

        const int32 samplerStateStart = texUnit;
        {
            char tmpData[32];
            LwString texName = LwString::FromEmptyPointer( tmpData, sizeof( tmpData ) );
            texName = "textureMaps";
            const size_t baseTexSize = texName.size();

            int32 numTextures = getProperty( tid, PbsProperty::NumTextures );

            for( int32 i = 0; i < numTextures; ++i )
            {
                texName.resize( baseTexSize );
                texName.a( i );  // textureMaps0
                setTextureReg( tid, PixelShader, texName.c_str(), texUnit++ );
            }
        }

        const int32 envProbeMap = getProperty( tid, PbsProperty::EnvProbeMap );
        const int32 targetEnvProbeMap = getProperty( tid, PbsProperty::TargetEnvprobeMap );
        if( ( envProbeMap && envProbeMap != targetEnvProbeMap ) || parallaxCorrectCubemaps )
        {
            if( !envProbeMap || envProbeMap == targetEnvProbeMap )
                setTextureReg( tid, PixelShader, "texEnvProbeMap", cubemapTexUnit );
            else
                setTextureReg( tid, PixelShader, "texEnvProbeMap", texUnit++ );
        }

        setProperty( tid, PbsProperty::Set1TextureSlotEnd, texUnit );

        if( getProperty( tid, HlmsBaseProp::Pose ) )
            setTextureReg( tid, VertexShader, "poseBuf", texUnit++ );

        // This is a regular property!
        setProperty( tid, "samplerStateStart", samplerStateStart );

        if( getProperty( HlmsBaseProp::ParticleSystem ) )
        {
            setProperty( tid, "particleSystemConstSlot", mParticleSystemConstSlot );
            if( mVaoManager->readOnlyIsTexBuffer() )
                setTextureReg( tid, VertexShader, "particleSystemGpuData", mParticleSystemSlot );
            else
                setProperty( tid, "particleSystemGpuData", mParticleSystemSlot );

            if( !casterPass )
            {
                setProperty( kNoTid, HlmsBaseProp::Normal, 1 );
                setProperty( kNoTid, HlmsBaseProp::Tangent, 1 );
            }
            setProperty( kNoTid, HlmsBaseProp::UvCount, 1 );
        }
    }
    //-----------------------------------------------------------------------------------
    bool HlmsPbs::requiredPropertyByAlphaTest( IdString keyName )
    {
        bool retVal = keyName == PbsProperty::UvDetailWeight || keyName == PbsProperty::UvDetail0 ||
                      keyName == PbsProperty::UvDetail1 || keyName == PbsProperty::UvDetail2 ||
                      keyName == PbsProperty::UvDetail3 || keyName == HlmsBaseProp::UvCount ||
                      keyName == PbsProperty::UseTextureAlpha;

        for( int i = 0; i < 8 && !retVal; ++i )
            retVal |= keyName == *HlmsBaseProp::UvCountPtrs[i];

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    static bool SortByTextureLightMaskIdx( const Light *_a, const Light *_b )
    {
        return _a->mTextureLightMaskIdx < _b->mTextureLightMaskIdx;
    }
    //-----------------------------------------------------------------------------------
#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
    inline float *fillObbRestraint( const Light *light, const Matrix4 &viewMatrix, float *passBufferPtr )
    {
        // float4x3 obbRestraint;
        Node *obbRestraint = light->getObbRestraint();
        if( obbRestraint )
        {
            Matrix4 obbMat = obbRestraint->_getFullTransform();
            obbMat = viewMatrix * obbMat;
            obbMat = obbMat.inverseAffine();
#    if !OGRE_DOUBLE_PRECISION
            memcpy( passBufferPtr, &obbMat, sizeof( float ) * 12u );
            passBufferPtr += 12u;
#    else
            for( size_t i = 0u; i < 12u; ++i )
                *passBufferPtr++ = static_cast<float>( obbMat[0][i] );
#    endif
        }
        else
        {
            memset( passBufferPtr, 0, sizeof( float ) * 12u );
            passBufferPtr += 12u;
        }

        return passBufferPtr;
    }
#endif
    //-----------------------------------------------------------------------------------
    void HlmsPbs::analyzeBarriers( BarrierSolver &barrierSolver,
                                   ResourceTransitionArray &resourceTransitions, Camera *renderingCamera,
                                   const bool bCasterPass )
    {
        if( bCasterPass )
            return;

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        if( mPlanarReflections && mPlanarReflections->cameraMatches( renderingCamera ) )
        {
            const size_t maxActiveActors = mPlanarReflections->getMaxActiveActors();

            for( size_t i = 0u; i < maxActiveActors; ++i )
            {
                barrierSolver.resolveTransition(
                    resourceTransitions, mPlanarReflections->getTexture( (uint8)i ),
                    ResourceLayout::Texture, ResourceAccess::Read, 1u << PixelShader );
            }
        }
#endif
        if( mVctLighting )
        {
            const size_t numCascades = mVctLighting->getNumCascades();
            const size_t numVctTextures = mVctLighting->getNumVoxelTextures();

            for( size_t cascadeIdx = 0; cascadeIdx < numCascades; ++cascadeIdx )
            {
                TextureGpu **lightVoxelTexs = mVctLighting->getLightVoxelTextures( cascadeIdx );
                for( size_t i = 0; i < numVctTextures; ++i )
                {
                    barrierSolver.resolveTransition( resourceTransitions, lightVoxelTexs[i],
                                                     ResourceLayout::Texture, ResourceAccess::Read,
                                                     1u << PixelShader );
                }
            }
        }

        if( mIrradianceField )
        {
            barrierSolver.resolveTransition(  //
                resourceTransitions, mIrradianceField->getIrradianceTex(), ResourceLayout::Texture,
                ResourceAccess::Read, 1u << PixelShader );
            barrierSolver.resolveTransition(
                resourceTransitions, mIrradianceField->getDepthVarianceTex(), ResourceLayout::Texture,
                ResourceAccess::Read, 1u << PixelShader );
        }
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsPbs::preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                                        bool dualParaboloid, SceneManager *sceneManager )
    {
        OgreProfileExhaustive( "HlmsPbs::preparePassHash" );

        mT[kNoTid].setProperties.clear();

        if( shadowNode && mShadowFilter == ExponentialShadowMaps )
            setProperty( kNoTid, PbsProperty::ExponentialShadowMaps, mEsmK );

        // The properties need to be set before preparePassHash so that
        // they are considered when building the HlmsCache's hash.
        if( shadowNode && !casterPass )
        {
            // Shadow receiving can be improved in performance by using gather sampling.
            //(it's the only feature so far that uses gather)
            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
            if( capabilities->hasCapability( RSC_TEXTURE_GATHER ) )
                setProperty( kNoTid, HlmsBaseProp::TexGather, 1 );

            if( mShadowFilter == PCF_3x3 )
            {
                setProperty( kNoTid, PbsProperty::Pcf, 3 );
                setProperty( kNoTid, PbsProperty::PcfIterations, 4 );
            }
            else if( mShadowFilter == PCF_4x4 )
            {
                setProperty( kNoTid, PbsProperty::Pcf, 4 );
                setProperty( kNoTid, PbsProperty::PcfIterations, 9 );
            }
            else if( mShadowFilter == PCF_5x5 )
            {
                setProperty( kNoTid, PbsProperty::Pcf, 5 );
                setProperty( kNoTid, PbsProperty::PcfIterations, 16 );
            }
            else if( mShadowFilter == PCF_6x6 )
            {
                setProperty( kNoTid, PbsProperty::Pcf, 6 );
                setProperty( kNoTid, PbsProperty::PcfIterations, 25 );
            }
            else if( mShadowFilter == ExponentialShadowMaps )
            {
                // Already set
                // setProperty(  kNoTid,PbsProperty::ExponentialShadowMaps, 1 );
            }
            else
            {
                setProperty( kNoTid, PbsProperty::PcfIterations, 1 );
            }

            if( mShadowReceiversInPixelShader )
                setProperty( kNoTid, PbsProperty::ShadowsReceiveOnPs, 1 );

            if( mDebugPssmSplits )
            {
                int32 numPssmSplits = 0;
                const vector<Real>::type *pssmSplits = shadowNode->getPssmSplits( 0 );
                if( pssmSplits )
                {
                    numPssmSplits = static_cast<int32>( pssmSplits->size() - 1 );
                    if( numPssmSplits > 0 )
                        setProperty( kNoTid, PbsProperty::DebugPssmSplits, 1 );
                }
            }
        }

        mPrePassTextures = &sceneManager->getCurrentPrePassTextures();
        assert( mPrePassTextures->empty() || mPrePassTextures->size() == 2u );

        mPrePassMsaaDepthTexture = 0;
        if( !mPrePassTextures->empty() && ( *mPrePassTextures )[0]->isMultisample() )
        {
            TextureGpu *msaaDepthTexture = sceneManager->getCurrentPrePassDepthTexture();
            if( msaaDepthTexture )
                mPrePassMsaaDepthTexture = msaaDepthTexture;
            assert(
                ( mPrePassMsaaDepthTexture && mPrePassMsaaDepthTexture->getSampleDescription() ==
                                                  ( *mPrePassTextures )[0]->getSampleDescription() ) &&
                "Prepass + MSAA must specify an MSAA depth texture" );
        }

        mDepthTexture = sceneManager->getCurrentPrePassDepthTexture();
        mDepthTextureNoMsaa = sceneManager->getCurrentPassDepthTextureNoMsaa();

        mSsrTexture = sceneManager->getCurrentSsrTexture();
        assert( !( mPrePassTextures->empty() && mSsrTexture ) &&
                "Using SSR *requires* to be in prepass mode" );

        mRefractionsTexture = sceneManager->getCurrentRefractionsTexture();

        OGRE_ASSERT_LOW( ( !mRefractionsTexture || ( mRefractionsTexture && mDepthTextureNoMsaa ) ) &&
                         "Refractions texture requires a depth texture!" );

        const bool vctNeedsAmbientHemi =
            !casterPass && mVctLighting && mVctLighting->needsAmbientHemisphere();

        AmbientLightMode ambientMode = mAmbientLightMode;
        ColourValue upperHemisphere = sceneManager->getAmbientLightUpperHemisphere();
        ColourValue lowerHemisphere = sceneManager->getAmbientLightLowerHemisphere();

        const float envMapScale = upperHemisphere.a;
        // Ignore alpha channel
        upperHemisphere.a = lowerHemisphere.a = 1.0;

        const CompositorPass *pass = sceneManager->getCurrentCompositorPass();
        CompositorPassSceneDef const *passSceneDef = 0;

        if( pass && pass->getType() == PASS_SCENE )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef *>( pass->getDefinition() ) );
            passSceneDef = static_cast<const CompositorPassSceneDef *>( pass->getDefinition() );
        }
        const bool isInstancedStereo = passSceneDef && passSceneDef->mInstancedStereo;
        if( isInstancedStereo )
            setProperty( kNoTid, HlmsBaseProp::VPos, 1 );

        mAtmosphere = sceneManager->getAtmosphere();

        if( mHlmsManager->getBlueNoiseTexture() )
            setProperty( kNoTid, HlmsBaseProp::BlueNoise, 1 );

        if( !casterPass )
        {
            if( mIndustryCompatible )
                setProperty( kNoTid, PbsProperty::IndustryCompatible, 1 );
            if( mPerceptualRoughness )
                setProperty( kNoTid, PbsProperty::PerceptualRoughness, 1 );
            if( mLightProfilesTexture )
                setProperty( kNoTid, PbsProperty::LightProfilesTexture, 1 );
            if( mLtcMatrixTexture )
                setProperty( kNoTid, PbsProperty::LtcTextureAvailable, 1 );

            if( mAmbientLightMode == AmbientAutoNormal || mAmbientLightMode == AmbientAutoInverted )
            {
                if( upperHemisphere == lowerHemisphere )
                {
                    if( upperHemisphere == ColourValue::Black )
                        ambientMode = AmbientNone;
                    else
                        ambientMode = AmbientFixed;
                }
                else
                {
                    if( mAmbientLightMode == AmbientAutoInverted )
                        ambientMode = AmbientHemisphereNormal;
                    else
                        ambientMode = AmbientHemisphereInverted;
                }
            }

            if( ambientMode == AmbientFixed )
                setProperty( kNoTid, PbsProperty::AmbientFixed, 1 );
            if( ambientMode == AmbientHemisphereNormal || ambientMode == AmbientHemisphereInverted )
            {
                setProperty( kNoTid, PbsProperty::AmbientHemisphere, 1 );
                if( ambientMode == AmbientHemisphereInverted )
                    setProperty( kNoTid, PbsProperty::AmbientHemisphereInverted, 1 );
            }
            if( ambientMode == AmbientSh || ambientMode == AmbientShMonochrome )
            {
                setProperty( kNoTid, PbsProperty::AmbientSh, 1 );
                if( ambientMode == AmbientShMonochrome )
                    setProperty( kNoTid, PbsProperty::AmbientShMonochrome, 1 );
            }

            if( envMapScale != 1.0f )
                setProperty( kNoTid, PbsProperty::EnvMapScale, 1 );

            const uint32 envFeatures = sceneManager->getEnvFeatures();

            if( envFeatures & SceneManager::EnvFeatures_DiffuseGiFromReflectionProbe )
                setProperty( kNoTid, PbsProperty::CubemapsAsDiffuseGi, 1 );

            // Save cubemap's name so that we never try to render & sample to/from it at the same time
            RenderPassDescriptor *renderPassDescriptor = mRenderSystem->getCurrentPassDescriptor();
            if( renderPassDescriptor->getNumColourEntries() > 0u )
            {
                TextureGpu *firstColourTarget = renderPassDescriptor->mColour[0].texture;
                if( firstColourTarget->getTextureType() == TextureTypes::TypeCube )
                {
                    setProperty( kNoTid, PbsProperty::TargetEnvprobeMap,
                                 static_cast<int32>( firstColourTarget->getName().getU32Value() ) );
                }
            }

            if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
            {
                setProperty( kNoTid, PbsProperty::ParallaxCorrectCubemaps, 1 );
                if( mParallaxCorrectedCubemap->getAutomaticMode() )
                {
                    setProperty( kNoTid, PbsProperty::EnableCubemapsAuto, 1 );
                    if( mParallaxCorrectedCubemap->getUseDpm2DArray() )
                        setProperty( kNoTid, PbsProperty::CubemapsUseDpm, 1 );
                }
            }

            if( mVctLighting )
            {
                setProperty( kNoTid, PbsProperty::VctNumProbes,
                             static_cast<int32>( mVctLighting->getNumCascades() ) );
                setProperty( kNoTid, PbsProperty::VctConeDirs, mVctFullConeCount ? 6 : 4 );
                setProperty( kNoTid, PbsProperty::VctAnisotropic, mVctLighting->isAnisotropic() );
                setProperty( kNoTid, PbsProperty::VctEnableSpecularSdfQuality,
                             mVctLighting->shouldEnableSpecularSdfQuality() );
                setProperty( kNoTid, PbsProperty::VctAmbientSphere, vctNeedsAmbientHemi );

                //'Static' reflections on cubemaps look horrible
                if( mParallaxCorrectedCubemap && mParallaxCorrectedCubemap->isRendering() )
                    setProperty( kNoTid, PbsProperty::VctDisableSpecular, 1 );
            }

            if( mIrradianceField )
            {
                setProperty( kNoTid, PbsProperty::IrradianceField, 1 );
                setProperty( kNoTid, PbsProperty::VctDisableDiffuse, 1 );
            }

            if( mIrradianceVolume )
                setProperty( kNoTid, PbsProperty::IrradianceVolumes, 1 );

            if( mAreaLightMasks )
            {
                const size_t numComponents =
                    PixelFormatGpuUtils::getNumberOfComponents( mAreaLightMasks->getPixelFormat() );
                if( numComponents > 2u )
                    setProperty( kNoTid, HlmsBaseProp::LightsAreaTexColour, 1 );
            }

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
            if( mUseObbRestraintAreaApprox )
                setProperty( kNoTid, PbsProperty::ObbRestraintApprox, 1 );
            if( mUseObbRestraintAreaLtc )
                setProperty( kNoTid, PbsProperty::ObbRestraintLtc, 1 );
#endif

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            mHasPlanarReflections = false;
            mLastBoundPlanarReflection = 0u;
            if( mPlanarReflections && mPlanarReflections->cameraMatches(
                                          sceneManager->getCamerasInProgress().renderingCamera ) )
            {
                mHasPlanarReflections = true;
                setProperty( kNoTid, PbsProperty::HasPlanarReflections,
                             mPlanarReflections->getMaxActiveActors() );
            }
#endif

#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
            if( mFineLightMaskGranularity )
                setProperty( kNoTid, HlmsBaseProp::FineLightMask, 1 );
#endif
        }

        const bool bNeedsViewMatrix = msHasParticleFX2Plugin;

        {
            uint32 numPassConstBuffers = 3u;
            if( !casterPass )
            {
                if( mUseLightBuffers )
                    numPassConstBuffers += 3u;
                numPassConstBuffers += mAtmosphere->preparePassHash( this, numPassConstBuffers );
            }

            if( msHasParticleFX2Plugin )
            {
                setProperty( kNoTid, HlmsBaseProp::ViewMatrix, 1 );
                mParticleSystemConstSlot = static_cast<uint8>( numPassConstBuffers );
                ++numPassConstBuffers;
            }

            mNumPassConstBuffers = numPassConstBuffers;

            setProperty( kNoTid, PbsProperty::NumPassConstBuffers, int32( numPassConstBuffers ) );
        }

        if( mOptimizationStrategy == LowerGpuOverhead )
            setProperty( kNoTid, PbsProperty::LowerGpuOverhead, 1 );

        HlmsCache retVal =
            Hlms::preparePassHashBase( shadowNode, casterPass, dualParaboloid, sceneManager );

        if( mUseLightBuffers )
            setProperty( kNoTid, PbsProperty::useLightBuffers, 1 );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        setProperty( kNoTid, PbsProperty::HwGammaRead, capabilities->hasCapability( RSC_HW_GAMMA ) );
        setProperty( kNoTid, PbsProperty::HwGammaWrite, 1 );
        retVal.setProperties = mT[kNoTid].setProperties;

        CamerasInProgress cameras = sceneManager->getCamerasInProgress();
        mConstantBiasScale = cameras.renderingCamera->_getConstantBiasScale();
        Matrix4 viewMatrix = cameras.renderingCamera->getVrViewMatrix( 0 );

        Matrix4 projectionMatrix = cameras.renderingCamera->getProjectionMatrixWithRSDepth();
        RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();

        if( renderPassDesc->requiresTextureFlipping() )
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];
        }

        const size_t numLights = static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::LightsSpot ) );
        const size_t numDirectionalLights =
            static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::LightsDirNonCaster ) );
        const size_t numShadowMapLights =
            static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::NumShadowMapLights ) );
        const size_t numPssmSplits =
            static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::PssmSplits ) );
        const size_t numAreaApproxLights =
            static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::LightsAreaApprox ) );
        const size_t numAreaLtcLights =
            static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::LightsAreaLtc ) );
        const uint32 realNumDirectionalLights = mRealNumDirectionalLights;
        const uint32 realNumAreaApproxLightsWithMask = mRealNumAreaApproxLightsWithMask;
        const uint32 realNumAreaApproxLights = mRealNumAreaApproxLights;
        const uint32 realNumAreaLtcLights = mRealNumAreaLtcLights;
#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
        const size_t numAreaApproxFloat4Vars = 7u + ( mUseObbRestraintAreaApprox ? 4u : 0u );
        const size_t numAreaLtcFloat4Vars = 7u + ( mUseObbRestraintAreaLtc ? 3u : 0u );
#else
        const size_t numAreaApproxFloat4Vars = 7u;
        const size_t numAreaLtcFloat4Vars = 7u;
#endif

        bool isPssmBlend = getProperty( kNoTid, HlmsBaseProp::PssmBlend ) != 0;
        bool isPssmFade = getProperty( kNoTid, HlmsBaseProp::PssmFade ) != 0;
        bool isStaticBranchShadowMapLights =
            getProperty( kNoTid, HlmsBaseProp::StaticBranchShadowMapLights ) != 0;

        bool isShadowCastingPointLight = false;

        // mat4 viewProj;
        size_t mapSize = 16 * 4;
        size_t mapSizeLight0 = 0;
        size_t mapSizeLight1 = 0;
        size_t mapSizeLight2 = 0;

        mGridBuffer = 0;
        mGlobalLightListBuffer = 0;

        mDecalsTextures[0] = 0;
        mDecalsTextures[1] = 0;
        mDecalsTextures[2] = 0;
        mDecalsDiffuseMergedEmissive = true;

        ForwardPlusBase *forwardPlus = sceneManager->_getActivePassForwardPlus();

        if( !casterPass )
        {
            if( forwardPlus )
            {
                mapSize += forwardPlus->getConstBufferSize();
                mGridBuffer = forwardPlus->getGridBuffer( cameras.cullingCamera );
                mGlobalLightListBuffer = forwardPlus->getGlobalLightListBuffer( cameras.cullingCamera );

                if( forwardPlus->getDecalsEnabled() )
                {
                    const PrePassMode prePassMode = sceneManager->getCurrentPrePassMode();
                    if( prePassMode != PrePassCreate )
                        mDecalsTextures[0] = sceneManager->getDecalsDiffuse();
                    if( prePassMode != PrePassUse )
                        mDecalsTextures[1] = sceneManager->getDecalsNormals();
                    if( prePassMode != PrePassCreate )
                        mDecalsTextures[2] = sceneManager->getDecalsEmissive();
                    mDecalsDiffuseMergedEmissive = sceneManager->isDecalsDiffuseEmissiveMerged();
                }
            }

            if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
            {
                mParallaxCorrectedCubemap->_notifyPreparePassHash( viewMatrix );
                mapSize += mParallaxCorrectedCubemap->getConstBufferSize();
            }

            if( mVctLighting )
                mapSize += mVctLighting->getConstBufferSize();

            if( mIrradianceField )
                mapSize += mIrradianceField->getConstBufferSize();

            // mat4 view + mat4 shadowRcv[numShadowMapLights].texViewProj +
            //              vec2 shadowRcv[numShadowMapLights].shadowDepthRange +
            //              float normalOffsetBias +
            //              float padding +
            //              vec4 shadowRcv[numShadowMapLights].invShadowMapSize +
            // mat3 invViewMatCubemap (upgraded to three vec4)
            mapSize += ( 16 + ( 16 + 2 + 2 + 4 ) * numShadowMapLights + 4 * 3 ) * 4;

            // float4 pccVctMinDistance_invPccVctInvDistance_rightEyePixelStartX_envMapNumMipmaps;
            mapSize += 4u * 4u;

            // float4 aspectRatio_planarReflNumMips_unused2;
            mapSize += 4u * 4u;

            // float2 invWindowRes + float2 windowResolution
            mapSize += 4u * 4u;

            // vec4 shadowRcv[numShadowMapLights].texViewZRow
            if( mShadowFilter == ExponentialShadowMaps )
                mapSize += ( 4 * 4 ) * numShadowMapLights;

            // vec4 pixelOffset2x
            if( passSceneDef && passSceneDef->mUvBakingSet != 0xFF )
                mapSize += 4u * 4u;

            // vec3 ambientUpperHemi + float envMapScale
            if( ( ambientMode >= AmbientFixed && ambientMode <= AmbientHemisphereInverted ) ||
                envMapScale != 1.0f || vctNeedsAmbientHemi )
            {
                mapSize += 4 * 4;
            }

            // vec3 ambientLowerHemi + padding + vec3 ambientHemisphereDir + padding
            if( ambientMode == AmbientHemisphereNormal || ambientMode == AmbientHemisphereInverted ||
                vctNeedsAmbientHemi )
            {
                mapSize += 8 * 4;
            }

            // float4 sh0 - sh6;
            if( ambientMode == AmbientSh )
                mapSize += 7u * 4u * 4u;

            // float4 sh0 - sh2;
            if( ambientMode == AmbientShMonochrome )
                mapSize += 3u * 4u * 4u;

            // vec3 irradianceOrigin + float maxPower +
            // vec3 irradianceSize + float invHeight + mat4 invView
            if( mIrradianceVolume )
                mapSize += ( 4 + 4 + 4 * 4 ) * 4;

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( mHasPlanarReflections )
                mapSize += mPlanarReflections->getConstBufferSize();
#endif
            // float pssmSplitPoints N times.
            mapSize += numPssmSplits * 4;

            if( isPssmBlend )
            {
                // float pssmBlendPoints N-1 times.
                mapSize += ( numPssmSplits - 1 ) * 4;
            }
            if( isPssmFade )
            {
                // float pssmFadePoint.
                mapSize += 4;
            }
            if( isStaticBranchShadowMapLights )
            {
                // float numShadowMapPointLights;
                // float numShadowMapSpotLights;
                mapSize += 4 + 4;
            }

            mapSize = alignToNextMultiple<size_t>( mapSize, 16 );

            if( numShadowMapLights > 0 )
            {
                // Six variables * 4 (padded vec3) * 4 (bytes) * numLights
                mapSizeLight0 += ( 6 * 4 * 4 ) * numLights;
            }
            else
            {
                // Three variables * 4 (padded vec3) * 4 (bytes) * numDirectionalLights
                mapSizeLight0 += ( 3 * 4 * 4 ) * numDirectionalLights;
            }

            mapSizeLight1 += ( numAreaApproxFloat4Vars * 4 * 4 ) * numAreaApproxLights;
            mapSizeLight2 += ( numAreaLtcFloat4Vars * 4 * 4 ) * numAreaLtcLights;
        }
        else
        {
            // float4x4 view
            if( bNeedsViewMatrix )
                mapSize += 16u * 4u;
            isShadowCastingPointLight = getProperty( kNoTid, HlmsBaseProp::ShadowCasterPoint ) != 0;
            // vec4 cameraPosWS
            if( isShadowCastingPointLight )
                mapSize += 4 * 4;
            // vec4 viewZRow
            if( mShadowFilter == ExponentialShadowMaps )
                mapSize += 4 * 4;
            // vec4 depthRange
            mapSize += ( 2 + 2 ) * 4;
        }

        const bool isCameraReflected = cameras.renderingCamera->isReflected();
        // vec4 clipPlane0
        if( isCameraReflected )
            mapSize += 4 * 4;

        // float4x4 viewProj[2] (second only) + float4 leftToRightView
        if( isInstancedStereo )
        {
            mapSize += 16u * 4u + 4u * 4u;

            // float4x4 leftEyeViewSpaceToCullCamClipSpace
            if( forwardPlus )
                mapSize += 16u * 4u;
        }

        mapSize += mListener->getPassBufferSize( shadowNode, casterPass, dualParaboloid, sceneManager );

        // Arbitrary 16kb (minimum supported by GL), should be enough.
        const size_t maxBufferSizeRaw = 16 * 1024;
        const size_t maxBufferSizeLight0 = ( 6 * 4 * 4 ) * 32;  // 32 forward lights should be enough
        const size_t maxBufferSizeLight1 = ( numAreaApproxFloat4Vars * 4 * 4 ) * 8;  // 8 area lights
        const size_t maxBufferSizeLight2 = ( numAreaLtcFloat4Vars * 4 * 4 ) * 8;     // 8 Ltc area lights

        assert( mapSize <= maxBufferSizeRaw );
        assert( !mUseLightBuffers || mapSizeLight0 <= maxBufferSizeLight0 );
        assert( !mUseLightBuffers || mapSizeLight1 <= maxBufferSizeLight1 );
        assert( !mUseLightBuffers || mapSizeLight2 <= maxBufferSizeLight2 );

        size_t maxBufferSize = maxBufferSizeRaw;
        if( !mUseLightBuffers )
            mapSize += mapSizeLight0 + mapSizeLight1 + mapSizeLight2;

        if( mCurrentPassBuffer >= mPassBuffers.size() )
        {
            mPassBuffers.push_back(
                mVaoManager->createConstBuffer( maxBufferSize, BT_DYNAMIC_PERSISTENT, 0, false ) );
        }

        if( mUseLightBuffers )
        {
            while( mCurrentPassBuffer >= mLight0Buffers.size() )
            {
                mLight0Buffers.push_back( mVaoManager->createConstBuffer(
                    maxBufferSizeLight0, BT_DYNAMIC_PERSISTENT, 0, false ) );
                mLight1Buffers.push_back( mVaoManager->createConstBuffer(
                    maxBufferSizeLight1, BT_DYNAMIC_PERSISTENT, 0, false ) );
                mLight2Buffers.push_back( mVaoManager->createConstBuffer(
                    maxBufferSizeLight2, BT_DYNAMIC_PERSISTENT, 0, false ) );
            }
        }

        ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer++];
        float *passBufferPtr = reinterpret_cast<float *>( passBuffer->map( 0, mapSize ) );

        ConstBufferPacked *light0Buffer = 0;
        ConstBufferPacked *light1Buffer = 0;
        ConstBufferPacked *light2Buffer = 0;
        float *light0BufferPtr = 0;
        float *light1BufferPtr = 0;
        float *light2BufferPtr = 0;
        if( mUseLightBuffers )
        {
            light0Buffer = mLight0Buffers[mCurrentPassBuffer - 1u];
            light1Buffer = mLight1Buffers[mCurrentPassBuffer - 1u];
            light2Buffer = mLight2Buffers[mCurrentPassBuffer - 1u];

            if( mapSizeLight0 > 0 )
                light0BufferPtr = reinterpret_cast<float *>( light0Buffer->map( 0, mapSizeLight0 ) );
            if( mapSizeLight1 > 0 )
                light1BufferPtr = reinterpret_cast<float *>( light1Buffer->map( 0, mapSizeLight1 ) );
            if( mapSizeLight2 > 0 )
                light2BufferPtr = reinterpret_cast<float *>( light2Buffer->map( 0, mapSizeLight2 ) );
        }

#ifndef NDEBUG
        const float *startupPtr = passBufferPtr;
        const float *light0startupPtr = light0BufferPtr;
        const float *light1startupPtr = light1BufferPtr;
        const float *light2startupPtr = light2BufferPtr;
#endif

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------

        if( !isInstancedStereo )
        {
            // mat4 viewProj;
            Matrix4 viewProjMatrix = projectionMatrix * viewMatrix;
            for( size_t i = 0u; i < 16u; ++i )
                *passBufferPtr++ = (float)viewProjMatrix[0][i];
        }
        else
        {
            // float4x4 viewProj[2];
            Matrix4 vrViewMat[2];
            for( size_t eyeIdx = 0u; eyeIdx < 2u; ++eyeIdx )
            {
                vrViewMat[eyeIdx] = cameras.renderingCamera->getVrViewMatrix( eyeIdx );
                Matrix4 vrProjMat = cameras.renderingCamera->getVrProjectionMatrix( eyeIdx );
                if( renderPassDesc->requiresTextureFlipping() )
                {
                    vrProjMat[1][0] = -vrProjMat[1][0];
                    vrProjMat[1][1] = -vrProjMat[1][1];
                    vrProjMat[1][2] = -vrProjMat[1][2];
                    vrProjMat[1][3] = -vrProjMat[1][3];
                }
                Matrix4 viewProjMatrix = vrProjMat * vrViewMat[eyeIdx];
                for( size_t i = 0; i < 16; ++i )
                    *passBufferPtr++ = (float)viewProjMatrix[0][i];
            }

            // float4x4 leftEyeViewSpaceToCullCamClipSpace
            if( forwardPlus )
            {
                Matrix4 cullViewMat = cameras.cullingCamera->getViewMatrix( true );
                Matrix4 cullProjMat = cameras.cullingCamera->getProjectionMatrix();
                if( renderPassDesc->requiresTextureFlipping() )
                {
                    cullProjMat[1][0] = -cullProjMat[1][0];
                    cullProjMat[1][1] = -cullProjMat[1][1];
                    cullProjMat[1][2] = -cullProjMat[1][2];
                    cullProjMat[1][3] = -cullProjMat[1][3];
                }
                Matrix4 leftEyeViewSpaceToCullCamClipSpace;
                leftEyeViewSpaceToCullCamClipSpace =
                    cullProjMat * cullViewMat * vrViewMat[0].inverseAffine();
                for( size_t i = 0u; i < 16u; ++i )
                    *passBufferPtr++ = (float)leftEyeViewSpaceToCullCamClipSpace[0][i];
            }

            // float4 leftToRightView
            const VrData *vrData = cameras.renderingCamera->getVrData();
            if( vrData )
            {
                for( size_t i = 0u; i < 3u; ++i )
                    *passBufferPtr++ = (float)vrData->mLeftToRight[i];
                *passBufferPtr++ = 0;
            }
            else
            {
                for( size_t i = 0u; i < 4u; ++i )
                    *passBufferPtr++ = 0.0f;
            }
        }

        if( mRenderSystem->getInvertedClipSpaceY() )
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];
        }

        // vec4 clipPlane0
        if( isCameraReflected )
        {
            const Plane &reflPlane = cameras.renderingCamera->getReflectionPlane();
            *passBufferPtr++ = (float)reflPlane.normal.x;
            *passBufferPtr++ = (float)reflPlane.normal.y;
            *passBufferPtr++ = (float)reflPlane.normal.z;
            *passBufferPtr++ = (float)reflPlane.d;
        }

        // vec4 cameraPosWS;
        if( isShadowCastingPointLight )
        {
            const Vector3 &camPos = cameras.renderingCamera->getDerivedPosition();
            *passBufferPtr++ = (float)camPos.x;
            *passBufferPtr++ = (float)camPos.y;
            *passBufferPtr++ = (float)camPos.z;
            *passBufferPtr++ = 1.0f;
        }

        mPreparedPass.viewMatrix = viewMatrix;

        mPreparedPass.shadowMaps.clear();

        Viewport *currViewports = mRenderSystem->getCurrentRenderViewports();
        TextureGpu *renderTarget = currViewports[0].getCurrentTarget();

        if( !casterPass )
        {
            // mat4 view;
            for( size_t i = 0; i < 16; ++i )
                *passBufferPtr++ = (float)viewMatrix[0][i];

            size_t shadowMapTexIdx = 0;
            const TextureGpuVec &contiguousShadowMapTex =
                shadowNode ? shadowNode->getContiguousShadowMapTex() : c_emptyTextureContainer;

            for( size_t i = 0u; i < numShadowMapLights; ++i )
            {
                // Skip inactive lights (e.g. no directional lights are available
                // and there's a shadow map that only accepts dir lights)
                while( !shadowNode->isShadowMapIdxActive( shadowMapTexIdx ) )
                    ++shadowMapTexIdx;

                // mat4 shadowRcv[numShadowMapLights].texViewProj
                Matrix4 viewProjTex = shadowNode->getViewProjectionMatrix( shadowMapTexIdx );
                for( size_t j = 0; j < 16; ++j )
                    *passBufferPtr++ = (float)viewProjTex[0][j];

                // vec4 texViewZRow;
                if( mShadowFilter == ExponentialShadowMaps )
                {
                    const Matrix4 &viewTex = shadowNode->getViewMatrix( shadowMapTexIdx );
                    *passBufferPtr++ = viewTex[2][0];
                    *passBufferPtr++ = viewTex[2][1];
                    *passBufferPtr++ = viewTex[2][2];
                    *passBufferPtr++ = viewTex[2][3];
                }

                const Light *shadowLight = shadowNode->getLightAssociatedWith( shadowMapTexIdx );

                // vec2 shadowRcv[numShadowMapLights].shadowDepthRange
                Real fNear, fFar;
                shadowNode->getMinMaxDepthRange( shadowMapTexIdx, fNear, fFar );
                const Real depthRange = fFar - fNear;
                if( shadowLight && shadowLight->getType() == Light::LT_POINT &&
                    mShadowFilter != ExponentialShadowMaps && mRenderSystem->isReverseDepth() )
                {
                    *passBufferPtr++ = fFar;
                }
                else
                {
                    *passBufferPtr++ = fNear;
                }
                *passBufferPtr++ = 1.0f / depthRange;
                *passBufferPtr++ = shadowNode->getNormalOffsetBias( (size_t)shadowMapTexIdx );
                *passBufferPtr++ = 0.0f;  // Padding

                // vec2 shadowRcv[numShadowMapLights].invShadowMapSize
                size_t shadowMapTexContigIdx =
                    shadowNode->getIndexToContiguousShadowMapTex( (size_t)shadowMapTexIdx );
                uint32 texWidth = contiguousShadowMapTex[shadowMapTexContigIdx]->getWidth();
                uint32 texHeight = contiguousShadowMapTex[shadowMapTexContigIdx]->getHeight();
                *passBufferPtr++ = 1.0f / float( texWidth );
                *passBufferPtr++ = 1.0f / float( texHeight );
                *passBufferPtr++ = static_cast<float>( texWidth );
                *passBufferPtr++ = static_cast<float>( texHeight );

                ++shadowMapTexIdx;
            }

            // vec4 pixelOffset2x
            if( passSceneDef && passSceneDef->mUvBakingSet != 0xFF )
            {
                *passBufferPtr++ = static_cast<float>( passSceneDef->mUvBakingOffset.x * Real( 2.0 ) /
                                                       Real( renderTarget->getWidth() ) );
                *passBufferPtr++ = static_cast<float>( passSceneDef->mUvBakingOffset.y * Real( 2.0 ) /
                                                       Real( renderTarget->getHeight() ) );
                *passBufferPtr++ = 0.0f;
                *passBufferPtr++ = 0.0f;
            }
            //---------------------------------------------------------------------------
            //                          ---- PIXEL SHADER ----
            //---------------------------------------------------------------------------

            Matrix3 viewMatrix3, invViewMatrixCubemap;
            viewMatrix.extract3x3Matrix( viewMatrix3 );
            // Cubemaps are left-handed.
            invViewMatrixCubemap = viewMatrix3;
            invViewMatrixCubemap[0][2] = -invViewMatrixCubemap[0][2];
            invViewMatrixCubemap[1][2] = -invViewMatrixCubemap[1][2];
            invViewMatrixCubemap[2][2] = -invViewMatrixCubemap[2][2];
            invViewMatrixCubemap = invViewMatrixCubemap.Inverse();

            // mat3 invViewMatCubemap
            for( size_t i = 0; i < 9; ++i )
            {
#ifdef OGRE_GLES2_WORKAROUND_2
                Matrix3 xRot( 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f );
                xRot = xRot * invViewMatrixCubemap;
                *passBufferPtr++ = (float)xRot[0][i];
#else
                *passBufferPtr++ = (float)invViewMatrixCubemap[0][i];
#endif

                // Alignment: each row/column is one vec4, despite being 3x3
                if( !( ( i + 1 ) % 3 ) )
                    ++passBufferPtr;
            }

            // float4 pccVctMinDistance_invPccVctInvDistance_rightEyePixelStartX_envMapNumMipmaps
            *passBufferPtr++ = mPccVctMinDistance;
            *passBufferPtr++ = mInvPccVctInvDistance;
            *passBufferPtr++ = (float)currViewports[1].getActualLeft();
            *passBufferPtr++ = mMaxSpecIblMipmap;

            {
                const float windowWidth = (float)renderTarget->getWidth();
                const float windowHeight = (float)renderTarget->getHeight();

                // float4 aspectRatio_planarReflNumMips_unused2
                *passBufferPtr++ = windowWidth / windowHeight;
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
                if( mPlanarReflections )
                {
                    // Assume all planar refl. probes have the same num. of mipmaps
                    *passBufferPtr++ = mPlanarReflections->getMaxNumMipmaps();
                }
                else
                {
                    *passBufferPtr++ = 0.0f;
                }
#else
                *passBufferPtr++ = 0.0f;
#endif
                *passBufferPtr++ = 0.0f;
                *passBufferPtr++ = 0.0f;

                // float2 invWindowRes + float2 windowResolution;
                *passBufferPtr++ = 1.0f / windowWidth;
                *passBufferPtr++ = 1.0f / windowHeight;
                *passBufferPtr++ = windowWidth;
                *passBufferPtr++ = windowHeight;
            }

            // vec3 ambientUpperHemi + padding
            if( ( ambientMode >= AmbientFixed && ambientMode <= AmbientHemisphereInverted ) ||
                envMapScale != 1.0f || vctNeedsAmbientHemi )
            {
                *passBufferPtr++ = static_cast<float>( upperHemisphere.r );
                *passBufferPtr++ = static_cast<float>( upperHemisphere.g );
                *passBufferPtr++ = static_cast<float>( upperHemisphere.b );
                *passBufferPtr++ = envMapScale;
            }

            // vec3 ambientLowerHemi + padding + vec3 ambientHemisphereDir + padding
            if( ambientMode == AmbientHemisphereNormal || ambientMode == AmbientHemisphereInverted ||
                vctNeedsAmbientHemi )
            {
                *passBufferPtr++ = static_cast<float>( lowerHemisphere.r );
                *passBufferPtr++ = static_cast<float>( lowerHemisphere.g );
                *passBufferPtr++ = static_cast<float>( lowerHemisphere.b );
                *passBufferPtr++ = 1.0f;

                Vector3 hemisphereDir = viewMatrix3 * sceneManager->getAmbientLightHemisphereDir();
                hemisphereDir.normalise();
                *passBufferPtr++ = static_cast<float>( hemisphereDir.x );
                *passBufferPtr++ = static_cast<float>( hemisphereDir.y );
                *passBufferPtr++ = static_cast<float>( hemisphereDir.z );
                *passBufferPtr++ = 1.0f;
            }

            // float4 sh0 - sh6;
            if( ambientMode == AmbientSh )
            {
                const float *ambientSphericalHarmonics = sceneManager->getSphericalHarmonics();
                for( size_t i = 0u; i < 9u; ++i )
                {
                    *passBufferPtr++ = ambientSphericalHarmonics[i * 3u + 0u];
                    *passBufferPtr++ = ambientSphericalHarmonics[i * 3u + 1u];
                    *passBufferPtr++ = ambientSphericalHarmonics[i * 3u + 2u];
                }
                *passBufferPtr++ = 0.0f;  // Unused / padding
            }

            // float4 sh0 - sh2;
            if( ambientMode == AmbientShMonochrome )
            {
                const float *ambientSphericalHarmonics = sceneManager->getSphericalHarmonics();
                for( size_t i = 0u; i < 9u; ++i )
                    *passBufferPtr++ = ambientSphericalHarmonics[i * 3u + 0u];
                *passBufferPtr++ = 0.0f;  // Unused / padding
                *passBufferPtr++ = 0.0f;  // Unused / padding
                *passBufferPtr++ = 0.0f;  // Unused / padding
            }

            if( mIrradianceVolume )
            {
                const Vector3 irradianceCellSize = mIrradianceVolume->getIrradianceCellSize();
                const Vector3 irradianceVolumeOrigin =
                    mIrradianceVolume->getIrradianceOrigin() / irradianceCellSize;
                const float fTexWidth =
                    static_cast<float>( mIrradianceVolume->getIrradianceVolumeTexture()->getWidth() );
                const float fTexDepth =
                    static_cast<float>( mIrradianceVolume->getIrradianceVolumeTexture()->getDepth() );

                *passBufferPtr++ = static_cast<float>( irradianceVolumeOrigin.x ) / fTexWidth;
                *passBufferPtr++ = static_cast<float>( irradianceVolumeOrigin.y );
                *passBufferPtr++ = static_cast<float>( irradianceVolumeOrigin.z ) / fTexDepth;
                *passBufferPtr++ =
                    mIrradianceVolume->getIrradianceMaxPower() * mIrradianceVolume->getPowerScale();

                const float fTexHeight =
                    static_cast<float>( mIrradianceVolume->getIrradianceVolumeTexture()->getHeight() );

                *passBufferPtr++ = 1.0f / ( fTexWidth * irradianceCellSize.x );
                *passBufferPtr++ = 1.0f / irradianceCellSize.y;
                *passBufferPtr++ = 1.0f / ( fTexDepth * irradianceCellSize.z );
                *passBufferPtr++ = 1.0f / fTexHeight;

                // mat4 invView;
                Matrix4 invViewMatrix = viewMatrix.inverse();
                for( size_t i = 0; i < 16; ++i )
                    *passBufferPtr++ = (float)invViewMatrix[0][i];
            }

            // float pssmSplitPoints
            for( size_t i = 0; i < numPssmSplits; ++i )
                *passBufferPtr++ = ( *shadowNode->getPssmSplits( 0 ) )[i + 1];

            size_t numPssmBlendsAndFade = 0u;
            if( isPssmBlend )
            {
                numPssmBlendsAndFade += numPssmSplits - 1u;

                // float pssmBlendPoints
                for( size_t i = 0u; i < numPssmSplits - 1u; ++i )
                    *passBufferPtr++ = ( *shadowNode->getPssmBlends( 0 ) )[i];
            }
            if( isPssmFade )
            {
                numPssmBlendsAndFade += 1u;

                // float pssmFadePoint
                *passBufferPtr++ = *shadowNode->getPssmFade( 0 );
            }
            if( isStaticBranchShadowMapLights )
            {
                // float numShadowMapPointLights;
                // float numShadowMapSpotLights;
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( passBufferPtr ) = mRealShadowMapPointLights;
                passBufferPtr++;
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( passBufferPtr ) = mRealShadowMapSpotLights;
                passBufferPtr++;
                numPssmBlendsAndFade += 2;
            }

            passBufferPtr += alignToNextMultiple<size_t>( numPssmSplits + numPssmBlendsAndFade, 4 ) -
                             ( numPssmSplits + numPssmBlendsAndFade );

            if( !mUseLightBuffers )
                light0BufferPtr = passBufferPtr;

            if( numShadowMapLights > 0 )
            {
                float invHeightLightProfileTex = 1.0f;
                if( mLightProfilesTexture )
                    invHeightLightProfileTex = 1.0f / float( mLightProfilesTexture->getHeight() );

                // All directional lights (caster and non-caster) are sent.
                // Then non-directional shadow-casting shadow lights are sent.
                size_t shadowLightIdx = 0;
                size_t nonShadowLightIdx = 0;
                const LightListInfo &globalLightList = sceneManager->getGlobalLightList();
                const LightClosestArray &lights = shadowNode->getShadowCastingLights();

                const CompositorShadowNode::LightsBitSet &affectedLights =
                    shadowNode->getAffectedLightsBitSet();

                const size_t shadowCastingDirLights =
                    static_cast<size_t>( getProperty( kNoTid, HlmsBaseProp::LightsDirectional ) );

                for( size_t i = 0u; i < numLights; ++i )
                {
                    Light const *light = 0;

                    if( i >= shadowCastingDirLights && i < numDirectionalLights )
                    {
                        if( i < realNumDirectionalLights )
                        {
                            while( affectedLights[nonShadowLightIdx] )
                                ++nonShadowLightIdx;

                            light = globalLightList.lights[nonShadowLightIdx++];

                            assert( light->getType() == Light::LT_DIRECTIONAL );
                        }
                        else
                        {
                            AutoParamDataSource *autoParamDataSource =
                                sceneManager->_getAutoParamDataSource();
                            light = &autoParamDataSource->_getBlankLight();
                        }
                    }
                    else
                    {
                        // Skip inactive lights (e.g. no directional lights are available
                        // and there's a shadow map that only accepts dir lights)
                        while( !lights[shadowLightIdx].light )
                            ++shadowLightIdx;
                        light = lights[shadowLightIdx++].light;
                    }

                    Vector4 lightPos4 = light->getAs4DVector();
                    Vector3 lightPos;

                    if( light->getType() == Light::LT_DIRECTIONAL )
                        lightPos = viewMatrix3 * Vector3( lightPos4.x, lightPos4.y, lightPos4.z );
                    else
                        lightPos = viewMatrix * Vector3( lightPos4.x, lightPos4.y, lightPos4.z );

                    // vec3 lights[numLights].position
                    *light0BufferPtr++ = lightPos.x;
                    *light0BufferPtr++ = lightPos.y;
                    *light0BufferPtr++ = lightPos.z;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
                    *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light0BufferPtr ) =
                        light->getLightMask();
#endif
                    ++light0BufferPtr;

                    // vec3 lights[numLights].diffuse
                    ColourValue colour = light->getDiffuseColour() * light->getPowerScale();
                    *light0BufferPtr++ = colour.r;
                    *light0BufferPtr++ = colour.g;
                    *light0BufferPtr++ = colour.b;
                    *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light0BufferPtr ) =
                        uint32( realNumDirectionalLights - shadowCastingDirLights );
                    ++light0BufferPtr;

                    // vec3 lights[numLights].specular
                    colour = light->getSpecularColour() * light->getPowerScale();
                    *light0BufferPtr++ = colour.r;
                    *light0BufferPtr++ = colour.g;
                    *light0BufferPtr++ = colour.b;
                    ++light0BufferPtr;

                    // vec3 lights[numLights].attenuation;
                    Real attenRange = light->getAttenuationRange();
                    Real attenLinear = light->getAttenuationLinear();
                    Real attenQuadratic = light->getAttenuationQuadric();
                    *light0BufferPtr++ = attenRange;
                    *light0BufferPtr++ = attenLinear;
                    *light0BufferPtr++ = attenQuadratic;
                    ++light0BufferPtr;

                    const uint16 lightProfileIdx = light->getLightProfileIdx();

                    // vec4 lights[numLights].spotDirection;
                    Vector3 spotDir = viewMatrix3 * light->getDerivedDirection();
                    *light0BufferPtr++ = spotDir.x;
                    *light0BufferPtr++ = spotDir.y;
                    *light0BufferPtr++ = spotDir.z;
                    *light0BufferPtr++ =
                        ( static_cast<float>( lightProfileIdx ) + 0.5f ) * invHeightLightProfileTex;

                    // vec4 lights[numLights].spotParams;
                    if( light->getType() != Light::LT_AREA_APPROX )
                    {
                        Radian innerAngle = light->getSpotlightInnerAngle();
                        Radian outerAngle = light->getSpotlightOuterAngle();
                        *light0BufferPtr++ = 1.0f / ( cosf( innerAngle.valueRadians() * 0.5f ) -
                                                      cosf( outerAngle.valueRadians() * 0.5f ) );
                        *light0BufferPtr++ = cosf( outerAngle.valueRadians() * 0.5f );
                        *light0BufferPtr++ = light->getSpotlightFalloff();
                    }
                    else
                    {
                        Quaternion qRot = light->getParentNode()->_getDerivedOrientation();
                        Vector3 xAxis = viewMatrix3 * qRot.xAxis();
                        *light0BufferPtr++ = xAxis.x;
                        *light0BufferPtr++ = xAxis.y;
                        *light0BufferPtr++ = xAxis.z;
                    }
                    *light0BufferPtr++ = 0.0f;
                }
            }
            else
            {
                // No shadow maps, only send directional lights
                const LightListInfo &globalLightList = sceneManager->getGlobalLightList();

                for( size_t i = 0u; i < realNumDirectionalLights; ++i )
                {
                    assert( globalLightList.lights[i]->getType() == Light::LT_DIRECTIONAL );

                    Vector4 lightPos4 = globalLightList.lights[i]->getAs4DVector();
                    Vector3 lightPos = viewMatrix3 * Vector3( lightPos4.x, lightPos4.y, lightPos4.z );

                    // vec3 lights[numLights].position
                    *light0BufferPtr++ = lightPos.x;
                    *light0BufferPtr++ = lightPos.y;
                    *light0BufferPtr++ = lightPos.z;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
                    *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light0BufferPtr ) =
                        globalLightList.lights[i]->getLightMask();
#endif
                    ++light0BufferPtr;

                    // vec3 lights[numLights].diffuse
                    ColourValue colour = globalLightList.lights[i]->getDiffuseColour() *
                                         globalLightList.lights[i]->getPowerScale();
                    *light0BufferPtr++ = colour.r;
                    *light0BufferPtr++ = colour.g;
                    *light0BufferPtr++ = colour.b;
                    *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light0BufferPtr ) =
                        realNumDirectionalLights;
                    ++light0BufferPtr;

                    // vec3 lights[numLights].specular
                    colour = globalLightList.lights[i]->getSpecularColour() *
                             globalLightList.lights[i]->getPowerScale();
                    *light0BufferPtr++ = colour.r;
                    *light0BufferPtr++ = colour.g;
                    *light0BufferPtr++ = colour.b;
                    ++light0BufferPtr;
                }

                memset(
                    light0BufferPtr, 0,
                    ( numDirectionalLights - realNumDirectionalLights ) * sizeof( float ) * 4u * 3u );
                light0BufferPtr += ( numDirectionalLights - realNumDirectionalLights ) * 4u * 3u;
            }

            if( !mUseLightBuffers )
                passBufferPtr = light0BufferPtr;

            float areaLightNumMipmaps = 0.0f;
            float areaLightNumMipmapsSpecFactor = 0.0f;

            if( mAreaLightMasks )
            {
                // Roughness minimum value is 0.02, so we need to map
                //[0.02; 1.0] -> [0; 1] in the pixel shader that's why we divide by 0.98.
                // The 2.0 is just arbitrary (it looks good)
                areaLightNumMipmaps = float( mAreaLightMasks->getNumMipmaps() - 1u );
                areaLightNumMipmapsSpecFactor = areaLightNumMipmaps * 2.0f / 0.98f;
            }

            // Send area lights. We need them sorted so textured ones
            // come first, as that what our shader expects
            mAreaLights.reserve( numAreaApproxLights );
            mAreaLights.clear();
            const LightListInfo &globalLightList = sceneManager->getGlobalLightList();
            size_t areaLightNumber = 0;
            for( size_t idx = mAreaLightsGlobalLightListStart;
                 idx < globalLightList.lights.size() && areaLightNumber < realNumAreaApproxLights;
                 ++idx )
            {
                if( globalLightList.lights[idx]->getType() == Light::LT_AREA_APPROX )
                {
                    mAreaLights.push_back( globalLightList.lights[idx] );
                    ++areaLightNumber;
                }
            }

            std::sort( mAreaLights.begin(), mAreaLights.end(), SortByTextureLightMaskIdx );

            if( !mUseLightBuffers )
                light1BufferPtr = passBufferPtr;

            for( size_t i = 0u; i < realNumAreaApproxLights; ++i )
            {
                Light const *light = mAreaLights[i];

                Vector4 lightPos4 = light->getAs4DVector();
                Vector3 lightPos = viewMatrix * Vector3( lightPos4.x, lightPos4.y, lightPos4.z );

                // vec3 areaApproxLights[numLights].position
                *light1BufferPtr++ = lightPos.x;
                *light1BufferPtr++ = lightPos.y;
                *light1BufferPtr++ = lightPos.z;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light1BufferPtr ) = light->getLightMask();
#endif
                ++light1BufferPtr;

                // vec3 areaApproxLights[numLights].diffuse
                ColourValue colour = light->getDiffuseColour() * light->getPowerScale();
                *light1BufferPtr++ = colour.r;
                *light1BufferPtr++ = colour.g;
                *light1BufferPtr++ = colour.b;
                *light1BufferPtr++ =
                    areaLightNumMipmaps * ( light->mTexLightMaskDiffuseMipStart / 65535.0f );

                // vec3 areaApproxLights[numLights].specular
                colour = light->getSpecularColour() * light->getPowerScale();
                *light1BufferPtr++ = colour.r;
                *light1BufferPtr++ = colour.g;
                *light1BufferPtr++ = colour.b;
                *light1BufferPtr++ = areaLightNumMipmapsSpecFactor;

                // vec4 areaApproxLights[numLights].attenuation;
                Real attenRange = light->getAttenuationRange();
                Real attenLinear = light->getAttenuationLinear();
                Real attenQuadratic = light->getAttenuationQuadric();
                *light1BufferPtr++ = attenRange;
                *light1BufferPtr++ = attenLinear;
                *light1BufferPtr++ = attenQuadratic;
                *light1BufferPtr++ = static_cast<float>( light->mTextureLightMaskIdx );

                const Vector2 rectSize = light->getDerivedRectSize();

                // vec4 areaApproxLights[numLights].direction;
                Vector3 areaDir = viewMatrix3 * light->getDerivedDirection();
                *light1BufferPtr++ = areaDir.x;
                *light1BufferPtr++ = areaDir.y;
                *light1BufferPtr++ = areaDir.z;
                *light1BufferPtr++ = 1.0f / rectSize.x;

                // vec4 areaApproxLights[numLights].tangent;
                Quaternion qRot = light->getParentNode()->_getDerivedOrientation();
                Vector3 xAxis = viewMatrix3 * -qRot.xAxis();
                *light1BufferPtr++ = xAxis.x;
                *light1BufferPtr++ = xAxis.y;
                *light1BufferPtr++ = xAxis.z;
                *light1BufferPtr++ = 1.0f / rectSize.y;

                // vec4 doubleSided;
                *light1BufferPtr++ = light->getDoubleSided() ? 1.0f : 0.0f;
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light1BufferPtr ) = realNumAreaApproxLights;
                ++light1BufferPtr;
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light1BufferPtr ) =
                    realNumAreaApproxLightsWithMask;
                ++light1BufferPtr;
                *light1BufferPtr++ = 0.0f;

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
                if( mUseObbRestraintAreaApprox )
                {
                    // float4 obbFadeFactorApprox
                    const Vector3 obbRestraintFadeFactor = light->_getObbRestraintFadeFactor();
                    *light1BufferPtr++ = obbRestraintFadeFactor.x;
                    *light1BufferPtr++ = obbRestraintFadeFactor.y;
                    *light1BufferPtr++ = obbRestraintFadeFactor.z;
                    *light1BufferPtr++ = 0.0f;

                    // float4x3 obbRestraint;
                    light1BufferPtr = fillObbRestraint( light, viewMatrix, light1BufferPtr );
                }
#endif
            }

            memset( light1BufferPtr, 0,
                    ( numAreaApproxLights - realNumAreaApproxLights ) * sizeof( float ) * 4u *
                        numAreaApproxFloat4Vars );
            light1BufferPtr +=
                ( numAreaApproxLights - realNumAreaApproxLights ) * 4u * numAreaApproxFloat4Vars;

            if( !mUseLightBuffers )
                passBufferPtr = light1BufferPtr;

            mAreaLights.reserve( numAreaLtcLights );
            mAreaLights.clear();
            areaLightNumber = 0;
            for( size_t idx = mAreaLightsGlobalLightListStart;
                 idx < globalLightList.lights.size() && areaLightNumber < realNumAreaLtcLights; ++idx )
            {
                if( globalLightList.lights[idx]->getType() == Light::LT_AREA_LTC )
                {
                    mAreaLights.push_back( globalLightList.lights[idx] );
                    ++areaLightNumber;
                }
            }

            // std::sort( mAreaLights.begin(), mAreaLights.end(), SortByTextureLightMaskIdx );

            if( !mUseLightBuffers )
                light2BufferPtr = passBufferPtr;

            for( size_t i = 0u; i < realNumAreaLtcLights; ++i )
            {
                Light const *light = mAreaLights[i];

                Vector4 lightPos4 = light->getAs4DVector();
                Vector3 lightPos = viewMatrix * Vector3( lightPos4.x, lightPos4.y, lightPos4.z );

                // vec3 areaLtcLights[numLights].position
                *light2BufferPtr++ = lightPos.x;
                *light2BufferPtr++ = lightPos.y;
                *light2BufferPtr++ = lightPos.z;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
                *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light2BufferPtr ) = light->getLightMask();
#endif
                ++light2BufferPtr;

                const Real attenRange = light->getAttenuationRange();

                // vec3 areaLtcLights[numLights].diffuse
                ColourValue colour = light->getDiffuseColour() * light->getPowerScale();
                *light2BufferPtr++ = colour.r;
                *light2BufferPtr++ = colour.g;
                *light2BufferPtr++ = colour.b;
                *light2BufferPtr++ = attenRange;

                // vec3 areaLtcLights[numLights].specular
                colour = light->getSpecularColour() * light->getPowerScale();
                *light2BufferPtr++ = colour.r;
                *light2BufferPtr++ = colour.g;
                *light2BufferPtr++ = colour.b;
                *light2BufferPtr++ = light->getDoubleSided() ? 1.0f : 0.0f;

                const Vector2 rectSize = light->getDerivedRectSize() * 0.5f;
                Quaternion qRot = light->getParentNode()->_getDerivedOrientation();
                Vector3 xAxis = ( viewMatrix3 * qRot.xAxis() ) * rectSize.x;
                Vector3 yAxis = ( viewMatrix3 * qRot.yAxis() ) * rectSize.y;

                Vector3 rectPoints[4];
                // vec4 areaLtcLights[numLights].points[4];
                rectPoints[0] = lightPos - xAxis - yAxis;
                rectPoints[1] = lightPos + xAxis - yAxis;
                rectPoints[2] = lightPos + xAxis + yAxis;
                rectPoints[3] = lightPos - xAxis + yAxis;

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
                // float4 obbFadeFactorApprox
                const Vector3 obbRestraintFadeFactor = light->_getObbRestraintFadeFactor();
#endif

                for( size_t j = 0; j < 4u; ++j )
                {
                    *light2BufferPtr++ = rectPoints[j].x;
                    *light2BufferPtr++ = rectPoints[j].y;
                    *light2BufferPtr++ = rectPoints[j].z;
#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
                    if( j == 0u )
                    {
                        *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light2BufferPtr ) =
                            realNumAreaLtcLights;
                        ++light2BufferPtr;
                    }
                    else
                        *light2BufferPtr++ = obbRestraintFadeFactor[j - 1u];
#else
                    *reinterpret_cast<uint32 * RESTRICT_ALIAS>( light2BufferPtr ) = realNumAreaLtcLights;
                    ++light2BufferPtr;
#endif
                }

#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
                if( mUseObbRestraintAreaLtc )
                {
                    // float4x3 obbRestraint;
                    light2BufferPtr = fillObbRestraint( light, viewMatrix, light2BufferPtr );
                }
#endif
            }

            memset( light2BufferPtr, 0,
                    ( numAreaLtcLights - realNumAreaLtcLights ) * sizeof( float ) * 4u *
                        numAreaLtcFloat4Vars );
            light2BufferPtr += ( numAreaLtcLights - realNumAreaLtcLights ) * 4u * numAreaLtcFloat4Vars;

            if( !mUseLightBuffers )
                passBufferPtr = light2BufferPtr;

            if( shadowNode )
            {
                mPreparedPass.shadowMaps.reserve( contiguousShadowMapTex.size() );

                TextureGpuVec::const_iterator itShadowMap = contiguousShadowMapTex.begin();
                TextureGpuVec::const_iterator enShadowMap = contiguousShadowMapTex.end();

                while( itShadowMap != enShadowMap )
                {
                    mPreparedPass.shadowMaps.push_back( *itShadowMap );
                    ++itShadowMap;
                }
            }

            if( forwardPlus )
            {
                forwardPlus->fillConstBufferData(
                    sceneManager->getCurrentViewport0(), renderPassDesc->requiresTextureFlipping(),
                    renderTarget->getHeight(), mShaderSyntax, isInstancedStereo, passBufferPtr );
                passBufferPtr += forwardPlus->getConstBufferSize() >> 2u;
            }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( mHasPlanarReflections )
            {
                mPlanarReflections->fillConstBufferData( renderTarget, cameras.renderingCamera,
                                                         passBufferPtr );
                passBufferPtr += mPlanarReflections->getConstBufferSize() >> 2u;
            }
#endif
            if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
            {
                mParallaxCorrectedCubemap->fillConstBufferData( viewMatrix, passBufferPtr );
                passBufferPtr += mParallaxCorrectedCubemap->getConstBufferSize() >> 2u;
            }

            if( mVctLighting )
            {
                mVctLighting->fillConstBufferData( viewMatrix, passBufferPtr );
                passBufferPtr += mVctLighting->getConstBufferSize() >> 2u;
            }

            if( mIrradianceField )
            {
                mIrradianceField->fillConstBufferData( viewMatrix, passBufferPtr );
                passBufferPtr += mIrradianceField->getConstBufferSize() >> 2u;
            }
        }
        else
        {
            if( bNeedsViewMatrix )
            {
                for( size_t i = 0u; i < 16u; ++i )
                    *passBufferPtr++ = (float)viewMatrix[0][i];
            }

            // vec4 viewZRow
            if( mShadowFilter == ExponentialShadowMaps )
            {
                *passBufferPtr++ = viewMatrix[2][0];
                *passBufferPtr++ = viewMatrix[2][1];
                *passBufferPtr++ = viewMatrix[2][2];
                *passBufferPtr++ = viewMatrix[2][3];
            }

            // vec2 depthRange;
            Real fNear, fFar;
            shadowNode->getMinMaxDepthRange( cameras.renderingCamera, fNear, fFar );
            const Real depthRange = fFar - fNear;
            *passBufferPtr++ = fNear;
            *passBufferPtr++ = 1.0f / depthRange;
            passBufferPtr += 2;
        }

        passBufferPtr = mListener->preparePassBuffer( shadowNode, casterPass, dualParaboloid,
                                                      sceneManager, passBufferPtr );

        assert( (size_t)( passBufferPtr - startupPtr ) * 4u == mapSize );

        if( mUseLightBuffers )
        {
            assert( (size_t)( light0BufferPtr - light0startupPtr ) * 4u == mapSizeLight0 );
            assert( (size_t)( light1BufferPtr - light1startupPtr ) * 4u == mapSizeLight1 );
            assert( (size_t)( light2BufferPtr - light2startupPtr ) * 4u == mapSizeLight2 );
            if( mapSizeLight0 > 0 )
                light0Buffer->unmap( UO_KEEP_PERSISTENT );
            if( mapSizeLight1 > 0 )
                light1Buffer->unmap( UO_KEEP_PERSISTENT );
            if( mapSizeLight2 > 0 )
                light2Buffer->unmap( UO_KEEP_PERSISTENT );
        }

        passBuffer->unmap( UO_KEEP_PERSISTENT );

        // mTexBuffers must hold at least one buffer to prevent out of bound exceptions.
        if( mTexBuffers.empty() )
        {
            size_t bufferSize =
                std::min<size_t>( mTextureBufferDefaultSize, mVaoManager->getReadOnlyBufferMaxSize() );
            ReadOnlyBufferPacked *newBuffer = mVaoManager->createReadOnlyBuffer(
                PFG_RGBA32_FLOAT, bufferSize, BT_DYNAMIC_PERSISTENT, 0, false );
            mTexBuffers.push_back( newBuffer );
        }

        mLastDescTexture = 0;
        mLastDescSampler = 0;
        mLastBoundPool = 0;

        if( mShadowFilter == ExponentialShadowMaps )
            mCurrentShadowmapSamplerblock = mShadowmapEsmSamplerblock;
        else if( mShadowmapSamplerblock && !getProperty( kNoTid, HlmsBaseProp::ShadowUsesDepthTexture ) )
            mCurrentShadowmapSamplerblock = mShadowmapSamplerblock;
        else
            mCurrentShadowmapSamplerblock = mShadowmapCmpSamplerblock;

        mTexBufUnitSlotEnd = mReservedTexBufferSlots;
        mTexUnitSlotStart =
            uint32( mPreparedPass.shadowMaps.size() + mReservedTexSlots + mReservedTexBufferSlots +
                    mListener->getNumExtraPassTextures( mT[kNoTid].setProperties, casterPass ) );

        if( !casterPass )
        {
            if( mGridBuffer )
            {
                mTexUnitSlotStart += 2;
                mTexBufUnitSlotEnd += 2;
            }
            if( mIrradianceVolume )
                mTexUnitSlotStart += 1;
            if( mVctLighting )
            {
                mTexUnitSlotStart +=
                    uint32( mVctLighting->getNumVoxelTextures() * mVctLighting->getNumCascades() );
            }
            if( mIrradianceField )
                mTexUnitSlotStart += 2u;
            if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
                mTexUnitSlotStart += 1;
            if( !mPrePassTextures->empty() )
                mTexUnitSlotStart += 2;
            if( mPrePassMsaaDepthTexture )
                mTexUnitSlotStart += 1;
            if( mDepthTexture )
                mTexUnitSlotStart += 1;
            if( mSsrTexture )
                mTexUnitSlotStart += 1;
            if( mDepthTextureNoMsaa && mDepthTexture != mDepthTextureNoMsaa )
                mTexUnitSlotStart += 1;
            if( mRefractionsTexture )
                mTexUnitSlotStart += 1;
            if( mAreaLightMasks && getProperty( kNoTid, HlmsBaseProp::LightsAreaTexMask ) > 0 )
            {
                mTexUnitSlotStart += 1;
                mUsingAreaLightMasks = true;
            }
            else
            {
                mUsingAreaLightMasks = false;
            }
            if( mLightProfilesTexture )
                mTexUnitSlotStart += 1;

            /// LTC / BRDF IBL reserved slot
            if( mLtcMatrixTexture )
                ++mTexUnitSlotStart;

            for( size_t i = 0u; i < 3u; ++i )
            {
                if( mDecalsTextures[i] && ( i != 2u || !mDecalsDiffuseMergedEmissive ) )
                {
                    ++mTexUnitSlotStart;
                }
            }

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( mHasPlanarReflections )
            {
                mTexUnitSlotStart += 1;

                mPlanarReflectionSlotIdx = static_cast<uint8>(
                    mTexUnitSlotStart - 1u -
                    mListener->getNumExtraPassTextures( mT[kNoTid].setProperties, casterPass ) );
            }
#endif
        }

        if( mHlmsManager->getBlueNoiseTexture() )
        {
            // Blue Noise is bound with shadow casters too
            ++mTexUnitSlotStart;
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            // Planar reflections did not account for our slot. Correct it.
            ++mPlanarReflectionSlotIdx;
#endif
        }

        uploadDirtyDatablocks();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsPbs::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                    bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "Trying to use slow-path on a desktop implementation. "
                     "Change the RenderQueue settings.",
                     "HlmsPbs::fillBuffersFor" );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsPbs::fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, true );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsPbs::fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                      bool casterPass, uint32 lastCacheHash,
                                      CommandBuffer *commandBuffer )
    {
        return fillBuffersFor( cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer,
                               false );
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsPbs::fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                    bool casterPass, uint32 lastCacheHash, CommandBuffer *commandBuffer,
                                    bool isV1 )
    {
        assert( dynamic_cast<const HlmsPbsDatablock *>( queuedRenderable.renderable->getDatablock() ) );
        const HlmsPbsDatablock *datablock =
            static_cast<const HlmsPbsDatablock *>( queuedRenderable.renderable->getDatablock() );

        if( OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH( lastCacheHash ) != mType )
        {
            // layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked *passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                VertexShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes() );

            uint32 constBufferSlot = 3u;

            if( mUseLightBuffers )
            {
                ConstBufferPacked *light0Buffer = mLight0Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes() );

                ConstBufferPacked *light1Buffer = mLight1Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes() );

                ConstBufferPacked *light2Buffer = mLight2Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes() );
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes() );

                constBufferSlot = 6u;
            }

            size_t texUnit = mReservedTexBufferSlots;

            if( !casterPass )
            {
                constBufferSlot += mAtmosphere->bindConstBuffers( commandBuffer, constBufferSlot );

                if( mGridBuffer )
                {
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, (uint16)texUnit++, mGlobalLightListBuffer, 0, 0 );
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer( PixelShader, (uint16)texUnit++, mGridBuffer, 0, 0 );
                }

                texUnit += mReservedTexSlots;

                if( !mPrePassTextures->empty() )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, ( *mPrePassTextures )[0], 0 );
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, ( *mPrePassTextures )[1], 0 );
                }

                if( mPrePassMsaaDepthTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() = CbTexture(
                        (uint16)texUnit++, mPrePassMsaaDepthTexture, 0,
                        PixelFormatGpuUtils::isDepth( mPrePassMsaaDepthTexture->getPixelFormat() ) );
                }

                if( mDepthTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, mDepthTexture, mDecalsSamplerblock,
                                   PixelFormatGpuUtils::isDepth( mDepthTexture->getPixelFormat() ) );
                }

                if( mSsrTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, mSsrTexture, 0 );
                }

                if( mDepthTextureNoMsaa && mDepthTextureNoMsaa != mPrePassMsaaDepthTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() = CbTexture(
                        (uint16)texUnit++, mDepthTextureNoMsaa, mDecalsSamplerblock,
                        PixelFormatGpuUtils::isDepth( mDepthTextureNoMsaa->getPixelFormat() ) );
                }

                if( mRefractionsTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit++, mRefractionsTexture, mDecalsSamplerblock );
                }

                if( mIrradianceVolume )
                {
                    TextureGpu *irradianceTex = mIrradianceVolume->getIrradianceVolumeTexture();
                    const HlmsSamplerblock *samplerblock = mIrradianceVolume->getIrradSamplerblock();

                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, irradianceTex, samplerblock );
                    ++texUnit;
                }

                if( mVctLighting )
                {
                    const size_t numCascades = mVctLighting->getNumCascades();
                    const size_t numVctTextures = mVctLighting->getNumVoxelTextures();
                    const HlmsSamplerblock *samplerblock = mVctLighting->getBindTrilinearSamplerblock();
                    for( size_t i = 0u; i < numVctTextures; ++i )
                    {
                        for( size_t cascadeIdx = 0; cascadeIdx < numCascades; ++cascadeIdx )
                        {
                            TextureGpu **lightVoxelTexs =
                                mVctLighting->getLightVoxelTextures( cascadeIdx );
                            *commandBuffer->addCommand<CbTexture>() =
                                CbTexture( (uint16)texUnit, lightVoxelTexs[i], samplerblock );
                            ++texUnit;
                        }
                    }
                }

                if( mIrradianceField )
                {
                    TODO_irradianceField_samplerblock;
                    const HlmsSamplerblock *samplerblock = mDecalsSamplerblock;
                    *commandBuffer->addCommand<CbTexture>() = CbTexture(
                        (uint16)texUnit++, mIrradianceField->getIrradianceTex(), samplerblock );
                    *commandBuffer->addCommand<CbTexture>() = CbTexture(
                        (uint16)texUnit++, mIrradianceField->getDepthVarianceTex(), samplerblock );
                }

                if( mUsingAreaLightMasks )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, mAreaLightMasks, mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                if( mLightProfilesTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, mLightProfilesTexture, mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                if( mLtcMatrixTexture )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, mLtcMatrixTexture, mAreaLightMasksSamplerblock );
                    ++texUnit;
                }

                for( size_t i = 0u; i < 3u; ++i )
                {
                    if( mDecalsTextures[i] && ( i != 2u || !mDecalsDiffuseMergedEmissive ) )
                    {
                        *commandBuffer->addCommand<CbTexture>() =
                            CbTexture( (uint16)texUnit, mDecalsTextures[i], mDecalsSamplerblock );
                        ++texUnit;
                    }
                }

                // We changed HlmsType, rebind the shared textures.
                FastArray<TextureGpu *>::const_iterator itor = mPreparedPass.shadowMaps.begin();
                FastArray<TextureGpu *>::const_iterator end = mPreparedPass.shadowMaps.end();
                while( itor != end )
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, *itor, mCurrentShadowmapSamplerblock );
                    ++texUnit;
                    ++itor;
                }

                if( mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering() )
                {
                    TextureGpu *pccTexture = mParallaxCorrectedCubemap->getBindTexture();
                    const HlmsSamplerblock *samplerblock =
                        mParallaxCorrectedCubemap->getBindTrilinearSamplerblock();
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture( (uint16)texUnit, pccTexture, samplerblock );
                    ++texUnit;
                }
            }

            if( mHlmsManager->getBlueNoiseTexture() )
            {
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture( (uint16)texUnit, mHlmsManager->getBlueNoiseTexture(), 0 );
                ++texUnit;
            }

            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastBoundPool = 0;

            // layout(binding = 2) uniform InstanceBuffer {} instance
            if( mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) + 4 ) <=
                    mCurrentConstBufferSize )
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0 );
            }

            rebindTexBuffer( commandBuffer );

#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            mLastBoundPlanarReflection = 0u;
            if( mHasPlanarReflections )
                ++texUnit;  // We do not bind this texture now, but its slot is reserved.
#endif
            mListener->hlmsTypeChanged( casterPass, commandBuffer, datablock, texUnit );
        }

        // Don't bind the material buffer on caster passes (important to keep
        // MDI & auto-instancing running on shadow map passes)
        if( mLastBoundPool != datablock->getAssignedPool() &&
            ( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS ||
              datablock->getAlphaHashing() ) )
        {
            // layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool *newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( VertexShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer( PixelShader, 1, newPool->materialBuffer, 0,
                                (uint32)newPool->materialBuffer->getTotalSizeBytes() );
            CubemapProbe *manualProbe = datablock->getCubemapProbe();
            if( manualProbe )
            {
                OGRE_ASSERT_HIGH( manualProbe->getCreator() == mParallaxCorrectedCubemap &&
                                  "Material has manual cubemap probe that does not match the "
                                  "PCC currently set" );
                ConstBufferPacked *probeConstBuf = manualProbe->getConstBufferForManualProbes();
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( PixelShader, uint16( mNumPassConstBuffers ), probeConstBuf, 0, 0 );
            }
            mLastBoundPool = newPool;
        }

        uint32 *RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;
        float *RESTRICT_ALIAS currentMappedTexBuffer = mCurrentMappedTexBuffer;

        bool hasSkeletonAnimation = queuedRenderable.renderable->hasSkeletonAnimation();
        uint32 numPoses = queuedRenderable.renderable->getNumPoses();
        uint32 poseWeightsNumFloats = ( ( numPoses >> 2u ) + std::min( numPoses % 4u, 1u ) ) * 4u;

        const Matrix4 &worldMat = queuedRenderable.movableObject->_getParentNodeFullTransform();

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------

        if( !hasSkeletonAnimation && numPoses == 0 )
        {
            // We need to correct currentMappedConstBuffer to point to the right texture buffer's
            // offset, which may not be in sync if the previous draw had skeletal and/or pose animation.
            const size_t currentConstOffset =
                static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) >>
                ( 2u + !casterPass );
            currentMappedConstBuffer = currentConstOffset + mStartMappedConstBuffer;
            bool exceedsConstBuffer =
                static_cast<size_t>( ( currentMappedConstBuffer - mStartMappedConstBuffer ) + 4u ) >
                mCurrentConstBufferSize;

            const size_t minimumTexBufferSize = 16u * ( 1u + !casterPass );
            bool exceedsTexBuffer =
                ( static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) +
                  minimumTexBufferSize ) >= mCurrentTexBufferSize;

            if( exceedsConstBuffer || exceedsTexBuffer )
            {
                currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

                if( exceedsTexBuffer )
                    mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
                else
                    rebindTexBuffer( commandBuffer, true, minimumTexBufferSize * sizeof( float ) );

                currentMappedTexBuffer = mCurrentMappedTexBuffer;
            }

            // uint worldMaterialIdx[]
            *currentMappedConstBuffer = datablock->getAssignedSlot() & 0x1FF;

            // mat4x3 world
#if !OGRE_DOUBLE_PRECISION
            memcpy( currentMappedTexBuffer, &worldMat, 4 * 3 * sizeof( float ) );
            currentMappedTexBuffer += 16;
#else
            for( int y = 0; y < 3; ++y )
            {
                for( int x = 0; x < 4; ++x )
                {
                    *currentMappedTexBuffer++ = worldMat[y][x];
                }
            }
            currentMappedTexBuffer += 4;
#endif

            // mat4 worldView
            Matrix4 tmp = mPreparedPass.viewMatrix.concatenateAffine( worldMat );
#if !OGRE_DOUBLE_PRECISION
            memcpy( currentMappedTexBuffer, &tmp, sizeof( Matrix4 ) * !casterPass );
            currentMappedTexBuffer += 16 * !casterPass;
#else
            if( !casterPass )
            {
                for( int y = 0; y < 4; ++y )
                {
                    for( int x = 0; x < 4; ++x )
                    {
                        *currentMappedTexBuffer++ = tmp[y][x];
                    }
                }
            }
#endif
        }
        else
        {
            bool exceedsConstBuffer = (size_t)( ( currentMappedConstBuffer - mStartMappedConstBuffer ) +
                                                4 ) > mCurrentConstBufferSize;

            if( hasSkeletonAnimation )
            {
                if( isV1 )
                {
                    uint16 numWorldTransforms = queuedRenderable.renderable->getNumWorldTransforms();
                    assert( numWorldTransforms <= 256u );

                    const size_t poseDataSize = numPoses > 0u ? ( 4u + poseWeightsNumFloats ) : 0u;
                    const size_t minimumTexBufferSize = 12 * numWorldTransforms + poseDataSize;
                    const bool exceedsTexBuffer =
                        static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) +
                            minimumTexBufferSize >=
                        mCurrentTexBufferSize;

                    if( exceedsConstBuffer || exceedsTexBuffer )
                    {
                        currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

                        if( exceedsTexBuffer )
                            mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
                        else
                            rebindTexBuffer( commandBuffer, true,
                                             minimumTexBufferSize * sizeof( float ) );

                        currentMappedTexBuffer = mCurrentMappedTexBuffer;
                    }

                    // uint worldMaterialIdx[]
                    size_t distToWorldMatStart =
                        static_cast<size_t>( mCurrentMappedTexBuffer - mStartMappedTexBuffer );
                    distToWorldMatStart >>= 2;
                    *currentMappedConstBuffer = uint32( ( distToWorldMatStart << 9 ) |
                                                        ( datablock->getAssignedSlot() & 0x1FF ) );

                    // vec4 worldMat[][3]
                    // TODO: Don't rely on a virtual function + make a direct 4x3 copy
                    Matrix4 tmp[256];
                    queuedRenderable.renderable->getWorldTransforms( tmp );
                    for( size_t i = 0u; i < numWorldTransforms; ++i )
                    {
#if !OGRE_DOUBLE_PRECISION
                        memcpy( currentMappedTexBuffer, &tmp[i], 12 * sizeof( float ) );
                        currentMappedTexBuffer += 12;
#else
                        for( int y = 0; y < 3; ++y )
                        {
                            for( int x = 0; x < 4; ++x )
                            {
                                *currentMappedTexBuffer++ = tmp[i][y][x];
                            }
                        }
#endif
                    }
                }
                else
                {
                    SkeletonInstance *skeleton = queuedRenderable.movableObject->getSkeletonInstance();

                    OGRE_ASSERT_HIGH(
                        dynamic_cast<const RenderableAnimated *>( queuedRenderable.renderable ) );

                    const RenderableAnimated *renderableAnimated =
                        static_cast<const RenderableAnimated *>( queuedRenderable.renderable );

                    const RenderableAnimated::IndexMap *indexMap =
                        renderableAnimated->getBlendIndexToBoneIndexMap();

                    const size_t poseDataSize = numPoses > 0u ? ( 4u + poseWeightsNumFloats ) : 0u;
                    const size_t minimumTexBufferSize = 12 * indexMap->size() + poseDataSize;
                    bool exceedsTexBuffer =
                        static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) +
                            minimumTexBufferSize >=
                        mCurrentTexBufferSize;

                    if( exceedsConstBuffer || exceedsTexBuffer )
                    {
                        currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

                        if( exceedsTexBuffer )
                            mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
                        else
                            rebindTexBuffer( commandBuffer, true,
                                             minimumTexBufferSize * sizeof( float ) );

                        currentMappedTexBuffer = mCurrentMappedTexBuffer;
                    }

                    // uint worldMaterialIdx[]
                    size_t distToWorldMatStart =
                        static_cast<size_t>( mCurrentMappedTexBuffer - mStartMappedTexBuffer );
                    distToWorldMatStart >>= 2;
                    *currentMappedConstBuffer = uint32( ( distToWorldMatStart << 9 ) |
                                                        ( datablock->getAssignedSlot() & 0x1FF ) );

                    RenderableAnimated::IndexMap::const_iterator itBone = indexMap->begin();
                    RenderableAnimated::IndexMap::const_iterator enBone = indexMap->end();

                    while( itBone != enBone )
                    {
                        const SimpleMatrixAf4x3 &mat4x3 = skeleton->_getBoneFullTransform( *itBone );
                        mat4x3.streamTo4x3( currentMappedTexBuffer );
                        currentMappedTexBuffer += 12;

                        ++itBone;
                    }
                }
            }

            if( numPoses > 0 )
            {
                if( !hasSkeletonAnimation )
                {
                    // If not combined with skeleton animation, pose animations are gonna need 1 vec4's
                    // for pose data (base vertex, num vertices), enough vec4's to accomodate
                    // the weight of each pose, 3 vec4's for worldMat, and 4 vec4's for worldView.
                    const size_t minimumTexBufferSize = 4 + poseWeightsNumFloats + 3 * 4 + 4 * 4;
                    bool exceedsTexBuffer =
                        static_cast<size_t>( currentMappedTexBuffer - mStartMappedTexBuffer ) +
                            minimumTexBufferSize >=
                        mCurrentTexBufferSize;

                    if( exceedsConstBuffer || exceedsTexBuffer )
                    {
                        currentMappedConstBuffer = mapNextConstBuffer( commandBuffer );

                        if( exceedsTexBuffer )
                            mapNextTexBuffer( commandBuffer, minimumTexBufferSize * sizeof( float ) );
                        else
                            rebindTexBuffer( commandBuffer, true,
                                             minimumTexBufferSize * sizeof( float ) );

                        currentMappedTexBuffer = mCurrentMappedTexBuffer;
                    }

                    // uint worldMaterialIdx[]
                    size_t distToWorldMatStart =
                        static_cast<size_t>( mCurrentMappedTexBuffer - mStartMappedTexBuffer );
                    distToWorldMatStart >>= 2;
                    *currentMappedConstBuffer = uint32( ( distToWorldMatStart << 9 ) |
                                                        ( datablock->getAssignedSlot() & 0x1FF ) );
                }

                uint8 meshLod = queuedRenderable.movableObject->getCurrentMeshLod();
                const VertexArrayObjectArray &vaos =
                    queuedRenderable.renderable->getVaos( static_cast<VertexPass>( casterPass ) );
                VertexArrayObject *vao = vaos[meshLod];
                // According to this https://forums.ogre3d.org/viewtopic.php?f=25&t=95040#p545057
                // "macOS uses an old OpenGL version which doesn't use baseVertex"
#ifdef __APPLE__
                uint32 baseVertex = 0;
#else
                uint32 baseVertex =
                    static_cast<uint32>( vao->getBaseVertexBuffer()->_getFinalBufferStart() );
#endif
                memcpy( currentMappedTexBuffer, &baseVertex, sizeof( baseVertex ) );
                size_t numVertices = vao->getBaseVertexBuffer()->getNumElements();
                memcpy( currentMappedTexBuffer + 1, &numVertices, sizeof( numVertices ) );
                currentMappedTexBuffer += 4;

                memcpy( currentMappedTexBuffer, queuedRenderable.renderable->getPoseWeights(),
                        sizeof( float ) * numPoses );
                currentMappedTexBuffer += poseWeightsNumFloats;

                if( !hasSkeletonAnimation )
                {
                    // mat4x3 world
#if !OGRE_DOUBLE_PRECISION
                    memcpy( currentMappedTexBuffer, &worldMat, 4 * 3 * sizeof( float ) );
                    currentMappedTexBuffer += 12;
#else
                    for( int y = 0; y < 3; ++y )
                    {
                        for( int x = 0; x < 4; ++x )
                        {
                            *currentMappedTexBuffer++ = worldMat[y][x];
                        }
                    }
#endif

                    // mat4 worldView
                    Matrix4 tmp = mPreparedPass.viewMatrix.concatenateAffine( worldMat );
#if !OGRE_DOUBLE_PRECISION
                    memcpy( currentMappedTexBuffer, &tmp, sizeof( Matrix4 ) * !casterPass );
                    currentMappedTexBuffer += 16 * !casterPass;
#else
                    if( !casterPass )
                    {
                        for( int y = 0; y < 4; ++y )
                        {
                            for( int x = 0; x < 4; ++x )
                            {
                                *currentMappedTexBuffer++ = tmp[y][x];
                            }
                        }
                    }
#endif
                }

                size_t numTextures = 0u;
                if( datablock->mTexturesDescSet )
                    numTextures = datablock->mTexturesDescSet->mTextures.size();

                TexBufferPacked *poseBuf = queuedRenderable.renderable->getPoseTexBuffer();
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer( VertexShader, uint16( mTexUnitSlotStart + numTextures ), poseBuf, 0,
                                    (uint32)poseBuf->getTotalSizeBytes() );
            }

            // If the next entity will not be skeletally animated, we'll need
            // currentMappedTexBuffer to be 16/32-byte aligned.
            // Non-skeletally animated objects are far more common than skeletal ones,
            // so we do this here instead of doing it before rendering the non-skeletal ones.
            size_t currentConstOffset = (size_t)( currentMappedTexBuffer - mStartMappedTexBuffer );
            currentConstOffset =
                alignToNextMultiple<size_t>( currentConstOffset, 16 + 16 * !casterPass );
            currentConstOffset = std::min( currentConstOffset, mCurrentTexBufferSize );
            currentMappedTexBuffer = mStartMappedTexBuffer + currentConstOffset;
        }

        *reinterpret_cast<float * RESTRICT_ALIAS>( currentMappedConstBuffer + 1 ) =
            datablock->mShadowConstantBias * mConstantBiasScale;
#if !OGRE_NO_FINE_LIGHT_MASK_GRANULARITY
        *( currentMappedConstBuffer + 2u ) = queuedRenderable.movableObject->getLightMask();
#endif
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
        *( currentMappedConstBuffer + 3u ) = queuedRenderable.renderable->mCustomParameter & 0x7F;
#endif
        currentMappedConstBuffer += 4;

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        if( !casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS ||
            datablock->getAlphaHashing() )
        {
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
            if( !casterPass && mHasPlanarReflections &&
                ( queuedRenderable.renderable->mCustomParameter & 0x80 /* UseActiveActor */ ) &&
                mLastBoundPlanarReflection != queuedRenderable.renderable->mCustomParameter )
            {
                const uint8 activeActorIdx = queuedRenderable.renderable->mCustomParameter & 0x7F;
                TextureGpu *planarReflTex = mPlanarReflections->getTexture( activeActorIdx );
                *commandBuffer->addCommand<CbTexture>() = CbTexture(
                    uint16( mPlanarReflectionSlotIdx ), planarReflTex, mPlanarReflectionsSamplerblock );
                mLastBoundPlanarReflection = queuedRenderable.renderable->mCustomParameter;
            }
#endif
            if( datablock->mTexturesDescSet != mLastDescTexture )
            {
                if( datablock->mTexturesDescSet )
                {
                    // Rebind textures
                    size_t texUnit = mTexUnitSlotStart;

                    *commandBuffer->addCommand<CbTextures>() = CbTextures(
                        (uint16)texUnit, datablock->mCubemapIdxInDescSet, datablock->mTexturesDescSet );

                    if( !mHasSeparateSamplers )
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
                    }
                    // texUnit += datablock->mTexturesDescSet->mTextures.size();
                }

                mLastDescTexture = datablock->mTexturesDescSet;
            }

            if( datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers )
            {
                if( datablock->mSamplersDescSet )
                {
                    // Bind samplers
                    size_t texUnit = mTexUnitSlotStart;
                    *commandBuffer->addCommand<CbSamplers>() =
                        CbSamplers( (uint16)texUnit, datablock->mSamplersDescSet );
                    mLastDescSampler = datablock->mSamplersDescSet;
                }
            }
        }

        mCurrentMappedConstBuffer = currentMappedConstBuffer;
        mCurrentMappedTexBuffer = currentMappedTexBuffer;

        return uint32( ( ( mCurrentMappedConstBuffer - mStartMappedConstBuffer ) >> 2u ) - 1u );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::destroyAllBuffers()
    {
        HlmsBufferManager::destroyAllBuffers();

        mCurrentPassBuffer = 0;

        {
            ConstBufferPackedVec::const_iterator itor = mPassBuffers.begin();
            ConstBufferPackedVec::const_iterator end = mPassBuffers.end();

            while( itor != end )
            {
                if( ( *itor )->getMappingState() != MS_UNMAPPED )
                    ( *itor )->unmap( UO_UNMAP_ALL );
                mVaoManager->destroyConstBuffer( *itor );
                ++itor;
            }

            mPassBuffers.clear();
        }

        ///////// light buffers
        {
            ConstBufferPackedVec::const_iterator itor = mLight0Buffers.begin();
            ConstBufferPackedVec::const_iterator end = mLight0Buffers.end();

            while( itor != end )
            {
                if( ( *itor )->getMappingState() != MS_UNMAPPED )
                    ( *itor )->unmap( UO_UNMAP_ALL );
                mVaoManager->destroyConstBuffer( *itor );
                ++itor;
            }

            mLight0Buffers.clear();
        }
        {
            ConstBufferPackedVec::const_iterator itor = mLight1Buffers.begin();
            ConstBufferPackedVec::const_iterator end = mLight1Buffers.end();

            while( itor != end )
            {
                if( ( *itor )->getMappingState() != MS_UNMAPPED )
                    ( *itor )->unmap( UO_UNMAP_ALL );
                mVaoManager->destroyConstBuffer( *itor );
                ++itor;
            }

            mLight1Buffers.clear();
        }
        {
            ConstBufferPackedVec::const_iterator itor = mLight2Buffers.begin();
            ConstBufferPackedVec::const_iterator end = mLight2Buffers.end();

            while( itor != end )
            {
                if( ( *itor )->getMappingState() != MS_UNMAPPED )
                    ( *itor )->unmap( UO_UNMAP_ALL );
                mVaoManager->destroyConstBuffer( *itor );
                ++itor;
            }

            mLight2Buffers.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::postCommandBufferExecution( CommandBuffer *commandBuffer )
    {
        HlmsBufferManager::postCommandBufferExecution( commandBuffer );

        if( mPrePassMsaaDepthTexture )
        {
            // We need to unbind the depth texture, it may be used as a depth buffer later.
            size_t texUnit = mReservedTexBufferSlots + mReservedTexSlots + ( mGridBuffer ? 2u : 0u );
            if( !mPrePassTextures->empty() )
                texUnit += 2;

            mRenderSystem->_setTexture( texUnit, 0, false );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::frameEnded()
    {
        HlmsBufferManager::frameEnded();
        mCurrentPassBuffer = 0;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setStaticBranchingLights( bool staticBranchingLights )
    {
        if( staticBranchingLights )
        {
            // Make sure we calculate light positions in pixel shaders
            setShadowReceiversInPixelShader( true );
        }
        Hlms::setStaticBranchingLights( staticBranchingLights );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::resetIblSpecMipmap( uint8 numMipmaps )
    {
        if( numMipmaps != 0u )
        {
            mAutoSpecIblMaxMipmap = false;
            mMaxSpecIblMipmap = numMipmaps;
        }
        else
        {
            mAutoSpecIblMaxMipmap = true;
            mMaxSpecIblMipmap = 1.0f;

            HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
            HlmsDatablockMap::const_iterator endt = mDatablocks.end();

            while( itor != endt )
            {
                assert( dynamic_cast<HlmsPbsDatablock *>( itor->second.datablock ) );
                HlmsPbsDatablock *datablock = static_cast<HlmsPbsDatablock *>( itor->second.datablock );

                TextureGpu *reflTexture = datablock->getTexture( PBSM_REFLECTION );
                if( reflTexture )
                {
                    mMaxSpecIblMipmap =
                        std::max<float>( reflTexture->getNumMipmaps(), mMaxSpecIblMipmap );
                }
                ++itor;
            }

            if( mParallaxCorrectedCubemap )
            {
                TextureGpu *bindTexture = mParallaxCorrectedCubemap->getBindTexture();
                if( bindTexture )
                {
                    mMaxSpecIblMipmap =
                        std::max<float>( bindTexture->getNumMipmaps(), mMaxSpecIblMipmap );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::_notifyIblSpecMipmap( uint8 numMipmaps )
    {
        if( mAutoSpecIblMaxMipmap )
            mMaxSpecIblMipmap = std::max<float>( numMipmaps, mMaxSpecIblMipmap );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::loadLtcMatrix()
    {
        const uint32 poolId = 992044u;  // Same as BlueNoise (BlueNoise has different pixel format)

        TextureGpuManager *textureGpuManager = mRenderSystem->getTextureGpuManager();
        if( !textureGpuManager->hasPoolId( poolId, 64u, 64u, 1u, PFG_RGBA16_FLOAT ) )
            textureGpuManager->reservePoolId( poolId, 64u, 64u, 3u, 1u, PFG_RGBA16_FLOAT );

        TextureGpu *ltcMat0 = textureGpuManager->createOrRetrieveTexture(
            "ltcMatrix0.dds", GpuPageOutStrategy::Discard, TextureFlags::AutomaticBatching,
            TextureTypes::Type2D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, 0, poolId );
        TextureGpu *ltcMat1 = textureGpuManager->createOrRetrieveTexture(
            "ltcMatrix1.dds", GpuPageOutStrategy::Discard, TextureFlags::AutomaticBatching,
            TextureTypes::Type2D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, 0, poolId );
        TextureGpu *brtfLut2 = textureGpuManager->createOrRetrieveTexture(
            "brtfLutDfg.dds", GpuPageOutStrategy::Discard, TextureFlags::AutomaticBatching,
            TextureTypes::Type2D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, 0, poolId );

        ltcMat0->scheduleTransitionTo( GpuResidency::Resident, nullptr, false, true );
        ltcMat1->scheduleTransitionTo( GpuResidency::Resident, nullptr, false, true );
        brtfLut2->scheduleTransitionTo( GpuResidency::Resident, nullptr, false, true );

        ltcMat0->waitForMetadata();
        ltcMat1->waitForMetadata();
        brtfLut2->waitForMetadata();

        OGRE_ASSERT_LOW(
            ltcMat0->getTexturePool() == ltcMat1->getTexturePool() &&
            "ltcMatrix0.dds & ltcMatrix1.dds must be the same resolution and pixel format" );
        OGRE_ASSERT_LOW(
            ltcMat0->getTexturePool() == brtfLut2->getTexturePool() &&
            "ltcMatrix0.dds & brtfLutDfg2.dds must be the same resolution and pixel format" );
        OGRE_ASSERT_LOW( ltcMat0->getInternalSliceStart() == 0u );
        OGRE_ASSERT_LOW( ltcMat1->getInternalSliceStart() == 1u );
        OGRE_ASSERT_LOW( brtfLut2->getInternalSliceStart() == 2u );

        mLtcMatrixTexture = ltcMat0;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths )
    {
        // We need to know what RenderSystem is currently in use, as the
        // name of the compatible shading language is part of the path
        Ogre::RenderSystem *renderSystem = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::String shaderSyntax = "GLSL";
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            shaderSyntax = "HLSL";
        else if( renderSystem->getName() == "Metal Rendering Subsystem" )
            shaderSyntax = "Metal";

        // Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back( "Hlms/Common/" + shaderSyntax );
        outLibraryFoldersPaths.push_back( "Hlms/Common/Any" );
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any" );
#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any/Atmosphere" );
#endif
        outLibraryFoldersPaths.push_back( "Hlms/Pbs/Any/Main" );

        // Fill the data folder path
        outDataFolderPath = "Hlms/Pbs/" + shaderSyntax;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setShadowReceiversInPixelShader( bool bInPixelShader )
    {
        mShadowReceiversInPixelShader = bInPixelShader;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setDebugPssmSplits( bool bDebug ) { mDebugPssmSplits = bDebug; }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setPerceptualRoughness( bool bPerceptualRoughness )
    {
        mPerceptualRoughness = bPerceptualRoughness;
    }
    //-----------------------------------------------------------------------------------
    bool HlmsPbs::getPerceptualRoughness() const { return mPerceptualRoughness; }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setShadowSettings( ShadowFilter filter )
    {
        mShadowFilter = filter;
        ShadowCameraSetup::setUseEsm( filter == ExponentialShadowMaps );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setEsmK( uint16 K )
    {
        assert( K != 0 && "A value of K = 0 is invalid!" );
        mEsmK = K;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setAmbientLightMode( AmbientLightMode mode ) { mAmbientLightMode = mode; }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setParallaxCorrectedCubemap( ParallaxCorrectedCubemapBase *pcc,
                                               float pccVctMinDistance, float pccVctMaxDistance )
    {
        OGRE_ASSERT_LOW( pccVctMinDistance < pccVctMaxDistance );
        mParallaxCorrectedCubemap = pcc;
        mPccVctMinDistance = pccVctMinDistance;
        mInvPccVctInvDistance = 1.0f / ( pccVctMaxDistance - pccVctMinDistance );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setAreaLightMasks( TextureGpu *areaLightMask ) { mAreaLightMasks = areaLightMask; }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setLightProfilesTexture( TextureGpu *lightProfilesTex )
    {
        mLightProfilesTexture = lightProfilesTex;
    }
#ifdef OGRE_BUILD_COMPONENT_PLANAR_REFLECTIONS
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setPlanarReflections( PlanarReflections *planarReflections )
    {
        mPlanarReflections = planarReflections;
        if( !mPlanarReflections )
            mHasPlanarReflections = false;
    }
    //-----------------------------------------------------------------------------------
    PlanarReflections *HlmsPbs::getPlanarReflections() const { return mPlanarReflections; }
#endif
#if OGRE_ENABLE_LIGHT_OBB_RESTRAINT
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setUseObbRestraints( bool areaApprox, bool areaLtc )
    {
        mUseObbRestraintAreaApprox = areaApprox;
        mUseObbRestraintAreaLtc = areaLtc;
    }
#endif
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setUseLightBuffers( bool b ) { mUseLightBuffers = b; }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setDefaultBrdfWithDiffuseFresnel( bool bDefaultToDiffuseFresnel )
    {
        mDefaultBrdfWithDiffuseFresnel = bDefaultToDiffuseFresnel;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::setIndustryCompatible( bool bIndustryCompatible )
    {
        mIndustryCompatible = bIndustryCompatible;
    }
#if !OGRE_NO_JSON
    //-----------------------------------------------------------------------------------
    void HlmsPbs::_loadJson( const rapidjson::Value &jsonValue, const HlmsJson::NamedBlocks &blocks,
                             HlmsDatablock *datablock, const String &resourceGroup,
                             HlmsJsonListener *listener, const String &additionalTextureExtension ) const
    {
        HlmsJsonPbs jsonPbs( mHlmsManager, mRenderSystem->getTextureGpuManager(), listener,
                             additionalTextureExtension );
        jsonPbs.loadMaterial( jsonValue, blocks, datablock, resourceGroup );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::_saveJson( const HlmsDatablock *datablock, String &outString,
                             HlmsJsonListener *listener, const String &additionalTextureExtension ) const
    {
        HlmsJsonPbs jsonPbs( mHlmsManager, mRenderSystem->getTextureGpuManager(), listener,
                             additionalTextureExtension );
        jsonPbs.saveMaterial( datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbs::_collectSamplerblocks( set<const HlmsSamplerblock *>::type &outSamplerblocks,
                                         const HlmsDatablock *datablock ) const
    {
        HlmsJsonPbs::collectSamplerblocks( datablock, outSamplerblocks );
    }
#endif
    //-----------------------------------------------------------------------------------
    HlmsDatablock *HlmsPbs::createDatablockImpl( IdString datablockName,
                                                 const HlmsMacroblock *macroblock,
                                                 const HlmsBlendblock *blendblock,
                                                 const HlmsParamVec &paramVec )
    {
        return OGRE_NEW HlmsPbsDatablock( datablockName, this, macroblock, blendblock, paramVec );
    }
}  // namespace Ogre
