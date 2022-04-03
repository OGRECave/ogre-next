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

#include "OgreD3D11Prerequisites.h"

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
#    ifndef __D3D11StereoDriverNVIDIA_H__
#        define __D3D11StereoDriverNVIDIA_H__

#        include "OgreD3D11Device.h"
#        include "OgreD3D11StereoDriverImpl.h"
#        include "nvapi.h"

namespace Ogre
{
    /** Interface of the NVIDIA stereo driver */
    class _OgreD3D11Export D3D11StereoDriverNVIDIA : public D3D11StereoDriverImpl
    {
        // Interface
    public:
        D3D11StereoDriverNVIDIA();
        virtual ~D3D11StereoDriverNVIDIA();
        virtual bool addRenderWindow( D3D11RenderWindowBase *renderWindow );
        virtual bool removeRenderWindow( const String &renderWindowName );
        virtual bool isStereoEnabled( const String &renderWindowName );
        virtual bool setDrawBuffer( ColourBufferType colourBuffer );

    protected:
        bool logErrorMessage( NvAPI_Status nvStatus );

        typedef struct OgreStereoHandle
        {
            D3D11RenderWindowBase *renderWindow;
            StereoHandle           nvapiStereoHandle;
            NvU8                   isStereoOn;
        };

        typedef map<String, OgreStereoHandle>::type StereoHandleMap;
        StereoHandleMap                             mStereoMap;
        NvU8                                        mStereoEnabled;
    };
}  // namespace Ogre
#    endif
#endif
