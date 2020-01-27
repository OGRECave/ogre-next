
#ifndef _Demo_IesProfilesGameState_H_
#define _Demo_IesProfilesGameState_H_

#include "OgrePrerequisites.h"

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"

#include "TutorialGameState.h"

namespace Ogre
{
    class LightProfiles;
}

namespace Demo
{
    static const Ogre::uint32 c_numAreaLights = 4u;

    class IesProfilesGameState : public TutorialGameState
    {
        Ogre::LightProfiles *mLightProfiles;

    public:
        IesProfilesGameState( const Ogre::String &helpDescription );

        virtual void createScene01( void );
        virtual void destroyScene( void );

        virtual void update( float timeSinceLast );
    };
}  // namespace Demo

#endif
