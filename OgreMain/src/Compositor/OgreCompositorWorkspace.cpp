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

#include "Compositor/OgreCompositorWorkspace.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/Pass/PassShadows/OgreCompositorPassShadows.h"
#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUp.h"
#include "Compositor/Pass/PassWarmUp/OgreCompositorPassWarmUpDef.h"
#include "OgreCamera.h"
#include "OgreLogManager.h"
#include "OgreProfiler.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"

namespace Ogre
{
    CompositorWorkspace::CompositorWorkspace( IdType id, const CompositorWorkspaceDef *definition,
                                              const CompositorChannelVec &externalRenderTargets,
                                              SceneManager *sceneManager, Camera *defaultCam,
                                              RenderSystem *renderSys, bool bEnabled,
                                              uint8 executionMask, uint8 viewportModifierMask,
                                              const Vector4 &vpOffsetScale,
                                              const UavBufferPackedVec *uavBuffers,
                                              const ResourceStatusMap *initialLayouts ) :
        IdObject( id ),
        mDefinition( definition ),
        mValid( false ),
        mEnabled( bEnabled ),
        mAmalgamatedProfiling( false ),
        mDefaultCamera( defaultCam ),
        mSceneManager( sceneManager ),
        mRenderSys( renderSys ),
        mExternalRenderTargets( externalRenderTargets ),
        mExecutionMask( executionMask ),
        mViewportModifierMask( viewportModifierMask ),
        mViewportModifier( vpOffsetScale )
    {
        assert( ( !defaultCam || ( defaultCam->getSceneManager() == sceneManager ) ) &&
                "Camera was created with a different SceneManager than supplied" );

        if( uavBuffers )
            mExternalBuffers = *uavBuffers;

        if( initialLayouts )
            mInitialLayouts = *initialLayouts;

        TextureGpu *finalTarget = getFinalTarget();

        // We need this so OpenGL can switch contexts (if needed) before creating the textures
        if( finalTarget )
            mRenderSys->_setCurrentDeviceFromTexture( finalTarget );

        // Create global textures
        TextureDefinitionBase::createTextures( definition->mLocalTextureDefs, mGlobalTextures, id,
                                               finalTarget, mRenderSys );

        // Create local buffers
        mGlobalBuffers.reserve( mDefinition->mLocalBufferDefs.size() );
        TextureDefinitionBase::createBuffers( definition->mLocalBufferDefs, mGlobalBuffers, finalTarget,
                                              mRenderSys );

        recreateAllNodes();

        mCurrentWidth = finalTarget->getWidth();
        mCurrentHeight = finalTarget->getHeight();
        mCurrentOrientationMode = finalTarget->getOrientationMode();

        // Some RenderSystems assume we start the frame with empty pointer.
        if( finalTarget )
            mRenderSys->_setCurrentDeviceFromTexture( finalTarget );
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspace::~CompositorWorkspace()
    {
        destroyAllNodes();

        // Destroy our global buffers
        TextureDefinitionBase::destroyBuffers( mDefinition->mLocalBufferDefs, mGlobalBuffers,
                                               mRenderSys );

        // Destroy our global textures
        TextureDefinitionBase::destroyTextures( mGlobalTextures, mRenderSys );
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::createAllNodes()
    {
        destroyAllNodes();

        CompositorWorkspaceDef::NodeAliasMap::const_iterator itor = mDefinition->mAliasedNodes.begin();
        CompositorWorkspaceDef::NodeAliasMap::const_iterator endt = mDefinition->mAliasedNodes.end();

        const CompositorManager2 *compoManager = mDefinition->mCompositorManager;

        TextureGpu *finalTarget = getFinalTarget();

        while( itor != endt )
        {
            const CompositorNodeDef *nodeDef = compoManager->getNodeDefinition( itor->second );
            CompositorNode *newNode =
                OGRE_NEW CompositorNode( Id::generateNewId<CompositorNode>(), itor->first, nodeDef, this,
                                         mRenderSys, finalTarget );
            mNodeSequence.push_back( newNode );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::destroyAllNodes()
    {
        mValid = false;
        {
            CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
            CompositorNodeVec::const_iterator endt = mNodeSequence.end();

            while( itor != endt )
            {
                ( *itor )->destroyAllPasses();
                ++itor;
            }

            itor = mNodeSequence.begin();
            while( itor != endt )
                OGRE_DELETE *itor++;
            mNodeSequence.clear();
        }

        {
            CompositorShadowNodeVec::const_iterator itor = mShadowNodes.begin();
            CompositorShadowNodeVec::const_iterator endt = mShadowNodes.end();

            while( itor != endt )
            {
                ( *itor )->destroyAllPasses();
                ++itor;
            }

            itor = mShadowNodes.begin();
            while( itor != endt )
                OGRE_DELETE *itor++;
            mShadowNodes.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::connectAllNodes()
    {
        {
            // First connect the external dependencies, otherwise
            // the node could end up not being processed
            {
                CompositorWorkspaceDef::ChannelRouteList::const_iterator itor =
                    mDefinition->mExternalChannelRoutes.begin();
                CompositorWorkspaceDef::ChannelRouteList::const_iterator endt =
                    mDefinition->mExternalChannelRoutes.end();

                while( itor != endt )
                {
                    if( itor->outChannel >= mExternalRenderTargets.size() )
                    {
                        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                     "Workspace '" + mDefinition->mName.getFriendlyText() +
                                         "' expects"
                                         " at least " +
                                         StringConverter::toString( itor->outChannel ) +
                                         " external inputs but only " +
                                         StringConverter::toString( mExternalRenderTargets.size() ) +
                                         " were provided to addWorkspace.",
                                     "CompositorWorkspace::connectAllNodes" );
                    }

                    CompositorNode *node = findNode( itor->inNode );
                    node->connectExternalRT( mExternalRenderTargets[itor->outChannel], itor->inChannel );
                    ++itor;
                }
            }

            {
                CompositorWorkspaceDef::ChannelRouteList::const_iterator itor =
                    mDefinition->mExternalBufferChannelRoutes.begin();
                CompositorWorkspaceDef::ChannelRouteList::const_iterator endt =
                    mDefinition->mExternalBufferChannelRoutes.end();

                while( itor != endt )
                {
                    CompositorNode *node = findNode( itor->inNode );
                    node->connectExternalBuffer( mExternalBuffers[itor->outChannel], itor->inChannel );
                    ++itor;
                }
            }
        }

        CompositorNodeVec unprocessedList( mNodeSequence.begin(), mNodeSequence.end() );
        CompositorNodeVec processedList;
        processedList.reserve( mNodeSequence.size() );

        bool noneProcessed = false;

        while( !unprocessedList.empty() && !noneProcessed )
        {
            noneProcessed = true;
            CompositorNodeVec::iterator itor = unprocessedList.begin();
            CompositorNodeVec::iterator endt = unprocessedList.end();

            while( itor != endt )
            {
                CompositorNode *node = *itor;
                if( node->areAllInputsConnected() )
                {
                    // This node has no missing dependency, we can process it!
                    CompositorWorkspaceDef::ChannelRouteList::const_iterator itRoute =
                        mDefinition->mChannelRoutes.begin();
                    CompositorWorkspaceDef::ChannelRouteList::const_iterator enRoute =
                        mDefinition->mChannelRoutes.end();

                    // Connect all nodes according to our routing map. Perhaps I could've chosen a more
                    // efficient representation for lookup. Oh well, it's not like there's a 1000 nodes
                    // per workspace. If this is a hotspot, refactor.
                    while( itRoute != enRoute )
                    {
                        if( itRoute->outNode == node->getName() )
                        {
                            node->connectTo( itRoute->outChannel, findNode( itRoute->inNode, true ),
                                             itRoute->inChannel );
                        }
                        ++itRoute;
                    }

                    itRoute = mDefinition->mBufferChannelRoutes.begin();
                    enRoute = mDefinition->mBufferChannelRoutes.end();

                    while( itRoute != enRoute )
                    {
                        if( itRoute->outNode == node->getName() )
                        {
                            node->connectBufferTo( itRoute->outChannel,
                                                   findNode( itRoute->inNode, true ),
                                                   itRoute->inChannel );
                        }
                        ++itRoute;
                    }

                    // The processed list is now in order
                    processedList.push_back( *itor );

                    // Remove processed nodes from the list. We'll keep until there's no one left
                    itor = efficientVectorRemove( unprocessedList, itor );
                    endt = unprocessedList.end();

                    noneProcessed = false;
                }
                else
                {
                    ++itor;
                }
            }
        }

        // unprocessedList should be empty by now, or contain only disabled nodes.
        bool incomplete = false;
        {
            CompositorNodeVec::const_iterator itor = unprocessedList.begin();
            CompositorNodeVec::const_iterator endt = unprocessedList.end();

            while( itor != endt )
            {
                incomplete |= ( *itor )->getEnabled();
                ++itor;
            }
        }

        if( incomplete )
        {
            CompositorNodeVec::const_iterator itor = unprocessedList.begin();
            CompositorNodeVec::const_iterator endt = unprocessedList.end();
            while( itor != endt )
            {
                if( ( *itor )->getEnabled() )
                {
                    LogManager::getSingleton().logMessage(
                        "WARNING: Node '" + ( *itor )->getName().getFriendlyText() +
                        "' has the following "
                        "channels in a disconnected state. Workspace won't work until they're solved:" );

                    const CompositorChannelVec &inputChannels = ( *itor )->getInputChannel();
                    CompositorChannelVec::const_iterator itChannels = inputChannels.begin();
                    CompositorChannelVec::const_iterator enChannels = inputChannels.end();
                    while( itChannels != enChannels )
                    {
                        if( !*itChannels )
                        {
                            const size_t channelIdx =
                                static_cast<size_t>( itChannels - inputChannels.begin() );
                            LogManager::getSingleton().logMessage(
                                "\t\t\t Channel # " + StringConverter::toString( channelIdx ) );
                        }
                        ++itChannels;
                    }
                }

                ++itor;
            }
        }
        else
        {
            // We need mNodeSequence in the right order of execution!
            mNodeSequence.clear();
            mNodeSequence.insert( mNodeSequence.end(), processedList.begin(), processedList.end() );

            CompositorNodeVec::iterator itor = mNodeSequence.begin();
            CompositorNodeVec::iterator endt = mNodeSequence.end();

            while( itor != endt )
            {
                ( *itor )->createPasses();
                ++itor;
            }

            // Now manage automatic shadow nodes present PASS_SCENE passes
            //(when using SHADOW_NODE_FIRST_ONLY)
            setupPassesShadowNodes();

            // unprocessedList may not be empty if they were incomplete but disabled.
            mNodeSequence.insert( mNodeSequence.end(), unprocessedList.begin(), unprocessedList.end() );

            mValid = true;

            _notifyBarriersDirty();
        }

#if OGRE_PROFILING
        Profiler::getSingleton().reset( true );
#endif
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::clearAllConnections()
    {
        {
            CompositorNodeVec::iterator itor = mNodeSequence.begin();
            CompositorNodeVec::iterator endt = mNodeSequence.end();

            while( itor != endt )
            {
                ( *itor )->_notifyCleared();
                ++itor;
            }
        }

        {
            CompositorShadowNodeVec::const_iterator itor = mShadowNodes.begin();
            CompositorShadowNodeVec::const_iterator endt = mShadowNodes.end();

            while( itor != endt )
            {
                ( *itor )->destroyAllPasses();
                ++itor;
            }

            itor = mShadowNodes.begin();
            while( itor != endt )
                OGRE_DELETE *itor++;
            mShadowNodes.clear();
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::setupPassesShadowNodes()
    {
        CompositorShadowNodeVec::iterator itShadowNode = mShadowNodes.begin();
        CompositorShadowNodeVec::iterator enShadowNode = mShadowNodes.end();

        while( itShadowNode != enShadowNode )
        {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
            set<Camera *>::type usedCameras;
#endif
            Camera *lastCamera = 0;
            CompositorShadowNode *shadowNode = *itShadowNode;

            CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
            CompositorNodeVec::const_iterator endt = mNodeSequence.end();
            while( itor != endt )
            {
                const CompositorPassVec &passes = ( *itor )->_getPasses();
                CompositorPassVec::const_iterator itPasses = passes.begin();
                CompositorPassVec::const_iterator enPasses = passes.end();

                while( itPasses != enPasses )
                {
                    if( ( *itPasses )->getType() == PASS_SHADOWS )
                    {
                        OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassShadows *>( *itPasses ) );
                        CompositorPassShadows *pass = static_cast<CompositorPassShadows *>( *itPasses );
                        const FastArray<CompositorShadowNode *> &shadowNodes = pass->getShadowNodes();

                        FastArray<CompositorShadowNode *>::const_iterator itShadow = shadowNodes.begin();
                        FastArray<CompositorShadowNode *>::const_iterator enShadow = shadowNodes.end();

                        while( itShadow != enShadow )
                        {
                            if( shadowNode == *itShadow )
                            {
                                lastCamera = pass->getCullCamera();
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
                                usedCameras.insert( lastCamera );
#endif
                            }
                            ++itShadow;
                        }
                    }
                    else if( ( *itPasses )->getType() == PASS_SCENE )
                    {
                        assert( dynamic_cast<CompositorPassScene *>( *itPasses ) );
                        CompositorPassScene *pass = static_cast<CompositorPassScene *>( *itPasses );

                        if( shadowNode == pass->getShadowNode() )
                        {
                            ShadowNodeRecalculation recalc =
                                pass->getDefinition()->mShadowNodeRecalculation;

                            if( recalc == SHADOW_NODE_RECALCULATE )
                            {
                                // We're forced to recalculate anyway, save the new camera
                                lastCamera = pass->getCullCamera();
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
                                usedCameras.insert( lastCamera );
#endif
                            }
                            else if( recalc == SHADOW_NODE_FIRST_ONLY )
                            {
                                if( lastCamera != pass->getCullCamera() )
                                {
                                    // Either this is the first one, or camera changed.
                                    // We need to recalculate
                                    pass->_setUpdateShadowNode( true );
                                    lastCamera = pass->getCullCamera();

                                    // Performance warning check. Only on non-release builds.
                                    // We don't raise the log on SHADOW_NODE_RECALCULATE because
                                    // that's explicit. We assume the user knows what he's doing.
                                    //(may be he changed the objects without us knowing
                                    // through a listener)
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
                                    if( usedCameras.find( lastCamera ) != usedCameras.end() )
                                    {
                                        // clang-format off
                                        LogManager::getSingleton().logMessage(
                "\tPerformance Warning: Shadow Node '" + (*itor)->getName().getFriendlyText() +
                "' is forced to recalculate twice (or more) its contents for the same camera.\n"
                "\tThis happens when assigning a shadow node to a pass using a different camera "
                "and then using it back again in another pass with the older camera.\n"
                "\tYou can fix this by cloning the shadow node and using the clone for the pass with "
                "a different camera. But beware you'll be trading performance for more VRAM usage.\n"
                "\tOr you can ignore this warning." );
                                        // clang-format on
                                    }
                                    else
                                    {
                                        usedCameras.insert( lastCamera );
                                    }
#endif
                                }
                                else
                                {
                                    pass->_setUpdateShadowNode( false );
                                }
                            }
                        }
                    }
                    else if( ( *itPasses )->getType() == PASS_WARM_UP )
                    {
                        assert( dynamic_cast<CompositorPassWarmUp *>( *itPasses ) );
                        CompositorPassWarmUp *pass = static_cast<CompositorPassWarmUp *>( *itPasses );

                        if( shadowNode == pass->getShadowNode() )
                        {
                            ShadowNodeRecalculation recalc =
                                pass->getDefinition()->mShadowNodeRecalculation;

                            if( recalc == SHADOW_NODE_RECALCULATE )
                            {
                                // We're forced to recalculate anyway, save the new camera
                                lastCamera = pass->getCamera();
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
                                usedCameras.insert( lastCamera );
#endif
                            }
                            else if( recalc == SHADOW_NODE_FIRST_ONLY )
                            {
                                if( lastCamera != pass->getCamera() )
                                {
                                    // Either this is the first one, or camera changed.
                                    // We need to recalculate
                                    pass->_setUpdateShadowNode( true );
                                    lastCamera = pass->getCamera();

                                    // Performance warning check. Only on non-release builds.
                                    // We don't raise the log on SHADOW_NODE_RECALCULATE because
                                    // that's explicit. We assume the user knows what he's doing.
                                    //(may be he changed the objects without us knowing
                                    // through a listener)
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_LOW
                                    if( usedCameras.find( lastCamera ) != usedCameras.end() )
                                    {
                                        // clang-format off
                                        LogManager::getSingleton().logMessage(
                "\tPerformance Warning: Shadow Node '" + (*itor)->getName().getFriendlyText() +
                "' is forced to recalculate twice (or more) its contents for the same camera.\n"
                "\tThis happens when assigning a shadow node to a pass using a different camera "
                "and then using it back again in another pass with the older camera.\n"
                "\tYou can fix this by cloning the shadow node and using the clone for the pass with "
                "a different camera. But beware you'll be trading performance for more VRAM usage.\n"
                "\tOr you can ignore this warning." );
                                        // clang-format on
                                    }
                                    else
                                    {
                                        usedCameras.insert( lastCamera );
                                    }
#endif
                                }
                                else
                                {
                                    pass->_setUpdateShadowNode( false );
                                }
                            }
                        }
                    }
                    ++itPasses;
                }
                ++itor;
            }
            ++itShadowNode;
        }
    }
    //-----------------------------------------------------------------------------------
    CompositorNode *CompositorWorkspace::getLastEnabledNode()
    {
        CompositorNode *retVal = 0;

        CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
        CompositorNodeVec::const_iterator endt = mNodeSequence.end();

        while( itor != endt )
        {
            if( ( *itor )->getEnabled() )
                retVal = *itor;
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorNode *CompositorWorkspace::findNode( IdString aliasName, bool includeShadowNodes ) const
    {
        CompositorNode *retVal = findNodeNoThrow( aliasName, includeShadowNodes );

        if( !retVal )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Couldn't find node with name '" + aliasName.getFriendlyText() +
                             "'. includeShadowNodes = " +
                             ( includeShadowNodes ? String( "true" ) : String( "false" ) ),
                         "CompositorWorkspace::findNode" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorNode *CompositorWorkspace::findNodeNoThrow( IdString aliasName,
                                                          bool includeShadowNodes ) const
    {
        CompositorNode *retVal = 0;
        CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
        CompositorNodeVec::const_iterator endt = mNodeSequence.end();

        while( itor != endt && !retVal )
        {
            if( ( *itor )->getName() == aliasName )
                retVal = *itor;
            ++itor;
        }

        if( !retVal && includeShadowNodes )
            retVal = findShadowNode( aliasName );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const CompositorChannel &CompositorWorkspace::getGlobalTexture( IdString name ) const
    {
        size_t index;
        TextureDefinitionBase::TextureSource textureSource;
        mDefinition->getTextureSource( name, index, textureSource );
        return mGlobalTextures[index];
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::setListener( CompositorWorkspaceListener *listener )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "This method has been superseded by CompositorWorkspace::addListener "
                     "and CompositorWorkspace::removeListener.",
                     "CompositorWorkspace::setListener" );
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspaceListener *CompositorWorkspace::getListener() const
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "This method has been superseded by CompositorWorkspace::getListeners.",
                     "CompositorWorkspace::getListener" );
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::addListener( CompositorWorkspaceListener *listener )
    {
        mListeners.push_back( listener );
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::removeListener( CompositorWorkspaceListener *listener )
    {
        CompositorWorkspaceListenerVec::iterator itor =
            std::find( mListeners.begin(), mListeners.end(), listener );

        if( itor != mListeners.end() )
            mListeners.erase( itor );
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::fillPassesUsingRenderWindows(
        PassesByRenderWindowMap &passesUsingRenderWindows )
    {
        CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
        CompositorNodeVec::const_iterator endt = mNodeSequence.end();

        while( itor != endt )
        {
            CompositorNode *node = *itor;
            if( node->getEnabled() )
            {
                const CompositorPassVec &passes = node->_getPasses();

                CompositorPassVec::const_iterator itPass = passes.begin();
                CompositorPassVec::const_iterator enPass = passes.end();

                while( itPass != enPass )
                {
                    const RenderPassDescriptor *renderPassDesc = ( *itPass )->getRenderPassDesc();

                    if( renderPassDesc && !( *itPass )->getDefinition()->mSkipLoadStoreSemantics )
                    {
                        const size_t numColourEntries = renderPassDesc->getNumColourEntries();
                        for( size_t i = 0u; i < numColourEntries; ++i )
                        {
                            TextureGpu *texture;

                            // Add the pass only once
                            texture = renderPassDesc->mColour[i].texture;
                            if( texture && texture->isRenderWindowSpecific() )
                                passesUsingRenderWindows[texture].push_back( *itPass );
                            else
                            {
                                texture = renderPassDesc->mColour[i].resolveTexture;
                                if( texture && texture->isRenderWindowSpecific() )
                                    passesUsingRenderWindows[texture].push_back( *itPass );
                            }
                        }
                    }

                    ++itPass;
                }
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::recreateAllNodes()
    {
        createAllNodes();
        connectAllNodes();
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::reconnectAllNodes()
    {
        clearAllConnections();
        connectAllNodes();
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::resetAllNumPassesLeft()
    {
        CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
        CompositorNodeVec::const_iterator endt = mNodeSequence.end();

        while( itor != endt )
        {
            ( *itor )->resetAllNumPassesLeft();
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    Camera *CompositorWorkspace::findCamera( IdString cameraName ) const
    {
        return mSceneManager->findCamera( cameraName );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *CompositorWorkspace::getFinalTarget() const
    {
        TextureGpu *finalTarget = 0;
        if( !mExternalRenderTargets.empty() )
            finalTarget = mExternalRenderTargets.front();

        return finalTarget;
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_notifyBarriersDirty() { getCompositorManager()->_notifyBarriersDirty(); }
    //-----------------------------------------------------------------------------------
    CompositorManager2 *CompositorWorkspace::getCompositorManager()
    {
        return mDefinition->mCompositorManager;
    }
    //-----------------------------------------------------------------------------------
    const CompositorManager2 *CompositorWorkspace::getCompositorManager() const
    {
        return mDefinition->mCompositorManager;
    }
    //-----------------------------------------------------------------------------------
    size_t CompositorWorkspace::getFrameCount() const
    {
        return mDefinition->mCompositorManager->getFrameCount();
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_beginUpdate( bool forceBeginFrame, const bool bInsideAutoreleasePool )
    {
        if( !bInsideAutoreleasePool )
        {
            mRenderSys->compositorWorkspaceBegin( this, forceBeginFrame );
            return;
        }

        // We need to do this so that D3D9 (and D3D11?) knows which device
        // is active now, so that _beginFrame calls go to the right device.
        TextureGpu *finalTarget = getFinalTarget();
        if( finalTarget )
        {
            mRenderSys->_setCurrentDeviceFromTexture( finalTarget );
            if( finalTarget->isRenderWindowSpecific() || forceBeginFrame )
            {
                // Begin the frame
                mRenderSys->_beginFrame();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_endUpdate( bool forceEndFrame, const bool bInsideAutoreleasePool )
    {
        if( !bInsideAutoreleasePool )
        {
            mRenderSys->compositorWorkspaceEnd( this, forceEndFrame );
            return;
        }

        // We need to do this so that D3D9 (and D3D11?) knows which device
        // is active now, so that _endFrame calls go to the right device.
        TextureGpu *finalTarget = getFinalTarget();
        if( finalTarget )
        {
            mRenderSys->_setCurrentDeviceFromTexture( finalTarget );
            if( finalTarget->isRenderWindowSpecific() || forceEndFrame )
            {
                // End the frame
                mRenderSys->_endFrame();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_update( const bool bInsideAutoreleasePool )
    {
        if( !bInsideAutoreleasePool )
        {
            mRenderSys->compositorWorkspaceUpdate( this );
            return;
        }
        {
            CompositorWorkspaceListenerVec::const_iterator itor = mListeners.begin();
            CompositorWorkspaceListenerVec::const_iterator endt = mListeners.end();

            while( itor != endt )
            {
                ( *itor )->workspacePreUpdate( this );
                ++itor;
            }
        }

        TextureGpu *finalTarget = getFinalTarget();
        // We need to do this so that D3D9 (and D3D11?) knows which device
        // is active now, so that our calls go to the right device.
        mRenderSys->_setCurrentDeviceFromTexture( finalTarget );

        if( mCurrentWidth != finalTarget->getWidth() || mCurrentHeight != finalTarget->getHeight() ||
            mCurrentOrientationMode != finalTarget->getOrientationMode() )
        {
            // Main RenderTarget reference changed resolution. Some nodes may need to rebuild
            // their textures if they're based on mRenderWindow's resolution.
            mCurrentWidth = finalTarget->getWidth();
            mCurrentHeight = finalTarget->getHeight();
            mCurrentOrientationMode = finalTarget->getOrientationMode();

            {
                CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
                CompositorNodeVec::const_iterator endt = mNodeSequence.end();

                while( itor != endt )
                {
                    CompositorNode *node = *itor;
                    node->finalTargetResized01( finalTarget );
                    ++itor;
                }

                itor = mNodeSequence.begin();
                while( itor != endt )
                {
                    CompositorNode *node = *itor;
                    node->finalTargetResized02( finalTarget );
                    ++itor;
                }
            }

            {
                CompositorShadowNodeVec::const_iterator itor = mShadowNodes.begin();
                CompositorShadowNodeVec::const_iterator endt = mShadowNodes.end();

                while( itor != endt )
                {
                    CompositorShadowNode *node = *itor;
                    node->finalTargetResized01( finalTarget );
                    ++itor;
                }

                itor = mShadowNodes.begin();
                while( itor != endt )
                {
                    CompositorShadowNode *node = *itor;
                    node->finalTargetResized02( finalTarget );
                    ++itor;
                }
            }

            CompositorNodeVec allNodes;
            allNodes.reserve( mNodeSequence.size() + mShadowNodes.size() );
            allNodes.insert( allNodes.end(), mNodeSequence.begin(), mNodeSequence.end() );
            allNodes.insert( allNodes.end(), mShadowNodes.begin(), mShadowNodes.end() );
            TextureDefinitionBase::recreateResizableTextures01( mDefinition->mLocalTextureDefs,
                                                                mGlobalTextures, finalTarget );
            TextureDefinitionBase::recreateResizableTextures02( mDefinition->mLocalTextureDefs,
                                                                mGlobalTextures, allNodes, 0 );
            TextureDefinitionBase::recreateResizableBuffers(
                mDefinition->mLocalBufferDefs, mGlobalBuffers, finalTarget, mRenderSys, allNodes, 0 );
        }

        mDefinition->mCompositorManager->getBarrierSolver().assumeTransitions( mInitialLayouts );

        CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
        CompositorNodeVec::const_iterator endt = mNodeSequence.end();

        while( itor != endt )
        {
            CompositorNode *node = *itor;
            if( node->getEnabled() )
            {
                if( node->areAllInputsConnected() )
                {
                    node->_update( (Camera *)0, mSceneManager );
                }
                else
                {
                    // If we get here, this means a node didn't have all of its input channels connected,
                    // but we ignored it because the node was disabled. But now it is enabled again.
                    LogManager::getSingleton().logMessage(
                        "ERROR: Invalid Node '" + node->getName().getFriendlyText() +
                        "' was re-enabled without calling CompositorWorkspace::clearAllConnections" );
                    mValid = false;
                }
            }
            ++itor;
        }

        CompositorWorkspaceListenerVec::const_iterator itor2 = mListeners.begin();
        CompositorWorkspaceListenerVec::const_iterator end2 = mListeners.end();

        while( itor2 != end2 )
        {
            ( *itor2 )->workspacePosUpdate( this );
            ++itor2;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_swapFinalTarget( vector<TextureGpu *>::type &swappedTargets )
    {
        CompositorChannelVec::const_iterator itor = mExternalRenderTargets.begin();
        CompositorChannelVec::const_iterator endt = mExternalRenderTargets.end();

        while( itor != endt )
        {
            TextureGpu *externalTarget = *itor;
            const bool alreadySwapped = std::find( swappedTargets.begin(), swappedTargets.end(),
                                                   externalTarget ) != swappedTargets.end();

            if( !alreadySwapped )
            {
                externalTarget->swapBuffers();
                swappedTargets.push_back( externalTarget );
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorWorkspace::_validateFinalTarget()
    {
        TextureGpu *finalTarget = getFinalTarget();
        if( finalTarget )
            mRenderSys->_setCurrentDeviceFromTexture( finalTarget );
    }
    //-----------------------------------------------------------------------------------
    CompositorShadowNode *CompositorWorkspace::findShadowNode( IdString nodeDefName ) const
    {
        CompositorShadowNode *retVal = 0;

        CompositorShadowNodeVec::const_iterator itor = mShadowNodes.begin();
        CompositorShadowNodeVec::const_iterator endt = mShadowNodes.end();

        while( itor != endt && !retVal )
        {
            if( ( *itor )->getName() == nodeDefName )
                retVal = *itor;
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorShadowNode *CompositorWorkspace::findOrCreateShadowNode( IdString nodeDefName,
                                                                       bool &bCreated )
    {
        CompositorShadowNode *retVal = findShadowNode( nodeDefName );
        bCreated = false;

        if( !retVal )
        {
            // Not found, create one.
            const CompositorManager2 *compoManager = mDefinition->mCompositorManager;
            TextureGpu *finalTarget = getFinalTarget();
            const CompositorShadowNodeDef *def = compoManager->getShadowNodeDefinition( nodeDefName );
            retVal = OGRE_NEW CompositorShadowNode( Id::generateNewId<CompositorNode>(), def, this,
                                                    mRenderSys, finalTarget );
            mShadowNodes.push_back( retVal );
            bCreated = true;
        }

        return retVal;
    }
}  // namespace Ogre
