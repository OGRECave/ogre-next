#include "Android/OgreAPKFileSystemArchive.h"

#include <OgreLogManager.h>
#include <OgreString.h>
#include <OgreStringConverter.h>

namespace Ogre
{
    static std::map<String, std::vector<String> > mFiles;

    bool IsFolderParsed( String Folder )
    {
        bool parsed = false;
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( Folder );
        if( iter != mFiles.end() )
            parsed = true;
        return parsed;
    }

    void ParseFolder( AAssetManager *AssetMgr, String Folder )
    {
        std::vector<String> mFilenames;
        AAssetDir *dir = AAssetManager_openDir( AssetMgr, Folder.c_str() );
        const char *fileName = NULL;
        while( ( fileName = AAssetDir_getNextFileName( dir ) ) != NULL )
        {
            mFilenames.push_back( String( fileName ) );
        }
        mFiles.insert( std::make_pair( Folder, mFilenames ) );
    }

    APKFileSystemArchive::APKFileSystemArchive( const String &name, const String &archType,
                                                AAssetManager *assetMgr ) :
        Archive( name, archType ),
        mAssetMgr( assetMgr )
    {
        if( !mName.empty() && mName[0] == '/' )
            mName.erase( mName.begin() );

        mPathPreFix = mName;
        if( !mPathPreFix.empty() && mPathPreFix.back() != '/' )
            mPathPreFix += "/";

        if( !IsFolderParsed( mName ) )
        {
            ParseFolder( mAssetMgr, mName );
        }
    }

    APKFileSystemArchive::~APKFileSystemArchive()
    {
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( mName );
        iter->second.clear();
        mFiles.erase( iter );
        unload();
    }

    bool APKFileSystemArchive::isCaseSensitive() const { return true; }

    void APKFileSystemArchive::load() {}

    void APKFileSystemArchive::unload() {}

    DataStreamPtr APKFileSystemArchive::open( const Ogre::String &filename, bool readOnly )
    {
        DataStreamPtr stream;
        AAsset *asset =
            AAssetManager_open( mAssetMgr, ( mPathPreFix + filename ).c_str(), AASSET_MODE_BUFFER );
        if( asset )
        {
            const size_t length = static_cast<size_t>( AAsset_getLength( asset ) );
            void *membuf = OGRE_MALLOC( length, Ogre::MEMCATEGORY_GENERAL );
            memcpy( membuf, AAsset_getBuffer( asset ), length );
            AAsset_close( asset );

            stream = Ogre::DataStreamPtr(
                new Ogre::MemoryDataStream( filename, membuf, length, true, true ) );
        }
        return stream;
    }

    DataStreamPtr APKFileSystemArchive::create( const Ogre::String &filename )
    {
        return DataStreamPtr();
    }

    void APKFileSystemArchive::remove( const String &filename ) {}

    StringVectorPtr APKFileSystemArchive::list( bool recursive, bool dirs )
    {
        StringVectorPtr files( new StringVector );
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( mName );
        std::vector<String> fileList = iter->second;
        for( size_t i = 0; i < fileList.size(); i++ )
        {
            files->push_back( fileList[i] );
        }
        return files;
    }

    FileInfoListPtr APKFileSystemArchive::listFileInfo( bool recursive, bool dirs )
    {
        FileInfoListPtr files( new FileInfoList );
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( mName );
        std::vector<String> fileList = iter->second;
        for( size_t i = 0; i < fileList.size(); i++ )
        {
            AAsset *asset = AAssetManager_open( mAssetMgr, ( mPathPreFix + fileList[i] ).c_str(),
                                                AASSET_MODE_UNKNOWN );
            if( asset )
            {
                FileInfo info;
                info.archive = this;
                info.filename = fileList[i];
                info.path = mName;
                info.basename = fileList[i];
                info.compressedSize = static_cast<size_t>( AAsset_getLength( asset ) );
                info.uncompressedSize = info.compressedSize;
                files->push_back( info );
                AAsset_close( asset );
            }
        }
        return files;
    }

    StringVectorPtr APKFileSystemArchive::find( const String &pattern, bool recursive, bool dirs )
    {
        StringVectorPtr files( new StringVector );
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( mName );
        std::vector<String> fileList = iter->second;
        for( size_t i = 0; i < fileList.size(); i++ )
        {
            if( StringUtil::match( fileList[i], pattern ) )
                files->push_back( fileList[i] );
        }
        return files;
    }

    FileInfoListPtr APKFileSystemArchive::findFileInfo( const String &pattern, bool recursive,
                                                        bool dirs )
    {
        FileInfoListPtr files( new FileInfoList );
        std::map<String, std::vector<String> >::iterator iter = mFiles.find( mName );
        std::vector<String> fileList = iter->second;
        for( size_t i = 0; i < fileList.size(); i++ )
        {
            if( StringUtil::match( fileList[i], pattern ) )
            {
                AAsset *asset = AAssetManager_open( mAssetMgr, ( mPathPreFix + fileList[i] ).c_str(),
                                                    AASSET_MODE_UNKNOWN );
                if( asset )
                {
                    FileInfo info;
                    info.archive = this;
                    info.filename = fileList[i];
                    info.path = mName;
                    info.basename = fileList[i];
                    info.compressedSize = static_cast<size_t>( AAsset_getLength( asset ) );
                    info.uncompressedSize = info.compressedSize;
                    files->push_back( info );
                    AAsset_close( asset );
                }
            }
        }
        return files;
    }

    bool APKFileSystemArchive::exists( const String &filename )
    {
        AAsset *asset =
            AAssetManager_open( mAssetMgr, ( mPathPreFix + filename ).c_str(), AASSET_MODE_UNKNOWN );
        if( asset )
        {
            AAsset_close( asset );
            return true;
        }
        return false;
    }

    time_t APKFileSystemArchive::getModifiedTime( const Ogre::String &filename ) { return 0; }

    //////////////////////////////////////////////////////////////////////////////

    const String &APKFileSystemArchiveFactory::getType() const
    {
        static String type = "APKFileSystem";
        return type;
    }

    void APKFileSystemArchiveFactory::convertPath( String &inOutPath ) const
    {
        if( inOutPath.size() > 0 && inOutPath[0] == '/' )
            inOutPath.erase( inOutPath.begin() );
    }

}  // namespace Ogre
