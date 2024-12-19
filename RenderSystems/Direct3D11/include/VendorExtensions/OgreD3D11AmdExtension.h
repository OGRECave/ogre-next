/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#ifndef _OgreD3D11AmdExtension_H_
#define _OgreD3D11AmdExtension_H_

#include "VendorExtensions/OgreD3D11VendorExtension.h"

#if !OGRE_NO_AMD_AGS && OGRE_CPU == OGRE_CPU_X86  // x86 or x64 only
#    include <amd_ags.h>
#endif

#include "OgreHeaderPrefix.h"

namespace Ogre
{
#if !OGRE_NO_AMD_AGS && OGRE_CPU == OGRE_CPU_X86  // x86 or x64 only
    class _OgreD3D11Export D3D11AmdExtension : public D3D11VendorExtension
    {
    protected:
        AGSContext *mAgsContext;
        AGSGPUInfo  mGpuInfo;

        FastArray<AGSDX11ReturnedParams> mReturnedParams;

        static void dumpAgsInfo( const AGSGPUInfo &gpuInfo );

        virtual HRESULT createDeviceImpl( const String &appName, IDXGIAdapter *adapter,
                                          D3D_DRIVER_TYPE driverType, UINT deviceFlags,
                                          D3D_FEATURE_LEVEL *pFirstFL, UINT numFeatureLevels,
                                          D3D_FEATURE_LEVEL *outFeatureLevel, ID3D11Device **outDevice );

    public:
        D3D11AmdExtension();
        virtual ~D3D11AmdExtension();

        static bool recommendsAgs( IDXGIAdapter *adapter );

        virtual void destroyDevice( ID3D11Device *device );
    };
#else
    struct _OgreD3D11Export D3D11AmdExtension : public D3D11VendorExtension
    {
    public:
        static bool recommendsAgs( IDXGIAdapter *adapter ) { return false; }
    };
#endif
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
