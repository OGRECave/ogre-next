
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

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsPbs.h"

using namespace Demo;

namespace Demo
{
    TextureResidencyGameState::TextureResidencyGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mNumInitialTextures( 0 ),
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

        mChangeLog.push_back( targetResidency );

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
    void TextureResidencyGameState::enableHeavyRamMode(void)
    {
        const Ogre::String textureNames[] =
        {
            "snow_1024.jpg",    //1024x1024
            "AreaTexDX10.dds",
            "MRAMOR6X6.jpg",    //600x600
            "KAMEN320x240.jpg", //640x477
        };

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        for( size_t i=0; i<sizeof( textureNames ) / sizeof( textureNames[0] ); ++i )
        {
            for( size_t j=0; j<128; ++j )
            {
                Ogre::TextureGpu *texture = 0;
                texture = textureMgr->createOrRetrieveTexture(
                              textureNames[i],
                              "TestTex" + Ogre::StringConverter::toString( mTextures.size() ),
                              Ogre::GpuPageOutStrategy::Discard/*AlwaysKeepSystemRamCopy*/,
                              Ogre::TextureFlags::AutomaticBatching |
                              Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
                              Ogre::TextureTypes::Type2D,
                              Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
                texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );

                mTextures.push_back( texture );
            }
        }

        //Ensure all the new textures are shown
        if( isShowingTextureOnScreen() )
            showTexturesOnScreen();
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::disableHeavyRamMode(void)
    {
        //Ensure we don't try to show dangling pointers on screen
        const bool wasShowingTexturesOnScreen = isShowingTextureOnScreen();
        if( wasShowingTexturesOnScreen )
            hideTexturesFromScreen();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        const size_t numTextures = mTextures.size();
        for( size_t i=mNumInitialTextures; i<numTextures; ++i )
            textureMgr->destroyTexture( mTextures[i] );

        mTextures.erase( mTextures.begin() + mNumInitialTextures, mTextures.end() );

        if( wasShowingTexturesOnScreen )
            showTexturesOnScreen();
    }
    //-----------------------------------------------------------------------------------
    bool TextureResidencyGameState::isInHeavyRamMode(void) const
    {
        return mTextures.size() > mNumInitialTextures;
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::testSequence(void)
    {
        const bool oldSetting = mWaitForStreamingCompletion;
        mWaitForStreamingCompletion = false;

        for( int j=0; j<3; ++j )
        {
            for( size_t i=0; i<200; ++i )
                switchTextureResidency( (Ogre::GpuResidency::GpuResidency)(i % 3) );
            //switchTextureResidency( (i % 2) ? Ogre::GpuResidency::OnStorage : Ogre::GpuResidency::Resident );

            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
            textureMgr->waitForStreamingCompletion();

            Ogre::TextureGpu *texture = mTextures.front();
            if( texture->getResidencyStatus() != texture->getNextResidencyStatus() )
                throw;
        }

        mWaitForStreamingCompletion = oldSetting;
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::testRandom(void)
    {
        const bool oldSetting = mWaitForStreamingCompletion;
        mWaitForStreamingCompletion = false;

        //Make sure these 3 are included in the test coverage
        switchTextureResidency( Ogre::GpuResidency::Resident );
        switchTextureResidency( Ogre::GpuResidency::OnSystemRam );
        switchTextureResidency( Ogre::GpuResidency::Resident );

        srand( 101 );
        for( size_t i=0; i<100; ++i )
            switchTextureResidency( (Ogre::GpuResidency::GpuResidency)(rand() % 3) );

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
        textureMgr->waitForStreamingCompletion();

        mWaitForStreamingCompletion = oldSetting;
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::testRamStress(void)
    {
        const bool oldSetting = mWaitForStreamingCompletion;
        mWaitForStreamingCompletion = false;

        for( int j=0; j<3; ++j )
        {
            for( size_t i=0; i<2000; ++i )
                switchTextureResidency( (i % 2) ? Ogre::GpuResidency::OnStorage :
                                                  Ogre::GpuResidency::Resident );

            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
            textureMgr->waitForStreamingCompletion();

            Ogre::TextureGpu *texture = mTextures.front();
            if( texture->getResidencyStatus() != texture->getNextResidencyStatus() )
                throw;
        }

        mWaitForStreamingCompletion = oldSetting;
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::showTexturesOnScreen(void)
    {
        hideTexturesFromScreen();

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

        Ogre::SceneNode *staticRootNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC );

        const size_t numTextures = mTextures.size();

        for( size_t i=0; i<numTextures; ++i )
        {
            VisibleItem visibleItem;
            visibleItem.item = sceneManager->createItem(
                                   "Cube_d.mesh",
                                   Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                   Ogre::SCENE_STATIC );

            Ogre::SceneNode *sceneNode = staticRootNode->createChildSceneNode( Ogre::SCENE_STATIC );
            sceneNode->setPosition( (i % 5u) * 2.5f - 5.0f,
                                    (i % 4u) * 2.5f - 3.75f, 0.0f );
            sceneNode->attachObject( visibleItem.item );

            Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( i );
            Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                        hlmsPbs->createDatablock( datablockName,
                                                  datablockName,
                                                  Ogre::HlmsMacroblock(),
                                                  Ogre::HlmsBlendblock(),
                                                  Ogre::HlmsParamVec() ) );
            datablock->mAllowTextureResidencyChange = false;
            if( mTextures[i]->getTextureType() != Ogre::TextureTypes::TypeCube )
                datablock->setTexture( Ogre::PBSM_EMISSIVE, mTextures[i] );
            else
                datablock->setTexture( Ogre::PBSM_REFLECTION, mTextures[i] );
            visibleItem.item->setDatablock( datablock );
            visibleItem.datablock = datablock;
            mVisibleItems.push_back( visibleItem );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::hideTexturesFromScreen(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

        //LIFO order removal is better for Ogre
        VisibleItemVec::const_reverse_iterator itor = mVisibleItems.rbegin();
        VisibleItemVec::const_reverse_iterator end  = mVisibleItems.rend();

        while( itor != end )
        {
            Ogre::SceneNode *sceneNode = itor->item->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );

            sceneManager->destroyItem( itor->item );
            hlmsPbs->destroyDatablock( itor->datablock->getName() );

            ++itor;
        }

        mVisibleItems.clear();
    }
    //-----------------------------------------------------------------------------------
    bool TextureResidencyGameState::isShowingTextureOnScreen(void) const
    {
        return !mVisibleItems.empty();
    }
    //-----------------------------------------------------------------------------------
    void TextureResidencyGameState::createScene01(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        //textureMgr->createOrRetrieveTexture(  );
        Ogre::TextureGpu *texture = 0;

        texture = textureMgr->createOrRetrieveTexture(
                      "MRAMOR6X6.jpg", Ogre::GpuPageOutStrategy::Discard/*AlwaysKeepSystemRamCopy*/,
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

        mNumInitialTextures = mTextures.size();

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
        if( !mChangeLog.empty() )
            outText += "\nLast Supposed State: " + residencyNames[mChangeLog.back()];
        outText += "\nPress F2 to switch to OnStorage";
        outText += "\nPress F3 to switch to OnSystemRam";
        outText += "\nPress F4 to switch to Resident";
        outText += "\nPress F5 to wait for streaming completion ";
        outText += mWaitForStreamingCompletion ? "[On]" : "[Off]";
        outText += "\nPress F6 to run sequential test";
        outText += "\nPress F7 to run random test";
        outText += "\nPress F8 to run ram stress test";
        outText += "\nPress F9 for heavy RAM mode ";
        outText += isInHeavyRamMode() ? "[On]" : "[Off]";
        outText += "\nPress F10 to show textures on screen ";
        outText += isShowingTextureOnScreen() ? "[On]" : "[Off]";
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
        else if( arg.keysym.sym == SDLK_F6 )
        {
            testSequence();
        }
        else if( arg.keysym.sym == SDLK_F7 )
        {
            testRandom();
        }
        else if( arg.keysym.sym == SDLK_F8 )
        {
            testRamStress();
        }
        else if( arg.keysym.sym == SDLK_F9 )
        {
            if( !isInHeavyRamMode() )
                enableHeavyRamMode();
            else
                disableHeavyRamMode();
        }
        else if( arg.keysym.sym == SDLK_F10 )
        {
            if( !isShowingTextureOnScreen() )
                showTexturesOnScreen();
            else
                hideTexturesFromScreen();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
