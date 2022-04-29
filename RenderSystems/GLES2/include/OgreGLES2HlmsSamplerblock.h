/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#ifndef _OgreGLES2HlmsSamplerblock_H_
#define _OgreGLES2HlmsSamplerblock_H_

#include "OgreGLES2Prerequisites.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** A sampler block contains settings that go hand in hand with a texture, and thus
        are common to many textures.
        This is very analogous to D3D11_SAMPLER_DESC. @see HlmsDatablock
        Up to 32 different blocks are allowed!
    */
    struct _OgreGLES2Export GLES2HlmsSamplerblock
    {
        /// It's different from HlmsSamplerblock::mId. Can't be zero.
        uint32  mInternalId;
        GLint   mMinFilter;
        GLint   mMagFilter;
        GLint   mU;
        GLint   mV;
        GLint   mW;
        GLfloat mAnisotropy;
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
