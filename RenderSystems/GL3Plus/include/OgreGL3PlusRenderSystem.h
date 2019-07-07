/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
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

#include "OgreMaterialManager.h"
#include "OgreRenderSystem.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreDescriptorSetUav.h"
#include "OgreGLSLShader.h"
#include "OgreGL3PlusPixelFormatToShaderType.h"
#include "OgreGL3PlusRenderPassDescriptor.h"

namespace Ogre {
    class GL3PlusContext;
    class GL3PlusSupport;
    class GL3PlusRTTManager;
    class GLSLShaderManager;
    class GLSLShaderFactory;

    namespace v1
    {
        class HardwareBufferManager;
    }

    /**
       Implementation of GL 3 as a rendering system.
    */
    class _OgreGL3PlusExport GL3PlusRenderSystem : public RenderSystem
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

        void initConfigOptions(void);

        /// Store last colour write state
        uint8 mBlendChannelMask;

        /// Store last depth write state
        bool mDepthWrite;

        /// Store last scissor enable state
        bool mScissorsEnabled;

        GLfloat mAutoTextureMatrix[16];

        bool mUseAutoTextureMatrix;

        /// GL support class, used for creating windows etc.
        GL3PlusSupport *mGLSupport;

        /* The main GL context - main thread only */
        GL3PlusContext *mMainContext;

        /* The current GL context  - main thread only */
        GL3PlusContext *mCurrentContext;

        typedef list<GL3PlusContext*>::type GL3PlusContextList;
        /// List of background thread contexts
        GL3PlusContextList mBackgroundContextList;

        /// For rendering legacy objects.
        GLuint  mGlobalVao;
        v1::VertexData  *mCurrentVertexBuffer;
        v1::IndexData   *mCurrentIndexBuffer;
        GLenum          mCurrentPolygonMode;

        GLSLShaderManager *mShaderManager;
        GLSLShaderFactory* mGLSLShaderFactory;
        v1::HardwareBufferManager* mHardwareBufferManager;

        /** Manager object for creating render textures.
            Direct render to texture via FBO is preferable
            to pbuffers, which depend on the GL support used and are generally
            unwieldy and slow. However, FBO support for stencil buffers is poor.
        */
        GL3PlusRTTManager *mRTTManager;

        /** These variables are used for caching RenderSystem state.
            They are cached because OpenGL state changes can be quite expensive,
            which is especially important on mobile or embedded systems.
        */
        GLenum mActiveTextureUnit;
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

        GLint getCombinedMinMipFilter(void) const;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
		/// @copydoc RenderSystem::setDrawBuffer
		virtual bool setDrawBuffer(ColourBufferType colourBuffer);
#endif

        /// @copydoc RenderSystem::checkExtension
        virtual bool checkExtension( const String &ext ) const;

        /// @copydoc RenderSystem::getPixelFormatToShaderType
        virtual const PixelFormatToShaderType* getPixelFormatToShaderType(void) const;

        virtual void _clearStateAndFlushCommandBuffer(void);

        unsigned char *mSwIndirectBufferPtr;

        uint8   mFirstUavBoundSlot;
        uint8   mLastUavBoundPlusOne;

        uint8                   mClipDistances;
        GL3PlusHlmsPso const    *mPso;
        GLSLShader *mCurrentComputeShader;

        GLuint  mNullColourFramebuffer;

        GL3PlusPixelFormatToShaderType mPixelFormatToShaderType;

        FrameBufferDescMap mFrameBufferDescMap;

        GLint getTextureAddressingMode(TextureAddressingMode tam) const;
        static GLenum getBlendMode(SceneBlendFactor ogreBlend);
        static GLenum getBlendOperation( SceneBlendOperation op );

        bool activateGLTextureUnit(size_t unit);
        void bindVertexElementToGpu( const v1::VertexElement &elem,
                                     v1::HardwareVertexBufferSharedPtr vertexBuffer,
                                     const size_t vertexStart,
                                     vector<GLuint>::type &attribsBound,
                                     vector<GLuint>::type &instanceAttribsBound,
                                     bool updateVAO);

        void transformViewportCoords( int x, int &y, int width, int height );

    public:
        // Default constructor / destructor
        GL3PlusRenderSystem();
        ~GL3PlusRenderSystem();

        friend class ShaderGeneratorTechniqueResolverListener;

        // ----------------------------------
        // Overridden RenderSystem functions
        // ----------------------------------
        /** See
            RenderSystem
        */
        const String& getName(void) const;
        /** See
            RenderSystem
        */
            const String& getFriendlyName(void) const;
        /** See
            RenderSystem
        */
        ConfigOptionMap& getConfigOptions(void);
        /** See
            RenderSystem
        */
        void setConfigOption(const String &name, const String &value);
        /** See
            RenderSystem
        */
        String validateConfigOptions(void);
        /** See
            RenderSystem
        */
        Window* _initialise( bool autoCreateWindow, const String& windowTitle = "OGRE Render Window" );
        /** See
            RenderSystem
        */
        virtual RenderSystemCapabilities* createRenderSystemCapabilities() const;
        /** See
            RenderSystem
        */
        void initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, Window* primary);
        /** See
            RenderSystem
        */
        void reinitialise(void); // Used if settings changed mid-rendering
        /** See
            RenderSystem
        */
        void shutdown(void);

        /// @copydoc RenderSystem::_createRenderWindow
        Window *_createRenderWindow( const String &name, uint32 width, uint32 height,
                                     bool fullScreen, const NameValuePairList *miscParams = 0 );

        /// @copydoc RenderSystem::_createRenderWindows
        bool _createRenderWindows( const RenderWindowDescriptionList &renderWindowDescriptions,
                                   WindowList &createdWindows );

        /// @copydoc RenderSystem::_createDepthBufferFor
        DepthBuffer* _createDepthBufferFor( RenderTarget *renderTarget, bool exactMatchFormat );

        /// Mimics D3D9RenderSystem::_getDepthStencilFormatFor, if no FBO RTT manager, outputs GL_NONE
        void _getDepthStencilFormatFor( GLenum internalColourFormat, GLenum *depthFormat,
                                        GLenum *stencilFormat );

        virtual void _setCurrentDeviceFromTexture( TextureGpu *texture );
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

        /** See
            RenderSystem
        */
        String getErrorDescription(long errorNumber) const;
        /** See
            RenderSystem
        */
        VertexElementType getColourVertexElementType(void) const;

        // -----------------------------
        // Low-level overridden members
        // -----------------------------
        /** See
            RenderSystem
        */
        void _useLights(const LightList& lights, unsigned short limit) { };   // Not supported
        /** See
            RenderSystem
        */
        bool areFixedFunctionLightsInViewSpace() const { return true; }
        /** See
            RenderSystem
        */
        void _setWorldMatrix(const Matrix4 &m);
        /** See
            RenderSystem
        */
        void _setViewMatrix(const Matrix4 &m);
        /** See
            RenderSystem
        */
        void _setProjectionMatrix(const Matrix4 &m);
        /** See
            RenderSystem
        */
        void _setSurfaceParams(const ColourValue &ambient,
                               const ColourValue &diffuse, const ColourValue &specular,
                               const ColourValue &emissive, Real shininess,
                               TrackVertexColourType tracking) {}
        /** See
            RenderSystem
        */
        void _setPointParameters(Real size, bool attenuationEnabled,
                                 Real constant, Real linear, Real quadratic, Real minSize, Real maxSize);
        /** See
            RenderSystem
        */
        void _setPointSpritesEnabled(bool enabled);
        /** See
         RenderSystem
         */
        void _setVertexTexture(size_t unit, TextureGpu *tex);
        /** See
         RenderSystem
         */
        void _setGeometryTexture(size_t unit, TextureGpu *tex);
        /** See
         RenderSystem
         */
        void _setTessellationHullTexture(size_t unit, TextureGpu *tex);
        /** See
         RenderSystem
         */
        void _setTessellationDomainTexture(size_t unit, TextureGpu *tex);
        /** See
            RenderSystem
        */
        void _setTexture( size_t unit, TextureGpu *tex );
        /// See RenderSystem
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                   uint32 hazardousTexIdx );
        virtual void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set );
        virtual void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set );
        virtual void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set );
    protected:
        virtual void setBufferUavCS( uint32 slot, const DescriptorSetUav::BufferSlot &bufferSlot );
        virtual void setTextureUavCS( uint32 slot, const DescriptorSetUav::TextureSlot &texSlot,
                                      GLuint srvView );
    public:
        virtual void _setUavCS( uint32 slotStart, const DescriptorSetUav *set );
        /** See
            RenderSystem
        */
        void _setTextureCoordCalculation(size_t stage, TexCoordCalcMethod m,
                                         const Frustum* frustum = 0) { };   // Not supported
        /** See
            RenderSystem
        */
        void _setTextureBlendMode(size_t stage, const LayerBlendModeEx& bm) { };   // Not supported
        /** See
            RenderSystem
        */
        void _setTextureMatrix(size_t stage, const Matrix4& xform) { };   // Not supported

        virtual void flushUAVs(void);

        /** See
            RenderSystem
        */
        void _setViewport(Viewport *vp);
        virtual void _resourceTransitionCreated( ResourceTransition *resTransition );
        virtual void _resourceTransitionDestroyed( ResourceTransition *resTransition );
        virtual void _executeResourceTransition( ResourceTransition *resTransition );

        virtual void _hlmsPipelineStateObjectCreated( HlmsPso *newPso );
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso );
        virtual void _hlmsMacroblockCreated( HlmsMacroblock *newBlock );
        virtual void _hlmsMacroblockDestroyed( HlmsMacroblock *block );
        virtual void _hlmsBlendblockCreated( HlmsBlendblock *newBlock );
        virtual void _hlmsBlendblockDestroyed( HlmsBlendblock *block );
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock );
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block );
        virtual void _descriptorSetTexture2Created( DescriptorSetTexture2 *newSet );
        virtual void _descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set );
        virtual void _descriptorSetUavCreated( DescriptorSetUav *newSet );
        virtual void _descriptorSetUavDestroyed( DescriptorSetUav *set );
        void _setHlmsMacroblock( const HlmsMacroblock *macroblock, const GL3PlusHlmsPso *pso );
        void _setHlmsBlendblock( const HlmsBlendblock *blendblock, const GL3PlusHlmsPso *pso );
        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock );
        virtual void _setPipelineStateObject( const HlmsPso *pso );

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer );

        virtual void _hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso );
        virtual void _hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso );
        virtual void _setComputePso( const HlmsComputePso *pso );
        /** See
            RenderSystem
        */
        void _beginFrame(void);
        /** See
            RenderSystem
        */
        void _endFrame(void);
        /** See
            RenderSystem
        */
        void _setDepthBias(float constantBias, float slopeScaleBias);
        virtual void _makeRsProjectionMatrix( const Matrix4& matrix,
                                              Matrix4& dest, Real nearPlane,
                                              Real farPlane, ProjectionType projectionType );
        /** See
            RenderSystem
        */
        virtual void _convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest);
        virtual Real getRSDepthRange(void) const;
        /** See
            RenderSystem
        */
        void setClipPlane (ushort index, Real A, Real B, Real C, Real D);
        /** See
            RenderSystem
        */
        void enableClipPlane (ushort index, bool enable);
        /** See
            RenderSystem.
        */
        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );
        /** See
            RenderSystem
        */
        void _render(const v1::RenderOperation& op);

        virtual void _dispatch( const HlmsComputePso &pso );

        virtual void _setVertexArrayObject( const VertexArrayObject *vao );
        virtual void _render( const CbDrawCallIndexed *cmd );
        virtual void _render( const CbDrawCallStrip *cmd );
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulated( const CbDrawCallStrip *cmd );
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd );

        virtual void _startLegacyV1Rendering(void);
        virtual void _setRenderOperation( const v1::CbRenderOp *cmd );
        virtual void _render( const v1::CbDrawCallIndexed *cmd );
        virtual void _render( const v1::CbDrawCallStrip *cmd );
        virtual void _renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd );
        virtual void _renderNoBaseInstance( const v1::CbDrawCallStrip *cmd );

        virtual void clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                       TextureGpu *anyTarget, uint8 mipLevel );
        virtual void discardFrameBuffer( unsigned int buffers );
        HardwareOcclusionQuery* createHardwareOcclusionQuery(void);
        Real getHorizontalTexelOffset(void) { return 0.0; }               // No offset in GL
        Real getVerticalTexelOffset(void) { return 0.0; }                 // No offset in GL
        Real getMinimumDepthInputValue(void);
        Real getMaximumDepthInputValue(void);
        OGRE_MUTEX(mThreadInitMutex);
        void registerThread();
        void unregisterThread();
        void preExtraThreadsStarted();
        void postExtraThreadsStarted();
        void setClipPlanesImpl(const Ogre::PlaneList& planeList);

        // ----------------------------------
        // GL3PlusRenderSystem specific members
        // ----------------------------------
        /** Returns the main context */
        GL3PlusContext* _getMainContext() { return mMainContext; }
        /** Unregister a render target->context mapping. If the context of target
            is the current context, change the context to the main context so it
            can be destroyed safely.

            @note This is automatically called by the destructor of
            GL3PlusContext.
        */
        void _unregisterContext(GL3PlusContext *context);
        /** Switch GL context, dealing with involved internal cached states too
         */
        void _switchContext(GL3PlusContext *context);
        /** One time initialization for the RenderState of a context. Things that
            only need to be set once, like the LightingModel can be defined here.
        */
        void _oneTimeContextInitialization();
        void initialiseContext( Window *primary );
        /**
         * Set current render target to target, enabling its GL context if needed
         */
        void _setRenderTarget( RenderTarget *target, uint8 viewportRenderTargetFlags );

        GLint convertCompareFunction(CompareFunction func) const;
        GLint convertStencilOp(StencilOperation op) const;

        const GL3PlusSupport* getGLSupport(void) const { return mGLSupport; }

        void bindGpuProgramParameters(GpuProgramType gptype, GpuProgramParametersSharedPtr params, uint16 mask);
        void bindGpuProgramPassIterationParameters(GpuProgramType gptype);

        /// @copydoc RenderSystem::_setSceneBlending
        void _setSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor, SceneBlendOperation op );
        /// @copydoc RenderSystem::_setSeparateSceneBlending
        void _setSeparateSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor, SceneBlendFactor sourceFactorAlpha, SceneBlendFactor destFactorAlpha, SceneBlendOperation op, SceneBlendOperation alphaOp );
        /// @copydoc RenderSystem::getDisplayMonitorCount
        unsigned int getDisplayMonitorCount() const;

        void _setSceneBlendingOperation(SceneBlendOperation op);
        void _setSeparateSceneBlendingOperation(SceneBlendOperation op, SceneBlendOperation alphaOp);
        /// @copydoc RenderSystem::hasAnisotropicMipMapFilter
        virtual bool hasAnisotropicMipMapFilter() const { return false; }

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
    };
}

#endif
