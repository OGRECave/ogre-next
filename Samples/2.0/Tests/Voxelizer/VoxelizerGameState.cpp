
#include "VoxelizerGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMeshManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2Serializer.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreHlmsPbs.h"

#include "OgreRoot.h"
#include "Vct/OgreVctVoxelizer.h"
#include "Vct/OgreVctLighting.h"

#include "Utils/MeshUtils.h"
#include "Utils/TestUtils.h"

#include "OgreWindow.h"

using namespace Demo;

#define TODO_do_this_in_voxelizer

namespace Demo
{
    VoxelizerGameState::VoxelizerGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mVoxelizer( 0 ),
        mVctLighting( 0 ),
        mThinWallCounter( 1.0f ),
        mDebugVisualizationMode( Ogre::VctVoxelizer::DebugVisualizationNone ),
        mNumBounces( 6u ),
        mCurrentScene( SceneCornell ),
        mTestUtils( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::cycleVisualizationMode( bool bPrev )
    {
        if( !bPrev )
        {
            mDebugVisualizationMode = (mDebugVisualizationMode + 1u) %
                                      (Ogre::VctVoxelizer::DebugVisualizationNone + 2u);
        }
        else
        {
            mDebugVisualizationMode = (mDebugVisualizationMode +
                                       Ogre::VctVoxelizer::DebugVisualizationNone + 2u - 1u) %
                                      (Ogre::VctVoxelizer::DebugVisualizationNone + 2u);
        }
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        if( mDebugVisualizationMode <= Ogre::VctVoxelizer::DebugVisualizationNone )
        {
            mVctLighting->setDebugVisualization( false, sceneManager );
            mVoxelizer->setDebugVisualization( static_cast<Ogre::VctVoxelizer::
                                               DebugVisualizationMode>( mDebugVisualizationMode ),
                                               sceneManager );
        }
        else
        {
            mVoxelizer->setDebugVisualization( Ogre::VctVoxelizer::DebugVisualizationNone,
                                               sceneManager );
            mVctLighting->setDebugVisualization( true, sceneManager );
        }

        const bool showItems = mDebugVisualizationMode == Ogre::VctVoxelizer::DebugVisualizationNone ||
                               mDebugVisualizationMode == Ogre::VctVoxelizer::DebugVisualizationEmissive;

        Ogre::FastArray<Ogre::Item*>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item*>::const_iterator end  = mItems.end();
        while( itor != end )
        {
            (*itor)->setVisible( showItems );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::toggletVctQuality(void)
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );
        hlmsPbs->setVctFullConeCount( !hlmsPbs->getVctFullConeCount() );
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::voxelizeScene(void)
    {
        TODO_do_this_in_voxelizer;
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        sceneManager->updateSceneGraph();

        mVoxelizer->removeAllItems();

        Ogre::FastArray<Ogre::Item*>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item*>::const_iterator end  = mItems.end();

        while( itor != end )
            mVoxelizer->addItem( *itor++, false );

        mVoxelizer->autoCalculateRegion();
        mVoxelizer->dividideOctants( 1u, 1u, 1u );

        mVoxelizer->build( sceneManager );

        if( !mVctLighting )
        {
            mVctLighting = new Ogre::VctLighting( Ogre::Id::generateNewId<Ogre::VctLighting>(),
                                                  mVoxelizer, true );
            mVctLighting->setAllowMultipleBounces( true );

            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );
            hlmsPbs->setVctLighting( mVctLighting );
        }

        mVctLighting->update( sceneManager, mNumBounces, mThinWallCounter );
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::cycleScenes( bool bPrev )
    {
        if( !bPrev )
            mCurrentScene = static_cast<Scenes>( (mCurrentScene + 1u) % NumScenes );
        else
            mCurrentScene = static_cast<Scenes>( (mCurrentScene + NumScenes - 1u) % NumScenes );

        destroyCurrentScene();

        switch( mCurrentScene )
        {
        case SceneCornell:
        case NumScenes:
            createCornellScene(); break;
        case SceneSibenik:
            createSibenikScene(); break;
        case SceneStress:
            createStressScene(); break;
        }

        voxelizeScene();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::destroyCurrentScene(void)
    {
        mVoxelizer->removeAllItems();
        mThinWallCounter = 1.0f;

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::FastArray<Ogre::Item*>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item*>::const_iterator end  = mItems.end();
        while( itor != end )
        {
            Ogre::SceneNode *sceneNode = (*itor)->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
            sceneManager->destroyItem( *itor );
            ++itor;
        }

        mItems.clear();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createCornellScene(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = 0;
        Ogre::SceneNode *sceneNode = 0;

        item = sceneManager->createItem( "CornellBox.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        mItems.push_back( item );

        item = sceneManager->createItem( "athene.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.01f ) );
        sceneNode->setPosition( 0, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );

        //Ogre::Item *item2 = sceneManager->createItem( "athene.mesh Imported",
        item = sceneManager->createItem( "Sphere1000.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.45f ) );
        sceneNode->setPosition( 0.6f, item->getWorldAabbUpdated().mHalfSize.y, -0.25f );
        mItems.push_back( item );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        item->setDatablock( hlmsManager->getHlms( Ogre::HLMS_UNLIT )->getDefaultDatablock() );

        item = sceneManager->createItem( "tudorhouse.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.001f ) );
        sceneNode->setPosition( -0.5, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );

        mThinWallCounter = 2.0f;
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createSibenikScene(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = 0;
        Ogre::SceneNode *sceneNode = 0;

        item = sceneManager->createItem( "sibenik.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.1f ) );
        sceneNode->setPosition( 0, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createStressScene(void)
    {
        createCornellScene();

        if( !mTestUtils )
        {
            mTestUtils = new TestUtils();
            mTestUtils->generateRandomBlankTextures( 512u, 128u, 256u );
            mTestUtils->generateUnlitDatablocksWithTextures( 256u, 0u, 256u );
            mTestUtils->generatePbsDatablocksWithTextures( 256u, 256u, 256u );
        }

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = 0;
        Ogre::SceneNode *sceneNode = 0;

        for( size_t i=0; i<512u; ++i )
        {
            item = sceneManager->createItem( "Cube_d.mesh",
                                             Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                             Ogre::SCENE_STATIC );
            sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC )->
                        createChildSceneNode( Ogre::SCENE_STATIC );
            sceneNode->setScale( Ogre::Vector3( 0.05f ) );

            const float xPos = (i % 16u) - 7.5f;
            const float zPos = (i / 16u) % 16u - 7.5f;
            const float yPos = i / (16u * 16u);

            sceneNode->setPosition( xPos * 0.12f, 0.05f + 0.35f * yPos, zPos * 0.12f );
            sceneNode->attachObject( item );

            if( i < 256u )
            {
                item->setDatablockOrMaterialName( "UnitTestUnlit" +
                                                  Ogre::StringConverter::toString( i ) );
            }
            else
            {
                item->setDatablockOrMaterialName( "UnitTestPbs" +
                                                  Ogre::StringConverter::toString( i - 256u ) );
            }

            mItems.push_back( item );
        }
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        MeshUtils::importV1Mesh( "athene.mesh",
                                 Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        MeshUtils::importV1Mesh( "tudorhouse.mesh",
                                 Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        MeshUtils::importV1Mesh( "sibenik.mesh",
                                 Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( Ogre::Math::PI ); //Since we don't do HDR, counter the PBS' division by PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mCameraController = new CameraController( mGraphicsSystem, false );

        TutorialGameState::createScene01();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        mVoxelizer =
                new Ogre::VctVoxelizer( Ogre::Id::generateNewId<Ogre::VctVoxelizer>(),
                                        root->getRenderSystem(), root->getHlmsManager(),
                                        true );

        mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 1.8f, 2.5f ) );

        createCornellScene();
        //createStressScene();
        //createSibenikScene();
        voxelizeScene();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::destroyScene(void)
    {
        delete mVctLighting;
        mVctLighting = 0;
        delete mVoxelizer;
        mVoxelizer = 0;

        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );
            hlmsPbs->setVctLighting( 0 );
        }

        delete mTestUtils;
        mTestUtils = 0;
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::update( float timeSinceLast )
    {
//        if( mGraphicsSystem->getRenderWindow()->isVisible() )
//        {
//            Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

//            static int frame = 0;
//            if( frame > 1 )
//            {
//                mVoxelizer->build( sceneManager );
//                mVctLighting->update( sceneManager, mNumBounces );
//            }
//            ++frame;
//        }
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

        static const Ogre::String visualizationModes[] =
        {
            "[Albedo]",
            "[Normal]",
            "[Emissive]",
            "[None]",
            "[Lighting]"
        };

        static const Ogre::String sceneNames[] =
        {
            "[Cornell]",
            "[Sibenik]",
            "[Stress Test]",
        };

        outText += "\nPress F2 to cycle visualization modes ";
        outText += visualizationModes[mDebugVisualizationMode];
        outText += "\nPress F3 to toggle VCT quality [";
        outText += hlmsPbs->getVctFullConeCount() ? "High]" : "Low]";
        outText += "\nPress F4 to toggle Anisotropic VCT [";
        outText += mVctLighting->isAnisotropic() ? "Anisotropic]" : "Isotropic]";
        outText += "\nPress [Shift+] F5 to increase/decrease num indirect bounces [";
        outText += Ogre::StringConverter::toString( mNumBounces );
        outText += "]";
        outText += "\nPress [Shift+] F6 to cycle scenes ";
        outText += sceneNames[mCurrentScene];
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS|KMOD_LSHIFT|KMOD_RSHIFT)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            cycleVisualizationMode( (arg.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) );
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            toggletVctQuality();
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            mVctLighting->setAnisotropic( !mVctLighting->isAnisotropic() );
            mVctLighting->update( mGraphicsSystem->getSceneManager(), mNumBounces, mThinWallCounter );
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            if( !(arg.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT)) )
                ++mNumBounces;
            else if( mNumBounces > 0u )
                --mNumBounces;

            mVctLighting->update( mGraphicsSystem->getSceneManager(), mNumBounces, mThinWallCounter );
        }
        else if( arg.keysym.sym == SDLK_F6 )
        {
            cycleScenes( arg.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT) );
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
