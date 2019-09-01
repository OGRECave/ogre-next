
#ifndef _Demo_InstancedStereoGameState_H_
#define _Demo_InstancedStereoGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "TutorialGameState.h"

#include "OgreCommon.h"

namespace Demo
{
    class InstancedStereoGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mSceneNode[16];

        Ogre::SceneNode     *mLightNodes[3];

        bool                mAnimateObjects;

        Ogre::uint32        mCurrentForward3DPreset;

        Ogre::LightArray    mGeneratedLights;
        Ogre::uint32        mNumLights;
        float               mLightRadius;
        bool                mLowThreshold;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void changeForward3DPreset( bool goForward );

        void generateLights(void);

    public:
        InstancedStereoGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
