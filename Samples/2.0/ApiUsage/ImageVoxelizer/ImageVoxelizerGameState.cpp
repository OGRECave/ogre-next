
#include "ImageVoxelizerGameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreCamera.h"
#include "OgreHlmsPbs.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

#include "IrradianceField/OgreIrradianceField.h"
#include "Vct/OgreVctCascadedVoxelizer.h"
#include "Vct/OgreVctImageVoxelizer.h"
#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVoxelizedMeshCache.h"

#include "Utils/MeshUtils.h"
#include "Utils/TestUtils.h"

using namespace Demo;

namespace Demo
{
    static int g_frame = 0;

    ImageVoxelizerGameState::ImageVoxelizerGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mCurrCascadeIdx( 0u ),
        mCascadedVoxelizer( 0 ),
        mStepSize( 3.0f ),
        mThinWallCounter( 1.0f ),
        mDebugVisualizationMode( Ogre::VctImageVoxelizer::DebugVisualizationNone ),
        mNumBounces( 2u ),
        mCurrentScene( SceneCornell ),
        mTestUtils( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleCascade( bool bPrev )
    {
        // Turn off current visualizations
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::VctImageVoxelizer *voxelizer = mCascadedVoxelizer->getCascade( mCurrCascadeIdx ).voxelizer;
        Ogre::VctLighting *vctLighting = mCascadedVoxelizer->getVctLighting( mCurrCascadeIdx );

        vctLighting->setDebugVisualization( false, sceneManager );
        voxelizer->setDebugVisualization( Ogre::VctImageVoxelizer::DebugVisualizationNone,
                                          sceneManager );

        // Cycle cascade
        if( !bPrev )
        {
            mCurrCascadeIdx = ( mCurrCascadeIdx + 1u ) % mCascadedVoxelizer->getNumCascades();
        }
        else
        {
            mCurrCascadeIdx = ( mCurrCascadeIdx + mCascadedVoxelizer->getNumCascades() - 1u ) %
                              mCascadedVoxelizer->getNumCascades();
        }

        // Turn on visualizations again, but for the new cascade (if any enabled)
        ++mDebugVisualizationMode;
        cycleVisualizationMode( true );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleVisualizationMode( bool bPrev )
    {
        if( !bPrev )
        {
            mDebugVisualizationMode = ( mDebugVisualizationMode + 1u ) %
                                      ( Ogre::VctImageVoxelizer::DebugVisualizationNone + 2u );
        }
        else
        {
            mDebugVisualizationMode =
                ( mDebugVisualizationMode + Ogre::VctImageVoxelizer::DebugVisualizationNone + 2u - 1u ) %
                ( Ogre::VctImageVoxelizer::DebugVisualizationNone + 2u );
        }
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::VctImageVoxelizer *voxelizer = mCascadedVoxelizer->getCascade( mCurrCascadeIdx ).voxelizer;
        Ogre::VctLighting *vctLighting = mCascadedVoxelizer->getVctLighting( mCurrCascadeIdx );

        if( mDebugVisualizationMode <= Ogre::VctImageVoxelizer::DebugVisualizationNone )
        {
            vctLighting->setDebugVisualization( false, sceneManager );
            voxelizer->setDebugVisualization(
                static_cast<Ogre::VctImageVoxelizer::DebugVisualizationMode>( mDebugVisualizationMode ),
                sceneManager );
        }
        else
        {
            voxelizer->setDebugVisualization( Ogre::VctImageVoxelizer::DebugVisualizationNone,
                                              sceneManager );
            vctLighting->setDebugVisualization( true, sceneManager );
        }

        const bool showItems =
            mDebugVisualizationMode == Ogre::VctImageVoxelizer::DebugVisualizationNone ||
            mDebugVisualizationMode == Ogre::VctImageVoxelizer::DebugVisualizationEmissive;

        Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item *>::const_iterator endt = mItems.end();
        while( itor != endt )
        {
            ( *itor )->setVisible( showItems );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::toggletVctQuality()
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setVctFullConeCount( !hlmsPbs->getVctFullConeCount() );
    }
    //-----------------------------------------------------------------------------------
    ImageVoxelizerGameState::GiMode ImageVoxelizerGameState::getGiMode() const
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        const bool hasVct = hlmsPbs->getVctLighting() != 0;
        if( hasVct )
            return VctOnly;
        else
            return NoGI;
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleGiModes( bool bPrev )
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        ImageVoxelizerGameState::GiMode giMode = getGiMode();

        if( !bPrev )
        {
            giMode = static_cast<ImageVoxelizerGameState::GiMode>( ( giMode + 1u ) % NumGiModes );
        }
        else
        {
            giMode = static_cast<ImageVoxelizerGameState::GiMode>( ( giMode + NumGiModes - 1u ) %
                                                                   NumGiModes );
        }

        switch( giMode )
        {
        case NoGI:
            hlmsPbs->setIrradianceField( 0 );
            hlmsPbs->setVctLighting( 0 );
            break;
        case VctOnly:
            hlmsPbs->setIrradianceField( 0 );
            hlmsPbs->setVctLighting( mCascadedVoxelizer->getVctLighting( 0u ) );
            break;
        case NumGiModes:
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleScenes( bool bPrev )
    {
        if( !bPrev )
            mCurrentScene = static_cast<Scenes>( ( mCurrentScene + 1u ) % NumScenes );
        else
            mCurrentScene = static_cast<Scenes>( ( mCurrentScene + NumScenes - 1u ) % NumScenes );

        destroyCurrentScene();

        switch( mCurrentScene )
        {
        case SceneCornell:
        case NumScenes:
            createCornellScene();
            break;
        case SceneSibenik:
            createSibenikScene();
            break;
        case SceneStress:
            createStressScene();
            break;
        }

        mCascadedVoxelizer->addAllItems( mGraphicsSystem->getSceneManager(), 0xffffffff, false );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::destroyCurrentScene()
    {
        mCascadedVoxelizer->removeAllItems();
        mThinWallCounter = 1.0f;

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item *>::const_iterator endt = mItems.end();
        while( itor != endt )
        {
            Ogre::SceneNode *sceneNode = ( *itor )->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
            sceneManager->destroyItem( *itor );
            ++itor;
        }

        mItems.clear();
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::createCornellScene()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = 0;
        Ogre::SceneNode *sceneNode = 0;

        item = sceneManager->createItem( "CornellBox.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                        ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        mItems.push_back( item );

        item = sceneManager->createItem( "athene.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                        ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.01f ) );
        sceneNode->setPosition( 0, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );

        // Ogre::Item *item2 = sceneManager->createItem( "athene.mesh Imported",
        item = sceneManager->createItem( "Sphere1000.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                        ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.45f ) );
        sceneNode->setPosition( 0.6f, item->getWorldAabbUpdated().mHalfSize.y, -0.25f );
        mItems.push_back( item );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        item->setDatablock( hlmsManager->getHlms( Ogre::HLMS_UNLIT )->getDefaultDatablock() );

        item = sceneManager->createItem( "tudorhouse.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                        ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.001f ) );
        sceneNode->setPosition( -0.5, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );

        mThinWallCounter = 2.0f;
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::createSibenikScene()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::Item *item = 0;
        Ogre::SceneNode *sceneNode = 0;

        item = sceneManager->createItem( "sibenik.mesh",
                                         Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                         Ogre::SCENE_DYNAMIC );
        sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                        ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->setScale( Ogre::Vector3( 0.1f ) );
        sceneNode->setPosition( 0, item->getWorldAabbUpdated().mHalfSize.y, 0 );
        mItems.push_back( item );

        // Sibenik is a very big mesh. We want to add it with 128x128x128
        Ogre::VoxelizedMeshCache *meshCache = mCascadedVoxelizer->getMeshCache();
        meshCache->setCacheResolution( 128u, 128u, 128u, 128u, 128u, 128u, Ogre::Vector3::UNIT_SCALE );
        meshCache->addMeshToCache( item->getMesh(), sceneManager,
                                   sceneManager->getDestinationRenderSystem(),
                                   mGraphicsSystem->getRoot()->getHlmsManager(), item );

        // Set back some sane defaults
        meshCache->setCacheResolution( 64u, 64u, 64u, 64u, 64u, 64u, Ogre::Vector3( 2.0f ) );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::createStressScene()
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

        for( size_t i = 0; i < 512u; ++i )
        {
            item = sceneManager->createItem( "Cube_d.mesh",
                                             Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                             Ogre::SCENE_STATIC );
            sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC )
                            ->createChildSceneNode( Ogre::SCENE_STATIC );
            sceneNode->setScale( Ogre::Vector3( 0.05f ) );

            const float xPos = ( i % 16u ) - 7.5f;
            const float zPos = ( i / 16u ) % 16u - 7.5f;
            const float yPos = float( i ) / ( 16u * 16u );

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
    void ImageVoxelizerGameState::createScene01()
    {
        // We need this so the cache works fine. Actually we can avoid this if we saved
        // the meshes again (as v2). But oh well...
        Ogre::Mesh::msUseTimestampAsHash = true;

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
        light->setPowerScale( Ogre::Math::PI );  // Since we don't do HDR, counter the PBS' division by
                                                 // PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mCameraController = new CameraController( mGraphicsSystem, false );
        mCameraController->mCameraBaseSpeed *= 0.01f;

        TutorialGameState::createScene01();

        Ogre::Root *root = mGraphicsSystem->getRoot();
#ifdef DISABLED2
        mVoxelizer =
            new Ogre::VctImageVoxelizer( Ogre::Id::generateNewId<Ogre::VctImageVoxelizer>(),
                                         root->getRenderSystem(), root->getHlmsManager(), true );
        {
            mIrradianceField = new Ogre::IrradianceField( root, sceneManager );

            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs =
                static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
#    ifdef DISABLED
            hlmsPbs->setIrradianceField( mIrradianceField );
#    endif
        }
#endif
        mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 1.8f, 4.03f ) );
        /*mGraphicsSystem->getCamera()->setPosition( mGraphicsSystem->getCamera()->getPosition() +
                                                   Ogre::Vector3( -0.6f, -0.1f, 0.1f ) );*/
        // voxelizeScene();

        mCascadedVoxelizer = new Ogre::VctCascadedVoxelizer();
        mCascadedVoxelizer->reserveNumCascades( 2u );
        Ogre::VctCascadeSetting cascadeSetting;
        // cascadeSetting.setOctantSubdivision( 2u );

        cascadeSetting.cameraStepSize = 1.0f;  // Will be overriden by autoCalculateStepSizes

        // Cascades are much more stable and consistent between each other if
        // cascade[i].areaHalfSize / cascade[i].resolution is multiple of the
        // previous cascade (also best match if that multiple is 2 or 4)
        cascadeSetting.areaHalfSize = 5.0f;
        cascadeSetting.setResolution( 128u );
        mCascadedVoxelizer->addCascade( cascadeSetting );
        cascadeSetting.areaHalfSize = 10.0f;
        cascadeSetting.setResolution( 128u );
        mCascadedVoxelizer->addCascade( cascadeSetting );
        cascadeSetting.areaHalfSize = 15.0f;
        cascadeSetting.setResolution( 64u );
        mCascadedVoxelizer->addCascade( cascadeSetting );
        cascadeSetting.areaHalfSize = 60.0f;
        cascadeSetting.setResolution( 64u );
        mCascadedVoxelizer->addCascade( cascadeSetting );
        mCascadedVoxelizer->autoCalculateStepSizes( Ogre::Vector3( mStepSize ) );

        mCascadedVoxelizer->init( root->getRenderSystem(), root->getHlmsManager(), mNumBounces, true );

        createCornellScene();
        // createStressScene();
        // createSibenikScene();

        mCascadedVoxelizer->addAllItems( sceneManager, 0xffffffff, false );
        mCascadedVoxelizer->setCameraPosition( mGraphicsSystem->getCamera()->getDerivedPosition() );
        sceneManager->updateSceneGraph();
        mCascadedVoxelizer->update( sceneManager );
        mCascadedVoxelizer->setAutoUpdate( mGraphicsSystem->getRoot()->getCompositorManager2(),
                                           mGraphicsSystem->getSceneManager() );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setVctLighting( mCascadedVoxelizer->getVctLighting( 0u ) );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::destroyScene()
    {
        delete mCascadedVoxelizer;
        mCascadedVoxelizer = 0;

        {
            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs =
                static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
            hlmsPbs->setVctLighting( 0 );
            hlmsPbs->setIrradianceField( 0 );
        }

        delete mTestUtils;
        mTestUtils = 0;
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::update( float timeSinceLast )
    {
        if( mGraphicsSystem->getRenderWindow()->isVisible() )
        {
#if 0
            Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
            if( g_frame == 3 )
            {
                /*mGraphicsSystem->getCamera()->setPosition( mGraphicsSystem->getCamera()->getPosition()
                   + Ogre::Vector3( -0.6f, -0.1f, 0.1f ) );*/
                mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 1.8f, 5.14f ) );
            }
            /*else if( g_frame == 4 )
            {
                mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 1.8f, 4.03f ) * 1.0f );
                mGraphicsSystem->getCamera()->lookAt( mGraphicsSystem->getCamera()->getPosition() +
                                                      Ogre::Vector3( 0, 0, -1 ) );
            }*/
#endif

            if( g_frame > 1 )
            {
                // mVoxelizer->build( sceneManager );
                // mThinWallCounter = 2.0f;
                // mThinWallCounter = 1.00f;
                // mThinWallCounter = 1.00f;
                // mVctLighting->update( sceneManager, mNumBounces, mThinWallCounter );
            }
            ++g_frame;
        }

        TutorialGameState::update( timeSinceLast );

        mCascadedVoxelizer->setCameraPosition( mGraphicsSystem->getCamera()->getDerivedPosition() );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        // clang-format off
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
        // clang-format on

        outText += "\nF2 to cycle visualization modes ";
        outText += visualizationModes[mDebugVisualizationMode];
        outText += "\nF3 to toggle VCT quality [";
        outText += hlmsPbs->getVctFullConeCount() ? "High]" : "Low]";
        if( mCascadedVoxelizer->getVctLighting( 0u ) )
        {
            // mVctLighting may be nullptr if GI mode started as
            // IfdOnly with mUseRasterIrradianceField = true
            outText += "\nF4 to toggle Anisotropic VCT [";
            outText += mCascadedVoxelizer->getVctLighting( 0u )->isAnisotropic() ? "Anisotropic]"
                                                                                 : "Isotropic]";

            outText += "\n[Shift+] F5 to increase/decrease num indirect bounces [";
            outText += Ogre::StringConverter::toString( mNumBounces );
            outText += "]";
        }
        outText += "\n[Shift+] F6 to cycle scenes ";
        outText += sceneNames[mCurrentScene];
        outText += "\n[Shift+] F7 to cycle GI [";
        const GiMode giMode = getGiMode();
        switch( giMode )
        {
        case NoGI:
            outText += "No GI]";
            break;
        case VctOnly:
            outText += "VCT]";
            break;
        case NumGiModes:
            break;
        }
        outText += "\n[Shift+] F8 to cycle cascade [";
        outText += Ogre::StringConverter::toString( mCurrCascadeIdx );
        outText += "/";
        outText += Ogre::StringConverter::toString( mCascadedVoxelizer->getNumCascades() - 1u );
        outText += "]\n[Shift+] F9 to alter camera update step size [";
        outText += Ogre::StringConverter::toString( mStepSize );
        outText += "]";
        outText += "]\nF10 consistent cascade steps [";
        outText += mCascadedVoxelizer->getConsistentCascadeSteps() ? "Yes]" : "No]";
        outText += "]";
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            cycleVisualizationMode( ( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) );
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            toggletVctQuality();
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            mCascadedVoxelizer->setNewSettings( mCascadedVoxelizer->getNumBounces(),
                                                !mCascadedVoxelizer->isAnisotropic() );
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                ++mNumBounces;
            else if( mNumBounces > 0u )
                --mNumBounces;

            mCascadedVoxelizer->setNewSettings( mNumBounces, mCascadedVoxelizer->isAnisotropic() );
        }
        else if( arg.keysym.sym == SDLK_F6 )
        {
            cycleScenes( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else if( arg.keysym.sym == SDLK_F7 )
        {
            cycleGiModes( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else if( arg.keysym.sym == SDLK_F8 )
        {
            cycleCascade( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else if( arg.keysym.sym == SDLK_F9 )
        {
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                ++mStepSize;
            else if( mStepSize > 1.0f )
                --mStepSize;
            mCascadedVoxelizer->autoCalculateStepSizes( Ogre::Vector3( mStepSize ) );
        }
        else if( arg.keysym.sym == SDLK_F10 )
        {
            mCascadedVoxelizer->setConsistentCascadeSteps(
                !mCascadedVoxelizer->getConsistentCascadeSteps() );
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
