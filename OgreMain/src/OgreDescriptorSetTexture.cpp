/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreDescriptorSetTexture.h"

#include "OgreException.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureGpu.h"
#include "Vao/OgreTexBufferPacked.h"

namespace Ogre
{
    void DescriptorSetTexture::checkValidity() const
    {
#if OGRE_DEBUG_MODE
        size_t totalTexturesUsed = 0u;

        for( size_t i = 0; i < NumShaderTypes; ++i )
            totalTexturesUsed += mShaderTypeTexCount[i];

        assert( totalTexturesUsed > 0 &&
                "This DescriptorSetTexture doesn't use any texture! Perhaps incorrectly setup?" );
        assert( totalTexturesUsed == mTextures.size() &&
                "This DescriptorSetTexture doesn't use as many textures as it "
                "claims to have, or uses more than it has provided" );
#endif
    }
    //-----------------------------------------------------------------------------------
    bool DescriptorSetTexture2::TextureSlot::formatNeedsReinterpret() const
    {
        return pixelFormat != PFG_UNKNOWN && pixelFormat != texture->getPixelFormat();
    }
    //-----------------------------------------------------------------------------------
    bool DescriptorSetTexture2::TextureSlot::needsDifferentView() const
    {
        return formatNeedsReinterpret() || mipmapLevel != 0 || textureArrayIndex != 0 ||
               numMipmaps != 0 ||
               ( cubemapsAs2DArrays && ( texture->getTextureType() == TextureTypes::TypeCube ||
                                         texture->getTextureType() == TextureTypes::TypeCubeArray ) );
    }
    //-----------------------------------------------------------------------------------
    void DescriptorSetTexture2::checkValidity() const
    {
        assert( !mTextures.empty() &&
                "This DescriptorSetTexture2 doesn't use any texture/buffer! "
                "Perhaps incorrectly setup?" );

#if OGRE_DEBUG_MODE
        size_t totalTexturesUsed = 0u;

        for( size_t i = 0; i < NumShaderTypes; ++i )
            totalTexturesUsed += mShaderTypeTexCount[i];

        assert( totalTexturesUsed > 0 &&
                "This DescriptorSetTexture doesn't use any texture! Perhaps incorrectly setup?" );
        assert( totalTexturesUsed == mTextures.size() &&
                "This DescriptorSetTexture doesn't use as many textures as it "
                "claims to have, or uses more than it has provided" );
#endif

        FastArray<Slot>::const_iterator itor = mTextures.begin();
        FastArray<Slot>::const_iterator endt = mTextures.end();

        while( itor != endt )
        {
            const Slot &slot = *itor;
            if( slot.isTexture() )
            {
                const TextureSlot &texSlot = slot.getTexture();
                if( texSlot.formatNeedsReinterpret() && !texSlot.texture->isReinterpretable() )
                {
                    // This warning here is for
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "To reinterpret the texture as a different format, "
                                 "you must set the reinterpretable flag "
                                 "(TextureFlags::Reinterpretable) for "
                                 "texture: '" +
                                     texSlot.texture->getNameStr() + "' " +
                                     texSlot.texture->getSettingsDesc(),
                                 "DescriptorSetTexture2::checkValidity" );
                }
            }
            else if( slot.isBuffer() )
            {
                const BufferSlot &bufferSlot = slot.getBuffer();
                if( bufferSlot.buffer->getBufferType() >= BT_DYNAMIC_DEFAULT )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Dynamic buffers cannot be baked into a static DescriptorSet",
                                 "DescriptorSetTexture2::checkValidity" );
                }
            }

            ++itor;
        }
    }
}  // namespace Ogre
