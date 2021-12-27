
#ifndef _Demo_GraphicsGameState_H_
#define _Demo_GraphicsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "OgreVector3.h"

namespace Demo
{
    class GraphicsSystem;

    class GraphicsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode;
        Ogre::Vector3 mLastPosition;
        Ogre::Vector3 mCurrentPosition;

        bool        mEnableInterpolation;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        GraphicsGameState( const Ogre::String &helpDescription );

        Ogre::Vector3& _getLastPositionRef()        { return mLastPosition; }
        Ogre::Vector3& _getCurrentPositionRef()      { return mCurrentPosition; }

        virtual void createScene01();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
