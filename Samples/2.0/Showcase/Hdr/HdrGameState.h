
#ifndef _Demo_HdrGameState_H_
#define _Demo_HdrGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class HdrGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;
        Ogre::uint32 mCurrentPreset;

        Ogre::String mPresetName;
        float mExposure;
        float mMinAutoExposure;
        float mMaxAutoExposure;
        float mBloomFullThreshold;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void switchPreset( int direction = 1 );

    public:
        HdrGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
