/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#ifndef _OgreNULLAsyncTextureTicket_H_
#define _OgreNULLAsyncTextureTicket_H_

#include "OgreNULLPrerequisites.h"

#include "OgreAsyncTextureTicket.h"
#include "OgreTextureBox.h"

namespace Ogre
{
    /** See AsyncTextureTicket
     */
    class _OgreNULLExport NULLAsyncTextureTicket final : public AsyncTextureTicket
    {
    protected:
        uint8 *mVboName;

        TextureBox mapImpl( uint32 slice ) override;
        void       unmapImpl() override;

    public:
        NULLAsyncTextureTicket( uint32 width, uint32 height, uint32 depthOrSlices,
                                TextureTypes::TextureTypes textureType,
                                PixelFormatGpu             pixelFormatFamily );
        ~NULLAsyncTextureTicket() override;

        bool queryIsTransferDone() override;
    };
}  // namespace Ogre

#endif
