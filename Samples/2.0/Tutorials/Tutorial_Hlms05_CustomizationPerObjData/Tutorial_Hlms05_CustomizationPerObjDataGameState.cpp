
#include "Tutorial_Hlms05_CustomizationPerObjDataGameState.h"

#include "Tutorial_Hlms05_CustomizationPerObjData.h"
#include "Tutorial_Hlms05_MyHlmsPbs.h"

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
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpuManager.h"
#include "OgreWindow.h"

using namespace Demo;

Hlms05CustomizationPerObjDataGameState::Hlms05CustomizationPerObjDataGameState(
    const Ogre::String &helpDescription ) :
    TutorialGameState( helpDescription )
{
}
//-----------------------------------------------------------------------------
void Hlms05CustomizationPerObjDataGameState::createScene01()
{
    Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

    const float armsLength = 2.5f;

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
        for( int j = 0; j < 4; ++j )
        {
            Ogre::String meshName;

            if( i == j )
                meshName = "Sphere1000.mesh";
            else
                meshName = "Cube_d.mesh";

            Ogre::Item *item = sceneManager->createItem(
                meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                Ogre::SCENE_DYNAMIC );
            if( i % 2 == 0 )
                item->setDatablock( "Rocks" );
            else
                item->setDatablock( "Marble" );

            item->setVisibilityFlags( 0x000000001 );

            if( i == 0u )
            {
                item->getSubItem( 0 )->setCustomParameter(
                    Ogre::MyHlmsPbs::kColourId, Ogre::Vector4( 1.0f, 1.0f, float( j ) / 4.0f, 1.0f ) );
                mChangingItems.push_back( item );
            }

            const size_t idx = static_cast<size_t>( i * 4 + j );

            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                             ->createChildSceneNode( Ogre::SCENE_DYNAMIC );

            sceneNode->setPosition( ( Ogre::Real( i ) - 1.5f ) * armsLength, 2.0f,
                                    ( Ogre::Real( j ) - 1.5f ) * armsLength );
            sceneNode->setScale( 0.65f, 0.65f, 0.65f );

            sceneNode->roll( Ogre::Radian( (Ogre::Real)idx ) );

            sceneNode->attachObject( item );
        }
    }

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

    mCameraController = new CameraController( mGraphicsSystem, false );

    TutorialGameState::createScene01();
}
//-----------------------------------------------------------------------------
void Hlms05CustomizationPerObjDataGameState::update( float timeSinceLast )
{
    for( Ogre::Item *item : mChangingItems )
    {
        Ogre::Vector4 colour = item->getSubItem( 0 )->getCustomParameter( Ogre::MyHlmsPbs::kColourId );
        colour.z += timeSinceLast;
        Ogre::Real unusedIntPart;
        colour.z = modf( colour.z, &unusedIntPart );
        item->getSubItem( 0 )->setCustomParameter( Ogre::MyHlmsPbs::kColourId, colour );
    }

    TutorialGameState::update( timeSinceLast );
}
