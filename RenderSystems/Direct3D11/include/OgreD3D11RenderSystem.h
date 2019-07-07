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
#ifndef __D3D11RENDERSYSTEM_H__
#define __D3D11RENDERSYSTEM_H__

#include "OgreD3D11Prerequisites.h"
#include "OgreRenderSystem.h"
#include "OgreD3D11Device.h"
#include "OgreD3D11DeviceResource.h"
#include "OgreD3D11Driver.h"
#include "OgreD3D11RenderPassDescriptor.h"
#include "OgreD3D11PixelFormatToShaderType.h"

namespace Ogre 
{
	// Enable recognizing SM2.0 HLSL shaders.
	// (the same shader code could be used by many RenderSystems, directly or via Cg)
	#define SUPPORT_SM2_0_HLSL_SHADERS  0

    class D3D11DriverList;
    class D3D11Driver;
	class D3D11StereoDriverBridge;
	
    /**
    Implementation of DirectX11 as a rendering system.
    */
    class _OgreD3D11Export D3D11RenderSystem
        : public RenderSystem
        , protected D3D11DeviceResourceManager
    {
    private:
        Ogre::String mDriverName;    // it`s hint rather than hard requirement, could be ignored if empty or device removed
        D3D_DRIVER_TYPE mDriverType; // should be XXX_HARDWARE, XXX_SOFTWARE or XXX_WARP, never XXX_UNKNOWN or XXX_NULL
        D3D_FEATURE_LEVEL mFeatureLevel;
        D3D_FEATURE_LEVEL mMinRequestedFeatureLevel;
        D3D_FEATURE_LEVEL mMaxRequestedFeatureLevel;

        /// Direct3D rendering device
        D3D11Device     mDevice;

        D3D11VendorExtension *mVendorExtension;
        
        // Stored options
        ConfigOptionMap mOptions;

        /// List of D3D drivers installed (video cards)
        D3D11DriverList* mDriverList;
        /// Currently active driver
        D3D11Driver mActiveD3DDriver;
        /// NVPerfHUD allowed?
        bool mUseNVPerfHUD;
		int mSwitchingFullscreenCounter;	// Are we switching from windowed to fullscreen 

        static void createD3D11Device( D3D11VendorExtension *vendorExtension,
                                       const String &appName,
                                       D3D11Driver* d3dDriver, D3D_DRIVER_TYPE driverType,
                                       D3D_FEATURE_LEVEL minFL, D3D_FEATURE_LEVEL maxFL,
                                       D3D_FEATURE_LEVEL* pFeatureLevel,
                                       ID3D11DeviceN **outDevice, ID3D11Device1 **outDevice1 );

        D3D11DriverList* getDirect3DDrivers(bool refreshList = false);
        void refreshD3DSettings(void);
        void refreshFSAAOptions(void);

        void freeDevice(void);
        void createDevice( const String &windowTitle );
#if OGRE_PLATFORM != OGRE_PLATFORM_WINRT
            bool isWindows8OrGreater();
#endif
        v1::D3D11HardwareBufferManager* mHardwareBufferManager;
        D3D11GpuProgramManager* mGpuProgramManager;
        D3D11HLSLProgramFactory* mHLSLProgramFactory;

        /// Internal method for populating the capabilities structure
        RenderSystemCapabilities* createRenderSystemCapabilities() const;
        /** See RenderSystem definition */
        void initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, Window* primary);

        void convertVertexShaderCaps(RenderSystemCapabilities* rsc) const;
        void convertPixelShaderCaps(RenderSystemCapabilities* rsc) const;
        void convertGeometryShaderCaps(RenderSystemCapabilities* rsc) const;
        void convertHullShaderCaps(RenderSystemCapabilities* rsc) const;
        void convertDomainShaderCaps(RenderSystemCapabilities* rsc) const;
        void convertComputeShaderCaps(RenderSystemCapabilities* rsc) const;

        bool checkVertexTextureFormats(void);
        
        //TODO: Looks like dead code or useless now
        bool mReadBackAsTexture;

        ID3D11Buffer    *mBoundIndirectBuffer;
        unsigned char   *mSwIndirectBufferPtr;
        D3D11HlmsPso    *mPso;
        D3D11HLSLProgram* mBoundComputeProgram;
        uint32          mMaxBoundUavCS;

        uint            mNumberOfViews;
        ID3D11RenderTargetView *mRenderTargetViews[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        ID3D11DepthStencilView *mDepthStencilView;

        /// For rendering legacy objects.
        v1::VertexData  *mCurrentVertexBuffer;
        v1::IndexData   *mCurrentIndexBuffer;

        TextureUnitState::BindingType mBindingType;

        UINT                        mStencilRef;

        ID3D11ShaderResourceView * mBoundTextures[OGRE_MAX_TEXTURE_LAYERS];
        size_t mBoundTexturesCount;

        // List of class instances per shader stage
        ID3D11ClassInstance* mClassInstances[6][8];

        // Number of class instances per shader stage
        UINT mNumClassInstances[6];
        
        // Store created shader subroutines, to prevent creation and destruction every frame
        typedef std::map<String, ID3D11ClassInstance*> ClassInstanceMap;
        typedef std::map<String, ID3D11ClassInstance*>::iterator ClassInstanceIterator;
        ClassInstanceMap mInstanceMap;

        /// structure holding texture unit settings for every stage
        struct sD3DTextureStageDesc
        {
            /// the type of the texture
            TextureType type;

            /// texture 
            ID3D11ShaderResourceView  *pTex;
            bool used;
        } mTexStageDesc[OGRE_MAX_TEXTURE_LAYERS];

        size_t     mLastTextureUnitState;
		bool       mSamplerStatesChanged;

        FrameBufferDescMap mFrameBufferDescMap;


        /// Primary window, the one used to create the device
        D3D11Window* mPrimaryWindow;

        typedef vector<D3D11Window*>::type SecondaryWindowList;
        // List of additional windows after the first (swap chains)
        SecondaryWindowList mSecondaryWindows;

        bool mRenderSystemWasInited;

        D3D11PixelFormatToShaderType mD3D11PixelFormatToShaderType;

        ID3D11ShaderResourceView *mNullViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
        uint32 mMaxSrvCount[NumShaderTypes];
        uint32 mMaxComputeShaderSrvCount;

        String mLastWindowTitlePassedToExtensions;
		
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
		D3D11StereoDriverBridge* mStereoDriver;
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT
		Windows::Foundation::EventRegistrationToken suspendingToken, surfaceContentLostToken;
#endif

    protected:

        void setClipPlanesImpl(const PlaneList& clipPlanes);

        /**
         * With DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL flag render target views are unbound
         * from us each Present(), and we need the way to reestablish connection.
         */
        void _setRenderTargetViews( uint8 viewportRenderTargetFlags );

    public:
        // constructor
        D3D11RenderSystem( );

        // destructor
        ~D3D11RenderSystem();
		
		
		int getSwitchingFullscreenCounter() const					{ return mSwitchingFullscreenCounter; }
        void addToSwitchingFullscreenCounter()                      { ++mSwitchingFullscreenCounter; }
		
        void initRenderSystem();

        virtual void initConfigOptions(void);

        // Overridden RenderSystem functions
        ConfigOptionMap& getConfigOptions(void);
        String validateConfigOptions(void);
        Window* _initialise( bool autoCreateWindow, const String& windowTitle = "OGRE Render Window"  );
        /// @copydoc RenderSystem::_createRenderWindow
        Window* _createRenderWindow( const String &name, uint32 width, uint32 height,
                                     bool fullScreen, const NameValuePairList *miscParams = 0 );
        void _notifyWindowDestroyed( Window *window );

        /// @copydoc RenderSystem::fireDeviceEvent
        void fireDeviceEvent( D3D11Device* device, const String & name,
                              D3D11Window *sendingWindow );
#if !TODO_OGRE_2_2
        void fireDeviceEvent( D3D11Device* device, const String & name,
                              D3D11RenderWindowBase *sendingWindow = NULL ) {}
#endif

        /// @copydoc RenderSystem::createRenderTexture
        RenderTexture * createRenderTexture( const String & name, unsigned int width, unsigned int height,
            TextureType texType = TEX_TYPE_2D, PixelFormat internalFormat = PF_X8R8G8B8, 
            const NameValuePairList *miscParams = 0 ); 

        virtual void _setCurrentDeviceFromTexture( TextureGpu *texture ) {}

        virtual FrameBufferDescMap& _getFrameBufferDescMap(void)        { return mFrameBufferDescMap; }
        virtual RenderPassDescriptor* createRenderPassDescriptor(void);
        virtual void beginRenderPassDescriptor( RenderPassDescriptor *desc,
                                                TextureGpu *anyTarget, uint8 mipLevel,
                                                const Vector4 &viewportSize,
                                                const Vector4 &scissors,
                                                bool overlaysEnabled,
                                                bool warnIfRtvWasFlushed );
        virtual void endRenderPassDescriptor(void);

        TextureGpu* createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                          PixelFormatGpu depthBufferFormat );

        /// @copydoc RenderSystem::createMultiRenderTarget
        virtual MultiRenderTarget * createMultiRenderTarget(const String & name);

        virtual DepthBuffer* _createDepthBufferFor( RenderTarget *renderTarget, bool exactMatchFormat );

        /// Reverts _addManualDepthBuffer actions
        void _removeManualDepthBuffer(DepthBuffer *depthBuffer);
        /// @copydoc RenderSystem::detachRenderTarget
        virtual RenderTarget * detachRenderTarget(const String &name);

        const String& getName(void) const;
		
		const String& getFriendlyName(void) const;
		
        void getCustomAttribute(const String& name, void* pData);
        // Low-level overridden members
        void setConfigOption( const String &name, const String &value );
        void reinitialise();
        void shutdown();
        void validateDevice(bool forceDeviceElection = false);
        void handleDeviceLost();
        void setShadingType( ShadeOptions so );
        void setLightingEnabled( bool enabled );
        void destroyRenderTarget(const String& name);
        VertexElementType getColourVertexElementType(void) const;
        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );
        void setNormaliseNormals(bool normalise);

        virtual String getErrorDescription(long errorNumber) const;

        // Low-level overridden members, mainly for internal use
        D3D11HLSLProgram* _getBoundComputeProgram() const;
        void _useLights(const LightList& lights, unsigned short limit);
        void _setWorldMatrix( const Matrix4 &m );
        void _setViewMatrix( const Matrix4 &m );
        void _setProjectionMatrix( const Matrix4 &m );
        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse, const ColourValue &specular, const ColourValue &emissive, Real shininess, TrackVertexColourType tracking );
        void _setPointSpritesEnabled(bool enabled);
        void _setPointParameters(Real size, bool attenuationEnabled, 
            Real constant, Real linear, Real quadratic, Real minSize, Real maxSize);
        virtual void _setTexture( size_t unit, TextureGpu *texPtr );
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                   uint32 hazardousTexIdx );
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set );
        virtual void _setUavCS( uint32 slotStart, const DescriptorSetUav *set );
        void _setBindingType(TextureUnitState::BindingType bindingType);
        void _setVertexTexture(size_t unit, TextureGpu *tex);
        void _setGeometryTexture(size_t unit, TextureGpu *tex);
        void _setTessellationHullTexture(size_t unit, TextureGpu *tex);
        void _setTessellationDomainTexture(size_t unit, TextureGpu *tex);
        void _setTextureCoordCalculation(size_t unit, TexCoordCalcMethod m, const Frustum* frustum = 0);
        void _setTextureBlendMode( size_t unit, const LayerBlendModeEx& bm );
        void _setTextureMatrix( size_t unit, const Matrix4 &xform );
        void _setViewport( Viewport *vp );

        virtual void flushUAVs(void) {}

        virtual void _hlmsPipelineStateObjectCreated( HlmsPso *newPso );
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso );
        virtual void _hlmsMacroblockCreated( HlmsMacroblock *newBlock );
        virtual void _hlmsMacroblockDestroyed( HlmsMacroblock *block );
        virtual void _hlmsBlendblockCreated( HlmsBlendblock *newBlock );
        virtual void _hlmsBlendblockDestroyed( HlmsBlendblock *block );
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock );
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block );
        virtual void _descriptorSetTextureCreated( DescriptorSetTexture *newSet );
        virtual void _descriptorSetTextureDestroyed( DescriptorSetTexture *set );
        virtual void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet );
        virtual void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set );
        virtual void _descriptorSetUavCreated( DescriptorSetUav *newSet );
        virtual void _descriptorSetUavDestroyed( DescriptorSetUav *set );
        void _setHlmsMacroblock( const HlmsMacroblock *macroblock );
        void _setHlmsBlendblock( const HlmsBlendblock *blendblock );
        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock );
        virtual void _setPipelineStateObject( const HlmsPso *pso );

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer );

        virtual void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso );
        virtual void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso );
        virtual void _setComputePso( const HlmsComputePso *pso );

        void _beginFrame(void);
        void _endFrame(void);
        void _setFog( FogMode mode = FOG_NONE, const ColourValue& colour = ColourValue::White, Real expDensity = 1.0, Real linearStart = 0.0, Real linearEnd = 1.0 );
        void _renderUsingReadBackAsTexture(unsigned int passNr, Ogre::String variableName,unsigned int StartSlot);
        void _render(const v1::RenderOperation& op);

        virtual void _dispatch( const HlmsComputePso &pso );

        virtual void _setVertexArrayObject( const VertexArrayObject *vao );
        virtual void _render( const CbDrawCallIndexed *cmd );
        virtual void _render( const CbDrawCallStrip *cmd );
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulated( const CbDrawCallStrip *cmd );

        virtual void _setRenderOperation( const v1::CbRenderOp *cmd );
        virtual void _render( const v1::CbDrawCallIndexed *cmd );
        virtual void _render( const v1::CbDrawCallStrip *cmd );
        /** See
          RenderSystem
         */
        void bindGpuProgramParameters(GpuProgramType gptype, GpuProgramParametersSharedPtr params, uint16 mask);
        /** See
          RenderSystem
         */
        void bindGpuProgramPassIterationParameters(GpuProgramType gptype);

        virtual void clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                       TextureGpu *anyTarget, uint8 mipLevel );
        void discardFrameBuffer( unsigned int buffers );
        void setClipPlane (ushort index, Real A, Real B, Real C, Real D);
        void enableClipPlane (ushort index, bool enable);
        HardwareOcclusionQuery* createHardwareOcclusionQuery(void);
        Real getHorizontalTexelOffset(void);
        Real getVerticalTexelOffset(void);
        Real getMinimumDepthInputValue(void);
        Real getMaximumDepthInputValue(void);
        void registerThread();
        void unregisterThread();
        void preExtraThreadsStarted();
        void postExtraThreadsStarted();

        /**
         * Set current render target to target, enabling its GL context if needed
         */
        virtual void _setRenderTarget( RenderTarget *target, uint8 viewportRenderTargetFlags );

        /** Check whether or not filtering is supported for the precise texture format requested
        with the given usage options.
        */
        bool _checkTextureFilteringSupported(TextureType ttype, PixelFormat format, int usage);

        void determineFSAASettings(uint fsaa, const String& fsaaHint, DXGI_FORMAT format, DXGI_SAMPLE_DESC* outFSAASettings);

        /// @copydoc RenderSystem::getDisplayMonitorCount
        unsigned int getDisplayMonitorCount() const;

        /// @copydoc RenderSystem::hasAnisotropicMipMapFilter
        virtual bool hasAnisotropicMipMapFilter() const { return true; }  

        D3D11Device &_getDevice() { return mDevice; }
        
        
        D3D_FEATURE_LEVEL _getFeatureLevel() const { return mFeatureLevel; }

        /// @copydoc RenderSystem::setSubroutine
        void setSubroutine(GpuProgramType gptype, unsigned int slotIndex, const String& subroutineName);
        
        /// @copydoc RenderSystem::setSubroutineName
        void setSubroutine(GpuProgramType gptype, const String& slotName, const String& subroutineName);

        /// @copydoc RenderSystem::beginProfileEvent
        virtual void beginProfileEvent( const String &eventName );

        /// @copydoc RenderSystem::endProfileEvent
        virtual void endProfileEvent( void );

        /// @copydoc RenderSystem::markProfileEvent
        virtual void markProfileEvent( const String &eventName );

        virtual void initGPUProfiling(void);
        virtual void deinitGPUProfiling(void);
        virtual void beginGPUSampleProfile( const String &name, uint32 *hashCache );
        virtual void endGPUSampleProfile( const String &name );
		
		/// @copydoc RenderSystem::setDrawBuffer
		virtual bool setDrawBuffer(ColourBufferType colourBuffer);

        /// @copydoc RenderSystem::getPixelFormatToShaderType
        virtual const PixelFormatToShaderType* getPixelFormatToShaderType(void) const;

        virtual void _clearStateAndFlushCommandBuffer(void);
    };
}
#endif
