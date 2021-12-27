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
#include "OgreMetalPixelFormatToShaderType.h"
#include "OgreMetalRenderPassDescriptor.h"

#include "OgreRenderSystem.h"
#include "OgreMetalDevice.h"

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
    class _OgreMetalExport MetalRenderSystem : public RenderSystem
    {
        struct CachedDepthStencilState
        {
            uint16                      refCount;
            bool                        depthWrite;
            CompareFunction             depthFunc;
            StencilParams               stencilParams;

            id<MTLDepthStencilState>    depthStencilState;

            CachedDepthStencilState() : refCount( 0 ) {}

            bool operator < ( const CachedDepthStencilState &other ) const
            {
                if( this->depthWrite != other.depthWrite )
                    return this->depthWrite < other.depthWrite;
                if( this->depthFunc != other.depthFunc )
                    return this->depthFunc < other.depthFunc;

                return this->stencilParams < other.stencilParams;
            }

            bool operator != ( const CachedDepthStencilState &other ) const
            {
                return this->depthWrite != other.depthWrite ||
                       this->depthFunc != other.depthFunc ||
                       this->stencilParams != other.stencilParams;
            }
        };

        typedef vector<CachedDepthStencilState>::type CachedDepthStencilStateVec;

    private:
        String mDeviceName;    // it`s hint rather than hard requirement, could be ignored if empty or device removed
        bool mInitialized;
        v1::HardwareBufferManager   *mHardwareBufferManager;
        MetalGpuProgramManager      *mShaderManager;
        MetalProgramFactory         *mMetalProgramFactory;

        ConfigOptionMap mOptions;

        MetalPixelFormatToShaderType mPixelFormatToShaderType;

        __unsafe_unretained id<MTLBuffer>   mIndirectBuffer;
        unsigned char                       *mSwIndirectBufferPtr;
        CachedDepthStencilStateVec          mDepthStencilStates;
        MetalHlmsPso const                  *mPso;
        HlmsComputePso const                *mComputePso;
        
        bool mStencilEnabled;
        uint32_t mStencilRefValue;

        //For v1 rendering.
        v1::IndexData       *mCurrentIndexBuffer;
        v1::VertexData      *mCurrentVertexBuffer;
        MTLPrimitiveType    mCurrentPrimType;

        //TODO: AutoParamsBuffer probably belongs to MetalDevice (because it's per device?)
        typedef vector<ConstBufferPacked*>::type ConstBufferPackedVec;
        ConstBufferPackedVec    mAutoParamsBuffer;
        size_t                  mAutoParamsBufferIdx;
        uint8                   *mCurrentAutoParamsBufferPtr;
        size_t                  mCurrentAutoParamsBufferSpaceLeft;
        size_t                  mHistoricalAutoParamsSize[60];

        uint8           mNumMRTs;
        MetalRenderTargetCommon     *mCurrentColourRTs[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        MetalDeviceList             mDeviceList;
        MetalDevice                 *mActiveDevice;
        __unsafe_unretained id<MTLRenderCommandEncoder> mActiveRenderEncoder;

        MetalDevice             mDevice;
        dispatch_semaphore_t    mMainGpuSyncSemaphore;
        bool                    mMainSemaphoreAlreadyWaited;
        bool                    mBeginFrameOnceStarted;

        MetalFrameBufferDescMap mFrameBufferDescMap;
        uint32                  mEntriesToFlush;
        bool                    mVpChanged;
        bool                    mInterruptedRenderCommandEncoder;

        MetalDeviceList* getDeviceList( bool refreshList = false );
        void refreshFSAAOptions();

        void setActiveDevice( MetalDevice *device );

        id<MTLDepthStencilState> getDepthStencilState( HlmsPso *pso );
        void removeDepthStencilState( HlmsPso *pso );

        void cleanAutoParamsBuffers();

    public:
        MetalRenderSystem();
        virtual ~MetalRenderSystem();

        virtual void shutdown();

        virtual const String& getName() const;
        virtual const String& getFriendlyName() const;
        void initConfigOptions();
        virtual ConfigOptionMap& getConfigOptions() { return mOptions; }
        virtual void setConfigOption(const String &name, const String &value);

        virtual HardwareOcclusionQuery* createHardwareOcclusionQuery();

        virtual String validateConfigOptions()  { return BLANKSTRING; }

        virtual RenderSystemCapabilities* createRenderSystemCapabilities() const;

        virtual void reinitialise();

        virtual Window* _initialise( bool autoCreateWindow,
                                     const String& windowTitle = "OGRE Render Window" );

        virtual Window* _createRenderWindow( const String &name, uint32 width, uint32 height,
                                             bool fullScreen, const NameValuePairList *miscParams = 0 );

        virtual String getErrorDescription(long errorNumber) const;

        bool hasStoreAndMultisampleResolve() const;

        virtual void _useLights(const LightList& lights, unsigned short limit);
        virtual void _setWorldMatrix(const Matrix4 &m);
        virtual void _setViewMatrix(const Matrix4 &m);
        virtual void _setProjectionMatrix(const Matrix4 &m);

        virtual void _setSurfaceParams( const ColourValue &ambient,
                                const ColourValue &diffuse, const ColourValue &specular,
                                const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking = TVC_NONE );
        virtual void _setPointSpritesEnabled(bool enabled);
        virtual void _setPointParameters(Real size, bool attenuationEnabled,
            Real constant, Real linear, Real quadratic, Real minSize, Real maxSize);

        void flushUAVs();

        virtual void _setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly );
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                   uint32 hazardousTexIdx );
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set );
        virtual void _setUavCS( uint32 slotStart, const DescriptorSetUav *set );

        virtual void _setCurrentDeviceFromTexture( TextureGpu *texture );
        virtual MetalFrameBufferDescMap& _getFrameBufferDescMap()   { return mFrameBufferDescMap; }
        virtual RenderPassDescriptor* createRenderPassDescriptor();
        virtual void beginRenderPassDescriptor( RenderPassDescriptor *desc,
                                                TextureGpu *anyTarget, uint8 mipLevel,
                                                const Vector4 *viewportSizes,
                                                const Vector4 *scissors,
                                                uint32 numViewports,
                                                bool overlaysEnabled,
                                                bool warnIfRtvWasFlushed );
        void executeRenderPassDescriptorDelayedActions( bool officialCall );
        virtual void executeRenderPassDescriptorDelayedActions();
        inline void endRenderPassDescriptor( bool isInterruptingRender );
        virtual void endRenderPassDescriptor();
    protected:
        virtual TextureGpu *createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                                  PixelFormatGpu depthBufferFormat, uint16 poolId );

    public:
        virtual void _setTextureCoordCalculation(size_t unit, TexCoordCalcMethod m,
                                                 const Frustum* frustum = 0);
        virtual void _setTextureBlendMode(size_t unit, const LayerBlendModeEx& bm);
        virtual void _setTextureMatrix(size_t unit, const Matrix4& xform);

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer );

        virtual void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso );
        virtual void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso );

        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );

        /// See VaoManager::waitForTailFrameToFinish
        virtual void _waitForTailFrameToFinish();

        virtual void _beginFrameOnce();
        virtual void _endFrameOnce();

        virtual void _beginFrame();
        virtual void _endFrame();

        virtual void _hlmsPipelineStateObjectCreated( HlmsPso *newPso );
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso );
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock );
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block );
    protected:
        template <typename TDescriptorSetTexture,
                  typename TTexSlot,
                  typename TBufferPacked,
                  bool isUav>
        void _descriptorSetTextureCreated( TDescriptorSetTexture *newSet,
                                           const FastArray<TTexSlot> &texContainer,
                                           uint16 *shaderTypeTexCount );
        void destroyMetalDescriptorSetTexture( MetalDescriptorSetTexture *metalSet );
    public:
        virtual void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet );
        virtual void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set );
        virtual void _descriptorSetUavCreated( DescriptorSetUav *newSet );
        virtual void _descriptorSetUavDestroyed( DescriptorSetUav *set );

        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock );
        virtual void _setPipelineStateObject( const HlmsPso *pso );
        virtual void _setComputePso( const HlmsComputePso *pso );

        virtual VertexElementType getColourVertexElementType() const;

        virtual void _dispatch( const HlmsComputePso &pso );

        virtual void _setVertexArrayObject( const VertexArrayObject *vao );

        virtual void _render( const CbDrawCallIndexed *cmd );
        virtual void _render( const CbDrawCallStrip *cmd );
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulated( const CbDrawCallStrip *cmd );

        virtual void _setRenderOperation( const v1::CbRenderOp *cmd );
        virtual void _render( const v1::CbDrawCallIndexed *cmd );
        virtual void _render( const v1::CbDrawCallStrip *cmd );
        virtual void _render( const v1::RenderOperation &op );

        virtual void bindGpuProgramParameters(GpuProgramType gptype,
            GpuProgramParametersSharedPtr params, uint16 variabilityMask);
        virtual void bindGpuProgramPassIterationParameters(GpuProgramType gptype);

        virtual void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                                       uint8 mipLevel );

        virtual Real getHorizontalTexelOffset();
        virtual Real getVerticalTexelOffset();
        virtual Real getMinimumDepthInputValue();
        virtual Real getMaximumDepthInputValue();

        virtual void preExtraThreadsStarted();
        virtual void postExtraThreadsStarted();
        virtual void registerThread();
        virtual void unregisterThread();
        virtual unsigned int getDisplayMonitorCount() const     { return 1; }

        virtual SampleDescription validateSampleDescription( const SampleDescription &sampleDesc,
                                                             PixelFormatGpu format );

        virtual const PixelFormatToShaderType* getPixelFormatToShaderType() const;

        virtual void beginProfileEvent( const String &eventName );
        virtual void endProfileEvent();
        virtual void markProfileEvent( const String &event );

        virtual void initGPUProfiling();
        virtual void deinitGPUProfiling();
        virtual void beginGPUSampleProfile( const String &name, uint32 *hashCache );
        virtual void endGPUSampleProfile( const String &name );

        virtual bool hasAnisotropicMipMapFilter() const         { return true; }

        virtual void setClipPlanesImpl(const PlaneList& clipPlanes);
        virtual void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities* caps,
                                                             Window *primary );
        virtual void updateCompositorManager( CompositorManager2 *compositorManager );
        virtual void compositorWorkspaceBegin( CompositorWorkspace *workspace,
                                               const bool forceBeginFrame );
        virtual void compositorWorkspaceUpdate( CompositorWorkspace *workspace );
        virtual void compositorWorkspaceEnd( CompositorWorkspace *workspace, const bool forceEndFrame );

        virtual void flushCommands();

        MetalDevice* getActiveDevice()                      { return mActiveDevice; }
        MetalProgramFactory* getMetalProgramFactory()       { return mMetalProgramFactory; }

        void _notifyActiveEncoderEnded( bool callEndRenderPassDesc );
        void _notifyActiveComputeEnded();
        void _notifyNewCommandBuffer();
        void _notifyDeviceStalled();


    };
}

#endif
