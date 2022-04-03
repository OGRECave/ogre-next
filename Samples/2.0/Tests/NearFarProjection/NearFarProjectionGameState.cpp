
#include "NearFarProjectionGameState.h"

#include "GraphicsSystem.h"

#include "OgreCamera.h"
#include "OgreItem.h"
#include "OgreLwString.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSceneManager.h"

using namespace Demo;

namespace Demo
{
    NearFarProjectionGameState::NearFarProjectionGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription )
    {
    }
    //-----------------------------------------------------------------------------------
    void NearFarProjectionGameState::createScene01()
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Z, 0.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Y, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true,
            true, true );

        {
            // We must alter the AABB because we want to always pass frustum culling
            // Otherwise frustum culling may hide bugs in the projection matrix math
            planeMesh->load();
            Ogre::Aabb aabb = planeMesh->getAabb();
            aabb.mHalfSize.z = aabb.mHalfSize.x;
            planeMesh->_setBounds( aabb );
        }

        Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
        mSceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                         ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
        mSceneNode->setScale( Ogre::Vector3( 1000.0f ) );
        mSceneNode->attachObject( item );

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( Ogre::Math::PI );  // Since we don't do HDR, counter the PBS' division by
                                                 // PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( 0, 0, 0 );
        camera->setOrientation( Ogre::Quaternion::IDENTITY );

        camera->setNearClipDistance( 0.5f );
        mSceneNode->setPosition( 0, 0, -0.5f + 1e-6f );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void NearFarProjectionGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        const Ogre::RenderSystem *renderSystem =
            mGraphicsSystem->getSceneManager()->getDestinationRenderSystem();

        TutorialGameState::generateDebugText( timeSinceLast, outText );
        outText += "\nZ Mode: ";
        outText += renderSystem->isReverseDepth() ? "[Reverse Z]" : "[Normal Z]";
        outText += "\nF2 to test behind near plane (should be blue)";
        outText += "\nF3 to test after near plane (should be grey)";
        outText += "\nF4 to test behind far plane (should be grey)";
        outText += "\nF5 to test after far plane (should be blue)";

        char tmpBuffer[256];
        Ogre::LwString tmpStr( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        tmpStr.a( "\nObject Z: ", -mSceneNode->_getDerivedPositionUpdated().z );
        outText += tmpStr.c_str();

        const Ogre::Camera *camera = mGraphicsSystem->getCamera();
        tmpStr.clear();
        tmpStr.a( "\nCamera Near: ", camera->getNearClipDistance(),
                  "; Far: ", camera->getFarClipDistance() );
        outText += tmpStr.c_str();
    }
    //-----------------------------------------------------------------------------------
    void NearFarProjectionGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( ( arg.keysym.mod & ~( KMOD_NUM | KMOD_CAPS ) ) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        Ogre::Camera *camera = mGraphicsSystem->getCamera();

        if( arg.keysym.sym == SDLK_F2 )
        {
            mSceneNode->setPosition( 0, 0, -camera->getNearClipDistance() + 1e-6f );
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            mSceneNode->setPosition( 0, 0, -camera->getNearClipDistance() - 1e-6f );
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            mSceneNode->setPosition( 0, 0, -camera->getFarClipDistance() + 0.5f );
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            mSceneNode->setPosition( 0, 0, -camera->getFarClipDistance() - 0.5f );
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}  // namespace Demo
