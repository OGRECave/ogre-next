
#ifndef _Demo_DecalsGameState_H_
#define _Demo_DecalsGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    struct DebugDecalVisual
    {
        Ogre::Item *plane;
        Ogre::Item *cube;
        Ogre::SceneNode *sceneNode;
    };

    class DecalsGameState : public TutorialGameState
    {
        Ogre::SceneNode *mSceneNode[16];

        Ogre::SceneNode *mLightNodes[3];

        bool mAnimateObjects;

        size_t mNumSpheres;
        Ogre::uint8 mTransparencyMode;
        float mTransparencyValue;

        DebugDecalVisual *mDecalDebugVisual;

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

        void createDecalDebugData();
        DebugDecalVisual *attachDecalDebugHelper( Ogre::SceneNode *decalNode );
        void destroyDecalDebugHelper( DebugDecalVisual *decalDebugVisual );
        void setTransparencyToMaterials();

    public:
        DecalsGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
