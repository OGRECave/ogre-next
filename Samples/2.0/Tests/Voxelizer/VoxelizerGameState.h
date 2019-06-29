
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

        Ogre::uint32    mDebugVisualizationMode;

        void cycleVisualizationMode();

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        VoxelizerGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);
        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
