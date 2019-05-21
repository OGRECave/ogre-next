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
#ifndef __D3D11TEXTURE_H__
#define __D3D11TEXTURE_H__

#include "OgreD3D11Prerequisites.h"
#include "OgreD3D11DeviceResource.h"
#include "OgreTexture.h"
#include "OgreRenderTexture.h"
#include "OgreSharedPtr.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 && !defined(_WIN32_WINNT_WIN8)
    #ifndef USE_D3DX11_LIBRARY
        #define USE_D3DX11_LIBRARY
    #endif
#endif

namespace Ogre {
    class _OgreD3D11Export D3D11Texture
        : public Texture
        , protected D3D11DeviceResource
    {
        struct CachedUavView
        {
            ComPtr<ID3D11UnorderedAccessView> uavView;
            int32           mipmapLevel;
            int32           textureArrayIndex;
            PixelFormat     pixelFormat;
        };

	public:
		/// constructor 
		D3D11Texture(ResourceManager* creator, const String& name, ResourceHandle handle,
			const String& group, bool isManual, ManualResourceLoader* loader,
			D3D11Device & device);
		/// destructor
		~D3D11Texture();

		/// overridden from Texture
		void copyToTexture(TexturePtr& target);
		/// overridden from Texture
		void loadImage(const Image &img);


		/// @copydoc Texture::getBuffer
        v1::HardwarePixelBufferSharedPtr getBuffer(size_t face, size_t mipmap);

		ID3D11Resource *getTextureResource() { assert(mpTex); return mpTex.Get(); }
		/// retrieves a pointer to the actual texture
        ID3D11ShaderResourceView *getSrvView();

        ID3D11UnorderedAccessView* getUavView( int32 mipmapLevel, int32 textureArrayIndex,
                                               PixelFormat pixelFormat );

		ID3D11Texture1D * GetTex1D() { return mp1DTex.Get(); };
		ID3D11Texture2D * GetTex2D() { return mp2DTex.Get(); };
		ID3D11Texture3D	* GetTex3D() { return mp3DTex.Get(); };

		ID3D11Texture2D *getResolvedTexture2D() { assert(mpResolved2DTex); return mpResolved2DTex.Get(); }
		bool hasResolvedTexture2D() const { return mpResolved2DTex.Get() != 0; }

		bool HasAutoMipMapGenerationEnabled() const { return mAutoMipMapGeneration; }

        DXGI_FORMAT getD3dFormat(void) const                        { return mD3DFormat; }
        DXGI_SAMPLE_DESC getD3dSampleDesc(void) const               { return mFSAAType; }

	protected:
		TextureUsage _getTextureUsage() { return static_cast<TextureUsage>(mUsage); }

    protected:
        // needed to store data between prepareImpl and loadImpl
        typedef SharedPtr<vector<MemoryDataStreamPtr>::type > LoadedStreams;

        template<typename fromtype, typename totype>
        void _queryInterface(const ComPtr<fromtype>& from, ComPtr<totype> *to)
        {
            HRESULT hr = from.As(to);

            if(FAILED(hr) || mDevice.isError())
            {
                this->freeInternalResources();
				String errorDescription = mDevice.getErrorDescription(hr);
				OGRE_EXCEPT_EX(Exception::ERR_RENDERINGAPI_ERROR, hr, "Can't get base texture\nError Description:" + errorDescription, 
                    "D3D11Texture::_queryInterface" );
            }
        }
#ifdef USE_D3DX11_LIBRARY       
        void _loadDDS(DataStreamPtr &dstream);
#endif
        void _create1DResourceView();
        void _create2DResourceView();
        void _create3DResourceView();

        ID3D11UnorderedAccessView* createUavView( int cacheIdx, int32 mipmapLevel,
                                                  int32 textureArrayIndex,
                                                  PixelFormat pixelFormat );

        /// internal method, load a normal texture
        void _loadTex(LoadedStreams & loadedStreams);

        /// internal method, create a blank normal 1D Dtexture
        void _create1DTex();
        /// internal method, create a blank normal 2D texture
        void _create2DTex();
        /// internal method, create a blank cube texture
        void _create3DTex();

        /// @copydoc Texture::createInternalResources
        void createInternalResources(void);
        /// @copydoc Texture::createInternalResourcesImpl
        void createInternalResourcesImpl(void);
        /// @copydoc Texture::freeInternalResources
        void freeInternalResources(void);
        /// free internal resources
        void freeInternalResourcesImpl(void);
        /// internal method, set Texture class source image protected attributes
        void _setSrcAttributes(unsigned long width, unsigned long height, unsigned long depth, PixelFormat format);
        /// internal method, set Texture class final texture protected attributes
        void _setFinalAttributes(unsigned long width, unsigned long height, unsigned long depth, PixelFormat format, UINT miscflags);

        virtual void _autogenerateMipmaps(void);

        /// internal method, create D3D11HardwarePixelBuffers for every face and
        /// mipmap level. This method must be called after the D3D texture object was created
        void _createSurfaceList(void);

        void notifyDeviceLost(D3D11Device* device);
        void notifyDeviceRestored(D3D11Device* device);

        /// @copydoc Resource::prepareImpl
        void prepareImpl(void);
        /// @copydoc Resource::unprepareImpl
        void unprepareImpl(void);
        /// overridden from Resource
        void loadImpl();
        /// overridden from Resource
        void postLoadImpl();

        /** Vector of pointers to streams that were pulled from disk by
            prepareImpl  but have yet to be pushed into texture memory
            by loadImpl.  Should be cleared on load and on unprepare.
        */
        LoadedStreams mLoadedStreams;
        LoadedStreams _prepareNormTex();
        LoadedStreams _prepareVolumeTex();
        LoadedStreams _prepareCubeTex();

    protected:
        D3D11Device&	mDevice;

        DXGI_FORMAT mD3DFormat;         // Effective pixel format, already gamma corrected if requested
        DXGI_SAMPLE_DESC mFSAAType;     // Effective FSAA mode, limited by hardware capabilities

        // device depended resources
        ComPtr<ID3D11Resource> mpTex;   // actual texture
        ComPtr<ID3D11Texture1D> mp1DTex;
        ComPtr<ID3D11Texture2D> mp2DTex;
        ComPtr<ID3D11Texture3D> mp3DTex;
        ComPtr<ID3D11Texture2D> mpResolved2DTex; /// Used by MSAA only.

        ComPtr<ID3D11ShaderResourceView> mpShaderResourceView;
        ComPtr<ID3D11ShaderResourceView> mpShaderResourceViewMsaa;
        CachedUavView  mCachedUavViews[4];
        uint8          mCurrentCacheCursor;

        bool mAutoMipMapGeneration;

        /// Vector of pointers to subsurfaces
        typedef vector<v1::HardwarePixelBufferSharedPtr>::type SurfaceList;
        SurfaceList                     mSurfaceList;
    };

    /// RenderTexture implementation for D3D11
    class _OgreD3D11Export D3D11RenderTexture
        : public RenderTexture
        , protected D3D11DeviceResource
    {
        D3D11Device & mDevice;
        ComPtr<ID3D11RenderTargetView> mRenderTargetView;
        bool mHasFsaaResource;
    public:
        D3D11RenderTexture( const String &name, v1::D3D11HardwarePixelBuffer *buffer, uint32 zoffset,
                            bool writeGamma, uint fsaa, const String &fsaaHint, D3D11Device &device );
        virtual ~D3D11RenderTexture();

        void rebind(v1::D3D11HardwarePixelBuffer *buffer);

        virtual void getCustomAttribute( const String& name, void *pData );

        bool requiresTextureFlipping() const { return false; }

        virtual void swapBuffers(void);

    protected:
        void notifyDeviceLost(D3D11Device* device);
        void notifyDeviceRestored(D3D11Device* device);
    };

}

#endif
