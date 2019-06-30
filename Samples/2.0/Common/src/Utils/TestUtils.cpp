
#include "Utils/TestUtils.h"

#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"

#include "OgreRoot.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"

namespace Demo
{
    TestUtils::TestUtils() :
        mNumGenTextures( 0 ),
        mNumUnlitDatablocks( 0 ),
        mNumPbsDatablocks( 0 ),
        mTextureManager( Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager() )
    {
    }
    //-------------------------------------------------------------------------
    void TestUtils::generateRandomBlankTextures( Ogre::uint32 numTextures,
                                                 Ogre::uint16 minWidth, Ogre::uint16 maxWidth,
                                                 Ogre::PixelFormatGpu pixelFormat )
    {
        using namespace Ogre;
        for( uint32 i=0; i<numTextures; ++i )
        {
            const String texName = "UnitTestTex/" + StringConverter::toString( mNumGenTextures + i );
            TextureGpu *texture =
                    mTextureManager->createTexture( texName, texName, GpuPageOutStrategy::Discard,
                                                    TextureFlags::AutomaticBatching |
                                                    TextureFlags::PrefersLoadingFromFileAsSRGB,
                                                    TextureTypes::Type2D );

            Image2 *imagePtr = new Image2();

            const uint32 texWidth  = static_cast<uint32>( Math::RangeRandom( minWidth, maxWidth ) );
            const uint32 texHeight = static_cast<uint32>( Math::RangeRandom( minWidth, maxWidth ) );

            const size_t sizeBytes = PixelFormatGpuUtils::calculateSizeBytes(
                                         texWidth, texHeight, 1u, 1u, pixelFormat, 1u, 4u );
            uint8 *data = reinterpret_cast<uint8*>(
                                    OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_GENERAL ) );
            memset( data, 0xFF, sizeBytes );
            imagePtr->loadDynamicImage( data, texWidth, texHeight, 1u,
                                       TextureTypes::Type2D, pixelFormat, true, 1u );

            //Ogre will call "delete imagePtr" when done, because we're passing
            //true to autoDeleteImage argument in scheduleTransitionTo
            texture->scheduleTransitionTo( GpuResidency::Resident, imagePtr, true );
        }

        mNumGenTextures += numTextures;
    }
    //-------------------------------------------------------------------------
    void TestUtils::generateDuplicateTextures( Ogre::uint32 numTextures,
                                               const Ogre::StringVector &sourceTex )
    {
        using namespace Ogre;

        OGRE_ASSERT_LOW( !sourceTex.empty() );

        size_t sourceTexIdx = 0u;

        for( uint32 i=0; i<numTextures; ++i )
        {
            const String texName = "UnitTestTex/" + StringConverter::toString( mNumGenTextures + i );
            mTextureManager->createOrRetrieveTexture( texName, sourceTex[sourceTexIdx],
                                                      GpuPageOutStrategy::Discard,
                                                      TextureFlags::AutomaticBatching |
                                                      TextureFlags::PrefersLoadingFromFileAsSRGB,
                                                      TextureTypes::Type2D );
            sourceTexIdx = (sourceTexIdx + 1u) % sourceTex.size();
        }

        mNumGenTextures += numTextures;
    }
    //-------------------------------------------------------------------------
    Ogre::StringVector TestUtils::getSourceTexList(void) const
    {
        Ogre::StringVector retVal;
        retVal.push_back( "1d_debug.png" );
        retVal.push_back( "1d_SPIRAL.png" );
        retVal.push_back( "BeachStones.jpg" );
        retVal.push_back( "blue_jaiqua.jpg" );
        retVal.push_back( "BumpyMetal.jpg" );
        retVal.push_back( "Dirt.jpg" );
        retVal.push_back( "MtlPlat2.jpg" );
        retVal.push_back( "MRAMOR6X6.jpg" );
        retVal.push_back( "Rocks_Diffuse.tga" );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void TestUtils::generateUnlitDatablocksWithTextures( Ogre::uint32 numDatablocks,
                                                         Ogre::uint32 firstTextureIdx,
                                                         Ogre::uint32 numTextures )
    {
        using namespace Ogre;

        Ogre::Root *root = Root::getSingletonPtr();
        Ogre::Hlms *hlmsUnlit = root->getHlmsManager()->getHlms( Ogre::HLMS_UNLIT );

        uint32 textureIdx = 0;
        for( uint32 i=0; i<numDatablocks; ++i )
        {
            const String materialName = "UnitTestUnlit" +
                                        StringConverter::toString( mNumUnlitDatablocks + i );

            HlmsDatablock *datablockBase =
                    hlmsUnlit->createDatablock( materialName, materialName, HlmsMacroblock(),
                                                HlmsBlendblock(), HlmsParamVec() );

            OGRE_ASSERT_HIGH( dynamic_cast<HlmsUnlitDatablock*>( datablockBase ) );
            HlmsUnlitDatablock *datablock = static_cast<HlmsUnlitDatablock*>( datablockBase );

            const String texName = "UnitTestTex/" + StringConverter::toString( firstTextureIdx +
                                                                               textureIdx );
            datablock->setTexture( 0, texName );

            textureIdx = (textureIdx + 1u) % numTextures;
        }

        mNumUnlitDatablocks += numDatablocks;
    }
    //-------------------------------------------------------------------------
    void TestUtils::generatePbsDatablocksWithTextures( Ogre::uint32 numDatablocks,
                                                       Ogre::uint32 firstTextureIdx,
                                                       Ogre::uint32 numTextures )
    {
        using namespace Ogre;

        Ogre::Root *root = Root::getSingletonPtr();
        Ogre::Hlms *hlmsPbs = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );

        uint32 textureIdx = 0;
        for( uint32 i=0; i<numDatablocks; ++i )
        {
            const String materialName = "UnitTestPbs" +
                                        StringConverter::toString( mNumPbsDatablocks + i );

            HlmsDatablock *datablockBase =
                    hlmsPbs->createDatablock( materialName, materialName, HlmsMacroblock(),
                                              HlmsBlendblock(), HlmsParamVec() );

            OGRE_ASSERT_HIGH( dynamic_cast<HlmsPbsDatablock*>( datablockBase ) );
            HlmsPbsDatablock *datablock = static_cast<HlmsPbsDatablock*>( datablockBase );

            const String texName = "UnitTestTex/" + StringConverter::toString( firstTextureIdx +
                                                                               textureIdx );
            datablock->setTexture( PBSM_DIFFUSE, texName );

            textureIdx = (textureIdx + 1u) % numTextures;
        }

        mNumPbsDatablocks += numDatablocks;
    }
}
