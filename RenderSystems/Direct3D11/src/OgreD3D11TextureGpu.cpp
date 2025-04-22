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

#include "OgreD3D11TextureGpu.h"

#include "OgreD3D11Device.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11TextureGpuManager.h"
#include "OgreException.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreVector2.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    D3D11TextureGpu::D3D11TextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                      VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                      TextureTypes::TextureTypes initialType,
                                      TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDisplayTextureName( 0 )
    {
        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    D3D11TextureGpu::~D3D11TextureGpu() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::notifyDeviceLost( D3D11Device *device )
    {
        mPendingResidencyChanges = 0;  // we already cleared D3D11TextureGpuManager::mScheduledTasks
        mTexturePool = 0;              // texture pool is already destroyed
        mInternalSliceStart = 0;
        if( getResidencyStatus() == GpuResidency::Resident )
        {
            _transitionTo( GpuResidency::OnStorage, (uint8 *)0 );
            mNextResidencyStatus = GpuResidency::Resident;
        }

        mDisplayTextureName = 0;
        mDefaultDisplaySrv.Reset();
    }
    //---------------------------------------------------------------------
    void D3D11TextureGpu::notifyDeviceRestored( D3D11Device *device, unsigned pass )
    {
        if( pass == 0 )
        {
            // D3D11Window*::notifyDeviceRestored handles RenderWindow-specific textures
            if( !isRenderWindowSpecific() )
            {
                _setToDisplayDummyTexture();

                if( getNextResidencyStatus() == GpuResidency::Resident )
                {
                    mNextResidencyStatus = mResidencyStatus;
                    scheduleTransitionTo( GpuResidency::Resident );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::create1DTexture()
    {
        D3D11_TEXTURE1D_DESC desc;
        memset( &desc, 0, sizeof( desc ) );

        desc.Width = static_cast<UINT>( mWidth );
        desc.MipLevels = static_cast<UINT>( mNumMipmaps );
        desc.ArraySize = static_cast<UINT>( mDepthOrSlices );
        if( isReinterpretable() || ( isTexture() && PixelFormatGpuUtils::isDepth( mPixelFormat ) ) )
            desc.Format = D3D11Mappings::getFamily( mPixelFormat );
        else
            desc.Format = D3D11Mappings::get( mPixelFormat );
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if( isTexture() )
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if( isRenderToTexture() )
        {
            if( PixelFormatGpuUtils::isDepth( mPixelFormat ) )
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            else
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if( isUav() )
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        if( mTextureType == TextureTypes::TypeCube || mTextureType == TextureTypes::TypeCubeArray )
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        if( allowsAutoMipmaps() )
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();

        ID3D11Texture1D *texture = 0;
        HRESULT hr = device->CreateTexture1D( &desc, 0, &texture );
        mFinalTextureName.Attach( texture );

        if( FAILED( hr ) || device.isError() )
        {
            this->destroyInternalResourcesImpl();
            // TODO: Should we call this->_transitionTo( GpuResidency::OnStorage, 0 );
            // instead? But doing such thing is problematic because we're already
            // inside _transitionTo
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Texture '" + getNameStr() +
                                "': "
                                "Error creating texture\nError Description:" +
                                errorDescription,
                            "D3D11TextureGpu::create1DTexture" );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::create2DTexture( bool msaaTextureOnly /* = false */ )
    {
        D3D11_TEXTURE2D_DESC desc;
        memset( &desc, 0, sizeof( desc ) );

        desc.Width = static_cast<UINT>( mWidth );
        desc.Height = static_cast<UINT>( mHeight );
        desc.MipLevels = static_cast<UINT>( mNumMipmaps );
        desc.ArraySize = static_cast<UINT>( mDepthOrSlices );
        if( isReinterpretable() || ( isTexture() && PixelFormatGpuUtils::isDepth( mPixelFormat ) ) )
            desc.Format = D3D11Mappings::getFamily( mPixelFormat );
        else
            desc.Format = D3D11Mappings::get( mPixelFormat );
        OGRE_ASSERT_LOW( desc.Format != DXGI_FORMAT_UNKNOWN &&
                         "Invalid PixelFormatGpu requested. Did you request a CPU-only format?" );
        if( isMultisample() && hasMsaaExplicitResolves() )
        {
            desc.SampleDesc.Count = mSampleDescription.getColourSamples();
            desc.SampleDesc.Quality = mSampleDescription.getCoverageSamples()
                                          ? mSampleDescription.getCoverageSamples()
                                          : D3D11Mappings::get( mSampleDescription.getMsaaPattern() );
        }
        else
        {
            desc.SampleDesc.Count = 1u;
            desc.SampleDesc.Quality = 0;
        }
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if( isTexture() )
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if( isRenderToTexture() )
        {
            if( PixelFormatGpuUtils::isDepth( mPixelFormat ) )
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            else
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if( isUav() )
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        if( mTextureType == TextureTypes::TypeCube || mTextureType == TextureTypes::TypeCubeArray )
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        if( allowsAutoMipmaps() )
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();

        ID3D11Texture2D *texture = 0;
        HRESULT hr = S_FALSE;
        if( !msaaTextureOnly )
        {
            hr = device->CreateTexture2D( &desc, 0, &texture );
            mFinalTextureName.Attach( texture );
        }

        if( FAILED( hr ) || device.isError() )
        {
            this->destroyInternalResourcesImpl();
            // TODO: Should we call this->_transitionTo( GpuResidency::OnStorage, 0 );
            // instead? But doing such thing is problematic because we're already
            // inside _transitionTo
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Texture '" + getNameStr() +
                                "': "
                                "Error creating texture\nError Description:" +
                                errorDescription,
                            "D3D11TextureGpu::create2DTexture" );
        }

        if( isMultisample() && !hasMsaaExplicitResolves() )
        {
            // We just created the resolve texture. Must create the actual MSAA surface now.
            desc.SampleDesc.Count = mSampleDescription.getColourSamples();
            desc.SampleDesc.Quality = mSampleDescription.getCoverageSamples()
                                          ? mSampleDescription.getCoverageSamples()
                                          : D3D11Mappings::get( mSampleDescription.getMsaaPattern() );

            // Reset bind flags. We won't bind it as SRV, allows more aggressive
            // optimizations on AMD cards (DCC - Delta Color Compression since GCN 1.2)
            if( PixelFormatGpuUtils::isDepth( mPixelFormat ) )
                desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            else
                desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            desc.MiscFlags = 0;

            texture = 0;
            hr = device->CreateTexture2D( &desc, 0, &texture );
            mMsaaFramebufferName.Attach( texture );

            if( FAILED( hr ) || device.isError() )
            {
                this->destroyInternalResourcesImpl();
                // TODO: Should we call this->_transitionTo( GpuResidency::OnStorage, 0 );
                // instead? But doing such thing is problematic because we're already
                // inside _transitionTo
                String errorDescription = device.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Texture '" + getNameStr() +
                                    "': "
                                    "Error creating MSAA surface\nError Description:" +
                                    errorDescription,
                                "D3D11TextureGpu::create2DTexture" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::create3DTexture()
    {
        D3D11_TEXTURE3D_DESC desc;
        memset( &desc, 0, sizeof( desc ) );

        desc.Width = static_cast<UINT>( mWidth );
        desc.Height = static_cast<UINT>( mHeight );
        desc.Depth = static_cast<UINT>( mDepthOrSlices );
        desc.MipLevels = static_cast<UINT>( mNumMipmaps );
        if( isReinterpretable() || ( isTexture() && PixelFormatGpuUtils::isDepth( mPixelFormat ) ) )
            desc.Format = D3D11Mappings::getFamily( mPixelFormat );
        else
            desc.Format = D3D11Mappings::get( mPixelFormat );
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if( isTexture() )
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if( isRenderToTexture() )
        {
            if( PixelFormatGpuUtils::isDepth( mPixelFormat ) )
                desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
            else
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
        if( isUav() )
            desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        if( mTextureType == TextureTypes::TypeCube || mTextureType == TextureTypes::TypeCubeArray )
            desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        if( allowsAutoMipmaps() )
        {
            desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
        }

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();

        ID3D11Texture3D *texture = 0;
        HRESULT hr = device->CreateTexture3D( &desc, 0, &texture );
        mFinalTextureName.Attach( texture );

        if( FAILED( hr ) || device.isError() )
        {
            this->destroyInternalResourcesImpl();
            // TODO: Should we call this->_transitionTo( GpuResidency::OnStorage, 0 );
            // instead? But doing such thing is problematic because we're already
            // inside _transitionTo
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Texture '" + getNameStr() +
                                "': "
                                "Error creating texture\nError Description:" +
                                errorDescription,
                            "D3D11TextureGpu::create2DTexture" );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::createInternalResourcesImpl()
    {
        if( mPixelFormat == PFG_NULL )
            return;  // Nothing to do

        switch( mTextureType )
        {
        case TextureTypes::Unknown:
            OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                         "Texture '" + getNameStr() +
                             "': "
                             "Ogre should never hit this path",
                         "D3D11TextureGpu::createInternalResourcesImpl" );
            break;
        case TextureTypes::Type1D:
        case TextureTypes::Type1DArray:
            create1DTexture();
            break;
        case TextureTypes::Type2D:
        case TextureTypes::Type2DArray:
        case TextureTypes::TypeCube:
        case TextureTypes::TypeCubeArray:
            create2DTexture();
            break;
        case TextureTypes::Type3D:
            create3DTexture();
            break;
        }

        // Set debug name for RenderDoc and similar tools
        const String texName = getNameStr();
        mFinalTextureName->SetPrivateData( WKPDID_D3DDebugObjectName,
                                           static_cast<UINT>( texName.size() ), texName.c_str() );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::destroyInternalResourcesImpl()
    {
        mDefaultDisplaySrv.Reset();

        if( hasAutomaticBatching() && mTexturePool )
        {
            // This will end up calling _notifyTextureSlotChanged,
            // setting mTexturePool & mInternalSliceStart to 0
            mTextureManager->_releaseSlotFromTexture( this );
        }
        mFinalTextureName.Reset();
        mMsaaFramebufferName.Reset();

        _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::notifyDataIsReady()
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        assert( mFinalTextureName || mPixelFormat == PFG_NULL );

        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;

        mDefaultDisplaySrv.Reset();

        mDisplayTextureName = mFinalTextureName.Get();
        if( isTexture() )
        {
            DescriptorSetTexture2::TextureSlot texSlot(
                DescriptorSetTexture2::TextureSlot::makeEmpty() );
            mDefaultDisplaySrv = createSrv( texSlot );
        }

        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool D3D11TextureGpu::_isDataReadyImpl() const
    {
        return mDisplayTextureName == mFinalTextureName.Get() && mDataPreparationsPending == 0u;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::_setToDisplayDummyTexture()
    {
        mDefaultDisplaySrv.Reset();

        if( !mTextureManager )
        {
            assert( isRenderWindowSpecific() );
            return;  // This can happen if we're a window and we're on shutdown
        }

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        if( hasAutomaticBatching() )
        {
            mDisplayTextureName = textureManagerD3d->getBlankTextureD3dName( TextureTypes::Type2DArray );
            if( isTexture() )
            {
                mDefaultDisplaySrv = textureManagerD3d->getBlankTextureSrv( TextureTypes::Type2DArray );
            }
        }
        else
        {
            mDisplayTextureName = textureManagerD3d->getBlankTextureD3dName( mTextureType );
            if( isTexture() )
            {
                mDefaultDisplaySrv = textureManagerD3d->getBlankTextureSrv( mTextureType );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        TextureGpu::_notifyTextureSlotChanged( newPool, slice );

        _setToDisplayDummyTexture();

        if( mTexturePool )
        {
            assert( dynamic_cast<D3D11TextureGpu *>( mTexturePool->masterTexture ) );
            D3D11TextureGpu *masterTexture =
                static_cast<D3D11TextureGpu *>( mTexturePool->masterTexture );
            mFinalTextureName = masterTexture->mFinalTextureName;
        }

        notifyAllListenersTextureChanged( TextureGpuListener::PoolTextureSlotChanged );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::setTextureType( TextureTypes::TextureTypes textureType )
    {
        const TextureTypes::TextureTypes oldType = mTextureType;
        TextureGpu::setTextureType( textureType );

        if( oldType != mTextureType && mDisplayTextureName != mFinalTextureName.Get() )
            _setToDisplayDummyTexture();
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                  const TextureBox &srcBox, uint8 srcMipLevel,
                                  bool keepResolvedTexSynced,
                                  CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode,
                                  CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode )
    {
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel, srcTransitionMode,
                            dstTransitionMode );

        assert( dynamic_cast<D3D11TextureGpu *>( dst ) );
        D3D11TextureGpu *dstD3d = static_cast<D3D11TextureGpu *>( dst );

        D3D11_BOX d3dBox;
        d3dBox.front = srcBox.z;
        d3dBox.back = srcBox.depth;
        d3dBox.top = srcBox.y;
        d3dBox.bottom = srcBox.height;
        d3dBox.left = srcBox.x;
        d3dBox.right = srcBox.width;

        D3D11_BOX *d3dBoxPtr = &d3dBox;

        if( srcBox.equalSize( this->getEmptyBox( srcMipLevel ) ) )
            d3dBoxPtr = 0;

        ID3D11Resource *srcTextureName = this->mFinalTextureName.Get();
        ID3D11Resource *dstTextureName = dstD3d->mFinalTextureName.Get();

        if( this->isMultisample() && !this->hasMsaaExplicitResolves() )
            srcTextureName = this->mMsaaFramebufferName.Get();
        if( dstD3d->isMultisample() && !dstD3d->hasMsaaExplicitResolves() )
            dstTextureName = dstD3d->mMsaaFramebufferName.Get();

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();
        ID3D11DeviceContextN *context = device.GetImmediateContext();

        DXGI_FORMAT format = D3D11Mappings::get( dstD3d->getPixelFormat() );

        for( uint32 i = 0; i < srcBox.numSlices; ++i )
        {
            const UINT srcResourceIndex = D3D11CalcSubresource(
                srcMipLevel, srcBox.sliceStart + i + this->getInternalSliceStart(), this->mNumMipmaps );
            const UINT dstResourceIndex = D3D11CalcSubresource(
                dstMipLevel, dstBox.sliceStart + i + dstD3d->getInternalSliceStart(),
                dstD3d->mNumMipmaps );
            context->CopySubresourceRegion( dstTextureName, dstResourceIndex, dstBox.x, dstBox.y,
                                            dstBox.z, srcTextureName, srcResourceIndex, d3dBoxPtr );

            if( dstD3d->isMultisample() && !dstD3d->hasMsaaExplicitResolves() && keepResolvedTexSynced )
            {
                // Must keep the resolved texture up to date.
                context->ResolveSubresource( dstD3d->mFinalTextureName.Get(), dstResourceIndex,
                                             dstD3d->mMsaaFramebufferName.Get(), dstResourceIndex,
                                             format );
            }
        }

        if( device.isError() )
        {
            String errorDescription = device.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Texture '" + getNameStr() +
                             "': "
                             "Error copying and/or resolving texture\nError Description:" +
                             errorDescription,
                         "D3D11TextureGpu::copyTo" );
        }

        // Do not perform the sync if notifyDataIsReady hasn't been called yet (i.e. we're
        // still building the HW mipmaps, and the texture will never be ready)
        if( dst->_isDataReadyImpl() &&
            dst->getGpuPageOutStrategy() == GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            dst->_syncGpuResidentToSystemRam();
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::_autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode
                                                /*transitionMode*/ )
    {
        if( !mFinalTextureName || !isDataReady() )
            return;

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();
        device.GetImmediateContext()->GenerateMips( mDefaultDisplaySrv.Get() );
    }
    //-----------------------------------------------------------------------------------
    ComPtr<ID3D11ShaderResourceView> D3D11TextureGpu::createSrv(
        const DescriptorSetTexture2::TextureSlot &texSlot ) const
    {
        assert( isTexture() &&
                "This texture is marked as 'TextureFlags::NotTexture', which "
                "means it can't be used for reading as a regular texture." );

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC *srvDescPtr = 0;

        PixelFormatGpu format = texSlot.pixelFormat;
        if( format == PFG_UNKNOWN )
            format = mPixelFormat;

        if( format != mPixelFormat || texSlot.cubemapsAs2DArrays || texSlot.mipmapLevel > 0 ||
            texSlot.numMipmaps != 0 || texSlot.textureArrayIndex > 0 ||
            ( getTextureType() == TextureTypes::Type2DArray && mDepthOrSlices == 1u ) ||
            ( mTexturePool && mTexturePool->masterTexture->getNumSlices() == 1u ) ||
            isReinterpretable() || PixelFormatGpuUtils::isDepth( mPixelFormat ) )
        {
            memset( &srvDesc, 0, sizeof( srvDesc ) );

            srvDesc.Format = D3D11Mappings::getForSrv( format );

            const TextureTypes::TextureTypes textureType = getInternalTextureType();

            const bool isMsaaSrv = isMultisample() && hasMsaaExplicitResolves();
            srvDesc.ViewDimension =
                D3D11Mappings::get( textureType, texSlot.cubemapsAs2DArrays, isMsaaSrv );

            if( isMsaaSrv )
            {
                srvDesc.Texture2DMSArray.FirstArraySlice = texSlot.textureArrayIndex;
                srvDesc.Texture2DMSArray.ArraySize = mDepthOrSlices - texSlot.textureArrayIndex;
            }
            else if( textureType == TextureTypes::TypeCubeArray && !texSlot.cubemapsAs2DArrays )
            {
                // Write to all elements due to C++ aliasing rules on the union.
                srvDesc.TextureCubeArray.MostDetailedMip = texSlot.mipmapLevel;
                srvDesc.TextureCubeArray.MipLevels = mNumMipmaps - texSlot.mipmapLevel;
                srvDesc.TextureCubeArray.First2DArrayFace = texSlot.textureArrayIndex;
                srvDesc.TextureCubeArray.NumCubes = ( mDepthOrSlices - texSlot.textureArrayIndex ) / 6u;
            }
            else
            {
                // It's a union, so 2DArray == everyone else.
                uint8 numMipmaps = texSlot.numMipmaps;
                if( !texSlot.numMipmaps )
                    numMipmaps = mNumMipmaps - texSlot.mipmapLevel;

                OGRE_ASSERT_LOW( numMipmaps <= mNumMipmaps - texSlot.mipmapLevel &&
                                 "Asking for more mipmaps than the texture has!" );

                srvDesc.Texture2DArray.MostDetailedMip = texSlot.mipmapLevel;
                srvDesc.Texture2DArray.MipLevels = numMipmaps;

                if( textureType == TextureTypes::Type1DArray ||
                    textureType == TextureTypes::Type2DArray )
                {
                    srvDesc.Texture2DArray.FirstArraySlice = texSlot.textureArrayIndex;
                    srvDesc.Texture2DArray.ArraySize = mDepthOrSlices - texSlot.textureArrayIndex;
                }
            }

            srvDescPtr = &srvDesc;
        }

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();
        ComPtr<ID3D11ShaderResourceView> retVal;
        HRESULT hr =
            device->CreateShaderResourceView( mDisplayTextureName, srvDescPtr, retVal.GetAddressOf() );
        if( FAILED( hr ) || device.isError() )
        {
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Texture '" + getNameStr() +
                                "': "
                                "Error creating ShaderResourceView\nError Description:" +
                                errorDescription,
                            "D3D11TextureGpu::createSrv" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ComPtr<ID3D11ShaderResourceView> D3D11TextureGpu::createSrv() const
    {
        assert( isTexture() &&
                "This texture is marked as 'TextureFlags::NotTexture', which "
                "means it can't be used for reading as a regular texture." );
        assert( mDefaultDisplaySrv.Get() &&
                "Either the texture wasn't properly loaded or _setToDisplayDummyTexture "
                "wasn't called when it should have been" );

        return mDefaultDisplaySrv;
    }
    //-----------------------------------------------------------------------------------
    ComPtr<ID3D11UnorderedAccessView> D3D11TextureGpu::createUav(
        const DescriptorSetUav::TextureSlot &texSlot ) const
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        memset( &uavDesc, 0, sizeof( uavDesc ) );

        PixelFormatGpu finalFormat;
        if( texSlot.pixelFormat == PFG_UNKNOWN )
            finalFormat = PixelFormatGpuUtils::getEquivalentLinear( mPixelFormat );
        else
            finalFormat = texSlot.pixelFormat;

        uavDesc.Format = D3D11Mappings::get( finalFormat );

        switch( mTextureType )
        {
        case TextureTypes::Type1D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = static_cast<UINT>( texSlot.mipmapLevel );
            break;
        case TextureTypes::Type1DArray:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1DArray.MipSlice = static_cast<UINT>( texSlot.mipmapLevel );
            uavDesc.Texture1DArray.FirstArraySlice = static_cast<UINT>( texSlot.textureArrayIndex );
            uavDesc.Texture1DArray.ArraySize =
                static_cast<UINT>( getNumSlices() - texSlot.textureArrayIndex );
            break;
        case TextureTypes::Type2D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = static_cast<UINT>( texSlot.mipmapLevel );
            break;
        case TextureTypes::Type2DArray:
        case TextureTypes::TypeCube:
        case TextureTypes::TypeCubeArray:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.MipSlice = static_cast<UINT>( texSlot.mipmapLevel );
            uavDesc.Texture2DArray.FirstArraySlice = texSlot.textureArrayIndex;
            uavDesc.Texture2DArray.ArraySize =
                static_cast<UINT>( getNumSlices() - texSlot.textureArrayIndex );
            break;
        case TextureTypes::Type3D:
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.MipSlice = static_cast<UINT>( texSlot.mipmapLevel );
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = static_cast<UINT>( getDepth() >> texSlot.mipmapLevel );
            break;
        default:
            break;
        }

        D3D11TextureGpuManager *textureManagerD3d =
            static_cast<D3D11TextureGpuManager *>( mTextureManager );
        D3D11Device &device = textureManagerD3d->getDevice();

        ComPtr<ID3D11UnorderedAccessView> retVal;
        HRESULT hr = device->CreateUnorderedAccessView( mFinalTextureName.Get(), &uavDesc,
                                                        retVal.GetAddressOf() );
        if( FAILED( hr ) )
        {
            String errorDescription = device.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Failed to create UAV view on texture '" + this->getNameStr() +
                                "'\nError Description: " + errorDescription,
                            "D3D11TextureGpu::createUav" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool D3D11TextureGpu::isMsaaPatternSupported( MsaaPatterns::MsaaPatterns pattern )
    {
        return pattern != MsaaPatterns::Center;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mSampleDescription.getColourSamples() );
        if( mSampleDescription.getColourSamples() <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mSampleDescription.getMsaaPattern() != MsaaPatterns::Undefined );

            if( mSampleDescription.getMsaaPattern() == MsaaPatterns::Standard )
            {
                // As defined per D3D11_STANDARD_MULTISAMPLE_PATTERN docs.
                switch( mSampleDescription.getColourSamples() )
                {
                case 2:
                    locations.push_back( Vector2( Real( 4.0 / 8.0 ), Real( 4.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -4.0 / 8.0 ), Real( -4.0 / 8.0 ) ) );
                    break;
                case 4:
                    locations.push_back( Vector2( Real( -2.0 / 8.0 ), Real( -6.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 6.0 / 8.0 ), Real( -2.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -6.0 / 8.0 ), Real( 2.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 2.0 / 8.0 ), Real( 6.0 / 8.0 ) ) );
                    break;
                case 8:
                    locations.push_back( Vector2( Real( 1.0 / 8.0 ), Real( -3.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -1.0 / 8.0 ), Real( 3.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 5.0 / 8.0 ), Real( 1.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -3.0 / 8.0 ), Real( -5.0 / 8.0 ) ) );

                    locations.push_back( Vector2( Real( -5.0 / 8.0 ), Real( 5.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -7.0 / 8.0 ), Real( -1.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 3.0 / 8.0 ), Real( 7.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 7.0 / 8.0 ), Real( -7.0 / 8.0 ) ) );
                    break;
                case 16:
                    locations.push_back( Vector2( Real( 1.0 / 8.0 ), Real( 1.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -1.0 / 8.0 ), Real( -3.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -3.0 / 8.0 ), Real( 2.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 4.0 / 8.0 ), Real( -1.0 / 8.0 ) ) );

                    locations.push_back( Vector2( Real( -5.0 / 8.0 ), Real( -2.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 2.0 / 8.0 ), Real( 5.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 5.0 / 8.0 ), Real( 3.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 3.0 / 8.0 ), Real( -5.0 / 8.0 ) ) );

                    locations.push_back( Vector2( Real( -2.0 / 8.0 ), Real( 6.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 0.0 / 8.0 ), Real( -7.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -4.0 / 8.0 ), Real( -6.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -6.0 / 8.0 ), Real( 4.0 / 8.0 ) ) );

                    locations.push_back( Vector2( Real( -8.0 / 8.0 ), Real( 0.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 7.0 / 8.0 ), Real( -4.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( 6.0 / 8.0 ), Real( 7.0 / 8.0 ) ) );
                    locations.push_back( Vector2( Real( -7.0 / 8.0 ), Real( -8.0 / 8.0 ) ) );
                    break;
                }
            }
            else
            {
                // Center
                for( uint8 i = 0; i < mSampleDescription.getColourSamples(); ++i )
                    locations.push_back( Vector2( 0, 0 ) );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpu::getCustomAttribute( IdString name, void *pData )
    {
        if( name == msFinalTextureBuffer || name == "ID3D11Resource" )
        {
            ID3D11Resource **pTex = (ID3D11Resource **)pData;
            *pTex = mFinalTextureName.Get();
        }
        else if( name == msMsaaTextureBuffer )
        {
            ID3D11Resource **pTex = (ID3D11Resource **)pData;
            *pTex = mMsaaFramebufferName.Get();
        }
        else
        {
            TextureGpu::getCustomAttribute( name, pData );
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    D3D11TextureGpuRenderTarget::D3D11TextureGpuRenderTarget(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType,
        TextureGpuManager *textureManager ) :
        D3D11TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        ,
        mOrientationMode( msDefaultOrientationMode )
#endif
    {
        if( mPixelFormat == PFG_NULL )
            mDepthBufferPoolId = 0;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuRenderTarget::_setDepthBufferDefaults( uint16 depthBufferPoolId,
                                                               bool preferDepthTexture,
                                                               PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        OGRE_ASSERT_MEDIUM( mSourceType != TextureSourceType::SharedDepthBuffer &&
                            "Cannot call _setDepthBufferDefaults on a shared depth buffer!" );
        mDepthBufferPoolId = depthBufferPoolId;
        mPreferDepthTexture = preferDepthTexture;
        mDesiredDepthBufferFormat = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 D3D11TextureGpuRenderTarget::getDepthBufferPoolId() const { return mDepthBufferPoolId; }
    //-----------------------------------------------------------------------------------
    bool D3D11TextureGpuRenderTarget::getPreferDepthTexture() const { return mPreferDepthTexture; }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu D3D11TextureGpuRenderTarget::getDesiredDepthBufferFormat() const
    {
        return mDesiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuRenderTarget::setOrientationMode( OrientationMode orientationMode )
    {
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        mOrientationMode = orientationMode;
#endif
    }
    //-----------------------------------------------------------------------------------
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
    OrientationMode D3D11TextureGpuRenderTarget::getOrientationMode() const { return mOrientationMode; }
#endif
}  // namespace Ogre
