/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
#include "OgreD3D11PixelFormatToShaderType.h"
#include "OgreD3D11RenderPassDescriptor.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
#    include <winrt/base.h>
#endif

namespace Ogre
{
// Enable recognizing SM2.0 HLSL shaders.
// (the same shader code could be used by many RenderSystems, directly or via Cg)
#define SUPPORT_SM2_0_HLSL_SHADERS 0

    class D3D11DriverList;
    class D3D11Driver;
    class D3D11StereoDriverBridge;

    /**
    Implementation of DirectX11 as a rendering system.
    */
    class _OgreD3D11Export D3D11RenderSystem : public RenderSystem, protected D3D11DeviceResourceManager
    {
    private:
        Ogre::String mDriverName;  // it`s hint rather than hard requirement, could be ignored if empty
                                   // or device removed
        D3D_DRIVER_TYPE mDriverType;  // should be XXX_HARDWARE, XXX_SOFTWARE or XXX_WARP, never
                                      // XXX_UNKNOWN or XXX_NULL
        D3D_FEATURE_LEVEL mFeatureLevel;
        D3D_FEATURE_LEVEL mMinRequestedFeatureLevel;
        D3D_FEATURE_LEVEL mMaxRequestedFeatureLevel;

        /// Direct3D rendering device
        D3D11Device mDevice;

        D3D11VendorExtension *mVendorExtension;

        // Stored options
        ConfigOptionMap mOptions;

        /// List of D3D drivers installed (video cards)
        D3D11DriverList *mDriverList;
        /// Currently active driver
        D3D11Driver mActiveD3DDriver;
        /// NVPerfHUD allowed?
        bool mUseNVPerfHUD;
        int  mSwitchingFullscreenCounter;  // Are we switching from windowed to fullscreen

        static void createD3D11Device( D3D11VendorExtension *vendorExtension, const String &appName,
                                       D3D11Driver *d3dDriver, D3D_DRIVER_TYPE driverType,
                                       D3D_FEATURE_LEVEL minFL, D3D_FEATURE_LEVEL maxFL,
                                       D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11Device **outDevice );

        D3D11DriverList *getDirect3DDrivers( bool refreshList = false );

        void refreshD3DSettings();
        void refreshFSAAOptions();

        void freeDevice();
        void createDevice();

        v1::D3D11HardwareBufferManager *mHardwareBufferManager;
        D3D11GpuProgramManager         *mGpuProgramManager;
        D3D11HLSLProgramFactory        *mHLSLProgramFactory;

        /// Internal method for populating the capabilities structure
        RenderSystemCapabilities *createRenderSystemCapabilities() const override;
        /** See RenderSystem definition */
        void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                     Window                   *primary ) override;

        void convertVertexShaderCaps( RenderSystemCapabilities *rsc ) const;
        void convertPixelShaderCaps( RenderSystemCapabilities *rsc ) const;
        void convertGeometryShaderCaps( RenderSystemCapabilities *rsc ) const;
        void convertHullShaderCaps( RenderSystemCapabilities *rsc ) const;
        void convertDomainShaderCaps( RenderSystemCapabilities *rsc ) const;
        void convertComputeShaderCaps( RenderSystemCapabilities *rsc ) const;

        bool checkVertexTextureFormats();

        ID3D11Buffer     *mBoundIndirectBuffer;
        unsigned char    *mSwIndirectBufferPtr;
        D3D11HlmsPso     *mPso;
        D3D11HLSLProgram *mBoundComputeProgram;
        uint32            mMaxBoundUavCS;

        uint                    mNumberOfViews;
        ID3D11RenderTargetView *mRenderTargetViews[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        ID3D11DepthStencilView *mDepthStencilView;

        /// For rendering legacy objects.
        v1::VertexData *mCurrentVertexBuffer;
        v1::IndexData  *mCurrentIndexBuffer;

        TextureUnitState::BindingType mBindingType;

        UINT mStencilRef;

        // List of class instances per shader stage
        ID3D11ClassInstance *mClassInstances[6][8];

        // Number of class instances per shader stage
        UINT mNumClassInstances[6];

        // Store created shader subroutines, to prevent creation and destruction every frame
        typedef std::map<String, ID3D11ClassInstance *>           ClassInstanceMap;
        typedef std::map<String, ID3D11ClassInstance *>::iterator ClassInstanceIterator;

        ClassInstanceMap mInstanceMap;

        D3D11FrameBufferDescMap mFrameBufferDescMap;

        /// Primary window, the one used to create the device
        D3D11Window *mPrimaryWindow;

        typedef vector<D3D11Window *>::type SecondaryWindowList;
        // List of additional windows after the first (swap chains)
        SecondaryWindowList mSecondaryWindows;

        bool mRenderSystemWasInited;

        D3D11PixelFormatToShaderType mD3D11PixelFormatToShaderType;

        ID3D11ShaderResourceView *mNullViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
        uint32                    mMaxSrvCount[NumShaderTypes];
        uint32                    mMaxComputeShaderSrvCount;

#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        D3D11StereoDriverBridge *mStereoDriver;
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WINRT && defined( __cplusplus_winrt )
        Windows::Foundation::EventRegistrationToken suspendingToken, surfaceContentLostToken;
#elif OGRE_PLATFORM == OGRE_PLATFORM_WINRT && !defined( __cplusplus_winrt )
        winrt::event_token suspendingToken, surfaceContentLostToken;
#endif

    protected:
        void setClipPlanesImpl( const PlaneList &clipPlanes ) override;

    public:
        // constructor
        D3D11RenderSystem();

        // destructor
        ~D3D11RenderSystem();

        int  getSwitchingFullscreenCounter() const { return mSwitchingFullscreenCounter; }
        void addToSwitchingFullscreenCounter() { ++mSwitchingFullscreenCounter; }

        void initRenderSystem();

        void initConfigOptions();

        // Overridden RenderSystem functions
        ConfigOptionMap &getConfigOptions() override;
        String           validateConfigOptions() override;

        Window *_initialise( bool          autoCreateWindow,
                             const String &windowTitle = "OGRE Render Window" ) override;
        /// @copydoc RenderSystem::_createRenderWindow
        Window *_createRenderWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                     const NameValuePairList *miscParams = 0 ) override;
        void    _notifyWindowDestroyed( Window *window );

        void fireDeviceEvent( D3D11Device *device, const String &name,
                              D3D11Window *sendingWindow = NULL );

        void _setCurrentDeviceFromTexture( TextureGpu *texture ) override {}

        D3D11FrameBufferDescMap &_getFrameBufferDescMap() { return mFrameBufferDescMap; }
        RenderPassDescriptor    *createRenderPassDescriptor() override;
        void beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                        uint8 mipLevel, const Vector4 *viewportSizes,
                                        const Vector4 *scissors, uint32 numViewports,
                                        bool overlaysEnabled, bool warnIfRtvWasFlushed ) override;
        void endRenderPassDescriptor() override;

        TextureGpu *createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                          PixelFormatGpu depthBufferFormat, uint16 poolId ) override;

        const String &getName() const override;

        const String &getFriendlyName() const override;

        void getCustomAttribute( const String &name, void *pData ) override;
        // Low-level overridden members
        void        setConfigOption( const String &name, const String &value ) override;
        const char *getPriorityConfigOption( size_t idx ) const override;
        size_t      getNumPriorityConfigOptions() const override;

        void              reinitialise() override;
        void              shutdown() override;
        bool              isDeviceLost() override;
        bool              validateDevice( bool forceDeviceElection = false ) override;
        void              handleDeviceLost();
        void              setShadingType( ShadeOptions so );
        void              setLightingEnabled( bool enabled );
        VertexElementType getColourVertexElementType() const override;
        void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams ) override;
        void setNormaliseNormals( bool normalise );

        String getErrorDescription( long errorNumber ) const override;

        // Low-level overridden members, mainly for internal use
        D3D11HLSLProgram *_getBoundComputeProgram() const;

        void _useLights( const LightList &lights, unsigned short limit ) override;
        void _setWorldMatrix( const Matrix4 &m ) override;
        void _setViewMatrix( const Matrix4 &m ) override;
        void _setProjectionMatrix( const Matrix4 &m ) override;
        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                const ColourValue &specular, const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking ) override;
        void _setPointSpritesEnabled( bool enabled ) override;
        void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                  Real quadratic, Real minSize, Real maxSize ) override;
        void _setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                           uint32 hazardousTexIdx ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setUavCS( uint32 slotStart, const DescriptorSetUav *set ) override;
        void _setBindingType( TextureUnitState::BindingType bindingType ) override;
        void _setVertexTexture( size_t unit, TextureGpu *tex ) override;
        void _setGeometryTexture( size_t unit, TextureGpu *tex ) override;
        void _setTessellationHullTexture( size_t unit, TextureGpu *tex ) override;
        void _setTessellationDomainTexture( size_t unit, TextureGpu *tex ) override;
        void _setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                          const Frustum *frustum = 0 ) override;
        void _setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) override;
        void _setTextureMatrix( size_t unit, const Matrix4 &xform ) override;

        bool _hlmsPipelineStateObjectCreated( HlmsPso *newPso, uint64 deadline ) override;
        void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso ) override;
        void _hlmsMacroblockCreated( HlmsMacroblock *newBlock ) override;
        void _hlmsMacroblockDestroyed( HlmsMacroblock *block ) override;

    protected:
        void createBlendState( const HlmsBlendblock *newBlock, bool bIsMultisample,
                               ComPtr<ID3D11BlendState> &outBlendblock );

    public:
        void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock ) override;
        void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block ) override;
        void _descriptorSetTextureCreated( DescriptorSetTexture *newSet ) override;
        void _descriptorSetTextureDestroyed( DescriptorSetTexture *set ) override;
        void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet ) override;
        void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set ) override;
        void _descriptorSetUavCreated( DescriptorSetUav *newSet ) override;
        void _descriptorSetUavDestroyed( DescriptorSetUav *set ) override;
        void _setHlmsMacroblock( const HlmsMacroblock *macroblock );
        void _setHlmsBlendblock( ComPtr<ID3D11BlendState> blendState );
        void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock ) override;
        void _setPipelineStateObject( const HlmsPso *pso ) override;

        void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) override;

        void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso ) override;
        void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso ) override;
        void _setComputePso( const HlmsComputePso *pso ) override;

        void _beginFrame() override;
        void _endFrame() override;
        void _render( const v1::RenderOperation &op ) override;

        void _dispatch( const HlmsComputePso &pso ) override;

        void _setVertexArrayObject( const VertexArrayObject *vao ) override;
        void _render( const CbDrawCallIndexed *cmd ) override;
        void _render( const CbDrawCallStrip *cmd ) override;
        void _renderEmulated( const CbDrawCallIndexed *cmd ) override;
        void _renderEmulated( const CbDrawCallStrip *cmd ) override;

        void _setRenderOperation( const v1::CbRenderOp *cmd ) override;
        void _render( const v1::CbDrawCallIndexed *cmd ) override;
        void _render( const v1::CbDrawCallStrip *cmd ) override;
        /** See
          RenderSystem
         */
        void bindGpuProgramParameters( GpuProgramType gptype, GpuProgramParametersSharedPtr params,
                                       uint16 mask ) override;
        /** See
          RenderSystem
         */
        void bindGpuProgramPassIterationParameters( GpuProgramType gptype ) override;

        void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                               uint8 mipLevel ) override;
        void setClipPlane( ushort index, Real A, Real B, Real C, Real D );
        void enableClipPlane( ushort index, bool enable );

        HardwareOcclusionQuery *createHardwareOcclusionQuery() override;

        Real getHorizontalTexelOffset() override;
        Real getVerticalTexelOffset() override;
        Real getMinimumDepthInputValue() override;
        Real getMaximumDepthInputValue() override;
        void registerThread() override;
        void unregisterThread() override;
        void preExtraThreadsStarted() override;
        void postExtraThreadsStarted() override;

        SampleDescription validateSampleDescription( const SampleDescription &sampleDesc,
                                                     PixelFormatGpu           format,
                                                     uint32                   textureFlags ) override;

        /// @copydoc RenderSystem::getDisplayMonitorCount
        unsigned int getDisplayMonitorCount() const override;

        /// @copydoc RenderSystem::hasAnisotropicMipMapFilter
        bool hasAnisotropicMipMapFilter() const override { return true; }

        D3D11Device &_getDevice() { return mDevice; }

        D3D_FEATURE_LEVEL _getFeatureLevel() const { return mFeatureLevel; }

        void setSubroutine( GpuProgramType gptype, size_t slotIndex, const String &subroutineName );

        void setSubroutine( GpuProgramType gptype, const String &slotName,
                            const String &subroutineName );

        /// @copydoc RenderSystem::beginProfileEvent
        void beginProfileEvent( const String &eventName ) override;

        /// @copydoc RenderSystem::endProfileEvent
        void endProfileEvent() override;

        /// @copydoc RenderSystem::markProfileEvent
        void markProfileEvent( const String &eventName ) override;

        void debugAnnotationPush( const String &event ) override;
        void debugAnnotationPop() override;

        void initGPUProfiling() override;
        void deinitGPUProfiling() override;
        void beginGPUSampleProfile( const String &name, uint32 *hashCache ) override;
        void endGPUSampleProfile( const String &name ) override;

        /// @copydoc RenderSystem::setDrawBuffer
        bool setDrawBuffer( ColourBufferType colourBuffer ) override;

        /// @copydoc RenderSystem::getPixelFormatToShaderType
        const PixelFormatToShaderType *getPixelFormatToShaderType() const override;

        void _clearStateAndFlushCommandBuffer() override;
        void flushCommands() override;
    };
}  // namespace Ogre
#endif
