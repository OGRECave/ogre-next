
#include "GraphicsSystem.h"
#include "DecalsGameState.h"

#include "OgreSceneManager.h"
#include "OgreCamera.h"
#include "OgreRoot.h"
#include "OgreWindow.h"
#include "OgreConfigFile.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpuManager.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
    return Demo::MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
}

namespace Demo
{
    class DecalsGraphicsSystem : public GraphicsSystem
    {
        virtual Ogre::CompositorWorkspace* setupCompositor()
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(),
                                                    mCamera, "PbsMaterialsWorkspace", true );
        }

        virtual void setupResources(void)
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load(mResourcePath + "resources2.cfg");

            Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( dataFolder.empty() )
                dataFolder = "./";
            else if( *(dataFolder.end() - 1) != '/' )
                dataFolder += "/";

            dataFolder += "2.0/scripts/materials/PbsMaterials";

            addResourceLocation( dataFolder, "FileSystem", "General" );
        }

        void reserveDecalTextures(void)
        {
            /*
            Decals support having up to 3 texture arrays, one for diffuse, normal and emissive maps.
            That means that all your diffuse texture must share the same resolution & format.

            You CAN create the texture normally via TextureManager by creating a TEX_TYPE_2D_ARRAY
            texture and managing the slices yourself. There is no need to depend on the
            HlmsTextureManager, other than being a convenience.

            We must reserve the textures and load them first, before initialiseAllResourceGroups
            loads materials that may end up putting the textures in a different array.

            If we do not do this, then we have to assign them a different alias to load it twice
            on RAM, e.g. call
            textureManager->createOrRetrieveTexture( "different_name_floor_bump",
                                                     "floor_bump.PNG",... );
            */
            Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
            /*
            These pool IDs must be unique per texture array, to ensure textures go into the
            right array.
            However you'll see here that both decalDiffuseId & decalNormalId are the same.
            This is because normal maps, being TEXTURE_TYPE_NORMALS and of a different format
            (RG8_SNORM instead of RGBA8_UNORM) will end up in different arrays anyway.
            So the reasons they will end up in different pools anyway is because:
                1. One uses TEXTURE_TYPE_DIFFUSE, the other TEXTURE_TYPE_NORMALS
                2. One uses RGBA8_UNORM, the other RG8_SNORM

            For example if you want to load diffuse decals into one array, and textures for
            area lights into a different array, then you would use different pool IDs:
                decalDiffuseId = 1;
                areaLightsId = 2;
            */
            const Ogre::uint32 decalDiffuseId = 1;
            const Ogre::uint32 decalNormalId = 1;

            //TODO: These pools should be destroyed manually or else they will live
            //forever until Ogre shutdowns
            textureManager->reservePoolId( decalDiffuseId, 512u, 512u, 8u, 10u,
                                           Ogre::PFG_RGBA8_UNORM_SRGB );
            textureManager->reservePoolId( decalNormalId, 512u, 512u, 8u, 10u,
                                           Ogre::PFG_RG8_SNORM );

            /*
                Create a blank diffuse & normal map textures, so we can use index 0 to "disable" them
                if we want them disabled for a particular Decal.
                This is not necessary if you intend to have all your decals using both diffuse
                and normals.
            */
            Ogre::uint8 *blackBuffer = reinterpret_cast<Ogre::uint8*>(
                                           OGRE_MALLOC_SIMD( 512u * 512u * 4u,
                                                             Ogre::MEMCATEGORY_RESOURCE ) );
            memset( blackBuffer, 0, 512u * 512u * 4u );
            Ogre::Image2 blackImage;
            blackImage.loadDynamicImage( blackBuffer, 512u, 512u, 1u, Ogre::TextureTypes::Type2D,
                                         Ogre::PFG_RGBA8_UNORM_SRGB, true );
            blackImage.generateMipmaps( false, Ogre::Image2::FILTER_NEAREST );
            Ogre::TextureGpu *decalTexture = 0;
            decalTexture = textureManager->createOrRetrieveTexture(
                               "decals_disabled_diffuse",
                               Ogre::GpuPageOutStrategy::Discard,
                               Ogre::TextureFlags::AutomaticBatching |
                               Ogre::TextureFlags::ManualTexture,
                               Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, decalDiffuseId );
            decalTexture->setResolution( blackImage.getWidth(), blackImage.getHeight() );
            decalTexture->setNumMipmaps( blackImage.getNumMipmaps() );
            decalTexture->setPixelFormat( blackImage.getPixelFormat() );
            decalTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            blackImage.uploadTo( decalTexture, 0, decalTexture->getNumMipmaps() - 1u );

            blackImage.freeMemory();
            blackBuffer = reinterpret_cast<Ogre::uint8*>(
                              OGRE_MALLOC_SIMD( 512u * 512u * 2u, Ogre::MEMCATEGORY_RESOURCE ) );
            memset( blackBuffer, 0, 512u * 512u * 2u );
            blackImage.loadDynamicImage( blackBuffer, 512u, 512u, 1u, Ogre::TextureTypes::Type2D,
                                         Ogre::PFG_RG8_SNORM, true );
            blackImage.generateMipmaps( false, Ogre::Image2::FILTER_NEAREST );
            decalTexture = textureManager->createOrRetrieveTexture(
                               "decals_disabled_normals",
                               Ogre::GpuPageOutStrategy::Discard,
                               Ogre::TextureFlags::AutomaticBatching |
                               Ogre::TextureFlags::ManualTexture,
                               Ogre::TextureTypes::Type2D, Ogre::BLANKSTRING, 0, decalNormalId );
            decalTexture->setResolution( blackImage.getWidth(), blackImage.getHeight() );
            decalTexture->setNumMipmaps( blackImage.getNumMipmaps() );
            decalTexture->setPixelFormat( blackImage.getPixelFormat() );
            decalTexture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
            blackImage.uploadTo( decalTexture, 0, decalTexture->getNumMipmaps() - 1u );

            /*
                Now actually create the decals into the array with the pool ID we desire.
            */
            textureManager->createOrRetrieveTexture(
                        "floor_diffuse.PNG", Ogre::GpuPageOutStrategy::Discard,
                        Ogre::CommonTextureTypes::Diffuse,
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        decalDiffuseId );
            textureManager->createOrRetrieveTexture(
                        "floor_bump.PNG", Ogre::GpuPageOutStrategy::Discard,
                        Ogre::CommonTextureTypes::NormalMap,
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        decalNormalId );
        }

        virtual void loadResources(void)
        {
            registerHlms();

            loadTextureCache();
            loadHlmsDiskCache();

            reserveDecalTextures();

            // Initialise, parse scripts etc
            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );
        }

    public:
        DecalsGraphicsSystem( GameState *gameState ) :
            GraphicsSystem( gameState )
        {
        }
    };

    void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
                                         GraphicsSystem **outGraphicsSystem,
                                         GameState **outLogicGameState,
                                         LogicSystem **outLogicSystem )
    {
        DecalsGameState *gfxGameState = new DecalsGameState(
        ""
        "\n"
        "LEGAL: Uses Saint Peter's Basilica (C) by Emil Persson under CC Attrib 3.0 Unported\n"
        "See Samples/Media/materials/textures/Cubemaps/License.txt for more information." );

        GraphicsSystem *graphicsSystem = new DecalsGraphicsSystem( gfxGameState );

        gfxGameState->_notifyGraphicsSystem( graphicsSystem );

        *outGraphicsGameState = gfxGameState;
        *outGraphicsSystem = graphicsSystem;
    }

    void MainEntryPoints::destroySystems( GameState *graphicsGameState,
                                          GraphicsSystem *graphicsSystem,
                                          GameState *logicGameState,
                                          LogicSystem *logicSystem )
    {
        delete graphicsSystem;
        delete graphicsGameState;
    }

    const char* MainEntryPoints::getWindowTitle(void)
    {
        return "Screen Space Decals";
    }
}
