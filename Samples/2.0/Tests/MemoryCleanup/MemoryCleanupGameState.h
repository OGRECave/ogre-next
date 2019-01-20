
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

        void testSequence(void);

        void createCleanupScene(void);
        void destroyCleanupScene(void);
        bool isSceneLoaded(void) const;

    public:
        MemoryCleanupGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
