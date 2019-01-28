
#ifndef _Demo_UpdatingDecalsAndAreaLightTexGameState_H_
#define _Demo_UpdatingDecalsAndAreaLightTexGameState_H_

#include "OgrePrerequisites.h"
#include "OgreOverlayPrerequisites.h"
#include "OgreOverlay.h"
#include "TutorialGameState.h"


namespace Demo
{
    static const Ogre::uint32 c_numAreaLights = 4u;

    class UpdatingDecalsAndAreaLightTexGameState : public TutorialGameState
    {
        Ogre::SceneNode     *mLightNodes[c_numAreaLights];
        Ogre::Light         *mAreaLights[c_numAreaLights];

        bool                mUseTextureFromFile[c_numAreaLights];
        float               mLightTexRadius[c_numAreaLights];

        Ogre::TextureGpu    *mAreaMaskTex;
        bool                mUseSynchronousMethod;

        void createAreaMask( float radius, Ogre::Image2 &outImage );
        void createAreaPlaneMesh(void);
        Ogre::HlmsDatablock* setupDatablockTextureForLight( Ogre::Light *light, size_t idx );
        void createPlaneForAreaLight( Ogre::Light *light, size_t idx );

        void createLight( const Ogre::Vector3 &position, size_t idx );
        void setupLightTexture( size_t idx );

        virtual void generateDebugText( float timeSinceLast, Ogre::String &outText );

    public:
        UpdatingDecalsAndAreaLightTexGameState( const Ogre::String &helpDescription );

        virtual void createScene01(void);
        virtual void destroyScene(void);

        virtual void update( float timeSinceLast );

        virtual void keyReleased( const SDL_KeyboardEvent &arg );
    };
}

#endif
