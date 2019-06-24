
#ifndef _Demo_VoxelizerGameState_H_
#define _Demo_VoxelizerGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Ogre
{
    class VctVoxelizer;
    class VctLighting;
}

namespace Demo
{
    class VoxelizerGameState : public TutorialGameState
    {
        Ogre::VctVoxelizer  *mVoxelizer;
        Ogre::VctLighting   *mVctLighting;

    public:
        VoxelizerGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);
        virtual void update( float timeSinceLast );
    };
}

#endif
