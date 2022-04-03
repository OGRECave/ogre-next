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

#ifndef _OgreGL3PlusMappings_H_
#define _OgreGL3PlusMappings_H_

#include "OgreGL3PlusPrerequisites.h"

#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreGL3PlusExport GL3PlusMappings
    {
    public:
        static GLenum get( TextureTypes::TextureTypes textureType, bool cubemapsAs2DArrays );
        /// Returns GL internal format
        static GLenum get( PixelFormatGpu pf );

        static void getFormatAndType( PixelFormatGpu pf, GLenum &format, GLenum &type );
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
