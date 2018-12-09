
#include "TextureResidencyGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"

using namespace Demo;

namespace Demo
{
    TextureResidencyGameState::TextureResidencyGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mWaitForStreamingCompletion( true )
    {
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::switchTextureResidency( int intTargetResidency )
    {
        Ogre::GpuResidency::GpuResidency targetResidency =
                static_cast<Ogre::GpuResidency::GpuResidency>( intTargetResidency );

        if( mTextures.front()->getNextResidencyStatus() == targetResidency )
            return;

        std::vector<Ogre::TextureGpu*>::const_iterator itor = mTextures.begin();
        std::vector<Ogre::TextureGpu*>::const_iterator end  = mTextures.end();

        while( itor != end )
        {
            Ogre::TextureGpu *texture = *itor;
            texture->scheduleTransitionTo( targetResidency );
            ++itor;
        }

        if( mWaitForStreamingCompletion )
        {
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
            textureMgr->waitForStreamingCompletion();
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        //textureMgr->createOrRetrieveTexture(  );
        Ogre::TextureGpu *texture = 0;

        texture = textureMgr->createOrRetrieveTexture(
                      "MRAMOR6X6.jpg", Ogre::GpuPageOutStrategy::Discard,
                      Ogre::CommonTextureTypes::Diffuse,
                      Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        mTextures.push_back( texture );

        texture = textureMgr->createOrRetrieveTexture(
                      "SaintPetersBasilica.dds", Ogre::GpuPageOutStrategy::Discard,
                      Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
                      Ogre::TextureTypes::TypeCube,
                      Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                      Ogre::TextureFilter::TypeGenerateDefaultMipmaps );

        mTextures.push_back( texture );

        std::vector<Ogre::TextureGpu*>::const_iterator itor = mTextures.begin();
        std::vector<Ogre::TextureGpu*>::const_iterator end  = mTextures.end();

        while( itor != end )
        {
            texture = *itor;
            texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            ++itor;
        }

        textureMgr->waitForStreamingCompletion();

        itor = mTextures.begin();
        while( itor != end )
        {
            texture = *itor;
            texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );
            ++itor;
        }

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::update( float timeSinceLast )
    {
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        static const Ogre::String residencyNames[] =
        {
            "OnStorage",
            "OnSystemRam",
            "Resident"
        };

        outText += "\nCurrent Texture State: " + residencyNames[mTextures.front()->getResidencyStatus()];
        outText += "\nPress F2 to switch to OnStorage";
        outText += "\nPress F3 to switch to OnSystemRam";
        outText += "\nPress F4 to switch to Resident";
        outText += "\nPress F5 to wait for streaming completion ";
        outText += mWaitForStreamingCompletion ? "[On]" : "[Off]";
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            switchTextureResidency( Ogre::GpuResidency::OnStorage );
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            switchTextureResidency( Ogre::GpuResidency::OnSystemRam );
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            switchTextureResidency( Ogre::GpuResidency::Resident );
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            mWaitForStreamingCompletion = !mWaitForStreamingCompletion;
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
