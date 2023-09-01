
#ifndef Demo_ParticleFX2GameState_H
#define Demo_ParticleFX2GameState_H

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    namespace BlendingMethod
    {
        enum BlendingMethod
        {
            AlphaHashing,
            AlphaHashingA2C,
            AlphaBlending,
            NumBlendingMethods
        };
    }

    class ParticleFX2GameState : public TutorialGameState
    {
        Ogre::SceneNode *mParticleSystem3_RootSceneNode;
        Ogre::SceneNode *mParticleSystem3_EmmitterSceneNode;
        float mTime;

        BlendingMethod::BlendingMethod mCurrBlendingMethod;

        void applyBlendingMethodToMaterials();
        void applyBlendingMethodToMaterial( Ogre::HlmsDatablock *datablock );

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        ParticleFX2GameState( const Ogre::String &helpDescription );

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
