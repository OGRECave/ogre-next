
#ifndef _Demo_VoxelizerGameState_H_
#define _Demo_VoxelizerGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    class VoxelizerGameState : public TutorialGameState
    {
    public:
        VoxelizerGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void update( float timeSinceLast );
    };
}

#endif
