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

#include "Compositor/OgreCompositorNode.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "Compositor/Pass/OgreCompositorPassProvider.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClear.h"
#include "Compositor/Pass/PassCompute/OgreCompositorPassCompute.h"
#include "Compositor/Pass/PassCompute/OgreCompositorPassComputeDef.h"
#include "Compositor/Pass/PassDepthCopy/OgreCompositorPassDepthCopy.h"
#include "Compositor/Pass/PassDepthCopy/OgreCompositorPassDepthCopyDef.h"
#include "Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecular.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmap.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/Pass/PassShadows/OgreCompositorPassShadows.h"
#include "Compositor/Pass/PassShadows/OgreCompositorPassShadowsDef.h"
#include "Compositor/Pass/PassStencil/OgreCompositorPassStencil.h"
#include "Compositor/Pass/PassTargetBarrier/OgreCompositorPassTargetBarrier.h"
#include "Compositor/Pass/PassUav/OgreCompositorPassUav.h"
#include "Compositor/Pass/PassUav/OgreCompositorPassUavDef.h"
#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUp.h"
#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUpDef.h"
#include "OgreLogManager.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpu.h"
#include "Vao/OgreUavBufferPacked.h"

namespace Ogre
{
    CompositorNode::CompositorNode( IdType id, IdString name, const CompositorNodeDef *definition,
                                    CompositorWorkspace *workspace, RenderSystem *renderSys,
                                    TextureGpu *finalTarget ) :
        IdObject( id ),
        mName( name ),
        mEnabled( definition->mStartEnabled ),
        mNumConnectedInputs( 0 ),
        mNumConnectedBufferInputs( 0 ),
        mWorkspace( workspace ),
        mRenderSystem( renderSys ),
        mDefinition( definition )
    {
        mInTextures.resize( mDefinition->getNumInputChannels(), CompositorChannel() );
        mOutTextures.resize( mDefinition->mOutChannelMapping.size() );
        mConnectedNodes.resize( mDefinition->mOutChannelMapping.size() +
                                mDefinition->mOutBufferChannelMapping.size() );

        // Create local textures
        TextureDefinitionBase::createTextures( definition->mLocalTextureDefs, mLocalTextures, id,
                                               finalTarget, mRenderSystem );

        const CompositorNamedBufferVec &globalBuffers = workspace->getGlobalBuffers();

        // Create local buffers
        mBuffers.reserve( mDefinition->mLocalBufferDefs.size() + mDefinition->mInputBuffers.size() +
                          globalBuffers.size() );
        TextureDefinitionBase::createBuffers( definition->mLocalBufferDefs, mBuffers, finalTarget,
                                              renderSys );

        // Local textures will be routed now.
        routeOutputs();
    }
    //-----------------------------------------------------------------------------------
    CompositorNode::~CompositorNode()
    {
        // Passes need to be destroyed before destroying all nodes, since some
        // passes may hold listener references to these TextureGpus
        assert( mPasses.empty() && "CompositorNode::destroyAllPasses not called!" );

        // Don't leave dangling pointers
        disconnectOutput();

        // Destroy our local buffers
        TextureDefinitionBase::destroyBuffers( mDefinition->mLocalBufferDefs, mBuffers, mRenderSystem );

        // Destroy our local textures
        TextureDefinitionBase::destroyTextures( mLocalTextures, mRenderSystem );
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::destroyAllPasses()
    {
        // Destroy all passes
        CompositorPassVec::const_iterator itor = mPasses.begin();
        CompositorPassVec::const_iterator endt = mPasses.end();
        while( itor != endt )
            OGRE_DELETE *itor++;
        mPasses.clear();
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::routeOutputs()
    {
        CompositorChannelVec::iterator itor = mOutTextures.begin();
        CompositorChannelVec::iterator endt = mOutTextures.end();
        CompositorChannelVec::iterator begin = mOutTextures.begin();

        while( itor != endt )
        {
            size_t index;
            TextureDefinitionBase::TextureSource textureSource;
            mDefinition->getTextureSource( static_cast<size_t>( itor - begin ), index, textureSource );

            assert( textureSource == TextureDefinitionBase::TEXTURE_LOCAL ||
                    textureSource == TextureDefinitionBase::TEXTURE_INPUT );

            if( textureSource == TextureDefinitionBase::TEXTURE_LOCAL )
                *itor = mLocalTextures[index];
            else
                *itor = mInTextures[index];
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::disconnectOutput()
    {
        CompositorNodeVec::iterator itor = mConnectedNodes.begin();
        CompositorNodeVec::iterator endt = mConnectedNodes.end();

        while( itor != endt )
        {
            if( *itor )
            {
                CompositorChannelVec::const_iterator texIt = mLocalTextures.begin();
                CompositorChannelVec::const_iterator texEn = mLocalTextures.end();

                while( texIt != texEn )
                    ( *itor )->notifyDestroyed( *texIt++ );

                CompositorNodeDef::BufferDefinitionVec::const_iterator bufIt =
                    mDefinition->mLocalBufferDefs.begin();
                CompositorNodeDef::BufferDefinitionVec::const_iterator bufEn =
                    mDefinition->mLocalBufferDefs.end();

                while( bufIt != bufEn )
                {
                    UavBufferPacked *uavBuffer = this->getDefinedBufferNoThrow( bufIt->name );
                    if( uavBuffer )
                        ( *itor )->notifyDestroyed( uavBuffer );
                    ++bufIt;
                }
            }

            *itor = 0;
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::populateGlobalBuffers()
    {
        // Makes global buffers visible to our passes.
        CompositorNamedBuffer cmp;
        const CompositorNamedBufferVec &globalBuffers = mWorkspace->getGlobalBuffers();
        CompositorNamedBufferVec::const_iterator itor = globalBuffers.begin();
        CompositorNamedBufferVec::const_iterator endt = globalBuffers.end();
        while( itor != endt )
        {
            CompositorNamedBufferVec::iterator itBuf =
                std::lower_bound( mBuffers.begin(), mBuffers.end(), itor->name, cmp );
            if( itBuf != mBuffers.end() && itBuf->name == itor->name )
            {
                LogManager::getSingleton().logMessage( "WARNING: Locally defined buffer '" +
                                                       itBuf->name.getFriendlyText() + "' in Node '" +
                                                       mDefinition->mNameStr +
                                                       "' occludes global texture"
                                                       " of the same name." );
            }
            else
            {
                mBuffers.insert( itBuf, 1, *itor );
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::notifyRecreated( TextureGpu *channel )
    {
        // Notify nodes we're connected to that have that texture too.
        CompositorNodeVec::const_iterator nodeIt = mConnectedNodes.begin();
        CompositorChannelVec::const_iterator texIt = mOutTextures.begin();
        CompositorChannelVec::const_iterator texEn = mOutTextures.end();

        while( texIt != texEn )
        {
            if( *texIt == channel )
            {
                if( *nodeIt )
                {
                    // *nodeIt can be nullptr if user defined an output
                    // but never connected it to lead anywhere.
                    ( *nodeIt )->notifyRecreated( channel );
                }
            }
            ++nodeIt;
            ++texIt;
        }

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
        {
            ( *passIt )->notifyRecreated( channel );
            ++passIt;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer )
    {
        // Reset our inputs and notify nodes that we're connected to that also use that buffer.
        CompositorNamedBufferVec::iterator bufIt = mBuffers.begin();
        CompositorNamedBufferVec::iterator bufEn = mBuffers.end();

        // We can't early out, it's possible to assign the same output to two different
        // input channels (though it would work very unintuitively...)
        while( bufIt != bufEn )
        {
            if( bufIt->buffer == oldBuffer )
            {
                bufIt->buffer = newBuffer;

                // Check if we'll need to clear our outputs
                CompositorNodeVec::const_iterator nodeIt =
                    mConnectedNodes.begin() + ptrdiff_t( mDefinition->mOutChannelMapping.size() );
                IdStringVec::const_iterator itor = mDefinition->mOutBufferChannelMapping.begin();
                IdStringVec::const_iterator endt = mDefinition->mOutBufferChannelMapping.end();

                while( itor != endt )
                {
                    if( *itor == bufIt->name )
                    {
                        if( *nodeIt )
                        {
                            // *nodeIt can be nullptr if user defined an output
                            // but never connected it to lead anywhere.
                            ( *nodeIt )->notifyRecreated( oldBuffer, newBuffer );
                        }
                    }
                    ++nodeIt;
                    ++itor;
                }
            }

            ++bufIt;
        }

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
        {
            ( *passIt )->notifyRecreated( oldBuffer, newBuffer );
            ++passIt;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::notifyDestroyed( TextureGpu *channel )
    {
        // Clear our inputs
        CompositorChannelVec::iterator texIt = mInTextures.begin();
        CompositorChannelVec::iterator texEn = mInTextures.end();

        // We can't early out, it's possible to assign the same output to two different
        // input channels (though it would work very unintuitively...)
        while( texIt != texEn )
        {
            if( *texIt == channel )
            {
                *texIt = CompositorChannel();
                --mNumConnectedInputs;
            }
            ++texIt;
        }

        // Clear our outputs
        CompositorNodeVec::iterator nodeIt = mConnectedNodes.begin();
        texIt = mOutTextures.begin();
        texEn = mOutTextures.end();

        while( texIt != texEn )
        {
            if( *texIt == channel )
            {
                // Our attachees may have that texture too.
                if( *nodeIt )
                {
                    // *nodeIt can be nullptr if user defined an output
                    // but never connected it to lead anywhere.
                    ( *nodeIt )->notifyDestroyed( channel );
                }
                *nodeIt = 0;
                *texIt = CompositorChannel();
                --mNumConnectedInputs;
            }
            ++nodeIt;
            ++texIt;
        }

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
        {
            ( *passIt )->notifyDestroyed( channel );
            ++passIt;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::notifyDestroyed( const UavBufferPacked *buffer )
    {
        // Clear our inputs
        CompositorNamedBufferVec::iterator bufIt = mBuffers.begin();
        CompositorNamedBufferVec::iterator bufEn = mBuffers.end();

        // We can't early out, it's possible to assign the same output to two different
        // input channels (though it would work very unintuitively...)
        while( bufIt != bufEn )
        {
            if( bufIt->buffer == buffer )
            {
                // Check if we'll need to clear our outputs
                CompositorNodeVec::iterator nodeIt =
                    mConnectedNodes.begin() + ptrdiff_t( mDefinition->mOutChannelMapping.size() );
                IdStringVec::const_iterator itor = mDefinition->mOutBufferChannelMapping.begin();
                IdStringVec::const_iterator endt = mDefinition->mOutBufferChannelMapping.end();

                while( itor != endt )
                {
                    if( *nodeIt )
                    {
                        // *nodeIt can be nullptr if user defined an output
                        // but never connected it to lead anywhere.
                        ( *nodeIt )->notifyDestroyed( buffer );
                        *nodeIt = 0;
                    }
                    ++nodeIt;
                    ++itor;
                }

                // Remove this buffer.
                bufIt = mBuffers.erase( bufIt );
                bufEn = mBuffers.end();
                --mNumConnectedBufferInputs;
            }
            else
            {
                ++bufIt;
            }
        }

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
        {
            ( *passIt )->notifyDestroyed( buffer );
            ++passIt;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::_notifyCleared()
    {
        // Clear our inputs
        CompositorChannelVec::iterator texIt = mInTextures.begin();
        CompositorChannelVec::iterator texEn = mInTextures.end();

        while( texIt != texEn )
            *texIt++ = CompositorChannel();

        mNumConnectedInputs = 0;

        // Clear our inputs (buffers)
        CompositorNamedBuffer cmp;
        IdStringVec::const_iterator bufNameIt = mDefinition->mInputBuffers.begin();
        IdStringVec::const_iterator bufNameEn = mDefinition->mInputBuffers.end();

        while( bufNameIt != bufNameEn )
        {
            CompositorNamedBufferVec::iterator itBuf =
                std::lower_bound( mBuffers.begin(), mBuffers.end(), *bufNameIt, cmp );
            mBuffers.erase( itBuf );
            ++bufNameIt;
        }

        mNumConnectedBufferInputs = 0;

        // This call will clear only our outputs that come from input channels.
        routeOutputs();

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
            OGRE_DELETE *passIt++;

        mPasses.clear();

        CompositorNodeVec::iterator nodeIt = mConnectedNodes.begin();
        CompositorNodeVec::iterator nodeEn = mConnectedNodes.end();
        while( nodeIt != nodeEn )
            *nodeIt++ = 0;
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::setEnabled( bool bEnabled )
    {
        if( mEnabled != bEnabled )
        {
            mEnabled = bEnabled;
            mWorkspace->_notifyBarriersDirty();
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::connectBufferTo( size_t outChannelA, CompositorNode *nodeB, size_t inChannelB )
    {
        if( inChannelB >= nodeB->mDefinition->mInputBuffers.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no input channel #" + StringConverter::toString( inChannelB ) +
                             " for node " + nodeB->mDefinition->mNameStr +
                             " when trying to connect it "
                             "from " +
                             this->mDefinition->mNameStr + " channel #" +
                             StringConverter::toString( outChannelA ),
                         "CompositorNode::connectBufferTo" );
        }
        if( outChannelA >= this->mDefinition->mOutBufferChannelMapping.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no output channel #" + StringConverter::toString( outChannelA ) +
                             " for node " + this->mDefinition->mNameStr +
                             " when trying to connect it "
                             "to " +
                             nodeB->mDefinition->mNameStr + " channel #" +
                             StringConverter::toString( inChannelB ),
                         "CompositorNode::connectBufferTo" );
        }

        CompositorNamedBuffer cmp;

        const IdString &outBufferName = this->mDefinition->mOutBufferChannelMapping[outChannelA];
        CompositorNamedBufferVec::iterator outBuf =
            std::lower_bound( this->mBuffers.begin(), this->mBuffers.end(), outBufferName, cmp );

        // Nodes must be connected in the right order. We know for sure there is a buffer named
        // outBufferName in either input/local because we checked in mapOutputBufferChannel.
        assert( ( outBuf != mBuffers.end() && outBuf->name == outBufferName ) &&
                "Compositor node got connected in the wrong order!" );

        const IdString &inBufferName = nodeB->mDefinition->mInputBuffers[inChannelB];
        CompositorNamedBufferVec::iterator inBuf =
            std::lower_bound( nodeB->mBuffers.begin(), nodeB->mBuffers.end(), inBufferName, cmp );

        if( inBufferName == IdString() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Input buffer channels must not have gaps."
                         " Channel #" +
                             StringConverter::toString( inChannelB ) + " from node '" +
                             nodeB->mDefinition->mNameStr + "' is not defined.",
                         "CompositorNode::connectBufferTo" );
        }

        if( inBuf != nodeB->mBuffers.end() && inBuf->name == inBufferName )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Buffer with name '" + inBufferName.getFriendlyText() +
                             "' was already defined in node " + nodeB->mDefinition->mNameStr +
                             " while trying to connect to its "
                             "channel #" +
                             StringConverter::toString( inChannelB ) + "from " +
                             this->mDefinition->mNameStr + " channel #" +
                             StringConverter::toString( outChannelA ),
                         "CompositorNode::connectBufferTo" );
        }
        else
        {
            nodeB->mBuffers.insert( inBuf, 1, *outBuf );
            ++nodeB->mNumConnectedBufferInputs;
            this->mConnectedNodes[this->mDefinition->mOutChannelMapping.size() + outChannelA] = nodeB;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::connectTo( size_t outChannelA, CompositorNode *nodeB, size_t inChannelB )
    {
        if( inChannelB >= nodeB->mInTextures.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no input channel #" + StringConverter::toString( inChannelB ) +
                             " for node " + nodeB->mDefinition->mNameStr +
                             " when trying to connect it "
                             "from " +
                             this->mDefinition->mNameStr + " channel #" +
                             StringConverter::toString( outChannelA ),
                         "CompositorNode::connectTo" );
        }
        if( outChannelA >= this->mOutTextures.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no output channel #" + StringConverter::toString( outChannelA ) +
                             " for node " + this->mDefinition->mNameStr +
                             " when trying to connect it "
                             "to " +
                             nodeB->mDefinition->mNameStr + " channel #" +
                             StringConverter::toString( inChannelB ),
                         "CompositorNode::connectTo" );
        }

        // Nodes must be connected in the right order (and after routeOutputs was called)
        // to avoid passing null pointers (which is probably not what we wanted)
        assert( this->mOutTextures[outChannelA] && "Compositor node got connected in the wrong order!" );

        if( !nodeB->mInTextures[inChannelB] )
            ++nodeB->mNumConnectedInputs;
        nodeB->mInTextures[inChannelB] = this->mOutTextures[outChannelA];

        if( nodeB->mNumConnectedInputs >= nodeB->mInTextures.size() )
            nodeB->routeOutputs();

        this->mConnectedNodes[outChannelA] = nodeB;
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::connectExternalRT( TextureGpu *externalTexture, size_t inChannelA )
    {
        if( inChannelA >= mInTextures.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no input channel #" + StringConverter::toString( inChannelA ) +
                             " for node " + mDefinition->mNameStr,
                         "CompositorNode::connectFinalRT" );
        }

        if( !mInTextures[inChannelA] )
            ++mNumConnectedInputs;
        mInTextures[inChannelA] = externalTexture;

        if( this->mNumConnectedInputs >= this->mInTextures.size() )
            this->routeOutputs();
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::connectExternalBuffer( UavBufferPacked *buffer, size_t inChannelA )
    {
        if( inChannelA >= this->mDefinition->mInputBuffers.size() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "There is no input buffer channel #" + StringConverter::toString( inChannelA ) +
                             " for node " + mDefinition->mNameStr,
                         "CompositorNode::connectExternalBuffer" );
        }

        CompositorNamedBuffer cmp;
        const IdString &inBufferName = mDefinition->mInputBuffers[inChannelA];
        CompositorNamedBufferVec::iterator inBuf =
            std::lower_bound( mBuffers.begin(), mBuffers.end(), inBufferName, cmp );

        if( inBufferName == IdString() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Input buffer channels must not have gaps."
                         " Channel #" +
                             StringConverter::toString( inChannelA ) + " from node '" +
                             mDefinition->mNameStr + "' is not defined.",
                         "CompositorNode::connectExternalBuffer" );
        }

        if( inBuf != mBuffers.end() && inBuf->name == inBufferName )
        {
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Buffer with name '" + inBufferName.getFriendlyText() +
                             "' was already defined in node " + mDefinition->mNameStr +
                             " while trying to connect to its "
                             "channel #" +
                             StringConverter::toString( inChannelA ) +
                             ". Did you define the buffer twice?",
                         "CompositorNode::connectExternalBuffer" );
        }
        else
        {
            const CompositorNamedBuffer namedBuffer( inBufferName, buffer );
            mBuffers.insert( inBuf, 1, namedBuffer );
            ++mNumConnectedBufferInputs;
        }
    }
    //-----------------------------------------------------------------------------------
    bool CompositorNode::areAllInputsConnected() const
    {
        return mNumConnectedInputs == mInTextures.size() &&
               mNumConnectedBufferInputs == mDefinition->mInputBuffers.size();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *CompositorNode::getDefinedTexture( IdString textureName ) const
    {
        TextureGpu *channel = 0;
        size_t index;
        TextureDefinitionBase::TextureSource textureSource;
        mDefinition->getTextureSource( textureName, index, textureSource );
        switch( textureSource )
        {
        case TextureDefinitionBase::TEXTURE_INPUT:
            channel = mInTextures[index];
            break;
        case TextureDefinitionBase::TEXTURE_LOCAL:
            channel = mLocalTextures[index];
            break;
        case TextureDefinitionBase::TEXTURE_GLOBAL:
            channel = mWorkspace->getGlobalTexture( textureName );
            break;
        default:
            break;
        }

        return channel;
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked *CompositorNode::getDefinedBuffer( IdString bufferName ) const
    {
        UavBufferPacked *retVal = getDefinedBufferNoThrow( bufferName );

        if( !retVal )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find UAV buffer " + bufferName.getFriendlyText() + " in node '" +
                             mDefinition->mNameStr + "'",
                         "CompositorNode::getDefinedBuffer" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked *CompositorNode::getDefinedBufferNoThrow( IdString bufferName ) const
    {
        UavBufferPacked *retVal = 0;

        CompositorNamedBuffer cmp;
        CompositorNamedBufferVec::const_iterator itBuf =
            std::lower_bound( mBuffers.begin(), mBuffers.end(), bufferName, cmp );
        if( itBuf != mBuffers.end() && itBuf->name == bufferName )
            retVal = itBuf->buffer;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::createPasses()
    {
        populateGlobalBuffers();

        CompositorTargetDefVec::const_iterator itor = mDefinition->mTargetPasses.begin();
        CompositorTargetDefVec::const_iterator endt = mDefinition->mTargetPasses.end();

        while( itor != endt )
        {
            RenderTargetViewDef const *rtvDef = 0;

            if( itor->getRenderTargetName() != IdString() )
                rtvDef = mDefinition->getRenderTargetViewDef( itor->getRenderTargetName() );

            const CompositorPassDefVec &passes = itor->getCompositorPasses();

            CompositorPassTargetBarrier *targetBarrier = 0;
            if( itor->getTargetLevelBarrier() )
            {
                targetBarrier = OGRE_NEW CompositorPassTargetBarrier( itor->getTargetLevelBarrierDef(),
                                                                      this, passes.size() );
                postInitializePass( targetBarrier );
                mPasses.push_back( targetBarrier );
            }

            CompositorPassDefVec::const_iterator itPass = passes.begin();
            CompositorPassDefVec::const_iterator enPass = passes.end();

            while( itPass != enPass )
            {
                CompositorPass *newPass = 0;
                switch( ( *itPass )->getType() )
                {
                case PASS_CLEAR:
                    newPass =
                        OGRE_NEW CompositorPassClear( static_cast<CompositorPassClearDef *>( *itPass ),
                                                      mWorkspace->getSceneManager(), rtvDef, this );
                    break;
                case PASS_QUAD:
                    newPass = OGRE_NEW CompositorPassQuad(
                        static_cast<CompositorPassQuadDef *>( *itPass ), mWorkspace->getDefaultCamera(),
                        this, rtvDef, mRenderSystem->getHorizontalTexelOffset(),
                        mRenderSystem->getVerticalTexelOffset() );
                    break;
                case PASS_SCENE:
                    newPass =
                        OGRE_NEW CompositorPassScene( static_cast<CompositorPassSceneDef *>( *itPass ),
                                                      mWorkspace->getDefaultCamera(), rtvDef, this );
                    break;
                case PASS_STENCIL:
                    newPass = OGRE_NEW CompositorPassStencil(
                        static_cast<CompositorPassStencilDef *>( *itPass ), rtvDef, this,
                        mRenderSystem );
                    break;
                case PASS_DEPTHCOPY:
                    newPass = OGRE_NEW CompositorPassDepthCopy(
                        static_cast<CompositorPassDepthCopyDef *>( *itPass ), rtvDef, this );
                    break;
                case PASS_UAV:
                    newPass = OGRE_NEW CompositorPassUav( static_cast<CompositorPassUavDef *>( *itPass ),
                                                          this, rtvDef );
                    break;
                case PASS_MIPMAP:
                    newPass = OGRE_NEW CompositorPassMipmap(
                        static_cast<CompositorPassMipmapDef *>( *itPass ), rtvDef, this );
                    break;
                case PASS_IBL_SPECULAR:
                    newPass = OGRE_NEW CompositorPassIblSpecular(
                        static_cast<CompositorPassIblSpecularDef *>( *itPass ), rtvDef, this );
                    break;
                case PASS_SHADOWS:
                    newPass = OGRE_NEW CompositorPassShadows(
                        static_cast<CompositorPassShadowsDef *>( *itPass ),
                        mWorkspace->getDefaultCamera(), this, rtvDef );
                    break;
                case PASS_COMPUTE:
                    newPass = OGRE_NEW CompositorPassCompute(
                        static_cast<CompositorPassComputeDef *>( *itPass ),
                        mWorkspace->getDefaultCamera(), this, rtvDef );
                    break;
                case PASS_WARM_UP:
                    newPass =
                        OGRE_NEW CompositorPassWarmUp( static_cast<CompositorPassWarmUpDef *>( *itPass ),
                                                       mWorkspace->getDefaultCamera(), this, rtvDef );
                    break;
                case PASS_CUSTOM:
                {
                    CompositorPassProvider *passProvider =
                        mWorkspace->getCompositorManager()->getCompositorPassProvider();
                    newPass = passProvider->addPass( *itPass, mWorkspace->getDefaultCamera(), this,
                                                     rtvDef, mWorkspace->getSceneManager() );
                }
                break;
                case PASS_TARGET_BARRIER:
                default:
                    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                                 "Pass type not implemented or not recognized",
                                 "CompositorNode::initializePasses" );
                }

                postInitializePass( newPass );

                mPasses.push_back( newPass );
                if( targetBarrier )
                    targetBarrier->addPass( newPass );
                ++itPass;
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::_update( const Camera *lodCamera, SceneManager *sceneManager )
    {
        // If we're in a caster pass, we need to skip shadow map passes that have no light associated
        const CompositorShadowNode *shadowNode = 0;
        if( sceneManager->_getCurrentRenderStage() == SceneManager::IRS_RENDER_TO_TEXTURE )
            shadowNode = sceneManager->getCurrentShadowNode();
        uint8 executionMask = mWorkspace->getExecutionMask();

        // Execute our passes
        CompositorPassVec::const_iterator itor = mPasses.begin();
        CompositorPassVec::const_iterator endt = mPasses.end();

        while( itor != endt )
        {
            CompositorPass *pass = *itor;
            const CompositorPassDef *passDef = pass->getDefinition();

            const CompositorTargetDef *targetDef = passDef->getParentTargetDef();

            if( executionMask & passDef->mExecutionMask &&
                ( !shadowNode || ( !shadowNode->isShadowMapIdxInValidRange( passDef->mShadowMapIdx ) ||
                                   ( shadowNode->_shouldUpdateShadowMapIdx( passDef->mShadowMapIdx ) &&
                                     ( shadowNode->getShadowMapLightTypeMask( passDef->mShadowMapIdx ) &
                                       targetDef->getShadowMapSupportedLightTypes() ) ) ) ) )
            {
                // Make explicitly exposed textures available to materials during this pass.
                const size_t oldNumTextures = sceneManager->getNumCompositorTextures();
                IdStringVec::const_iterator itExposed = passDef->mExposedTextures.begin();
                IdStringVec::const_iterator enExposed = passDef->mExposedTextures.end();

                while( itExposed != enExposed )
                {
                    TextureGpu *exposedChannel = this->getDefinedTexture( *itExposed );
                    sceneManager->_addCompositorTexture( *itExposed, exposedChannel );
                    ++itExposed;
                }

                // Execute pass
                pass->execute( lodCamera );

                // Remove our textures
                sceneManager->_removeCompositorTextures( oldNumTextures );
            }
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::finalTargetResized01( const TextureGpu *finalTarget )
    {
        TextureDefinitionBase::recreateResizableTextures01( mDefinition->mLocalTextureDefs,
                                                            mLocalTextures, finalTarget );
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::finalTargetResized02( const TextureGpu *finalTarget )
    {
        TextureDefinitionBase::recreateResizableTextures02( mDefinition->mLocalTextureDefs,
                                                            mLocalTextures, mConnectedNodes, &mPasses );
        TextureDefinitionBase::recreateResizableBuffers( mDefinition->mLocalBufferDefs, mBuffers,
                                                         finalTarget, mRenderSystem, mConnectedNodes,
                                                         &mPasses );

        CompositorPassVec::const_iterator passIt = mPasses.begin();
        CompositorPassVec::const_iterator passEn = mPasses.end();
        while( passIt != passEn )
        {
            ( *passIt )->notifyRecreated( finalTarget );
            ++passIt;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorNode::resetAllNumPassesLeft()
    {
        CompositorPassVec::const_iterator itor = mPasses.begin();
        CompositorPassVec::const_iterator endt = mPasses.end();

        while( itor != endt )
        {
            ( *itor )->resetNumPassesLeft();
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    size_t CompositorNode::getPassNumber( CompositorPass *pass ) const
    {
        return mDefinition->getPassNumber( pass->getDefinition() );
    }
}  // namespace Ogre
