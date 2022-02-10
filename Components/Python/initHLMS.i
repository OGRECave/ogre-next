%{
#include <OgreArchiveManager.h>
#include <OgreHlmsManager.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsPbs.h>
%}

// TODO this is not good place for this function ... but is lot of code used in almost any Ogre based project without changes (changes structure of HLMS dirs is not recomended, HMLS resorces must be upgraded with Ogre, etc ...), so maybe Ogre API should provide those helper function?

%inline %{
void initHLMS(const Ogre::String& path, const Ogre::String& debugPath = Ogre::BLANKSTRING) {
	Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
	const Ogre::String rootHlmsFolder(path);
	const Ogre::String archiveType("FileSystem");
	Ogre::String mainFolderPath;
	Ogre::StringVector libraryFoldersPaths;
	Ogre::StringVector::const_iterator libraryFolderPathIt;
	Ogre::StringVector::const_iterator libraryFolderPathEn;
	
	Ogre::HlmsUnlit *hlmsUnlit = 0;
	Ogre::HlmsPbs *hlmsPbs = 0;
	
	// Create & Register HlmsUnlit
	{
		// Get the path to all the subdirectories used by HlmsUnlit
		Ogre::HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
		Ogre::Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );
		
		// Get the library archive(s)
		Ogre::ArchiveVec archiveUnlitLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while( libraryFolderPathIt != libraryFolderPathEn )
		{
			Ogre::Archive *archiveLibrary = archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
			archiveUnlitLibraryFolders.push_back( archiveLibrary );
			++libraryFolderPathIt;
		}
		
		// Create and register the unlit Hlms
		hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
		if (debugPath != Ogre::BLANKSTRING)
			hlmsUnlit->setDebugOutputPath(true, true, debugPath);
		else
			hlmsUnlit->setDebugOutputPath(false, false);
		Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
	}
	
	// Create & Register HlmsPbs
	{
		Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
		Ogre::Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath, archiveType, true );
		
		Ogre::ArchiveVec archivePbsLibraryFolders;
		libraryFolderPathIt = libraryFoldersPaths.begin();
		libraryFolderPathEn = libraryFoldersPaths.end();
		while( libraryFolderPathIt != libraryFolderPathEn )
		{
			Ogre::Archive *archiveLibrary = archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, archiveType, true );
			archivePbsLibraryFolders.push_back( archiveLibrary );
			++libraryFolderPathIt;
		}
		
		hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
		if (debugPath != Ogre::BLANKSTRING)
			hlmsPbs->setDebugOutputPath(true, true, debugPath);
		else
			hlmsPbs->setDebugOutputPath(false, false);
		Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
	}
	
	// fixes for 3D11 ...
	Ogre::RenderSystem *renderSystem = Ogre::Root::getSingleton().getRenderSystem();
	if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" ) {
		//Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
		//and below to avoid saturating AMD's discard limit (8MB) or
		//saturate the PCIE bus in some low end machines.
		bool supportsNoOverwriteOnTextureBuffers;
		renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV", &supportsNoOverwriteOnTextureBuffers );
		
		if( !supportsNoOverwriteOnTextureBuffers ) {
			hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
			hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
		}
	}
}
%}
