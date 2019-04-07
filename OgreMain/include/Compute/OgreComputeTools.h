/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _OgreComputeTools_H_
#define _OgreComputeTools_H_

#include "OgrePrerequisites.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreExport ComputeTools
    {
        HlmsCompute     *mHlmsCompute;

    public:
        ComputeTools( HlmsCompute *hlmsCompute );

        /** Clears the whole texture to the given bit pattern
        @remarks
            We do NOT place memory barriers, thus it's your responsability to do it!!!
        @param texture
            Texture to clear. Must be UAV.
        @param clearValue
            Bit pattern to clear to
        */
        void clearUav( TextureGpu *texture, uint32 clearValue[4] );

        /** Same as ComputeTools::clearUav but specifically for floats, and asserts
            if the texture is of integer format (i.e. it's not float, half, unorm, snorm)

            @see    ComputeTools::clearUav
        @param texture
            Texture to clear. Must be UAV.
        @param clearValue
            Value to clear to
        */
        void clearUavFloat( TextureGpu *texture, float clearValue[4] );

        /** Same as ComputeTools::clearUav but specifically for floats, and asserts
            if the texture is not of integer (signed or unsigned) format
            (i.e. it's float, half, unorm, snorm)

            @see    ComputeTools::clearUav
        @param texture
            Texture to clear. Must be UAV.
        @param clearValue
            Value to clear to
        */
        void clearUavUint( TextureGpu *texture, uint32 clearValue[4] );
    };
}

#include "OgreHeaderSuffix.h"

#endif
