
#include "IesProfilesGameState.h"

#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreItem.h"
#include "OgreSceneManager.h"

#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"

#include "OgreCamera.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreHlmsUnlitDatablock.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreRoot.h"

#include "LightProfiles/OgreLightProfiles.h"

namespace Demo
{
    IesProfilesGameState::IesProfilesGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mLightProfiles( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    void IesProfilesGameState::createScene01( void )
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();

        // Set the texture mask to PBS.
        Ogre::Hlms *hlms = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
        assert( dynamic_cast<Ogre::HlmsPbs *>( hlms ) );
        Ogre::HlmsPbs *pbs = static_cast<Ogre::HlmsPbs *>( hlms );

        mLightProfiles = OGRE_NEW Ogre::LightProfiles( pbs, textureMgr );

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        // Setup the floor and wall
        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(
            "Plane v1", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f, 1, 1, true, 1, 4.0f, 4.0f,
            Ogre::Vector3::UNIT_Z, Ogre::v1::HardwareBuffer::HBU_STATIC,
            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
            "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );

        {
            // Floor
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "MirrorLike" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                                             ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
            sceneNode->attachObject( item );

            // Wall
            item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "MirrorLike" );
            sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
                            ->createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->pitch( Ogre::Degree( 90.0f ) );
            sceneNode->setPosition( 0, 0, -2.0 );
            sceneNode->attachObject( item );
        }

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

        sceneManager->setForwardClustered( true, 16, 8, 24, 96, 0, 0, 2, 50 );

        struct LightProfileParams
        {
            const char *name;
            float power;
        };

        // clang-format off
        const LightProfileParams lightProfiles[c_numAreaLights] =
        {
            { 0, Ogre::Math::PI },
            { "x-arrow-soft.ies", 180.0f },
            { "bollard.ies", 18.0f },
            { "star-focused.ies", 700.0f }
        };
        // clang-format on

        for( size_t i = 0; i < c_numAreaLights; ++i )
        {
            Ogre::Light *light = sceneManager->createLight();
            Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
            lightNode->attachObject( light );
            lightNode->setPosition( ( i - c_numAreaLights * 0.5f ) * 6.0f, 8.0f, -0.5f );
            light->setPowerScale( lightProfiles[i].power );
            light->setType( Ogre::Light::LT_SPOTLIGHT );
            light->setDirection( Ogre::Vector3( 0, -1, 0 ).normalisedCopy() );
            light->setSpotlightInnerAngle( Ogre::Degree( 160.0f ) );
            light->setSpotlightOuterAngle( Ogre::Degree( 170.0f ) );

            if( lightProfiles[i].name != 0 )
            {
                mLightProfiles->loadIesProfile(
                    lightProfiles[i].name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                    false );
                mLightProfiles->assignProfile( lightProfiles[i].name, light );
            }
        }

        mLightProfiles->build();

        mCameraController = new CameraController( mGraphicsSystem, false );

        Ogre::Camera *camera = mGraphicsSystem->getCamera();
        camera->setPosition( Ogre::Vector3( 0, 10, 25 ) );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void IesProfilesGameState::destroyScene( void )
    {
        OGRE_DELETE mLightProfiles;
        mLightProfiles = 0;
    }
    //-----------------------------------------------------------------------------------
    void IesProfilesGameState::update( float timeSinceLast )
    {
        TutorialGameState::update( timeSinceLast );
    }
}  // namespace Demo
