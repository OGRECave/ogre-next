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

#ifndef _OgreVulkanTextureGpu_H_
#define _OgreVulkanTextureGpu_H_

#include "OgreVulkanPrerequisites.h"

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

    class _OgreVulkanExport VulkanTextureGpu : public TextureGpu
    {
    protected:
        virtual void createInternalResourcesImpl( void );
        virtual void destroyInternalResourcesImpl( void );

    public:
        VulkanTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager,
                          IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType,
                          TextureGpuManager *textureManager );
        virtual ~VulkanTextureGpu();

        virtual void getSubsampleLocations( vector<Vector2>::type locations );
        virtual void notifyDataIsReady( void );

        virtual void _autogenerateMipmaps( void );
        virtual void _setToDisplayDummyTexture( void );

        virtual bool _isDataReadyImpl( void ) const;
    };

    class _OgreVulkanExport VulkanTextureGpuRenderTarget : public VulkanTextureGpu
    {
    protected:
        uint16 mDepthBufferPoolId;
        bool mPreferDepthTexture;
        PixelFormatGpu mDesiredDepthBufferFormat;

    public:
        VulkanTextureGpuRenderTarget( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                      VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                      TextureTypes::TextureTypes initialType,
                                      TextureGpuManager *textureManager );

        virtual void _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                              PixelFormatGpu desiredDepthBufferFormat );
        virtual uint16 getDepthBufferPoolId( void ) const;
        virtual bool getPreferDepthTexture( void ) const;
        virtual PixelFormatGpu getDesiredDepthBufferFormat( void ) const;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
