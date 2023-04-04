
#ifndef Demo_ParticleFXGameState_H_
#define Demo_ParticleFXGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class ParticleFXGameState : public TutorialGameState
    {
        Ogre::SceneNode *mParticleSystem3_RootSceneNode;
        Ogre::SceneNode *mParticleSystem3_EmmitterSceneNode;
        float mTime;

    public:
        ParticleFXGameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
