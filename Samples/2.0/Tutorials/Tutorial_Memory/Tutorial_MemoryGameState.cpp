
#include "Tutorial_MemoryGameState.h"
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
#include "OgreHlmsUnlitDatablock.h"

//Reuse the scene from SceneFormat which is a "heavy" one.
//We don't want to focus on geometry loading here, we want
//to focus on memory management functions in this sample
#include "../../ApiUsage/SceneFormat/SceneFormatGameState.h"
#include "../../ApiUsage/SceneFormat/SceneFormatGameState.cpp"

#include "OgreTextureGpuManager.h"
#include "OgreLwString.h"

using namespace Demo;

namespace Demo
{
    MemoryGameState::MemoryGameState( const Ogre::String &helpDescription ) :
        TutorialGameState( helpDescription ),
        mSceneGenerator( 0 ),
        mTightMemoryBudget( false )
    {
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::createCleanupScene(void)
    {
        destroyCleanupScene();

        if( !mSceneGenerator )
        {
            mSceneGenerator = new SceneFormatGameState( "" );
            mSceneGenerator->_notifyGraphicsSystem( mGraphicsSystem );
        }
        mSceneGenerator->generateScene();
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::destroyCleanupScene(void)
    {
        if( mSceneGenerator )
            mSceneGenerator->resetScene();

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().getByName( "Sphere1000.mesh" );
        if( mesh )
            mesh->unload();
        mesh = Ogre::MeshManager::getSingleton().getByName( "Cube_d.mesh" );
        if( mesh )
            mesh->unload();
    }
    //-----------------------------------------------------------------------------------
    bool MemoryGameState::isSceneLoaded(void) const
    {
        return true;
    }
    //-----------------------------------------------------------------------------------
    template <typename T, size_t MaxNumTextures>
    void MemoryGameState::unloadTexturesFromUnusedMaterials( Ogre::HlmsDatablock *datablock,
                                                             std::set<Ogre::TextureGpu*> &usedTex,
                                                             std::set<Ogre::TextureGpu*> &unusedTex )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<T*>( datablock ) );
        T *derivedDatablock = static_cast<T*>( datablock );

        for( size_t texUnit=0; texUnit<MaxNumTextures; ++texUnit )
        {
            //Check each texture from the material
            Ogre::TextureGpu *tex = derivedDatablock->getTexture( texUnit );
            if( tex )
            {
                //If getLinkedRenderables is empty, then the material is not in use,
                //and thus so is potentially the texture
                if( !datablock->getLinkedRenderables().empty() )
                    usedTex.insert( tex );
                else
                    unusedTex.insert( tex );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::unloadTexturesFromUnusedMaterials(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

        std::set<Ogre::TextureGpu*> usedTex;
        std::set<Ogre::TextureGpu*> unusedTex;

        //Check each material from each Hlms (except low level) to see if their material is
        //currently in use. If it's not, then its textures may be not either
        for( size_t i=Ogre::HLMS_PBS; i<Ogre::HLMS_MAX; ++i )
        {
            Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );

            if( hlms )
            {
                const Ogre::Hlms::HlmsDatablockMap &datablocks = hlms->getDatablockMap();

                Ogre::Hlms::HlmsDatablockMap::const_iterator itor = datablocks.begin();
                Ogre::Hlms::HlmsDatablockMap::const_iterator end  = datablocks.end();

                while( itor != end )
                {
                    if( i == Ogre::HLMS_PBS )
                    {
                        unloadTexturesFromUnusedMaterials<Ogre::HlmsPbsDatablock,
                                Ogre::NUM_PBSM_TEXTURE_TYPES>( itor->second.datablock,
                                                               usedTex, unusedTex );
                    }
                    else if( i == Ogre::HLMS_UNLIT )
                    {
                        unloadTexturesFromUnusedMaterials<Ogre::HlmsUnlitDatablock,
                                Ogre::NUM_UNLIT_TEXTURE_TYPES>( itor->second.datablock,
                                                                usedTex, unusedTex );
                    }

                    ++itor;
                }
            }
        }

        //Unload all unused textures, unless they're also in the "usedTex" (a texture may be
        //set to a material that is currently unused, and also in another material in use)
        std::set<Ogre::TextureGpu*>::const_iterator itor = unusedTex.begin();
        std::set<Ogre::TextureGpu*>::const_iterator end  = unusedTex.end();

        while( itor != end )
        {
            if( usedTex.find( *itor ) == usedTex.end() )
                (*itor)->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::unloadUnusedTextures(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();

        const Ogre::TextureGpuManager::ResourceEntryMap &entries = textureGpuManager->getEntries();

        Ogre::TextureGpuManager::ResourceEntryMap::const_iterator itor = entries.begin();
        Ogre::TextureGpuManager::ResourceEntryMap::const_iterator end  = entries.end();

        while( itor != end )
        {
            const Ogre::TextureGpuManager::ResourceEntry &entry = itor->second;

            const Ogre::vector<Ogre::TextureGpuListener*>::type &listeners =
                    entry.texture->getListeners();

            bool canBeUnloaded = true;

            Ogre::vector<Ogre::TextureGpuListener*>::type::const_iterator itListener = listeners.begin();
            Ogre::vector<Ogre::TextureGpuListener*>::type::const_iterator enListener = listeners.end();

            while( itListener != enListener )
            {
                //We must use dynamic_cast because we don't know if it's safe to cast
                Ogre::HlmsDatablock *datablock = dynamic_cast<Ogre::HlmsDatablock*>( *itListener );
                if( datablock )
                    canBeUnloaded = false;

                canBeUnloaded &= (*itListener)->shouldStayLoaded( entry.texture );

                ++itListener;
            }

            if( entry.texture->getTextureType() != Ogre::TextureTypes::Type2D ||
                !entry.texture->hasAutomaticBatching() ||
                !entry.texture->isTexture() ||
                entry.texture->isRenderToTexture() ||
                entry.texture->isUav() )
            {
                //This is likely a texture internal to us (i.e. a PCC probe)
                //Note that cubemaps loaded from file will also fall here, at
                //least the way I wrote the if logic.
                //
                //You may have to customize this further
                canBeUnloaded = false;
            }

            if( canBeUnloaded )
                entry.texture->scheduleTransitionTo( Ogre::GpuResidency::OnStorage );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::minimizeMemory(void)
    {
        setTightMemoryBudget();
        unloadTexturesFromUnusedMaterials();
        unloadUnusedTextures();

        Ogre::SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
        sceneManager->shrinkToFitMemoryPools();

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();
        Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();
        vaoManager->cleanupEmptyPools();
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::setTightMemoryBudget()
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();

        textureGpuManager->setStagingTextureMaxBudgetBytes( 8u * 1024u * 1024u );
        textureGpuManager->setWorkerThreadMaxPreloadBytes( 8u * 1024u * 1024u );
        textureGpuManager->setWorkerThreadMaxPerStagingTextureRequestBytes( 4u * 1024u * 1024u );

        Ogre::TextureGpuManager::BudgetEntryVec budget;
        textureGpuManager->setWorkerThreadMinimumBudget( budget );

        mTightMemoryBudget = true;
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::setRelaxedMemoryBudget(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && \
    OGRE_PLATFORM != OGRE_PLATFORM_ANDROID && \
    OGRE_ARCH_TYPE != OGRE_ARCHITECTURE_32
        textureGpuManager->setStagingTextureMaxBudgetBytes( 256u * 1024u * 1024u );
#else
        textureGpuManager->setStagingTextureMaxBudgetBytes( 128u * 1024u * 1024u );
#endif
        textureGpuManager->setWorkerThreadMaxPreloadBytes( 256u * 1024u * 1024u );
        textureGpuManager->setWorkerThreadMaxPerStagingTextureRequestBytes( 64u * 1024u * 1024u );

        textureGpuManager->setWorkerThreadMinimumBudget( mDefaultBudget );

        mTightMemoryBudget = false;
    }
    //-----------------------------------------------------------------------------------
    size_t MemoryGameState::getNumSlicesFor( Ogre::TextureGpu *texture,
                                             Ogre::TextureGpuManager *textureManager )
    {
        if( mTightMemoryBudget )
            return 1u;
        else
            return Ogre::DefaultTextureGpuManagerListener::getNumSlicesFor( texture, textureManager );
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::createScene01(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();
        textureGpuManager->setTextureGpuManagerListener( this );

        //setTightMemoryBudget();
        mDefaultBudget = textureGpuManager->getBudget();

        Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                            1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC,
                                            Ogre::v1::HardwareBuffer::HBU_STATIC );

        Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
                    "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

        planeMesh->importV1( planeMeshV1.get(), true, true, true );
        planeMeshV1->unload();
        Ogre::v1::MeshManager::getSingleton().remove( planeMeshV1 );

        createCleanupScene();
        mCameraController = new CameraController( mGraphicsSystem, false );
        TutorialGameState::createScene01();
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::destroyScene(void)
    {
        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();
        textureGpuManager->setTextureGpuManagerListener( 0 );

        destroyCleanupScene();
        delete mSceneGenerator;
        mSceneGenerator = 0;
        TutorialGameState::destroyScene();
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::update( float timeSinceLast )
    {
        TutorialGameState::update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        TutorialGameState::generateDebugText( timeSinceLast, outText );

        outText += "\nPress F2 to switch to load scene";
        outText += "\nPress F3 to switch to destroy scene";
        outText += "\nPress F4 to toggle tight memory budget ";
        outText += mTightMemoryBudget ? "[Tight]" : "[Relaxed]";
        outText += "\nPress F5 to minimize memory";

        //NOTE: Some of these routines to retrieve memory may be relatively "expensive".
        //So don't call them every frame in the final build for the user.

        Ogre::Root *root = mGraphicsSystem->getRoot();
        Ogre::RenderSystem *renderSystem = root->getRenderSystem();
        Ogre::VaoManager *vaoManager = renderSystem->getVaoManager();
        //Note that VaoManager::getMemoryStats gives us a lot of info that we don't show on screen!
        //Dumping to a Log is highly recommended!
        Ogre::VaoManager::MemoryStatsEntryVec memoryStats;
        size_t freeBytes;
        size_t capacityBytes;
        vaoManager->getMemoryStats( memoryStats, capacityBytes, freeBytes, 0 );

        const size_t bytesToMb = 1024u * 1024u;
        char tmpBuffer[256];
        Ogre::LwString text( Ogre::LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        text.clear();
        text.a( "\n\nGPU buffer pools (meshes, const, texture, indirect & uav buffers): ",
                (Ogre::uint32)((capacityBytes - freeBytes) / bytesToMb), "/",
                (Ogre::uint32)(capacityBytes / bytesToMb), " MB" );
        outText += text.c_str();

        Ogre::TextureGpuManager *textureGpuManager = renderSystem->getTextureGpuManager();
        size_t textureBytesCpu, textureBytesGpu, usedStagingTextureBytes, availableStagingTextureBytes;
        textureGpuManager->getMemoryStats( textureBytesCpu, textureBytesGpu,
                                           usedStagingTextureBytes, availableStagingTextureBytes );

        text.clear();
        text.a( "\nGPU StagingTextures. In use: ",
                (Ogre::uint32)(usedStagingTextureBytes / bytesToMb), " MB. Available: ",
                (Ogre::uint32)(availableStagingTextureBytes / bytesToMb), " MB. Total:",
                (Ogre::uint32)((usedStagingTextureBytes + availableStagingTextureBytes) / bytesToMb) );
        outText += text.c_str();

        text.clear();
        text.a( "\nGPU Textures:\t", (Ogre::uint32)(textureBytesGpu / bytesToMb), " MB" );
        text.a( "\nCPU Textures:\t", (Ogre::uint32)(textureBytesCpu / bytesToMb), " MB" );
        text.a( "\nTotal GPU:\t",
                (Ogre::uint32)((capacityBytes + textureBytesGpu +
                                usedStagingTextureBytes + availableStagingTextureBytes) /
                               bytesToMb), " MB" );
        outText += text.c_str();
    }
    //-----------------------------------------------------------------------------------
    void MemoryGameState::keyReleased( const SDL_KeyboardEvent &arg )
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
            if( mTightMemoryBudget )
                setRelaxedMemoryBudget();
            else
                setTightMemoryBudget();
        }
        else if( arg.keysym.sym == SDLK_F5 )
        {
            minimizeMemory();
        }
        else
        {
            TutorialGameState::keyReleased( arg );
        }
    }
}
