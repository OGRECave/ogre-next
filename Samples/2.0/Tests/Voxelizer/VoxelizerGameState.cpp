
#include "VoxelizerGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSceneManager.h"

#include "OgreCamera.h"

#include "OgreHlmsPbs.h"

#include "OgreRoot.h"
#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVctVoxelizer.h"

#include "IrradianceField/OgreIrradianceField.h"

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
        mIrradianceField( 0 ),
        mUseRasterIrradianceField( false ),
        mDebugVisualizationMode( Ogre::VctVoxelizer::DebugVisualizationNone ),
        mIfdDebugVisualizationMode( Ogre::IrradianceField::DebugVisualizationNone ),
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
            mDebugVisualizationMode =
                ( mDebugVisualizationMode + 1u ) % ( Ogre::VctVoxelizer::DebugVisualizationNone + 2u );
        }
        else
        {
            mDebugVisualizationMode =
                ( mDebugVisualizationMode + Ogre::VctVoxelizer::DebugVisualizationNone + 2u - 1u ) %
                ( Ogre::VctVoxelizer::DebugVisualizationNone + 2u );
        }
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        if( mDebugVisualizationMode <= Ogre::VctVoxelizer::DebugVisualizationNone )
        {
            mVctLighting->setDebugVisualization( false, sceneManager );
            mVoxelizer->setDebugVisualization(
                static_cast<Ogre::VctVoxelizer::DebugVisualizationMode>( mDebugVisualizationMode ),
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

        Ogre::FastArray<Ogre::Item *>::const_iterator itor = mItems.begin();
        Ogre::FastArray<Ogre::Item *>::const_iterator end = mItems.end();
        while( itor != end )
        {
            ( *itor )->setVisible( showItems );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::toggletVctQuality()
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setVctFullConeCount( !hlmsPbs->getVctFullConeCount() );
    }
    //-----------------------------------------------------------------------------------
    VoxelizerGameState::GiMode VoxelizerGameState::getGiMode() const
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
    void VoxelizerGameState::cycleIfdProbeVisualizationMode( bool bPrev )
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
    void VoxelizerGameState::cycleIrradianceField( bool bPrev )
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        VoxelizerGameState::GiMode giMode = getGiMode();

        if( !bPrev )
        {
            giMode = static_cast<VoxelizerGameState::GiMode>( ( giMode + 1u ) % NumGiModes );
        }
        else
        {
            giMode =
                static_cast<VoxelizerGameState::GiMode>( ( giMode + NumGiModes - 1u ) % NumGiModes );
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
    void VoxelizerGameState::voxelizeScene()
    {
        TODO_do_this_in_voxelizer;
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
    void VoxelizerGameState::cycleScenes( bool bPrev )
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

        voxelizeScene();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::destroyCurrentScene()
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
    void VoxelizerGameState::createCornellScene()
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
    void VoxelizerGameState::createSibenikScene()
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
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createStressScene()
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
            const float yPos = static_cast<float>( i / ( 16u * 16u ) );

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
    void VoxelizerGameState::createScene01()
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
        light->setPowerScale( Ogre::Math::PI );  // Since we don't do HDR, counter the PBS' division by
                                                 // PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mCameraController = new CameraController( mGraphicsSystem, false );

        TutorialGameState::createScene01();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        mVoxelizer = new Ogre::VctVoxelizer( Ogre::Id::generateNewId<Ogre::VctVoxelizer>(),
                                             root->getRenderSystem(), root->getHlmsManager(), true );
        {
            mIrradianceField = new Ogre::IrradianceField( root, sceneManager );

            Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();

            assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs =
                static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
            hlmsPbs->setIrradianceField( mIrradianceField );
        }

        mGraphicsSystem->getCamera()->setPosition( Ogre::Vector3( 0.0f, 1.8f, 2.5f ) );

        createCornellScene();
        // createStressScene();
        // createSibenikScene();
        voxelizeScene();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::destroyScene()
    {
        delete mIrradianceField;
        mIrradianceField = 0;
        delete mVctLighting;
        mVctLighting = 0;
        delete mVoxelizer;
        mVoxelizer = 0;

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

        if( getGiMode() == IfdVct || getGiMode() == IfdOnly )
            mIrradianceField->update( mUseRasterIrradianceField ? 4u : 200u );
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    bool VoxelizerGameState::needsVoxels() const
    {
        return !( getGiMode() == IfdOnly && mUseRasterIrradianceField );
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
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
    void VoxelizerGameState::keyReleased( const SDL_KeyboardEvent &arg )
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
            const VoxelizerGameState::GiMode giMode = getGiMode();
            if( giMode == IfdVct || giMode == IfdOnly )
            {
                mUseRasterIrradianceField = !mUseRasterIrradianceField;
                voxelizeScene();
            }
        }
        else if( arg.keysym.sym == SDLK_F9 )
        {
            const VoxelizerGameState::GiMode giMode = getGiMode();
            if( giMode == IfdVct || giMode == IfdOnly )
                cycleIfdProbeVisualizationMode( arg.keysym.mod & ( KMOD_LSHIFT | KMOD_RSHIFT ) );
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
