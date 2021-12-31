
#ifndef _Demo_InstantRadiosityGameState_H_
#define _Demo_InstantRadiosityGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

#include "OgreLight.h"

#include "SdlEmulationLayer.h"
#if OGRE_USE_SDL2
#    include "SDL_keyboard.h"
#endif

namespace Ogre
{
    class InstantRadiosity;
    class HlmsPbsDatablock;
    class IrradianceVolume;
}  // namespace Ogre

namespace Demo
{
    class InstantRadiosityGameState : public TutorialGameState
    {
        Ogre::SceneNode *mLightNode;
        Ogre::Light *mLight;
        Ogre::Light::LightTypes mCurrentType;

        Ogre::InstantRadiosity *mInstantRadiosity;
        Ogre::IrradianceVolume *mIrradianceVolume;
        Ogre::Real mIrradianceCellSize;

        std::map<SDL_Keycode, SDL_Keysym> mKeysHold;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void createLight();
        void updateIrradianceVolume();

    public:
        InstantRadiosityGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyPressed( const SDL_KeyboardEvent &arg ) override;
        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
