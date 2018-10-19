
#ifndef _Demo_Particle_Systems_H_
#define _Demo_Particle_Systems_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class ParticlesGameState : public TutorialGameState
    {
        Ogre::ParticleSystem* mParticleSystem;
        Ogre::SceneNode* mParticlesNode;
        Ogre::Real mAngle;

    public:
        ParticlesGameState(const Ogre::String &helpDescription);

        virtual void createScene01(void);
        virtual void update(float timeSinceLast);
    };
}

#endif
