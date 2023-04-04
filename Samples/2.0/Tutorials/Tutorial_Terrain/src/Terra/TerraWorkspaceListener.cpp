/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

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

#include "Terra/TerraWorkspaceListener.h"

#include "Terra/Hlms/OgreHlmsTerra.h"
#include "Terra/Terra.h"

#include "Compositor/OgreCompositorShadowNode.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "OgreCamera.h"

namespace Ogre
{
    TerraWorkspaceListener::TerraWorkspaceListener( HlmsTerra *hlmsTerra ) :
        m_hlmsTerra( hlmsTerra ),
        m_terraNeedsRestoring( false )
    {
    }
    //-------------------------------------------------------------------------
    TerraWorkspaceListener::~TerraWorkspaceListener() {}
    //-------------------------------------------------------------------------
    void TerraWorkspaceListener::passPreExecute( CompositorPass *pass )
    {
        const CompositorPassDef *definition = pass->getDefinition();
        if( definition->getType() != PASS_SCENE )
            return;  // We don't care

        OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef *>( definition ) );
        const CompositorPassSceneDef *sceneDef =
            static_cast<const CompositorPassSceneDef *>( definition );

        if( sceneDef->mShadowNodeRecalculation != SHADOW_NODE_CASTER_PASS )
            return;

        OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassScene *>( pass ) );
        const CompositorPassScene *passScene = static_cast<const CompositorPassScene *>( pass );

        const Camera *renderingCamera = passScene->getCamera();
        const SceneManager *sceneManager = renderingCamera->getSceneManager();

        const CompositorShadowNode *shadowNode = sceneManager->getCurrentShadowNode();

        const Light *light = shadowNode->getLightAssociatedWith( definition->mShadowMapIdx );

        if( light->getType() != Light::LT_DIRECTIONAL )
        {
            const bool needsRestoring = m_terraNeedsRestoring;

            const FastArray<Terra *> &linkedTerras = m_hlmsTerra->getLinkedTerras();

            FastArray<Terra *>::const_iterator itor = linkedTerras.begin();
            FastArray<Terra *>::const_iterator endt = linkedTerras.end();

            while( itor != endt )
            {
                Terra *terra = *itor;
                if( !needsRestoring )
                {
                    // This is our first time rendering non-directional shadows
                    // We have to save the state
                    terra->_swapSavedState();
                }
                terra->setCastShadows( true );
                terra->setCamera( renderingCamera );

                // Use 5.0f as epsilon because that guarantees us
                // it will never trigger a shadow recalculation
                terra->update( Ogre::Vector3::ZERO, 5.0f );
                ++itor;
            }

            m_terraNeedsRestoring = true;
        }
        else
        {
            const FastArray<Terra *> &linkedTerras = m_hlmsTerra->getLinkedTerras();

            FastArray<Terra *>::const_iterator itor = linkedTerras.begin();
            FastArray<Terra *>::const_iterator endt = linkedTerras.end();

            while( itor != endt )
            {
                ( *itor )->setCastShadows( false );
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void TerraWorkspaceListener::passSceneAfterShadowMaps( CompositorPassScene *pass )
    {
        if( !m_terraNeedsRestoring )
            return;  // Nothing to restore

        if( pass )  // Can be nullptr if caller is a PASS_WARM_UP
        {
            const CompositorPassDef *definition = pass->getDefinition();
            if( definition->getType() != PASS_SCENE )
                return;  // We don't care

            OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef *>( definition ) );
            const CompositorPassSceneDef *sceneDef =
                static_cast<const CompositorPassSceneDef *>( definition );

            if( sceneDef->mShadowNodeRecalculation == SHADOW_NODE_CASTER_PASS )
                return;  // passPreExecute handled this
        }

        const FastArray<Terra *> &linkedTerras = m_hlmsTerra->getLinkedTerras();

        FastArray<Terra *>::const_iterator itor = linkedTerras.begin();
        FastArray<Terra *>::const_iterator endt = linkedTerras.end();

        while( itor != endt )
        {
            ( *itor )->setCamera( 0 );
            ( *itor )->_swapSavedState();
            ++itor;
        }

        m_terraNeedsRestoring = false;
    }
}  // namespace Ogre
