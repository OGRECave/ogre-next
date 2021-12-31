
#ifndef _Demo_ShadowMapDebuggingGameState_H_
#define _Demo_ShadowMapDebuggingGameState_H_

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Demo
{
    class ShadowMapDebuggingGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        Ogre::v1::Overlay *mDebugOverlayPSSM;
        Ogre::v1::Overlay *mDebugOverlaySpotlights;

        /// Chooses between compute & pixel shader based ESM shadow node.
        /// Compute shader filter is faster for large kernels; but beware
        /// of mobile hardware where compute shaders are slow)
        /// Pixel shader filter is faster for small kernels, also to use as a fallback
        /// on GPUs that don't support compute shaders, or where compute shaders are slow).
        /// For reference large kernels means kernelRadius > 2 (approx)
        const char *chooseEsmShadowNode();
        void setupShadowNode( bool forEsm );

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void createShadowMapDebugOverlays();
        void destroyShadowMapDebugOverlays();

    public:
        ShadowMapDebuggingGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
