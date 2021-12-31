
#ifndef _Demo_UpdatingDecalsAndAreaLightTexGameState_H_
#define _Demo_UpdatingDecalsAndAreaLightTexGameState_H_

#include "OgreOverlay.h"
#include "OgreOverlayPrerequisites.h"
#include "OgrePrerequisites.h"

#include "TutorialGameState.h"

namespace Demo
{
    static const Ogre::uint32 c_numAreaLights = 4u;

    class UpdatingDecalsAndAreaLightTexGameState : public TutorialGameState
    {
        Ogre::SceneNode *mLightNodes[c_numAreaLights];
        Ogre::Light *mAreaLights[c_numAreaLights];

        bool mUseTextureFromFile[c_numAreaLights];
        float mLightTexRadius[c_numAreaLights];

        Ogre::TextureGpu *mAreaMaskTex;
        bool mUseSynchronousMethod;

        /** Creates an Image of a hollow rectangle on the GPU
        @param radius
            Radius of the hollow rectangle (i.e. width of the "lines")
        @param outImage
            Image containing the texture to upload to GPU
        */
        void createAreaMask( float radius, Ogre::Image2 &outImage );
        /// Creates the Mesh for the billboards
        void createAreaPlaneMesh();
        /// Setups a datablock (material) for the billboard showing where the light is
        /// emitting so that it can use the same texture the light is using.
        ///
        /// Must be called again whenever the texture changes (i.e. setupLightTexture was called)
        Ogre::HlmsDatablock *setupDatablockTextureForLight( Ogre::Light *light, size_t idx );
        /// Creates a billboard showing where the given light is emitting
        void createPlaneForAreaLight( Ogre::Light *light, size_t idx );

        void createLight( const Ogre::Vector3 &position, size_t idx );
        /// Calls createAreaMask and uploads it to the GPU texture.
        void setupLightTexture( size_t idx );

        void generateDebugText( float timeSinceLast, Ogre::String &outText ) override;

    public:
        UpdatingDecalsAndAreaLightTexGameState( const Ogre::String &helpDescription );

        void createScene01() override;
        void destroyScene() override;

        void update( float timeSinceLast ) override;

        void keyReleased( const SDL_KeyboardEvent &arg ) override;
    };
}  // namespace Demo

#endif
