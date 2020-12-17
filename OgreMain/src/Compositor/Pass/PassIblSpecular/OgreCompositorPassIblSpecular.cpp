/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecular.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "OgreRenderSystem.h"

#include "OgreLwString.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"

#include "OgreHlmsManager.h"
#include "OgreRoot.h"

#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreLogManager.h"

namespace Ogre
{
    CompositorPassIblSpecularDef::~CompositorPassIblSpecularDef() {}
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecularDef::setCubemapInput( const String &textureName )
    {
        if( textureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( textureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        mInputTextureName = textureName;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecularDef::setCubemapOutput( const String &textureName )
    {
        if( textureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( textureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        mOutputTextureName = textureName;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassIblSpecular::CompositorPassIblSpecular( const CompositorPassIblSpecularDef *definition,
                                                          const RenderTargetViewDef *rtv,
                                                          CompositorNode *parentNode ) :
        CompositorPass( definition, parentNode ),
        mInputTexture( 0 ),
        mOutputTexture( 0 ),
        mDefinition( definition )
    {
        initialize( rtv );

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool hasCompute = caps->hasCapability( RSC_COMPUTE_PROGRAM );

        mInputTexture = mParentNode->getDefinedTexture( mDefinition->getInputTextureName() );
        mOutputTexture = mParentNode->getDefinedTexture( mDefinition->getOutputTextureName() );

        if( !mInputTexture )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Input texture " + mDefinition->getInputTextureName().getFriendlyText() +
                             " not found! Node: " + mParentNode->getName().getFriendlyText(),
                         "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }
        if( !mOutputTexture )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Output texture " + mDefinition->getOutputTextureName().getFriendlyText() +
                             " not found! Node: " + mParentNode->getName().getFriendlyText(),
                         "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }

        if( mInputTexture == mOutputTexture )
            return;  // Special case for PCC - IBL is not wanted. Nothing to do
        if( mOutputTexture->getNumMipmaps() == 1u )
            return;  // Fast copy / passthrough

        if( mInputTexture->getTextureType() != TextureTypes::TypeCube )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "IblSpecular requires a Cubemap texture as input! Node: " +
                             mParentNode->getName().getFriendlyText(),
                         "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }

        if( mInputTexture->getTextureType() == TextureTypes::TypeCube &&
            mInputTexture->getNumMipmaps() < mOutputTexture->getNumMipmaps() )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "In IblSpecular, Input texture must have at least as many mipmaps as the output "
                "texture! input.mipmaps = " +
                    StringConverter::toString( mInputTexture->getNumMipmaps() ) + "; output.mipmaps = " +
                    StringConverter::toString( mOutputTexture->getNumMipmaps() ) +
                    "; Node: " + mParentNode->getName().getFriendlyText(),
                "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }

        if( !mInputTexture->allowsAutoMipmaps() )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "Input texture for IblSpecular must have been created with AllowAutomipmaps! Node: " +
                    mParentNode->getName().getFriendlyText(),
                "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }
        if( !mOutputTexture->isUav() && !mDefinition->mForceMipmapFallback && hasCompute )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Output texture for IblSpecular must be UAV! Node: " +
                             mParentNode->getName().getFriendlyText(),
                         "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }

        if( !mDefinition->mForceMipmapFallback )
        {
            // Even if hasCompute == false, we must call setupComputeShaders so that we
            // can warn the user the requirements are met. Otherwise it won't raise
            // exceptions until the app gets deployed on a system that does
            // support Compute
            setupComputeShaders();
        }
    }
    //-----------------------------------------------------------------------------------
    CompositorPassIblSpecular::~CompositorPassIblSpecular() { destroyComputeShaders(); }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecular::destroyComputeShaders( void )
    {
        if( !mJobs.empty() )
        {
            assert( dynamic_cast<HlmsCompute *>( mJobs[0]->getCreator() ) );
            HlmsCompute *hlmsCompute = static_cast<HlmsCompute *>( mJobs[0]->getCreator() );

            vector<HlmsComputeJob *>::type::iterator itor = mJobs.begin();
            vector<HlmsComputeJob *>::type::iterator endt = mJobs.end();

            while( itor != endt )
            {
                // renderSystem->_resourceTransitionDestroyed( &itor->resourceTransition );
                hlmsCompute->destroyComputeJob( ( *itor )->getName() );
                ++itor;
            }

            mJobs.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecular::setupComputeShaders( void )
    {
        destroyComputeShaders();

        HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
        HlmsCompute *hlmsCompute = hlmsManager->getComputeHlms();

        HlmsComputeJob *iblSpecular = hlmsCompute->findComputeJobNoThrow( "IblSpecular/Integrate" );

#if OGRE_NO_JSON
        OGRE_EXCEPT(
            Exception::ERR_INVALIDPARAMS,
            "To use PASS_IBL_SPECULAR with compute shaders, Ogre must be build with JSON support "
            "and you must include the resource bundled at "
            "Samples/Media/2.0/scripts/materials/Common",
            "CompositorPassIblSpecular::CompositorPassIblSpecular" );
#endif

        if( !iblSpecular )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "To use PASS_IBL_SPECULAR with compute shaders, you must include the resources "
                         "bundled at Samples/Media/Compute/Algorithms/IBL\n"
                         "Could not find IblSpecular",
                         "CompositorPassIblSpecular::CompositorPassIblSpecular" );
        }

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();

        if( !caps->hasCapability( RSC_COMPUTE_PROGRAM ) )
        {
            LogManager::getSingleton().logMessage(
                "[INFO] Compute Shaders not supported. Using fallback IblSpecular generation." );
            return;
        }

        const bool hasTypedUavLoads = caps->hasCapability( RSC_TYPED_UAV_LOADS );

        const String newId = StringConverter::toString( Id::generateNewId<CompositorPassIblSpecular>() );

        HlmsSamplerblock anisoSamplerblock;
        anisoSamplerblock.setFiltering( TFO_ANISOTROPIC );
        anisoSamplerblock.setAddressingMode( TAM_WRAP );
        anisoSamplerblock.mMaxAnisotropy = 16.0f;

        const uint8 outNumMips = mOutputTexture->getNumMipmaps();

        for( uint8 mip = 0u; mip < outNumMips; ++mip )
        {
            String mipNum = "/mip" + StringConverter::toString( mip );
            HlmsComputeJob *job = iblSpecular->clone( "IblSpecular/Integrate/" + newId + mipNum );

            job->setProperty( "typed_uav_loads", hasTypedUavLoads ? 1 : 0 );

            DescriptorSetTexture2::TextureSlot texSlot(
                DescriptorSetTexture2::TextureSlot::makeEmpty() );
            texSlot.texture = mInputTexture;
            job->setTexture( 0, texSlot, &anisoSamplerblock );

            DescriptorSetUav::TextureSlot uavSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
            uavSlot.texture = mOutputTexture;
            uavSlot.access = hasTypedUavLoads ? ResourceAccess::ReadWrite : ResourceAccess::Write;
            uavSlot.mipmapLevel = mip;
            uavSlot.pixelFormat =
                PixelFormatGpuUtils::getEquivalentLinear( mOutputTexture->getPixelFormat() );
            job->_setUavTexture( 0, uavSlot );

            ShaderParams &shaderParams = job->getShaderParams( "default" );

            ShaderParams::Param *p;

            const float p_convolutionSamplesOffset = 0;
            float p_convolutionSampleCount = mDefinition->mSamplesPerIteration;
            float p_convolutionMaxSamples =
                mDefinition->mNumInitialPasses != std::numeric_limits<uint32>::max()
                    ? ( mDefinition->mSamplesPerIteration * mDefinition->mNumInitialPasses )
                    : mDefinition->mSamplesPerIteration;
            const float p_convolutionRoughness = mip / static_cast<float>( outNumMips - 1u );

            if( !hasTypedUavLoads && p_convolutionSampleCount != p_convolutionMaxSamples )
            {
                p_convolutionSampleCount = mDefinition->mSamplesSingleIterationFallback;
                p_convolutionMaxSamples = mDefinition->mSamplesSingleIterationFallback;
            }

            shaderParams.mParams.push_back( ShaderParams::Param() );
            p = &shaderParams.mParams.back();
            p->name = "params0";
            p->setManualValue( Vector4( p_convolutionSamplesOffset, p_convolutionSampleCount,
                                        p_convolutionMaxSamples, p_convolutionRoughness ) );

            const float p_convolutionMip = mip;
            const float p_environmentScale = 1.0f;

            shaderParams.mParams.push_back( ShaderParams::Param() );
            p = &shaderParams.mParams.back();
            p->name = "params1";
            p->setManualValue( Vector4( p_convolutionMip, p_environmentScale, 0, 0 ) );

            shaderParams.mParams.push_back( ShaderParams::Param() );
            p = &shaderParams.mParams.back();
            p->name = "params2";
            p->setManualValue( Vector4( mInputTexture->getWidth(), mInputTexture->getHeight(),
                                        std::max( mOutputTexture->getWidth() >> mip, 1u ),
                                        std::max( mOutputTexture->getHeight() >> mip, 1u ) ) );

            shaderParams.mParams.push_back( ShaderParams::Param() );
            p = &shaderParams.mParams.back();
            p->name = "iblCorrection";
            p->setManualValue( mDefinition->mIblCorrectionVSH );

            mJobs.push_back( job );
        }

        if( !hasTypedUavLoads && mNumPassesLeft != std::numeric_limits<uint32>::max() )
            mNumPassesLeft = 1u;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecular::execute( const Camera *lodCamera )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;

            vector<HlmsComputeJob *>::const_iterator itor = mJobs.begin();
            vector<HlmsComputeJob *>::const_iterator endt = mJobs.end();

            while( itor != endt )
            {
                ShaderParams &shaderParams = ( *itor )->getShaderParams( "default" );
                ShaderParams::Param &p = shaderParams.mParams.front();
                Vector4 oldParam = p.getManualValue<Vector4>();
                oldParam.x += 1.0f;  // Advance p_convolutionSamplesOffset
                p.setManualValue( oldParam );

                ++itor;
            }

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
            if( mInputTexture != mOutputTexture )
            {
                // If output has no mipmaps, do a fast path copy
                if( mOutputTexture->getNumMipmaps() > 1u )
                    mInputTexture->_autogenerateMipmaps();

                {
                    // Prepare mInputTexture for copying. We have to do it here because analyzeBarriers
                    // prepared mInputTexture for generating mipmaps
                    mResourceTransitions.clear();
                    mBarrierSolver.resolveTransition( mResourceTransitions, mInputTexture,
                                                      ResourceLayout::CopySrc, ResourceAccess::Read,
                                                      0u );
                    RenderSystem *renderSystem = mParentNode->getRenderSystem();
                    renderSystem->executeResourceTransition( mResourceTransitions );
                }

                const uint8 outNumMips = mOutputTexture->getNumMipmaps();

                for( uint8 mip = 0u; mip < outNumMips; ++mip )
                {
                    TextureBox emptyBox = mOutputTexture->getEmptyBox( mip );
                    mInputTexture->copyTo( mOutputTexture, emptyBox, mip, emptyBox, mip, true,
                                           ResourceAccess::Undefined );
                }
            }
        }
        else
        {
            assert( dynamic_cast<HlmsCompute *>( mJobs[0]->getCreator() ) );
            HlmsCompute *hlmsCompute = static_cast<HlmsCompute *>( mJobs[0]->getCreator() );

            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->endRenderPassDescriptor();

            mInputTexture->_autogenerateMipmaps();

            {
                mResourceTransitions.clear();
                mBarrierSolver.resolveTransition( mResourceTransitions, mInputTexture,
                                                  ResourceLayout::Texture, ResourceAccess::Read,
                                                  1u << GPT_COMPUTE_PROGRAM );
                renderSystem->executeResourceTransition( mResourceTransitions );
            }

            vector<HlmsComputeJob *>::type::iterator itor = mJobs.begin();
            vector<HlmsComputeJob *>::type::iterator endt = mJobs.end();

            while( itor != endt )
            {
                hlmsCompute->dispatch( *itor, 0, 0 );

                // Don't issue a barrier for the last one. The compositor will take care of that one.
                /*if( itor != endt - 1u )
                    renderSystem->_executeResourceTransition( &itor->resourceTransition );*/
                ++itor;
            }
        }

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecular::analyzeBarriers( void )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->flushPendingAutoResourceLayouts();

        mResourceTransitions.clear();

        // Do not use base class'
        // CompositorPass::analyzeBarriers();

        const bool usesCompute = !mJobs.empty();

        if( usesCompute )
        {
            // Check <anything> -> MipmapGen for mInputTexture
            resolveTransition( mInputTexture, ResourceLayout::MipmapGen, ResourceAccess::ReadWrite, 0u );

            // Check <anything> -> UAV for mOutputTexture (we need to write to it).
            resolveTransition( mOutputTexture, ResourceLayout::Uav, ResourceAccess::Write,
                               1u << GPT_COMPUTE_PROGRAM );
        }
        else
        {
            if( mInputTexture != mOutputTexture )
            {
                if( mOutputTexture->getNumMipmaps() > 1u )
                {
                    // Check <anything> -> MipmapGen for mInputTexture
                    resolveTransition( mInputTexture, ResourceLayout::MipmapGen,
                                       ResourceAccess::ReadWrite, 0u );
                }

                resolveTransition( mOutputTexture, ResourceLayout::CopyDst, ResourceAccess::Write, 0u );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool CompositorPassIblSpecular::notifyRecreated( const TextureGpu *channel )
    {
        bool usedByUs = CompositorPass::notifyRecreated( channel );

        if( usedByUs )
            setupComputeShaders();

        return usedByUs;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassIblSpecular::resetNumPassesLeft( void )
    {
        vector<HlmsComputeJob *>::const_iterator itor = mJobs.begin();
        vector<HlmsComputeJob *>::const_iterator endt = mJobs.end();

        while( itor != endt )
        {
            ShaderParams &shaderParams = ( *itor )->getShaderParams( "default" );

            // We don't know if shaderParams.mParams exists yet
            if( !shaderParams.mParams.empty() )
            {
                ShaderParams::Param &p = shaderParams.mParams.front();
                Vector4 oldParam = p.getManualValue<Vector4>();
                oldParam.x = 0.0f;
                p.setManualValue( oldParam );
            }

            ++itor;
        }

        CompositorPass::resetNumPassesLeft();

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool hasTypedUavLoads = caps->hasCapability( RSC_TYPED_UAV_LOADS );
        if( !hasTypedUavLoads && mNumPassesLeft != std::numeric_limits<uint32>::max() )
            mNumPassesLeft = 1u;
    }
}  // namespace Ogre
