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

#ifndef _OgreNULLTextureGpu_H_
#define _OgreNULLTextureGpu_H_

#include "OgreNULLPrerequisites.h"

#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    class _OgreNULLExport NULLTextureGpu : public TextureGpu
    {
    protected:
        void createInternalResourcesImpl() override;
        void destroyInternalResourcesImpl() override;

    public:
        NULLTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager,
                        IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType,
                        TextureGpuManager *textureManager );
        ~NULLTextureGpu() override;

        void getSubsampleLocations( vector<Vector2>::type locations ) override;
        void notifyDataIsReady() override;

        void _autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode transitionMode =
                                       CopyEncTransitionMode::Auto ) override;
        void _setToDisplayDummyTexture() override;

        bool _isDataReadyImpl() const override;
    };

    class _OgreNULLExport NULLTextureGpuRenderTarget : public NULLTextureGpu
    {
    protected:
        uint16         mDepthBufferPoolId;
        bool           mPreferDepthTexture;
        PixelFormatGpu mDesiredDepthBufferFormat;

    public:
        NULLTextureGpuRenderTarget( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                    VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                    TextureTypes::TextureTypes initialType,
                                    TextureGpuManager         *textureManager );

        void           _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                                PixelFormatGpu desiredDepthBufferFormat ) override;
        uint16         getDepthBufferPoolId() const override;
        bool           getPreferDepthTexture() const override;
        PixelFormatGpu getDesiredDepthBufferFormat() const override;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
