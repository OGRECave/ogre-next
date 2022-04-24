
#ifndef Demo_AtmosphereGameState_H_
#define Demo_AtmosphereGameState_H_

#include "OgrePrerequisites.h"

#include "OgreAtmosphereNpr.h"

#include "TutorialGameState.h"

namespace Demo
{
    class AtmosphereGameState : public TutorialGameState
    {
        float mTimeOfDay;
        float mAzimuth;

        bool mMultiplePresets;

        Ogre::AtmosphereNpr *mAtmosphere;
        Ogre::Light *mSunLight;

        template <typename T>
        void description( const char *keys, const char *setting, T value, Ogre::String &outText,
                          bool bForce = false ) const;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        AtmosphereGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
