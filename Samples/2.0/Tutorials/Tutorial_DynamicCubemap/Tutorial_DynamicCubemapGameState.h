
#ifndef _Demo_DynamicCubemapGameState_H_
#define _Demo_DynamicCubemapGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"

namespace Demo
{
    enum IblQuality
    {
        MipmapsLowest,
        IblLow,
        IblHigh
    };

    class DynamicCubemapGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mSceneNode[16];

        Ogre::SceneNode     *mLightNodes[3];

        bool                mAnimateObjects;

        IblQuality          mIblQuality;

        std::vector<Ogre::MovableObject*> mSpheres;
        std::vector<Ogre::MovableObject*> mObjects;

        Ogre::Camera                *mCubeCamera;
        Ogre::TextureGpu            *mDynamicCubemap;
        Ogre::CompositorWorkspace   *mDynamicCubemapWorkspace;

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        DynamicCubemapGameState( const Ogre::String &helpDescription );

        Ogre::CompositorWorkspace* setupCompositor(void);

        virtual void createScene01(void);
        virtual void destroyScene(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
