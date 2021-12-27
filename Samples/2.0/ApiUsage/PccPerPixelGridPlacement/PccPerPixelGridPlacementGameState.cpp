
#include "PccPerPixelGridPlacementGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"

#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"

#include "OgreCamera.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreRoot.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "OgreLwString.h"

#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"
#include "Cubemaps/OgrePccPerPixelGridPlacement.h"

using namespace Demo;

namespace Demo
{
    PccPerPixelGridPlacementGameState::PccPerPixelGridPlacementGameState(
        const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mParallaxCorrectedCubemap( 0 ),
        mUseDpm2DArray( false )
    {
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::setupParallaxCorrectCubemaps()
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        if( mParallaxCorrectedCubemap )
        {
            hlmsPbs->setParallaxCorrectedCubemap( 0 );

            delete mParallaxCorrectedCubemap;
            mParallaxCorrectedCubemap = 0;
        }

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::CompositorManager2 *compositorManager = root->getCompositorManager2();
        Ogre::CompositorWorkspaceDef *workspaceDef =
            compositorManager->getWorkspaceDefinition( "LocalCubemapsProbeWorkspace" );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        mParallaxCorrectedCubemap = new Ogre::ParallaxCorrectedCubemapAuto(
            Ogre::Id::generateNewId<Ogre::ParallaxCorrectedCubemapAuto>(), root, sceneManager,
            workspaceDef );

        mParallaxCorrectedCubemap->setUseDpm2DArray( mUseDpm2DArray );
        mUseDpm2DArray = mParallaxCorrectedCubemap->getUseDpm2DArray();  // Setting may be overriden

        Ogre::PccPerPixelGridPlacement pccGridPlacement;
        Ogre::Aabb aabb( Ogre::Vector3::ZERO, Ogre::Vector3( 0.5f ) );
        pccGridPlacement.setFullRegion( aabb );
        pccGridPlacement.setOverlap( Ogre::Vector3( 1.25f ) );
        pccGridPlacement.setParallaxCorrectedCubemapAuto( mParallaxCorrectedCubemap );
        pccGridPlacement.buildStart( 256u, mGraphicsSystem->getCamera(), Ogre::PFG_RGBA8_UNORM_SRGB,
                                     0.02f, 10.0f );
        pccGridPlacement.buildEnd();

        hlmsPbs->setParallaxCorrectedCubemap( mParallaxCorrectedCubemap );
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::forceUpdateAllProbes()
    {
        const Ogre::CubemapProbeVec &probes = mParallaxCorrectedCubemap->getProbes();

        Ogre::CubemapProbeVec::const_iterator itor = probes.begin();
        Ogre::CubemapProbeVec::const_iterator end = probes.end();

        while( itor != end )
        {
            ( *itor )->mDirty = true;
            ++itor;
        }

        if( mParallaxCorrectedCubemap )
            mParallaxCorrectedCubemap->updateAllDirtyProbes();
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        // Per pixel reflections REQUIRE Forward Clustered
        sceneManager->setForwardClustered( true, 16, 8, 24, 4, 0, 16u, 0.2f, 10 );
        // sceneManager->setForwardClustered( true, 1, 1, 1, 96, 0, 3, 2, 50 );

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        {
            Ogre::Item *item = sceneManager->createItem( "InteriorCube.mesh" );
            item->setCastShadows( false );
            Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode();
            sceneNode->attachObject( item );
        }

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( 1.0f );
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mLightNodes[0] = lightNode;

        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f );  // Warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        // lightNode->setPosition( -2.0f, 6.0f, 10.0f );
        lightNode->setPosition( -12.0f, 6.0f, 8.0f );
        light->setDirection( Ogre::Vector3( 1.5, -1, -0.5 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        light->setSpotlightOuterAngle( Ogre::Degree( 80.0f ) );

        mLightNodes[1] = lightNode;

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f );  // Cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( 2.0f, 6.0f, -3.0f );
        light->setDirection( Ogre::Vector3( -0.5, -1, 0.5 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );
        light->setSpotlightOuterAngle( Ogre::Degree( 80.0f ) );

        mLightNodes[2] = lightNode;

        mCameraController = new CameraController( mGraphicsSystem, false );
        mCameraController->mCameraBaseSpeed = 1.0f;
        mCameraController->mCameraSpeedBoost = 10.0f;

        mGraphicsSystem->getCamera()->setPosition( 0, 0, 0.5 );

        TutorialGameState::createScene01();

        setupParallaxCorrectCubemaps();
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::destroyScene()
    {
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
        hlmsPbs->setParallaxCorrectedCubemap( 0 );

        delete mParallaxCorrectedCubemap;
        mParallaxCorrectedCubemap = 0;
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::update( float timeSinceLast )
    {
        // Have the parallax corrected cubemap system keep track of the camera.
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        mParallaxCorrectedCubemap->setUpdatedTrackedDataFromCamera( camera );

        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::generateDebugText( float timeSinceLast,
                                                               Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nPress F8 to switch between Cubemap Arrays & DPM ";
        outText += mUseDpm2DArray ? "[Dual Paraboloid Mapping 2D]" : "[Cubemap Arrays]";

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        outText += "\nCamera: ";
        outText += Ogre::StringConverter::toString( camera->getPosition().x ) + ", " +
                   Ogre::StringConverter::toString( camera->getPosition().y ) + ", " +
                   Ogre::StringConverter::toString( camera->getPosition().z );
    }
    //-----------------------------------------------------------------------------------
    void PccPerPixelGridPlacementGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F8 )
        {
            mUseDpm2DArray = !mUseDpm2DArray;
            setupParallaxCorrectCubemaps();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
