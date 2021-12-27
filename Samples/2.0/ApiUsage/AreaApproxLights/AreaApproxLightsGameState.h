
#ifndef _Demo_AreaApproxLightsGameState_H_
#define _Demo_AreaApproxLightsGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "TutorialGameState.h"


namespace Demo
{
    class AreaApproxLightsGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mSceneNode[16];

        Ogre::SceneNode     *mLightNodes[3];
        Ogre::Light         *mAreaLights[2];
        Ogre::HlmsUnlitDatablock *mAreaLightPlaneDatablocks[2];

        bool                mAnimateObjects;

        Ogre::TextureGpu    *mAreaMaskTex;

        void createAreaMask();
        void createAreaPlaneMesh();
        Ogre::HlmsUnlitDatablock* createPlaneForAreaLight( Ogre::Light *light );

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        AreaApproxLightsGameState( const Ogre::String &helpDescription );

        virtual void createScene01();
        virtual void destroyScene();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
