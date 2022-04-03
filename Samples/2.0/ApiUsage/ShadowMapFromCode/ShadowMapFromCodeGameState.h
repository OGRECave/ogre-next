
#ifndef _Demo_ShadowMapFromCodeGameState_H_
#define _Demo_ShadowMapFromCodeGameState_H_

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

#define USE_STATIC_BRANCHING_FOR_SHADOWMAP_LIGHTS 1

namespace Demo
{
    class ShadowMapFromCodeGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

#ifdef USE_STATIC_BRANCHING_FOR_SHADOWMAP_LIGHTS
        Ogre::SceneNode *mLightNodes[5];
#else
        Ogre::SceneNode *mLightNodes[3];
#endif

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
        ShadowMapFromCodeGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
