
#include "OgrePrerequisites.h"
#include "OgreStringVector.h"
#include "OgrePixelFormatGpu.h"

namespace Demo
{
    /** Functions mostly useful for Unit Testing
    */
    class TestUtils
    {
        Ogre::uint32    mNumGenTextures;
        Ogre::uint32    mNumUnlitDatablocks;
        Ogre::uint32    mNumPbsDatablocks;

        Ogre::TextureGpuManager *mTextureManager;
    public:
        TestUtils();

        /** Generates N random textures with names like 'UnitTestTex/0' where 0 is in range
            [mNumGenTextures; mNumGenTextures + numTextures)

            They're all filled with 0xFF
        @remarks
            The resolution will be randomly between
                width = [minWidth; maxWidth];
                height = [minWidth; maxWidth];
            The textures are not guaranteed to be squared.
        */
        void generateRandomBlankTextures( Ogre::uint32 numTextures,
                                          Ogre::uint16 minWidth,
                                          Ogre::uint16 maxWidth,
                                          Ogre::PixelFormatGpu pixelFormat=Ogre::PFG_RGBA8_UNORM_SRGB );

        /** Generates N random duplicates of the textures in sourceTex[] with names like
            'UnitTestTex/0' where 0 is in range [mNumGenTextures; mNumGenTextures + numTextures)

        @remarks
            We generate 'numTextures', *not* numTextures * sourceTex.size() textures
        @param numTextures
            Number of textures to generate
        @param sourceTex
            List of textures to clone. Must be non-empty.
            Use TestUtils::getSourceTexList if you don't know what to enter
        */
        void generateDuplicateTextures( Ogre::uint32 numTextures, const Ogre::StringVector &sourceTex );

        Ogre::StringVector getSourceTexList(void) const;

        /** Generates N random datablocks of Unlit type with names like:
            'UnitTestUnlit/0' where 0 is in range [mNumGenTextures; mNumGenTextures + numDatablocks)
        @param numDatablocks
            Number of unlit datablocks to create
        @param firstTextureIdx
            Value in range [0; mNumGenTextures)
        @param numTextures
            Number of textures to shuffle across the created datablocks.
            firstTextureIdx + numTextures must be < mNumGenTextures
        */
        void generateUnlitDatablocksWithTextures( Ogre::uint32 numDatablocks,
                                                  Ogre::uint32 firstTextureIdx,
                                                  Ogre::uint32 numTextures );

        /// See TestUtils::generateUnlitDatablocksWithTextures
        void generatePbsDatablocksWithTextures( Ogre::uint32 numDatablocks,
                                                Ogre::uint32 firstTextureIdx,
                                                Ogre::uint32 numTextures );
    };
}
