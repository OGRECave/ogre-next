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
#ifndef __Zip_H__
#define __Zip_H__

#include "OgrePrerequisites.h"

#include "OgreArchive.h"
#include "OgreArchiveFactory.h"

#if !defined( OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API ) || \
    defined( OGRE_SHADER_THREADING_USE_TLS )
#    include "Threading/OgreLightweightMutex.h"
#else
#    include "Threading/OgreThreadHeaders.h"
#endif

#include "OgreHeaderPrefix.h"

// Forward declaration for zziplib to avoid header file dependency.
typedef struct zzip_dir       ZZIP_DIR;
typedef struct zzip_file      ZZIP_FILE;
typedef union _zzip_plugin_io zzip_plugin_io_handlers;

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Specialisation of the Archive class to allow reading of files from a zip
        format source archive.
    @remarks
        This archive format supports all archives compressed in the standard
        zip format, including iD pk3 files.
    */
    class _OgreExport ZipArchive : public Archive
    {
    protected:
        /// Handle to root zip file
        ZZIP_DIR *mZzipDir;
        /// Handle any errors from zzip
        void checkZzipError( int zzipError, const String &operation ) const;
        /// File list (since zziplib seems to only allow scanning of dir tree once)
        FileInfoList mFileList;
        /// A pointer to file io alternative implementation
        zzip_plugin_io_handlers *mPluginIo;

#if !defined( OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API ) || \
    defined( OGRE_SHADER_THREADING_USE_TLS )
        LightweightMutex mMutex;
#else
        OGRE_AUTO_MUTEX;
#endif

        FileInfoListPtr findFileInfo( const String &pattern, bool recursive, bool dirs,
                                      bool bAlreadyLocked );

    public:
        ZipArchive( const String &name, const String &archType,
                    zzip_plugin_io_handlers *pluginIo = NULL );
        ~ZipArchive() override;
        /// @copydoc Archive::isCaseSensitive
        bool isCaseSensitive() const override { return false; }

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
    };

    /** Specialisation of ArchiveFactory for Zip files. */
    class _OgreExport ZipArchiveFactory : public ArchiveFactory
    {
    public:
        ~ZipArchiveFactory() override {}
        /// @copydoc FactoryObj::getType
        const String &getType() const override;
        /// @copydoc FactoryObj::createInstance
        Archive *createInstance( const String &name, bool readOnly ) override
        {
            if( !readOnly )
                return NULL;

            return OGRE_NEW ZipArchive( name, "Zip" );
        }
        /// @copydoc FactoryObj::destroyInstance
        void destroyInstance( Archive *ptr ) override { OGRE_DELETE ptr; }
    };

    /** Specialisation of ZipArchiveFactory for embedded Zip files. */
    class _OgreExport EmbeddedZipArchiveFactory : public ZipArchiveFactory
    {
    protected:
        /// A static pointer to file io alternative implementation for the embedded files
        static zzip_plugin_io_handlers *mPluginIo;

    public:
        EmbeddedZipArchiveFactory();
        ~EmbeddedZipArchiveFactory() override;
        /// @copydoc FactoryObj::getType
        const String &getType() const override;
        /// @copydoc FactoryObj::createInstance
        Archive *createInstance( const String &name, bool readOnly ) override
        {
            ZipArchive *resZipArchive = OGRE_NEW ZipArchive( name, "EmbeddedZip", mPluginIo );
            return resZipArchive;
        }

        /** a function type to decrypt embedded zip file
        @param pos pos in file
        @param buf current buffer to decrypt
        @param len - length of buffer
        @return success
        */
        typedef bool ( *DecryptEmbeddedZipFileFunc )( size_t pos, void *buf, size_t len );

        /// Add an embedded file to the embedded file list
        static void addEmbbeddedFile( const String &name, const uint8 *fileData, size_t fileSize,
                                      DecryptEmbeddedZipFileFunc decryptFunc );

        /// Remove an embedded file to the embedded file list
        static void removeEmbbeddedFile( const String &name );
    };

    /** Specialisation of DataStream to handle streaming data from zip archives. */
    class _OgreExport ZipDataStream final : public DataStream
    {
    protected:
        ZZIP_FILE *mZzipFile;
        /// We need caching because sometimes serializers step back in data stream and zziplib behaves
        /// slow
        StaticCache<2 * OGRE_STREAM_TEMP_SIZE> mCache;

    public:
        /// Unnamed constructor
        ZipDataStream( ZZIP_FILE *zzipFile, size_t uncompressedSize );
        /// Constructor for creating named streams
        ZipDataStream( const String &name, ZZIP_FILE *zzipFile, size_t uncompressedSize );
        ~ZipDataStream() override;
        /// @copydoc DataStream::read
        size_t read( void *buf, size_t count ) override;
        /// @copydoc DataStream::write
        size_t write( const void *buf, size_t count ) override;
        /// @copydoc DataStream::skip
        void skip( long count ) override;
        /// @copydoc DataStream::seek
        void seek( size_t pos ) override;
        /// @copydoc DataStream::seek
        size_t tell() const override;
        /// @copydoc DataStream::eof
        bool eof() const override;
        /// @copydoc DataStream::close
        void close() override;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
