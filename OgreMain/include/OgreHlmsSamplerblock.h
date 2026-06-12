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
#ifndef _OgreHlmsSamplerblock_H_
#define _OgreHlmsSamplerblock_H_

#include "OgrePrerequisites.h"

#include "OgreColourValue.h"
#include "OgreCommon.h"
#include "OgreHlmsDatablock.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    enum TextureAddressingMode
    {
        /// Texture wraps at values over 1.0.
        TAM_WRAP,
        /// Texture mirrors (flips) at joins over 1.0.
        TAM_MIRROR,
        /// Texture clamps at 1.0.
        TAM_CLAMP,
        /// Texture coordinates outside the range [0.0, 1.0] are set to the border colour.
        TAM_BORDER,
        /// Unknown
        TAM_UNKNOWN = 99
    };

    /** A sampler block contains settings that go hand in hand with a texture, and thus
        are common to many textures.
        This is very analogous to D3D11_SAMPLER_DESC. @see HlmsDatablock
        Up to 32 different blocks are allowed!
    */
    struct _OgreExport HlmsSamplerblock : public BasicBlock
    {
        FilterOptions mMinFilter;
        FilterOptions mMagFilter;
        FilterOptions mMipFilter;

        TextureAddressingMode mU;
        TextureAddressingMode mV;
        TextureAddressingMode mW;

        /// This allows you to adjust the mipmap calculation up or down for a
        /// given texture unit. Negative values force a larger mipmap to be used,
        /// positive values force a smaller mipmap to be used. Units are in numbers
        /// of levels, so +1 forces the mipmaps to one smaller level.
        /// @note Only does something if render system has capability RSC_MIPMAP_LOD_BIAS.
        Real  mMipLodBias;
        float mMaxAnisotropy;
        /// Defaults to NUM_COMPARE_FUNCTIONS which means disabled.
        CompareFunction mCompareFunction;
        ColourValue     mBorderColour;
        float           mMinLod;
        float           mMaxLod;

        HlmsSamplerblock();

        bool operator!=( const HlmsSamplerblock &_r ) const
        {
            // Don't include the ID in the comparision
            return mAllowGlobalDefaults != _r.mAllowGlobalDefaults ||  //
                   mMinFilter != _r.mMinFilter ||                      //
                   mMagFilter != _r.mMagFilter ||                      //
                   mMipFilter != _r.mMipFilter ||                      //
                   mU != _r.mU ||                                      //
                   mV != _r.mV ||                                      //
                   mW != _r.mW ||                                      //
                   mMipLodBias != _r.mMipLodBias ||                    //
                   mMaxAnisotropy != _r.mMaxAnisotropy ||              //
                   mCompareFunction != _r.mCompareFunction ||          //
                   mBorderColour != _r.mBorderColour ||                //
                   mMinLod != _r.mMinLod ||                            //
                   mMaxLod != _r.mMaxLod;
        }

        /// Encodes all data into multiple uint64. Technically it can server as serialization.
        /// But it's used for generating unique name out of hashes in JSON.
        void encode( uint64 ( &outVals )[6] ) const
        {
            outVals[0] = static_cast<uint64_t>( mMinFilter ) << 56ul |          // 8 bits (56-63)
                         static_cast<uint64_t>( mMagFilter ) << 48ul |          // 8 bits (48-55)
                         static_cast<uint64_t>( mMipFilter ) << 40ul |          // 8 bits (40-47)
                         static_cast<uint64_t>( mU ) << 32ul |                  // 8 bits (32-39)
                         static_cast<uint64_t>( mV ) << 24ul |                  // 8 bits (24-31)
                         static_cast<uint64_t>( mW ) << 16ul |                  // 8 bits (16-23)
                         static_cast<uint64_t>( mAllowGlobalDefaults ) << 0ul;  // 1 bit (0)
#if OGRE_DOUBLE_PRECISION == 1
            outVals[1] = bit_cast<uint64_t>( mMipLodBias );  // 64 bits (double)
#else
            outVals[1] = bit_cast<uint32_t>( mMipLodBias );  // 32 bits (float)
#endif
            outVals[2] =
                ( static_cast<uint64_t>( bit_cast<uint32_t>( mMaxAnisotropy ) ) << 32ul ) |  // 32 bits
                static_cast<uint64_t>( mCompareFunction );                                   // 8 bits

            outVals[3] = ( uint64_t( bit_cast<uint32_t>( mBorderColour.r ) ) << 32ul ) |
                         ( uint64_t( bit_cast<uint32_t>( mBorderColour.g ) ) << 32ul );

            outVals[4] = ( uint64_t( bit_cast<uint32_t>( mBorderColour.b ) ) << 32ul ) |
                         ( uint64_t( bit_cast<uint32_t>( mBorderColour.a ) ) << 32ul );

            outVals[5] = static_cast<uint64_t>( bit_cast<float>( mMinLod ) ) << 32ul |  // min lod
                         bit_cast<uint32_t>( mMaxLod );                                 // max lod
        }

        /// Helper function to set filtering to the most common settings
        void setFiltering( TextureFilterOptions filterType );
        void setAddressingMode( TextureAddressingMode addressingMode );
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
