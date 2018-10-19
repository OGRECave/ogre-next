
#include "ParticlesGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMeshManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreParticleSystem.h"

#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

using namespace Demo;

namespace Demo
{
    ParticlesGameState::ParticlesGameState(const Ogre::String &helpDescription) :
        TutorialGameState(helpDescription),
        mAngle(0)
    {
    }
    //-----------------------------------------------------------------------------------
    void ParticlesGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        
        // Create camera
        mCameraController = new CameraController( mGraphicsSystem, false );
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( Ogre::Vector3( 0, 2.5, 4 ) );
        
        // Setup lighting
        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( Ogre::Math::PI ); //Since we don't do HDR, counter the PBS' division by PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3(-1, -1, -1).normalisedCopy() );
        
        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );
        
        // Add a floor
        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 5.0f, 5.0f,
                                            1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );
 
        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );
        
        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "Marble" );
            
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, 0, 0 );
            sceneNode->attachObject( item );
            
            //Change the addressing mode of the roughness map to wrap via code.
            //Detail maps default to wrap, but the rest to clamp.
            assert( dynamic_cast<Ogre::HlmsPbsDatablock*>( item->getSubItem(0)->getDatablock() ) );
            Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                                                            item->getSubItem(0)->getDatablock() );
            //Make a hard copy of the sampler block
            Ogre::HlmsSamplerblock samplerblock( *datablock->getSamplerblock( Ogre::PBSM_ROUGHNESS ) );
            samplerblock.mU = Ogre::TAM_WRAP;
            samplerblock.mV = Ogre::TAM_WRAP;
            samplerblock.mW = Ogre::TAM_WRAP;
            //Set the new samplerblock. The Hlms system will
            //automatically create the API block if necessary
            datablock->setSamplerblock( Ogre::PBSM_ROUGHNESS, samplerblock );    
        }        
        
        // Create particle system
        mParticleSystem = sceneManager->createParticleSystem("Examples/Smoke");
        mParticlesNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                createChildSceneNode( Ogre::SCENE_DYNAMIC );
        mParticlesNode->attachObject( mParticleSystem );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void ParticlesGameState::update( float timeSinceLast )
    {        
        const Ogre::Real angSpd = 0.5;
        mAngle += angSpd * timeSinceLast;
        const Ogre::Real radius = 1.2;
        mParticlesNode->setPosition(Ogre::Math::Cos(mAngle)*radius, 1.01, Ogre::Math::Sin(mAngle)*radius);
        
        TutorialGameState::update( timeSinceLast );
    }
}
