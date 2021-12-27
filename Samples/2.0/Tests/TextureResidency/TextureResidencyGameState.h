
#ifndef _Demo_TextureResidencyGameState_H_
#define _Demo_TextureResidencyGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "OgreGpuResource.h"
#include "TutorialGameState.h"


namespace Demo
{
    class TextureResidencyGameState : public TutorialGameState
    {
        std::vector<Ogre::TextureGpu*>		mTextures;
        std::vector<Ogre::GpuResidency::GpuResidency>   mChangeLog;
        size_t                              mNumInitialTextures;
        bool                                mWaitForStreamingCompletion;

        struct VisibleItem
        {
            Ogre::Item          *item;
            Ogre::HlmsDatablock *datablock;
        };

        typedef std::vector<VisibleItem> VisibleItemVec;
        VisibleItemVec  mVisibleItems;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void switchTextureResidency( int targetResidency );

        void enableHeavyRamMode();
        void disableHeavyRamMode();
        bool isInHeavyRamMode() const;

        void testSequence();
        void testRandom();
        void testRamStress();

        void showTexturesOnScreen();
        void hideTexturesFromScreen();
        bool isShowingTextureOnScreen() const;

    public:
        TextureResidencyGameState( const Ogre::String &helpDescription );

        virtual void createScene01();

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
