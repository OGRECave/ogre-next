
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
        mVoxelizer( 0 ),
        mVctLighting( 0 ),
        mCascadedVoxelizer( 0 ),
        mThinWallCounter( 1.0f ),
        mIrradianceField( 0 ),
        mUseRasterIrradianceField( false ),
        mDebugVisualizationMode( Ogre::VctImageVoxelizer::DebugVisualizationNone ),
        mIfdDebugVisualizationMode( Ogre::IrradianceField::DebugVisualizationNone ),
        mNumBounces( 0u ),
        mCurrentScene( SceneCornell ),
        mTestUtils( 0 )
    {
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

        if( mDebugVisualizationMode <= Ogre::VctImageVoxelizer::DebugVisualizationNone )
        {
            mVctLighting->setDebugVisualization( false, sceneManager );
            mVoxelizer->setDebugVisualization(
                static_cast<Ogre::VctImageVoxelizer::DebugVisualizationMode>( mDebugVisualizationMode ),
                sceneManager );
        }
        else
        {
            mVoxelizer->setDebugVisualization( Ogre::VctImageVoxelizer::DebugVisualizationNone,
                                               sceneManager );
            mVctLighting->setDebugVisualization( true, sceneManager );
        }

        const bool showItems =
            mDebugVisualizationMode == Ogre::VctImageVoxelizer::DebugVisualizationNone ||
            mDebugVisualizationMode == Ogre::VctImageVoxelizer::DebugVisualizationEmissive;

        Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item *>::const_iterator end = mItems.end();
        while( itor != end )
        {
            ( *itor )->setVisible( showItems );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::toggletVctQuality( void )
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setVctFullConeCount( !hlmsPbs->getVctFullConeCount() );
    }
    //-----------------------------------------------------------------------------------
    ImageVoxelizerGameState::GiMode ImageVoxelizerGameState::getGiMode( void ) const
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        const bool hasVct = hlmsPbs->getVctLighting() != 0;
        const bool hasIfd = hlmsPbs->getIrradianceField() != 0;
        if( hasVct && hasIfd )
            return IfdVct;
        else if( hasIfd )
            return IfdOnly;
        else if( hasVct )
            return VctOnly;
        else
            return NoGI;
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleIfdProbeVisualizationMode( bool bPrev )
    {
        if( !bPrev )
        {
            mIfdDebugVisualizationMode = ( mIfdDebugVisualizationMode + 1u ) %
                                         ( Ogre::IrradianceField::DebugVisualizationNone + 1u );
        }
        else
        {
            mIfdDebugVisualizationMode = ( mIfdDebugVisualizationMode +
                                           Ogre::IrradianceField::DebugVisualizationNone + 1u - 1u ) %
                                         ( Ogre::IrradianceField::DebugVisualizationNone + 1u );
        }
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        mIrradianceField->setDebugVisualization(
            static_cast<Ogre::IrradianceField::DebugVisualizationMode>( mIfdDebugVisualizationMode ),
            sceneManager, 5u );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::cycleIrradianceField( bool bPrev )
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

        if( giMode != IfdOnly && giMode != IfdVct )
        {
            // Disable IFD visualization while not in use
            mIrradianceField->setDebugVisualization( Ogre::IrradianceField::DebugVisualizationNone, 0,
                                                     mIrradianceField->getDebugTessellation() );
        }
        else
        {
            // Restore IFD visualization if it was in use
            mIrradianceField->setDebugVisualization(
                static_cast<Ogre::IrradianceField::DebugVisualizationMode>( mIfdDebugVisualizationMode ),
                mGraphicsSystem->getSceneManager(), 5u );
        }

        switch( giMode )
        {
        case NoGI:
            hlmsPbs->setIrradianceField( 0 );
            hlmsPbs->setVctLighting( 0 );
            break;
        case IfdOnly:
            hlmsPbs->setIrradianceField( mIrradianceField );
            hlmsPbs->setVctLighting( 0 );
            break;
        case VctOnly:
            hlmsPbs->setIrradianceField( 0 );
            hlmsPbs->setVctLighting( mVctLighting );
            break;
        case IfdVct:
            hlmsPbs->setIrradianceField( mIrradianceField );
            hlmsPbs->setVctLighting( mVctLighting );
            break;
        case NumGiModes:
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::voxelizeScene( void )
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        sceneManager->updateSceneGraph();

        mVoxelizer->removeAllItems();

        Ogre::IrradianceFieldSettings ifSettings;
        Ogre::Aabb fieldAabb( Ogre::Aabb::BOX_NULL );

        if( needsVoxels() )
        {
            Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
            Ogre::FastArray<Ogre::Item *>::const_iterator endt = mItems.end();

            while( itor != endt )
                mVoxelizer->addItem( *itor++ );

            mVoxelizer->autoCalculateRegion();
            mVoxelizer->dividideOctants( 1u, 1u, 1u );

            mVoxelizer->build( sceneManager );
            g_frame = 0;

            if( !mVctLighting )
            {
                mVctLighting = new Ogre::VctLighting( Ogre::Id::generateNewId<Ogre::VctLighting>(),
                                                      mVoxelizer, true );
                mVctLighting->setAllowMultipleBounces( true );

                Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

                assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
                Ogre::HlmsPbs *hlmsPbs =
                    static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
                hlmsPbs->setVctLighting( mVctLighting );
            }

            mVctLighting->update( sceneManager, mNumBounces, mThinWallCounter );

            fieldAabb.setExtents( mVoxelizer->getVoxelOrigin(),
                                  mVoxelizer->getVoxelOrigin() + mVoxelizer->getVoxelSize() );
        }

        if( ( getGiMode() == IfdOnly || getGiMode() == IfdVct ) && mUseRasterIrradianceField )
        {
            Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
            Ogre::FastArray<Ogre::Item *>::const_iterator endt = mItems.end();

            while( itor != endt )
            {
                fieldAabb.merge( ( *itor )->getWorldAabb() );
                ++itor;
            }

            /*for( int i=0;i<3;++i)
                ifSettings.mNumProbes[i] = 1u;
            ifSettings.mIrradianceResolution = 16u;
            ifSettings.mDepthProbeResolution = 32u;*/

            // Generate IFD via raster
            ifSettings.mRasterParams.mWorkspaceName = "IrradianceFieldRasterWorkspace";
            ifSettings.mRasterParams.mCameraNear = 0.01f;
            ifSettings.mRasterParams.mCameraFar = fieldAabb.getSize().length() * 2.0f;
        }

        mIrradianceField->initialize( ifSettings, fieldAabb.getMinimum(), fieldAabb.getSize(),
                                      mVctLighting );
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

        mCascadedVoxelizer->removeAllItems();
        mCascadedVoxelizer->addAllItems( mGraphicsSystem->getSceneManager(), 0xffffffff, false );
        // voxelizeScene();
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::destroyCurrentScene( void )
    {
        mVoxelizer->removeAllItems();
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
    void ImageVoxelizerGameState::createCornellScene( void )
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
    void ImageVoxelizerGameState::createSibenikScene( void )
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
    void ImageVoxelizerGameState::createStressScene( void )
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
            const float yPos = i / ( 16u * 16u );

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
    void ImageVoxelizerGameState::createScene01( void )
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
        cascadeSetting.setResolution( 128u );
        // cascadeSetting.setOctantSubdivision( 2u );

        cascadeSetting.areaHalfSize = 5.0f;
        cascadeSetting.cameraStepSize = 1.0f;
        mCascadedVoxelizer->addCascade( cascadeSetting );
        cascadeSetting.areaHalfSize = 30.0f;
        cascadeSetting.setResolution( 128u );
        mCascadedVoxelizer->addCascade( cascadeSetting );
        mCascadedVoxelizer->autoCalculateStepSizes( Ogre::Vector3( 4.0f ) );

        mCascadedVoxelizer->init( root->getRenderSystem(), root->getHlmsManager(), 2u );

        createCornellScene();
        // createStressScene();
        // createSibenikScene();

        mCascadedVoxelizer->addAllItems( sceneManager, 0xffffffff, false );
        mCascadedVoxelizer->setCameraPosition( mGraphicsSystem->getCamera()->getDerivedPosition() );
        sceneManager->updateSceneGraph();
        mCascadedVoxelizer->update( sceneManager );
        mCascadedVoxelizer->setAutoUpdate( mGraphicsSystem->getRoot()->getCompositorManager2(),
                                           mGraphicsSystem->getSceneManager() );

        mVoxelizer = mCascadedVoxelizer->getCascade( 0u ).voxelizer;
        mVctLighting = mCascadedVoxelizer->getVctLighting( 0u );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setVctLighting( mVctLighting );
    }
    //-----------------------------------------------------------------------------------
    void ImageVoxelizerGameState::destroyScene( void )
    {
        delete mIrradianceField;
        mIrradianceField = 0;
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
            Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

#if 1
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

        if( getGiMode() == IfdVct || getGiMode() == IfdOnly )
            mIrradianceField->update( mUseRasterIrradianceField ? 4u : 200u );
        TutorialGameState::update( timeSinceLast );

        mCascadedVoxelizer->setCameraPosition( mGraphicsSystem->getCamera()->getDerivedPosition() );
    }
    //-----------------------------------------------------------------------------------
    bool ImageVoxelizerGameState::needsVoxels( void ) const
    {
        return !( getGiMode() == IfdOnly && mUseRasterIrradianceField );
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

        static const Ogre::String ifdProbeVisualizationModes[] =
        {
            "[Irradiance]",
            "[Depth]",
            "[None]"
        };
        // clang-format on

        outText += "\nF2 to cycle visualization modes ";
        outText += visualizationModes[mDebugVisualizationMode];
        outText += "\nF3 to toggle VCT quality [";
        outText += hlmsPbs->getVctFullConeCount() ? "High]" : "Low]";
        if( mVctLighting )
        {
            // mVctLighting may be nullptr if GI mode started as
            // IfdOnly with mUseRasterIrradianceField = true
            outText += "\nF4 to toggle Anisotropic VCT [";
            outText += mVctLighting->isAnisotropic() ? "Anisotropic]" : "Isotropic]";

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
        case IfdOnly:
            outText += "IFD]";
            break;
        case VctOnly:
            outText += "VCT]";
            break;
        case IfdVct:
            outText += "IFD+VCT]";
            break;
        case NumGiModes:
            break;
        }

        if( giMode == IfdVct || giMode == IfdOnly )
        {
            outText += "\nF8 to generate IFD via rasterization ";
            outText += mUseRasterIrradianceField ? "[Raster]" : "[Voxels]";
        }

        if( giMode == IfdVct || giMode == IfdOnly )
        {
            outText += "\n[Shift+] F9 to cycle IFD debug visualization ";
            outText += ifdProbeVisualizationModes[mIfdDebugVisualizationMode];
        }
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
        else if( arg.keysym.sym == SDLK_F4 && mVctLighting )
        {
            mVctLighting->setAnisotropic( !mVctLighting->isAnisotropic() );
            mVctLighting->update( mGraphicsSystem->getSceneManager(), mNumBounces, mThinWallCounter );
        }
        else if( arg.keysym.sym == SDLK_F5 && mVctLighting )
        {
            if( !( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) ) )
                ++mNumBounces;
            else if( mNumBounces > 0u )
                --mNumBounces;

            mVctLighting->update( mGraphicsSystem->getSceneManager(), mNumBounces, mThinWallCounter );
            mIrradianceField->reset();
        }
        else if( arg.keysym.sym == SDLK_F6 )
        {
            cycleScenes( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else if( arg.keysym.sym == SDLK_F7 )
        {
            cycleIrradianceField( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else if( arg.keysym.sym == SDLK_F8 )
        {
            const ImageVoxelizerGameState::GiMode giMode = getGiMode();
            if( giMode == IfdVct || giMode == IfdOnly )
            {
                mUseRasterIrradianceField = !mUseRasterIrradianceField;
                voxelizeScene();
            }
        }
        else if( arg.keysym.sym == SDLK_F9 )
        {
            const ImageVoxelizerGameState::GiMode giMode = getGiMode();
            if( giMode == IfdVct || giMode == IfdOnly )
                cycleIfdProbeVisualizationMode( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
