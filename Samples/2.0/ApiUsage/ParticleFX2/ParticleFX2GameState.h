
#ifndef Demo_ParticleFX2GameState_H
#define Demo_ParticleFX2GameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class ParticleFX2GameState : public TutorialGameState
    {
        Ogre::SceneNode *mParticleSystem3_RootSceneNode;
        Ogre::SceneNode *mParticleSystem3_EmmitterSceneNode;
        float mTime;

    public:
        ParticleFX2GameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;
    };
}  // namespace Demo

#endif
