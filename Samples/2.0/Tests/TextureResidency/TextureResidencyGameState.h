
#ifndef _Demo_TextureResidencyGameState_H_
#define _Demo_TextureResidencyGameState_H_

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "OgreGpuResource.h"
#include "TutorialGameState.h"

namespace Demo
{
    class TextureResidencyGameState : public TutorialGameState
    {
        std::vector<Ogre::TextureGpu *> mTextures;
        std::vector<Ogre::GpuResidency::GpuResidency> mChangeLog;
        size_t mNumInitialTextures;
        bool mWaitForStreamingCompletion;

        struct VisibleItem
        {
            Ogre::Item *item;
            Ogre::HlmsDatablock *datablock;
        };

        typedef std::vector<VisibleItem> VisibleItemVec;
        VisibleItemVec mVisibleItems;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

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

        void createScene01() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
