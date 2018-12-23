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

#include "Compositor/Pass/PassUav/OgreCompositorPassUav.h"
#include "Compositor/Pass/PassUav/OgreCompositorPassUavDef.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Vao/OgreUavBufferPacked.h"

#include "OgreRenderSystem.h"
#include "OgreTextureGpuManager.h"
#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreDescriptorSetUav.h"

namespace Ogre
{
    void CompositorPassUavDef::setUav( uint32 slot, bool isExternal, const String &textureName,
                                       ResourceAccess::ResourceAccess access,
                                       int32 mipmapLevel, PixelFormatGpu pixelFormat )
    {
        if( !isExternal )
        {
            if( textureName.find( "global_" ) == 0 )
            {
                mParentNodeDef->addTextureSourceName( textureName, 0,
                                                      TextureDefinitionBase::TEXTURE_GLOBAL );
            }
        }

        if( textureName.empty() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot supply empty name for UAV texture",
                         "CompositorPassUavDef::setUav" );
        }

        mTextureSources.push_back( TextureSource( slot, textureName, isExternal,
                                                  access, mipmapLevel, pixelFormat ) );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUavDef::addUavBuffer( uint32 slotIdx, IdString bufferName,
                                             ResourceAccess::ResourceAccess access, size_t offset,
                                             size_t sizeBytes )
    {
        assert( access != ResourceAccess::Undefined );
        mBufferSources.push_back( BufferSource( slotIdx, bufferName, access, offset, sizeBytes ) );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassUav::CompositorPassUav( const CompositorPassUavDef *definition,
                                          CompositorNode *parentNode,
                                          const RenderTargetViewDef *rtv ) :
                CompositorPass( definition, parentNode ),
                mDefinition( definition ),
                mDescriptorSetUav( 0 )
    {
        initialize( rtv );
    }
    //-----------------------------------------------------------------------------------
    CompositorPassUav::~CompositorPassUav()
    {
        destroyDescriptorSetUav();
    }
    //-----------------------------------------------------------------------------------
    uint32 CompositorPassUav::calculateNumberUavSlots(void) const
    {
        uint32 retVal = 0;

        {
            const CompositorPassUavDef::TextureSources &textureSources =
                    mDefinition->getTextureSources();
            CompositorPassUavDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassUavDef::TextureSources::const_iterator end  = textureSources.end();

            while( itor != end )
            {
                retVal = std::max( retVal, itor->uavSlot + 1u );
                ++itor;
            }
        }

        {
            const CompositorPassUavDef::BufferSourceVec &bufferSources = mDefinition->getBufferSources();
            CompositorPassUavDef::BufferSourceVec::const_iterator itor = bufferSources.begin();
            CompositorPassUavDef::BufferSourceVec::const_iterator end  = bufferSources.end();
            while( itor != end )
            {
                retVal = std::max( retVal, itor->uavSlot + 1u );
                ++itor;
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::setupDescriptorSetUav(void)
    {
        destroyDescriptorSetUav();

        DescriptorSetUav descSetUav;
        {
            descSetUav.mUavs.resize( calculateNumberUavSlots() );

            const CompositorPassUavDef::TextureSources &textureSources =
                    mDefinition->getTextureSources();
            CompositorPassUavDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassUavDef::TextureSources::const_iterator end  = textureSources.end();
            while( itor != end )
            {
                TextureGpu *texture;

                if( !itor->isExternal )
                    texture = mParentNode->getDefinedTexture( itor->textureName );
                else
                {
                    RenderSystem *renderSystem = mParentNode->getRenderSystem();
                    TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
                    texture = textureManager->findTextureNoThrow( itor->textureName );
                }

                if( !texture )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Texture with name: " +
                                 itor->textureName.getFriendlyText() +
                                 " does not exist. The texture must exist by the time the "
                                 "workspace is executed. Are you trying to use a texture "
                                 "defined by the compositor? If so you need to set it via "
                                 "'uav' instead of 'uav_external'", "CompositorPassUav::execute" );
                }

                texture->addListener( this );

                DescriptorSetUav::Slot slot( DescriptorSetUav::SlotTypeTexture );
                DescriptorSetUav::TextureSlot &textureSlot = slot.getTexture();
                textureSlot.texture             = texture;
                textureSlot.access              = itor->access;
                textureSlot.mipmapLevel         = itor->mipmapLevel;
                textureSlot.textureArrayIndex   = 0;
                textureSlot.pixelFormat         = itor->pixelFormat;

                descSetUav.mUavs[itor->uavSlot] = slot;
                ++itor;
            }
        }

        {
            const CompositorPassUavDef::BufferSourceVec &bufferSources = mDefinition->getBufferSources();
            CompositorPassUavDef::BufferSourceVec::const_iterator itor = bufferSources.begin();
            CompositorPassUavDef::BufferSourceVec::const_iterator end  = bufferSources.end();
            while( itor != end )
            {
                UavBufferPacked *uavBuffer = 0;

                if( itor->bufferName != IdString() )
                    uavBuffer = mParentNode->getDefinedBuffer( itor->bufferName );

                DescriptorSetUav::Slot slot( DescriptorSetUav::SlotTypeBuffer );
                DescriptorSetUav::BufferSlot &bufferSlot = slot.getBuffer();
                bufferSlot.buffer       = uavBuffer;
                bufferSlot.offset       = itor->offset;
                bufferSlot.sizeBytes    = itor->sizeBytes;
                bufferSlot.access       = itor->access;

                descSetUav.mUavs[itor->uavSlot] = slot;
                ++itor;
            }
        }

        if( !descSetUav.mUavs.empty() )
        {
            HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
            mDescriptorSetUav = hlmsManager->getDescriptorSetUav( descSetUav );
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::execute( const Camera *lodCamera )
    {
        //Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        CompositorWorkspaceListener *listener = mParentNode->getWorkspace()->getListener();
        if( listener )
            listener->passEarlyPreExecute( this );

        if( !mDescriptorSetUav )
            setupDescriptorSetUav();

        //Fire the listener in case it wants to change anything
        if( listener )
            listener->passPreExecute( this );

        //Do not execute resource transitions. This pass shouldn't have them.
        //The transitions are made when the bindings are needed
        //(<sarcasm>we'll have fun with the validation layers later</sarcasm>).
        //executeResourceTransitions();
        assert( mResourceTransitions.empty() );

        RenderSystem *renderSystem = mParentNode->getRenderSystem();

        if( mDefinition->mStartingSlot != std::numeric_limits<uint8>::max() )
            renderSystem->setUavStartingSlot( mDefinition->mStartingSlot );

        renderSystem->queueBindUAVs( mDescriptorSetUav );

        if( listener )
            listener->passPosExecute( this );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::_placeBarriersAndEmulateUavExecution(
                                            BoundUav boundUavs[64], ResourceAccessMap &uavsAccess,
                                            ResourceLayoutMap &resourcesLayout )
    {
        {
            const CompositorPassUavDef::TextureSources &textureSources =
                    mDefinition->getTextureSources();
            CompositorPassUavDef::TextureSources::const_iterator itor = textureSources.begin();
            CompositorPassUavDef::TextureSources::const_iterator end  = textureSources.end();
            while( itor != end )
            {
                TextureGpu *texture = 0;

                if( !itor->isExternal )
                {
                    texture = mParentNode->getDefinedTexture( itor->textureName );
                }
                else if( itor->textureName != IdString() )
                {
                    RenderSystem *renderSystem = mParentNode->getRenderSystem();
                    TextureGpuManager *textureManager = renderSystem->getTextureGpuManager();
                    //TODO: Should we be using createOrRetrieve???
                    texture = textureManager->findTextureNoThrow(
                                itor->textureName/*,
                                ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME*/ );
                }

                if( !texture )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Texture with name: " +
                                 itor->textureName.getFriendlyText() +
                                 " does not exist. The texture must exist by the time the "
                                 "workspace is executed. Are you trying to use a texture "
                                 "defined by the compositor? If so you need to set it via "
                                 "'uav' instead of 'uav_external'",
                                 "CompositorPassUav::_placeBarriersAndEmulateUavExecution" );
                }

                if( !texture->isUav() )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Texture " + texture->getNameStr() +
                                 " must have been created with TextureFlags:Uav to be bound as UAV",
                                 "CompositorPassUav::_placeBarriersAndEmulateUavExecution" );
                }

                //Only "simulate the bind" of UAVs. We will evaluate the actual resource
                //transition when the UAV is actually used in the subsequent passes.
                boundUavs[itor->uavSlot].rttOrBuffer = texture;
                boundUavs[itor->uavSlot].boundAccess = itor->access;

                ++itor;
            }
        }

        {
            const CompositorPassUavDef::BufferSourceVec &bufferSources = mDefinition->getBufferSources();
            CompositorPassUavDef::BufferSourceVec::const_iterator itor = bufferSources.begin();
            CompositorPassUavDef::BufferSourceVec::const_iterator end  = bufferSources.end();
            while( itor != end )
            {
                UavBufferPacked *uavBuffer = 0;

                if( itor->bufferName != IdString() )
                    uavBuffer = mParentNode->getDefinedBuffer( itor->bufferName );

                //Only "simulate the bind" of UAVs. We will evaluate the actual resource
                //transition when the UAV is actually used in the subsequent passes.
                boundUavs[itor->uavSlot].rttOrBuffer = uavBuffer;
                boundUavs[itor->uavSlot].boundAccess = itor->access;

                ++itor;
            }
        }

        //Take the chance to create all the mDescriptorSetUav
        setupDescriptorSetUav();

        //Do not use base class functionality at all.
        //CompositorPass::_placeBarriersAndEmulateUavExecution();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::destroyDescriptorSetUav()
    {
        if( mDescriptorSetUav )
        {
            HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
            FastArray<DescriptorSetUav::Slot>::const_iterator itor = mDescriptorSetUav->mUavs.begin();
            FastArray<DescriptorSetUav::Slot>::const_iterator end  = mDescriptorSetUav->mUavs.end();

            while( itor != end )
            {
                if( itor->isTexture() )
                    itor->getTexture().texture->removeListener( this );

                ++itor;
            }

            hlmsManager->destroyDescriptorSetUav( mDescriptorSetUav );
            mDescriptorSetUav = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer )
    {
        destroyDescriptorSetUav();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassUav::notifyTextureChanged( TextureGpu *texture,
                                                  TextureGpuListener::Reason reason, void *extraData )
    {
        destroyDescriptorSetUav();
    }
}
