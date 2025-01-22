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

#ifndef __GLES2RenderSystem_H__
#define __GLES2RenderSystem_H__

#include "OgreGLES2Prerequisites.h"

#include "OgreGLES2PixelFormatToShaderType.h"
#include "OgreHlmsSamplerblock.h"
#include "OgreMaterialManager.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    class GLES2Context;
    class GLES2Support;
    class GLES2RTTManager;
    class GLES2GpuProgramManager;
    class GLSLESShaderFactory;
    namespace v1
    {
        class HardwareBufferManager;
    }
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    class GLES2ManagedResourceManager;
#endif

    /**
      Implementation of GL ES 2.x as a rendering system.
     */
    class _OgreGLES2Export GLES2RenderSystem : public RenderSystem
    {
    private:
        /// View matrix to set world against
        Matrix4 mViewMatrix;
        Matrix4 mWorldMatrix;
        Matrix4 mTextureMatrix;

        /// Last min & mip filtering options, so we can combine them
        FilterOptions mMinFilter;
        FilterOptions mMipFilter;

        /// Holds texture type settings for every stage
        GLenum mTextureTypes[OGRE_MAX_TEXTURE_LAYERS];

        GLfloat mLargestSupportedAnisotropy;

        /// Number of fixed-function texture units
        unsigned short mFixedFunctionTextureUnits;

        /// Store last colour write state
        uint8 mBlendChannelMask;

        /// Store last depth write state
        bool mDepthWrite;

        /// Store last scissor enable state
        bool mScissorsEnabled;

        GLfloat mAutoTextureMatrix[16];

        bool mUseAutoTextureMatrix;

        /// GL support class, used for creating windows etc.
        GLES2Support *mGLSupport;

        /* The main GL context - main thread only */
        GLES2Context *mMainContext;

        /* The current GL context  - main thread only */
        GLES2Context *mCurrentContext;

        typedef list<GLES2Context *>::type GLES2ContextList;
        /// List of background thread contexts
        GLES2ContextList mBackgroundContextList;

        /// For rendering legacy objects.
        GLuint mGlobalVao;
        v1::VertexData *mCurrentVertexBuffer;
        v1::IndexData *mCurrentIndexBuffer;
        GLenum mCurrentPolygonMode;

        GLES2GpuProgramManager *mGpuProgramManager;
        GLSLESShaderFactory *mGLSLESShaderFactory;
        v1::HardwareBufferManager *mHardwareBufferManager;

        /** Manager object for creating render textures.
            Direct render to texture via GL_OES_framebuffer_object is preferable
            to pbuffers, which depend on the GL support used and are generally
            unwieldy and slow. However, FBO support for stencil buffers is poor.
          */
        GLES2RTTManager *mRTTManager;

        /** These variables are used for caching RenderSystem state.
            They are cached because OpenGL state changes can be quite expensive,
            which is especially important on mobile or embedded systems.
        */
        GLenum mActiveTextureUnit;

        /// Check if the GL system has already been initialised
        bool mGLInitialised;
        bool mUseAdjacency;

        bool mHasDiscardFramebuffer;

        // local data member of _render that were moved here to improve performance
        // (save allocations)
        vector<GLuint>::type mRenderAttribsBound;
        vector<GLuint>::type mRenderInstanceAttribsBound;

        GLint getCombinedMinMipFilter() const;

        /// @copydoc RenderSystem::getPixelFormatToShaderType
        virtual const PixelFormatToShaderType *getPixelFormatToShaderType() const;

        unsigned char *mSwIndirectBufferPtr;

        GLES2HlmsPso const *mPso;

        GLuint mNullColourFramebuffer;

        GLES2PixelFormatToShaderType mPixelFormatToShaderType;

        GLint getTextureAddressingMode( TextureAddressingMode tam ) const;
        GLenum getBlendMode( SceneBlendFactor ogreBlend ) const;
        GLenum getBlendOperation( SceneBlendOperation op ) const;

        bool activateGLTextureUnit( size_t unit );
        void bindVertexElementToGpu( const v1::VertexElement &elem,
                                     v1::HardwareVertexBufferSharedPtr vertexBuffer,
                                     const size_t vertexStart, vector<GLuint>::type &attribsBound,
                                     vector<GLuint>::type &instanceAttribsBound, bool updateVAO );

        void correctViewport( GLint x, GLint &y, GLint &w, GLint &h, RenderTarget *renderTarget );

        // Mipmap count of the actual bounded texture
        size_t mCurTexMipCount;

    public:
        // Default constructor / destructor
        GLES2RenderSystem();
        virtual ~GLES2RenderSystem();

        friend class ShaderGeneratorTechniqueResolverListener;

        // ----------------------------------
        // Overridden RenderSystem functions
        // ----------------------------------
        /** See
          RenderSystem
         */
        const String &getName() const;
        /** See
          RenderSystem
         */
        const String &getFriendlyName() const;
        /** See
          RenderSystem
         */
        ConfigOptionMap &getConfigOptions();
        /** See
          RenderSystem
         */
        void setConfigOption( const String &name, const String &value );
        /** See
          RenderSystem
         */
        String validateConfigOptions();
        /** See
          RenderSystem
         */
        RenderWindow *_initialise( bool autoCreateWindow,
                                   const String &windowTitle = "OGRE Render Window" );
        /** See
          RenderSystem
         */
        virtual RenderSystemCapabilities *createRenderSystemCapabilities() const;
        /** See
          RenderSystem
         */
        void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                     RenderTarget *primary );
        /** See
          RenderSystem
         */
        void reinitialise();  // Used if settings changed mid-rendering
        /** See
          RenderSystem
         */
        void shutdown();

        /// @copydoc RenderSystem::_createRenderWindow
        RenderWindow *_createRenderWindow( const String &name, unsigned int width, unsigned int height,
                                           bool fullScreen, const NameValuePairList *miscParams = 0 );

        DepthBuffer *_createDepthBufferFor( RenderTarget *renderTarget, bool exactMatchFormat );

        /// Mimics D3D9RenderSystem::_getDepthStencilFormatFor, if no FBO RTT manager, outputs GL_NONE
        void _getDepthStencilFormatFor( GLenum internalColourFormat, GLenum *depthFormat,
                                        GLenum *stencilFormat );

        virtual MultiRenderTarget *createMultiRenderTarget( const String &name );

        /** See
          RenderSystem
         */
        void destroyRenderWindow( RenderWindow *pWin );
        /** See
          RenderSystem
         */
        String getErrorDescription( long errorNumber ) const;
        /** See
          RenderSystem
         */
        VertexElementType getColourVertexElementType() const;

        // -----------------------------
        // Low-level overridden members
        // -----------------------------
        /** See
         RenderSystem
         */
        void _useLights( const LightList &lights, unsigned short limit ){};  // Not supported
        /** See
         RenderSystem
         */
        bool areFixedFunctionLightsInViewSpace() const { return true; }
        /** See
         RenderSystem
         */
        void _setWorldMatrix( const Matrix4 &m );
        /** See
         RenderSystem
         */
        void _setViewMatrix( const Matrix4 &m );
        /** See
         RenderSystem
         */
        void _setProjectionMatrix( const Matrix4 &m );
        /** See
         RenderSystem
         */
        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                const ColourValue &specular, const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking )
        {
        }
        /** See
         RenderSystem
         */
        void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                  Real quadratic, Real minSize, Real maxSize )
        {
        }
        /** See
         RenderSystem
         */
        void _setPointSpritesEnabled( bool enabled ) {}
        /** See
         RenderSystem
         */
        void _setTexture( size_t unit, bool enabled, Texture *tex, bool bDepthReadOnly );
        /** See
         RenderSystem
         */
        void _setTextureCoordCalculation( size_t stage, TexCoordCalcMethod m,
                                          const Frustum *frustum = 0 ){};  // Not supported
        /** See
         RenderSystem
         */
        void _setTextureBlendMode( size_t stage, const LayerBlendModeEx &bm ){};  // Not supported
        /** See
         RenderSystem
         */
        void _setTextureMatrix( size_t stage, const Matrix4 &xform ){};  // Not supported

        virtual void queueBindUAV( uint32 slot, TexturePtr texture,
                                   ResourceAccess::ResourceAccess access = ResourceAccess::ReadWrite,
                                   int32 mipmapLevel = 0, int32 textureArrayIndex = 0,
                                   PixelFormat pixelFormat = PF_UNKNOWN );
        virtual void queueBindUAV( uint32 slot, UavBufferPacked *buffer,
                                   ResourceAccess::ResourceAccess access = ResourceAccess::ReadWrite,
                                   size_t offset = 0, size_t sizeBytes = 0 );

        virtual void clearUAVs();

        virtual void _bindTextureUavCS( uint32 slot, Texture *texture,
                                        ResourceAccess::ResourceAccess access, int32 mipmapLevel,
                                        int32 textureArrayIndex, PixelFormat pixelFormat );
        virtual void _setTextureCS( uint32 slot, bool enabled, Texture *texPtr );
        virtual void _setHlmsSamplerblockCS( uint8 texUnit, const HlmsSamplerblock *samplerblock );

        /** See
         RenderSystem
         */
        void _setViewport( Viewport *vp );

        virtual bool _hlmsPipelineStateObjectCreated( HlmsPso *newPso, uint64 deadline );
        virtual void _hlmsPipelineStateObjectDestroyed( HlmsPso *pso );
        virtual void _hlmsMacroblockCreated( HlmsMacroblock *newBlock );
        virtual void _hlmsMacroblockDestroyed( HlmsMacroblock *block );
        virtual void _hlmsBlendblockCreated( HlmsBlendblock *newBlock );
        virtual void _hlmsBlendblockDestroyed( HlmsBlendblock *block );
        virtual void _hlmsSamplerblockCreated( HlmsSamplerblock *newBlock );
        virtual void _hlmsSamplerblockDestroyed( HlmsSamplerblock *block );
        void _setHlmsMacroblock( const HlmsMacroblock *macroblock, const GLES2HlmsPso *pso );
        void _setHlmsBlendblock( const HlmsBlendblock *blendblock, const GLES2HlmsPso *pso );
        virtual void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock );
        virtual void _setPipelineStateObject( const HlmsPso *pso );

        virtual void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer );
        virtual void _setComputePso( const HlmsComputePso *pso );

        /** See
         RenderSystem
         */
        void _beginFrame();
        /** See
         RenderSystem
         */
        void _endFrame();
        /** See
         RenderSystem
         */
        void _setDepthBias( float constantBias, float slopeScaleBias );
        /** See
         RenderSystem
         */
        void _setDepthBufferCheckEnabled( bool enabled = true );
        /** See
         RenderSystem
         */
        void _setDepthBufferWriteEnabled( bool enabled = true );
        /** See
         RenderSystem
         */
        void _makeProjectionMatrix( const Radian &fovy, Real aspect, Real nearPlane, Real farPlane,
                                    Matrix4 &dest, bool forGpuProgram = false );
        /** See
         RenderSystem
         */
        void _makeProjectionMatrix( Real left, Real right, Real bottom, Real top, Real nearPlane,
                                    Real farPlane, Matrix4 &dest, bool forGpuProgram = false );
        /** See
         RenderSystem
         */
        void _makeOrthoMatrix( const Radian &fovy, Real aspect, Real nearPlane, Real farPlane,
                               Matrix4 &dest, bool forGpuProgram = false );
        /** See
         RenderSystem
         */
        void _applyObliqueDepthProjection( Matrix4 &matrix, const Plane &plane, bool forGpuProgram );
        /** See
         RenderSystem
         */
        void setClipPlane( ushort index, Real A, Real B, Real C, Real D );
        /** See
         RenderSystem
         */
        void enableClipPlane( ushort index, bool enable );
        /** See
         RenderSystem
         */
        virtual void setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams );
        /** See
         RenderSystem
         */
        virtual bool hasAnisotropicMipMapFilter() const { return false; }
        /** See
         RenderSystem
         */
        void _render( const v1::RenderOperation &op );

        virtual void _dispatch( const HlmsComputePso &pso );

        virtual void _setVertexArrayObject( const VertexArrayObject *vao );
        virtual void _render( const CbDrawCallIndexed *cmd );
        virtual void _render( const CbDrawCallStrip *cmd );
        virtual void _renderEmulated( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulated( const CbDrawCallStrip *cmd );
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd );
        virtual void _renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd );

        virtual void _startLegacyV1Rendering();
        virtual void _setRenderOperation( const v1::CbRenderOp *cmd );
        virtual void _render( const v1::CbDrawCallIndexed *cmd );
        virtual void _render( const v1::CbDrawCallStrip *cmd );
        virtual void _renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd );
        virtual void _renderNoBaseInstance( const v1::CbDrawCallStrip *cmd );

        virtual void clearFrameBuffer( unsigned int buffers,
                                       const ColourValue &colour = ColourValue::Black, Real depth = 1.0f,
                                       unsigned short stencil = 0 );
        virtual void discardFrameBuffer( unsigned int buffers );
        HardwareOcclusionQuery *createHardwareOcclusionQuery();
        Real getHorizontalTexelOffset() { return 0.0; }     // No offset in GL
        Real getVerticalTexelOffset() { return 0.0; }       // No offset in GL
        Real getMinimumDepthInputValue() { return -1.0f; }  // Range [-1.0f, 1.0f]
        Real getMaximumDepthInputValue() { return 1.0f; }   // Range [-1.0f, 1.0f]
        OGRE_MUTEX( mThreadInitMutex );
        void registerThread();
        void unregisterThread();
        void preExtraThreadsStarted();
        void postExtraThreadsStarted();
        void setClipPlanesImpl( const Ogre::PlaneList &planeList ) {}

        // ----------------------------------
        // GLES2RenderSystem specific members
        // ----------------------------------
        bool hasMinGLVersion( int major, int minor ) const;
        bool checkExtension( const String &ext ) const;

        /** Returns the main context */
        GLES2Context *_getMainContext() { return mMainContext; }
        /** Unregister a render target->context mapping. If the context of target
         is the current context, change the context to the main context so it
         can be destroyed safely.

         @note This is automatically called by the destructor of
         GLES2Context.
         */
        void _unregisterContext( GLES2Context *context );
        /** Switch GL context, dealing with involved internal cached states too
         */
        void _switchContext( GLES2Context *context );
        /** One time initialization for the RenderState of a context. Things that
         only need to be set once, like the LightingModel can be defined here.
         */
        void _oneTimeContextInitialization();
        void initialiseContext( RenderWindow *primary );
        /**
         * Set current render target to target, enabling its GL context if needed
         */
        void _setRenderTarget( RenderTarget *target, uint8 viewportRenderTargetFlags );

        GLES2Support *getGLES2Support() { return mGLSupport; }
        GLint convertCompareFunction( CompareFunction func ) const;
        GLint convertStencilOp( StencilOperation op, bool invert = false ) const;

        void bindGpuProgramParameters( GpuProgramType gptype, GpuProgramParametersSharedPtr params,
                                       uint16 mask );
        void bindGpuProgramPassIterationParameters( GpuProgramType gptype );

        void _setSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
                                SceneBlendOperation op );
        void _setSeparateSceneBlending( SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
                                        SceneBlendFactor sourceFactorAlpha,
                                        SceneBlendFactor destFactorAlpha, SceneBlendOperation op,
                                        SceneBlendOperation alphaOp );
        /// @copydoc RenderSystem::getDisplayMonitorCount
        unsigned int getDisplayMonitorCount() const;

        void _setSceneBlendingOperation( SceneBlendOperation op );
        void _setSeparateSceneBlendingOperation( SceneBlendOperation op, SceneBlendOperation alphaOp );

        void _destroyDepthBuffer( RenderWindow *pRenderWnd );

        /// @copydoc RenderSystem::beginProfileEvent
        virtual void beginProfileEvent( const String &eventName );

        /// @copydoc RenderSystem::endProfileEvent
        virtual void endProfileEvent();

        /// @copydoc RenderSystem::markProfileEvent
        virtual void markProfileEvent( const String &eventName );

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        void resetRenderer( RenderWindow *pRenderWnd );

        static GLES2ManagedResourceManager *getResourceManager();

    private:
        static GLES2ManagedResourceManager *mResourceManager;
#endif

        virtual void initGPUProfiling();
        virtual void deinitGPUProfiling();
        virtual void beginGPUSampleProfile( const String &name, uint32 *hashCache );
        virtual void endGPUSampleProfile( const String &name );
    };
}  // namespace Ogre

#endif
