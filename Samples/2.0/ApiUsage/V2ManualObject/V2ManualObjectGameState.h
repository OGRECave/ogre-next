
#ifndef _Demo_V2ManualObjectGameState_H_
#define _Demo_V2ManualObjectGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class V2ManualObjectGameState : public TutorialGameState
    {
    public:
        V2ManualObjectGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;

    private:
        Ogre::ManualObject *mManualObject;
        bool mFirstFrame;
        std::vector<Ogre::Vector3> mVertices;
        float mAccumulator;
        bool mAnimate;

        void fillBuffer( float uvOffset );
    };
}  // namespace Demo

#endif
