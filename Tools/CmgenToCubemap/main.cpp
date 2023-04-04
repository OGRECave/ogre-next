
#include "OgreFileSystem.h"
#include "OgreImage2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"

#include "OgreLwString.h"

#if OGRE_NO_FREEIMAGE == 0
#    include "OgreFreeImageCodec2.h"
#endif
#if OGRE_NO_DDS_CODEC == 0
#    include "OgreDDSCodec2.h"
#endif
#if OGRE_NO_STBI_CODEC == 0
#    include "OgreSTBICodec.h"
#endif
#include "OgreOITDCodec.h"
#if OGRE_NO_PVRTC_CODEC == 0
#    include "OgrePVRTCCodec.h"
#endif
#if OGRE_NO_ETC_CODEC == 0
#    include "OgreETCCodec.h"
#endif
#if OGRE_NO_ASTC_CODEC == 0
#    include "OgreASTCCodec.h"
#endif

#include "OgreLogManager.h"

#include <fstream>

const char *faceNames[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };

Ogre::LogManager *g_logManager = 0;

static void initCodecs()
{
    using namespace Ogre;

    g_logManager = new Ogre::LogManager();

#if OGRE_NO_DDS_CODEC == 0
    DDSCodec2::startup();
#endif
#if OGRE_NO_FREEIMAGE == 0
    FreeImageCodec2::startup();
#endif
#if OGRE_NO_PVRTC_CODEC == 0
    PVRTCCodec::startup();
#endif
#if OGRE_NO_ETC_CODEC == 0
    ETCCodec::startup();
#endif
#if OGRE_NO_STBI_CODEC == 0
    STBIImageCodec::startup();
#endif
#if OGRE_NO_ASTC_CODEC == 0
    ASTCCodec::startup();
#endif
    OITDCodec::startup();
}

static bool loadIntoImage( const std::string &folderPath, const char *filename, Ogre::Image2 &outImage )
{
    bool bOpenSuccess = false;

    std::string fullPath = folderPath + filename;

    printf( "Opening %s...", fullPath.c_str() );

    using namespace Ogre;
    std::ifstream ifs( fullPath.c_str(), std::ios::binary | std::ios::in );
    if( ifs.is_open() )
    {
        const String::size_type extPos = fullPath.find_last_of( '.' );
        if( extPos != String::npos )
        {
            bOpenSuccess = true;
            const String texExt = fullPath.substr( extPos + 1 );
            DataStreamPtr dataStream( OGRE_NEW FileStreamDataStream( filename, &ifs, false ) );
            outImage.load( dataStream, texExt );
            printf( " OK" );
        }
        else
        {
            fprintf( stderr, "\nCould not open %s\n", fullPath.c_str() );
        }

        ifs.close();
    }

    printf( "\n" );
    fflush( stdout );

    return bOpenSuccess;
}

int main( int argc, const char *argv[] )
{
    if( argc != 5u )
    {
        printf(
            "Tool to stitch together cubemap faces (individual files) generated\n"
            "by cmgen from Filament. These files have the filename pattern:\n"
            "    m0_nx.png m0_ny.png m0_nz.png m0_px.png m0_py.png m0_pz.png m1_nx.png m1_ny.png\n"
            "This tool generates a single cubemap file\n"
            "(must be supported by format e.g. KTX, DDS, OITD).\n"
            "\n"
            "\n"
            "USAGE:\n"
            "   OgreCmgenToCubemap /path/to/folder/from/cmgen png 6 output.oitd\n"
            "\n"
            "    'png' is the format extension of the generated files from cmgen.\n"
            "    '6' is the max mipmap.\n" );
        return -1;
    }

    std::string folderPath = argv[1];
    const uint8_t maxNumMipmaps = (uint8_t)atoi( argv[3] ) + 1u;
    const char *extension = argv[2];
    const char *outputFilename = argv[4];

    if( !folderPath.empty() && folderPath[folderPath.size() - 1u] != '/' )
        folderPath.push_back( '/' );

    initCodecs();

    using namespace Ogre;
    char tmpBuffer[256];

    LwString filename( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

    Image2 faceImage;
    filename.a( "m", 0, faceNames[0], ".", extension );

    const bool bOpenSuccess = loadIntoImage( folderPath, filename.c_str(), faceImage );

    if( !bOpenSuccess )
    {
        printf( "Could not determine resolution nor pixel format. Aborting\n" );
        return -1;
    }

    const uint32_t width = faceImage.getWidth();
    const uint32_t height = faceImage.getHeight();
    const PixelFormatGpu pixelFormat = faceImage.getPixelFormat();

    uint8_t numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( width, height );
    numMipmaps = std::min( numMipmaps, maxNumMipmaps );
    const size_t requiredBytes =
        PixelFormatGpuUtils::calculateSizeBytes( width, height, 1u, 6u, pixelFormat, numMipmaps, 4u );

    void *data = OGRE_MALLOC_SIMD( requiredBytes, MEMCATEGORY_GENERAL );
    Image2 finalCubemap;
    finalCubemap.loadDynamicImage( data, width, height, 6u, TextureTypes::TypeCube, pixelFormat, true,
                                   numMipmaps );

    for( uint8_t mip = 0u; mip < numMipmaps; ++mip )
    {
        for( uint32_t face = 0u; face < 6u; ++face )
        {
            if( mip != 0u || face != 0u )
            {
                // Mip 0 Face 0 was already loaded
                filename.clear();
                filename.a( "m", mip, faceNames[face], ".", extension );
                loadIntoImage( folderPath, filename.c_str(), faceImage );
            }

            TextureBox srcBox = faceImage.getData( 0u );
            TextureBox dstBox = finalCubemap.getData( mip );

            dstBox.sliceStart = face;
            dstBox.numSlices = 1u;
            dstBox.copyFrom( srcBox );
            dstBox.numSlices = 6u;
        }
    }

    finalCubemap.save( outputFilename, 0u, numMipmaps );

    delete g_logManager;
}
