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

#include "OgreStableHeaders.h"

#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmap.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "OgreRenderSystem.h"

#include "OgreLwString.h"
#include "OgreTextureGpuManager.h"

#include "OgreHlmsManager.h"
#include "OgreRoot.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreLogManager.h"

namespace Ogre
{
    CompositorPassMipmap::CompositorPassMipmap( const CompositorPassMipmapDef *definition,
                                                const RenderTargetViewDef *rtv,
                                                CompositorNode *parentNode ) :
        CompositorPass( definition, parentNode ),
        mWarnedNoAutomipmapsAlready( false ),
        mDefinition( definition )
    {
        initialize( rtv );

        size_t numColourEntries = mRenderPassDesc->getNumColourEntries();
        for( size_t i = 0; i < numColourEntries; ++i )
        {
            if( mRenderPassDesc->mColour[i].resolveTexture )
                mTextures.push_back( mRenderPassDesc->mColour[i].resolveTexture );
            else
            {
                if( mRenderPassDesc->mColour[i].texture->isMultisample() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Cannot generate mipmaps for MSAA textures! Node: " +
                                     mParentNode->getName().getFriendlyText(),
                                 "CompositorPassMipmap::CompositorPassMipmap" );
                }

                if( !mRenderPassDesc->mColour[i].texture->isTexture() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "PASS_MIPMAP can only be used with textures that can "
                                 "be interpreted as textures and as UAVs",
                                 "CompositorPassMipmap::CompositorPassMipmap" );
                }

                mTextures.push_back( mRenderPassDesc->mColour[i].texture );
            }
        }

        if( mDefinition->mMipmapGenerationMethod == CompositorPassMipmapDef::Compute ||
            mDefinition->mMipmapGenerationMethod == CompositorPassMipmapDef::ComputeHQ )
        {
            setupComputeShaders();
        }
    }
    //-----------------------------------------------------------------------------------
    CompositorPassMipmap::~CompositorPassMipmap() { destroyComputeShaders(); }
    //-----------------------------------------------------------------------------------
    void CompositorPassMipmap::destroyComputeShaders( void )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();

        if( !mJobs.empty() )
        {
            assert( dynamic_cast<HlmsCompute *>( mJobs[0]->getCreator() ) );
            HlmsCompute *hlmsCompute = static_cast<HlmsCompute *>( mJobs[0]->getCreator() );

            FastArray<HlmsComputeJob *>::iterator itor = mJobs.begin();
            FastArray<HlmsComputeJob *>::iterator endt = mJobs.end();

            while( itor != endt )
            {
                hlmsCompute->destroyComputeJob( ( *itor )->getName() );
                ++itor;
            }

            mJobs.clear();
        }

        TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
        TextureGpuVec::iterator itor = mTmpTextures.begin();
        TextureGpuVec::iterator endt = mTmpTextures.end();

        while( itor != endt )
            textureManager->destroyTexture( *itor++ );

        mTmpTextures.clear();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassMipmap::setupComputeShaders( void )
    {
        destroyComputeShaders();

        HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
        HlmsCompute *hlmsCompute = hlmsManager->getComputeHlms();

        HlmsComputeJob *blurH = hlmsCompute->findComputeJobNoThrow( "Mipmap/GaussianBlurH" );
        HlmsComputeJob *blurV = hlmsCompute->findComputeJobNoThrow( "Mipmap/GaussianBlurV" );

#if OGRE_NO_JSON
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "To use PASS_MIPMAP with compute shaders, Ogre must be build with JSON support "
                     "and you must include the resource bundled at "
                     "Samples/Media/2.0/scripts/materials/Common",
                     "CompositorPassMipmap::CompositorPassMipmap" );
#endif

        if( !blurH || !blurV )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use PASS_MIPMAP with compute shaders, you must include the resources "
                         "bundled at Samples/Media/2.0/scripts/materials/Common\n"
                         "Could not find Mipmap/GaussianBlurH and Mipmap/GaussianBlurV",
                         "CompositorPassMipmap::CompositorPassMipmap" );
        }

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();

        if( !caps->hasCapability( RSC_COMPUTE_PROGRAM ) )
        {
            LogManager::getSingleton().logMessage(
                "[INFO] Compute Shaders not supported. Using fallback mipmap generation." );
            return;
        }

        setGaussianFilterParams( blurH, mDefinition->mKernelRadius,
                                 mDefinition->mGaussianDeviationFactor );
        setGaussianFilterParams( blurV, mDefinition->mKernelRadius,
                                 mDefinition->mGaussianDeviationFactor );

        TextureGpuVec::iterator itor = mTextures.begin();
        TextureGpuVec::iterator endt = mTextures.end();

        while( itor != endt )
        {
            TextureGpu *texture = *itor;

            if( texture->getNumMipmaps() > 1u )
            {
                if( !texture->isUav() || !texture->isTexture() )
                {
                    OGRE_EXCEPT(
                        Exception::ERR_INVALIDPARAMS,
                        "Texture '" + texture->getNameStr() +
                            "' must be flagged as UAV and Texture in order to be able to generate "
                            "mipmaps using Compute Shaders",
                        "CompositorPassMipmap::CompositorPassMipmap" );
                }

                const String newId =
                    StringConverter::toString( Id::generateNewId<CompositorPassMipmap>() );

                TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
                TextureGpu *tmpTex = textureManager->createTexture(
                    "MipmapTmpTexture " + newId, GpuPageOutStrategy::Discard,
                    TextureFlags::Uav | TextureFlags::DiscardableContent, texture->getTextureType() );
                mTmpTextures.push_back( tmpTex );

                tmpTex->setResolution( texture->getWidth() >> 1u, texture->getHeight(),
                                       texture->getDepth() );
                tmpTex->setPixelFormat( texture->getPixelFormat() );
                tmpTex->setNumMipmaps( texture->getNumMipmaps() );

                tmpTex->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

                const uint8 numMips = texture->getNumMipmaps();
                const PixelFormatGpu pf = texture->getPixelFormat();

                uint32 currWidth = texture->getWidth();
                uint32 currHeight = texture->getHeight();

                for( size_t mip = 0; mip < numMips - 1u; ++mip )
                {
                    const String mipString = StringConverter::toString( mip );
                    HlmsComputeJob *blurH2 =
                        blurH->clone( "Mipmap_BlurH" + newId + " mip " + mipString );
                    HlmsComputeJob *blurV2 =
                        blurV->clone( "Mipmap_BlurV" + newId + " mip " + mipString );

                    ShaderParams::Param paramLodIdx;
                    paramLodIdx.name = "srcLodIdx";
                    ShaderParams::Param paramOutputSize;
                    paramOutputSize.name = "g_f4OutputSize";
                    ShaderParams::Param paramDstLodIdx;
                    paramDstLodIdx.name = "dstLodIdx";

                    ShaderParams *shaderParams = 0;

                    paramLodIdx.setManualValue( (float)mip );
                    paramOutputSize.setManualValue( Vector4( (float)currWidth, (float)currHeight,
                                                             1.0f / currWidth, 1.0f / currHeight ) );
                    paramDstLodIdx.setManualValue( (uint32)mip );

                    shaderParams = &blurH2->getShaderParams( "default" );
                    shaderParams->mParams.push_back( paramLodIdx );
                    shaderParams->mParams.push_back( paramOutputSize );
                    shaderParams->setDirty();
                    shaderParams = &blurH2->getShaderParams( "metal" );
                    shaderParams->mParams.push_back( paramDstLodIdx );
                    shaderParams->setDirty();

                    blurH2->setProperty( "width_with_lod", static_cast<int32>( currWidth ) );
                    blurH2->setProperty( "height_with_lod", static_cast<int32>( currHeight ) );

                    currWidth = std::max( currWidth >> 1u, 1u );
                    paramOutputSize.setManualValue( Vector4( (float)currWidth, (float)currHeight,
                                                             1.0f / currWidth, 1.0f / currHeight ) );
                    paramDstLodIdx.setManualValue( ( uint32 )( mip + 1u ) );

                    shaderParams = &blurV2->getShaderParams( "default" );
                    shaderParams->mParams.push_back( paramLodIdx );
                    shaderParams->mParams.push_back( paramOutputSize );
                    shaderParams->setDirty();
                    shaderParams = &blurV2->getShaderParams( "metal" );
                    shaderParams->mParams.push_back( paramDstLodIdx );
                    shaderParams->setDirty();

                    DescriptorSetTexture2::TextureSlot texSlot(
                        DescriptorSetTexture2::TextureSlot::makeEmpty() );
                    DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                    uavSlot.access = ResourceAccess::Write;
                    uavSlot.textureArrayIndex = 0;
                    uavSlot.pixelFormat = pf;

                    texSlot.texture = texture;
                    uavSlot.texture = tmpTex;
                    uavSlot.mipmapLevel = static_cast<uint8>( mip );
                    blurH2->setTexture( 0, texSlot );
                    blurH2->_setUavTexture( 0, uavSlot );

                    texSlot.texture = tmpTex;
                    uavSlot.texture = texture;
                    uavSlot.mipmapLevel = static_cast<uint8>( mip + 1u );
                    blurV2->setTexture( 0, texSlot );
                    blurV2->_setUavTexture( 0, uavSlot );

                    blurV2->setProperty( "width_with_lod", static_cast<int32>( currWidth ) );
                    blurV2->setProperty( "height_with_lod", static_cast<int32>( currHeight ) );

                    mJobs.push_back( blurH2 );
                    mJobs.push_back( blurV2 );

                    currHeight = std::max( currHeight >> 1u, 1u );
                }
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassMipmap::setGaussianFilterParams( HlmsComputeJob *job, uint8 kernelRadius,
                                                        float gaussianDeviationFactor )
    {
        assert( !( kernelRadius & 0x01 ) && "kernelRadius must be even!" );

        if( job->getProperty( "kernel_radius" ) != kernelRadius )
            job->setProperty( "kernel_radius", kernelRadius );
        ShaderParams &shaderParams = job->getShaderParams( "default" );

        std::vector<float> weights( kernelRadius + 1u );

        const float fKernelRadius = kernelRadius;
        const float gaussianDeviation = fKernelRadius * gaussianDeviationFactor;

        // It's 2.0f if using the approximate filter (sampling between two pixels to
        // get the bilinear interpolated result and cut the number of samples in half)
        const float stepSize = 1.0f;

        // Calculate the weights
        float fWeightSum = 0;
        for( uint32 i = 0; i < kernelRadius + 1u; ++i )
        {
            const float val = i - fKernelRadius + ( 1.0f - 1.0f / stepSize );
            float fWeight = 1.0f / std::sqrt( 2.0f * Math::PI * gaussianDeviation * gaussianDeviation );
            fWeight *= exp( -( val * val ) / ( 2.0f * gaussianDeviation * gaussianDeviation ) );

            fWeightSum += fWeight;
            weights[i] = fWeight;
        }

        fWeightSum = fWeightSum * 2.0f - weights[kernelRadius];

        // Normalize the weights
        for( uint32 i = 0; i < kernelRadius + 1u; ++i )
            weights[i] /= fWeightSum;

        // Remove shader constants from previous calls (needed in case we've reduced the radius size)
        ShaderParams::ParamVec::iterator itor = shaderParams.mParams.begin();
        ShaderParams::ParamVec::iterator endt = shaderParams.mParams.end();

        while( itor != endt )
        {
            String::size_type pos = itor->name.find( "c_weights[" );

            if( pos != String::npos )
            {
                itor = shaderParams.mParams.erase( itor );
                endt = shaderParams.mParams.end();
            }
            else
            {
                ++itor;
            }
        }

        const bool bIsMetal = job->getCreator()->getShaderProfile() == "metal";

        // Set the shader constants, 16 at a time (since that's the limit of what ManualParam can hold)
        char tmp[32];
        LwString weightsString( LwString::FromEmptyPointer( tmp, sizeof( tmp ) ) );
        const uint32 floatsPerParam = sizeof( ShaderParams::ManualParam().dataBytes ) / sizeof( float );
        for( uint32 i = 0; i < kernelRadius + 1u; i += floatsPerParam )
        {
            weightsString.clear();
            if( bIsMetal )
                weightsString.a( "c_weights[", i, "]" );
            else
                weightsString.a( "c_weights[", ( i >> 2u ), "]" );

            ShaderParams::Param p;
            p.isAutomatic = false;
            p.isDirty = true;
            p.name = weightsString.c_str();
            shaderParams.mParams.push_back( p );
            ShaderParams::Param *param = &shaderParams.mParams.back();

            param->setManualValue( &weights[i], std::min<uint32>( floatsPerParam, weights.size() - i ) );
        }

        shaderParams.setDirty();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassMipmap::execute( const Camera *lodCamera )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        analyzeBarriers();
        executeResourceTransitions();

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        const bool usesCompute = !mJobs.empty();

        if( !usesCompute )
        {
            TextureGpuVec::const_iterator itor = mTextures.begin();
            TextureGpuVec::const_iterator endt = mTextures.end();

            while( itor != endt )
            {
                if( ( *itor )->getNumMipmaps() > 1u )
                {
                    if( ( *itor )->allowsAutoMipmaps() )
                        ( *itor )->_autogenerateMipmaps();
                    else if( !mWarnedNoAutomipmapsAlready )
                    {
                        LogManager::getSingleton().logMessage(
                            "WARNING: generate_mipmaps called for texture " + ( *itor )->getNameStr() +
                            " but it was not created with "
                            "auto mipmap support! If using scripts, add keyword 'automipmaps'. "
                            "If using code, set texture flag 'TextureFlags::AllowAutomipmaps'" );
                        mWarnedNoAutomipmapsAlready = true;
                    }
                }
                ++itor;
            }
        }
        else
        {
            assert( dynamic_cast<HlmsCompute *>( mJobs[0]->getCreator() ) );
            HlmsCompute *hlmsCompute = static_cast<HlmsCompute *>( mJobs[0]->getCreator() );

            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->endRenderPassDescriptor();

            FastArray<HlmsComputeJob *>::iterator itor = mJobs.begin();
            FastArray<HlmsComputeJob *>::iterator endt = mJobs.end();

            while( itor != endt )
            {
                ( *itor )->analyzeBarriers( mResourceTransitions );
                renderSystem->executeResourceTransition( mResourceTransitions );
                hlmsCompute->dispatch( *itor, 0, 0 );
                ++itor;
            }
        }

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassMipmap::analyzeBarriers( void )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->flushPendingAutoResourceLayouts();

        mResourceTransitions.clear();

        // Do not use base class'
        // CompositorPass::analyzeBarriers();

        const bool usesCompute = !mJobs.empty();

        const ResourceLayout::Layout targetLayout =
            usesCompute ? ResourceLayout::Uav : ResourceLayout::MipmapGen;
        const uint8 stage = usesCompute ? ( 1u << GPT_COMPUTE_PROGRAM ) : 0u;

        // Check <anything> -> RT for every RTT in the textures we'll be generating mipmaps.
        TextureGpuVec::const_iterator itTex = mTextures.begin();
        TextureGpuVec::const_iterator enTex = mTextures.end();

        while( itTex != enTex )
        {
            if( ( *itTex )->getNumMipmaps() > 1u )
            {
                if( ( *itTex )->allowsAutoMipmaps() )
                    resolveTransition( *itTex, targetLayout, ResourceAccess::ReadWrite, stage );
            }
            ++itTex;
        }
    }
    //-----------------------------------------------------------------------------------
    bool CompositorPassMipmap::notifyRecreated( const TextureGpu *channel )
    {
        bool usedByUs = CompositorPass::notifyRecreated( channel );

        if( usedByUs )
        {
            mWarnedNoAutomipmapsAlready = false;
            if( mDefinition->mMipmapGenerationMethod == CompositorPassMipmapDef::Compute ||
                mDefinition->mMipmapGenerationMethod == CompositorPassMipmapDef::ComputeHQ )
            {
                setupComputeShaders();
            }
        }

        return usedByUs;
    }
}  // namespace Ogre
