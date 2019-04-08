
#include "VoxelizerGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"
#include "OgreMesh.h"
#include "OgreMeshManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2Serializer.h"

#include "OgreCamera.h"
#include "OgreRenderWindow.h"

#include "OgreRoot.h"
#include "Vct/OgreVctVoxelizer.h"

using namespace Demo;

#define TODO_do_this_in_voxelizer

namespace Demo
{
    Ogre::VctVoxelizer *voxelizer = 0;
    VoxelizerGameState::VoxelizerGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription )
    {
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::createScene01(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        Ogre::v1::MeshPtr v1Mesh;
        Ogre::MeshPtr Voxelizer;

        //---------------------------------------------------------------------------------------
        //Import Athene to v2 and render it without saving to disk.
        //---------------------------------------------------------------------------------------

        //Load the v1 mesh. Notice the v1 namespace
        //Also notice the HBU_STATIC flag; since the HBU_WRITE_ONLY
        //bit would prohibit us from reading the data for importing.
        v1Mesh = Ogre::v1::MeshManager::getSingleton().load(
                    "athene.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                    Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC );

        //Create a v2 mesh to import to, with a different name (arbitrary).
        Voxelizer = Ogre::MeshManager::getSingleton().createManual(
                    "athene.mesh Imported", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        bool halfPosition   = true;
        bool halfUVs        = true;
        bool useQtangents   = true;

        //Import the v1 mesh to v2
        Voxelizer->importV1( v1Mesh.get(), halfPosition, halfUVs, useQtangents );

        //We don't need the v1 mesh. Free CPU memory, get it out of the GPU.
        //Leave it loaded if you want to use athene with v1 Entity.
        v1Mesh->unload();

        //Create an Item with the model we just imported.
        //Notice we use the name of the imported model. We could also use the overload
        //with the mesh pointer:
        //  item = sceneManager->createItem( Voxelizer, Ogre::SCENE_DYNAMIC );
        Ogre::Item *item = sceneManager->createItem( "athene.mesh Imported",
                                                     Ogre::ResourceGroupManager::
                                                     AUTODETECT_RESOURCE_GROUP_NAME,
                                                     Ogre::SCENE_DYNAMIC );
        Ogre::SceneNode *sceneNode = sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->attachObject( item );
        sceneNode->scale( 0.1f, 0.1f, 0.1f );

        Ogre::Light *light = sceneManager->createLight();
        Ogre::SceneNode *lightNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setPowerScale( Ogre::Math::PI ); //Since we don't do HDR, counter the PBS' division by PI
        light->setType( Ogre::Light::LT_DIRECTIONAL );
        light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

        mCameraController = new CameraController( mGraphicsSystem, false );

        TutorialGameState::createScene01();

        TODO_do_this_in_voxelizer;
        sceneManager->updateSceneGraph();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        voxelizer =
                new Ogre::VctVoxelizer( Ogre::Id::generateNewId<Ogre::VctVoxelizer>(),
                                        root->getRenderSystem(), root->getHlmsManager() );
        voxelizer->addItem( item, false );
        voxelizer->autoCalculateRegion();
        voxelizer->dividideOctants( 1u, 1u, 1u );
        voxelizer->build();
    }
    //-----------------------------------------------------------------------------------
    void VoxelizerGameState::update( float timeSinceLast )
    {
        //voxelizer->build();
    }
}
