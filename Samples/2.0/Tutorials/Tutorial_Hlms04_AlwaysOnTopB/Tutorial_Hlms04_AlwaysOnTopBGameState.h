
#ifndef Demo_Hlms04AlwaysOnTopBGameState_H
#define Demo_Hlms04AlwaysOnTopBGameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Hlms04AlwaysOnTopBGameState : public TutorialGameState
    {
        std::vector<Ogre::Item *> mClones;

        void createBar( const bool bAlwaysOnTop );

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        Hlms04AlwaysOnTopBGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;
    };
}  // namespace Demo

#endif
