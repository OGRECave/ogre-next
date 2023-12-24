
#include "Tutorial_Hlms04_AlwaysOnTopBGameState.h"

#include "Tutorial_Hlms04_AlwaysOnTopB.h"
#include "Tutorial_Hlms04_MyHlmsPbs.h"

#include "CameraController.h"
#include "GraphicsSystem.h"
#include "OgreCamera.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSceneManager.h"

using namespace Demo;

Hlms04AlwaysOnTopBGameState::Hlms04AlwaysOnTopBGameState( const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription )
{
}
//-----------------------------------------------------------------------------
void Hlms04AlwaysOnTopBGameState::createBar( const bool bAlwaysOnTop )
{
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    Ogre::Item *item = sceneManager->createItem(
        "Cube_d.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    Ogre::Item *clearClone = 0;

    Ogre::SceneNode *cameraNode = mGraphicsSystem->getCamera()->getParentSceneNode();
    Ogre::SceneNode *sceneNode = cameraNode->createChildSceneNode();
    sceneNode->setScale( 0.1f, 0.5f, 0.1f );
    sceneNode->setPosition( bAlwaysOnTop ? 0.4f : -0.4f, 0, -1.0f );
    sceneNode->pitch( Ogre::Degree( -30.0f ) );

    if( bAlwaysOnTop )
    {
        clearClone = sceneManager->createItem(
            "Cube_d.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
        clearClone->setCastShadows( false );
        sceneNode->attachObject( clearClone );

        if( item->getSkeletonInstance() )
        {
            // If the Item has skeleton animations, make sure they share the same skeleton
            // so they are in the exact same state (and also saves RAM & CPU).
            clearClone->useSkeletonInstanceFrom( item );
            mClones.push_back( clearClone );
        }
    }
    sceneNode->attachObject( item );

    if( bAlwaysOnTop )
    {
        const size_t numSubItems = item->getNumSubItems();
        for( size_t i = 0u; i < numSubItems; ++i )
        {
            Ogre::SubItem *subItemClone = clearClone->getSubItem( i );
            subItemClone->setCustomParameter( Ogre::MyHlmsPbs::kClearDepthId, Ogre::Vector4::ZERO );

            subItemClone->setDatablock( "ClearDepth" );

            // Make sure the clone gets rendered before the item; so the depth is cleared first.
            Ogre::SubItem *subItem = item->getSubItem( i );
            subItemClone->setRenderQueueSubGroup( 0u );
            subItem->setRenderQueueSubGroup( 1u );
        }
    }

    item->setRenderQueueGroup( 99u );
    if( clearClone )
        clearClone->setRenderQueueGroup( 99u );
}
//-----------------------------------------------------------------------------
void Hlms04AlwaysOnTopBGameState::createScene01()
{
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    {
        // We want the camera to be attached to its own node. We'll attach the two bars
        // to the camera so they follow the camera everywhere it goes.
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        const Ogre::Vector3 origPos = camera->getDerivedPosition();
        const Ogre::Quaternion origRot = camera->getDerivedOrientation();

        camera->setPosition( Ogre::Vector3::ZERO );
        camera->setOrientation( Ogre::Quaternion::IDENTITY );

        camera->detachFromParent();
        Ogre::SceneNode *cameraNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                          ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        cameraNode->attachObject( camera );
        cameraNode->setPosition( origPos + Ogre::Vector3( 0, -1.7f, -11.2f ) );
        cameraNode->setOrientation( origRot );
    }

    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
        "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
        Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
        Ogre::v1::HardwareBuffer::HBU_STATIC );

    Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
        "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true,
        true );

    {
        Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
        item->setDatablock( "Marble" );
        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setPosition( 0, -1, 0 );
        sceneNode->attachObject( item );

        // Change the addressing mode of the roughness map to wrap via code.
        // Detail maps default to wrap, but the rest to clamp.
        assert( dynamic_cast<Ogre::HlmsPbsDatablock *>( item->getSubItem( 0 )->getDatablock() ) );
        Ogre::HlmsPbsDatablock *datablock =
            static_cast<Ogre::HlmsPbsDatablock *>( item->getSubItem( 0 )->getDatablock() );
        // Make a hard copy of the sampler block
        Ogre::HlmsSamplerblock samplerblock( *datablock->getSamplerblock( Ogre::PBSM_ROUGHNESS ) );
        samplerblock.mU = Ogre::TAM_WRAP;
        samplerblock.mV = Ogre::TAM_WRAP;
        samplerblock.mW = Ogre::TAM_WRAP;
        // Set the new samplerblock. The Hlms system will
        // automatically create the API block if necessary
        datablock->setSamplerblock( Ogre::PBSM_ROUGHNESS, samplerblock );
    }

    for( int i = 0; i < 4; ++i )
    {
        Ogre::String meshName;

        meshName = "Cube_d.mesh";

        Ogre::Item *item = sceneManager->createItem(
            meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::SCENE_DYNAMIC );
        if( i % 2 == 0 )
            item->setDatablock( "Rocks" );
        else
            item->setDatablock( "Marble" );

        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );

        Ogre::Quaternion qRot( Ogre::Radian( ( float( i ) / 4.0f ) * Ogre::Math::TWO_PI ),
                               Ogre::Vector3::UNIT_Y );

        sceneNode->setPosition( qRot * Ogre::Vector3( 0, 2.0f, 3.0f ) );
        sceneNode->setOrientation( qRot );
        sceneNode->setScale( 3.0f, 2.0f, 0.1f );

        sceneNode->attachObject( item );
    }

    createBar( false );
    createBar( true );

    Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

    Ogre::Light *light = sceneManager->createLight();
    Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject( light );
    light->setPowerScale( 1.0f );
    light->setType( Ogre::Light::LT_DIRECTIONAL );
    light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

    sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                   Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                   -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

    mGraphicsSystem->createAtmosphere( light );

    light = sceneManager->createLight();
    lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject( light );
    light->setDiffuseColour( 0.8f, 0.4f, 0.2f );  // Warm
    light->setSpecularColour( 0.8f, 0.4f, 0.2f );
    light->setPowerScale( Ogre::Math::PI );
    light->setType( Ogre::Light::LT_SPOTLIGHT );
    lightNode->setPosition( -10.0f, 10.0f, 10.0f );
    light->setDirection( Ogre::Vector3( 1, -1, -1 ).normalisedCopy() );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

    light = sceneManager->createLight();
    lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject( light );
    light->setDiffuseColour( 0.2f, 0.4f, 0.8f );  // Cold
    light->setSpecularColour( 0.2f, 0.4f, 0.8f );
    light->setPowerScale( Ogre::Math::PI );
    light->setType( Ogre::Light::LT_SPOTLIGHT );
    lightNode->setPosition( 10.0f, 10.0f, -10.0f );
    light->setDirection( Ogre::Vector3( -1, -1, 1 ).normalisedCopy() );
    light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

    mCameraController = new CameraController( mGraphicsSystem, true );

    TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------
void Hlms04AlwaysOnTopBGameState::destroyScene()
{
    for( Ogre::Item *item : mClones )
    {
        if( item->sharesSkeletonInstance() )
            item->stopUsingSkeletonInstanceFromMaster();
    }
    mClones.clear();
}
//-----------------------------------------------------------------------------
void Hlms04AlwaysOnTopBGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
{
    TutorialGameState::generateDebugText( timeSinceLast, outText );

    outText +=
        "\n\nThe left bar clips through objects."
        "\nThe right bar is always on top."
        "\n\nPress F1 for more info on the technique.";
}
