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
#ifndef __OgreW32Context_H__
#define __OgreW32Context_H__

#include "OgreGL3PlusContext.h"
#include "OgreWin32Prerequisites.h"

namespace Ogre
{
    class _OgreGL3PlusExport Win32Context : public GL3PlusContext
    {
    public:
        Win32Context( HDC hdc, HGLRC glrc, uint32 contexMajorVersion, uint32 contexMinorVersion );
        virtual ~Win32Context();

        HGLRC getHGLRC() { return mGlrc; }

        /** See GL3PlusContext */
        virtual void setCurrent();
        /** See GL3PlusContext */
        virtual void endCurrent();
        /// @copydoc GL3PlusContext::clone
        GL3PlusContext *clone() const;

        virtual void releaseContext();

    protected:
        HDC    mHDC;
        HGLRC  mGlrc;
        uint32 mContexMajorVersion;
        uint32 mContexMinorVersion;
    };
}  // namespace Ogre

#endif
