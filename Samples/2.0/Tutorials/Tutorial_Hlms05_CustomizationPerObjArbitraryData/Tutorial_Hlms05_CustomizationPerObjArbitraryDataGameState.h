
#ifndef Demo_Hlms05CustomizationPerObjArbitraryDataGameState_H
#define Demo_Hlms05CustomizationPerObjArbitraryDataGameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class Hlms05CustomizationPerObjArbitraryDataGameState : public TutorialGameState
    {
        std::vector<Ogre::Item *> mChangingItems;

    public:
        Hlms05CustomizationPerObjArbitraryDataGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
