
#include "MeshLodGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"
#include "OgreWindow.h"

#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreTextureGpuManager.h"
#include "OgreTextureFilters.h"
#include "OgreHlmsPbs.h"

#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"

using namespace Demo;

namespace Demo
{
    MeshLodGameState::MeshLodGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription )
    {
    }
    //-----------------------------------------------------------------------------------
    void MeshLodGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createByImportingV1(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    planeMeshV1.get(), true, true, true );

        {
            Ogre::Item *item = sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
            item->setDatablock( "Marble" );
            Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                                                    createChildSceneNode( Ogre::SCENE_DYNAMIC );
            sceneNode->setPosition( 0, -1, 0 );
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

        Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

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
        light->setDiffuseColour( 0.8f, 0.4f, 0.2f ); //Warm
        light->setSpecularColour( 0.8f, 0.4f, 0.2f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( -10.0f, 10.0f, 10.0f );
        light->setDirection( Ogre::Vector3( 1, -1, -1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        mLightNodes[1] = lightNode;

        light = sceneManager->createLight();
        lightNode = rootNode->createChildSceneNode();
        lightNode->attachObject( light );
        light->setDiffuseColour( 0.2f, 0.4f, 0.8f ); //Cold
        light->setSpecularColour( 0.2f, 0.4f, 0.8f );
        light->setPowerScale( Ogre::Math::PI );
        light->setType( Ogre::Light::LT_SPOTLIGHT );
        lightNode->setPosition( 10.0f, 10.0f, -10.0f );
        light->setDirection( Ogre::Vector3( -1, -1, 1 ).normalisedCopy() );
        light->setAttenuationBasedOnRadius( 10.0f, 0.01f );

        mLightNodes[2] = lightNode;

        mCameraController = new CameraController( mGraphicsSystem, false );

        Ogre::LodStrategyManager::getSingleton().setDefaultStrategy(
            Ogre::ScreenRatioPixelCountLodStrategy::getSingletonPtr() );

        // Load V1 mesh
        Ogre::v1::MeshPtr meshV1 = Ogre::v1::MeshManager::getSingleton().load(
            "Sinbad.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        // Generate LOD levels
        Ogre::LodConfig lodConfig;

        Ogre::MeshLodGenerator lodGenerator;
        lodGenerator.getAutoconfig( meshV1, lodConfig );

        lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

        for( Ogre::LodConfig::LodLevelList::iterator it = lodConfig.levels.begin();
             it != lodConfig.levels.end(); ++it )
        {
            // 33886.08 is a magic number
            it->distance = lodConfig.strategy->transformUserValue( it->distance / ( 33886.08f ) );
        }

        lodGenerator.generateLodLevels( lodConfig );
        // ----------------

        // Convert the mesh from v1 into v2
        Ogre::MeshPtr meshV2 = Ogre::MeshManager::getSingleton().createByImportingV1(
            "Sinbad_v2_import.mesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            meshV1.get(), true, true, true );
        meshV2->load();

        Ogre::ConfigFile cf;
        cf.load( mGraphicsSystem->getResourcePath() + "resources2.cfg" );

        /*
        Uncomment this to save the v2 mesh *with* LODs to disk
        Ogre::MeshSerializer serializer(
            mGraphicsSystem->getRoot()->getRenderSystem()->getVaoManager() );
        serializer.exportMesh(
            meshV2.get(), cf.getSetting( "DoNotUseAsResource", "Hlms", "" ) + "/models/Sinbad_v2.mesh"
        );*/

        meshV1->unload();
        Ogre::v1::MeshManager::getSingleton().remove( meshV1 );

        Ogre::HlmsManager *hlmsManager = mGraphicsSystem->getRoot()->getHlmsManager();
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

        Ogre::HlmsMacroblock macroblock;
        macroblock.mPolygonMode = Ogre::PM_WIREFRAME;

        Ogre::HlmsPbsDatablock *datablock =
            static_cast<Ogre::HlmsPbsDatablock *>( hlmsPbs->createDatablock(
                "Sinbad", "Sinbad", macroblock, Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );

        // Create item from imported mesh -> LOD working
        Ogre::Item *item = sceneManager->createItem( meshV2, Ogre::SCENE_STATIC );

        for( size_t i = 0; i < item->getNumSubItems(); ++i )
        {
            item->getSubItem( i )->setDatablock( datablock );
        }

        Ogre::SceneNode *node =
            sceneManager->getRootSceneNode()->createChildSceneNode( Ogre::SCENE_STATIC );
        node->setPosition( Ogre::Vector3( 0.0f, 2.5f, 0.0f ) );
        node->setScale( Ogre::Vector3( 0.5f ) );
        node->attachObject( item );

        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
}
