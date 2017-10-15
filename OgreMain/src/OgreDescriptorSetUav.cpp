/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "OgreDescriptorSetUav.h"
#include "OgreTextureGpu.h"
#include "OgrePixelFormatGpuUtils.h"
#include "Vao/OgreUavBufferPacked.h"

#include "OgreException.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    bool DescriptorSetUav::TextureSlot::formatNeedsReinterpret(void) const
    {
        PixelFormatGpu format = pixelFormat;
        if( format == PFG_UNKNOWN )
            format = PixelFormatGpuUtils::getEquivalentLinear( texture->getPixelFormat() );

        return format != texture->getPixelFormat();
    }
    //-----------------------------------------------------------------------------------
    bool DescriptorSetUav::TextureSlot::needsDifferentView(void) const
    {
        return formatNeedsReinterpret() || mipmapLevel != 0 || textureArrayIndex != 0;
    }
    //-----------------------------------------------------------------------------------
    void DescriptorSetUav::checkValidity(void) const
    {
        assert( !mUavs.empty() &&
                "This DescriptorSetUav doesn't use any texture/buffer! Perhaps incorrectly setup?" );

        FastArray<Slot>::const_iterator itor = mUavs.begin();
        FastArray<Slot>::const_iterator end  = mUavs.end();

        while( itor != end )
        {
            const Slot &slot = *itor;
            if( slot.isTexture() )
            {
                const TextureSlot &texSlot = slot.getTexture();
                if( texSlot.formatNeedsReinterpret() && !texSlot.texture->isReinterpretable() )
                {
                    //This warning here is for
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "UAV sRGB textures must be bound as non-sRGB. "
                                 "You must set the reinterpretable flag "
                                 "(TextureFlags::Reinterpretable) for "
                                 "texture: '" + texSlot.texture->getNameStr() + "' " +
                                 texSlot.texture->getSettingsDesc(),
                                 "DescriptorSetUav::checkValidity" );
                }
            }
            else if( slot.isBuffer() )
            {
                const BufferSlot &bufferSlot = slot.getBuffer();
                if( bufferSlot.buffer->getBufferType() >= BT_DYNAMIC_DEFAULT )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Dynamic buffers cannot be baked into a static DescriptorSet",
                                 "DescriptorSetUav::checkValidity" );
                }
            }

            ++itor;
        }
    }
}
