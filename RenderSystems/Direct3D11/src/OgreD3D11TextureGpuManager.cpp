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

#include "OgreD3D11TextureGpuManager.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11Device.h"
#include "OgreD3D11TextureGpu.h"
#include "OgreD3D11StagingTexture.h"
#include "OgreD3D11AsyncTextureTicket.h"
#include "OgreD3D11TextureGpuWindow.h"

#include "Vao/OgreD3D11VaoManager.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreVector2.h"

#include "OgreException.h"

namespace Ogre
{
    D3D11TextureGpuManager::D3D11TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem,
                                                    D3D11Device &device ) :
        TextureGpuManager( vaoManager, renderSystem ),
        mDevice( device )
    {
        memset( mBlankTexture, 0, sizeof(mBlankTexture) );

        D3D11_TEXTURE1D_DESC desc1;
        D3D11_TEXTURE2D_DESC desc2;
        D3D11_TEXTURE3D_DESC desc3;

        memset( &desc1, 0, sizeof(desc1) );
        memset( &desc2, 0, sizeof(desc2) );
        memset( &desc3, 0, sizeof(desc3) );

        desc1.Width     = 4;
        desc1.MipLevels = 1u;
        desc1.ArraySize = 1u;
        desc1.Format    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc1.Usage     = D3D11_USAGE_IMMUTABLE;
        desc1.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        desc2.Width     = 4u;
        desc2.Height    = 4u;
        desc2.MipLevels = 1u;
        desc2.ArraySize = 1u;
        desc2.Format    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc2.SampleDesc.Count = 1u;
        desc2.Usage     = D3D11_USAGE_IMMUTABLE;
        desc2.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        desc3.Width     = 4u;
        desc3.Height    = 4u;
        desc3.Depth     = 4u;
        desc3.MipLevels = 1u;
        desc3.Format    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc3.Usage     = D3D11_USAGE_IMMUTABLE;
        desc3.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        //Must be large enough to hold the biggest transfer we'll do.
        uint8 c_whiteData[4*4*6*4];
        uint8 c_blackData[4*4*6*4];
        memset( c_whiteData, 0xff, sizeof( c_whiteData ) );
        memset( c_blackData, 0x00, sizeof( c_blackData ) );

        D3D11_SUBRESOURCE_DATA dataWhite;
        D3D11_SUBRESOURCE_DATA dataBlack[6];

        dataWhite.pSysMem           = c_whiteData;
        dataWhite.SysMemPitch       = 4u * sizeof(uint32);
        dataWhite.SysMemSlicePitch  = dataWhite.SysMemPitch * 4u;

        for( size_t i=0; i<6u; ++i )
        {
            dataBlack[i].pSysMem            = c_blackData;
            dataBlack[i].SysMemPitch        = 4u * sizeof(uint32);
            dataBlack[i].SysMemSlicePitch   = dataBlack[i].SysMemPitch * 4u;
        }

        ID3D11Texture1D *tex1D;
        ID3D11Texture2D *tex2D;
        ID3D11Texture3D *tex3D;

        for( int i=1; i<=TextureTypes::Type3D; ++i )
        {
            HRESULT hr = 0;
            switch( i )
            {
            case TextureTypes::Unknown:
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE, "Ogre should never hit this path",
                             "D3D11TextureGpuManager::D3D11TextureGpuManager" );
                break;
            case TextureTypes::Type1D:
            case TextureTypes::Type1DArray:
                hr = mDevice->CreateTexture1D( &desc1, &dataWhite, &tex1D );
                mBlankTexture[i] = tex1D;
                break;
            case TextureTypes::Type2D:
            case TextureTypes::Type2DArray:
                hr = mDevice->CreateTexture2D( &desc2, &dataWhite, &tex2D );
                mBlankTexture[i] = tex2D;
                break;
            case TextureTypes::TypeCube:
            case TextureTypes::TypeCubeArray:
                desc2.ArraySize = 6;
                desc2.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
                hr = mDevice->CreateTexture2D( &desc2, dataBlack, &tex2D );
                mBlankTexture[i] = tex2D;
                desc2.MiscFlags = 0;
                desc2.ArraySize = 1;
                break;
            case TextureTypes::Type3D:
                hr = mDevice->CreateTexture3D( &desc3, &dataWhite, &tex3D );
                mBlankTexture[i] = tex3D;
                break;
            }

            if( FAILED(hr) || mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Error creating dummy textures\nError Description:" + errorDescription,
                                "D3D11TextureGpuManager::D3D11TextureGpuManager" );
            }

            hr = device->CreateShaderResourceView( mBlankTexture[i], 0, &mDefaultSrv[i] );
            if( FAILED(hr) || mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Error creating dummy SRVs\nError Description:" + errorDescription,
                                "D3D11TextureGpuManager::D3D11TextureGpuManager" );
            }
        }

        mBlankTexture[TextureTypes::Unknown]    = mBlankTexture[TextureTypes::Type2D];
        mDefaultSrv[TextureTypes::Unknown]      = mDefaultSrv[TextureTypes::Type2D];
    }
    //-----------------------------------------------------------------------------------
    D3D11TextureGpuManager::~D3D11TextureGpuManager()
    {
        destroyAll();

        for( int i=1; i<=TextureTypes::Type3D; ++i )
        {
            SAFE_RELEASE( mDefaultSrv[i] );
            SAFE_RELEASE( mBlankTexture[i] );
        }
        memset( mBlankTexture, 0, sizeof(mBlankTexture) );
        memset( mDefaultSrv, 0, sizeof(mDefaultSrv) );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* D3D11TextureGpuManager::createTextureGpuWindow( ID3D11Texture2D *backbuffer,
                                                                Window *window )
    {
        return OGRE_NEW D3D11TextureGpuWindow( GpuPageOutStrategy::Discard, mVaoManager,
                                               "RenderWindow",
                                               TextureFlags::NotTexture|
                                               TextureFlags::RenderToTexture|
                                               TextureFlags::RenderWindowSpecific|
                                               TextureFlags::MsaaExplicitResolve,
                                               TextureTypes::Type2D, this, backbuffer, window );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* D3D11TextureGpuManager::createWindowDepthBuffer(void)
    {
        return OGRE_NEW D3D11TextureGpuRenderTarget( GpuPageOutStrategy::Discard, mVaoManager,
                                                     "RenderWindow DepthBuffer",
                                                     TextureFlags::NotTexture|
                                                     TextureFlags::RenderToTexture|
                                                     TextureFlags::RenderWindowSpecific,
                                                     TextureTypes::Type2D, this );
    }
    //-----------------------------------------------------------------------------------
    ID3D11Resource* D3D11TextureGpuManager::getBlankTextureD3dName(
            TextureTypes::TextureTypes textureType ) const
    {
        return mBlankTexture[textureType];
    }
    //-----------------------------------------------------------------------------------
    ID3D11ShaderResourceView* D3D11TextureGpuManager::getBlankTextureSrv(
            TextureTypes::TextureTypes textureType ) const
    {
        return mDefaultSrv[textureType];
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* D3D11TextureGpuManager::createTextureImpl(
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            IdString name, uint32 textureFlags, TextureTypes::TextureTypes initialType )
    {
        D3D11TextureGpu *retVal = 0;
        if( textureFlags & TextureFlags::RenderToTexture )
        {
            retVal = OGRE_NEW D3D11TextureGpuRenderTarget( pageOutStrategy, mVaoManager, name,
                                                           textureFlags, initialType, this );
        }
        else
        {
            retVal = OGRE_NEW D3D11TextureGpu( pageOutStrategy, mVaoManager, name,
                                               textureFlags, initialType, this );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    StagingTexture* D3D11TextureGpuManager::createStagingTextureImpl( uint32 width, uint32 height,
                                                                      uint32 depth, uint32 slices,
                                                                      PixelFormatGpu pixelFormat )
    {
        D3D11StagingTexture *retVal =
                OGRE_NEW D3D11StagingTexture( mVaoManager,
                                              PixelFormatGpuUtils::getFamily( pixelFormat ),
                                              width, height, std::max( depth, slices ), mDevice );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuManager::destroyStagingTextureImpl( StagingTexture *stagingTexture )
    {
        //Do nothing, caller will delete stagingTexture.
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket* D3D11TextureGpuManager::createAsyncTextureTicketImpl(
            uint32 width, uint32 height, uint32 depthOrSlices,
            TextureTypes::TextureTypes textureType, PixelFormatGpu pixelFormatFamily )
    {
        D3D11VaoManager *vaoManager = static_cast<D3D11VaoManager*>( mVaoManager );
        return OGRE_NEW D3D11AsyncTextureTicket( width, height, depthOrSlices, textureType,
                                                 pixelFormatFamily, vaoManager );
    }
}
