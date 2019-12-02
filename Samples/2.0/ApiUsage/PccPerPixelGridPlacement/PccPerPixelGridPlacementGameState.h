
#ifndef _Demo_PccPerPixelGridPlacementGameState_H_
#define _Demo_PccPerPixelGridPlacementGameState_H_

#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Ogre
{
    class ParallaxCorrectedCubemapAuto;
    class PccPerPixelGridPlacement;
    class HlmsPbsDatablock;
}  // namespace Ogre

namespace Demo
{
    class PccPerPixelGridPlacementGameState : public TutorialGameState
    {
        Ogre::SceneNode *mLightNodes[3];

        Ogre::ParallaxCorrectedCubemapAuto *mParallaxCorrectedCubemap;
        bool mUseDpm2DArray;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

        void setupParallaxCorrectCubemaps( void );
        void forceUpdateAllProbes( void );

    public:
        PccPerPixelGridPlacementGameState( const Ogre::String &helpDescription );

        virtual void createScene01( void );
        virtual void destroyScene( void );

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}  // namespace Demo

#endif
