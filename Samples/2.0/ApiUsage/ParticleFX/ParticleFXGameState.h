
#ifndef Demo_ParticleFXGameState_H_
#define Demo_ParticleFXGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class ParticleFXGameState : public TutorialGameState
    {
    public:
        ParticleFXGameState( const Ogre::String &helpDescription );

        void createScene01() override;
    };
}  // namespace Demo

#endif
