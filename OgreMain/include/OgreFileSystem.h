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
#ifndef __FileSystem_H__
#define __FileSystem_H__

#include "OgrePrerequisites.h"

#include "OgreArchive.h"
#include "OgreArchiveFactory.h"
#include "Threading/OgreThreadHeaders.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** Some machinery to properly handle Unicode filesystem pathes.
     *   It is designed to be later replaced by typedef std::filesystem::path FileSystemPath;
     *   FileSystemPath is always internally Unicode and is intended to be passed to the same
     *   std::[iof]stream constructors and methods that accepts std::filesystem::path in C++17.
     *   Note: Narrow strings are interpreted as UTF-8 on most platforms, but on Windows
     *   interpretation in both this machinery and FileSystemArchive is controlled by internal
     *   define _OGRE_FILESYSTEM_ARCHIVE_UNICODE, and could be CP_UTF8, CP_ACP or even CP_OEMCP.
     */
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
    typedef std::wstring FileSystemPath;
    FileSystemPath       fileSystemPathFromString( const String &path );
    String               fileSystemPathToString( const FileSystemPath &path );
#else
    typedef String               FileSystemPath;
    inline const FileSystemPath &fileSystemPathFromString( const String &path ) { return path; }
    inline const String         &fileSystemPathToString( const FileSystemPath &path ) { return path; }
#endif

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Specialisation of the Archive class to allow reading of files from
        filesystem folders / directories.
    */
    class _OgreExport FileSystemArchive final : public Archive
    {
    protected:
        /** Utility method to retrieve all files in a directory matching pattern.
        @param pattern
            File pattern.
        @param recursive
            Whether to cascade down directories.
        @param dirs
            Set to @c true if you want the directories to be listed instead of files.
        @param simpleList
            Populated if retrieving a simple list.
        @param detailList
            Populated if retrieving a detailed list.
        */
        void findFiles( const String &pattern, bool recursive, bool dirs, StringVector *simpleList,
                        FileInfoList *detailList );

        OGRE_AUTO_MUTEX;

    public:
        FileSystemArchive( const String &name, const String &archType, bool readOnly );
        ~FileSystemArchive() override;

        /// @copydoc Archive::isCaseSensitive
        bool isCaseSensitive() const override;

        /// @copydoc Archive::load
        void load() override;
        /// @copydoc Archive::unload
        void unload() override;

        /// @copydoc Archive::open
        DataStreamPtr open( const String &filename, bool readOnly = true ) override;

        /// @copydoc Archive::create
        DataStreamPtr create( const String &filename ) override;

        /// @copydoc Archive::remove
        void remove( const String &filename ) override;

        /// @copydoc Archive::list
        StringVectorPtr list( bool recursive = true, bool dirs = false ) override;

        /// @copydoc Archive::listFileInfo
        FileInfoListPtr listFileInfo( bool recursive = true, bool dirs = false ) override;

        /// @copydoc Archive::find
        StringVectorPtr find( const String &pattern, bool recursive = true, bool dirs = false ) override;

        /// @copydoc Archive::findFileInfo
        FileInfoListPtr findFileInfo( const String &pattern, bool recursive = true,
                                      bool dirs = false ) override;

        /// @copydoc Archive::exists
        bool exists( const String &filename ) override;

        /// @copydoc Archive::getModifiedTime
        time_t getModifiedTime( const String &filename ) override;

        /// Set whether filesystem enumeration will include hidden files or not.
        /// This should be called prior to declaring and/or initializing filesystem
        /// resource locations. The default is true (ignore hidden files).
        static void setIgnoreHidden( bool ignore ) { msIgnoreHidden = ignore; }

        /// Get whether hidden files are ignored during filesystem enumeration.
        static bool getIgnoreHidden() { return msIgnoreHidden; }

        static bool msIgnoreHidden;
    };

    /** Specialisation of ArchiveFactory for FileSystem files. */
    class _OgreExport FileSystemArchiveFactory final : public ArchiveFactory
    {
    public:
        ~FileSystemArchiveFactory() override {}
        /// @copydoc FactoryObj::getType
        const String &getType() const override;
        /// @copydoc FactoryObj::createInstance
        Archive *createInstance( const String &name, bool readOnly ) override
        {
            return OGRE_NEW FileSystemArchive( name, "FileSystem", readOnly );
        }
        /// @copydoc FactoryObj::destroyInstance
        void destroyInstance( Archive *ptr ) override { OGRE_DELETE ptr; }
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __FileSystem_H__
