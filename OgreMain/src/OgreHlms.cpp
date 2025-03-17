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

#include "OgreHlms.h"

#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgreBitset.inl"
#include "OgreCamera.h"
#include "OgreDepthBuffer.h"
#include "OgreFileSystem.h"
#include "OgreForward3D.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsManager.h"
#include "OgreLight.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreRenderQueue.h"
#include "OgreRootLayout.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "ParticleSystem/OgreParticleSystem2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#    include "OSX/macUtils.h"
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    include "iOS/macUtils.h"
#endif

#include "Hash/MurmurHash3.h"

#include <fstream>

#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
#    define OGRE_HASH128_FUNC MurmurHash3_x86_128
#else
#    define OGRE_HASH128_FUNC MurmurHash3_x64_128
#endif

namespace Ogre
{
    const int HlmsBits::HlmsTypeBits = 3;
    const int HlmsBits::RenderableBits = 21;
    const int HlmsBits::PassBits = 8;

    const int HlmsBits::HlmsTypeShift = 32 - HlmsTypeBits;
    const int HlmsBits::RenderableShift = HlmsTypeShift - RenderableBits;
    const int HlmsBits::PassShift = RenderableShift - PassBits;

    const int HlmsBits::RendarebleHlmsTypeMask = ( 1 << ( HlmsTypeBits + RenderableBits ) ) - 1;
    const int HlmsBits::HlmsTypeMask = ( 1 << HlmsTypeBits ) - 1;
    const int HlmsBits::RenderableMask = ( 1 << RenderableBits ) - 1;
    const int HlmsBits::PassMask = ( 1 << PassBits ) - 1;

    // Change per mesh (hash can be cached on the renderable)
    const IdString HlmsBaseProp::Skeleton = IdString( "hlms_skeleton" );
    const IdString HlmsBaseProp::BonesPerVertex = IdString( "hlms_bones_per_vertex" );
    const IdString HlmsBaseProp::Pose = IdString( "hlms_pose" );
    const IdString HlmsBaseProp::PoseHalfPrecision = IdString( "hlms_pose_half" );
    const IdString HlmsBaseProp::PoseNormals = IdString( "hlms_pose_normals" );

    const IdString HlmsBaseProp::Normal = IdString( "hlms_normal" );
    const IdString HlmsBaseProp::QTangent = IdString( "hlms_qtangent" );
    const IdString HlmsBaseProp::Tangent = IdString( "hlms_tangent" );
    const IdString HlmsBaseProp::Tangent4 = IdString( "hlms_tangent4" );

    const IdString HlmsBaseProp::Colour = IdString( "hlms_colour" );

    const IdString HlmsBaseProp::IdentityWorld = IdString( "hlms_identity_world" );
    const IdString HlmsBaseProp::IdentityViewProj = IdString( "hlms_identity_viewproj" );
    const IdString HlmsBaseProp::IdentityViewProjDynamic = IdString( "hlms_identity_viewproj_dynamic" );

    const IdString HlmsBaseProp::UvCount = IdString( "hlms_uv_count" );
    const IdString HlmsBaseProp::UvCount0 = IdString( "hlms_uv_count0" );
    const IdString HlmsBaseProp::UvCount1 = IdString( "hlms_uv_count1" );
    const IdString HlmsBaseProp::UvCount2 = IdString( "hlms_uv_count2" );
    const IdString HlmsBaseProp::UvCount3 = IdString( "hlms_uv_count3" );
    const IdString HlmsBaseProp::UvCount4 = IdString( "hlms_uv_count4" );
    const IdString HlmsBaseProp::UvCount5 = IdString( "hlms_uv_count5" );
    const IdString HlmsBaseProp::UvCount6 = IdString( "hlms_uv_count6" );
    const IdString HlmsBaseProp::UvCount7 = IdString( "hlms_uv_count7" );

    // Change per frame (grouped together with scene pass)
    const IdString HlmsBaseProp::LightsDirectional = IdString( "hlms_lights_directional" );
    const IdString HlmsBaseProp::LightsDirNonCaster = IdString( "hlms_lights_directional_non_caster" );
    const IdString HlmsBaseProp::LightsPoint = IdString( "hlms_lights_point" );
    const IdString HlmsBaseProp::LightsSpot = IdString( "hlms_lights_spot" );
    const IdString HlmsBaseProp::LightsAreaApprox = IdString( "hlms_lights_area_approx" );
    const IdString HlmsBaseProp::LightsAreaLtc = IdString( "hlms_lights_area_ltc" );
    const IdString HlmsBaseProp::LightsAreaTexMask = IdString( "hlms_lights_area_tex_mask" );
    const IdString HlmsBaseProp::LightsAreaTexColour = IdString( "hlms_lights_area_tex_colour" );
    const IdString HlmsBaseProp::AllPointLights = IdString( "hlms_all_point_lights" );

    // Change per scene pass
    const IdString HlmsBaseProp::PsoClipDistances = IdString( "hlms_pso_clip_distances" );
    const IdString HlmsBaseProp::GlobalClipPlanes = IdString( "hlms_global_clip_planes" );
    const IdString HlmsBaseProp::EmulateClipDistances = IdString( "hlms_emulate_clip_distances" );
    const IdString HlmsBaseProp::DualParaboloidMapping = IdString( "hlms_dual_paraboloid_mapping" );
    const IdString HlmsBaseProp::InstancedStereo = IdString( "hlms_instanced_stereo" );
    const IdString HlmsBaseProp::ViewMatrix = IdString( "hlms_view_matrix" );
    const IdString HlmsBaseProp::StaticBranchLights = IdString( "hlms_static_branch_lights" );
    const IdString HlmsBaseProp::StaticBranchShadowMapLights =
        IdString( "hlms_static_branch_shadow_map_lights" );
    const IdString HlmsBaseProp::NumShadowMapLights = IdString( "hlms_num_shadow_map_lights" );
    const IdString HlmsBaseProp::NumShadowMapTextures = IdString( "hlms_num_shadow_map_textures" );
    const IdString HlmsBaseProp::PssmSplits = IdString( "hlms_pssm_splits" );
    const IdString HlmsBaseProp::PssmBlend = IdString( "hlms_pssm_blend" );
    const IdString HlmsBaseProp::PssmFade = IdString( "hlms_pssm_fade" );
    const IdString HlmsBaseProp::ShadowCaster = IdString( "hlms_shadowcaster" );
    const IdString HlmsBaseProp::ShadowCasterDirectional = IdString( "hlms_shadowcaster_directional" );
    const IdString HlmsBaseProp::ShadowCasterPoint = IdString( "hlms_shadowcaster_point" );
    const IdString HlmsBaseProp::ShadowUsesDepthTexture = IdString( "hlms_shadow_uses_depth_texture" );
    const IdString HlmsBaseProp::RenderDepthOnly = IdString( "hlms_render_depth_only" );
    const IdString HlmsBaseProp::FineLightMask = IdString( "hlms_fine_light_mask" );
    const IdString HlmsBaseProp::UseUvBaking = IdString( "hlms_use_uv_baking" );
    const IdString HlmsBaseProp::UvBaking = IdString( "hlms_uv_baking" );
    const IdString HlmsBaseProp::BakeLightingOnly = IdString( "hlms_bake_lighting_only" );
    const IdString HlmsBaseProp::MsaaSamples = IdString( "hlms_msaa_samples" );
    const IdString HlmsBaseProp::GenNormalsGBuf = IdString( "hlms_gen_normals_gbuffer" );
    const IdString HlmsBaseProp::PrePass = IdString( "hlms_prepass" );
    const IdString HlmsBaseProp::UsePrePass = IdString( "hlms_use_prepass" );
    const IdString HlmsBaseProp::UsePrePassMsaa = IdString( "hlms_use_prepass_msaa" );
    const IdString HlmsBaseProp::UseSsr = IdString( "hlms_use_ssr" );
    const IdString HlmsBaseProp::SsRefractionsAvailable = IdString( "hlms_ss_refractions_available" );
    const IdString HlmsBaseProp::EnableVpls = IdString( "hlms_enable_vpls" );
    const IdString HlmsBaseProp::Fog = IdString( "hlms_fog" );
    const IdString HlmsBaseProp::ForwardPlus = IdString( "hlms_forwardplus" );
    const IdString HlmsBaseProp::ForwardPlusFlipY = IdString( "hlms_forwardplus_flipY" );
    const IdString HlmsBaseProp::ForwardPlusDebug = IdString( "hlms_forwardplus_debug" );
    const IdString HlmsBaseProp::ForwardPlusFadeAttenRange =
        IdString( "hlms_forward_fade_attenuation_range" );
    const IdString HlmsBaseProp::ForwardPlusFineLightMask =
        IdString( "hlms_forwardplus_fine_light_mask" );
    const IdString HlmsBaseProp::ForwardPlusCoversEntireTarget =
        IdString( "hlms_forwardplus_covers_entire_target" );
    const IdString HlmsBaseProp::Forward3DNumSlices = IdString( "forward3d_num_slices" );
    const IdString HlmsBaseProp::FwdClusteredWidthxHeight = IdString( "fwd_clustered_width_x_height" );
    const IdString HlmsBaseProp::FwdClusteredWidth = IdString( "fwd_clustered_width" );
    const IdString HlmsBaseProp::FwdClusteredLightsPerCell = IdString( "fwd_clustered_lights_per_cell" );
    const IdString HlmsBaseProp::EnableDecals = IdString( "hlms_enable_decals" );
    const IdString HlmsBaseProp::FwdPlusDecalsSlotOffset =
        IdString( "hlms_forwardplus_decals_slot_offset" );
    const IdString HlmsBaseProp::DecalsDiffuse = IdString( "hlms_decals_diffuse" );
    const IdString HlmsBaseProp::DecalsNormals = IdString( "hlms_decals_normals" );
    const IdString HlmsBaseProp::DecalsEmissive = IdString( "hlms_decals_emissive" );
    const IdString HlmsBaseProp::FwdPlusCubemapSlotOffset =
        IdString( "hlms_forwardplus_cubemap_slot_offset" );
    const IdString HlmsBaseProp::BlueNoise = IdString( "hlms_blue_noise" );
    const IdString HlmsBaseProp::ParticleSystem = IdString( "hlms_particle_system" );
    const IdString HlmsBaseProp::ParticleType = IdString( "hlms_particle_type" );
    const IdString HlmsBaseProp::ParticleTypePoint = IdString( "particle_type_point" );
    const IdString HlmsBaseProp::ParticleTypeOrientedCommon =
        IdString( "particle_type_oriented_common" );
    const IdString HlmsBaseProp::ParticleTypeOrientedSelf = IdString( "particle_type_oriented_self" );
    const IdString HlmsBaseProp::ParticleTypePerpendicularCommon =
        IdString( "particle_type_perpendicular_common" );
    const IdString HlmsBaseProp::ParticleTypePerpendicularSelf =
        IdString( "particle_type_perpendicular_self" );
    const IdString HlmsBaseProp::ParticleRotation = IdString( "hlms_particle_rotation" );
    const IdString HlmsBaseProp::Forward3D = IdString( "forward3d" );
    const IdString HlmsBaseProp::ForwardClustered = IdString( "forward_clustered" );
    const IdString HlmsBaseProp::VPos = IdString( "hlms_vpos" );
    const IdString HlmsBaseProp::ScreenPosInt = IdString( "hlms_screen_pos_int" );
    const IdString HlmsBaseProp::ScreenPosUv = IdString( "hlms_screen_pos_uv" );
    const IdString HlmsBaseProp::VertexId = IdString( "hlms_vertex_id" );

    // Change per material (hash can be cached on the renderable)
    const IdString HlmsBaseProp::AlphaTest = IdString( "alpha_test" );
    const IdString HlmsBaseProp::AlphaTestShadowCasterOnly = IdString( "alpha_test_shadow_caster_only" );
    const IdString HlmsBaseProp::AlphaBlend = IdString( "hlms_alphablend" );
    const IdString HlmsBaseProp::AlphaToCoverage = IdString( "hlms_alpha_to_coverage" );
    const IdString HlmsBaseProp::AlphaHash = IdString( "hlms_alpha_hash" );
    const IdString HlmsBaseProp::AccurateNonUniformNormalScaling =
        IdString( "hlms_accurate_non_uniform_normal_scaling" );
    const IdString HlmsBaseProp::ScreenSpaceRefractions = IdString( "hlms_screen_space_refractions" );
    // We use a different convention because it's a really private property that ideally
    // shouldn't be exposed to users.
    const IdString HlmsBaseProp::_DatablockCustomPieceShaderName[NumShaderTypes] = {
        IdString( "_DatablockCustomPieceShaderNameVS" ), IdString( "_DatablockCustomPieceShaderNamePS" ),
        IdString( "_DatablockCustomPieceShaderNameGS" ), IdString( "_DatablockCustomPieceShaderNameHS" ),
        IdString( "_DatablockCustomPieceShaderNameDS" )
    };

    const IdString HlmsBaseProp::NoReverseDepth = IdString( "hlms_no_reverse_depth" );
    const IdString HlmsBaseProp::ReadOnlyIsTex = IdString( "hlms_readonly_is_tex" );

    // clang-format off
    const IdString HlmsBaseProp::Syntax         = IdString( "syntax" );
    const IdString HlmsBaseProp::Hlsl           = IdString( "hlsl" );
    const IdString HlmsBaseProp::Glsl           = IdString( "glsl" );
    const IdString HlmsBaseProp::Glslvk         = IdString( "glslvk" );
    const IdString HlmsBaseProp::Hlslvk         = IdString( "hlslvk" );
    const IdString HlmsBaseProp::Metal          = IdString( "metal" );
    const IdString HlmsBaseProp::GL3Plus        = IdString( "GL3+" );
    const IdString HlmsBaseProp::iOS            = IdString( "iOS" );
    const IdString HlmsBaseProp::macOS          = IdString( "macOS" );
    const IdString HlmsBaseProp::PrecisionMode  = IdString( "precision_mode" );
    const IdString HlmsBaseProp::Full32         = IdString( "full32" );
    const IdString HlmsBaseProp::Midf16         = IdString( "midf16" );
    const IdString HlmsBaseProp::Relaxed        = IdString( "relaxed" );
    const IdString HlmsBaseProp::FastShaderBuildHack= IdString( "fast_shader_build_hack" );
    const IdString HlmsBaseProp::TexGather      = IdString( "hlms_tex_gather" );
    const IdString HlmsBaseProp::DisableStage   = IdString( "hlms_disable_stage" );
    // clang-format on

    const IdString HlmsBasePieces::AlphaTestCmpFunc = IdString( "alpha_test_cmp_func" );

    // GL extensions
    const IdString HlmsBaseProp::GlAmdTrinaryMinMax = IdString( "hlms_amd_trinary_minmax" );

    const IdString *HlmsBaseProp::UvCountPtrs[8] = {  //
        &HlmsBaseProp::UvCount0,                      //
        &HlmsBaseProp::UvCount1,                      //
        &HlmsBaseProp::UvCount2,                      //
        &HlmsBaseProp::UvCount3,                      //
        &HlmsBaseProp::UvCount4,                      //
        &HlmsBaseProp::UvCount5,                      //
        &HlmsBaseProp::UvCount6,                      //
        &HlmsBaseProp::UvCount7
    };

    const IdString HlmsPsoProp::Macroblock = IdString( "PsoMacroblock" );
    const IdString HlmsPsoProp::Blendblock = IdString( "PsoBlendblock" );
    const IdString HlmsPsoProp::InputLayoutId = IdString( "InputLayoutId" );
    const IdString HlmsPsoProp::StrongMacroblockBits = IdString( "StrongMacroblockBits" );
    const IdString HlmsPsoProp::StrongBlendblockBits = IdString( "StrongBlendblockBits" );

    const String ShaderFiles[] = { "VertexShader_vs", "PixelShader_ps", "GeometryShader_gs",
                                   "HullShader_hs", "DomainShader_ds" };
    const String PieceFilePatterns[] = { "piece_vs", "piece_ps", "piece_gs", "piece_hs", "piece_ds" };

    // Must be sorted from best to worst
    const String BestD3DShaderTargets[NumShaderTypes][5] = {
        { "vs_5_0", "vs_4_1", "vs_4_0", "vs_4_0_level_9_3", "vs_4_0_level_9_1" },
        { "ps_5_0", "ps_4_1", "ps_4_0", "ps_4_0_level_9_3", "ps_4_0_level_9_1" },
        { "gs_5_0", "gs_4_1", "gs_4_0", "placeholder", "placeholder" },
        { "hs_5_0", "hs_4_1", "hs_4_0", "placeholder", "placeholder" },
        { "ds_5_0", "ds_4_1", "ds_4_0", "placeholder", "placeholder" },
    };

    static HlmsListener c_defaultListener;

#ifdef OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API
#    ifdef OGRE_SHADER_THREADING_USE_TLS
    thread_local uint32 Hlms::msThreadId = 0u;
#    endif
#endif

    LightweightMutex Hlms::msGlobalMutex;
    bool Hlms::msHasParticleFX2Plugin = false;

    Hlms::Hlms( HlmsTypes type, const String &typeName, Archive *dataFolder,
                ArchiveVec *libraryFolders ) :
        mDataFolder( dataFolder ),
        mHlmsManager( 0 ),
        mShadersGenerated( 0u ),
        mLightGatheringMode( LightGatherForward ),
        mStaticBranchingLights( false ),
        mShaderCodeCacheDirty( false ),
        mParticleSystemConstSlot( 0u ),
        mParticleSystemSlot( 0u ),
        mNumLightsLimit( 0u ),
        mNumAreaApproxLightsLimit( 1u ),
        mNumAreaLtcLightsLimit( 1u ),
        mAreaLightsGlobalLightListStart( 0u ),
        mRealNumAreaApproxLightsWithMask( 0u ),
        mRealNumAreaApproxLights( 0u ),
        mRealNumAreaLtcLights( 0u ),
        mListener( &c_defaultListener ),
        mRenderSystem( 0 ),
        mShaderProfile( "unset!" ),
        mShaderSyntax( "unset!" ),
        mShaderFileExt( "unset!" ),
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        mDebugOutput( true ),
#else
        mDebugOutput( false ),
#endif
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        mDebugOutputProperties( false ),
#else
        mDebugOutputProperties( false ),
#endif
        mPrecisionMode( PrecisionFull32 ),
        mFastShaderBuildHack( false ),
        mDefaultDatablock( 0 ),
        mType( type ),
        mTypeName( typeName ),
        mTypeNameStr( typeName )
    {
        memset( mShaderTargets, 0, sizeof( mShaderTargets ) );

        _setNumThreads( 1u );

        if( libraryFolders )
        {
            ArchiveVec::const_iterator itor = libraryFolders->begin();
            ArchiveVec::const_iterator endt = libraryFolders->end();

            while( itor != endt )
            {
                Library library;
                library.dataFolder = *itor;
                mLibrary.push_back( library );
                ++itor;
            }
        }

        enumeratePieceFiles();

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        mOutputPath = macCachePath() + '/';
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        mOutputPath = fileSystemPathToString(
            Windows::Storage::ApplicationData::Current->TemporaryFolder->Path->Data() );
#endif
    }
    //-----------------------------------------------------------------------------------
    Hlms::~Hlms()
    {
        clearShaderCache();

        _destroyAllDatablocks();

        if( mHlmsManager && mType < HLMS_MAX )
        {
            mHlmsManager->unregisterHlms( mType );
            mHlmsManager = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    static void hashFileConcatenate( DataStreamPtr &inFile, FastArray<uint8> &fileContents )
    {
        const size_t fileSize = inFile->size();
        fileContents.resize( fileSize + sizeof( uint64 ) * 2u );

        uint64 hashResult[2];
        memset( hashResult, 0, sizeof( hashResult ) );
        inFile->read( fileContents.begin() + sizeof( uint64 ) * 2u, fileSize );
        OGRE_HASH128_FUNC( fileContents.begin(), static_cast<int>( fileContents.size() ), IdString::Seed,
                           hashResult );
        memcpy( fileContents.begin(), hashResult, sizeof( uint64 ) * 2u );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::hashPieceFiles( Archive *archive, const StringVector &pieceFiles,
                               FastArray<uint8> &fileContents ) const
    {
        StringVector::const_iterator itor = pieceFiles.begin();
        StringVector::const_iterator endt = pieceFiles.end();

        while( itor != endt )
        {
            // Only open piece files with current render system extension
            const String::size_type extPos0 = itor->find( mShaderFileExt );
            const String::size_type extPos1 = itor->find( ".any" );
            if( extPos0 == itor->size() - mShaderFileExt.size() || extPos1 == itor->size() - 4u )
            {
                DataStreamPtr inFile = archive->open( *itor );
                hashFileConcatenate( inFile, fileContents );
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::DatablockCustomPieceFile::getCodeChecksum( uint64 outHash[2] ) const
    {
        memset( outHash, 0, sizeof( uint64 ) * 2u );
        OGRE_HASH128_FUNC( sourceCode.data(), static_cast<int>( sourceCode.size() ), IdString::Seed,
                           outHash );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::getTemplateChecksum( uint64 outHash[2] ) const
    {
        FastArray<uint8> fileContents;
        fileContents.resize( sizeof( uint64 ) * 2u, 0 );

        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            const String filename = ShaderFiles[i] + mShaderFileExt;
            if( mDataFolder->exists( filename ) )
            {
                // Library piece files first
                LibraryVec::const_iterator itor = mLibrary.begin();
                LibraryVec::const_iterator endt = mLibrary.end();

                while( itor != endt )
                {
                    hashPieceFiles( itor->dataFolder, itor->pieceFiles[i], fileContents );
                    ++itor;
                }

                // Main piece files
                hashPieceFiles( mDataFolder, mPieceFiles[i], fileContents );

                // The shader file
                DataStreamPtr inFile = mDataFolder->open( filename );
                hashFileConcatenate( inFile, fileContents );
            }
        }

        memcpy( outHash, fileContents.begin(), sizeof( uint64 ) * 2u );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::enumeratePieceFiles()
    {
        if( !mDataFolder )
            return;  // Some Hlms implementations may not use template files at all

        bool hasValidFile = false;

        // Check this folder can at least generate one valid type of shader.
        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            // Probe both types since this may be called before we know what RS to use.
            const String filename = ShaderFiles[i];
            hasValidFile |= mDataFolder->exists( filename + ".glsl" );
            hasValidFile |= mDataFolder->exists( filename + ".hlsl" );
            hasValidFile |= mDataFolder->exists( filename + ".metal" );
            hasValidFile |= mDataFolder->exists( filename + ".any" );
        }

        if( !hasValidFile )
        {
            OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                         "Data folder provided contains no valid template shader files. "
                         "Did you provide the right folder location? Check you have the "
                         "right read pemissions. Folder: " +
                             mDataFolder->getName(),
                         "Hlms::Hlms" );
        }

        enumeratePieceFiles( mDataFolder, mPieceFiles );

        LibraryVec::iterator itor = mLibrary.begin();
        LibraryVec::iterator endt = mLibrary.end();

        while( itor != endt )
        {
            bool foundPieceFiles = enumeratePieceFiles( itor->dataFolder, itor->pieceFiles );

            if( !foundPieceFiles )
            {
                LogManager::getSingleton().logMessage( "HLMS Library path '" +
                                                       itor->dataFolder->getName() +
                                                       "' has no piece files. Are you sure you provided "
                                                       "the right path with read access?" );
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::enumeratePieceFiles( Archive *dataFolder, StringVector *pieceFiles )
    {
        bool retVal = false;
        StringVectorPtr stringVectorPtr = dataFolder->list( false, false );

        StringVector stringVectorLowerCase( *stringVectorPtr );

        {
            StringVector::iterator itor = stringVectorLowerCase.begin();
            StringVector::iterator endt = stringVectorLowerCase.end();
            while( itor != endt )
            {
                std::transform( itor->begin(), itor->end(), itor->begin(), ::tolower );
                ++itor;
            }
        }

        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            StringVector::const_iterator itLowerCase = stringVectorLowerCase.begin();
            StringVector::const_iterator itor = stringVectorPtr->begin();
            StringVector::const_iterator endt = stringVectorPtr->end();

            while( itor != endt )
            {
                if( itLowerCase->find( PieceFilePatterns[i] ) != String::npos ||
                    itLowerCase->find( "piece_all" ) != String::npos )
                {
                    retVal = true;
                    pieceFiles[i].push_back( *itor );
                }

                ++itLowerCase;
                ++itor;
            }

            // Enumeration order depends on OS and filesystem. Ensure deterministic alphabetical order.
            std::sort( pieceFiles[i].begin(), pieceFiles[i].end() );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setProperty( size_t tid, IdString key, int32 value )
    {
        HlmsProperty p( key, value );
        HlmsPropertyVec::iterator it = std::lower_bound(
            mT[tid].setProperties.begin(), mT[tid].setProperties.end(), p, OrderPropertyByIdString );
        if( it == mT[tid].setProperties.end() || it->keyName != p.keyName )
            mT[tid].setProperties.insert( it, p );
        else
            *it = p;
    }
    //-----------------------------------------------------------------------------------
    int32 Hlms::getProperty( size_t tid, IdString key, int32 defaultVal ) const
    {
        HlmsProperty p( key, 0 );
        HlmsPropertyVec::const_iterator it = std::lower_bound(
            mT[tid].setProperties.begin(), mT[tid].setProperties.end(), p, OrderPropertyByIdString );
        if( it != mT[tid].setProperties.end() && it->keyName == p.keyName )
            defaultVal = it->value;

        return defaultVal;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::unsetProperty( size_t tid, IdString key )
    {
        HlmsProperty p( key, 0 );
        HlmsPropertyVec::iterator it = std::lower_bound(
            mT[tid].setProperties.begin(), mT[tid].setProperties.end(), p, OrderPropertyByIdString );
        if( it != mT[tid].setProperties.end() && it->keyName == p.keyName )
            mT[tid].setProperties.erase( it );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setProperty( HlmsPropertyVec &properties, IdString key, int32 value )
    {
        HlmsProperty p( key, value );
        HlmsPropertyVec::iterator it =
            std::lower_bound( properties.begin(), properties.end(), p, OrderPropertyByIdString );
        if( it == properties.end() || it->keyName != p.keyName )
            properties.insert( it, p );
        else
            *it = p;
    }
    //-----------------------------------------------------------------------------------
    int32 Hlms::getProperty( const HlmsPropertyVec &properties, IdString key, int32 defaultVal )
    {
        HlmsProperty p( key, 0 );
        HlmsPropertyVec::const_iterator it =
            std::lower_bound( properties.begin(), properties.end(), p, OrderPropertyByIdString );
        if( it != properties.end() && it->keyName == p.keyName )
            defaultVal = it->value;

        return defaultVal;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::findBlockEnd( SubStringRef &outSubString, bool &syntaxError, bool allowsElse )
    {
        bool isElse = false;

        const char *blockNames[] = { "foreach", "property", "piece", "else" };

        cbitset32<2048> allowedElses;
        if( allowsElse )
            allowedElses.set( 0 );

        String::const_iterator it = outSubString.begin();
        String::const_iterator en = outSubString.end();

        int nesting = 0;

        while( it != en && nesting >= 0 )
        {
            if( *it == '@' )
            {
                SubStringRef subString( &outSubString.getOriginalBuffer(), it + 1 );

                bool start = subString.startWith( "end" );
                if( start )
                {
                    --nesting;
                    it += sizeof( "end" ) - 1;
                    continue;
                }
                else
                {
                    if( allowsElse )
                        start = subString.startWith( "else" );
                    if( start )
                    {
                        if( !allowedElses.test( static_cast<size_t>( nesting ) ) )
                        {
                            syntaxError = true;
                            printf( "Unexpected @else while looking for @end\nNear: '%s'\n",
                                    &( *subString.begin() ) );
                        }
                        if( nesting == 0 )
                        {
                            // Decrement nesting so that we're out and tell caller we went from
                            //@property() through @else. Caller will later have to go from
                            //@else to @end
                            isElse = true;
                            --nesting;
                        }
                        else
                        {
                            // Do not decrease 'nesting', as we now need to look for "@end" but
                            // unset allowedElses, so that we do not allow two consecutive @else
                            allowedElses.setValue( static_cast<size_t>( nesting ), 0u );
                        }
                        it += sizeof( "else" ) - 1;
                        continue;
                    }
                    else
                    {
                        for( size_t i = 0; i < sizeof( blockNames ) / sizeof( char * ); ++i )
                        {
                            bool startBlock = subString.startWith( blockNames[i] );
                            if( startBlock )
                            {
                                it = subString.begin() +
                                     static_cast<ptrdiff_t>( strlen( blockNames[i] ) );
                                if( i == 3 )
                                {
                                    // Do not increase 'nesting' for "@else"
                                    if( !allowedElses.test( static_cast<size_t>( nesting ) ) )
                                    {
                                        syntaxError = true;
                                        printf( "Unexpected @else while looking for @end\nNear: '%s'\n",
                                                &( *subString.begin() ) );
                                    }
                                }
                                else
                                {
                                    ++nesting;
                                }
                                allowedElses.setValue( static_cast<size_t>( nesting ), i == 1u );
                                break;
                            }
                        }
                    }
                }
            }

            ++it;
        }

        assert( nesting >= -1 );

        if( it != en && nesting < 0 )
        {
            int keywordLength = ( isElse ? sizeof( "else" ) : sizeof( "end" ) ) - 1;
            outSubString.setEnd(
                static_cast<size_t>( it - outSubString.getOriginalBuffer().begin() - keywordLength ) );
        }
        else
        {
            syntaxError = true;

            char tmpData[64];
            memset( tmpData, 0, sizeof( tmpData ) );
            strncpy( tmpData, &( *outSubString.begin() ),
                     std::min<size_t>( 63u, outSubString.getSize() ) );

            printf(
                "Syntax Error at line %lu: start block (e.g. @foreach; @property) "
                "without matching @end\nNear: '%s'\n",
                calculateLineCount( outSubString ), tmpData );
        }

        return isElse;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::evaluateExpression( SubStringRef &outSubString, bool &outSyntaxError,
                                   const size_t tid ) const
    {
        size_t expEnd = evaluateExpressionEnd( outSubString );

        if( expEnd == String::npos )
        {
            outSyntaxError = true;
            return false;
        }

        SubStringRef subString( &outSubString.getOriginalBuffer(), outSubString.getStart(),
                                outSubString.getStart() + expEnd );

        outSubString =
            SubStringRef( &outSubString.getOriginalBuffer(), outSubString.getStart() + expEnd + 1 );

        bool textStarted = false;
        bool syntaxError = false;
        bool nextExpressionNegates = false;

        std::vector<Expression *> expressionParents;
        ExpressionVec outExpressions;
        outExpressions.clear();
        outExpressions.resize( 1 );

        Expression *currentExpression = &outExpressions.back();

        String::const_iterator it = subString.begin();
        String::const_iterator en = subString.end();

        while( it != en && !syntaxError )
        {
            char c = *it;

            if( c == '(' )
            {
                currentExpression->children.push_back( Expression() );
                expressionParents.push_back( currentExpression );

                currentExpression->children.back().negated = nextExpressionNegates;

                textStarted = false;
                nextExpressionNegates = false;

                currentExpression = &currentExpression->children.back();
            }
            else if( c == ')' )
            {
                if( expressionParents.empty() )
                    syntaxError = true;
                else
                {
                    currentExpression = expressionParents.back();
                    expressionParents.pop_back();
                }

                textStarted = false;
            }
            else if( c == ' ' || c == '\t' || c == '\n' || c == '\r' )
            {
                textStarted = false;
            }
            else if( c == '!' &&
                     // Avoid treating "!=" as a negation of variable.
                     ( ( it + 1 ) == en || *( it + 1 ) != '=' ) )
            {
                nextExpressionNegates = true;
            }
            else
            {
                if( !textStarted )
                {
                    textStarted = true;
                    currentExpression->children.push_back( Expression() );
                    currentExpression->children.back().negated = nextExpressionNegates;
                }

                if( c == '&' || c == '|' || c == '=' || c == '<' || c == '>' ||
                    c == '!' /* can only mean "!=" */ )
                {
                    if( currentExpression->children.empty() || nextExpressionNegates )
                    {
                        syntaxError = true;
                    }
                    else if( !currentExpression->children.back().value.empty() &&
                             c != *( currentExpression->children.back().value.end() - 1 ) && c != '=' )
                    {
                        currentExpression->children.push_back( Expression() );
                    }
                }

                currentExpression->children.back().value.push_back( c );
                nextExpressionNegates = false;
            }

            ++it;
        }

        bool retVal = false;

        if( !expressionParents.empty() )
            syntaxError = true;

        if( !syntaxError )
            retVal = evaluateExpressionRecursive( outExpressions, syntaxError, tid ) != 0;

        if( syntaxError )
            printf( "Syntax Error at line %lu\n", calculateLineCount( subString ) );

        outSyntaxError = syntaxError;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    int32 Hlms::evaluateExpressionRecursive( ExpressionVec &expression, bool &outSyntaxError,
                                             const size_t tid ) const
    {
        bool syntaxError = outSyntaxError;
        bool lastExpWasOperator = true;
        ExpressionVec::iterator itor = expression.begin();
        ExpressionVec::iterator endt = expression.end();

        while( itor != endt )
        {
            Expression &exp = *itor;

            if( exp.value == "&&" )
                exp.type = EXPR_OPERATOR_AND;
            else if( exp.value == "||" )
                exp.type = EXPR_OPERATOR_OR;
            else if( exp.value == "<" )
                exp.type = EXPR_OPERATOR_LE;
            else if( exp.value == "<=" )
                exp.type = EXPR_OPERATOR_LEEQ;
            else if( exp.value == "==" )
                exp.type = EXPR_OPERATOR_EQ;
            else if( exp.value == "!=" )
                exp.type = EXPR_OPERATOR_NEQ;
            else if( exp.value == ">" )
                exp.type = EXPR_OPERATOR_GR;
            else if( exp.value == ">=" )
                exp.type = EXPR_OPERATOR_GREQ;
            else if( !exp.children.empty() )
                exp.type = EXPR_OBJECT;
            else
                exp.type = EXPR_VAR;

            if( ( exp.isOperator() && lastExpWasOperator ) ||
                ( !exp.isOperator() && !lastExpWasOperator ) )
            {
                syntaxError = true;
                printf( "Unrecognized token '%s'", exp.value.c_str() );
            }
            else
            {
                lastExpWasOperator = exp.isOperator();
            }

            ++itor;
        }

        // If we don't check 'expression.size() > 3u' here, we can end up in infinite recursion
        // later on (because operators get turned into EXPR_OBJECT and thus the object
        // is evaluated recusrively, and turned again into EXPR_OBJECT)
        if( !syntaxError && expression.size() > 3u )
        {
            // We will now enclose "a < b" into "(a < b)" other wise statements like these:
            // a && b < c will be parsed as (a && b) < c which is completely counterintuitive.

            // We need expression.size() > 3 which is guaranteed because if back nor front
            // are neither operators and we can't have two operators in a row, then they can
            // only be in the middle, or there is no operator at all.
            itor = expression.begin() + 1;
            endt = expression.end();
            while( itor != endt )
            {
                if( itor->type >= EXPR_OPERATOR_LE && itor->type <= EXPR_OPERATOR_GREQ )
                {
                    // We need to merge n-1, n, n+1 into:
                    // (n-1)' = EXPR_OBJECT with 3 children:
                    //      n-1, n, n+1
                    // and then remove both n & n+1.
                    itor->children.resize( 3 );

                    itor->children[1].type = itor->type;
                    itor->children[1].value.swap( itor->value );
                    itor->children[0].swap( *( itor - 1 ) );
                    itor->children[2].swap( *( itor + 1 ) );

                    itor->type = EXPR_OBJECT;

                    ( itor - 1 )->swap( *itor );

                    itor = expression.erase( itor, itor + 2 );
                    endt = expression.end();
                }
                else
                {
                    ++itor;
                }
            }
        }

        // Evaluate the individual properties.
        itor = expression.begin();
        while( itor != endt && !syntaxError )
        {
            Expression &exp = *itor;
            if( exp.type == EXPR_VAR )
            {
                char *endPtr;
                exp.result = static_cast<int32>( strtol( exp.value.c_str(), &endPtr, 10 ) );
                if( exp.value.c_str() == endPtr )
                {
                    // This isn't a number. Let's try if it's a variable
                    exp.result = getProperty( tid, exp.value );
                }
                lastExpWasOperator = false;
            }
            else
            {
                exp.result = evaluateExpressionRecursive( exp.children, syntaxError, tid );
                lastExpWasOperator = false;
            }

            ++itor;
        }

        // Perform operations between the different properties.
        int32 retVal = 1;
        if( !syntaxError )
        {
            itor = expression.begin();

            ExpressionType nextOperation = EXPR_VAR;

            while( itor != endt )
            {
                int32 result = itor->negated ? !itor->result : itor->result;

                switch( nextOperation )
                {
                case EXPR_OPERATOR_OR:
                    retVal = ( retVal != 0 ) | ( result != 0 );
                    break;
                case EXPR_OPERATOR_AND:
                    retVal = ( retVal != 0 ) & ( result != 0 );
                    break;
                case EXPR_OPERATOR_LE:
                    retVal = retVal < result;
                    break;
                case EXPR_OPERATOR_LEEQ:
                    retVal = retVal <= result;
                    break;
                case EXPR_OPERATOR_EQ:
                    retVal = retVal == result;
                    break;
                case EXPR_OPERATOR_NEQ:
                    retVal = retVal != result;
                    break;
                case EXPR_OPERATOR_GR:
                    retVal = retVal > result;
                    break;
                case EXPR_OPERATOR_GREQ:
                    retVal = retVal >= result;
                    break;

                case EXPR_OBJECT:
                case EXPR_VAR:
                    if( !itor->isOperator() )
                        retVal = result;
                    break;
                }

                nextOperation = itor->type;

                ++itor;
            }
        }

        outSyntaxError = syntaxError;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    size_t Hlms::evaluateExpressionEnd( const SubStringRef &outSubString )
    {
        String::const_iterator it = outSubString.begin();
        String::const_iterator en = outSubString.end();

        int nesting = 0;

        while( it != en && nesting >= 0 )
        {
            if( *it == '(' )
                ++nesting;
            else if( *it == ')' )
                --nesting;
            ++it;
        }

        assert( nesting >= -1 );

        size_t retVal = String::npos;
        if( it != en && nesting < 0 )
        {
            retVal = static_cast<size_t>( it - outSubString.begin() - 1u );
        }
        else
        {
            printf( "Syntax Error at line %lu: opening parenthesis without matching closure\n",
                    calculateLineCount( outSubString ) );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::evaluateParamArgs( SubStringRef &outSubString, StringVector &outArgs,
                                  bool &outSyntaxError )
    {
        size_t expEnd = evaluateExpressionEnd( outSubString );

        if( expEnd == String::npos )
        {
            outSyntaxError = true;
            return;
        }

        SubStringRef subString( &outSubString.getOriginalBuffer(), outSubString.getStart(),
                                outSubString.getStart() + expEnd );

        outSubString =
            SubStringRef( &outSubString.getOriginalBuffer(), outSubString.getStart() + expEnd + 1 );

        int expressionState = 0;
        bool syntaxError = false;

        outArgs.clear();
        outArgs.emplace_back();

        String::const_iterator it = subString.begin();
        String::const_iterator en = subString.end();

        while( it != en && !syntaxError )
        {
            char c = *it;

            if( c == '(' || c == ')' || c == '@' || c == '&' || c == '|' )
            {
                syntaxError = true;
            }
            else if( c == ' ' || c == '\t' || c == '\n' || c == '\r' )
            {
                if( expressionState == 1 )
                    expressionState = 2;
            }
            else if( c == ',' )
            {
                expressionState = 0;
                outArgs.emplace_back();
            }
            else
            {
                if( expressionState == 2 )
                {
                    printf( "Syntax Error at line %lu: ',' or ')' expected\n",
                            calculateLineCount( subString ) );
                    syntaxError = true;
                }
                else
                {
                    outArgs.back().push_back( *it );
                    expressionState = 1;
                }
            }

            ++it;
        }

        if( syntaxError )
            printf( "Syntax Error at line %lu\n", calculateLineCount( subString ) );

        outSyntaxError = syntaxError;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::copy( String &outBuffer, const SubStringRef &inSubString, size_t length )
    {
        outBuffer.append( inSubString.begin(), inSubString.begin() + static_cast<ptrdiff_t>( length ) );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::repeat( String &outBuffer, const SubStringRef &inSubString, size_t length, size_t passNum,
                       const String &counterVar )
    {
        String::const_iterator itor = inSubString.begin();
        String::const_iterator endt = inSubString.begin() + static_cast<ptrdiff_t>( length );

        while( itor != endt )
        {
            if( *itor == '@' && !counterVar.empty() )
            {
                SubStringRef subString( &inSubString.getOriginalBuffer(), itor + 1 );
                if( subString.startWith( counterVar ) )
                {
                    char tmp[16];
                    std::snprintf( tmp, sizeof( tmp ), "%lu", (unsigned long)passNum );
                    outBuffer += tmp;
                    itor += static_cast<ptrdiff_t>( counterVar.size() + 1u );
                }
                else
                {
                    outBuffer.push_back( *itor++ );
                }
            }
            else
            {
                outBuffer.push_back( *itor++ );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    int setOp( int /*op1*/, int op2 ) { return op2; }
    int addOp( int op1, int op2 ) { return op1 + op2; }
    int subOp( int op1, int op2 ) { return op1 - op2; }
    int mulOp( int op1, int op2 ) { return op1 * op2; }
    int divOp( int op1, int op2 ) { return op1 / op2; }
    int modOp( int op1, int op2 ) { return op1 % op2; }
    int minOp( int op1, int op2 ) { return std::min( op1, op2 ); }
    int maxOp( int op1, int op2 ) { return std::max( op1, op2 ); }

    struct Operation
    {
        const char *opName;
        size_t length;
        int ( *opFunc )( int, int );
        Operation( const char *_name, size_t len, int ( *_opFunc )( int, int ) ) :
            opName( _name ),
            length( len ),
            opFunc( _opFunc )
        {
        }
    };

    const Operation c_operations[8] = {
        Operation( "pset", sizeof( "@pset" ), &setOp ), Operation( "padd", sizeof( "@padd" ), &addOp ),
        Operation( "psub", sizeof( "@psub" ), &subOp ), Operation( "pmul", sizeof( "@pmul" ), &mulOp ),
        Operation( "pdiv", sizeof( "@pdiv" ), &divOp ), Operation( "pmod", sizeof( "@pmod" ), &modOp ),
        Operation( "pmin", sizeof( "@pmin" ), &minOp ), Operation( "pmax", sizeof( "@pmax" ), &maxOp )
    };
    //-----------------------------------------------------------------------------------
    inline int Hlms::interpretAsNumberThenAsProperty( const String &argValue, const size_t tid ) const
    {
        int opValue = StringConverter::parseInt( argValue, -std::numeric_limits<int>::max() );
        if( opValue == -std::numeric_limits<int>::max() )
        {
            // Not a number, interpret as property
            opValue = getProperty( tid, argValue );
        }

        return opValue;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::parseMath( const String &inBuffer, String &outBuffer, const size_t tid )
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );

        size_t pos;
        pos = subString.find( "@" );
        size_t keyword = std::numeric_limits<size_t>::max();

        while( pos != String::npos && keyword == std::numeric_limits<size_t>::max() )
        {
            size_t maxSize = subString.findFirstOf( " \t(", pos + 1 );
            maxSize = maxSize == String::npos ? subString.getSize() : maxSize;
            SubStringRef keywordStr( &inBuffer, subString.getStart() + pos + 1,
                                     subString.getStart() + maxSize );

            for( size_t i = 0; i < 8 && keyword == std::numeric_limits<size_t>::max(); ++i )
            {
                if( keywordStr.matchEqual( c_operations[i].opName ) )
                    keyword = i;
            }

            if( keyword == std::numeric_limits<size_t>::max() )
                pos = subString.find( "@", pos + 1 );
        }

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + c_operations[keyword].length );
            evaluateParamArgs( subString, argValues, syntaxError );

            syntaxError |= argValues.size() < 2 || argValues.size() > 3;

            if( !syntaxError )
            {
                const IdString dstProperty = argValues[0];
                const size_t idx = argValues.size() == 3 ? 1 : 0;
                const int op1Value = interpretAsNumberThenAsProperty( argValues[idx], tid );
                const int op2Value = interpretAsNumberThenAsProperty( argValues[idx + 1], tid );

                int result = c_operations[keyword].opFunc( op1Value, op2Value );
                setProperty( tid, dstProperty, result );
            }
            else
            {
                unsigned long lineCount = calculateLineCount( subString );
                if( keyword <= 1 )
                {
                    printf( "Syntax Error at line %lu: @%s expects one parameter\n", lineCount,
                            c_operations[keyword].opName );
                }
                else
                {
                    printf( "Syntax Error at line %lu: @%s expects two or three parameters\n", lineCount,
                            c_operations[keyword].opName );
                }
            }

            pos = subString.find( "@" );
            keyword = std::numeric_limits<size_t>::max();

            while( pos != String::npos && keyword == std::numeric_limits<size_t>::max() )
            {
                size_t maxSize = subString.findFirstOf( " \t(", pos + 1 );
                maxSize = maxSize == String::npos ? subString.getSize() : maxSize;
                SubStringRef keywordStr( &inBuffer, subString.getStart() + pos + 1,
                                         subString.getStart() + maxSize );

                for( size_t i = 0; i < 8 && keyword == std::numeric_limits<size_t>::max(); ++i )
                {
                    if( keywordStr.matchEqual( c_operations[i].opName ) )
                        keyword = i;
                }

                if( keyword == std::numeric_limits<size_t>::max() )
                    pos = subString.find( "@", pos + 1 );
            }
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::parseForEach( const String &inBuffer, String &outBuffer, const size_t tid ) const
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );
        size_t pos = subString.find( "@foreach" );

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + sizeof( "@foreach" ) );
            evaluateParamArgs( subString, argValues, syntaxError );

            SubStringRef blockSubString = subString;
            findBlockEnd( blockSubString, syntaxError );

            if( !syntaxError )
            {
                char *endPtr;
                int32 count = static_cast<int32>( strtol( argValues[0].c_str(), &endPtr, 10 ) );
                if( argValues[0].c_str() == endPtr )
                {
                    // This isn't a number. Let's try if it's a variable
                    // count = getProperty( argValues[0], -1 );
                    count = getProperty( tid, argValues[0], 0 );
                }

                /*if( count < 0 )
                {
                    printf( "Invalid parameter at line %lu (@foreach)."
                            " '%s' is not a number nor a variable\n",
                            calculateLineCount( blockSubString ), argValues[0].c_str() );
                    syntaxError = true;
                    count = 0;
                }*/

                String counterVar;
                if( argValues.size() > 1 )
                    counterVar = argValues[1];

                int32 start = 0;
                if( argValues.size() > 2 )
                {
                    start = static_cast<int32>( strtol( argValues[2].c_str(), &endPtr, 10 ) );
                    if( argValues[2].c_str() == endPtr )
                    {
                        // This isn't a number. Let's try if it's a variable
                        start = static_cast<int32>( getProperty( tid, argValues[2], -1 ) );
                    }

                    if( start < 0 )
                    {
                        printf(
                            "Invalid parameter at line %lu (@foreach)."
                            " '%s' is not a number nor a variable\n",
                            calculateLineCount( blockSubString ), argValues[2].c_str() );
                        syntaxError = true;
                        start = 0;
                        count = 0;
                    }
                }

                for( int32 i = start; i < count; ++i )
                {
                    repeat( outBuffer, blockSubString, blockSubString.getSize(),
                            static_cast<size_t>( i ), counterVar );
                }
            }

            subString.setStart( blockSubString.getEnd() + sizeof( "@end" ) );
            pos = subString.find( "@foreach" );
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::parseProperties( String &inBuffer, String &outBuffer, const size_t tid ) const
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        SubStringRef subString( &inBuffer, 0 );
        size_t pos = subString.find( "@property" );

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + sizeof( "@property" ) );
            bool result = evaluateExpression( subString, syntaxError, tid );

            SubStringRef blockSubString = subString;
            bool isElse = findBlockEnd( blockSubString, syntaxError, true );

            if( result && !syntaxError )
                copy( outBuffer, blockSubString, blockSubString.getSize() );

            if( !isElse )
            {
                subString.setStart( blockSubString.getEnd() + sizeof( "@end" ) );
                pos = subString.find( "@property" );
            }
            else
            {
                subString.setStart( blockSubString.getEnd() + sizeof( "@else" ) );
                blockSubString = subString;
                findBlockEnd( blockSubString, syntaxError );
                if( !syntaxError && !result )
                    copy( outBuffer, blockSubString, blockSubString.getSize() );
                subString.setStart( blockSubString.getEnd() + sizeof( "@end" ) );
                pos = subString.find( "@property" );
            }
        }

        copy( outBuffer, subString, subString.getSize() );

        while( !syntaxError && outBuffer.find( "@property" ) != String::npos )
        {
            inBuffer.swap( outBuffer );
            syntaxError = parseProperties( inBuffer, outBuffer, tid );
        }

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::parseUndefPieces( String &inBuffer, String &outBuffer, const size_t tid )
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );
        size_t pos = subString.find( "@undefpiece" );

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + sizeof( "@undefpiece" ) );
            evaluateParamArgs( subString, argValues, syntaxError );

            syntaxError |= argValues.size() != 1u;

            if( !syntaxError )
            {
                const IdString pieceName( argValues[0] );
                PiecesMap::iterator it = mT[tid].pieces.find( pieceName );
                if( it != mT[tid].pieces.end() )
                    mT[tid].pieces.erase( it );
            }
            else
            {
                printf( "Syntax Error at line %lu: @undefpiece expects one parameter",
                        calculateLineCount( subString ) );
            }

            pos = subString.find( "@undefpiece" );
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::collectPieces( const String &inBuffer, String &outBuffer, const size_t tid )
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );
        size_t pos = subString.find( "@piece" );

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + sizeof( "@piece" ) );
            evaluateParamArgs( subString, argValues, syntaxError );

            syntaxError |= argValues.size() != 1;

            if( !syntaxError )
            {
                const IdString pieceName( argValues[0] );
                PiecesMap::const_iterator it = mT[tid].pieces.find( pieceName );
                if( it != mT[tid].pieces.end() )
                {
                    syntaxError = true;
                    printf( "Error at line %lu: @piece '%s' already defined",
                            calculateLineCount( subString ), argValues[0].c_str() );
                }
                else
                {
                    SubStringRef blockSubString = subString;
                    findBlockEnd( blockSubString, syntaxError );

                    String tmpBuffer;
                    copy( tmpBuffer, blockSubString, blockSubString.getSize() );
                    mT[tid].pieces[pieceName] = tmpBuffer;

                    subString.setStart( blockSubString.getEnd() + sizeof( "@end" ) );
                }
            }
            else
            {
                printf( "Syntax Error at line %lu: @piece expects one parameter",
                        calculateLineCount( subString ) );
            }

            pos = subString.find( "@piece" );
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::insertPieces( String &inBuffer, String &outBuffer, const size_t tid ) const
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );
        size_t pos = subString.find( "@insertpiece" );

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + sizeof( "@insertpiece" ) );
            evaluateParamArgs( subString, argValues, syntaxError );

            syntaxError |= argValues.size() != 1;

            if( !syntaxError )
            {
                const IdString pieceName( argValues[0] );
                PiecesMap::const_iterator it = mT[tid].pieces.find( pieceName );
                if( it != mT[tid].pieces.end() )
                    outBuffer += it->second;
            }
            else
            {
                printf( "Syntax Error at line %lu: @insertpiece expects one parameter",
                        calculateLineCount( subString ) );
            }

            pos = subString.find( "@insertpiece" );
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    const Operation c_counterOperations[10] = {
        Operation( "counter", sizeof( "@counter" ), 0 ), Operation( "value", sizeof( "@value" ), 0 ),
        Operation( "set", sizeof( "@set" ), &setOp ),    Operation( "add", sizeof( "@add" ), &addOp ),
        Operation( "sub", sizeof( "@sub" ), &subOp ),    Operation( "mul", sizeof( "@mul" ), &mulOp ),
        Operation( "div", sizeof( "@div" ), &divOp ),    Operation( "mod", sizeof( "@mod" ), &modOp ),
        Operation( "min", sizeof( "@min" ), &minOp ),    Operation( "max", sizeof( "@max" ), &maxOp )
    };
    //-----------------------------------------------------------------------------------
    bool Hlms::parseCounter( const String &inBuffer, String &outBuffer, const size_t tid )
    {
        outBuffer.clear();
        outBuffer.reserve( inBuffer.size() );

        StringVector argValues;
        SubStringRef subString( &inBuffer, 0 );

        size_t pos;
        pos = subString.find( "@" );
        size_t keyword = std::numeric_limits<size_t>::max();

        if( pos != String::npos )
        {
            size_t maxSize = subString.findFirstOf( " \t(", pos + 1 );
            maxSize = maxSize == String::npos ? subString.getSize() : maxSize;
            SubStringRef keywordStr( &inBuffer, subString.getStart() + pos + 1,
                                     subString.getStart() + maxSize );

            for( size_t i = 0; i < 10 && keyword == std::numeric_limits<size_t>::max(); ++i )
            {
                if( keywordStr.matchEqual( c_counterOperations[i].opName ) )
                    keyword = i;
            }

            if( keyword == std::numeric_limits<size_t>::max() )
                pos = String::npos;
        }

        bool syntaxError = false;

        while( pos != String::npos && !syntaxError )
        {
            // Copy what comes before the block
            copy( outBuffer, subString, pos );

            subString.setStart( subString.getStart() + pos + c_counterOperations[keyword].length );
            evaluateParamArgs( subString, argValues, syntaxError );

            if( keyword <= 1 )
                syntaxError |= argValues.size() != 1;
            else
                syntaxError |= argValues.size() < 2 || argValues.size() > 3;

            if( !syntaxError )
            {
                if( argValues.size() == 1 )
                {
                    const IdString dstProperty = argValues[0];
                    const IdString srcProperty = dstProperty;
                    int op1Value = getProperty( tid, srcProperty );

                    //@value & @counter write, the others are invisible
                    char tmp[16];
                    std::snprintf( tmp, sizeof( tmp ), "%i", op1Value );
                    outBuffer += tmp;

                    if( keyword == 0 )
                    {
                        ++op1Value;
                        setProperty( tid, dstProperty, op1Value );
                    }
                }
                else
                {
                    const IdString dstProperty = argValues[0];
                    const size_t idx = argValues.size() == 3 ? 1 : 0;
                    const int op1Value = interpretAsNumberThenAsProperty( argValues[idx], tid );
                    const int op2Value = interpretAsNumberThenAsProperty( argValues[idx + 1], tid );

                    int result = c_counterOperations[keyword].opFunc( op1Value, op2Value );
                    setProperty( tid, dstProperty, result );
                }
            }
            else
            {
                unsigned long lineCount = calculateLineCount( subString );
                if( keyword <= 1 )
                {
                    printf( "Syntax Error at line %lu: @%s expects one parameter\n", lineCount,
                            c_counterOperations[keyword].opName );
                }
                else
                {
                    printf( "Syntax Error at line %lu: @%s expects two or three parameters\n", lineCount,
                            c_counterOperations[keyword].opName );
                }
            }

            pos = subString.find( "@" );
            keyword = std::numeric_limits<size_t>::max();

            if( pos != String::npos )
            {
                size_t maxSize = subString.findFirstOf( " \t(", pos + 1 );
                maxSize = maxSize == String::npos ? subString.getSize() : maxSize;
                SubStringRef keywordStr( &inBuffer, subString.getStart() + pos + 1,
                                         subString.getStart() + maxSize );

                for( size_t i = 0; i < 10 && keyword == std::numeric_limits<size_t>::max(); ++i )
                {
                    if( keywordStr.matchEqual( c_counterOperations[i].opName ) )
                        keyword = i;
                }

                if( keyword == std::numeric_limits<size_t>::max() )
                    pos = String::npos;
            }
        }

        copy( outBuffer, subString, subString.getSize() );

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::parseOffline( const String &filename, String &inString, String &outString,
                             const size_t tid )
    {
        mT[tid].setProperties.clear();
        mT[tid].pieces.clear();

        if( mShaderProfile == "glsl" || mShaderProfile == "glslvk" )  // TODO: String comparision
        {
            setProperty( tid, HlmsBaseProp::GL3Plus, mRenderSystem->getNativeShadingLanguageVersion() );
        }

        setProperty( tid, HlmsBaseProp::Syntax, static_cast<int32>( mShaderSyntax.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Hlsl, static_cast<int32>( HlmsBaseProp::Hlsl.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Glsl, static_cast<int32>( HlmsBaseProp::Glsl.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Glslvk,
                     static_cast<int32>( HlmsBaseProp::Glslvk.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Hlslvk,
                     static_cast<int32>( HlmsBaseProp::Hlslvk.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Metal, static_cast<int32>( HlmsBaseProp::Metal.getU32Value() ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        setProperty( tid, HlmsBaseProp::iOS, 1 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        setProperty( tid, HlmsBaseProp::macOS, 1 );
#endif
        setProperty( tid, HlmsBaseProp::Full32,
                     static_cast<int32>( HlmsBaseProp::Full32.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Midf16,
                     static_cast<int32>( HlmsBaseProp::Midf16.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::Relaxed,
                     static_cast<int32>( HlmsBaseProp::Relaxed.getU32Value() ) );
        setProperty( tid, HlmsBaseProp::PrecisionMode, getSupportedPrecisionModeHash() );

        if( mFastShaderBuildHack )
            setProperty( tid, HlmsBaseProp::FastShaderBuildHack, 1 );

        bool syntaxError = false;

        syntaxError |= this->parseMath( inString, outString, tid );
        while( !syntaxError && outString.find( "@foreach" ) != String::npos )
        {
            syntaxError |= this->parseForEach( outString, inString, tid );
            inString.swap( outString );
        }
        syntaxError |= this->parseProperties( outString, inString, tid );
        syntaxError |= this->parseUndefPieces( inString, outString, tid );
        while( !syntaxError && ( outString.find( "@piece" ) != String::npos ||
                                 outString.find( "@insertpiece" ) != String::npos ) )
        {
            syntaxError |= this->collectPieces( outString, inString, tid );
            syntaxError |= this->insertPieces( inString, outString, tid );
        }
        syntaxError |= this->parseCounter( outString, inString, tid );

        outString.swap( inString );

        if( syntaxError )
        {
            LogManager::getSingleton().logMessage( "There were HLMS syntax errors while parsing " +
                                                   filename );
        }

        return syntaxError;
    }
    //-----------------------------------------------------------------------------------
    uint32 Hlms::addRenderableCache( const HlmsPropertyVec &renderableSetProperties,
                                     const PiecesMap *pieces )
    {
        assert( mRenderableCache.size() <= HlmsBits::RenderableMask );

        RenderableCache cacheEntry( renderableSetProperties, pieces );

        RenderableCacheVec::iterator it =
            std::find( mRenderableCache.begin(), mRenderableCache.end(), cacheEntry );
        if( it == mRenderableCache.end() )
        {
            mRenderableCache.push_back( cacheEntry );
            it = mRenderableCache.end() - 1;
        }

        // 3 bits for mType (see getMaterial)
        return ( static_cast<uint32>( mType ) << HlmsBits::HlmsTypeShift ) |
               ( static_cast<uint32>( it - mRenderableCache.begin() ) << HlmsBits::RenderableShift );
    }
    //-----------------------------------------------------------------------------------
    const Hlms::RenderableCache &Hlms::getRenderableCache( uint32 hash ) const
    {
        return mRenderableCache[( hash >> HlmsBits::RenderableShift ) & HlmsBits::RenderableMask];
    }
    //-----------------------------------------------------------------------------------
    HlmsDatablock *Hlms::createDefaultDatablock()
    {
        return createDatablock( IdString(), "[Default]", HlmsMacroblock(), HlmsBlendblock(),
                                HlmsParamVec(), false );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setPrecisionMode( PrecisionMode precisionMode )
    {
        if( mPrecisionMode != precisionMode )
        {
            clearShaderCache();
            mPrecisionMode = precisionMode;
        }
    }
    //-----------------------------------------------------------------------------------
    Hlms::PrecisionMode Hlms::getPrecisionMode() const
    {
        return static_cast<PrecisionMode>( mPrecisionMode );
    }
    //-----------------------------------------------------------------------------------
    Hlms::PrecisionMode Hlms::getSupportedPrecisionMode() const
    {
        switch( mPrecisionMode )
        {
        case PrecisionFull32:
            return PrecisionFull32;

        case PrecisionMidf16:
        {
            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
            if( capabilities->hasCapability( RSC_SHADER_FLOAT16 ) )
                return PrecisionMidf16;
            else if( capabilities->hasCapability( RSC_SHADER_RELAXED_FLOAT ) )
                return PrecisionRelaxed;
            else
                return PrecisionFull32;
        }

        case PrecisionRelaxed:
        {
            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
            if( capabilities->hasCapability( RSC_SHADER_RELAXED_FLOAT ) )
                return PrecisionRelaxed;
            else if( capabilities->hasCapability( RSC_SHADER_FLOAT16 ) )
                return PrecisionMidf16;
            else
                return PrecisionFull32;
        }
        }

        return static_cast<PrecisionMode>( mPrecisionMode );
    }
    //-----------------------------------------------------------------------------------
    int32 Hlms::getSupportedPrecisionModeHash() const
    {
        switch( getSupportedPrecisionMode() )
        {
        case PrecisionFull32:
            return static_cast<int32>( HlmsBaseProp::Full32.getU32Value() );
        case PrecisionMidf16:
            return static_cast<int32>( HlmsBaseProp::Midf16.getU32Value() );
        case PrecisionRelaxed:
            return static_cast<int32>( HlmsBaseProp::Relaxed.getU32Value() );
        }

        // Silence MSVC warning
        return static_cast<int32>( HlmsBaseProp::Full32.getU32Value() );
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::getFastShaderBuildHack() const { return mFastShaderBuildHack; }
    //-----------------------------------------------------------------------------------
    void Hlms::setMaxNonCasterDirectionalLights( uint16 maxLights ) { mNumLightsLimit = maxLights; }
    //-----------------------------------------------------------------------------------
    void Hlms::setStaticBranchingLights( bool staticBranchingLights )
    {
        mStaticBranchingLights = staticBranchingLights;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setAreaLightForwardSettings( uint16 areaLightsApproxLimit, uint16 areaLightsLtcLimit )
    {
        mNumAreaApproxLightsLimit = areaLightsApproxLimit;
        mNumAreaLtcLightsLimit = areaLightsLtcLimit;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::saveAllTexturesFromDatablocks( const String &folderPath, set<String>::type &savedTextures,
                                              bool saveOitd, bool saveOriginal,
                                              HlmsTextureExportListener *listener )
    {
        HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
        HlmsDatablockMap::const_iterator endt = mDatablocks.end();

        while( itor != endt )
        {
            itor->second.datablock->saveTextures( folderPath, savedTextures, saveOitd, saveOriginal,
                                                  listener );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::reloadFrom( Archive *newDataFolder, ArchiveVec *libraryFolders )
    {
        clearShaderCache();

        if( libraryFolders )
        {
            mLibrary.clear();

            ArchiveVec::const_iterator itor = libraryFolders->begin();
            ArchiveVec::const_iterator endt = libraryFolders->end();

            while( itor != endt )
            {
                Library library;
                library.dataFolder = *itor;
                mLibrary.push_back( library );
                ++itor;
            }
        }
        else
        {
            LibraryVec::iterator itor = mLibrary.begin();
            LibraryVec::iterator endt = mLibrary.end();

            while( itor != endt )
            {
                for( size_t i = 0; i < NumShaderTypes; ++i )
                    itor->pieceFiles[i].clear();
                ++itor;
            }
        }

        for( size_t i = 0; i < NumShaderTypes; ++i )
            mPieceFiles[i].clear();

        mDataFolder = newDataFolder;
        enumeratePieceFiles();

        // Reload datablock pieces (those that can be reloaded from disk).
        for( auto &itor : mDatablockCustomPieceFiles )
        {
            DatablockCustomPieceFile &datablockCustomPiece = itor.second;
            if( datablockCustomPiece.isCacheable() )
            {
                const DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(
                    datablockCustomPiece.filename, datablockCustomPiece.resourceGroup );
                datablockCustomPiece.sourceCode = stream->getAsString();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    ArchiveVec Hlms::getPiecesLibraryAsArchiveVec() const
    {
        ArchiveVec retVal;
        LibraryVec::const_iterator itor = mLibrary.begin();
        LibraryVec::const_iterator endt = mLibrary.end();

        while( itor != endt )
        {
            retVal.push_back( itor->dataFolder );
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::_setNumThreads( size_t numThreads ) { mT.resize( numThreads ); }
    //-----------------------------------------------------------------------------------
    void Hlms::_setShadersGenerated( uint32 shadersGenerated ) { mShadersGenerated = shadersGenerated; }
    //-----------------------------------------------------------------------------------
    HlmsDatablock *Hlms::createDatablock( IdString name, const String &refName,
                                          const HlmsMacroblock &macroblockRef,
                                          const HlmsBlendblock &blendblockRef,
                                          const HlmsParamVec &paramVec, bool visibleToManager,
                                          const String &filename, const String &resourceGroup )
    {
        if( mDatablocks.find( name ) != mDatablocks.end() )
        {
            OGRE_EXCEPT(
                Exception::ERR_DUPLICATE_ITEM,
                "A material datablock with name '" + name.getFriendlyText() + "' already exists.",
                "Hlms::createDatablock" );
        }

        const HlmsMacroblock *macroblock = mHlmsManager->getMacroblock( macroblockRef );
        const HlmsBlendblock *blendblock = mHlmsManager->getBlendblock( blendblockRef );

        HlmsDatablock *retVal = createDatablockImpl( name, macroblock, blendblock, paramVec );

        mDatablocks[name] = DatablockEntry( retVal, visibleToManager, refName, filename, resourceGroup );

        retVal->calculateHash();

        if( visibleToManager )
            mHlmsManager->_datablockAdded( retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    HlmsDatablock *Hlms::getDatablock( IdString name ) const
    {
        HlmsDatablock *retVal = 0;
        HlmsDatablockMap::const_iterator itor = mDatablocks.find( name );
        if( itor != mDatablocks.end() )
            retVal = itor->second.datablock;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const String *Hlms::getNameStr( IdString name ) const
    {
        String const *retVal = 0;
        HlmsDatablockMap::const_iterator itor = mDatablocks.find( name );
        if( itor != mDatablocks.end() )
            retVal = &itor->second.name;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::getFilenameAndResourceGroup( IdString name, String const **outFilename,
                                            String const **outResourceGroup ) const
    {
        HlmsDatablockMap::const_iterator itor = mDatablocks.find( name );
        if( itor != mDatablocks.end() )
        {
            *outFilename = &itor->second.srcFile;
            *outResourceGroup = &itor->second.srcResourceGroup;
        }
        else
        {
            *outFilename = 0;
            *outResourceGroup = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::destroyDatablock( IdString name )
    {
        HlmsDatablockMap::iterator itor = mDatablocks.find( name );
        if( itor == mDatablocks.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Can't find datablock with name '" + name.getFriendlyText() + "'",
                         "Hlms::destroyDatablock" );
        }

        if( itor->second.visibleToManager )
            mHlmsManager->_datablockDestroyed( name );

        OGRE_DELETE itor->second.datablock;
        mDatablocks.erase( itor );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::_destroyAllDatablocks()
    {
        HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
        HlmsDatablockMap::const_iterator endt = mDatablocks.end();

        while( itor != endt )
        {
            if( itor->second.visibleToManager && mHlmsManager )
                mHlmsManager->_datablockDestroyed( itor->first );

            OGRE_DELETE itor->second.datablock;
            ++itor;
        }

        mDatablocks.clear();
        mDefaultDatablock = 0;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::destroyAllDatablocks()
    {
        _destroyAllDatablocks();
        mDefaultDatablock = createDefaultDatablock();
    }
    //-----------------------------------------------------------------------------------
    HlmsDatablock *Hlms::getDefaultDatablock() const { return mDefaultDatablock; }
    //-----------------------------------------------------------------------------------
    bool Hlms::findParamInVec( const HlmsParamVec &paramVec, IdString key, String &inOut )
    {
        bool retVal = false;
        HlmsParamVec::const_iterator it =
            std::lower_bound( paramVec.begin(), paramVec.end(),
                              std::pair<IdString, String>( key, String() ), OrderParamVecByKey );
        if( it != paramVec.end() && it->first == key )
        {
            inOut = it->second;
            retVal = true;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    HlmsCache *Hlms::addStubShaderCache( uint32 hash )
    {
        HlmsCache cache( hash, mType, HLMS_CACHE_FLAGS_COMPILATION_REQUIRED, HlmsPso() );
        HlmsCacheVec::iterator it =
            std::lower_bound( mShaderCache.begin(), mShaderCache.end(), &cache, OrderCacheByHash );

        OGRE_ASSERT_LOW(
            ( it == mShaderCache.end() || ( *it )->hash != hash ) &&
            "Can't add the same shader to the cache twice! (or a hash collision happened)" );

        HlmsCache *retVal = new HlmsCache( cache );
        mShaderCache.insert( it, retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *Hlms::addShaderCache( uint32 hash, const HlmsPso &pso )
    {
        ScopedLock lock( mMutex );

        HlmsCache cache( hash, mType, HLMS_CACHE_FLAGS_NONE, pso );
        HlmsCacheVec::iterator it =
            std::lower_bound( mShaderCache.begin(), mShaderCache.end(), &cache, OrderCacheByHash );

        OGRE_ASSERT_LOW(
            ( it == mShaderCache.end() || ( *it )->hash != hash ) &&
            "Can't add the same shader to the cache twice! (or a hash collision happened)" );

        HlmsCache *retVal = new HlmsCache( cache );
        mShaderCache.insert( it, retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *Hlms::getShaderCache( uint32 hash ) const
    {
        HlmsCache cache( hash, mType, HLMS_CACHE_FLAGS_NONE, HlmsPso() );
        HlmsCacheVec::const_iterator it =
            std::lower_bound( mShaderCache.begin(), mShaderCache.end(), &cache, OrderCacheByHash );

        if( it != mShaderCache.end() && ( *it )->hash == hash )
            return *it;

        return 0;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::clearShaderCache()
    {
        mPassCache.clear();

        // Empty mShaderCache so that mHlmsManager->destroyMacroblock would
        // be harmless even if _notifyMacroblockDestroyed gets called.
        HlmsCacheVec shaderCache;
        shaderCache.swap( mShaderCache );
        HlmsCacheVec::const_iterator itor = shaderCache.begin();
        HlmsCacheVec::const_iterator endt = shaderCache.end();

        while( itor != endt )
        {
            mRenderSystem->_hlmsPipelineStateObjectDestroyed( &( *itor )->pso );
            if( ( *itor )->pso.strongBlocks & HlmsPso::HasStrongMacroblock )
                mHlmsManager->destroyMacroblock( ( *itor )->pso.macroblock );
            if( ( *itor )->pso.strongBlocks & HlmsPso::HasStrongBlendblock )
                mHlmsManager->destroyBlendblock( ( *itor )->pso.blendblock );

            delete *itor;
            ++itor;
        }

        shaderCache.clear();

        mShaderCodeCache.clear();
        mShadersGenerated = 0u;
        mShaderCodeCacheDirty = true;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::processPieces( Archive *archive, const StringVector &pieceFiles, const size_t tid )
    {
        StringVector::const_iterator itor = pieceFiles.begin();
        StringVector::const_iterator endt = pieceFiles.end();

        while( itor != endt )
        {
            // Only open piece files with current render system extension
            const String::size_type extPos0 = itor->find( mShaderFileExt );
            const String::size_type extPos1 = itor->find( ".any" );
            if( extPos0 == itor->size() - mShaderFileExt.size() || extPos1 == itor->size() - 4u )
            {
                DataStreamPtr inFile = archive->open( *itor );

                String inString;
                String outString;

                inString.resize( inFile->size() );
                inFile->read( &inString[0], inFile->size() );

                this->parseMath( inString, outString, tid );
                while( outString.find( "@foreach" ) != String::npos )
                {
                    this->parseForEach( outString, inString, tid );
                    inString.swap( outString );
                }
                this->parseProperties( outString, inString, tid );
                this->parseUndefPieces( inString, outString, tid );
                this->collectPieces( outString, inString, tid );
                this->parseCounter( inString, outString, tid );
            }
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::dumpProperties( std::ofstream &outFile, const size_t tid )
    {
        outFile.write( "#if 0", sizeof( "#if 0" ) - 1u );

        char tmpBuffer[64];
        char friendlyText[32];
        LwString value( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        {
            HlmsPropertyVec::const_iterator itor = mT[tid].setProperties.begin();
            HlmsPropertyVec::const_iterator endt = mT[tid].setProperties.end();

            while( itor != endt )
            {
                itor->keyName.getFriendlyText( friendlyText, 32 );
                value.clear();
                value.a( itor->value );

                outFile.write( "\n\t***\t", sizeof( "\n\t***\t" ) - 1u );
                outFile.write( friendlyText,
                               static_cast<std::streamsize>( strnlen( friendlyText, 32 ) ) );
                outFile.write( "\t", sizeof( "\t" ) - 1u );
                outFile.write( value.c_str(), static_cast<std::streamsize>( value.size() ) );
                ++itor;
            }
        }

        outFile.write( "\n\tDONE DUMPING PROPERTIES", sizeof( "\n\tDONE DUMPING PROPERTIES" ) - 1u );

        {
            PiecesMap::const_iterator itor = mT[tid].pieces.begin();
            PiecesMap::const_iterator endt = mT[tid].pieces.end();

            while( itor != endt )
            {
                itor->first.getFriendlyText( friendlyText, 32 );
                outFile.write( "\n\t***\t", sizeof( "\n\t***\t" ) - 1u );
                outFile.write( friendlyText,
                               static_cast<std::streamsize>( strnlen( friendlyText, 32 ) ) );
                outFile.write( "\t", sizeof( "\t" ) - 1u );
                outFile.write( itor->second.c_str(),
                               static_cast<std::streamsize>( itor->second.size() ) );
                ++itor;
            }
        }

        outFile.write( "\n\tDONE DUMPING PIECES\n#endif\n",
                       sizeof( "\n\tDONE DUMPING PIECES\n#endif\n" ) - 1u );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::applyStrongBlockRules( HlmsPso &pso )
    {
        OGRE_ASSERT_LOW( pso.strongBlocks = 0 );

        // Modify a macroblock if needed.
        HlmsMacroblock macroblock = *pso.macroblock;
        macroblock.mRsData = nullptr;
        mListener->applyStrongMacroblockRules( macroblock,
                                               *this );  // Allows the listener to modify a macroblock
        applyStrongMacroblockRules(
            macroblock );  // Allows the implementation (inherited classes) to modify a macroblock
        OGRE_ASSERT_LOW( macroblock.mRsData ==
                         nullptr );  //  // Check implementation didn't reassign the blendblock.
        if( macroblock != *pso.macroblock )
        {
            // mHlmsManager->getMacroblock may be called from different Hlms implementations
            ScopedLock lock( msGlobalMutex );
            pso.macroblock = mHlmsManager->getMacroblock( macroblock );
            pso.strongBlocks |= HlmsPso::HasStrongMacroblock;
        }

        // Modify a blendblock if needed.
        HlmsBlendblock blendblock = *pso.blendblock;
        blendblock.mRsData = nullptr;
        mListener->applyStrongBlendblockRules( blendblock,
                                               *this );  // Allows the listener to modify a blendblock
        applyStrongBlendblockRules(
            blendblock );  // Allows the implementation (inherited classes) to modify a blendblock
        OGRE_ASSERT_LOW( blendblock.mRsData ==
                         nullptr );  // Check implementation didn't reassign the blendblock.
        if( blendblock != *pso.blendblock )
        {
            // mHlmsManager->getBlendblock may be called from different Hlms implementations
            ScopedLock lock( msGlobalMutex );
            pso.blendblock = mHlmsManager->getBlendblock( blendblock );
            pso.strongBlocks |= HlmsPso::HasStrongBlendblock;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::applyStrongMacroblockRules( HlmsMacroblock &macroblock )
    {
        const uint32 strongMacroblockBits = getProperty(HlmsPsoProp::StrongMacroblockBits);
        if (strongMacroblockBits == 0)
            return;

        // HlmsMacroblock::mScissorTestEnabled
        if ((strongMacroblockBits & HlmsPassPso::InvertScissorTest) == HlmsPassPso::InvertScissorTest)
            macroblock.mScissorTestEnabled = !macroblock.mScissorTestEnabled;
        else if( strongMacroblockBits & HlmsPassPso::ScissorTestEnabled )
            macroblock.mScissorTestEnabled = true;
        else if( strongMacroblockBits & HlmsPassPso::ScissorTestDisabled )
            macroblock.mScissorTestEnabled = false;

        // HlmsMacroblock::mDepthClamp
        if( ( strongMacroblockBits & HlmsPassPso::InvertDepthClamp ) == HlmsPassPso::InvertDepthClamp )
            macroblock.mDepthClamp = !macroblock.mDepthClamp;
        else if( strongMacroblockBits & HlmsPassPso::DepthClampEnabled )
            macroblock.mDepthClamp = true;
        else if( strongMacroblockBits & HlmsPassPso::DepthClampDisabled )
            macroblock.mDepthClamp = false;

        // HlmsMacroblock::mDepthCheck
        if( ( strongMacroblockBits & HlmsPassPso::InvertDepthCheck ) == HlmsPassPso::InvertDepthCheck )
            macroblock.mDepthCheck = !macroblock.mDepthCheck;
        else if( strongMacroblockBits & HlmsPassPso::DepthCheckEnabled )
            macroblock.mDepthCheck = true;
        else if( strongMacroblockBits & HlmsPassPso::DepthCheckDisabled )
            macroblock.mDepthCheck = false;

        // HlmsMacroblock::mDepthWrite
        if( ( strongMacroblockBits & HlmsPassPso::InvertDepthWrite ) == HlmsPassPso::InvertDepthWrite )
            macroblock.mDepthWrite = !macroblock.mDepthWrite;
        else if( strongMacroblockBits & HlmsPassPso::DepthWriteEnabled )
            macroblock.mDepthWrite = true;
        else if( strongMacroblockBits & HlmsPassPso::DepthWriteDisabled )
            macroblock.mDepthWrite = false;

        // HlmsMacroblock::mDepthFunc
        static_assert( ( HlmsPassPso::DepthFunc_ALWAYS_FAIL >> 8u ) - 1 == CMPF_ALWAYS_FAIL,
                       "DepthFunc_ALWAYS_FAIL doesn't match the CMPF_ALWAYS_FAIL." );
        static_assert( ( HlmsPassPso::DepthFunc_ALWAYS_PASS >> 8u ) - 1 == CMPF_ALWAYS_PASS,
                       "DepthFunc_ALWAYS_PASS doesn't match the CMPF_ALWAYS_PASS." );
        static_assert( ( HlmsPassPso::DepthFunc_LESS >> 8u ) - 1 == CMPF_LESS,
                       "DepthFunc_LESS doesn't match the CMPF_LESS." );
        static_assert( ( HlmsPassPso::DepthFunc_LESS_EQUAL >> 8u ) - 1 == CMPF_LESS_EQUAL,
                       "DepthFunc_LESS_EQUAL doesn't match the CMPF_LESS_EQUAL." );
        static_assert( ( HlmsPassPso::DepthFunc_EQUAL >> 8u ) - 1 == CMPF_EQUAL,
                       "DepthFunc_EQUAL doesn't match the CMPF_EQUAL." );
        static_assert( ( HlmsPassPso::DepthFunc_NOT_EQUAL >> 8u ) - 1 == CMPF_NOT_EQUAL,
                       "DepthFunc_NOT_EQUAL doesn't match the CMPF_NOT_EQUAL." );
        static_assert( ( HlmsPassPso::DepthFunc_GREATER_EQUAL >> 8u ) - 1 == CMPF_GREATER_EQUAL,
                       "DepthFunc_GREATER_EQUAL doesn't match the CMPF_GREATER_EQUAL." );
        static_assert( ( HlmsPassPso::DepthFunc_GREATER >> 8u ) - 1 == CMPF_GREATER,
                       "DepthFunc_GREATER doesn't match the CMPF_GREATER." );
        static_assert( NUM_COMPARE_FUNCTIONS == 8,
                       "CompareFunction enum has been changed. Update the "
                       "Hlms::applyStrongMacroblockRules implementation." );
        const uint32 depthFunc = ( strongMacroblockBits & HlmsPassPso::DepthFuncMask );
        if( depthFunc > 0 )
            macroblock.mDepthFunc = static_cast<CompareFunction>( ( depthFunc >> 8u ) - 1 );

        // HlmsMacroblock::mCullMode
        static_assert( ( HlmsPassPso::CullingMode_NONE >> 12u ) == CULL_NONE,
                       "CullingMode_NONE doesn't match the CULL_NONE." );
        static_assert( ( HlmsPassPso::CullingMode_CLOCKWISE >> 12u ) == CULL_CLOCKWISE,
                       "CullingMode_CLOCKWISE doesn't match the CULL_CLOCKWISE." );
        static_assert( ( HlmsPassPso::CullingMode_ANTICLOCKWISE >> 12u ) == CULL_ANTICLOCKWISE,
                       "CullingMode_ANTICLOCKWISE doesn't match the CULL_ANTICLOCKWISE." );
        const uint32 cullingMode = ( strongMacroblockBits & HlmsPassPso::CullingModeMask );
        if( cullingMode == HlmsPassPso::InvertCullingMode )
            macroblock.mCullMode =
                macroblock.mCullMode == CULL_CLOCKWISE ? CULL_ANTICLOCKWISE : CULL_CLOCKWISE;
        else if( cullingMode > 0 )
            macroblock.mCullMode = static_cast<CullingMode>( cullingMode >> 12u );

        // HlmsMacroblock::mPolygonMode
        static_assert( ( HlmsPassPso::PolygonMode_POINTS >> 16u ) == PM_POINTS,
                       "PolygonMode_POINTS doesn't match the PM_POINTS." );
        static_assert( ( HlmsPassPso::PolygonMode_WIREFRAME >> 16u ) == PM_WIREFRAME,
                       "PolygonMode_WIREFRAME doesn't match the PM_WIREFRAME." );
        static_assert( ( HlmsPassPso::PolygonMode_SOLID >> 16u ) == PM_SOLID,
                       "PolygonMode_SOLID doesn't match the PM_SOLID." );
        const uint32 polygonMode = ( strongMacroblockBits & HlmsPassPso::PolygonModeMask );
        if( polygonMode > 0 )
            macroblock.mPolygonMode = static_cast<PolygonMode>( polygonMode >> 16u );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::applyStrongBlendblockRules( HlmsBlendblock &blendblock )
    {
        const uint32 strongBlendblockBits = getProperty( HlmsPsoProp::StrongBlendblockBits );
        if( strongBlendblockBits == 0 )
            return;

        static_assert( HlmsPassPso::SourceBlendFactor_ONE - 1 == SBF_ONE,
                       "SourceBlendFactor_ONE doesn't match the SBF_ONE." );
        static_assert( HlmsPassPso::SourceBlendFactor_ZERO - 1 == SBF_ZERO,
                       "SourceBlendFactor_ZERO doesn't match the SBF_ZERO." );
        static_assert( HlmsPassPso::SourceBlendFactor_DEST_COLOUR - 1 == SBF_DEST_COLOUR,
                       "SourceBlendFactor_DEST_COLOUR doesn't match the SBF_DEST_COLOUR." );
        static_assert( HlmsPassPso::SourceBlendFactor_SOURCE_COLOUR - 1 == SBF_SOURCE_COLOUR,
                       "SourceBlendFactor_SOURCE_COLOUR doesn't match the SBF_SOURCE_COLOUR." );
        static_assert(
            HlmsPassPso::SourceBlendFactor_ONE_MINUS_DEST_COLOUR - 1 == SBF_ONE_MINUS_DEST_COLOUR,
            "SourceBlendFactor_ONE_MINUS_DEST_COLOUR doesn't match the SBF_ONE_MINUS_DEST_COLOUR." );
        static_assert(
            HlmsPassPso::SourceBlendFactor_ONE_MINUS_SOURCE_COLOUR - 1 == SBF_ONE_MINUS_SOURCE_COLOUR,
            "SourceBlendFactor_ONE_MINUS_SOURCE_COLOUR doesn't match the SBF_ONE_MINUS_SOURCE_COLOUR." );
        static_assert( HlmsPassPso::SourceBlendFactor_DEST_ALPHA - 1 == SBF_DEST_ALPHA,
                       "SourceBlendFactor_DEST_ALPHA doesn't match the SBF_DEST_ALPHA." );
        static_assert( HlmsPassPso::SourceBlendFactor_SOURCE_ALPHA - 1 == SBF_SOURCE_ALPHA,
                       "SourceBlendFactor_SOURCE_ALPHA doesn't match the SBF_SOURCE_ALPHA." );
        static_assert(
            HlmsPassPso::SourceBlendFactor_ONE_MINUS_DEST_ALPHA - 1 == SBF_ONE_MINUS_DEST_ALPHA,
            "SourceBlendFactor_ONE_MINUS_DEST_ALPHA doesn't match the SBF_ONE_MINUS_DEST_ALPHA." );
        static_assert(
            HlmsPassPso::SourceBlendFactor_ONE_MINUS_SOURCE_ALPHA - 1 == SBF_ONE_MINUS_SOURCE_ALPHA,
            "SourceBlendFactor_ONE_MINUS_SOURCE_ALPHA doesn't match the SBF_ONE_MINUS_SOURCE_ALPHA." );

        // HlmsBlendblock::mSourceBlendFactor
        const uint32 sourceBlendFactor = ( strongBlendblockBits & HlmsPassPso::SourceBlendFactorMask );
        if( sourceBlendFactor > 0 )
            blendblock.mSourceBlendFactor = static_cast<SceneBlendFactor>( ( sourceBlendFactor >> 0u ) - 1 );

        // HlmsBlendblock::mDestBlendFactor
        const uint32 destBlendFactor = ( strongBlendblockBits & HlmsPassPso::DestBlendFactorMask );
        if( destBlendFactor > 0 )
            blendblock.mDestBlendFactor =
                static_cast<SceneBlendFactor>( ( destBlendFactor >> 4u ) - 1 );

        // HlmsBlendblock::mSourceBlendFactorAlpha
        const uint32 sourceBlendFactorAplha = ( strongBlendblockBits & HlmsPassPso::SourceBlendFactorAlphaMask );
        if( sourceBlendFactorAplha > 0 )
            blendblock.mSourceBlendFactorAlpha =
                static_cast<SceneBlendFactor>( ( sourceBlendFactorAplha >> 8u ) - 1 );

        // HlmsBlendblock::mDestBlendFactorAlpha
        const uint32 destBlendFactorAlpha = ( strongBlendblockBits & HlmsPassPso::DestBlendFactorAlphaMask );
        if( destBlendFactorAlpha > 0 )
            blendblock.mDestBlendFactorAlpha = static_cast<SceneBlendFactor>( ( destBlendFactorAlpha >> 12u ) - 1 );

        static_assert( ( HlmsPassPso::BlendOperation_ADD - 1 ) >> 16u == SBO_ADD,
                       "BlendOperation_ADD doesn't match the SBO_ADD." );
        static_assert( ( HlmsPassPso::BlendOperation_SUBTRACT - 1 ) >> 16u == SBO_SUBTRACT,
                       "BlendOperation_SUBTRACT doesn't match the SBO_SUBTRACT." );
        static_assert(
            ( HlmsPassPso::BlendOperation_REVERSE_SUBTRACT - 1 ) >> 16u == SBO_REVERSE_SUBTRACT,
            "BlendOperation_REVERSE_SUBTRACT doesn't match the SBO_REVERSE_SUBTRACT." );
        static_assert( ( HlmsPassPso::BlendOperation_MIN - 1 ) >> 16u == SBO_MIN,
                       "BlendOperation_MIN doesn't match the SBO_MIN." );
        static_assert( ( HlmsPassPso::BlendOperation_MAX - 1 ) >> 16u == SBO_MAX,
                       "BlendOperation_MAX doesn't match the SBO_MAX." );

        // HlmsBlendblock::mBlendOperation
        const uint32 blendOperation =
            ( strongBlendblockBits & HlmsPassPso::BlendOperationMask );
        if( blendOperation > 0 )
            blendblock.mBlendOperation =
                static_cast<SceneBlendOperation>( ( blendOperation >> 16u ) - 1 );

        // HlmsBlendblock::mBlendOperationAlpha
        const uint32 blendOperationAlpha = ( strongBlendblockBits & HlmsPassPso::BlendOperationAlphaMask );
        if( blendOperationAlpha > 0 )
            blendblock.mBlendOperationAlpha =
                static_cast<SceneBlendOperation>( ( blendOperationAlpha >> 20u ) - 1 );
    }
    //-----------------------------------------------------------------------------------
    HighLevelGpuProgramPtr Hlms::compileShaderCode( const String &source,
                                                    const String &debugFilenameOutput, uint32 finalHash,
                                                    ShaderType shaderType, const size_t tid )
    {
        HighLevelGpuProgramManager *gpuProgramManager = HighLevelGpuProgramManager::getSingletonPtr();

        HighLevelGpuProgramPtr gp;
        {
            // gpuProgramManager->createProgram may be called from different Hlms implementations
            ScopedLock lock( msGlobalMutex );
            gp = gpuProgramManager->createProgram(
                StringConverter::toString( finalHash ) + ShaderFiles[shaderType],
                ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, mShaderProfile,
                static_cast<GpuProgramType>( shaderType ) );
        }
        gp->setSource( source, debugFilenameOutput );

        {
            RootLayout rootLayout;
            setupRootLayout( rootLayout, tid );
            gp->setRootLayout( gp->getType(), rootLayout );
        }

        if( mShaderTargets[shaderType] )
        {
            // D3D-specific
            gp->setParameter( "target", *mShaderTargets[shaderType] );
            gp->setParameter( "entry_point", "main" );
        }

        gp->setBuildParametersFromReflection( false );
        gp->setSkeletalAnimationIncluded( getProperty( tid, HlmsBaseProp::Skeleton ) != 0 );
        gp->setMorphAnimationIncluded( false );
        gp->setPoseAnimationIncluded( getProperty( tid, HlmsBaseProp::Pose ) != 0 );
        gp->setVpAndRtArrayIndexFromAnyShaderRequired(
            getProperty( tid, HlmsBaseProp::InstancedStereo ) != 0 );
        gp->setVertexTextureFetchRequired( false );

        gp->load();

        return gp;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::_compileShaderFromPreprocessedSource( const RenderableCache &mergedCache,
                                                     const String source[NumShaderTypes],
                                                     const uint32 shaderCounter, const size_t tid )
    {
        OgreProfileExhaustive( "Hlms::_compileShaderFromPreprocessedSource" );

        const uint32 uniqueName = mType * 100000000u + shaderCounter;

        ShaderCodeCache codeCache( mergedCache.pieces );
        codeCache.mergedCache.setProperties = mergedCache.setProperties;

        codeCache.mergedCache.setProperties.swap( mT[tid].setProperties );

        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            if( !source[i].empty() )
            {
                String debugFilenameOutput;
                std::ofstream debugDumpFile;
                if( mDebugOutput )
                {
                    debugFilenameOutput = mOutputPath + "./" + StringConverter::toString( uniqueName ) +
                                          ShaderFiles[i] + mShaderFileExt;
                    debugDumpFile.open( Ogre::fileSystemPathFromString( debugFilenameOutput ).c_str(),
                                        std::ios::out | std::ios::binary );

                    if( mDebugOutputProperties )
                        dumpProperties( debugDumpFile, tid );
                }

                codeCache.shaders[i] =
                    compileShaderCode( source[i], "", uniqueName, static_cast<ShaderType>( i ), tid );

                if( mDebugOutput )
                {
                    debugDumpFile.write( source[i].c_str(),
                                         static_cast<std::streamsize>( source[i].size() ) );
                }
            }
        }

        codeCache.mergedCache.setProperties.swap( mT[tid].setProperties );

        // Ensure code didn't accidentally modify mSetProperties
        OGRE_ASSERT_HIGH( codeCache.mergedCache.setProperties == mergedCache.setProperties );

        ScopedLock lock( mMutex );
        mShaderCodeCache.push_back( codeCache );
        mShaderCodeCacheDirty = true;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::compileShaderCode( ShaderCodeCache &codeCache, const uint32 shaderCounter,
                                  const size_t tid )
    {
        OgreProfileExhaustive( "Hlms::compileShaderCode" );

        // Give the shaders friendly base-10 names
        const uint32 uniqueName = mType * 100000000u + shaderCounter;

        mT[tid].setProperties = codeCache.mergedCache.setProperties;

        // Generate the shaders
        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            // Collect pieces
            mT[tid].pieces = codeCache.mergedCache.pieces[i];

            const String filename = ShaderFiles[i] + mShaderFileExt;
            if( mDataFolder->exists( filename ) )
            {
                if( mShaderProfile == "glsl" || mShaderProfile == "glslvk" )  // TODO: String comparision
                {
                    setProperty( tid, HlmsBaseProp::GL3Plus,
                                 mRenderSystem->getNativeShadingLanguageVersion() );
                }

                setProperty( tid, HlmsBaseProp::Syntax,
                             static_cast<int32>( mShaderSyntax.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Hlsl,
                             static_cast<int32>( HlmsBaseProp::Hlsl.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Glsl,
                             static_cast<int32>( HlmsBaseProp::Glsl.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Glslvk,
                             static_cast<int32>( HlmsBaseProp::Glslvk.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Hlslvk,
                             static_cast<int32>( HlmsBaseProp::Hlslvk.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Metal,
                             static_cast<int32>( HlmsBaseProp::Metal.getU32Value() ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
                setProperty( tid, HlmsBaseProp::iOS, 1 );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
                setProperty( tid, HlmsBaseProp::macOS, 1 );
#endif
                setProperty( tid, HlmsBaseProp::Full32,
                             static_cast<int32>( HlmsBaseProp::Full32.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Midf16,
                             static_cast<int32>( HlmsBaseProp::Midf16.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::Relaxed,
                             static_cast<int32>( HlmsBaseProp::Relaxed.getU32Value() ) );
                setProperty( tid, HlmsBaseProp::PrecisionMode, getSupportedPrecisionModeHash() );

                if( mFastShaderBuildHack )
                    setProperty( tid, HlmsBaseProp::FastShaderBuildHack, 1 );

                String debugFilenameOutput;
                std::ofstream debugDumpFile;
                if( mDebugOutput )
                {
                    debugFilenameOutput = mOutputPath + "./" + StringConverter::toString( uniqueName ) +
                                          ShaderFiles[i] + mShaderFileExt;
                    debugDumpFile.open( Ogre::fileSystemPathFromString( debugFilenameOutput ).c_str(),
                                        std::ios::out | std::ios::binary );

                    // We need to dump the properties before processing the files, as these
                    // may be overwritten or polluted by the files, thus hiding why we
                    // got this permutation.
                    if( mDebugOutputProperties )
                        dumpProperties( debugDumpFile, tid );
                }

                const int32 customPieceName =
                    getProperty( tid, HlmsBaseProp::_DatablockCustomPieceShaderName[i] );
                if( customPieceName )
                {
                    // Parse custom arbitrary shader piece specified by the datablock.
                    DatablockCustomPieceFileMap::const_iterator it =
                        mDatablockCustomPieceFiles.find( customPieceName );
                    OGRE_ASSERT_LOW( it != mDatablockCustomPieceFiles.end() );

                    String inString = it->second.sourceCode;
                    String outString;

                    this->parseMath( inString, outString, tid );
                    while( outString.find( "@foreach" ) != String::npos )
                    {
                        this->parseForEach( outString, inString, tid );
                        inString.swap( outString );
                    }
                    this->parseProperties( outString, inString, tid );
                    this->parseUndefPieces( inString, outString, tid );
                    this->collectPieces( outString, inString, tid );
                    this->parseCounter( inString, outString, tid );
                }

                // Library piece files first
                LibraryVec::const_iterator itor = mLibrary.begin();
                LibraryVec::const_iterator endt = mLibrary.end();

                while( itor != endt )
                {
                    processPieces( itor->dataFolder, itor->pieceFiles[i], tid );
                    ++itor;
                }

                // Main piece files
                processPieces( mDataFolder, mPieceFiles[i], tid );

                // Generate the shader file.
                DataStreamPtr inFile = mDataFolder->open( filename );

                String inString;
                String outString;

                inString.resize( inFile->size() );
                inFile->read( &inString[0], inFile->size() );

                bool syntaxError = false;

                syntaxError |= this->parseMath( inString, outString, tid );
                while( !syntaxError && outString.find( "@foreach" ) != String::npos )
                {
                    syntaxError |= this->parseForEach( outString, inString, tid );
                    inString.swap( outString );
                }
                syntaxError |= this->parseProperties( outString, inString, tid );
                syntaxError |= this->parseUndefPieces( inString, outString, tid );
                while( !syntaxError && ( outString.find( "@piece" ) != String::npos ||
                                         outString.find( "@insertpiece" ) != String::npos ) )
                {
                    syntaxError |= this->collectPieces( outString, inString, tid );
                    syntaxError |= this->insertPieces( inString, outString, tid );
                }
                syntaxError |= this->parseCounter( outString, inString, tid );

                outString.swap( inString );

                if( syntaxError )
                {
                    LogManager::getSingleton().logMessage(
                        "There were HLMS syntax errors while parsing " +
                        StringConverter::toString( uniqueName ) + ShaderFiles[i] );
                }

                // Now dump the processed file.
                if( mDebugOutput )
                {
                    debugDumpFile.write( &outString[0],
                                         static_cast<std::streamsize>( outString.size() ) );
                }

                // Don't create and compile if template requested not to
                if( !getProperty( tid, HlmsBaseProp::DisableStage ) )
                {
                    codeCache.shaders[i] = compileShaderCode( outString, debugFilenameOutput, uniqueName,
                                                              static_cast<ShaderType>( i ), tid );
                }

                // Reset the disable flag.
                setProperty( tid, HlmsBaseProp::DisableStage, 0 );
            }
        }

        ScopedLock lock( mMutex );
        mShaderCodeCache.push_back( codeCache );
        mShaderCodeCacheDirty = true;
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *Hlms::createShaderCacheEntry( uint32 renderableHash, const HlmsCache &passCache,
                                                   uint32 finalHash,
                                                   const QueuedRenderable &queuedRenderable,
                                                   HlmsCache *reservedStubEntry, uint64 deadline,
                                                   const size_t tid )
    {
        OgreProfileExhaustive( "Hlms::createShaderCacheEntry" );

        // Set the properties by merging the cache from the pass, with the cache from renderable
        mT[tid].setProperties.clear();
        // If retVal is null, we did something wrong earlier
        //(the cache should've been generated by now)
        const RenderableCache &renderableCache = getRenderableCache( renderableHash );
        mT[tid].setProperties.reserve( passCache.setProperties.size() +
                                       renderableCache.setProperties.size() );
        // Copy the properties from the renderable
        mT[tid].setProperties.insert( mT[tid].setProperties.end(), renderableCache.setProperties.begin(),
                                      renderableCache.setProperties.end() );
        {
            // Now copy the properties from the pass (one by one, since be must maintain the order)
            HlmsPropertyVec::const_iterator itor = passCache.setProperties.begin();
            HlmsPropertyVec::const_iterator endt = passCache.setProperties.end();

            while( itor != endt )
            {
                setProperty( tid, itor->keyName, itor->value );
                ++itor;
            }
        }

        mT[tid].textureNameStrings.clear();
        for( size_t i = 0; i < NumShaderTypes; ++i )
            mT[tid].textureRegs[i].clear();

        {
            // Add RenderSystem-specific properties
            IdStringVec::const_iterator itor = mRsSpecificExtensions.begin();
            IdStringVec::const_iterator endt = mRsSpecificExtensions.end();

            while( itor != endt )
                setProperty( tid, *itor++, 1 );
        }

        ShaderCodeCache codeCache( renderableCache.pieces );

        const PropertiesMergeStatus status =
            notifyPropertiesMergedPreGenerationStep( tid, codeCache.mergedCache.pieces );
        if( status != PropertiesMergeStatusOk )
        {
            const HlmsDatablock *datablock = queuedRenderable.renderable->getDatablock();
            const String meshName =
                SceneManager::deduceMovableObjectName( queuedRenderable.movableObject );

            LogManager::getSingleton().logMessage(
                "[tid = " + StringConverter::toString( tid ) + "] datablock '" +
                *datablock->getNameStr() + "' from MovableObject '" + meshName +
                "' has issues. See previous log entries matching the same tid." );

            if( status == PropertiesMergeStatusError )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Errors encountered while generating shaders. See Ogre.log",
                             "Hlms::createShaderCacheEntry" );
            }
        }
        mListener->propertiesMergedPreGenerationStep( this, passCache, renderableCache.setProperties,
                                                      renderableCache.pieces, mT[tid].setProperties,
                                                      queuedRenderable, tid );

        // Retrieve the shader code from the code cache
        unsetProperty( tid, HlmsPsoProp::Macroblock );
        unsetProperty( tid, HlmsPsoProp::Blendblock );
        unsetProperty( tid, HlmsPsoProp::InputLayoutId );
        codeCache.mergedCache.setProperties.swap( mT[tid].setProperties );
        {
            bool bIsInCache;

            uint32_t shaderCounter = 0u;
            {
                ScopedLock lock( mMutex );
                ShaderCodeCacheVec::iterator itCodeCache =
                    std::find( mShaderCodeCache.begin(), mShaderCodeCache.end(), codeCache );
                bIsInCache = itCodeCache != mShaderCodeCache.end();

                if( bIsInCache )
                {
                    // This requires the mutex as itCodeCache could be invalidated
                    for( size_t i = 0; i < NumShaderTypes; ++i )
                        codeCache.shaders[i] = itCodeCache->shaders[i];
                }
                else
                    shaderCounter = mShadersGenerated++;
            }

            if( !bIsInCache )
                compileShaderCode( codeCache, shaderCounter, tid );
            else
            {
                // This can be done in parallel, as we've copied what itCodeCache needed
                codeCache.mergedCache.setProperties.swap( mT[tid].setProperties );
            }
        }

        HlmsPso pso;
        pso.initialize();
        pso.vertexShader = codeCache.shaders[VertexShader];
        pso.geometryShader = codeCache.shaders[GeometryShader];
        pso.tesselationHullShader = codeCache.shaders[HullShader];
        pso.tesselationDomainShader = codeCache.shaders[DomainShader];
        pso.pixelShader = codeCache.shaders[PixelShader];

        bool casterPass = getProperty( tid, HlmsBaseProp::ShadowCaster ) != 0;

        const HlmsDatablock *datablock = queuedRenderable.renderable->getDatablock();
        pso.macroblock = datablock->getMacroblock( casterPass );
        pso.blendblock = datablock->getBlendblock( casterPass );
        pso.pass = passCache.pso.pass;

        applyStrongBlockRules( pso );

        const size_t numGlobalClipDistances = (size_t)getProperty( tid, HlmsBaseProp::PsoClipDistances );
        pso.clipDistances = static_cast<uint8>( ( 1u << numGlobalClipDistances ) - 1u );

        // TODO: Configurable somehow (likely should be in datablock).
        pso.sampleMask = 0xffffffff;

        if( queuedRenderable.renderable )
        {
            const VertexArrayObjectArray &vaos =
                queuedRenderable.renderable->getVaos( static_cast<VertexPass>( casterPass ) );
            if( !vaos.empty() )
            {
                // v2 object. TODO: LOD? Should we allow Vaos with different vertex formats on LODs?
                //(also the datablock hash in the renderable would have to account for that)
                pso.operationType = vaos.front()->getOperationType();
                pso.vertexElements = vaos.front()->getVertexDeclaration();
            }
            else
            {
                // v1 object.
                v1::RenderOperation renderOp;
                queuedRenderable.renderable->getRenderOperation( renderOp, casterPass );
                pso.operationType = renderOp.operationType;
                pso.vertexElements = renderOp.vertexData->vertexDeclaration->convertToV2();
            }

            pso.enablePrimitiveRestart = false;
        }

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        LogManager::getSingleton().logMessage(
            "Compiling new PSO for datablock: " + datablock->getName().getFriendlyText(), LML_TRIVIAL );
#endif
        bool rsPsoCreated = mRenderSystem->_hlmsPipelineStateObjectCreated( &pso, deadline );

        if( reservedStubEntry )
        {
            OGRE_ASSERT_LOW( !reservedStubEntry->pso.vertexShader &&
                             !reservedStubEntry->pso.geometryShader &&
                             !reservedStubEntry->pso.tesselationHullShader &&
                             !reservedStubEntry->pso.tesselationDomainShader &&
                             !reservedStubEntry->pso.pixelShader && "Race condition?" );
            if( rsPsoCreated )
                reservedStubEntry->pso = pso;
            reservedStubEntry->flags =
                !rsPsoCreated && reservedStubEntry->flags == HLMS_CACHE_FLAGS_COMPILATION_REQUESTED
                    ? HLMS_CACHE_FLAGS_COMPILATION_REQUIRED  // recoverable error (timeout?), reschedule
                    : HLMS_CACHE_FLAGS_NONE;
        }

        const HlmsCache *retVal =
            reservedStubEntry ? reservedStubEntry : addShaderCache( finalHash, pso );

        applyTextureRegisters( retVal, tid );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    Hlms::PropertiesMergeStatus Hlms::notifyPropertiesMergedPreGenerationStep( const size_t tid,
                                                                               PiecesMap *inOutPieces )
    {
        if( getProperty( tid, HlmsBaseProp::AlphaToCoverage ) == HlmsBlendblock::A2cEnabledMsaaOnly )
        {
            if( getProperty( tid, HlmsBaseProp::MsaaSamples ) <= 1 )
                setProperty( tid, HlmsBaseProp::AlphaToCoverage, 0 );
        }

        // Pass cache data cannot cache pieces, so it stored shadow maps' UV values as raw
        // floating point in the properties. We can now turn it into pieces.
        const int32 numShadowMapLights = getProperty( tid, HlmsBaseProp::NumShadowMapLights );

        char tmpBuffer[64];
        LwString propName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        char tmpBuffer2[32];
        LwString valueStr( LwString::FromEmptyPointer( tmpBuffer2, sizeof( tmpBuffer2 ) ) );

        propName = "hlms_shadowmap";
        const size_t basePropNameSize = propName.size();

        for( int32 i = 0; i < numShadowMapLights; ++i )
        {
            propName.resize( basePropNameSize );
            propName.a( i );  // hlms_shadowmap0

            const size_t basePropSize = propName.size();

            const uint32 kNumSuffixes = 6u;
            const char *suffixes[kNumSuffixes] = { "_uv_min_x",    "_uv_min_y",  //
                                                   "_uv_max_x",    "_uv_max_y",
                                                   "_uv_length_x", "_uv_length_y" };

            for( uint32 j = 0; j < kNumSuffixes; ++j )
            {
                propName.resize( basePropSize );
                propName.a( suffixes[j] );

                const IdString propNameHash = propName.c_str();

                const int32_t value = getProperty( tid, propNameHash, -1 );
                if( value != -1 )
                {
                    const float fValue = bit_cast<float>( value );

                    valueStr.clear();
                    valueStr.a( fValue );

                    const PiecesMap::value_type v = { propNameHash, valueStr.c_str() };

                    for( size_t k = 0u; k < NumShaderTypes; ++k )
                        inOutPieces[k].insert( v );
                }
            }
        }

        return PropertiesMergeStatusOk;
    }
    //-----------------------------------------------------------------------------------
    uint16 Hlms::calculateHashForV1( Renderable *renderable )
    {
        v1::RenderOperation op;
        // The Hlms uses the pass scene data to know whether this is a caster pass.
        // We want to know all the details, so request for the non-caster RenderOp.
        renderable->getRenderOperation( op, false );
        v1::VertexDeclaration *vertexDecl = op.vertexData->vertexDeclaration;
        const v1::VertexDeclaration::VertexElementList &elementList = vertexDecl->getElements();
        v1::VertexDeclaration::VertexElementList::const_iterator itor = elementList.begin();
        v1::VertexDeclaration::VertexElementList::const_iterator endt = elementList.end();

        uint numTexCoords = 0;
        while( itor != endt )
        {
            const v1::VertexElement &vertexElem = *itor;
            calculateHashForSemantic( vertexElem.getSemantic(), vertexElem.getType(),
                                      vertexElem.getIndex(), numTexCoords );
            ++itor;
        }

        // v1::VertexDeclaration doesn't hold opType information. We need to save it now.
        // This means we do not allow LODs with different operation types or vertex layouts
        uint16 inputLayoutId = vertexDecl->_getInputLayoutId( mHlmsManager, op.operationType );
        setProperty( kNoTid, HlmsPsoProp::InputLayoutId, inputLayoutId );

        return static_cast<uint16>( numTexCoords );
    }
    //-----------------------------------------------------------------------------------
    uint16 Hlms::calculateHashForV2( Renderable *renderable )
    {
        // TODO: Account LOD
        VertexArrayObject *vao = renderable->getVaos( VpNormal )[0];
        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

        uint numTexCoords = 0;
        uint16 semIndex[VES_COUNT];
        memset( semIndex, 0, sizeof( semIndex ) );
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator endt = vertexBuffers.end();

        while( itor != endt )
        {
            const VertexElement2Vec &vertexElements = ( *itor )->getVertexElements();
            VertexElement2Vec::const_iterator itElements = vertexElements.begin();
            VertexElement2Vec::const_iterator enElements = vertexElements.end();

            while( itElements != enElements )
            {
                calculateHashForSemantic( itElements->mSemantic, itElements->mType,
                                          semIndex[itElements->mSemantic - 1]++, numTexCoords );
                ++itElements;
            }

            ++itor;
        }

        // We do not allow LODs with different operation types or vertex layouts
        setProperty( kNoTid, HlmsPsoProp::InputLayoutId, vao->getInputLayoutId() );

        return static_cast<uint16>( numTexCoords );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::calculateHashForSemantic( VertexElementSemantic semantic, VertexElementType type,
                                         uint16 index, uint &inOutNumTexCoords )
    {
        switch( semantic )
        {
        case VES_NORMAL:
            if( v1::VertexElement::getTypeCount( type ) < 4 )
            {
                setProperty( kNoTid, HlmsBaseProp::Normal, 1 );
            }
            else
            {
                setProperty( kNoTid, HlmsBaseProp::QTangent, 1 );
            }
            break;
        case VES_TANGENT:
            setProperty( kNoTid, HlmsBaseProp::Tangent, 1 );
            if( v1::VertexElement::getTypeCount( type ) == 4 )
            {
                setProperty( kNoTid, HlmsBaseProp::Tangent4, 1 );
            }
            break;
        case VES_DIFFUSE:
            setProperty( kNoTid, HlmsBaseProp::Colour, 1 );
            break;
        case VES_TEXTURE_COORDINATES:
            inOutNumTexCoords = std::max<uint>( inOutNumTexCoords, index + 1 );
            setProperty( kNoTid, *HlmsBaseProp::UvCountPtrs[index],
                         v1::VertexElement::getTypeCount( type ) );
            break;
        case VES_BLEND_WEIGHTS:
            setProperty( kNoTid, HlmsBaseProp::BonesPerVertex, v1::VertexElement::getTypeCount( type ) );
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setupSharedBasicProperties( Renderable *renderable, const bool bCasterPass )
    {
        HlmsDatablock *datablock = renderable->getDatablock();

        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const int32 filenameHashId =
                datablock->getCustomPieceFileIdHash( static_cast<ShaderType>( i ) );
            if( filenameHashId )
                setProperty( kNoTid, HlmsBaseProp::_DatablockCustomPieceShaderName[i], filenameHashId );
        }

        setProperty( kNoTid, HlmsBaseProp::AlphaTest, datablock->getAlphaTest() != CMPF_ALWAYS_PASS );
        setProperty( kNoTid, HlmsBaseProp::AlphaTestShadowCasterOnly,
                     datablock->getAlphaTestShadowCasterOnly() );
        setProperty( kNoTid, HlmsBaseProp::AlphaBlend,
                     datablock->getBlendblock( bCasterPass )->isAutoTransparent() );
        setProperty( kNoTid, HlmsBaseProp::AlphaToCoverage,
                     datablock->getBlendblock( bCasterPass )->mAlphaToCoverage );
        if( datablock->getAlphaHashing() )
            setProperty( kNoTid, HlmsBaseProp::AlphaHash, 1 );

        if( renderable->getUseIdentityWorldMatrix() )
            setProperty( kNoTid, HlmsBaseProp::IdentityWorld, 1 );

        if( renderable->getParticleType() != ParticleType::NotParticle )
        {
            setProperty( kNoTid, HlmsBaseProp::ParticleSystem, 1 );
            setProperty( kNoTid, HlmsBaseProp::VertexId, 1 );

            IdString particleTypeName;
            switch( renderable->getParticleType() )
            {
            case ParticleType::NotParticle:
            case ParticleType::Point:
                particleTypeName = HlmsBaseProp::ParticleTypePoint;
                break;
            case ParticleType::OrientedCommon:
                particleTypeName = HlmsBaseProp::ParticleTypeOrientedCommon;
                break;
            case ParticleType::OrientedSelf:
                particleTypeName = HlmsBaseProp::ParticleTypeOrientedSelf;
                break;
            case ParticleType::PerpendicularCommon:
                particleTypeName = HlmsBaseProp::ParticleTypePerpendicularCommon;
                break;
            case ParticleType::PerpendicularSelf:
                particleTypeName = HlmsBaseProp::ParticleTypePerpendicularSelf;
                break;
            }

            setProperty( kNoTid, HlmsBaseProp::ParticleType,
                         static_cast<int32>( particleTypeName.getU32Value() ) );
            setProperty( kNoTid, particleTypeName,
                         static_cast<int32>( particleTypeName.getU32Value() ) );

            const ParticleSystemDef *systemDef = ParticleSystemDef::castFromRenderable( renderable );

            setProperty( kNoTid, HlmsBaseProp::ParticleRotation,
                         static_cast<int32>( systemDef->getRotationType() ) );

            setProperty( kNoTid, HlmsBaseProp::Colour, 1 );

            // The UVs are autogenerated so set UvCount0 to indicate it's available.
            // However we don't set HlmsBaseProp::UvCount because it's autogenerated.
            setProperty( kNoTid, HlmsBaseProp::UvCount0, 2 );
        }

        if( renderable->getUseIdentityViewProjMatrixIsDynamic() )
            setProperty( kNoTid, HlmsBaseProp::IdentityViewProjDynamic, 1 );
        else if( renderable->getUseIdentityProjection() )
            setProperty( kNoTid, HlmsBaseProp::IdentityViewProj, 1 );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::calculateHashFor( Renderable *renderable, uint32 &outHash, uint32 &outCasterHash )
    {
        OgreProfileExhaustive( "Hlms::calculateHashFor" );

        mT[kNoTid].setProperties.clear();

        setProperty( kNoTid, HlmsBaseProp::Skeleton, renderable->hasSkeletonAnimation() );

        setProperty( kNoTid, HlmsBaseProp::Pose, renderable->getNumPoses() );
        setProperty( kNoTid, HlmsBaseProp::PoseHalfPrecision, renderable->getPoseHalfPrecision() );
        setProperty( kNoTid, HlmsBaseProp::PoseNormals, renderable->getPoseNormals() );

        uint16 numTexCoords = 0;
        if( renderable->getVaos( VpNormal ).empty() )
            numTexCoords = calculateHashForV1( renderable );
        else
            numTexCoords = calculateHashForV2( renderable );

        setProperty( kNoTid, HlmsBaseProp::UvCount, numTexCoords );

        setupSharedBasicProperties( renderable, false );

        HlmsDatablock *datablock = renderable->getDatablock();

        setProperty( kNoTid, HlmsPsoProp::Macroblock, datablock->getMacroblock( false )->mLifetimeId );
        setProperty( kNoTid, HlmsPsoProp::Blendblock, datablock->getBlendblock( false )->mLifetimeId );

        PiecesMap pieces[NumShaderTypes];
        if( datablock->getAlphaTest() != CMPF_ALWAYS_PASS )
        {
            pieces[PixelShader][HlmsBasePieces::AlphaTestCmpFunc] =
                HlmsDatablock::getCmpString( datablock->getAlphaTest() );
        }
        calculateHashForPreCreate( renderable, pieces );

        const uint32 renderableHash = this->addRenderableCache( mT[kNoTid].setProperties, pieces );

        // For shadow casters, turn normals off. UVs & diffuse also off unless there's alpha testing.
        setProperty( kNoTid, HlmsBaseProp::Normal, 0 );
        setProperty( kNoTid, HlmsBaseProp::QTangent, 0 );
        setProperty( kNoTid, HlmsBaseProp::AlphaBlend,
                     datablock->getBlendblock( true )->isAutoTransparent() );
        setProperty( kNoTid, HlmsBaseProp::AlphaToCoverage,
                     datablock->getBlendblock( true )->mAlphaToCoverage );
        PiecesMap piecesCaster[NumShaderTypes];
        if( datablock->getAlphaTest() != CMPF_ALWAYS_PASS )
        {
            piecesCaster[PixelShader][HlmsBasePieces::AlphaTestCmpFunc] =
                pieces[PixelShader][HlmsBasePieces::AlphaTestCmpFunc];
        }
        if( !renderable->getVaos( VpShadow ).empty() )
        {
            // v2 objects can have an input layout that is different for shadow mapping
            VertexArrayObject *vao = renderable->getVaos( VpShadow )[0];
            setProperty( kNoTid, HlmsPsoProp::InputLayoutId, vao->getInputLayoutId() );
        }
        calculateHashForPreCaster( renderable, piecesCaster, pieces );
        setProperty( kNoTid, HlmsPsoProp::Macroblock, datablock->getMacroblock( true )->mLifetimeId );
        setProperty( kNoTid, HlmsPsoProp::Blendblock, datablock->getBlendblock( true )->mLifetimeId );
        uint32 renderableCasterHash = this->addRenderableCache( mT[kNoTid].setProperties, piecesCaster );

        outHash = renderableHash;
        outCasterHash = renderableCasterHash;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::analyzeBarriers( BarrierSolver &barrierSolver,
                                ResourceTransitionArray &resourceTransitions, Camera *renderingCamera,
                                const bool bCasterPass )
    {
    }
    //-----------------------------------------------------------------------------------
    HlmsCache Hlms::preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                                     bool dualParaboloid, SceneManager *sceneManager )
    {
        mT[kNoTid].setProperties.clear();
        return preparePassHashBase( shadowNode, casterPass, dualParaboloid, sceneManager );
    }
    //-----------------------------------------------------------------------------------
    HlmsCache Hlms::preparePassHashBase( const CompositorShadowNode *shadowNode, bool casterPass,
                                         bool dualParaboloid, SceneManager *sceneManager )
    {
        if( !mRenderSystem->isReverseDepth() )
            setProperty( kNoTid, HlmsBaseProp::NoReverseDepth, 1 );

        const CompositorPass *pass = sceneManager->getCurrentCompositorPass();

        bool bForceCullNone = false;

        if( !casterPass )
        {
            size_t numShadowMapLights = 0u;
            size_t numPssmSplits = 0;
            bool useStaticBranchShadowMapLights = false;

            if( shadowNode )
            {
                const vector<Real>::type *pssmSplits = shadowNode->getPssmSplits( 0 );
                if( pssmSplits )
                    numPssmSplits = pssmSplits->size() - 1u;
                setProperty( kNoTid, HlmsBaseProp::PssmSplits, static_cast<int32>( numPssmSplits ) );

                bool isPssmBlend = false;
                const vector<Real>::type *pssmBlends = shadowNode->getPssmBlends( 0 );
                if( pssmBlends )
                    isPssmBlend = pssmBlends->size() > 0;
                setProperty( kNoTid, HlmsBaseProp::PssmBlend, isPssmBlend );

                bool isPssmFade = false;
                const Real *pssmFade = shadowNode->getPssmFade( 0 );
                if( pssmFade )
                    isPssmFade = *pssmFade != 0.0f;
                setProperty( kNoTid, HlmsBaseProp::PssmFade, isPssmFade );

                const TextureGpuVec &contiguousShadowMapTex = shadowNode->getContiguousShadowMapTex();

                numShadowMapLights = shadowNode->getNumActiveShadowCastingLights();
                if( numPssmSplits )
                    numShadowMapLights += numPssmSplits - 1u;
                setProperty( kNoTid, HlmsBaseProp::NumShadowMapLights,
                             static_cast<int32>( numShadowMapLights ) );
                setProperty( kNoTid, HlmsBaseProp::NumShadowMapTextures,
                             static_cast<int32>( contiguousShadowMapTex.size() ) );

                useStaticBranchShadowMapLights =
                    mStaticBranchingLights && numShadowMapLights > numPssmSplits;
                if( useStaticBranchShadowMapLights )
                    setProperty( kNoTid, HlmsBaseProp::StaticBranchShadowMapLights, 1 );
                mRealShadowMapPointLights = 0u;
                mRealShadowMapSpotLights = 0u;

                {
                    const Ogre::CompositorShadowNodeDef *shadowNodeDef = shadowNode->getDefinition();

                    char tmpBuffer[64];
                    LwString propName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

                    propName = "hlms_shadowmap";
                    const size_t basePropNameSize = propName.size();

                    uint32 shadowMapTexIdx = 0u;

                    for( size_t i = 0; i < numShadowMapLights; ++i )
                    {
                        // Skip inactive lights (e.g. no directional lights are available
                        // and there's a shadow map that only accepts dir lights)
                        while( !shadowNode->isShadowMapIdxActive( shadowMapTexIdx ) )
                            ++shadowMapTexIdx;

                        const Ogre::ShadowTextureDefinition *shadowTexDef =
                            shadowNodeDef->getShadowTextureDefinition( shadowMapTexIdx );

                        propName.resize( basePropNameSize );
                        propName.a( (uint32)i );  // hlms_shadowmap0

                        const size_t basePropSize = propName.size();

                        setProperty( kNoTid, propName.c_str(),
                                     static_cast<int32>( shadowNode->getIndexToContiguousShadowMapTex(
                                         shadowMapTexIdx ) ) );

                        if( ( shadowTexDef->uvOffset != Vector2::ZERO ||
                              shadowTexDef->uvLength != Vector2::UNIT_SCALE ) ||
                            useStaticBranchShadowMapLights )
                        {
                            propName.resize( basePropSize );
                            propName.a( "_uvs_fulltex" );
                            setProperty( kNoTid, propName.c_str(), 1 );
                        }

                        propName.resize( basePropSize );
                        propName.a( "_uv_min_x" );
                        setProperty( kNoTid, propName.c_str(),
                                     bit_cast<int32_t>( (float)shadowTexDef->uvOffset.x ) );

                        propName.resize( basePropSize );
                        propName.a( "_uv_min_y" );
                        setProperty( kNoTid, propName.c_str(),
                                     bit_cast<int32_t>( (float)shadowTexDef->uvOffset.y ) );

                        const Vector2 uvMax = shadowTexDef->uvOffset + shadowTexDef->uvLength;
                        propName.resize( basePropSize );
                        propName.a( "_uv_max_x" );
                        setProperty( kNoTid, propName.c_str(), bit_cast<int32_t>( (float)uvMax.x ) );

                        propName.resize( basePropSize );
                        propName.a( "_uv_max_y" );
                        setProperty( kNoTid, propName.c_str(), bit_cast<int32_t>( (float)uvMax.y ) );

                        propName.resize( basePropSize );
                        propName.a( "_array_idx" );
                        setProperty( kNoTid, propName.c_str(), shadowTexDef->arrayIdx );

                        const Light *light = shadowNode->getLightAssociatedWith( shadowMapTexIdx );
                        if( useStaticBranchShadowMapLights )
                        {
                            propName.resize( basePropSize );
                            propName.a( "_uv_length_x" );
                            setProperty( kNoTid, propName.c_str(),
                                         bit_cast<int32_t>( (float)shadowTexDef->uvLength.x ) );

                            propName.resize( basePropSize );
                            propName.a( "_uv_length_y" );
                            setProperty( kNoTid, propName.c_str(),
                                         bit_cast<int32_t>( (float)shadowTexDef->uvLength.y ) );

                            if( light->getType() == Light::LT_DIRECTIONAL )
                            {
                                propName.resize( basePropSize );
                                propName.a( "_is_directional_light" );
                                setProperty( kNoTid, propName.c_str(), 1 );
                            }
                            else if( light->getType() == Light::LT_POINT )
                            {
                                ++mRealShadowMapPointLights;
                            }
                            else if( light->getType() == Light::LT_SPOTLIGHT )
                            {
                                ++mRealShadowMapSpotLights;
                            }
                        }
                        else
                        {
                            if( light->getType() == Light::LT_DIRECTIONAL )
                            {
                                propName.resize( basePropSize );
                                propName.a( "_is_directional_light" );
                                setProperty( kNoTid, propName.c_str(), 1 );
                            }
                            else if( light->getType() == Light::LT_POINT )
                            {
                                propName.resize( basePropSize );
                                propName.a( "_is_point_light" );
                                setProperty( kNoTid, propName.c_str(), 1 );

                                propName.resize( basePropSize );
                                propName.a( "_uv_length_x" );
                                setProperty( kNoTid, propName.c_str(),
                                             bit_cast<int32_t>( (float)shadowTexDef->uvLength.x ) );

                                propName.resize( basePropSize );
                                propName.a( "_uv_length_y" );
                                setProperty( kNoTid, propName.c_str(),
                                             bit_cast<int32_t>( (float)shadowTexDef->uvLength.y ) );
                            }
                            else if( light->getType() == Light::LT_SPOTLIGHT )
                            {
                                propName.resize( basePropSize );
                                propName.a( "_is_spot" );
                                setProperty( kNoTid, propName.c_str(), 1 );
                            }
                        }

                        ++shadowMapTexIdx;
                    }
                }

                int usesDepthTextures = -1;

                const size_t numShadowMapTextures = contiguousShadowMapTex.size();
                for( size_t i = 0; i < numShadowMapTextures; ++i )
                {
                    bool missmatch = false;

                    if( PixelFormatGpuUtils::isDepth( contiguousShadowMapTex[i]->getPixelFormat() ) )
                    {
                        missmatch = usesDepthTextures == 0;
                        usesDepthTextures = 1;
                    }
                    else
                    {
                        missmatch = usesDepthTextures == 1;
                        usesDepthTextures = 0;
                    }

                    if( missmatch )
                    {
                        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                                     "Mixing depth textures with non-depth textures for "
                                     "shadow mapping is not supported. Either all of "
                                     "them are depth textures, or none of them are.\n"
                                     "Shadow Node: '" +
                                         shadowNode->getName().getFriendlyText() + "'",
                                     "Hlms::preparePassHash" );
                    }
                }

                if( usesDepthTextures == -1 )
                    usesDepthTextures = 0;

                setProperty( kNoTid, HlmsBaseProp::ShadowUsesDepthTexture, usesDepthTextures );
            }

            if( pass && pass->getType() == PASS_SCENE )
            {
                OGRE_ASSERT_HIGH(
                    dynamic_cast<const CompositorPassSceneDef *>( pass->getDefinition() ) );
                const CompositorPassSceneDef *passSceneDef =
                    static_cast<const CompositorPassSceneDef *>( pass->getDefinition() );
                if( passSceneDef->mUvBakingSet != 0xFF )
                {
                    bForceCullNone = true;
                    setProperty( kNoTid, HlmsBaseProp::UseUvBaking, 1 );
                    setProperty( kNoTid, HlmsBaseProp::UvBaking, passSceneDef->mUvBakingSet );
                    if( passSceneDef->mBakeLightingOnly )
                        setProperty( kNoTid, HlmsBaseProp::BakeLightingOnly, 1 );
                }

                if( passSceneDef->mInstancedStereo )
                    setProperty( kNoTid, HlmsBaseProp::InstancedStereo, 1 );

                if( passSceneDef->mGenNormalsGBuf )
                    setProperty( kNoTid, HlmsBaseProp::GenNormalsGBuf, 1 );
            }

            ForwardPlusBase *forwardPlus = sceneManager->_getActivePassForwardPlus();
            if( forwardPlus )
                forwardPlus->setHlmsPassProperties( kNoTid, this );

            if( mShaderFileExt == ".glsl" )
            {
                // Actually the problem is not texture flipping, but origin. In D3D11,
                // we need to always flip because origin is different, but it's consistent
                // between texture and render window. In GL, RenderWindows don't need
                // to flip, but textures do.
                const RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();
                setProperty( kNoTid, HlmsBaseProp::ForwardPlusFlipY,
                             renderPassDesc->requiresTextureFlipping() );
            }

            int32 numLightsPerType[Light::NUM_LIGHT_TYPES];
            int32 numAreaApproxLightsWithMask = 0;
            memset( numLightsPerType, 0, sizeof( numLightsPerType ) );

            int32 shadowCasterDirectional = 0;

            if( mLightGatheringMode == LightGatherForwardPlus )
            {
                if( shadowNode )
                {
                    // Gather shadow casting lights, regardless of their type.
                    const LightClosestArray &lights = shadowNode->getShadowCastingLights();
                    LightClosestArray::const_iterator itor = lights.begin();
                    LightClosestArray::const_iterator endt = lights.end();
                    while( itor != endt )
                    {
                        if( itor->light )
                        {
                            if( itor->light->getType() == Light::LT_DIRECTIONAL )
                                ++shadowCasterDirectional;
                            ++numLightsPerType[itor->light->getType()];
                        }
                        ++itor;
                    }
                }

                // Always gather directional & area lights.
                numLightsPerType[Light::LT_DIRECTIONAL] = 0;
                numLightsPerType[Light::LT_AREA_APPROX] = 0;
                numLightsPerType[Light::LT_AREA_LTC] = 0;
                {
                    mAreaLightsGlobalLightListStart = std::numeric_limits<uint32>::max();
                    const LightListInfo &globalLightList = sceneManager->getGlobalLightList();
                    LightArray::const_iterator begin = globalLightList.lights.begin();
                    LightArray::const_iterator itor = begin;
                    LightArray::const_iterator endt = globalLightList.lights.end();

                    while( itor != endt )
                    {
                        const Light::LightTypes lightType = ( *itor )->getType();
                        if( lightType == Light::LT_DIRECTIONAL )
                            ++numLightsPerType[Light::LT_DIRECTIONAL];
                        else if( lightType == Light::LT_AREA_APPROX )
                        {
                            mAreaLightsGlobalLightListStart = std::min<uint32>(
                                mAreaLightsGlobalLightListStart, static_cast<uint32>( itor - begin ) );
                            ++numLightsPerType[Light::LT_AREA_APPROX];
                            if( ( *itor )->mTextureLightMaskIdx != std::numeric_limits<uint16>::max() )
                                ++numAreaApproxLightsWithMask;
                        }
                        else if( lightType == Light::LT_AREA_LTC )
                        {
                            mAreaLightsGlobalLightListStart = std::min<uint32>(
                                mAreaLightsGlobalLightListStart, static_cast<uint32>( itor - begin ) );
                            ++numLightsPerType[Light::LT_AREA_LTC];
                        }
                        ++itor;
                    }
                }

                mRealNumDirectionalLights =
                    static_cast<uint32>( numLightsPerType[Light::LT_DIRECTIONAL] );

                if( mNumLightsLimit > 0 &&
                    numLightsPerType[Light::LT_DIRECTIONAL] > shadowCasterDirectional )
                {
                    numLightsPerType[Light::LT_DIRECTIONAL] = shadowCasterDirectional + mNumLightsLimit;

                    setProperty( kNoTid, HlmsBaseProp::StaticBranchLights, 1 );
                }
            }
            else if( mLightGatheringMode == LightGatherForward )
            {
                if( shadowNode )
                {
                    // Gather shadow casting *directional* lights.
                    const LightClosestArray &lights = shadowNode->getShadowCastingLights();
                    LightClosestArray::const_iterator itor = lights.begin();
                    LightClosestArray::const_iterator endt = lights.end();
                    while( itor != endt )
                    {
                        if( itor->light && itor->light->getType() == Light::LT_DIRECTIONAL )
                            ++shadowCasterDirectional;
                        ++itor;
                    }
                }

                // Gather all lights.
                const LightListInfo &globalLightList = sceneManager->getGlobalLightList();
                LightArray::const_iterator itor = globalLightList.lights.begin();
                LightArray::const_iterator endt = globalLightList.lights.end();

                size_t numTotalLights = 0;

                while( itor != endt && numTotalLights < mNumLightsLimit )
                {
                    ++numLightsPerType[( *itor )->getType()];
                    ++numTotalLights;
                    ++itor;
                }
            }

            if( !useStaticBranchShadowMapLights && numLightsPerType[Light::LT_POINT] &&  //
                !numLightsPerType[Light::LT_DIRECTIONAL] &&                              //
                !numLightsPerType[Light::LT_SPOTLIGHT] )
            {
                setProperty( kNoTid, HlmsBaseProp::AllPointLights, 1 );
            }

            numLightsPerType[Light::LT_POINT] += numLightsPerType[Light::LT_DIRECTIONAL];
            numLightsPerType[Light::LT_SPOTLIGHT] += numLightsPerType[Light::LT_POINT];

            // We need to limit the number of area lights before and after rounding
            {
                // Approx area lights
                numLightsPerType[Light::LT_AREA_APPROX] = std::min<int32>(
                    numLightsPerType[Light::LT_AREA_APPROX], mNumAreaApproxLightsLimit );
                mRealNumAreaApproxLights =
                    static_cast<uint32>( numLightsPerType[Light::LT_AREA_APPROX] );
                mRealNumAreaApproxLightsWithMask = static_cast<uint32>( numAreaApproxLightsWithMask );
                numLightsPerType[Light::LT_AREA_APPROX] = std::min<int32>(
                    numLightsPerType[Light::LT_AREA_APPROX], mNumAreaApproxLightsLimit );
            }
            {
                // LTC area lights
                numLightsPerType[Light::LT_AREA_LTC] =
                    std::min<int32>( numLightsPerType[Light::LT_AREA_LTC], mNumAreaLtcLightsLimit );
                mRealNumAreaLtcLights = static_cast<uint32>( numLightsPerType[Light::LT_AREA_LTC] );
                numLightsPerType[Light::LT_AREA_LTC] =
                    std::min<int32>( numLightsPerType[Light::LT_AREA_LTC], mNumAreaLtcLightsLimit );
            }

            // The value is cummulative for each type (order: Directional, point, spot)
            setProperty( kNoTid, HlmsBaseProp::LightsDirectional, shadowCasterDirectional );
            setProperty( kNoTid, HlmsBaseProp::LightsDirNonCaster,
                         numLightsPerType[Light::LT_DIRECTIONAL] );
            setProperty( kNoTid, HlmsBaseProp::LightsPoint, numLightsPerType[Light::LT_POINT] );
            setProperty( kNoTid, HlmsBaseProp::LightsSpot, numLightsPerType[Light::LT_SPOTLIGHT] );
            if( numLightsPerType[Light::LT_AREA_APPROX] > 0 )
            {
                setProperty( kNoTid, HlmsBaseProp::LightsAreaApprox, mNumAreaApproxLightsLimit );
            }
            if( numLightsPerType[Light::LT_AREA_LTC] > 0 )
                setProperty( kNoTid, HlmsBaseProp::LightsAreaLtc, mNumAreaLtcLightsLimit );
            if( numAreaApproxLightsWithMask > 0 )
                setProperty( kNoTid, HlmsBaseProp::LightsAreaTexMask, 1 );

            if( shadowNode )
            {
                // Map shadowMapIdx -> light0Buf.lights[i] which is surprisingly hard.
                // This is needed by Normal Offset Mapping
                //
                // We couldn't do it up until now because we didn't know the light counts
                char tmpBuffer[64];
                LwString propName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

                propName = "hlms_shadowmap";
                const size_t basePropNameSize = propName.size();

                uint32 shadowMapIdx = 0u;
                int32 shadowMapLightIdx = 0;

                {
                    // Deal with first directional light (which may be PSSM)
                    size_t numFirstDirSplits = numPssmSplits;
                    if( numFirstDirSplits == 0u )
                        numFirstDirSplits = shadowCasterDirectional > 0 ? 1 : 0;

                    for( size_t i = 0u; i < numFirstDirSplits; ++i )
                    {
                        propName.resize( basePropNameSize );
                        propName.a( shadowMapIdx, "_light_idx" );  // hlms_shadowmap0_light_idx
                        setProperty( kNoTid, propName.c_str(), shadowMapLightIdx );
                        ++shadowMapIdx;
                    }

                    if( numFirstDirSplits > 0u )
                        ++shadowMapLightIdx;
                }
                {
                    // Deal with rest of caster directional lights
                    for( int32 i = 1; i < shadowCasterDirectional; ++i )
                    {
                        propName.resize( basePropNameSize );
                        propName.a( shadowMapIdx, "_light_idx" );  // hlms_shadowmap0_light_idx
                        setProperty( kNoTid, propName.c_str(), shadowMapLightIdx );
                        ++shadowMapIdx;
                        ++shadowMapLightIdx;
                    }
                }

                if( useStaticBranchShadowMapLights )
                {
                    setProperty( kNoTid, HlmsBaseProp::LightsPoint, 0 );
                }
                else
                {
                    // Deal with rest of non-caster directional lights
                    shadowMapLightIdx +=
                        numLightsPerType[Light::LT_DIRECTIONAL] - shadowCasterDirectional;

                    // Deal with the rest of the casting lights (point & spot)
                    const int32 numLights = numLightsPerType[Light::LT_SPOTLIGHT];
                    for( int32 i = numLightsPerType[Light::LT_DIRECTIONAL]; i < numLights; ++i )
                    {
                        propName.resize( basePropNameSize );
                        propName.a( shadowMapIdx, "_light_idx" );  // hlms_shadowmap0_light_idx
                        setProperty( kNoTid, propName.c_str(), shadowMapLightIdx );
                        ++shadowMapIdx;
                        ++shadowMapLightIdx;
                    }
                }
            }
        }
        else
        {
            setProperty( kNoTid, HlmsBaseProp::ShadowCaster, 1 );

            if( pass )
            {
                const uint32 shadowMapIdx = pass->getDefinition()->mShadowMapIdx;
                const Light *light = shadowNode->getLightAssociatedWith( shadowMapIdx );
                if( light->getType() == Light::LT_DIRECTIONAL )
                    setProperty( kNoTid, HlmsBaseProp::ShadowCasterDirectional, 1 );
                else if( light->getType() == Light::LT_POINT )
                    setProperty( kNoTid, HlmsBaseProp::ShadowCasterPoint, 1 );
            }

            setProperty( kNoTid, HlmsBaseProp::DualParaboloidMapping, dualParaboloid );

            setProperty( kNoTid, HlmsBaseProp::Forward3D, 0 );
            setProperty( kNoTid, HlmsBaseProp::NumShadowMapLights, 0 );
            setProperty( kNoTid, HlmsBaseProp::NumShadowMapTextures, 0 );
            setProperty( kNoTid, HlmsBaseProp::PssmSplits, 0 );
            setProperty( kNoTid, HlmsBaseProp::LightsDirectional, 0 );
            setProperty( kNoTid, HlmsBaseProp::LightsDirNonCaster, 0 );
            setProperty( kNoTid, HlmsBaseProp::LightsPoint, 0 );
            setProperty( kNoTid, HlmsBaseProp::LightsSpot, 0 );
            setProperty( kNoTid, HlmsBaseProp::LightsAreaApprox, 0 );

            const RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();

            setProperty( kNoTid, HlmsBaseProp::ShadowUsesDepthTexture,
                         ( renderPassDesc->getNumColourEntries() > 0 ) ? 0 : 1 );
        }

        const Camera *camera = sceneManager->getCamerasInProgress().renderingCamera;
        if( camera && camera->isReflected() )
        {
            setProperty( kNoTid, HlmsBaseProp::PsoClipDistances, 1 );
            setProperty( kNoTid, HlmsBaseProp::GlobalClipPlanes, 1 );
            // some Android devices(e.g. Mali-G77, Google Pixel 7 Pro) do not support user clip planes
            if( !mRenderSystem->getCapabilities()->hasCapability( RSC_USER_CLIP_PLANES ) )
                setProperty( kNoTid, HlmsBaseProp::EmulateClipDistances, 1 );
        }

        const RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();
        setProperty( kNoTid, HlmsBaseProp::RenderDepthOnly,
                     ( renderPassDesc->getNumColourEntries() > 0 ) ? 0 : 1 );

        if( sceneManager->getCurrentPrePassMode() == PrePassCreate )
        {
            setProperty( kNoTid, HlmsBaseProp::PrePass, 1 );
            setProperty( kNoTid, HlmsBaseProp::GenNormalsGBuf, 1 );
        }
        else if( sceneManager->getCurrentPrePassMode() == PrePassUse )
        {
            setProperty( kNoTid, HlmsBaseProp::UsePrePass, 1 );
            setProperty( kNoTid, HlmsBaseProp::VPos, 1 );

            setProperty( kNoTid, HlmsBaseProp::ScreenPosInt, 1 );

            {
                const TextureGpuVec &prePassTextures = sceneManager->getCurrentPrePassTextures();
                assert( !prePassTextures.empty() );
                if( prePassTextures[0]->isMultisample() )
                {
                    setProperty( kNoTid, HlmsBaseProp::UsePrePassMsaa,
                                 prePassTextures[0]->getSampleDescription().getColourSamples() );
                }
            }

            if( sceneManager->getCurrentSsrTexture() != 0 )
                setProperty( kNoTid, HlmsBaseProp::UseSsr, 1 );
        }

        if( pass && pass->getAnyTargetTexture() )
        {
            setProperty( kNoTid, HlmsBaseProp::MsaaSamples,
                         pass->getAnyTargetTexture()->getSampleDescription().getColourSamples() );
        }

        if( sceneManager->getCurrentRefractionsTexture() != 0 )
        {
            setProperty( kNoTid, HlmsBaseProp::VPos, 1 );
            setProperty( kNoTid, HlmsBaseProp::ScreenPosInt, 1 );
            setProperty( kNoTid, HlmsBaseProp::SsRefractionsAvailable, 1 );
        }

        mListener->preparePassHash( shadowNode, casterPass, dualParaboloid, sceneManager, this );

        PassCache passCache;
        passCache.passPso = getPassPsoForScene( sceneManager, bForceCullNone );
        passCache.properties = mT[kNoTid].setProperties;

        assert( mPassCache.size() <= HlmsBits::PassMask &&
                "Too many passes combinations, we'll overflow the bits assigned in the hash!" );
        PassCacheVec::iterator it = std::find( mPassCache.begin(), mPassCache.end(), passCache );
        if( it == mPassCache.end() )
        {
            mPassCache.push_back( passCache );
            it = mPassCache.end() - 1;
        }

        const uint32 hash = static_cast<uint32>( it - mPassCache.begin() ) << HlmsBits::PassShift;

        HlmsCache retVal( hash, mType, HLMS_CACHE_FLAGS_NONE, HlmsPso() );
        retVal.setProperties = mT[kNoTid].setProperties;
        retVal.pso.pass = passCache.passPso;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    HlmsPassPso Hlms::getPassPsoForScene( SceneManager *sceneManager, const bool bForceCullNone )
    {
        const RenderPassDescriptor *renderPassDesc = mRenderSystem->getCurrentPassDescriptor();

        HlmsPassPso passPso;

        uint32 strongMacroblockBits = getProperty(kNoTid, HlmsPsoProp::StrongMacroblockBits);

        // Needed so that memcmp in HlmsPassPso::operator == works correctly
        silent_memset( &passPso, 0, sizeof( HlmsPassPso ) );

        passPso.stencilParams = mRenderSystem->getStencilBufferParams();

        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            RenderPassColourTarget passColourTarget = renderPassDesc->mColour[i];

            if( passColourTarget.texture )
            {
                passPso.colourFormat[i] = renderPassDesc->mColour[i].texture->getPixelFormat();
                passPso.sampleDescription = renderPassDesc->mColour[i].texture->getSampleDescription();
            }
            else
                passPso.colourFormat[i] = PFG_NULL;

            if( passColourTarget.resolveTexture )
                passPso.resolveColourFormat[i] = passColourTarget.resolveTexture->getPixelFormat();
            else
                passPso.resolveColourFormat[i] = PFG_NULL;
        }

        passPso.depthFormat = PFG_NULL;
        if( renderPassDesc->mDepth.texture )
        {
            passPso.depthFormat = renderPassDesc->mDepth.texture->getPixelFormat();
            passPso.sampleDescription = renderPassDesc->mDepth.texture->getSampleDescription();
        }
        else
        {
            strongMacroblockBits |= HlmsPassPso::DepthCheckDisabled;
        }

        passPso.adapterId = 1;  // TODO: Ask RenderSystem current adapter ID.

        if( sceneManager->getCurrentPrePassMode() == PrePassUse )
            strongMacroblockBits |= HlmsPassPso::DepthWriteDisabled;

        if( sceneManager->getCamerasInProgress().renderingCamera->getNeedsDepthClamp() )
            strongMacroblockBits |= HlmsPassPso::DepthClampEnabled;

        if( bForceCullNone )
            passPso.strongMacroblockBits |= HlmsPassPso::ForceCullNone;

        const bool invertVertexWinding = mRenderSystem->getInvertVertexWinding();

        if( ( renderPassDesc->requiresTextureFlipping() && !invertVertexWinding ) ||
            ( !renderPassDesc->requiresTextureFlipping() && invertVertexWinding ) )
        {
            strongMacroblockBits |= HlmsPassPso::InvertCullingMode;
        }

        setProperty(kNoTid, HlmsPsoProp::StrongMacroblockBits, strongMacroblockBits);
        return passPso;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setTextureReg( size_t tid, ShaderType shaderType, const char *texName, int32 texUnit,
                              int32 numTexUnits )
    {
        OGRE_ASSERT_MEDIUM( numTexUnits < 16 );

        const uint32 startIdx = static_cast<uint32>( mT[tid].textureNameStrings.size() );
        char const *copyName = texName;
        while( *copyName != '\0' )
            mT[tid].textureNameStrings.push_back( *copyName++ );
        mT[tid].textureNameStrings.push_back( '\0' );
        mT[tid].textureRegs[shaderType].push_back( TextureRegs( startIdx, texUnit, numTexUnits ) );

        setProperty( tid, texName, texUnit );
    }
    //-----------------------------------------------------------------------------------
    void Hlms::applyTextureRegisters( const HlmsCache *psoEntry, const size_t tid )
    {
        if( mShaderProfile != "glsl" )
            return;  // D3D embeds the texture slots in the shader.

        GpuProgramPtr shaders[NumShaderTypes];
        shaders[VertexShader] = psoEntry->pso.vertexShader;
        shaders[PixelShader] = psoEntry->pso.pixelShader;
        shaders[GeometryShader] = psoEntry->pso.geometryShader;
        shaders[HullShader] = psoEntry->pso.tesselationHullShader;
        shaders[DomainShader] = psoEntry->pso.tesselationDomainShader;

        String paramName;
        for( size_t i = 0; i < NumShaderTypes; ++i )
        {
            if( shaders[i] )
            {
                GpuProgramParametersSharedPtr params = shaders[i]->getDefaultParameters();

                int texUnits[16];

                TextureRegsVec::const_iterator itor = mT[tid].textureRegs[i].begin();
                TextureRegsVec::const_iterator endt = mT[tid].textureRegs[i].end();

                while( itor != endt )
                {
                    const char *paramNameC = &mT[tid].textureNameStrings[itor->strNameIdxStart];
                    paramName = paramNameC;

                    const int texUnitStart = itor->texUnit;
                    const int numTexUnits = itor->numTexUnits;
                    for( int j = 0u; j < numTexUnits; ++j )
                        texUnits[j] = texUnitStart + j;
                    params->setNamedConstant( paramName, texUnits, static_cast<size_t>( numTexUnits ),
                                              1u );
                    ++itor;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache *Hlms::getMaterial( HlmsCache const *lastReturnedValue,        //
                                        const HlmsCache &passCache,                //
                                        const QueuedRenderable &queuedRenderable,  //
                                        bool casterPass,                           //
                                        ParallelHlmsCompileQueue *parallelQueue )
    {
        uint32 finalHash;
        uint32 hash[2];
        hash[0] = casterPass ? queuedRenderable.renderable->getHlmsCasterHash()
                             : queuedRenderable.renderable->getHlmsHash();
        hash[1] = passCache.hash;

        // If this assert triggers, we've created too many shader variations (or bug)
        OGRE_ASSERT_LOW( ( hash[0] >> HlmsBits::RenderableShift ) <= HlmsBits::RendarebleHlmsTypeMask &&
                         "Too many material / meshes variations" );
        OGRE_ASSERT_LOW( ( hash[1] >> HlmsBits::PassShift ) <= HlmsBits::PassMask &&
                         "Should never happen (we assert in preparePassHash)" );

        finalHash = hash[0] | hash[1];

        if( lastReturnedValue->hash != finalHash )
        {
            lastReturnedValue = this->getShaderCache( finalHash );

            if( !lastReturnedValue )
            {
                // Low level is a special case because it doesn't (yet?) support parallel compilation
                if( !parallelQueue || mType == HLMS_LOW_LEVEL )
                {
                    lastReturnedValue = createShaderCacheEntry(
                        hash[0], passCache, finalHash, queuedRenderable, nullptr, UINT64_MAX, kNoTid );
                }
                else
                {
                    // Create the entry now, but we'll fill it from a worker thread
                    HlmsCache *stubEntry = addStubShaderCache( finalHash );
                    lastReturnedValue = stubEntry;

                    parallelQueue->pushRequest(
                        { &passCache, stubEntry, queuedRenderable, hash[0], finalHash } );
                }
            }
            else if( lastReturnedValue->flags == HLMS_CACHE_FLAGS_COMPILATION_REQUIRED )
            {
                // Stub entry was created for previous frame, but compilation was skipped
                // due to exhausted time budget, and attempt should be repeated
                HlmsCache *stubEntry = const_cast<HlmsCache *>( lastReturnedValue );

                parallelQueue->pushRequest(
                    { &passCache, stubEntry, queuedRenderable, hash[0], finalHash } );
            }
        }

        return lastReturnedValue;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::compileStubEntry( const HlmsCache &passCache, HlmsCache *reservedStubEntry,
                                 uint64 deadline, QueuedRenderable queuedRenderable,
                                 uint32 renderableHash, uint32 finalHash, size_t tid )
    {
        createShaderCacheEntry( renderableHash, passCache, finalHash, queuedRenderable,
                                reservedStubEntry, deadline, tid );
    }
    //-----------------------------------------------------------------------------------
    uint32 Hlms::getMaterialSerial01( uint32 lastReturnedValue, const HlmsCache &passCache,
                                      const size_t passCacheIdx,
                                      const QueuedRenderable &queuedRenderable, bool casterPass,
                                      ParallelHlmsCompileQueue &parallelQueue )
    {
        uint32 finalHash;
        uint32 hash[2];
        hash[0] = casterPass ? queuedRenderable.renderable->getHlmsCasterHash()
                             : queuedRenderable.renderable->getHlmsHash();
        hash[1] = passCache.hash;

        // If this assert triggers, we've created too many shader variations (or bug)
        OGRE_ASSERT_LOW( ( hash[0] >> HlmsBits::RenderableShift ) <= HlmsBits::RendarebleHlmsTypeMask &&
                         "Too many material / meshes variations" );
        OGRE_ASSERT_LOW( ( hash[1] >> HlmsBits::PassShift ) <= HlmsBits::PassMask &&
                         "Should never happen (we assert in preparePassHash)" );

        finalHash = hash[0] | hash[1];

        if( lastReturnedValue != finalHash )
        {
            HlmsCache const *shaderCache = this->getShaderCache( finalHash );

            if( !shaderCache )
            {
                // Low level is a special case because it doesn't (yet?) support parallel compilation
                if( mType != HLMS_LOW_LEVEL )
                {
                    // Create the entry now, but we'll fill it from a worker thread
                    HlmsCache *stubEntry = addStubShaderCache( finalHash );
                    parallelQueue.pushWarmUpRequest(
                        { reinterpret_cast<const HlmsCache *>( passCacheIdx ), stubEntry,
                          queuedRenderable, hash[0], finalHash } );
                }
            }
        }

        lastReturnedValue = finalHash;

        return lastReturnedValue;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setDebugOutputPath( bool enableDebugOutput, bool outputProperties, const String &path )
    {
        mDebugOutput = enableDebugOutput;
        mDebugOutputProperties = outputProperties;
        mOutputPath = path;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::setListener( HlmsListener *listener )
    {
        if( !listener )
            mListener = &c_defaultListener;
        else
            mListener = listener;
    }
    //-----------------------------------------------------------------------------------
    HlmsListener *Hlms::getListener() const { return mListener == &c_defaultListener ? 0 : mListener; }
    //-----------------------------------------------------------------------------------
    void Hlms::_clearShaderCache() { clearShaderCache(); }
    //-----------------------------------------------------------------------------------
    void Hlms::_addDatablockCustomPieceFile( const String &filename, const String &resourceGroup )
    {
        const int32 filenameHash = static_cast<int32>( IdString( filename ).getU32Value() );
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHash );

        const DataStreamPtr stream =
            ResourceGroupManager::getSingleton().openResource( filename, resourceGroup );

        const String sourceCode = stream->getAsString();

        if( itor == mDatablockCustomPieceFiles.end() )
        {
            mDatablockCustomPieceFiles.insert(
                { filenameHash, { filename, resourceGroup, sourceCode } } );
        }
        else if( itor->second.sourceCode != sourceCode )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Calling setCustomPieceFile and/or setCustomPieceCodeFromMemory twice with "
                         "same filename '" +
                             filename +
                             "' but different content.\n"
                             "Maybe there's an HlmsDiskCache bug? Try deleting the cache.\n"
                             "Maybe there's a file name hash collision?",
                         "Hlms::_addDatablockCustomPieceFile" );
        }
    }
    //-----------------------------------------------------------------------------------
    Hlms::CachedCustomPieceFileStatus Hlms::_addDatablockCustomPieceFile(
        const String &filename, const String &resourceGroup, const uint64 sourceCodeHash[2] )
    {
        const int32 filenameHash = static_cast<int32>( IdString( filename ).getU32Value() );
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHash );

        try
        {
            Hlms::CachedCustomPieceFileStatus retVal = CCPFS_Success;

            const DataStreamPtr stream =
                ResourceGroupManager::getSingleton().openResource( filename, resourceGroup );

            const String sourceCode = stream->getAsString();

            DatablockCustomPieceFile entry{ filename, resourceGroup, sourceCode };

            uint64 currentChecksum[2];
            entry.getCodeChecksum( currentChecksum );
            if( currentChecksum[0] != sourceCodeHash[0] && currentChecksum[1] != sourceCodeHash[1] )
                retVal = CCPFS_OutOfDate;

            if( itor == mDatablockCustomPieceFiles.end() )
            {
                mDatablockCustomPieceFiles.insert( { filenameHash, entry } );
            }
            else if( itor->second.sourceCode != sourceCode )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Calling setCustomPieceFile and/or setCustomPieceCodeFromMemory twice with "
                             "same filename '" +
                                 filename +
                                 "' but different content.\n"
                                 "Maybe there's an HlmsDiskCache bug? Try deleting the cache.\n"
                                 "Maybe there's a file name hash collision?",
                             "Hlms::_addDatablockCustomPieceFile" );
            }

            return retVal;
        }
        catch( FileNotFoundException &e )
        {
            LogManager::getSingleton().logMessage( e.getFullDescription(), LML_CRITICAL );
            return CCPFS_CriticalError;
        }
    }
    //-----------------------------------------------------------------------------------
    void Hlms::_addDatablockCustomPieceFileFromMemory( const String &filename, const String &sourceCode )
    {
        const int32 filenameHash = static_cast<int32>( IdString( filename ).getU32Value() );
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHash );

        if( itor == mDatablockCustomPieceFiles.end() )
        {
            mDatablockCustomPieceFiles.insert( { filenameHash, { filename, "", sourceCode } } );
        }
        else if( itor->second.sourceCode != sourceCode )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Calling setCustomPieceFile and/or setCustomPieceCodeFromMemory twice with "
                         "same filename '" +
                             filename +
                             "' but different content.\n"
                             "Maybe there's an HlmsDiskCache bug? Try deleting the cache.\n"
                             "Maybe there's a file name hash collision?",
                         "Hlms::_addDatablockCustomPieceFileFromMemory" );
        }
    }
    //-----------------------------------------------------------------------------------
    bool Hlms::isDatablockCustomPieceFileCacheable( int32 filenameHashId ) const
    {
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHashId );
        if( itor != mDatablockCustomPieceFiles.end() )
            return itor->second.isCacheable();
        return false;
    }
    //-----------------------------------------------------------------------------------
    const String &Hlms::getDatablockCustomPieceFileNameStr( int32 filenameHashId ) const
    {
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHashId );
        if( itor != mDatablockCustomPieceFiles.end() )
            return itor->second.filename;
        return BLANKSTRING;
    }
    //-----------------------------------------------------------------------------------
    const Hlms::DatablockCustomPieceFile *Hlms::getDatablockCustomPieceData( int32 filenameHashId ) const
    {
        DatablockCustomPieceFileMap::const_iterator itor =
            mDatablockCustomPieceFiles.find( filenameHashId );
        if( itor != mDatablockCustomPieceFiles.end() )
            return &itor->second;
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void Hlms::_changeRenderSystem( RenderSystem *newRs )
    {
        clearShaderCache();
        mRenderSystem = newRs;

        mShaderProfile = "unset!";
        mShaderFileExt = "unset!";
        mShaderSyntax = "unset!";
        memset( mShaderTargets, 0, sizeof( mShaderTargets ) );

        if( mRenderSystem )
        {
            {
                mFastShaderBuildHack = false;
                const ConfigOptionMap &rsConfigOptions = newRs->getConfigOptions();
                ConfigOptionMap::const_iterator itor = rsConfigOptions.find( "Fast Shader Build Hack" );
                if( itor != rsConfigOptions.end() )
                    mFastShaderBuildHack = StringConverter::parseBool( itor->second.currentValue );
            }

            // Prefer glslvk over hlslvk over glsl
            const String shaderProfiles[6] = { "hlsl", "glsl", "hlslvk", "glslvk", "metal" };
            const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();

            for( size_t i = 0; i < 6; ++i )
            {
                if( capabilities->isShaderProfileSupported( shaderProfiles[i] ) )
                {
                    mShaderProfile = shaderProfiles[i];
                    mShaderSyntax = shaderProfiles[i];
                }
            }

            if( mShaderProfile == "hlsl" || mShaderProfile == "hlslvk" )
            {
                mShaderFileExt = ".hlsl";

                for( size_t i = 0; i < NumShaderTypes; ++i )
                {
                    for( size_t j = 0; j < 5 && !mShaderTargets[i]; ++j )
                    {
                        if( capabilities->isShaderProfileSupported( BestD3DShaderTargets[i][j] ) )
                            mShaderTargets[i] = &BestD3DShaderTargets[i][j];
                    }
                }
            }
            else if( mShaderProfile == "metal" )
            {
                mShaderFileExt = ".metal";
            }
            else
            {
                mShaderFileExt = ".glsl";

                if( mRenderSystem->checkExtension( "GL_AMD_shader_trinary_minmax" ) )
                    mRsSpecificExtensions.push_back( HlmsBaseProp::GlAmdTrinaryMinMax );

                struct Extensions
                {
                    const char *extName;
                    uint32 minGlVersion;
                };

                Extensions extensions[] = {
                    // clang-format off
                    { "GL_ARB_base_instance",               420 },
                    { "GL_ARB_shading_language_420pack",    420 },
                    { "GL_ARB_texture_buffer_range",        430 },
                    // clang-format on
                };

                for( size_t i = 0; i < sizeof( extensions ) / sizeof( extensions[0] ); ++i )
                {
                    if( mRenderSystem->getNativeShadingLanguageVersion() >= extensions[i].minGlVersion ||
                        mRenderSystem->checkExtension( extensions[i].extName ) )
                    {
                        mRsSpecificExtensions.push_back( extensions[i].extName );
                    }
                }
            }

            if( mRenderSystem->getVaoManager()->readOnlyIsTexBuffer() )
                mRsSpecificExtensions.push_back( HlmsBaseProp::ReadOnlyIsTex );

            if( !mDefaultDatablock )
                mDefaultDatablock = createDefaultDatablock();
        }
    }
    //-----------------------------------------------------------------------------------
    unsigned long Hlms::calculateLineCount( const String &buffer, size_t idx )
    {
        String::const_iterator itor = buffer.begin();
        String::const_iterator endt = buffer.begin() + static_cast<ptrdiff_t>( idx );

        unsigned long lineCount = 0;

        while( itor != endt )
        {
            if( *itor == '\n' )
                ++lineCount;
            ++itor;
        }

        return lineCount + 1;
    }
    //-----------------------------------------------------------------------------------
    unsigned long Hlms::calculateLineCount( const SubStringRef &subString )
    {
        return calculateLineCount( subString.getOriginalBuffer(), subString.getStart() );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    inline void Hlms::Expression::swap( Expression &other )
    {
        std::swap( this->result, other.result );
        std::swap( this->negated, other.negated );
        std::swap( this->type, other.type );
        this->children.swap( other.children );
        this->value.swap( other.value );
    }
}  // namespace Ogre

#undef OGRE_HASH128_FUNC
