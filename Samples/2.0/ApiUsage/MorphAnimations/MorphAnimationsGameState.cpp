
#include "MorphAnimationsGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMeshManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"

#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint.h"
#include "Animation/OgreSkeletonAnimation.h"

#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

using namespace Demo;

namespace Demo
{
    MorphAnimationsGameState::MorphAnimationsGameState(const Ogre::String &helpDescription) :
        TutorialGameState(helpDescription),
        mAccumulator(0)
    {
    }
    //-----------------------------------------------------------------------------------
    void MorphAnimationsGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        
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
            sceneNode->setPosition( 0, -1.02f, 0 );
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

        Ogre::v1::MeshPtr v1Mesh;
        Ogre::MeshPtr v2Mesh;

        bool halfPosition = true;
        bool halfUVs = true;
        bool useQtangents = false;
        bool halfPose = true;

        // Smiley
        {
            v1Mesh = Ogre::v1::MeshManager::getSingleton().load(
                "Smiley.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
            v2Mesh = Ogre::MeshManager::getSingleton().createManual(
                "Smiley.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            v2Mesh->importV1( v1Mesh.get(), halfPosition, halfUVs, useQtangents, halfPose);
            v1Mesh->unload();
        }

        Ogre::SceneNode *smileyNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                createChildSceneNode( Ogre::SCENE_DYNAMIC );

        mSmileyItem = sceneManager->createItem( "Smiley.mesh",
                                                Ogre::ResourceGroupManager::
                                                AUTODETECT_RESOURCE_GROUP_NAME,
                                                Ogre::SCENE_DYNAMIC );
        smileyNode->attachObject( mSmileyItem );
        smileyNode->setScale( Ogre::Vector3( 0.5 ) );
        smileyNode->setPosition( 0, 0.57f, 0 );
        smileyNode->setOrientation( Ogre::Quaternion( Ogre::Radian(1.5), Ogre::Vector3::UNIT_X ) );
        
        Ogre::Bone *bone = mSmileyItem->getSkeletonInstance()->getBone("Bone.001");
        bone->setOrientation( Ogre::Quaternion( Ogre::Radian( 0.4f ), Ogre::Vector3::UNIT_Y ) );

        // Spring

        {
            v1Mesh = Ogre::v1::MeshManager::getSingleton().load(
                "Spring.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
            v2Mesh = Ogre::MeshManager::getSingleton().createManual(
                "Spring.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            v2Mesh->importV1( v1Mesh.get(), halfPosition, halfUVs, useQtangents, halfPose );
            v1Mesh->unload();
        }

        Ogre::SceneNode *springNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                createChildSceneNode( Ogre::SCENE_DYNAMIC );

        mSpringItem = sceneManager->createItem( "Spring.mesh",
                                                Ogre::ResourceGroupManager::
                                                AUTODETECT_RESOURCE_GROUP_NAME,
                                                Ogre::SCENE_DYNAMIC );
        springNode->attachObject( mSpringItem );
        springNode->setScale( Ogre::Vector3( 1.2 ) );
        springNode->setPosition( 1, 0.57f, 0 );

        // Blob

        {
            v1Mesh = Ogre::v1::MeshManager::getSingleton().load(
                "Blob.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
            v2Mesh = Ogre::MeshManager::getSingleton().createManual(
                "Blob.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            v2Mesh->importV1( v1Mesh.get(), halfPosition, halfUVs, useQtangents, halfPose);
            v1Mesh->unload();
        }

        Ogre::SceneNode *blobNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                createChildSceneNode( Ogre::SCENE_DYNAMIC );

        mBlobItem = sceneManager->createItem( "Blob.mesh",
                                                Ogre::ResourceGroupManager::
                                                AUTODETECT_RESOURCE_GROUP_NAME,
                                                Ogre::SCENE_DYNAMIC );
        blobNode->attachObject( mBlobItem );
        blobNode->setScale( Ogre::Vector3( 0.4f ) );
        blobNode->setPosition( -1, 0.7f, 0.7f );

        // Add a material to the blob
        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    hlmsPbs->createDatablock(   "BlobMaterial",
                                                "BlobMaterial",
                                                Ogre::HlmsMacroblock(),
                                                Ogre::HlmsBlendblock(),
                                                Ogre::HlmsParamVec() ) );
        datablock->setDiffuse( Ogre::Vector3( 1.0f, 0.0f, 0.0f ) );
        datablock->setRoughness( 0.12f );
        datablock->setFresnel( Ogre::Vector3( 1.9f ), false );
        mBlobItem->setDatablock( datablock );

        // Lights
        
        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( Ogre::Math::PI ); //Since we don't do HDR, counter the PBS' division by PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3(-1, -1, -1).normalisedCopy() );
        
        sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f,
                                       Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f,
                                       -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );

        // Camera

        mCameraController = new CameraController( mGraphicsSystem, false );
        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( Ogre::Vector3( 0, 2.5, 4 ) );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void MorphAnimationsGameState::update( float timeSinceLast )
    {        
        mAccumulator += timeSinceLast;
        Ogre::SubItem* subItem;
        
        subItem = mSmileyItem->getSubItem( 0 );
        subItem->setPoseWeight( "Smile", (Ogre::Math::Sin(mAccumulator) + 1)/2 );
        subItem->setPoseWeight( "MouthOpen", (Ogre::Math::Sin(mAccumulator+1) + 1)/2 );
        subItem->setPoseWeight( "Sad", (Ogre::Math::Sin(mAccumulator+2) + 1)/2 );
        subItem->setPoseWeight( "EyesClosed", (Ogre::Math::Sin(mAccumulator+3) + 1)/2 );

        subItem = mSpringItem->getSubItem( 0 );
        subItem->setPoseWeight( "Compressed", (Ogre::Math::Sin(mAccumulator) + 1)/2 );

        subItem = mBlobItem->getSubItem( 0 );
        for( int i = 0; i < subItem->getNumPoses(); ++i )
        {
            subItem->setPoseWeight(i, Ogre::Math::Sin(mAccumulator * (1 + i * 0.1) * 3 + i) * 0.27 );
        }
        
        TutorialGameState::update( timeSinceLast );
    }
}
