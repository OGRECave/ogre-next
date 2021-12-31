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

#include "Compositor/Pass/PassCompute/OgreCompositorPassCompute.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassCompute/OgreCompositorPassComputeDef.h"
#include "OgreCamera.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"
#include "Vao/OgreUavBufferPacked.h"

namespace Ogre
{
    void CompositorPassComputeDef::addTextureSource( uint32 texUnitIdx, const String &textureName )
    {
        if( textureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( textureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        mTextureSources.push_back( ComputeTextureSource( texUnitIdx, textureName ) );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassComputeDef::addTextureSource( uint32 texUnitIdx, const String &textureName,
                                                     uint16 textureArrayIndex, uint8 mipmapLevel,
                                                     PixelFormatGpu pixelFormat )
    {
        mTextureSources.push_back( ComputeTextureSource( texUnitIdx, textureName, ResourceAccess::Read,
                                                         mipmapLevel, textureArrayIndex, pixelFormat,
                                                         true ) );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassComputeDef::addUavSource( uint32 texUnitIdx, const String &textureName,
                                                 ResourceAccess::ResourceAccess access,
                                                 uint16 textureArrayIndex, uint8 mipmapLevel,
                                                 PixelFormatGpu pixelFormat, bool allowWriteAfterWrite )
    {
        assert( access != ResourceAccess::Undefined );
        if( textureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( textureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        mUavSources.push_back( ComputeTextureSource( texUnitIdx, textureName, access, mipmapLevel,
                                                     textureArrayIndex, pixelFormat,
                                                     allowWriteAfterWrite ) );
    }
    //-----------------------------------------------------------------------------------
    //    void CompositorPassComputeDef::addTexBuffer( uint32 slotIdx, const String &bufferName,
    //                                                 size_t offset, size_t sizeBytes )
    //    {
    //        //TODO.
    //    }
    //-----------------------------------------------------------------------------------
    void CompositorPassComputeDef::addUavBuffer( uint32 slotIdx, const String &bufferName,
                                                 ResourceAccess::ResourceAccess access, size_t offset,
                                                 size_t sizeBytes, bool allowWriteAfterWrite )
    {
        assert( access != ResourceAccess::Undefined );
        mBufferSources.push_back(
            BufferSource( slotIdx, bufferName, access, offset, sizeBytes, allowWriteAfterWrite ) );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassCompute::CompositorPassCompute( const CompositorPassComputeDef *definition,
                                                  Camera *defaultCamera, CompositorNode *parentNode,
                                                  const RenderTargetViewDef *rtv ) :
        CompositorPass( definition, parentNode ),
        mDefinition( definition ),
        mCamera( 0 )
    {
        initialize( 0, true );

        HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
        HlmsCompute *hlmsCompute = hlmsManager->getComputeHlms();

        mComputeJob = hlmsCompute->findComputeJob( mDefinition->mJobName );

        assert( mDefinition->mExposedTextures.empty() &&
                "Invalid parameters set to the pass definition. Barriers may not behave correctly" );

        // List all our RTT dependencies
        const CompositorPassComputeDef::TextureSources &textureSources =
            mDefinition->getTextureSources();
        CompositorPassComputeDef::TextureSources::const_iterator itor = textureSources.begin();
        CompositorPassComputeDef::TextureSources::const_iterator endt = textureSources.end();
        while( itor != endt )
        {
            TextureGpu *channel = mParentNode->getDefinedTexture( itor->textureName );
            CompositorTextureVec::const_iterator it = mTextureDependencies.begin();
            CompositorTextureVec::const_iterator en = mTextureDependencies.end();
            while( it != en && it->name != itor->textureName )
                ++it;

            if( it == en )
                mTextureDependencies.push_back( CompositorTexture( itor->textureName, channel ) );

            ++itor;
        }

        {
            // Ensure our compute job has enough UAV units available.
            uint8 maxUsedSlot = 0u;
            const CompositorPassComputeDef::TextureSources &uavSources = mDefinition->getUavSources();
            CompositorPassComputeDef::TextureSources::const_iterator it = uavSources.begin();
            CompositorPassComputeDef::TextureSources::const_iterator en = uavSources.end();
            while( it != en )
            {
                maxUsedSlot = std::max( maxUsedSlot, static_cast<uint8>( it->texUnitIdx ) );
                ++it;
            }

            if( maxUsedSlot >= mComputeJob->getNumUavUnits() )
                mComputeJob->setNumUavUnits( maxUsedSlot + 1u );
        }

        setResourcesToJob();

        const CompositorWorkspace *workspace = parentNode->getWorkspace();
        if( mDefinition->mCameraName != IdString() )
            mCamera = workspace->findCamera( mDefinition->mCameraName );
        else
            mCamera = defaultCamera;
    }
    //-----------------------------------------------------------------------------------
    CompositorPassCompute::~CompositorPassCompute()
    {
        // Clear all our bindings to prevent leaving dangling pointers
        {
            const CompositorPassComputeDef::TextureSources &textureSources =
                mDefinition->getTextureSources();
            CompositorPassComputeDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassComputeDef::TextureSources::const_iterator endt = textureSources.end();
            while( itor != endt )
            {
                DescriptorSetTexture2::TextureSlot texSlot(
                    DescriptorSetTexture2::TextureSlot::makeEmpty() );
                mComputeJob->setTexture( (uint8)itor->texUnitIdx, texSlot );
                ++itor;
            }

            const CompositorPassComputeDef::TextureSources &uavSources = mDefinition->getUavSources();
            itor = uavSources.begin();
            endt = uavSources.end();
            while( itor != endt )
            {
                DescriptorSetUav::TextureSlot texSlot( DescriptorSetUav::TextureSlot::makeEmpty() );
                mComputeJob->_setUavTexture( (uint8)itor->texUnitIdx, texSlot );
                ++itor;
            }
        }

        {
            const CompositorPassComputeDef::BufferSourceVec &bufferSources =
                mDefinition->getBufferSources();
            CompositorPassComputeDef::BufferSourceVec::const_iterator itor = bufferSources.begin();
            CompositorPassComputeDef::BufferSourceVec::const_iterator endt = bufferSources.end();

            while( itor != endt )
            {
                DescriptorSetUav::BufferSlot bufferSlot( DescriptorSetUav::BufferSlot::makeEmpty() );
                mComputeJob->_setUavBuffer( (uint8)itor->slotIdx, bufferSlot );
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassCompute::setResourcesToJob()
    {
        {
            const CompositorPassComputeDef::TextureSources &textureSources =
                mDefinition->getTextureSources();
            CompositorPassComputeDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassComputeDef::TextureSources::const_iterator endt = textureSources.end();
            while( itor != endt )
            {
                DescriptorSetTexture2::TextureSlot texSlot(
                    DescriptorSetTexture2::TextureSlot::makeEmpty() );
                texSlot.texture = mParentNode->getDefinedTexture( itor->textureName );
                if( itor->usesAllFields )
                {
                    texSlot.mipmapLevel = static_cast<uint8>( itor->mipmapLevel );
                    texSlot.textureArrayIndex = static_cast<uint16>( itor->textureArrayIndex );
                    texSlot.pixelFormat = itor->pixelFormat;
                }
                mComputeJob->setTexture( (uint8)itor->texUnitIdx, texSlot );
                ++itor;
            }

            const CompositorPassComputeDef::TextureSources &uavSources = mDefinition->getUavSources();
            itor = uavSources.begin();
            endt = uavSources.end();
            while( itor != endt )
            {
                TextureGpu *texture = mParentNode->getDefinedTexture( itor->textureName );
                DescriptorSetUav::TextureSlot texSlot;
                texSlot.texture = texture;
                texSlot.access = itor->access;
                texSlot.mipmapLevel = itor->mipmapLevel;
                texSlot.textureArrayIndex = itor->textureArrayIndex;
                texSlot.pixelFormat = itor->pixelFormat;
                mComputeJob->_setUavTexture( (uint8)itor->texUnitIdx, texSlot );
                ++itor;
            }
        }

        {
            const CompositorPassComputeDef::BufferSourceVec &bufferSources =
                mDefinition->getBufferSources();
            CompositorPassComputeDef::BufferSourceVec::const_iterator itor = bufferSources.begin();
            CompositorPassComputeDef::BufferSourceVec::const_iterator endt = bufferSources.end();

            while( itor != endt )
            {
                UavBufferPacked *uavBuffer = mParentNode->getDefinedBuffer( itor->bufferName );
                DescriptorSetUav::BufferSlot bufferSlot;
                bufferSlot.buffer = uavBuffer;
                bufferSlot.offset = itor->offset;
                bufferSlot.sizeBytes = itor->sizeBytes;
                bufferSlot.access = itor->access;
                mComputeJob->_setUavBuffer( (uint8)itor->slotIdx, bufferSlot );
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassCompute::execute( const Camera *lodCamera )
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

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->endRenderPassDescriptor();

        // Set textures/uavs every frame
        setResourcesToJob();

        mComputeJob->analyzeBarriers( mResourceTransitions );
        executeResourceTransitions();

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        assert( dynamic_cast<HlmsCompute *>( mComputeJob->getCreator() ) );

        SceneManager *sceneManager = 0;
        if( mCamera )
            sceneManager = mCamera->getSceneManager();

        HlmsCompute *hlmsCompute = static_cast<HlmsCompute *>( mComputeJob->getCreator() );
        hlmsCompute->dispatch( mComputeJob, sceneManager, mCamera );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
}  // namespace Ogre
