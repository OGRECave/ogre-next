
#ifndef _Demo_MemoryCleanupGameState_H_
#define _Demo_MemoryCleanupGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "OgreGpuResource.h"
#include "TutorialGameState.h"


namespace Demo
{
    class MemoryCleanupGameState : public TutorialGameState
    {
        struct VisibleItem
        {
            Ogre::Item          *item;
            Ogre::HlmsDatablock *datablock;
        };

        typedef std::vector<VisibleItem> VisibleItemVec;
        VisibleItemVec  mVisibleItems;

        bool mReleaseMemoryOnCleanup;
        bool mReleaseGpuMemory;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void testSequence();

        void createCleanupScene();
        void destroyCleanupScene();
        bool isSceneLoaded() const;

    public:
        MemoryCleanupGameState( const Ogre::String &helpDescription );

        virtual void createScene01();
        virtual void destroyScene();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
