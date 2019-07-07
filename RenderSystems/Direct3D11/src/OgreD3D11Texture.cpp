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
#include "OgreD3D11Texture.h"
#include "OgreD3D11HardwarePixelBuffer.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11Device.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreRoot.h"
#include "OgreLogManager.h"
#include "OgreException.h"

// TODO: load DDS using DDSTextureLoader from DirectXTK rather than D3DX11
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 && !defined(_WIN32_WINNT_WIN8)
#define USE_D3DX11_LIBRARY
#endif

#ifdef USE_D3DX11_LIBRARY
#include <d3dx11.h>
#endif

namespace Ogre 
{
    //---------------------------------------------------------------------
    D3D11Texture::D3D11Texture(ResourceManager* creator, const String& name, 
        ResourceHandle handle, const String& group, bool isManual, 
        ManualResourceLoader* loader, D3D11Device & device)
        :Texture(creator, name, handle, group, isManual, loader),
        mDevice(device),
        mD3DFormat( DXGI_FORMAT_UNKNOWN ),
        mCurrentCacheCursor( 0 ),
        mAutoMipMapGeneration(false)
    {
        mFSAAType.Count = 1;
        mFSAAType.Quality = 0;
        memset( mCachedUavViews, 0, sizeof( mCachedUavViews ) );
    }
    //---------------------------------------------------------------------
    D3D11Texture::~D3D11Texture()
    {
        // have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if (isLoaded())
        {
            unload(); 
        }
        else
        {
            freeInternalResources();
        }
    }
    //---------------------------------------------------------------------
    void D3D11Texture::notifyDeviceLost(D3D11Device* device)
    {
        unloadImpl();
    }
    //---------------------------------------------------------------------
    void D3D11Texture::notifyDeviceRestored(D3D11Device* device)
    {
        if(mIsManual)
        {
            preLoadImpl();
            createInternalResourcesImpl();
            if (mLoader != NULL)
                mLoader->loadResource(this);
            postLoadImpl();
        }
        else
        {
            preLoadImpl();
            loadImpl();
            postLoadImpl();
        }
    }
    //---------------------------------------------------------------------
    void D3D11Texture::copyToTexture(TexturePtr& target)
    {
        // check if this & target are the same format and type
        // blitting from or to cube textures is not supported yet
        if (target->getUsage() != this->getUsage() ||
            target->getTextureType() != this->getTextureType())
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, 
                "Src. and dest. textures must be of same type and must have the same usage !!!", 
                "D3D11Texture::copyToTexture" );
        }


        D3D11Texture *other;
        // get the target
        other = static_cast< D3D11Texture * >( target.get() );

        if( mpResolved2DTex && !other->mpResolved2DTex )
            mDevice.GetImmediateContext()->CopyResource( other->getTextureResource(), mpResolved2DTex.Get() );
        else
            mDevice.GetImmediateContext()->CopyResource( other->getTextureResource(), mpTex.Get() );
        if (mDevice.isError())
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, 
                "D3D11 device cannot copy resource\nError Description:" + errorDescription,
                "D3D11Texture::copyToTexture");
        }
    }
    //---------------------------------------------------------------------
    void D3D11Texture::loadImage( const Image &img )
    {
        // Use OGRE its own codecs
        vector<const Image*>::type imagePtrs;
        imagePtrs.push_back(&img);
        _loadImages( imagePtrs );
    }
    //---------------------------------------------------------------------
    ID3D11ShaderResourceView* D3D11Texture::getSrvView()
    {
        assert(mpShaderResourceView);

        ID3D11ShaderResourceView *retVal = mpShaderResourceView.Get();

        if( mpResolved2DTex )
        {
            //Has MSAA
            RenderTarget *renderTarget = mSurfaceList[0]->getRenderTarget();
            if( !mFsaaExplicitResolve )
            {
                for( size_t face=0; face<getNumFaces(); ++face )
                {
                    renderTarget = mSurfaceList[face * (mNumMipmaps+1)]->getRenderTarget();
                    if( renderTarget->isFsaaResolveDirty() )
                        renderTarget->swapBuffers();
                }
            }
            else if( renderTarget->isFsaaResolveDirty() )
            {
                //Explicit resolves. Only use the Fsaa texture before it has been resolved
                retVal = mpShaderResourceViewMsaa.Get();
            }
        }

        if( (mUsage & (TU_AUTOMIPMAP|TU_RENDERTARGET|TU_AUTOMIPMAP_AUTO)) ==
                (TU_AUTOMIPMAP|TU_RENDERTARGET|TU_AUTOMIPMAP_AUTO) )
        {
            RenderTarget *renderTarget = mSurfaceList[0]->getRenderTarget();
            if( renderTarget->isMipmapsDirty() )
                this->_autogenerateMipmaps();
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    ID3D11UnorderedAccessView* D3D11Texture::createUavView( int cacheIdx, int32 mipmapLevel,
                                                            int32 textureArrayIndex,
                                                            PixelFormat pixelFormat )
    {
        assert( cacheIdx < 16 );

        mCachedUavViews[cacheIdx].uavView.Reset();

        mCachedUavViews[cacheIdx].mipmapLevel       = mipmapLevel;
        mCachedUavViews[cacheIdx].textureArrayIndex = textureArrayIndex;
        mCachedUavViews[cacheIdx].pixelFormat       = pixelFormat;

        D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV;

        descUAV.Format = D3D11Mappings::_getPF( pixelFormat );

        switch( this->getTextureType() )
        {
        case TEX_TYPE_1D:
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
            descUAV.Texture1D.MipSlice = static_cast<UINT>( mipmapLevel );
            break;
        case TEX_TYPE_2D:
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            descUAV.Texture2D.MipSlice = static_cast<UINT>( mipmapLevel );
            break;
        case TEX_TYPE_2D_ARRAY:
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            descUAV.Texture2DArray.MipSlice         = static_cast<UINT>( mipmapLevel );
            descUAV.Texture2DArray.FirstArraySlice  = textureArrayIndex;
            descUAV.Texture2DArray.ArraySize        = static_cast<UINT>( this->getDepth() -
                                                                         textureArrayIndex );
            break;
        case TEX_TYPE_3D:
            descUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            descUAV.Texture3D.MipSlice      = static_cast<UINT>( mipmapLevel );
            descUAV.Texture3D.FirstWSlice   = 0;
            descUAV.Texture3D.WSize         = static_cast<UINT>( this->getDepth() >> mipmapLevel );
            break;
        default:
            break;
        }

        mDevice.get()->CreateUnorderedAccessView( mpTex.Get(), &descUAV, mCachedUavViews[cacheIdx].uavView.ReleaseAndGetAddressOf() );

        mCurrentCacheCursor = (cacheIdx + 1) % 4;

        return mCachedUavViews[cacheIdx].uavView.Get();
    }
    //---------------------------------------------------------------------
    ID3D11UnorderedAccessView* D3D11Texture::getUavView( int32 mipmapLevel,
                                                         int32 textureArrayIndex,
                                                         PixelFormat pixelFormat )
    {
        ID3D11UnorderedAccessView *uavView = 0;

        for( int i=0; i<4; ++i )
        {
            //Reuse resource views. Reuse res. views that are bigger than what's requested too.
            if( mipmapLevel == mCachedUavViews[i].mipmapLevel &&
                textureArrayIndex == mCachedUavViews[i].textureArrayIndex &&
                pixelFormat == mCachedUavViews[i].pixelFormat )
            {
                uavView = mCachedUavViews[i].uavView.Get();
                break;
            }
            else if( !mCachedUavViews[i].uavView )
            {
                //We create in-order. If we hit here, the next ones are also null pointers.
                uavView = createUavView( i, mipmapLevel, textureArrayIndex, pixelFormat );
                break;
            }
        }

        if( !uavView )
        {
            //If we hit here, the cache is full and couldn't find a match.
            uavView = createUavView( mCurrentCacheCursor, mipmapLevel, textureArrayIndex, pixelFormat );
        }

        return uavView;
    }
    //---------------------------------------------------------------------
    void D3D11Texture::loadImpl()
    {
        if (mUsage & TU_RENDERTARGET)
        {
            createInternalResources();
            return;
        }

        // Make sure streams prepared.
        if (mLoadedStreams.isNull())
        {
            prepareImpl();
        }

        // Set reading positions of loaded streams to the beginning.
        for (uint i = 0; i < mLoadedStreams->size(); ++i)
        {
            MemoryDataStreamPtr curDataStream = (*mLoadedStreams)[i];

            curDataStream->seek(0);
        }

        // only copy is on the stack so well-behaved if exception thrown
        LoadedStreams loadedStreams = mLoadedStreams;

        this->_loadTex(loadedStreams);

    }
    //---------------------------------------------------------------------
    void D3D11Texture::freeInternalResources(void)
    {
        freeInternalResourcesImpl();
    }
    //---------------------------------------------------------------------
    void D3D11Texture::freeInternalResourcesImpl()
    {
        mSurfaceList.clear();        
        mpTex.Reset();
        mpResolved2DTex.Reset();
        mpShaderResourceView.Reset();
        mpShaderResourceViewMsaa.Reset();
        mp1DTex.Reset();
        mp2DTex.Reset();
        mp3DTex.Reset();

        for( int i=0; i<4; ++i )
        {
            mCachedUavViews[i].uavView.Reset();
        }
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_loadTex(LoadedStreams & loadedStreams)
    {
        size_t pos = mName.find_last_of(".");
        String ext = mName.substr(pos+1);
        String baseName = mName.substr(0, pos);
        if( getSourceFileType() != "dds" && getSourceFileType() != "oitd" &&
            this->getTextureType() == TEX_TYPE_CUBE_MAP )
        {
            // Load from 6 separate files
            // Use OGRE its own codecs
            //  String baseName;
            //  size_t pos = mName.find_last_of(".");
            
            //  if ( pos != String::npos )
            //      ext = mName.substr(pos+1);
            vector<Image>::type images(6);
            ConstImagePtrList imagePtrs;
            static const String suffixes[6] = {"_rt", "_lf", "_up", "_dn", "_fr", "_bk"};

            assert(loadedStreams->size()==6);
            for(size_t i = 0; i < 6; i++)
            {
                String fullName = baseName + suffixes[i];
                if (!ext.empty())
                    fullName = fullName + "." + ext;

                // find & load resource data intro stream to allow resource
                // group changes if required
                DataStreamPtr stream((*loadedStreams)[i]);

                images[i].load(stream, ext);

                uint32 imageMips = images[i].getNumMipmaps();

                if(imageMips < mNumMipmaps) {
                    mNumMipmaps = imageMips;
                }


                imagePtrs.push_back(&images[i]);
            }

            _loadImages( imagePtrs );

        }
        else
        {
            assert(loadedStreams->size()==1);

            Image img;
            DataStreamPtr dstream((*loadedStreams)[0]);
#ifdef USE_D3DX11_LIBRARY       
            if(ext=="dds")
            {
                _loadDDS(dstream);
            }
            else
#endif
            {
                img.load(dstream, ext);
                loadImage(img);
            }
        }

        _setSrcAttributes(mWidth, mHeight, mDepth, mFormat);

    }
    //---------------------------------------------------------------------
#ifdef USE_D3DX11_LIBRARY       
    void D3D11Texture::_loadDDS(DataStreamPtr &dstream)
    {
        HRESULT hr;

        MemoryDataStreamPtr memoryptr=MemoryDataStreamPtr(new MemoryDataStream(dstream));

        D3DX11_IMAGE_LOAD_INFO loadInfo;
        loadInfo.Usage          = D3D11Mappings::_getUsage(_getTextureUsage());
		loadInfo.CpuAccessFlags = D3D11Mappings::_getAccessFlags(_getTextureUsage());
        if(mUsage & TU_DYNAMIC)
        {
            loadInfo.MipLevels = 1;
        }

        // TO DO: check cpu access flags and use loadInfo only when it is needed.
        // this is the first try

        // Load the Texture
        if (loadInfo.CpuAccessFlags == D3D11_CPU_ACCESS_WRITE)
        {
            hr = D3DX11CreateTextureFromMemory( mDevice.get(), 
                memoryptr->getPtr(),
                memoryptr->size(),
                &loadInfo,
                NULL, 
                mpTex.ReleaseAndGetAddressOf(),
                NULL );
        }
        else
        {
            hr = D3DX11CreateTextureFromMemory( mDevice.get(), 
                memoryptr->getPtr(),
                memoryptr->size(),
                NULL,
                NULL, 
                mpTex.ReleaseAndGetAddressOf(),
                NULL );
        }

        if( FAILED( hr ) )
        {
            LogManager::getSingleton().logMessage("D3D11: " + mName + " Could not be loaded");
            return;
        }   

        D3D11_RESOURCE_DIMENSION dimension;
        mpTex->GetType(&dimension);

        switch (dimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            {
                _queryInterface<ID3D11Resource, ID3D11Texture1D>(mpTex, &mp1DTex);

                D3D11_TEXTURE1D_DESC desc;
                mp1DTex->GetDesc(&desc);
                
                mFormat = D3D11Mappings::_getPF(desc.Format);
                mTextureType = TEX_TYPE_1D;

                _create1DResourceView();
            }                   
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            {
                _queryInterface<ID3D11Resource, ID3D11Texture2D>(mpTex, &mp2DTex);

                D3D11_TEXTURE2D_DESC desc;
                mp2DTex->GetDesc(&desc);
                
                mFormat = D3D11Mappings::_getPF(desc.Format);
                
                if(desc.ArraySize % 6 == 0 && desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
                    mTextureType = TEX_TYPE_CUBE_MAP; //2darray cubemap
                else if(desc.ArraySize > 1)
                    mTextureType = TEX_TYPE_2D_ARRAY;
                else
                    mTextureType = TEX_TYPE_2D;
				
				//TODO: move this line to a proper place.
				_setSrcAttributes(desc.Width, desc.Height, 1, mFormat);
				
                _create2DResourceView();
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            {
                _queryInterface<ID3D11Resource, ID3D11Texture3D>(mpTex, &mp3DTex);

                D3D11_TEXTURE3D_DESC desc;
                mp3DTex->GetDesc(&desc);

                mFormat = D3D11Mappings::_getPF(desc.Format);
                mTextureType = TEX_TYPE_3D;

                _create3DResourceView();
            }
            break;
        }
    }
#endif
    //---------------------------------------------------------------------
    void D3D11Texture::createInternalResources(void)
    {
        createInternalResourcesImpl();
    }

    //---------------------------------------------------------------------
    void D3D11Texture::createInternalResourcesImpl(void)
    {
        // If mSrcWidth and mSrcHeight are zero, the requested extents have probably been set
        // through setWidth and setHeight, which set mWidth and mHeight. Take those values.
        if(mSrcWidth == 0 || mSrcHeight == 0) {
            mSrcWidth = mWidth;
            mSrcHeight = mHeight;
        }

        // PF_L8 maps to DXGI_FORMAT_R8_UNORM and grayscale textures became "redscale", without green and blue components.
        // This can be fixed by shader modification, but here we can only convert PF_L8 to PF_R8G8B8 manually to fix the issue.
        // Note, that you can use PF_R8 to explicitly request "redscale" behavior for grayscale textures, avoiding overhead.
        if(mFormat == PF_L8)
        {
            mFormat = PF_R8G8B8;
            LogManager::getSingleton().logMessage("D3D11: Grayscale L8 texture was unpacked to R8G8B8 for correctness. Use R8 or A8 formats explicitly to avoid this.");
        }

        // Choose closest supported D3D format
        mD3DFormat = D3D11Mappings::_getGammaFormat(D3D11Mappings::_getPF(D3D11Mappings::_getClosestSupportedPF(mFormat)), isHardwareGammaEnabled());

        mFSAAType.Count = 1;
        mFSAAType.Quality = 0;
        if((mUsage & TU_RENDERTARGET) != 0 && (mUsage & TU_DYNAMIC) == 0)
        {
            D3D11RenderSystem* rsys = static_cast<D3D11RenderSystem*>(Root::getSingleton().getRenderSystem());
            // http://msdn.microsoft.com/en-us/library/windows/desktop/ff476150%28v=vs.85%29.aspx#ID3D11Device_CreateTexture2D
            // 10Level9, When using D3D11_BIND_SHADER_RESOURCE, SampleDesc.Count must be 1.
            if(rsys->_getFeatureLevel() >= D3D_FEATURE_LEVEL_10_0 || (mUsage & TU_NOT_SRV))
                rsys->determineFSAASettings(mFSAA, mFSAAHint, mD3DFormat, &mFSAAType);
        }


        // load based on tex.type
        switch (this->getTextureType())
        {
        case TEX_TYPE_1D:
            {
                D3D11RenderSystem* rs = (D3D11RenderSystem*)Root::getSingleton().getRenderSystem();
                if(rs->_getFeatureLevel() >= D3D_FEATURE_LEVEL_10_0)
                {
                    this->_create1DTex();
                    break; // For Feature levels that do not support 1D textures, revert to creating a 2D texture.
                }
            }
        case TEX_TYPE_2D:
        case TEX_TYPE_CUBE_MAP:
        case TEX_TYPE_2D_ARRAY:
            this->_create2DTex();
            break;
        case TEX_TYPE_3D:
            this->_create3DTex();
            break;
        default:
            this->freeInternalResources();
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Unknown texture type", "D3D11Texture::createInternalResources" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_create1DTex()
    {
        // we must have those defined here
        assert(mSrcWidth > 0 || mSrcHeight > 0);

        // determine total number of mipmaps including main one (d3d11 convention)
        UINT numMips = (mNumRequestedMipmaps == MIP_UNLIMITED || (1U << mNumRequestedMipmaps) > mSrcWidth) ? 0 : mNumRequestedMipmaps + 1;

        D3D11_TEXTURE1D_DESC desc;
        desc.Width          = static_cast<UINT>(mSrcWidth);
        desc.MipLevels      = numMips;
        desc.ArraySize      = 1;
        desc.Format         = mD3DFormat;
		desc.Usage			= D3D11Mappings::_getUsage(_getTextureUsage());
		desc.BindFlags		= D3D11Mappings::_getTextureBindFlags(mD3DFormat, _getTextureUsage());
		desc.CPUAccessFlags = D3D11Mappings::_getAccessFlags(_getTextureUsage());
        desc.MiscFlags      = D3D11Mappings::_getTextureMiscFlags(desc.BindFlags, getTextureType(), _getTextureUsage());

        // create the texture
        HRESULT hr = mDevice->CreateTexture1D(  
            &desc,
            NULL,
            mp1DTex.ReleaseAndGetAddressOf());                      // data pointer
        // check result and except if failed
        if (FAILED(hr) || mDevice.isError())
        {
            this->freeInternalResources();
			String errorDescription = mDevice.getErrorDescription(hr);
			OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
				"Error creating texture\nError Description:" + errorDescription,
                "D3D11Texture::_create1DTex" );
        }

        _queryInterface<ID3D11Texture1D, ID3D11Resource>(mp1DTex, &mpTex);
        _create1DResourceView();
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_create1DResourceView()
    {
        // set final tex. attributes from tex. description
        // they may differ from the source image !!!
        HRESULT hr;
        D3D11_TEXTURE1D_DESC desc;

        // set final tex. attributes from tex. description
        // they may differ from the source image !!!
        mp1DTex->GetDesc(&desc);
        mNumMipmaps = static_cast<uint8>( desc.MipLevels - 1u );

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory( &srvDesc, sizeof(srvDesc) );

        if( !(mUsage & TU_NOT_SRV) )
        {
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = desc.MipLevels;

            hr = mDevice->CreateShaderResourceView( mp1DTex.Get(), &srvDesc, mpShaderResourceView.ReleaseAndGetAddressOf() );
            if (FAILED(hr) || mDevice.isError())
            {
                String errorDescription = mDevice.getErrorDescription(hr);
                OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                               "D3D11 device can't create shader resource view.\nError Description:" +
                               errorDescription,
                               "D3D11Texture::_create1DTex");
            }
        }

        this->_setFinalAttributes(desc.Width, 1, 1, D3D11Mappings::_getPF(desc.Format), desc.MiscFlags);
    }
    //---------------------------------------------------------------------
    inline bool IsPowerOfTwo(unsigned int n)                { return ((n&(n-1))==0);                    }
    //---------------------------------------------------------------------
    void D3D11Texture::_create2DTex()
    {
        // we must have those defined here
        assert(mSrcWidth > 0 || mSrcHeight > 0);

        // determine total number of mipmaps including main one (d3d11 convention)
        UINT numMips = (mNumRequestedMipmaps == MIP_UNLIMITED || (1U << mNumRequestedMipmaps) > std::max(mSrcWidth, mSrcHeight)) ? 0 : mNumRequestedMipmaps + 1;

        D3D11_TEXTURE2D_DESC desc;
        desc.Width          = static_cast<UINT>(mSrcWidth);
        desc.Height         = static_cast<UINT>(mSrcHeight);
        desc.MipLevels      = numMips;
        desc.ArraySize      = mDepth == 0 ? 1 : mDepth;
        desc.Format         = mD3DFormat;
        desc.SampleDesc     = mFSAAType;
        desc.Usage          = D3D11Mappings::_getUsage(_getTextureUsage());
        desc.BindFlags      = D3D11Mappings::_getTextureBindFlags(mD3DFormat, _getTextureUsage());
        desc.CPUAccessFlags = D3D11Mappings::_getAccessFlags(_getTextureUsage());
        desc.MiscFlags      = D3D11Mappings::_getTextureMiscFlags(desc.BindFlags, getTextureType(), _getTextureUsage());

        if (this->getTextureType() == TEX_TYPE_CUBE_MAP)
        {
                desc.ArraySize          = 6;
        }

        D3D11RenderSystem* rs = (D3D11RenderSystem*)Root::getSingleton().getRenderSystem();
        if(rs->_getFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
        {
            // http://msdn.microsoft.com/en-us/library/windows/desktop/ff476150%28v=vs.85%29.aspx#ID3D11Device_CreateTexture2D
            // ...If MipCount > 1, Dimensions must be integral power of two...
            if(!IsPowerOfTwo(desc.Width) || !IsPowerOfTwo(desc.Height))
            {
                desc.MipLevels = 1;
            }

#if 0
           // there seems to be a Microsoft bug that crash if you do GenerateMips in a level less then D3D_FEATURE_LEVEL_10_0
           // is this still true or addressed by the code above?
           desc.MipLevels = 1;
#endif
        }

        // create the texture
        HRESULT hr = mDevice->CreateTexture2D(  
            &desc,
            NULL,// data pointer
            mp2DTex.ReleaseAndGetAddressOf());
        // check result and except if failed
        if (FAILED(hr) || mDevice.isError())
        {
            this->freeInternalResources();
            String errorDescription = mDevice.getErrorDescription(hr);
			OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                "Error creating texture\nError Description:" + errorDescription, 
                "D3D11Texture::_create2DTex" );
        }

        if( desc.SampleDesc.Count > 1 && !(mUsage & TU_NOT_SRV))
        {
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            hr = mDevice->CreateTexture2D(
                &desc,
                NULL,// data pointer
                mpResolved2DTex.ReleaseAndGetAddressOf());
            // check result and except if failed
            if (FAILED(hr) || mDevice.isError())
            {
                this->freeInternalResources();
                String errorDescription = mDevice.getErrorDescription(hr);
                OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                    "Error creating resolve texture for MSAA\nError Description:" + errorDescription,
                    "D3D11Texture::_create2DTex" );
            }
        }

        //set the base texture we'll use in the render system
        _queryInterface<ID3D11Texture2D, ID3D11Resource>(mp2DTex, &mpTex);

        _create2DResourceView();
    }
    //----------------------------------------------------------------------------
    void D3D11Texture::_create2DResourceView()
    {
        // set final tex. attributes from tex. description
        // they may differ from the source image !!!
        HRESULT hr;
        D3D11_TEXTURE2D_DESC desc;
        mp2DTex->GetDesc(&desc);
        mNumMipmaps = static_cast<uint8>( desc.MipLevels - 1u );
        
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory( &srvDesc, sizeof(srvDesc) );

        if( !(mUsage & TU_NOT_SRV) )
        {
            srvDesc.Format = desc.Format;

            switch(this->getTextureType())
            {
            case TEX_TYPE_CUBE_MAP:
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = desc.MipLevels;
                srvDesc.TextureCube.MostDetailedMip = 0;
                break;

            case TEX_TYPE_2D_ARRAY:
                if (mFSAAType.Count > 1)
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
                }
                else
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
                }
                break;

            case TEX_TYPE_2D:
            case TEX_TYPE_1D:   // For Feature levels that do not support 1D textures, revert to creating a 2D texture.
                if (mFSAAType.Count > 1)
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = desc.MipLevels;
                }
                break;
            }

            if(mFSAAType.Count > 1)
            {
                hr = mDevice->CreateShaderResourceView( mp2DTex.Get(), &srvDesc, mpShaderResourceViewMsaa.ReleaseAndGetAddressOf() );
                if (FAILED(hr) || mDevice.isError())
                {
                    String errorDescription = mDevice.getErrorDescription(hr);
                    OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                                   "D3D11 device can't create MSAA shader resource view."
                                   "\nError Description:" + errorDescription,
                                   "D3D11Texture::_create2DTex");
                }

                switch(this->getTextureType())
                {
                case TEX_TYPE_2D_ARRAY:
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
                    break;
                case TEX_TYPE_2D:
                case TEX_TYPE_1D:   // For Feature levels that do not support 1D textures,
                                    // revert to creating a 2D texture.
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = desc.MipLevels;
                    break;
                }

                hr = mDevice->CreateShaderResourceView( mpResolved2DTex.Get(), &srvDesc, mpShaderResourceView.ReleaseAndGetAddressOf() );

                if (FAILED(hr) || mDevice.isError())
                {
                    String errorDescription = mDevice.getErrorDescription(hr);
                    OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                                   "D3D11 device can't create shader resource view for resolved texture."
                                   "\nError Description:" + errorDescription,
                                   "D3D11Texture::_create2DTex");
                }
            }
            else
            {
                hr = mDevice->CreateShaderResourceView( mp2DTex.Get(), &srvDesc, mpShaderResourceView.ReleaseAndGetAddressOf() );
                if (FAILED(hr) || mDevice.isError())
                {
                    String errorDescription = mDevice.getErrorDescription(hr);
                    OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                                   "D3D11 device can't create shader resource view.\nError Description:" +
                                   errorDescription,
                                   "D3D11Texture::_create2DTex");
                }
            }
        }

        this->_setFinalAttributes( desc.Width, desc.Height, desc.ArraySize / getNumFaces(), D3D11Mappings::_getPF(desc.Format), desc.MiscFlags );
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_create3DTex()
    {
        // we must have those defined here
        assert(mWidth > 0 && mHeight > 0 && mDepth>0);

        // determine total number of mipmaps including main one (d3d11 convention)
        UINT numMips = (mNumRequestedMipmaps == MIP_UNLIMITED || (1U << mNumRequestedMipmaps) >
                        std::max(std::max(mSrcWidth, mSrcHeight), mDepth)) ? 0 : mNumRequestedMipmaps + 1;

        D3D11_TEXTURE3D_DESC desc;
        desc.Width          = static_cast<UINT>(mSrcWidth);
        desc.Height         = static_cast<UINT>(mSrcHeight);
        desc.Depth          = static_cast<UINT>(mDepth);
        desc.MipLevels      = numMips;
        desc.Format         = mD3DFormat;
		desc.Usage			= D3D11Mappings::_getUsage(_getTextureUsage());
        desc.BindFlags      = D3D11Mappings::_getTextureBindFlags(mD3DFormat, _getTextureUsage());

        D3D11RenderSystem* rsys = static_cast<D3D11RenderSystem*>(Root::getSingleton().getRenderSystem());
        if (rsys->_getFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
           desc.BindFlags       &= ~D3D11_BIND_RENDER_TARGET;

		desc.CPUAccessFlags = D3D11Mappings::_getAccessFlags(_getTextureUsage());
        desc.MiscFlags      = 0;

        // create the texture
        HRESULT hr = mDevice->CreateTexture3D(  
            &desc,
            NULL,
            mp3DTex.ReleaseAndGetAddressOf());                      // data pointer
        // check result and except if failed
        if (FAILED(hr) || mDevice.isError())
        {
            this->freeInternalResources();
            String errorDescription = mDevice.getErrorDescription(hr);
			OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                "Error creating texture\nError Description:" + errorDescription, 
                "D3D11Texture::_create3DTex" );
        }

        _queryInterface<ID3D11Texture3D, ID3D11Resource>(mp3DTex, &mpTex);
        _create3DResourceView();
    }
    //-------------------------------------------------------------------------------
    void D3D11Texture::_create3DResourceView()
    {
        // set final tex. attributes from tex. description
        // they may differ from the source image !!!
        HRESULT hr;
        D3D11_TEXTURE3D_DESC desc;
        mp3DTex->GetDesc(&desc);
        mNumMipmaps = static_cast<uint8>( desc.MipLevels - 1u );

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory( &srvDesc, sizeof(srvDesc) );

        if( !(mUsage & TU_NOT_SRV) )
        {
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = 0;
            srvDesc.Texture3D.MipLevels = desc.MipLevels;
            hr = mDevice->CreateShaderResourceView( mp3DTex.Get(), &srvDesc, mpShaderResourceView.ReleaseAndGetAddressOf() );
            if (FAILED(hr) || mDevice.isError())
            {
                String errorDescription = mDevice.getErrorDescription(hr);
                OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
                               "D3D11 device can't create shader resource view.\nError Description:" +
                               errorDescription,
                               "D3D11Texture::_create3DTex");
            }
        }

        this->_setFinalAttributes( desc.Width, desc.Height, desc.Depth, D3D11Mappings::_getPF(desc.Format), desc.MiscFlags );
    }
    //-------------------------------------------------------------------------------
    void D3D11Texture::_setFinalAttributes(unsigned long width, unsigned long height, 
        unsigned long depth, PixelFormat format, UINT miscflags)
    { 
        // set target texture attributes
        mHeight = height; 
        mWidth = width; 
        mDepth = depth;
        mFormat = format;
        mAutoMipMapGeneration = miscflags & D3D11_RESOURCE_MISC_GENERATE_MIPS;

        // Update size (the final size, including temp space because in consumed memory)
        // this is needed in Resource class
        mSize = calculateSize();

        // say to the world what we are doing
        if (mWidth != mSrcWidth ||
            mHeight != mSrcHeight)
        {
            LogManager::getSingleton().logMessage("D3D11: ***** Dimensions altered by the render system");
            LogManager::getSingleton().logMessage("D3D11: ***** Source image dimensions : " + StringConverter::toString(mSrcWidth) + "x" + StringConverter::toString(mSrcHeight));
            LogManager::getSingleton().logMessage("D3D11: ***** Texture dimensions : " + StringConverter::toString(mWidth) + "x" + StringConverter::toString(mHeight));
        }

        // Create list of subsurfaces for getBuffer()
        _createSurfaceList();
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_setSrcAttributes(unsigned long width, unsigned long height, 
        unsigned long depth, PixelFormat format)
    { 
        // set source image attributes
        mSrcWidth = width; 
        mSrcHeight = height; 
        mSrcDepth = depth;
        mSrcFormat = format;
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_autogenerateMipmaps(void)
    {
        mDevice.GetImmediateContext()->GenerateMips( mpShaderResourceView.Get() );
        mSurfaceList[0]->getRenderTarget()->_setMipmapsUpdated();
    }
    //---------------------------------------------------------------------
    void D3D11Texture::_createSurfaceList(void)
    {
        if( mUsage & TU_NOT_SRV && !(mUsage & TU_RENDERTARGET) )
            return;

        unsigned int bufusage;
        if ((mUsage & TU_DYNAMIC))
        {
            bufusage = v1::HardwareBuffer::HBU_DYNAMIC;
        }
        else
        {
            bufusage = v1::HardwareBuffer::HBU_STATIC;
        }
        if (mUsage & TU_RENDERTARGET)
        {
            bufusage |= TU_RENDERTARGET;
        }

        bool updateOldList = mSurfaceList.size() == (getNumFaces() * (mNumMipmaps + 1));
        if(!updateOldList)
        {   
            // Create new list of surfaces
            mSurfaceList.clear();
            PixelFormat format = D3D11Mappings::_getClosestSupportedPF(mSrcFormat);

            for(size_t face=0; face<getNumFaces(); ++face)
            {
                size_t width  = mWidth;
                size_t height = mHeight;
                size_t depth  = getTextureType() == TEX_TYPE_CUBE_MAP ? 1u : mDepth;
                for(size_t mip=0; mip<=mNumMipmaps; ++mip)
                { 

                    v1::D3D11HardwarePixelBuffer *buffer;
                    buffer = new v1::D3D11HardwarePixelBuffer(
                        this, // parentTexture
                        mDevice, // device
                        mip,
                        width, 
                        height, 
                        depth,
                        face,
                        format,
                        mFSAA,
                        mFSAAHint,
                        (v1::HardwareBuffer::Usage)bufusage // usage
                        ); 

                    mSurfaceList.push_back(
                        v1::HardwarePixelBufferSharedPtr(buffer)
                        );

                    width  = std::max<size_t>( width >> 1, 1 );
                    height = std::max<size_t>( height>> 1, 1 );

                    if (depth > 1 && mTextureType != TEX_TYPE_2D_ARRAY)
                        depth = std::max<size_t>( depth >> 1, 1 );
                }
            }
        }

        // do we need to bind?

    }
    //---------------------------------------------------------------------
    v1::HardwarePixelBufferSharedPtr D3D11Texture::getBuffer(size_t face, size_t mipmap)
    {
        if(face >= getNumFaces())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "A three dimensional cube has six faces",
            "D3D11Texture::getBuffer");
        if(mipmap > mNumMipmaps)
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Mipmap index out of range",
            "D3D11Texture::getBuffer");
        size_t idx = face*(mNumMipmaps+1) + mipmap;
        assert(idx < mSurfaceList.size());
        return mSurfaceList[idx];
    }
    //---------------------------------------------------------------------
    void D3D11Texture::prepareImpl( void )
    {
        if (mUsage & TU_RENDERTARGET || isManuallyLoaded())
        {
            return;
        }

        LoadedStreams loadedStreams;

        // prepare load based on tex.type
        switch (getTextureType())
        {
        case TEX_TYPE_1D:
        case TEX_TYPE_2D:
        case TEX_TYPE_2D_ARRAY:
            loadedStreams = _prepareNormTex();
            break;
        case TEX_TYPE_3D:
            loadedStreams = _prepareVolumeTex();
            break;
        case TEX_TYPE_CUBE_MAP:
            loadedStreams = _prepareCubeTex();
            break;
        default:
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Unknown texture type", "D3D11Texture::prepareImpl" );
        }

        mLoadedStreams = loadedStreams;     
    }
    //---------------------------------------------------------------------
    D3D11Texture::LoadedStreams D3D11Texture::_prepareCubeTex()
    {
        assert(getTextureType() == TEX_TYPE_CUBE_MAP);

        LoadedStreams loadedStreams = LoadedStreams(OGRE_NEW_T (vector<MemoryDataStreamPtr>::type, MEMCATEGORY_GENERAL), SPFM_DELETE_T );
        // DDS load?
        if (getSourceFileType() == "dds" || getSourceFileType() == "oitd")
        {
            // find & load resource data
            DataStreamPtr dstream = 
                ResourceGroupManager::getSingleton().openResource(
                    mName, mGroup, true, this);
            loadedStreams->push_back(MemoryDataStreamPtr(OGRE_NEW MemoryDataStream(dstream)));
        }
        else
        {
            // Load from 6 separate files
            // Use OGRE its own codecs
            String baseName, ext;
            size_t pos = mName.find_last_of(".");
            baseName = mName.substr(0, pos);
            if ( pos != String::npos )
                ext = mName.substr(pos+1);
            static const String suffixes[6] = {"_rt", "_lf", "_up", "_dn", "_fr", "_bk"};

            for(size_t i = 0; i < 6; i++)
            {
                String fullName = baseName + suffixes[i];
                if (!ext.empty())
                    fullName = fullName + "." + ext;

                // find & load resource data intro stream to allow resource
                // group changes if required
                DataStreamPtr dstream = 
                    ResourceGroupManager::getSingleton().openResource(
                        fullName, mGroup, true, this);

                loadedStreams->push_back(MemoryDataStreamPtr(OGRE_NEW MemoryDataStream(dstream)));
            }
        }

        return loadedStreams;
    }
    //---------------------------------------------------------------------
    D3D11Texture::LoadedStreams D3D11Texture::_prepareVolumeTex()
    {
        assert(getTextureType() == TEX_TYPE_3D);

        // find & load resource data
        DataStreamPtr dstream = 
            ResourceGroupManager::getSingleton().openResource(
                mName, mGroup, true, this);

        LoadedStreams loadedStreams = LoadedStreams(OGRE_NEW_T (vector<MemoryDataStreamPtr>::type, MEMCATEGORY_GENERAL), SPFM_DELETE_T);
        loadedStreams->push_back(MemoryDataStreamPtr(OGRE_NEW MemoryDataStream(dstream)));
        return loadedStreams;
    }
    //---------------------------------------------------------------------
    D3D11Texture::LoadedStreams D3D11Texture::_prepareNormTex()
    {
        assert(getTextureType() == TEX_TYPE_1D || getTextureType() == TEX_TYPE_2D || getTextureType() == TEX_TYPE_2D_ARRAY);

        // find & load resource data
        DataStreamPtr dstream = 
            ResourceGroupManager::getSingleton().openResource(
                mName, mGroup, true, this);

        LoadedStreams loadedStreams = LoadedStreams(OGRE_NEW_T (vector<MemoryDataStreamPtr>::type, MEMCATEGORY_GENERAL), SPFM_DELETE_T);
        loadedStreams->push_back(MemoryDataStreamPtr(OGRE_NEW MemoryDataStream(dstream)));
        return loadedStreams;
    }
    //---------------------------------------------------------------------
    void D3D11Texture::unprepareImpl( void )
    {
        if (mUsage & TU_RENDERTARGET || isManuallyLoaded())
        {
            return;
        }   
    }
    //---------------------------------------------------------------------
    void D3D11Texture::postLoadImpl()
    {
        mLoadedStreams.setNull();   
    }
    //---------------------------------------------------------------------
    // D3D11RenderTexture
    //---------------------------------------------------------------------
    void D3D11RenderTexture::rebind( v1::D3D11HardwarePixelBuffer *buffer )
    {
        mBuffer = buffer;
        mWidth = (unsigned int) mBuffer->getWidth();
        mHeight = (unsigned int) mBuffer->getHeight();
        mFormat = mBuffer->getFormat();
        
        ID3D11Resource * pBackBuffer = buffer->getParentTexture()->getTextureResource();

        D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
        ZeroMemory( &RTVDesc, sizeof(RTVDesc) );

        RTVDesc.Format = buffer->getParentTexture()->getD3dFormat();
        switch(buffer->getParentTexture()->getTextureType())
        {
        case TEX_TYPE_1D:
            {
                D3D11RenderSystem* rs = (D3D11RenderSystem*)Root::getSingleton().getRenderSystem();
                if(rs->_getFeatureLevel() >= D3D_FEATURE_LEVEL_10_0)
                {
                    RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    break; // For Feature levels that do not support 1D textures, revert to creating a 2D texture.
                }
            }
        case TEX_TYPE_2D:
            RTVDesc.ViewDimension = (buffer->getParentTexture()->getD3dSampleDesc().Count > 1) ?
                D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
            break;
        case TEX_TYPE_2D_ARRAY:
            RTVDesc.ViewDimension = (buffer->getParentTexture()->getD3dSampleDesc().Count > 1) ?
                D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            RTVDesc.Texture2DArray.FirstArraySlice = mZOffset;
            RTVDesc.Texture2DArray.ArraySize = 1;
            break;
        case TEX_TYPE_CUBE_MAP:
            RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            RTVDesc.Texture2DArray.FirstArraySlice = buffer->getFace();
            RTVDesc.Texture2DArray.ArraySize = 1;
            break;
        case TEX_TYPE_3D:
            RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            break;
        default:
            assert(false);
        }
        HRESULT hr = mDevice->CreateRenderTargetView( pBackBuffer, &RTVDesc, mRenderTargetView.ReleaseAndGetAddressOf() );

        if (FAILED(hr) || mDevice.isError())
        {
			String errorDescription = mDevice.getErrorDescription(hr);
			OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr,
				"Error creating Render Target View\nError Description:" + errorDescription,
                "D3D11RenderTexture::rebind" );
        }
    }
    //---------------------------------------------------------------------
    void D3D11RenderTexture::getCustomAttribute( const String& name, void *pData )
    {
        if(name == "DDBACKBUFFER")
        {
            //IDXGISurface ** pSurf = (IDXGISurface **)pData;
            //*pSurf = static_cast<D3D11HardwarePixelBuffer*>(mBuffer)->getSurface();
            *static_cast<v1::HardwarePixelBuffer**>(pData) = mBuffer;
            return;
        }
		else if(name == "HWND" || name == "WINDOW")
        {
            HWND *pHwnd = (HWND*)pData;
            *pHwnd = NULL;
            return;
        }
        else if(name == "isTexture")
        {
            bool *b = static_cast< bool * >( pData );
            *b = true;
            return;
        }
        else if(name == "BUFFER")
        {
            *static_cast<v1::HardwarePixelBuffer**>(pData) = mBuffer;
            return;
        }
        else if( name == "ID3D11Texture2D" || name == "First_ID3D11Texture2D" )
        {
            ID3D11Texture2D **pBackBuffer = (ID3D11Texture2D**)pData;
            *pBackBuffer = static_cast<v1::D3D11HardwarePixelBuffer*>(mBuffer)->getParentTexture()->GetTex2D();
            return;
        }
        else if(name == "ID3D11RenderTargetView")
        {
            *static_cast<ID3D11RenderTargetView**>(pData) = mRenderTargetView.Get();
            return;
        }
        else if( name == "numberOfViews" )
        {
            unsigned int* n = static_cast<unsigned int*>(pData);
            *n = 1;
            return;
        }

        RenderTexture::getCustomAttribute(name, pData);
    }
    //---------------------------------------------------------------------
    D3D11RenderTexture::D3D11RenderTexture( const String &name, v1::D3D11HardwarePixelBuffer *buffer,
                                            uint32 zoffset,
                                            bool writeGamma,
                                            uint fsaa,
                                            const String &fsaaHint,
                                            D3D11Device & device ) :
        RenderTexture(buffer, zoffset),
        mDevice(device),
        mHasFsaaResource( false )
    {
        mName = name;
        mHwGamma = writeGamma;
        mFSAA = fsaa;
        mFSAAHint = fsaaHint;
        mHasFsaaResource = mFSAA > 1 || (atoi(mFSAAHint.c_str()) > 0);

        rebind(buffer);
    }
    //---------------------------------------------------------------------
    D3D11RenderTexture::~D3D11RenderTexture()
    {
    }
    //---------------------------------------------------------------------
    void D3D11RenderTexture::notifyDeviceLost(D3D11Device* device)
    {
    }
    //---------------------------------------------------------------------
    void D3D11RenderTexture::notifyDeviceRestored(D3D11Device* device)
    {
        rebind(static_cast<v1::D3D11HardwarePixelBuffer*>(mBuffer));
    }
    //---------------------------------------------------------------------
    void D3D11RenderTexture::swapBuffers(void)
    {
        if( isFsaaResolveDirty() && mHasFsaaResource )
        {
            assert( dynamic_cast<v1::D3D11HardwarePixelBuffer*>( mBuffer ) );
            v1::D3D11HardwarePixelBuffer *buffer = static_cast<v1::D3D11HardwarePixelBuffer*>( mBuffer );

            D3D11Texture *texture = buffer->getParentTexture();
            mDevice.GetImmediateContext()->ResolveSubresource( texture->getResolvedTexture2D(),
                                                               buffer->getSubresourceIndex(mZOffset),
                                                               texture->getTextureResource(),
                                                               buffer->getSubresourceIndex(mZOffset),
                                                               texture->getD3dFormat() );
        }
        RenderTexture::swapBuffers();
    }
}
