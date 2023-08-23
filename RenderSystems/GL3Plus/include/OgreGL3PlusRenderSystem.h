/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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

#ifndef __GL3PlusRenderSystem_H__
#define __GL3PlusRenderSystem_H__

#include "OgreGL3PlusPrerequisites.h"

#include "OgreDescriptorSetUav.h"
#include "OgreGL3PlusPixelFormatToShaderType.h"
#include "OgreGL3PlusRenderPassDescriptor.h"
#include "OgreGLSLShader.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreMaterialManager.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    class GL3PlusContext;
    class GL3PlusSupport;
    class GLSLShaderManager;
    class GLSLShaderFactory;

    namespace v1
    {
        class HardwareBufferManager;
    }

    /**
       Implementation of GL 3 as a rendering system.
    */
    class _OgreGL3PlusExport GL3PlusRenderSystem final : public RenderSystem
    {
    private:
        /// Rendering loop control
        bool mStopRendering;

        typedef unordered_map<GLenum, GLuint>::type BindBufferMap;

        /// View matrix to set world against
        Matrix4 mViewMatrix;
        Matrix4 mWorldMatrix;
        Matrix4 mTextureMatrix;

        /// Last min & mip filtering options, so we can combine them
        FilterOptions mMinFilter;
        FilterOptions mMipFilter;

        bool mTextureCompareEnabled;

        /** Used to store the number of mipmaps in the currently bound texture.  This is then
            used to modify the texture unit filtering.
        */
        size_t mTextureMipmapCount;

        /// Holds texture type settings for every stage
        GLenum mTextureTypes[OGRE_MAX_TEXTURE_LAYERS];

        GLfloat mLargestSupportedAnisotropy;

        /// Number of fixed-function texture units
        unsigned short mFixedFunctionTextureUnits;

        void initConfigOptions();

        /// Store last colour write state
        uint8 mBlendChannelMask;

        /// Store last depth write state
        bool mDepthWrite;

        /// Store last scissor enable state
        bool mScissorsEnabled;

        GLfloat mAutoTextureMatrix[16];

        bool mUseAutoTextureMatrix;

        bool mSupportsTargetIndependentRasterization;

        /// GL support class, used for creating windows etc.
        GL3PlusSupport *mGLSupport;

        /* The main GL context - main thread only */
        GL3PlusContext *mMainContext;

        /* The current GL context  - main thread only */
        GL3PlusContext *mCurrentContext;

        typedef list<GL3PlusContext *>::type GL3PlusContextList;
        /// List of background thread contexts
        GL3PlusContextList mBackgroundContextList;

        /// For rendering legacy objects.
        GLuint          mGlobalVao;
        v1::VertexData *mCurrentVertexBuffer;
        v1::IndexData  *mCurrentIndexBuffer;
        GLenum          mCurrentPolygonMode;

        GLSLShaderManager         *mShaderManager;
        GLSLShaderFactory         *mGLSLShaderFactory;
        v1::HardwareBufferManager *mHardwareBufferManager;

        /** These variables are used for caching RenderSystem state.
            They are cached because OpenGL state changes can be quite expensive,
            which is especially important on mobile or embedded systems.
        */
        GLenum        mActiveTextureUnit;
        BindBufferMap mActiveBufferMap;

        /// Check if the GL system has already been initialised
        bool mGLInitialised;
        bool mUseAdjacency;

        // check if GL 4.3 is supported
        bool mHasGL43;

        bool mHasArbInvalidateSubdata;

        // local data members of _render that were moved here to improve performance
        // (save allocations)
        vector<GLuint>::type mRenderAttribsBound;
        vector<GLuint>::type mRenderInstanceAttribsBound;

        GLint getCombinedMinMipFilter() const;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        /// @copydoc RenderSystem::setDrawBuffer
        bool setDrawBuffer( ColourBufferType colourBuffer ) override;
#endif

        /// @copydoc RenderSystem::checkExtension
        bool checkExtension( const String &ext ) const override;

        /// @copydoc RenderSystem::getPixelFormatToShaderType
        const PixelFormatToShaderType *getPixelFormatToShaderType() const override;

        void _clearStateAndFlushCommandBuffer() override;

        void flushCommands() override;

        unsigned char *mSwIndirectBufferPtr;

        uint8 mFirstUavBoundSlot;
        uint8 mLastUavBoundPlusOne;

        uint8                 mClipDistances;
        GL3PlusHlmsPso const *mPso;
        GLSLShader           *mCurrentComputeShader;

        GLuint mNullColourFramebuffer;

        GL3PlusPixelFormatToShaderType mPixelFormatToShaderType;

        GL3PlusFrameBufferDescMap mFrameBufferDescMap;

        GLint         getTextureAddressingMode( TextureAddressingMode tam ) const;
        static GLenum getBlendMode( SceneBlendFactor ogreBlend );
        static GLenum getBlendOperation( SceneBlendOperation op );

        bool activateGLTextureUnit( size_t unit );
        void bindVertexElementToGpu( const v1::VertexElement          &elem,
                                     v1::HardwareVertexBufferSharedPtr vertexBuffer,
                                     const size_t vertexStart, vector<GLuint>::type &attribsBound,
                                     vector<GLuint>::type &instanceAttribsBound, bool updateVAO );

        void transformViewportCoords( int x, int &y, int width, int height );

    public:
        // Default constructor / destructor
        GL3PlusRenderSystem( const NameValuePairList *options );
        ~GL3PlusRenderSystem() override;

        friend class ShaderGeneratorTechniqueResolverListener;

        // ----------------------------------
        // Overridden RenderSystem functions
        // ----------------------------------
        /** See
            RenderSystem
        */
        const String &getName() const override;
        /** See
            RenderSystem
        */
        const String &getFriendlyName() const override;
        /** See
            RenderSystem
        */
        ConfigOptionMap &getConfigOptions() override;
        /** See
            RenderSystem
        */
        void setConfigOption( const String &name, const String &value ) override;

        const char *getPriorityConfigOption( size_t idx ) const override;

        size_t getNumPriorityConfigOptions() const override;

        /** See
            RenderSystem
        */
        String validateConfigOptions() override;
        /** See
            RenderSystem
        */
        Window *_initialise( bool          autoCreateWindow,
                             const String &windowTitle = "OGRE Render Window" ) override;
        /** See
            RenderSystem
        */
        RenderSystemCapabilities *createRenderSystemCapabilities() const override;
        /** See
            RenderSystem
        */
        void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                     Window                   *primary ) override;
        /** See
            RenderSystem
        */
        void reinitialise() override;  // Used if settings changed mid-rendering
        /** See
            RenderSystem
        */
        void shutdown() override;

        /// @copydoc RenderSystem::_createRenderWindow
        Window *_createRenderWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                     const NameValuePairList *miscParams = 0 ) override;

        /// @copydoc RenderSystem::_createRenderWindows
        bool _createRenderWindows( const RenderWindowDescriptionList &renderWindowDescriptions,
                                   WindowList                        &createdWindows ) override;

        void                       _setCurrentDeviceFromTexture( TextureGpu *texture ) override;
        GL3PlusFrameBufferDescMap &_getFrameBufferDescMap() { return mFrameBufferDescMap; }
        RenderPassDescriptor      *createRenderPassDescriptor() override;
        void beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                        uint8 mipLevel, const Vector4 *viewportSizes,
                                        const Vector4 *scissors, uint32 numViewports,
                                        bool overlaysEnabled, bool warnIfRtvWasFlushed ) override;
        void endRenderPassDescriptor() override;

        TextureGpu *createDepthBufferFor( TextureGpu *colourTexture, bool preferDepthTexture,
                                          PixelFormatGpu depthBufferFormat, uint16 poolId ) override;

        /** See
            RenderSystem
        */
        String getErrorDescription( long errorNumber ) const override;
        /** See
            RenderSystem
        */
        VertexElementType getColourVertexElementType() const override;

        // -----------------------------
        // Low-level overridden members
        // -----------------------------
        /** See
            RenderSystem
        */
        void _useLights( const LightList & /*lights*/, unsigned short /*limit*/ ) override {
        }  // Not supported
        /** See
            RenderSystem
        */
        bool areFixedFunctionLightsInViewSpace() const override { return true; }
        /** See
            RenderSystem
        */
        void _setWorldMatrix( const Matrix4 &m ) override;
        /** See
            RenderSystem
        */
        void _setViewMatrix( const Matrix4 &m ) override;
        /** See
            RenderSystem
        */
        void _setProjectionMatrix( const Matrix4 &m ) override;
        /** See
            RenderSystem
        */
        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                const ColourValue &specular, const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking ) override
        {
        }
        /** See
            RenderSystem
        */
        void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                  Real quadratic, Real minSize, Real maxSize ) override;
        /** See
            RenderSystem
        */
        void _setPointSpritesEnabled( bool enabled ) override;
        /** See
         RenderSystem
         */
        void _setVertexTexture( size_t unit, TextureGpu *tex ) override;
        /** See
         RenderSystem
         */
        void _setGeometryTexture( size_t unit, TextureGpu *tex ) override;
        /** See
         RenderSystem
         */
        void _setTessellationHullTexture( size_t unit, TextureGpu *tex ) override;
        /** See
         RenderSystem
         */
        void _setTessellationDomainTexture( size_t unit, TextureGpu *tex ) override;
        /** See
            RenderSystem
        */
        void _setTexture( size_t unit, TextureGpu *tex, bool bDepthReadOnly ) override;
        /// See RenderSystem
        void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                           uint32 hazardousTexIdx ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) override;

    protected:
        void setBufferUavCS( uint32 slot, const DescriptorSetUav::BufferSlot &bufferSlot );
        void setTextureUavCS( uint32 slot, const DescriptorSetUav::TextureSlot &texSlot,
                              GLuint srvView );

    public:
        void _setUavCS( uint32 slotStart, const DescriptorSetUav *set ) override;
        /** See
            RenderSystem
        */
        void _setTextureCoordCalculation( size_t stage, TexCoordCalcMethod m,
                                          const Frustum *frustum = 0 ) override
        {
        }  // Not supported
        /** See
            RenderSystem
        */
        void _setTextureBlendMode( size_t stage, const LayerBlendModeEx &bm ) override {
        }  // Not supported
        /** See
            RenderSystem
        */
        void _setTextureMatrix( size_t stage, const Matrix4 &xform ) override{};  // Not supported

        void flushUAVs();

        void executeResourceTransition( const ResourceTransitionArray &rstCollection ) override;

        void _hlmsPipelineStateObjectCreated( HlmsPso *newPso ) override;
        void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso ) override;
        void _hlmsMacroblockCreated( HlmsMacroblock *newBlock ) override;
        void _hlmsMacroblockDestroyed( HlmsMacroblock *block ) override;
        void _hlmsBlendblockCreated( HlmsBlendblock *newBlock ) override;
        void _hlmsBlendblockDestroyed( HlmsBlendblock *block ) override;
        void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock ) override;
        void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block ) override;
        void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet ) override;
        void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set ) override;
        void _descriptorSetUavCreated( DescriptorSetUav *newSet ) override;
        void _descriptorSetUavDestroyed( DescriptorSetUav *set ) override;
        void _setHlmsMacroblock( const HlmsMacroblock *macroblock, const GL3PlusHlmsPso *pso );
        void _setHlmsBlendblock( const HlmsBlendblock *blendblock, const GL3PlusHlmsPso *pso,
                                 bool bIsMultisample );
        void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock ) override;
        void _setPipelineStateObject( const HlmsPso *pso ) override;

        void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) override;

        void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso ) override;
        void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso ) override;
        void _setComputePso( const HlmsComputePso *pso ) override;
        /** See
            RenderSystem
        */
        void _beginFrame() override;
        /** See
            RenderSystem
        */
        void _endFrame() override;
        /** See
            RenderSystem
        */
        void _setDepthBias( float constantBias, float slopeScaleBias );
        void _makeRsProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest, Real nearPlane,
                                      Real farPlane, ProjectionType projectionType ) override;
        /** See
            RenderSystem
        */
        void _convertProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest ) override;
        void _convertOpenVrProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest ) override;
        Real getRSDepthRange() const override;
        /** See
            RenderSystem
        */
        void setClipPlane( ushort index, Real A, Real B, Real C, Real D );
        /** See
            RenderSystem
        */
        void enableClipPlane( ushort index, bool enable );
        /** See
            RenderSystem.
        */
        void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams ) override;
        /** See
            RenderSystem
        */
        void _render( const v1::RenderOperation &op ) override;

        void _dispatch( const HlmsComputePso &pso ) override;

        void _setVertexArrayObject( const VertexArrayObject *vao ) override;
        void _render( const CbDrawCallIndexed *cmd ) override;
        void _render( const CbDrawCallStrip *cmd ) override;
        void _renderEmulated( const CbDrawCallIndexed *cmd ) override;
        void _renderEmulated( const CbDrawCallStrip *cmd ) override;
        void _renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd ) override;
        void _renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd ) override;

        void _startLegacyV1Rendering() override;
        void _setRenderOperation( const v1::CbRenderOp *cmd ) override;
        void _render( const v1::CbDrawCallIndexed *cmd ) override;
        void _render( const v1::CbDrawCallStrip *cmd ) override;
        void _renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd ) override;
        void _renderNoBaseInstance( const v1::CbDrawCallStrip *cmd ) override;

        void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                               uint8 mipLevel ) override;
        HardwareOcclusionQuery *createHardwareOcclusionQuery() override;
        Real                    getHorizontalTexelOffset() override { return 0.0; }  // No offset in GL
        Real                    getVerticalTexelOffset() override { return 0.0; }    // No offset in GL
        Real                    getMinimumDepthInputValue() override;
        Real                    getMaximumDepthInputValue() override;
        OGRE_MUTEX( mThreadInitMutex );
        void registerThread() override;
        void unregisterThread() override;
        void preExtraThreadsStarted() override;
        void postExtraThreadsStarted() override;
        void setClipPlanesImpl( const Ogre::PlaneList &planeList ) override;

        // ----------------------------------
        // GL3PlusRenderSystem specific members
        // ----------------------------------
        /** Returns the main context */
        GL3PlusContext *_getMainContext() { return mMainContext; }
        /** Unregister a render target->context mapping. If the context of target
            is the current context, change the context to the main context so it
            can be destroyed safely.

            @note This is automatically called by the destructor of
            GL3PlusContext.
        */
        void _unregisterContext( GL3PlusContext *context );
        /** Switch GL context, dealing with involved internal cached states too
         */
        void _switchContext( GL3PlusContext *context );
        /** One time initialization for the RenderState of a context. Things that
            only need to be set once, like the LightingModel can be defined here.
        */
        void _oneTimeContextInitialization();
        void initialiseContext( Window *primary );

        GLenum convertCompareFunction( CompareFunction func ) const;
        GLenum convertStencilOp( StencilOperation op ) const;

        bool supportsTargetIndependentRasterization() const
        {
            return mSupportsTargetIndependentRasterization;
        }

        const GL3PlusSupport *getGLSupport() const { return mGLSupport; }

        void bindGpuProgramParameters( GpuProgramType gptype, GpuProgramParametersSharedPtr params,
                                       uint16 mask ) override;
        void bindGpuProgramPassIterationParameters( GpuProgramType gptype ) override;

        void _setSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
                                SceneBlendOperation op );
        void _setSeparateSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
                                        SceneBlendFactor sourceFactorAlpha,
                                        SceneBlendFactor destFactorAlpha, SceneBlendOperation op,
                                        SceneBlendOperation alphaOp );
        /// @copydoc RenderSystem::getDisplayMonitorCount
        unsigned int getDisplayMonitorCount() const override;

        void _setSceneBlendingOperation( SceneBlendOperation op );
        void _setSeparateSceneBlendingOperation( SceneBlendOperation op, SceneBlendOperation alphaOp );
        /// @copydoc RenderSystem::hasAnisotropicMipMapFilter
        bool hasAnisotropicMipMapFilter() const override { return false; }

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
    };
}  // namespace Ogre

#endif
