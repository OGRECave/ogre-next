/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#ifndef _OgreMetalRenderSystem_H_
#define _OgreMetalRenderSystem_H_

#include "OgreMetalPrerequisites.h"

#include "OgreRenderSystem.h"

#include "OgreMetalDevice.h"
#include "OgreMetalPixelFormatToShaderType.h"
#include "OgreMetalRenderPassDescriptor.h"

#import <Metal/MTLRenderCommandEncoder.h>
#import <dispatch/dispatch.h>

namespace Ogre
{
    namespace v1
    {
        class HardwareBufferManager;
    }

    /**
       Implementation of Metal as a rendering system.
    */
    class _OgreMetalExport MetalRenderSystem final : public RenderSystem
    {
        struct CachedDepthStencilState
        {
            uint16          refCount;
            bool            depthWrite;
            CompareFunction depthFunc;
            StencilParams   stencilParams;

            id<MTLDepthStencilState> depthStencilState;

            CachedDepthStencilState() : refCount( 0 ) {}

            bool operator<( const CachedDepthStencilState &other ) const
            {
                if( this->depthWrite != other.depthWrite )
                    return this->depthWrite < other.depthWrite;
                if( this->depthFunc != other.depthFunc )
                    return this->depthFunc < other.depthFunc;

                return this->stencilParams < other.stencilParams;
            }

            bool operator!=( const CachedDepthStencilState &other ) const
            {
                return this->depthWrite != other.depthWrite ||  //
                       this->depthFunc != other.depthFunc ||    //
                       this->stencilParams != other.stencilParams;
            }
        };

        typedef vector<CachedDepthStencilState>::type CachedDepthStencilStateVec;

    private:
        String mDeviceName;  // it`s hint rather than hard requirement, could be ignored if empty or
                             // device removed
        bool                       mInitialized;
        v1::HardwareBufferManager *mHardwareBufferManager;
        MetalGpuProgramManager    *mShaderManager;
        MetalProgramFactory       *mMetalProgramFactory;

        ConfigOptionMap mOptions;

        MetalPixelFormatToShaderType mPixelFormatToShaderType;

        __unsafe_unretained id<MTLBuffer> mIndirectBuffer;
        unsigned char                    *mSwIndirectBufferPtr;
        CachedDepthStencilStateVec        mDepthStencilStates;  // GUARDED_BY( mMutexDepthStencilStates )
        LightweightMutex                  mMutexDepthStencilStates;
        MetalHlmsPso const               *mPso;
        HlmsComputePso const             *mComputePso;

        bool     mStencilEnabled;
        uint32_t mStencilRefValue;

        // For v1 rendering.
        v1::IndexData   *mCurrentIndexBuffer;
        v1::VertexData  *mCurrentVertexBuffer;
        MTLPrimitiveType mCurrentPrimType;

        // TODO: AutoParamsBuffer probably belongs to MetalDevice (because it's per device?)
        typedef vector<ConstBufferPacked *>::type ConstBufferPackedVec;

        ConstBufferPackedVec mAutoParamsBuffer;
        size_t               mAutoParamsBufferIdx;
        uint8               *mCurrentAutoParamsBufferPtr;
        size_t               mCurrentAutoParamsBufferSpaceLeft;
        size_t               mHistoricalAutoParamsSize[60];

        MetalDeviceList     mDeviceList;
        MetalDevice        *mActiveDevice;
        __unsafe_unretained id<MTLRenderCommandEncoder> mActiveRenderEncoder;

        MetalDevice          mDevice;
        dispatch_semaphore_t mMainGpuSyncSemaphore;
        bool                 mMainSemaphoreAlreadyWaited;
        bool                 mBeginFrameOnceStarted;

        MetalFrameBufferDescMap mFrameBufferDescMap;
        uint32                  mEntriesToFlush;
        bool                    mVpChanged;
        bool                    mInterruptedRenderCommandEncoder;

        MetalDeviceList *getDeviceList( bool refreshList = false );

        void refreshFSAAOptions();

        void setActiveDevice( MetalDevice *device );

        id<MTLDepthStencilState> getDepthStencilState( HlmsPso *pso );
        void                     removeDepthStencilState( HlmsPso *pso );

        void cleanAutoParamsBuffers();

    public:
        MetalRenderSystem();
        ~MetalRenderSystem() override;

        void shutdown() override;

        const String &getName() const override;
        const String &getFriendlyName() const override;

        void             initConfigOptions();
        ConfigOptionMap &getConfigOptions() override { return mOptions; }
        void             setConfigOption( const String &name, const String &value ) override;

        bool supportsMultithreadedShaderCompilation() const override;

        HardwareOcclusionQuery *createHardwareOcclusionQuery() override;

        String validateConfigOptions() override { return BLANKSTRING; }

        RenderSystemCapabilities *createRenderSystemCapabilities() const override;

        void reinitialise() override;

        Window *_initialise( bool          autoCreateWindow,
                             const String &windowTitle = "OGRE Render Window" ) override;

        Window *_createRenderWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                     const NameValuePairList *miscParams = 0 ) override;

        String getErrorDescription( long errorNumber ) const override;

        bool hasStoreAndMultisampleResolve() const;

        void _useLights( const LightList &lights, unsigned short limit ) override;
        void _setWorldMatrix( const Matrix4 &m ) override;
        void _setViewMatrix( const Matrix4 &m ) override;
        void _setProjectionMatrix( const Matrix4 &m ) override;

        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                const ColourValue &specular, const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking = TVC_NONE ) override;
        void _setPointSpritesEnabled( bool enabled ) override;
        void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                  Real quadratic, Real minSize, Real maxSize ) override;

        void flushUAVs();

        void _setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                           uint32 hazardousTexIdx ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setUavCS( uint32 slotStart, const DescriptorSetUav *set ) override;

        void                     _setCurrentDeviceFromTexture( TextureGpu *texture ) override;
        MetalFrameBufferDescMap &_getFrameBufferDescMap() { return mFrameBufferDescMap; }
        RenderPassDescriptor    *createRenderPassDescriptor() override;
        void        beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                               uint8 mipLevel, const Vector4 *viewportSizes,
                                               const Vector4 *scissors, uint32 numViewports,
                                               bool overlaysEnabled, bool warnIfRtvWasFlushed ) override;
        void        executeRenderPassDescriptorDelayedActions( bool officialCall );
        void        executeRenderPassDescriptorDelayedActions() override;
        inline void endRenderPassDescriptor( bool isInterruptingRender );
        void        endRenderPassDescriptor() override;

    protected:
        TextureGpu *createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                          PixelFormatGpu depthBufferFormat, uint16 poolId ) override;

    public:
        void _setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                          const Frustum *frustum = 0 ) override;
        void _setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) override;
        void _setTextureMatrix( size_t unit, const Matrix4 &xform ) override;

        void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) override;

        void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso ) override;
        void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso ) override;

        void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams ) override;

        /// See VaoManager::waitForTailFrameToFinish
        void _waitForTailFrameToFinish();

        void _beginFrameOnce() override;
        void _endFrameOnce() override;

        void _beginFrame() override;
        void _endFrame() override;

        void _hlmsPipelineStateObjectCreated( HlmsPso *newPso ) override;
        void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso ) override;
        void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock ) override;
        void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block ) override;

    protected:
        template <typename TDescriptorSetTexture, typename TTexSlot, typename TBufferPacked, bool isUav>
        void _descriptorSetTextureCreated( TDescriptorSetTexture     *newSet,
                                           const FastArray<TTexSlot> &texContainer,
                                           uint16                    *shaderTypeTexCount );
        void destroyMetalDescriptorSetTexture( MetalDescriptorSetTexture *metalSet );

    public:
        void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet ) override;
        void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set ) override;
        void _descriptorSetUavCreated( DescriptorSetUav *newSet ) override;
        void _descriptorSetUavDestroyed( DescriptorSetUav *set ) override;

        void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock ) override;
        void _setPipelineStateObject( const HlmsPso *pso ) override;
        void _setComputePso( const HlmsComputePso *pso ) override;

        VertexElementType getColourVertexElementType() const override;

        void _dispatch( const HlmsComputePso &pso ) override;

        void _setVertexArrayObject( const VertexArrayObject *vao ) override;

        void _render( const CbDrawCallIndexed *cmd ) override;
        void _render( const CbDrawCallStrip *cmd ) override;
        void _renderEmulated( const CbDrawCallIndexed *cmd ) override;
        void _renderEmulated( const CbDrawCallStrip *cmd ) override;

        void _setRenderOperation( const v1::CbRenderOp *cmd ) override;
        void _render( const v1::CbDrawCallIndexed *cmd ) override;
        void _render( const v1::CbDrawCallStrip *cmd ) override;
        void _render( const v1::RenderOperation &op ) override;

        void bindGpuProgramParameters( GpuProgramType gptype, GpuProgramParametersSharedPtr params,
                                       uint16 variabilityMask ) override;
        void bindGpuProgramPassIterationParameters( GpuProgramType gptype ) override;

        void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                               uint8 mipLevel ) override;

        Real getHorizontalTexelOffset() override;
        Real getVerticalTexelOffset() override;
        Real getMinimumDepthInputValue() override;
        Real getMaximumDepthInputValue() override;

        void preExtraThreadsStarted() override;
        void postExtraThreadsStarted() override;
        void registerThread() override;
        void unregisterThread() override;

        unsigned int getDisplayMonitorCount() const override { return 1; }

        SampleDescription validateSampleDescription( const SampleDescription &sampleDesc,
                                                     PixelFormatGpu           format,
                                                     uint32                   textureFlags ) override;

        const PixelFormatToShaderType *getPixelFormatToShaderType() const override;

        void beginProfileEvent( const String &eventName ) override;
        void endProfileEvent() override;
        void markProfileEvent( const String &event ) override;

        void initGPUProfiling() override;
        void deinitGPUProfiling() override;
        void beginGPUSampleProfile( const String &name, uint32 *hashCache ) override;
        void endGPUSampleProfile( const String &name ) override;

        bool hasAnisotropicMipMapFilter() const override { return true; }

        void setClipPlanesImpl( const PlaneList &clipPlanes ) override;
        void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                     Window                   *primary ) override;
        void updateCompositorManager( CompositorManager2 *compositorManager ) override;
        void compositorWorkspaceBegin( CompositorWorkspace *workspace,
                                       const bool           forceBeginFrame ) override;
        void compositorWorkspaceUpdate( CompositorWorkspace *workspace ) override;
        void compositorWorkspaceEnd( CompositorWorkspace *workspace, const bool forceEndFrame ) override;

        void flushCommands() override;

        MetalDevice *getActiveDevice() { return mActiveDevice; }

        MetalProgramFactory *getMetalProgramFactory() { return mMetalProgramFactory; }

        void _notifyActiveEncoderEnded( bool callEndRenderPassDesc );
        void _notifyActiveComputeEnded();
        void _notifyNewCommandBuffer();
        void _notifyDeviceStalled();
    };
}  // namespace Ogre

#endif
