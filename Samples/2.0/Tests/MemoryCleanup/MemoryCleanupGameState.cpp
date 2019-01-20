
#include "MemoryCleanupGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"
#include "OgreItem.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreCamera.h"

#include "OgreRoot.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsPbs.h"

using namespace Demo;

namespace Demo
{
    MemoryCleanupGameState::MemoryCleanupGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mReleaseMemoryOnCleanup( true ),
        mReleaseGpuMemory( true )
    {
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::testSequence(void)
    {
        for( int j=0; j<100; ++j )
        {
            destroyCleanupScene();
            createCleanupScene();
        }
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::createCleanupScene(void)
    {
        destroyCleanupScene();

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        Ogre::SceneNode *staticRootNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC );

        const size_t numObjs = 10000;

        for( size_t i=0; i<numObjs; ++i )
        {
            VisibleItem visibleItem;
            visibleItem.item = sceneManager->createItem(
                                   (i & 0x01) ? "Cube_d.mesh" : "Sphere1000.mesh",
                                   Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                   Ogre::SCENE_STATIC );

            Ogre::SceneNode *sceneNode = staticRootNode->createChildSceneNode( Ogre::SCENE_STATIC );
            sceneNode->setPosition( (i % 5u) * 2.5f - 5.0f,
                                    (i % 4u) * 2.5f - 3.75f, 0.0f );
            sceneNode->attachObject( visibleItem.item );
            visibleItem.datablock = 0;
            mVisibleItems.push_back( visibleItem );
        }
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::destroyCleanupScene(void)
    {
        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();

        //Force FIFO removal on purpose to avoid triggering
        //best case scenario (best case scenario is LIFO)
        VisibleItemVec::const_iterator itor = mVisibleItems.begin();
        VisibleItemVec::const_iterator end  = mVisibleItems.end();

        while( itor != end )
        {
            Ogre::SceneNode *sceneNode = itor->item->getParentSceneNode();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );

            sceneManager->destroyItem( itor->item );
            ++itor;
        }

        mVisibleItems.clear();

        if( mReleaseMemoryOnCleanup )
            sceneManager->shrinkToFitMemoryPools();

        if( mReleaseGpuMemory )
        {
            const char *meshNames[2] = { "Cube_d.mesh", "Sphere1000.mesh" };
            for( size_t i=0; i<2u; ++i )
            {
                Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().getByName( meshNames[i] );
                //Do not unload the first one, to test cleanupEmptyPools
                //can deal with this correctly
                if( i != 0 && mesh )
                    mesh->unload();
            }

            Ogre::RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();
            Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();
            vaoManager->cleanupEmptyPools();
        }
    }
    //-----------------------------------------------------------------------------------
    bool MemoryCleanupGameState::isSceneLoaded(void) const
    {
        return !mVisibleItems.empty();
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::createScene01(void)
    {
        createCleanupScene();
        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::destroyScene(void)
    {
        destroyCleanupScene();
        TutorialGameState::destroyScene();
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::update( float timeSinceLast )
    {
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        outText += "\nPress F2 to switch to load scene";
        outText += "\nPress F3 to switch to destroy scene";
        outText += "\nPress F4 to switch to release memory upon destruction ";
        outText += mReleaseMemoryOnCleanup ? "[On]" : "[Off]";
        outText += "\nPress F5 for memory torture test";
    }
    //-----------------------------------------------------------------------------------
    void MemoryCleanupGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) != 0 )
        {
            TutorialGameState::keyReleased( arg );
            return;
        }

        if( arg.keysym.sym == SDLK_F2 )
        {
            createCleanupScene();
        }
        else if( arg.keysym.sym == SDLK_F3 )
        {
            destroyCleanupScene();
        }
        else if( arg.keysym.sym == SDLK_F4 )
        {
            mReleaseMemoryOnCleanup = !mReleaseMemoryOnCleanup;
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            testSequence();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
