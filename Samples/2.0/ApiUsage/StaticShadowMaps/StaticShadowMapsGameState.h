
#ifndef _Demo_StaticShadowMapsGameState_H_
#define _Demo_StaticShadowMapsGameState_H_

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Ogre
{
    class CompositorShadowNode;
}

namespace Demo
{
    class StaticShadowMapsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;
        bool mUpdateShadowMaps;
        Ogre::CompositorShadowNode *mShadowNode;

        Ogre::v1::Overlay *mDebugOverlayPSSM;
        Ogre::v1::Overlay *mDebugOverlaySpotlights;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void createShadowMapDebugOverlays();

    public:
        StaticShadowMapsGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
