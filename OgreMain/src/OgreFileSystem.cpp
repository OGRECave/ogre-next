/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreFileSystem.h"

#include "OgreException.h"
#include "OgreString.h"
#include "OgreStringVector.h"

#include <fstream>

#include <sys/stat.h>

// clang-format off
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX || \
    OGRE_PLATFORM == OGRE_PLATFORM_APPLE || \
    OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || \
    OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || \
    OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN || \
    OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
#   include "OgreSearchOps.h"
#   include <sys/param.h>
#endif
// clang-format on

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#    define WIN32_LEAN_AND_MEAN
#    if !defined( NOMINMAX ) && defined( _MSC_VER )
#        define NOMINMAX  // required to stop windows.h messing up std::min
#    endif
#    include <direct.h>
#    include <io.h>
#    include <windows.h>
//#  define _OGRE_FILESYSTEM_ARCHIVE_UNICODE // base path and resources subpathes expected to be in UTF-8
// and wchar_t file IO routines are used
#endif

namespace Ogre
{
    bool FileSystemArchive::msIgnoreHidden = true;

    //-----------------------------------------------------------------------
    FileSystemArchive::FileSystemArchive( const String &name, const String &archType, bool readOnly ) :
        Archive( name, archType )
    {
        // Even failed attempt to write to read only location violates Apple AppStore validation process.
        // And successful writing to some probe file does not prove that whole location with subfolders
        // is writable. Therefore we accept read only flag from outside and do not try to be too smart.
        mReadOnly = readOnly;
    }
    //-----------------------------------------------------------------------
    bool FileSystemArchive::isCaseSensitive() const
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return false;
#else
        return true;
#endif
    }
    //-----------------------------------------------------------------------
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
    static bool is_reserved_dir( const wchar_t *fn )
#else
    static bool is_reserved_dir( const char *fn )
#endif
    {
        return ( fn[0] == '.' && ( fn[1] == 0 || ( fn[1] == '.' && fn[2] == 0 ) ) );
    }
    //-----------------------------------------------------------------------
    static bool is_absolute_path( const char *path )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        if( isalpha( uchar( path[0] ) ) && path[1] == ':' )
            return true;
#endif
        return path[0] == '/' || path[0] == '\\';
    }
    //-----------------------------------------------------------------------
    static String concatenate_path( const String &base, const String &name )
    {
        if( base.empty() || is_absolute_path( name.c_str() ) )
            return name;
        else
            return base + '/' + name;
    }
    //-----------------------------------------------------------------------
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    extern std::wstring to_wpath( const String &text, unsigned codepage = CP_UTF8 );

    std::wstring to_wpath( const String &text, unsigned codepage )
    {
        const int utf16Length =
            ::MultiByteToWideChar( codepage, 0, text.c_str(), (int)text.size(), NULL, 0 );
        if( utf16Length > 0 )
        {
            std::wstring wt;
            wt.resize( utf16Length );
            if( 0 != ::MultiByteToWideChar( codepage, 0, text.c_str(), (int)text.size(), &wt[0],
                                            (int)wt.size() ) )
            {
                return wt;
            }
        }
        return L"";
    }
    String from_wpath( const std::wstring &text, unsigned codepage = CP_UTF8 )
    {
        const int length =
            ::WideCharToMultiByte( codepage, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL );
        if( length > 0 )
        {
            String str;
            str.resize( length );
            if( 0 != ::WideCharToMultiByte( codepage, 0, text.c_str(), (int)text.size(), &str[0],
                                            (int)str.size(), NULL, NULL ) )
            {
                return str;
            }
        }
        return "";
    }
    FileSystemPath fileSystemPathFromString( const String &path )
    {
#    ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        return to_wpath( path, CP_UTF8 );
#    elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return to_wpath( path, AreFileApisANSI() ? CP_ACP : CP_OEMCP );
#    elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return to_wpath( path, CP_ACP );
#    endif
    }
    String fileSystemPathToString( const FileSystemPath &path )
    {
#    ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        return from_wpath( path, CP_UTF8 );
#    elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        return from_wpath( path, AreFileApisANSI() ? CP_ACP : CP_OEMCP );
#    elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return from_wpath( path, CP_ACP );
#    endif
    }
#endif
    //-----------------------------------------------------------------------
    void FileSystemArchive::findFiles( const String &pattern, bool recursive, bool dirs,
                                       StringVector *simpleList, FileInfoList *detailList )
    {
        intptr_t lHandle, res;
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        struct _wfinddata_t tagData;
#else
        struct _finddata_t tagData;
#endif

        // pattern can contain a directory name, separate it from mask
        size_t pos1 = pattern.rfind( '/' );
        size_t pos2 = pattern.rfind( '\\' );
        if( pos1 == pattern.npos || ( ( pos2 != pattern.npos ) && ( pos1 < pos2 ) ) )
            pos1 = pos2;
        String directory;
        if( pos1 != pattern.npos )
            directory = pattern.substr( 0, pos1 + 1 );

        String full_pattern = concatenate_path( mName, pattern );

#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        lHandle = _wfindfirst( to_wpath( full_pattern ).c_str(), &tagData );
#else
        lHandle = _findfirst( full_pattern.c_str(), &tagData );
#endif
        res = 0;
        while( lHandle != -1 && res != -1 )
        {
            if( ( dirs == ( ( tagData.attrib & _A_SUBDIR ) != 0 ) ) &&
                ( !msIgnoreHidden || ( tagData.attrib & _A_HIDDEN ) == 0 ) &&
                ( !dirs || !is_reserved_dir( tagData.name ) ) )
            {
                if( simpleList )
                {
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
                    simpleList->push_back( directory + from_wpath( tagData.name ) );
#else
                    simpleList->push_back( directory + tagData.name );
#endif
                }
                else if( detailList )
                {
                    FileInfo fi;
                    fi.archive = this;
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
                    fi.filename = directory + from_wpath( tagData.name );
                    fi.basename = from_wpath( tagData.name );
#else
                    fi.filename = directory + tagData.name;
                    fi.basename = tagData.name;
#endif
                    fi.path = directory;
                    fi.compressedSize = tagData.size;
                    fi.uncompressedSize = tagData.size;
                    detailList->push_back( fi );
                }
            }
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
            res = _wfindnext( lHandle, &tagData );
#else
            res = _findnext( lHandle, &tagData );
#endif
        }
        // Close if we found any files
        if( lHandle != -1 )
            _findclose( lHandle );

        // Now find directories
        if( recursive )
        {
            String base_dir = mName;
            if( !directory.empty() )
            {
                base_dir = concatenate_path( mName, directory );
                // Remove the last '/'
                base_dir.erase( base_dir.length() - 1 );
            }
            base_dir.append( "/*" );

            // Remove directory name from pattern
            String mask( "/" );
            if( pos1 != pattern.npos )
                mask.append( pattern.substr( pos1 + 1 ) );
            else
                mask.append( pattern );

#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
            lHandle = _wfindfirst( to_wpath( base_dir ).c_str(), &tagData );
#else
            lHandle = _findfirst( base_dir.c_str(), &tagData );
#endif
            res = 0;
            while( lHandle != -1 && res != -1 )
            {
                if( ( tagData.attrib & _A_SUBDIR ) &&
                    ( !msIgnoreHidden || ( tagData.attrib & _A_HIDDEN ) == 0 ) &&
                    !is_reserved_dir( tagData.name ) )
                {
                    // recurse
                    base_dir = directory;
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
                    base_dir.append( from_wpath( tagData.name ) ).append( mask );
#else
                    base_dir.append( tagData.name ).append( mask );
#endif
                    findFiles( base_dir, recursive, dirs, simpleList, detailList );
                }
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
                res = _wfindnext( lHandle, &tagData );
#else
                res = _findnext( lHandle, &tagData );
#endif
            }
            // Close if we found any files
            if( lHandle != -1 )
                _findclose( lHandle );
        }
    }
    //-----------------------------------------------------------------------
    FileSystemArchive::~FileSystemArchive() { unload(); }
    //-----------------------------------------------------------------------
    void FileSystemArchive::load()
    {
        // nothing to do here
    }
    //-----------------------------------------------------------------------
    void FileSystemArchive::unload()
    {
        // nothing to see here, move along
    }
    //-----------------------------------------------------------------------
    DataStreamPtr FileSystemArchive::open( const String &filename, bool readOnly )
    {
        String full_path = concatenate_path( mName, filename );

        // Use filesystem to determine size
        // (quicker than streaming to the end and back)
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        struct _stat64i32 tagStat;
        int ret = _wstat( to_wpath( full_path ).c_str(), &tagStat );
#else
        struct stat tagStat;
        int ret = stat( full_path.c_str(), &tagStat );
#endif
        assert( ret == 0 && "Problem getting file size" );
        (void)ret;  // Silence warning

        // Always open in binary mode
        // Also, always include reading
        std::ios::openmode mode = std::ios::in | std::ios::binary;
        std::istream *baseStream = 0;
        std::ifstream *roStream = 0;
        std::fstream *rwStream = 0;

        if( !readOnly && isReadOnly() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot open a file in read-write mode in a read-only archive",
                         "FileSystemArchive::open" );
        }

        if( !readOnly )
        {
            mode |= std::ios::out;
            rwStream = OGRE_NEW_T( std::fstream, MEMCATEGORY_GENERAL )();
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
            rwStream->open( to_wpath( full_path ).c_str(), mode );
#else
            rwStream->open( full_path.c_str(), mode );
#endif
            baseStream = rwStream;
        }
        else
        {
            roStream = OGRE_NEW_T( std::ifstream, MEMCATEGORY_GENERAL )();
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
            roStream->open( to_wpath( full_path ).c_str(), mode );
#else
            roStream->open( full_path.c_str(), mode );
#endif
            baseStream = roStream;
        }

        // Should check ensure open succeeded, in case fail for some reason.
        if( baseStream->fail() )
        {
            OGRE_DELETE_T( roStream, basic_ifstream, MEMCATEGORY_GENERAL );
            OGRE_DELETE_T( rwStream, basic_fstream, MEMCATEGORY_GENERAL );
            OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND, "Cannot open file: " + filename,
                         "FileSystemArchive::open" );
        }

        /// Construct return stream, tell it to delete on destroy
        FileStreamDataStream *stream = 0;
        if( rwStream )
        {
            // use the writeable stream
            stream = OGRE_NEW FileStreamDataStream( filename, rwStream, (size_t)tagStat.st_size, true );
        }
        else
        {
            // read-only stream
            stream = OGRE_NEW FileStreamDataStream( filename, roStream, (size_t)tagStat.st_size, true );
        }
        return DataStreamPtr( stream );
    }
    //---------------------------------------------------------------------
    DataStreamPtr FileSystemArchive::create( const String &filename )
    {
        if( isReadOnly() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot create a file in a read-only archive",
                         "FileSystemArchive::remove" );
        }

        String full_path = concatenate_path( mName, filename );

        // Always open in binary mode
        // Also, always include reading
        std::ios::openmode mode = std::ios::out | std::ios::binary;
        std::fstream *rwStream = OGRE_NEW_T( std::fstream, MEMCATEGORY_GENERAL )();
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        rwStream->open( to_wpath( full_path ).c_str(), mode );
#else
        rwStream->open( full_path.c_str(), mode );
#endif

        // Should check ensure open succeeded, in case fail for some reason.
        if( rwStream->fail() )
        {
            OGRE_DELETE_T( rwStream, basic_fstream, MEMCATEGORY_GENERAL );
            OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND, "Cannot open file: " + filename,
                         "FileSystemArchive::create" );
        }

        /// Construct return stream, tell it to delete on destroy
        FileStreamDataStream *stream = OGRE_NEW FileStreamDataStream( filename, rwStream, 0, true );

        return DataStreamPtr( stream );
    }
    //---------------------------------------------------------------------
    void FileSystemArchive::remove( const String &filename )
    {
        if( isReadOnly() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Cannot remove a file from a read-only archive",
                         "FileSystemArchive::remove" );
        }
        String full_path = concatenate_path( mName, filename );
#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        ::_wremove( to_wpath( full_path ).c_str() );
#else
        ::remove( full_path.c_str() );
#endif
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::list( bool recursive, bool dirs )
    {
        // directory change requires locking due to saved returns
        // Note that we have to tell the SharedPtr to use OGRE_DELETE_T not OGRE_DELETE by passing
        // category
        StringVectorPtr ret( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        findFiles( "*", recursive, dirs, ret.get(), 0 );

        return ret;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::listFileInfo( bool recursive, bool dirs )
    {
        // Note that we have to tell the SharedPtr to use OGRE_DELETE_T not OGRE_DELETE by passing
        // category
        FileInfoListPtr ret( OGRE_NEW_T( FileInfoList, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        findFiles( "*", recursive, dirs, 0, ret.get() );

        return ret;
    }
    //-----------------------------------------------------------------------
    StringVectorPtr FileSystemArchive::find( const String &pattern, bool recursive, bool dirs )
    {
        // Note that we have to tell the SharedPtr to use OGRE_DELETE_T not OGRE_DELETE by passing
        // category
        StringVectorPtr ret( OGRE_NEW_T( StringVector, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        findFiles( pattern, recursive, dirs, ret.get(), 0 );

        return ret;
    }
    //-----------------------------------------------------------------------
    FileInfoListPtr FileSystemArchive::findFileInfo( const String &pattern, bool recursive, bool dirs )
    {
        // Note that we have to tell the SharedPtr to use OGRE_DELETE_T not OGRE_DELETE by passing
        // category
        FileInfoListPtr ret( OGRE_NEW_T( FileInfoList, MEMCATEGORY_GENERAL )(), SPFM_DELETE_T );

        findFiles( pattern, recursive, dirs, 0, ret.get() );

        return ret;
    }
    //-----------------------------------------------------------------------
    bool FileSystemArchive::exists( const String &filename )
    {
        String full_path = concatenate_path( mName, filename );

#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        struct _stat64i32 tagStat;
        bool ret = ( _wstat( to_wpath( full_path ).c_str(), &tagStat ) == 0 );
#else
        struct stat tagStat;
        bool ret = ( stat( full_path.c_str(), &tagStat ) == 0 );
#endif

        // stat will return true if the filename is absolute, but we need to check
        // the file is actually in this archive
        if( ret && is_absolute_path( filename.c_str() ) )
        {
            // only valid if full path starts with our base
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
            // case insensitive on windows
            String lowerCaseName = mName;
            StringUtil::toLowerCase( lowerCaseName );
            ret = Ogre::StringUtil::startsWith( full_path, lowerCaseName, true );
#else
            // case sensitive
            ret = Ogre::StringUtil::startsWith( full_path, mName, false );
#endif
        }

        return ret;
    }
    //---------------------------------------------------------------------
    time_t FileSystemArchive::getModifiedTime( const String &filename )
    {
        String full_path = concatenate_path( mName, filename );

#ifdef _OGRE_FILESYSTEM_ARCHIVE_UNICODE
        struct _stat64i32 tagStat;
        bool ret = ( _wstat( to_wpath( full_path ).c_str(), &tagStat ) == 0 );
#else
        struct stat tagStat;
        bool ret = ( stat( full_path.c_str(), &tagStat ) == 0 );
#endif

        if( ret )
        {
            return tagStat.st_mtime;
        }
        else
        {
            return 0;
        }
    }
    //-----------------------------------------------------------------------
    const String &FileSystemArchiveFactory::getType() const
    {
        static String name = "FileSystem";
        return name;
    }

}  // namespace Ogre
