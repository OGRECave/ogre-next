
#ifndef _Demo_Forward3DGameState_H_
#define _Demo_Forward3DGameState_H_

#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "OgreCommon.h"

namespace Demo
{
    class Forward3DGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        Ogre::uint32 mCurrentForward3DPreset;

        Ogre::LightArray mGeneratedLights;
        Ogre::uint32 mNumLights;
        float mLightRadius;
        bool mLowThreshold;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void changeForward3DPreset( bool goForward );

        void generateLights();

    public:
        Forward3DGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
