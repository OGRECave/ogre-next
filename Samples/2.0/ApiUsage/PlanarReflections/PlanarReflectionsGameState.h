
#ifndef _Demo_PlanarReflectionsGameState_H_
#define _Demo_PlanarReflectionsGameState_H_

#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Ogre
{
    class PlanarReflections;
}

namespace Demo
{
    class PlanarReflectionsWorkspaceListener;

    class PlanarReflectionsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        Ogre::PlanarReflections *mPlanarReflections;
        PlanarReflectionsWorkspaceListener *mWorkspaceListener;

        void createReflectiveSurfaces();

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        PlanarReflectionsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
