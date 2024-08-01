
#ifndef Demo_Tutorial_Hlms04_AlwaysOnTopB_H
#define Demo_Tutorial_Hlms04_AlwaysOnTopB_H

#include "GraphicsSystem.h"

#include "System/Android/AndroidSystems.h"
#include "Tutorial_Hlms04_MyHlmsPbs.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreWindow.h"

#include "OgreArchiveManager.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"

namespace Demo
{
    class Hlms04AlwaysOnTopBGraphicsSystem final : public GraphicsSystem
    {
        Ogre::CompositorWorkspace *setupCompositor() override
        {
            Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
            return compositorManager->addWorkspace( mSceneManager, mRenderWindow->getTexture(), mCamera,
                                                    "PbsMaterialsWorkspace", true );
        }

        void setupResources() override
        {
            GraphicsSystem::setupResources();

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

            Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

            if( originalDataFolder.empty() )
                originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( originalDataFolder.end() - 1 ) != '/' )
                originalDataFolder += "/";

            const char *c_locations[] = {
                "2.0/scripts/materials/PbsMaterials",
                "2.0/scripts/materials/Tutorial_Hlms04_AlwaysOnTopB",
            };

            for( size_t i = 0; i < sizeof( c_locations ) / sizeof( c_locations[0] ); ++i )
            {
                Ogre::String dataFolder = originalDataFolder + c_locations[i];
                addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
            }
        }

        void registerHlms() override
        {
            GraphicsSystem::registerHlms();

            // Destroy the HlmsPbs that registerHlms(). We need to create it ourselves
            // with our custom pieces.
            Ogre::Root::getSingleton().getHlmsManager()->unregisterHlms( Ogre::HLMS_PBS );

            Ogre::ConfigFile cf;
            cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            Ogre::String rootHlmsFolder =
                Ogre::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
            Ogre::String rootHlmsFolder =
                mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

            if( rootHlmsFolder.empty() )
                rootHlmsFolder = AndroidSystems::isAndroid() ? "/" : "./";
            else if( *( rootHlmsFolder.end() - 1 ) != '/' )
                rootHlmsFolder += "/";

            // At this point rootHlmsFolder should be a valid path to the Hlms data folder

            Ogre::MyHlmsPbs *hlmsPbs = 0;

            // For retrieval of the paths to the different folders needed
            Ogre::String mainFolderPath;
            Ogre::StringVector libraryFoldersPaths;
            Ogre::StringVector::const_iterator libraryFolderPathIt;
            Ogre::StringVector::const_iterator libraryFolderPathEn;

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

            const Ogre::String &archiveType = getMediaReadArchiveType();

            {
                // Create & Register HlmsPbs
                // Do the same for HlmsPbs:
                Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
                Ogre::Archive *archivePbs =
                    archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );

                // Get the library archive(s)
                Ogre::ArchiveVec archivePbsLibraryFolders;
                libraryFolderPathIt = libraryFoldersPaths.begin();
                libraryFolderPathEn = libraryFoldersPaths.end();
                while( libraryFolderPathIt != libraryFolderPathEn )
                {
                    Ogre::Archive *archiveLibrary =
                        archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
                    archivePbsLibraryFolders.push_back( archiveLibrary );
                    ++libraryFolderPathIt;
                }

                {
                    // We now need to add our own customizations for this tutorial.
                    // The order in which we push to archivePbsLibraryFolders DOES matter
                    // since script order parsing matters.
                    Ogre::Archive *archiveLibrary = archiveManager.load(
                        rootHlmsFolder + "2.0/scripts/materials/Tutorial_Hlms04_AlwaysOnTopB",
                        archiveType, true );
                    archivePbsLibraryFolders.push_back( archiveLibrary );
                }

                // Create and register
                hlmsPbs = OGRE_NEW Ogre::MyHlmsPbs( archivePbs, &archivePbsLibraryFolders );

                Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
            }

            Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
            if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
            {
                // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
                // and below to avoid saturating AMD's discard limit (8MB) or
                // saturate the PCIE bus in some low end machines.
                bool supportsNoOverwriteOnTextureBuffers;
                renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                                  &supportsNoOverwriteOnTextureBuffers );

                if( !supportsNoOverwriteOnTextureBuffers )
                    hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
            }
        }

    public:
        Hlms04AlwaysOnTopBGraphicsSystem( GameState *gameState ) : GraphicsSystem( gameState ) {}
    };
}  // namespace Demo

#endif
