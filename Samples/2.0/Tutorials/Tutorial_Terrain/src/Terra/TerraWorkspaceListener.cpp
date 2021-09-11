
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

        const CompositorPassDef *definition = pass->getDefinition();
        if( definition->getType() != PASS_SCENE )
            return;  // We don't care

        OGRE_ASSERT_HIGH( dynamic_cast<const CompositorPassSceneDef *>( definition ) );
        const CompositorPassSceneDef *sceneDef =
            static_cast<const CompositorPassSceneDef *>( definition );

        if( sceneDef->mShadowNodeRecalculation == SHADOW_NODE_CASTER_PASS )
            return;  // passEarlyPreExecute handled this

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
