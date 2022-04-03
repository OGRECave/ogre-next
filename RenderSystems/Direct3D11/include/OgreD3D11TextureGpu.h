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

#ifndef _OgreD3D11TextureGpu_H_
#define _OgreD3D11TextureGpu_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreD3D11DeviceResource.h"
#include "OgreDescriptorSetTexture.h"
#include "OgreDescriptorSetUav.h"
#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreD3D11Export D3D11TextureGpu : public TextureGpu, protected D3D11DeviceResource
    {
    protected:
        /// The general case is that the whole D3D11 texture will be accessed through the SRV.
        /// That means: createSrv( this->getPixelFormat(), false );
        /// To avoid creating multiple unnecessary copies of the SRV, we keep a cache of that
        /// default SRV with us; and calling createSrv with default params will return
        /// this cache instead.
        ComPtr<ID3D11ShaderResourceView> mDefaultDisplaySrv;

        /// This will not be owned by us if hasAutomaticBatching is true.
        /// It will also not be owned by us if we're not in GpuResidency::Resident
        /// This will always point to:
        ///     * A D3D11 SRV owned by us.
        ///     * A 4x4 dummy texture (now owned by us).
        ///     * A 64x64 mipmapped texture of us (but now owned by us).
        ///     * A D3D11 SRV not owned by us, but contains the final information.
        ID3D11Resource *mDisplayTextureName;

        /// When we're transitioning to GpuResidency::Resident but we're not there yet,
        /// we will be either displaying a 4x4 dummy texture or a 64x64 one. However
        /// we reserve a spot to a final place will already be there for us once the
        /// texture data is fully uploaded. This variable contains that texture.
        /// Async upload operations should stack to this variable.
        /// May contain:
        ///     1. 0, if not resident or resident but not yet reached main thread.
        ///     2. The texture
        ///     3. An msaa texture (hasMsaaExplicitResolves == true)
        ///     4. The msaa resolved texture (hasMsaaExplicitResolves==false)
        ComPtr<ID3D11Resource> mFinalTextureName;

        /// Only used when hasMsaaExplicitResolves() == false.
        ComPtr<ID3D11Resource> mMsaaFramebufferName;

        void create1DTexture();
        void create2DTexture( bool msaaTextureOnly = false );
        void create3DTexture();

        void createInternalResourcesImpl() override;
        void destroyInternalResourcesImpl() override;

        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

    public:
        D3D11TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager,
                         IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType,
                         TextureGpuManager *textureManager );
        ~D3D11TextureGpu() override;

        void notifyDataIsReady() override;
        bool _isDataReadyImpl() const override;

        void setTextureType( TextureTypes::TextureTypes textureType ) override;

        void copyTo(
            TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel, const TextureBox &srcBox,
            uint8 srcMipLevel, bool keepResolvedTexSynced = true,
            CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode = CopyEncTransitionMode::Auto,
            CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode =
                CopyEncTransitionMode::Auto ) override;

        void _setToDisplayDummyTexture() override;
        void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice ) override;

        void _autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode transitionMode =
                                       CopyEncTransitionMode::Auto ) override;

        // The returned pointer has its ref. count incremented! Caller must decrease it!
        ComPtr<ID3D11ShaderResourceView> createSrv(
            const DescriptorSetTexture2::TextureSlot &texSlot ) const;
        ComPtr<ID3D11ShaderResourceView> createSrv() const;
        ID3D11ShaderResourceView *getDefaultDisplaySrv() const { return mDefaultDisplaySrv.Get(); }

        ComPtr<ID3D11UnorderedAccessView> createUav(
            const DescriptorSetUav::TextureSlot &texSlot ) const;

        bool isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern ) override;
        void getSubsampleLocations( vector<Vector2>::type locations ) override;

        void getCustomAttribute( IdString name, void *pData ) override;

        ID3D11Resource *getDisplayTextureName() const { return mDisplayTextureName; }
        ID3D11Resource *getFinalTextureName() const { return mFinalTextureName.Get(); }
        ID3D11Resource *getMsaaFramebufferName() const { return mMsaaFramebufferName.Get(); }
    };

    class _OgreD3D11Export D3D11TextureGpuRenderTarget : public D3D11TextureGpu
    {
    protected:
        uint16         mDepthBufferPoolId;
        bool           mPreferDepthTexture;
        PixelFormatGpu mDesiredDepthBufferFormat;
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        OrientationMode mOrientationMode;
#endif

    public:
        D3D11TextureGpuRenderTarget( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                     VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                     TextureTypes::TextureTypes initialType,
                                     TextureGpuManager         *textureManager );

        void           _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                                PixelFormatGpu desiredDepthBufferFormat ) override;
        uint16         getDepthBufferPoolId() const override;
        bool           getPreferDepthTexture() const override;
        PixelFormatGpu getDesiredDepthBufferFormat() const override;

        void setOrientationMode( OrientationMode orientationMode ) override;
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        OrientationMode getOrientationMode() const override;
#endif
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
