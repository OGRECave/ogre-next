
#ifndef Demo_Hlms05CustomizationPerObjDataGameState_H
#define Demo_Hlms05CustomizationPerObjDataGameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Hlms05CustomizationPerObjDataGameState : public TutorialGameState
    {
        std::vector<Ogre::Item *> mChangingItems;

    public:
        Hlms05CustomizationPerObjDataGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
