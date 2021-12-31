
#ifndef _Demo_AreaApproxLightsGameState_H_
#define _Demo_AreaApproxLightsGameState_H_

#include "OgreOverlayPrerequisites.h"

#include "OgrePrerequisites.h"

#include "OgreOverlay.h"
#include "TutorialGameState.h"

namespace Demo
{
    class AreaApproxLightsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];
        Ogre::Light *mAreaLights[2];
        Ogre::HlmsUnlitDatablock *mAreaLightPlaneDatablocks[2];

        bool mAnimateObjects;

        Ogre::TextureGpu *mAreaMaskTex;

        void createAreaMask();
        void createAreaPlaneMesh();
        Ogre::HlmsUnlitDatablock *createPlaneForAreaLight( Ogre::Light *light );

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        AreaApproxLightsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
