
#ifndef _Demo_Tutorial_TextureBakingGameState_H_
#define _Demo_Tutorial_TextureBakingGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "TutorialGameState.h"


namespace Demo
{
    static const Ogre::uint32 c_numAreaLights = 4u;

    namespace RenderingMode
    {
        enum RenderingMode
        {
            ShowRenderScene,
            ShowSceneWithBakedTexture,
            ShowBakedTexture,
        };
    }

    class Tutorial_TextureBakingGameState : public TutorialGameState
    {
        Ogre::Light         *mAreaLights[c_numAreaLights];
        Ogre::TextureGpu    *mAreaMaskTex;

        Ogre::TextureGpu            *mBakedResult;
        Ogre::CompositorWorkspace   *mBakedWorkspace;
        Ogre::CompositorWorkspace   *mShowBakedTexWorkspace;

        Ogre::Item      *mFloorRender;
        Ogre::Item      *mFloorBaked;

        RenderingMode::RenderingMode    mRenderingMode;
        bool            mBakeEveryFrame;

        /// Creates the Mesh for the billboards
        void createAreaPlaneMesh(void);
        /// Setups a datablock (material) for the billboard showing where the light is
        /// emitting so that it can use the same texture the light is using.
        ///
        /// Must be called again whenever the texture changes (i.e. setupLightTexture was called)
        Ogre::HlmsDatablock* setupDatablockTextureForLight( Ogre::Light *light, size_t idx );
        /// Creates a billboard showing where the given light is emitting
        void createPlaneForAreaLight( Ogre::Light *light, size_t idx );

        void createLight( const Ogre::Vector3 &position, size_t idx );
        /// Calls createAreaMask and uploads it to the GPU texture.
        void setupLightTexture( size_t idx );

        void createBakingTexture(void);
        void updateBakingTexture(void);

        void updateRenderingMode(void);

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        Tutorial_TextureBakingGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
