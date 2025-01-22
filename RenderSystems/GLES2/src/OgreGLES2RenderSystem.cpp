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
#define NOMINMAX
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2TextureManager.h"
#include "OgreGLES2DepthBuffer.h"
#include "OgreGLES2HardwarePixelBuffer.h"
#include "OgreGLES2HardwareBufferManager.h"
#include "OgreGLES2HardwareIndexBuffer.h"
#include "OgreGLES2HardwareVertexBuffer.h"
#include "OgreGLES2GpuProgramManager.h"
#include "OgreGLES2Util.h"
#include "OgreGLES2FBORenderTexture.h"
#include "OgreGLES2HardwareOcclusionQuery.h"
#include "OgreGLES2VertexArrayObject.h"
#include "OgreGLSLESShaderFactory.h"
#include "OgreFrustum.h"
#include "OgreGLSLESLinkProgram.h"
#include "OgreGLSLESLinkProgramManager.h"
#include "OgreGLSLESProgramPipelineManager.h"
#include "OgreGLSLESProgramPipeline.h"
#include "OgreGLES2HlmsPso.h"
#if OGRE_NO_GLES3_SUPPORT != 0
    #include "OgreGLES2HlmsSamplerblock.h"
#endif
#include "OgreHlmsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "Vao/OgreGLES2VaoManager.h"
#include "Vao/OgreGLES2VertexArrayObject.h"
#include "Vao/OgreGLES2BufferInterface.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "OgreRoot.h"
#include "OgreViewport.h"

#include "GLSLES/include/OgreGLSLESShader.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#   include "OgreEAGL2Window.h"
#   include "OgreEAGLES2Context.h"
#elif OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
#   include "OgreAndroidEGLWindow.h"
#   include "OgreAndroidEGLContext.h"
#elif OGRE_PLATFORM == OGRE_PLATFORM_NACL
#   include "OgreNaClWindow.h"
#else
#   include "OgreEGLWindow.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
#   include "OgreGLES2ManagedResourceManager.h"
Ogre::GLES2ManagedResourceManager* Ogre::GLES2RenderSystem::mResourceManager = NULL;
#endif

// Convenience macro from ARB_vertex_buffer_object spec
#define GL_BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

namespace Ogre {

    GLES2RenderSystem::GLES2RenderSystem()
        : mLargestSupportedAnisotropy(0),
          mBlendChannelMask( HlmsBlendblock::BlendChannelAll ),
          mDepthWrite(true),
          mScissorsEnabled(false),
          mGlobalVao(0),
          mCurrentVertexBuffer(0),
          mCurrentIndexBuffer(0),
          mCurrentPolygonMode( GL_TRIANGLES ),
          mGpuProgramManager(0),
          mGLSLESShaderFactory(0),
          mHardwareBufferManager(0),
          mRTTManager(0),
          mNullColourFramebuffer( 0 ),
          mHasDiscardFramebuffer(false),
          mCurTexMipCount(0)
    {
        LogManager::getSingleton().logMessage(getName() + " created.");

        mRenderAttribsBound.reserve(100);
#if OGRE_NO_GLES3_SUPPORT == 0
        mRenderInstanceAttribsBound.reserve(100);
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        mResourceManager = OGRE_NEW GLES2ManagedResourceManager();
#endif
        
        mGLSupport = getGLSupport();
        
        mWorldMatrix = Matrix4::IDENTITY;
        mViewMatrix = Matrix4::IDENTITY;

        mGLSupport->addConfig();

        for( size_t i = 0; i < OGRE_MAX_TEXTURE_LAYERS; i++ )
        {
            // Dummy value
            mTextureTypes[i] = 0;
        }

        mActiveRenderTarget = 0;
        mCurrentContext = 0;
        mMainContext = 0;
        mGLInitialised = false;
        mUseAdjacency = false;
        mMinFilter = FO_LINEAR;
        mMipFilter = FO_POINT;
        mSwIndirectBufferPtr = 0;
        mPso = 0;
    }

    GLES2RenderSystem::~GLES2RenderSystem()
    {
        shutdown();

        // Destroy render windows
        RenderTargetMap::iterator i;
        for (i = mRenderTargets.begin(); i != mRenderTargets.end(); ++i)
        {
            OGRE_DELETE i->second;
        }

        mRenderTargets.clear();
        OGRE_DELETE mGLSupport;
        mGLSupport = 0;

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        if (mResourceManager != NULL)
        {
            OGRE_DELETE mResourceManager;
            mResourceManager = NULL;
        }
#endif
    }

    const String& GLES2RenderSystem::getName() const
    {
        static String strName("OpenGL ES 2.x Rendering Subsystem");
        return strName;
    }

    const String& GLES2RenderSystem::getFriendlyName() const
    {
        static String strName("OpenGL ES 2.x");
        return strName;
    }

    ConfigOptionMap& GLES2RenderSystem::getConfigOptions()
    {
        return mGLSupport->getConfigOptions();
    }

    void GLES2RenderSystem::setConfigOption(const String &name, const String &value)
    {
        mGLSupport->setConfigOption(name, value);
    }

    String GLES2RenderSystem::validateConfigOptions()
    {
        // XXX Return an error string if something is invalid
        return mGLSupport->validateConfig();
    }

    bool GLES2RenderSystem::hasMinGLVersion(int major, int minor) const
    {
        return mGLSupport->hasMinGLVersion(major, minor);
    }

    bool GLES2RenderSystem::checkExtension(const String& ext) const
    {
        return mGLSupport->checkExtension(ext);
    }

    RenderWindow* GLES2RenderSystem::_initialise(bool autoCreateWindow,
                                                 const String& windowTitle)
    {
        mGLSupport->start();

        // Create the texture manager
        mTextureManager = OGRE_NEW GLES2TextureManager(*mGLSupport); 

        RenderWindow *autoWindow = mGLSupport->createWindow(autoCreateWindow,
                                                            this, windowTitle);
        RenderSystem::_initialise(autoCreateWindow, windowTitle);
        return autoWindow;
    }

    RenderSystemCapabilities* GLES2RenderSystem::createRenderSystemCapabilities() const
    {
        RenderSystemCapabilities* rsc = OGRE_NEW RenderSystemCapabilities();

        rsc->setCategoryRelevant(CAPS_CATEGORY_GL, true);
        rsc->setDriverVersion(mDriverVersion);

        const char* deviceName = (const char*)glGetString(GL_RENDERER);
        if (deviceName)
        {
            rsc->setDeviceName(deviceName);
        }

        rsc->setRenderSystemName(getName());
        rsc->parseVendorFromString(mGLSupport->getGLVendor());

        // Check for hardware mipmapping support.
        bool disableAutoMip = false;
#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        if (rsc->getVendor() == Ogre::GPU_MOZILLA)
            disableAutoMip = true;
#endif
        if (!disableAutoMip)
            rsc->setCapability(RSC_AUTOMIPMAP);

        // Check for blending support
        rsc->setCapability(RSC_BLENDING);

        // Multitexturing support and set number of texture units
        GLint units;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units));
        rsc->setNumTextureUnits(std::min<ushort>(16, units));

        // Check for Anisotropy support
        if (checkExtension("GL_EXT_texture_filter_anisotropic"))
        {
            // Max anisotropy
            GLfloat maxAnisotropy = 0;
            OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy));
            rsc->setMaxSupportedAnisotropy(maxAnisotropy);
            rsc->setCapability(RSC_ANISOTROPY);
        }

        // DOT3 support is standard
        rsc->setCapability(RSC_DOT3);
        
        // Cube map
        rsc->setCapability(RSC_CUBEMAPPING);

        // Point sprites
        rsc->setCapability(RSC_POINT_SPRITES);
        rsc->setCapability(RSC_POINT_EXTENDED_PARAMETERS);
        
        // Check for hardware stencil support and set bit depth
        GLint stencil;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_STENCIL_BITS, &stencil));
        if(stencil)
        {
            rsc->setCapability(RSC_HWSTENCIL);
            rsc->setCapability(RSC_TWO_SIDED_STENCIL);
            rsc->setStencilBufferBitDepth(stencil);
        }

#if OGRE_NO_GLES3_SUPPORT == 0
        //Extension support is disabled as it is a bit buggy and hard to support
        if (hasMinGLVersion(3, 0) || checkExtension("GL_EXT_sRGB"))
        {
            rsc->setCapability( RSC_HW_GAMMA );
        }
#endif

#if OGRE_NO_GLES3_SUPPORT == 0
        rsc->setCapability( RSC_TEXTURE_SIGNED_INT );
#endif
        
        // Vertex Buffer Objects are always supported by OpenGL ES
        rsc->setCapability(RSC_VBO);
        if(checkExtension("GL_OES_element_index_uint"))
            rsc->setCapability(RSC_32BIT_INDEX);

#if OGRE_NO_GLES2_VAO_SUPPORT == 0
#   if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_vertex_array_object") || emscripten_get_compiler_setting("LEGACY_GL_EMULATION"))
#   else
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_vertex_array_object"))
#   endif
            rsc->setCapability(RSC_VAO);
#endif

        // Check for texture compression
        // OpenGL ES - Check for these extensions too
        // For 2.0, http://www.khronos.org/registry/gles/api/2.0/gl2ext.h
        if (checkExtension("GL_IMG_texture_compression_pvrtc") ||
            checkExtension("GL_EXT_texture_compression_dxt1") ||
            checkExtension("GL_EXT_texture_compression_s3tc") ||
            checkExtension("GL_OES_compressed_ETC1_RGB8_texture") ||
            checkExtension("GL_AMD_compressed_ATC_texture") ||
            checkExtension("WEBGL_compressed_texture_s3tc") ||
            checkExtension("WEBGL_compressed_texture_atc") ||
            checkExtension("WEBGL_compressed_texture_pvrtc") ||
            checkExtension("WEBGL_compressed_texture_etc1"))

        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION);

            if(checkExtension("GL_IMG_texture_compression_pvrtc") ||
               checkExtension("GL_IMG_texture_compression_pvrtc2") ||
               checkExtension("WEBGL_compressed_texture_pvrtc"))
                rsc->setCapability(RSC_TEXTURE_COMPRESSION_PVRTC);
                
            if((checkExtension("GL_EXT_texture_compression_dxt1") &&
               checkExtension("GL_EXT_texture_compression_s3tc")) ||
               checkExtension("WEBGL_compressed_texture_s3tc"))
                rsc->setCapability(RSC_TEXTURE_COMPRESSION_DXT);

            if(checkExtension("GL_OES_compressed_ETC1_RGB8_texture") ||
               checkExtension("WEBGL_compressed_texture_etc1"))
                rsc->setCapability(RSC_TEXTURE_COMPRESSION_ETC1);

            if(hasMinGLVersion(3, 0))
                rsc->setCapability(RSC_TEXTURE_COMPRESSION_ETC2);

            if(checkExtension("GL_AMD_compressed_ATC_texture") ||
               checkExtension("WEBGL_compressed_texture_atc"))
                rsc->setCapability(RSC_TEXTURE_COMPRESSION_ATC);
        }

        rsc->setCapability(RSC_FBO);
        rsc->setCapability(RSC_HWRENDER_TO_TEXTURE);
#if OGRE_NO_GLES3_SUPPORT == 0
        // Probe number of draw buffers
        // Only makes sense with FBO support, so probe here
        GLint buffers;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &buffers);
        rsc->setNumMultiRenderTargets(std::min<int>(buffers, (GLint)OGRE_MAX_MULTIPLE_RENDER_TARGETS));
        rsc->setCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS);
#else
        rsc->setNumMultiRenderTargets(1);
#endif

        // Stencil wrapping
        rsc->setCapability(RSC_STENCIL_WRAP);

        // GL always shares vertex and fragment texture units (for now?)
        rsc->setVertexTextureUnitsShared(true);

        // Check for non-power-of-2 texture support
#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID && OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_texture_npot") || checkExtension("GL_ARB_texture_non_power_of_two"))
        {
            rsc->setCapability(RSC_NON_POWER_OF_2_TEXTURES);
            rsc->setNonPOW2TexturesLimited(false);
        }
        else
#endif
        {
            rsc->setNonPOW2TexturesLimited(true);
        }

        rsc->setCapability(RSC_TEXTURE_1D);
#if OGRE_NO_GLES3_SUPPORT == 0
        rsc->setCapability(RSC_TEXTURE_3D);
        rsc->setCapability(RSC_TEXTURE_2D_ARRAY);
#endif

        // UBYTE4 always supported
        rsc->setCapability(RSC_VERTEX_FORMAT_UBYTE4);

        // Infinite far plane always supported
        rsc->setCapability(RSC_INFINITE_FAR_PLANE);

        // Check for hardware occlusion support
        if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_occlusion_query_boolean"))
        {
            rsc->setCapability(RSC_HWOCCLUSION);
        }

        GLint maxRes2d, maxRes3d, maxResCube;
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_TEXTURE_SIZE,            &maxRes2d ) );
#if OGRE_NO_GLES3_SUPPORT == 0
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_3D_TEXTURE_SIZE,         &maxRes3d ) );
#else
        maxRes3d = 0;
#endif
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE,   &maxResCube ) );

        rsc->setMaximumResolutions( static_cast<ushort>(maxRes2d), static_cast<ushort>(maxRes3d),
                                    static_cast<ushort>(maxResCube) );

        // Point size
        GLfloat psRange[2] = {0.0, 0.0};
        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, psRange));
        rsc->setMaxPointSize(psRange[1]);
        // No point sprites, so no size
        rsc->setMaxPointSize(0.f);

        // GLSL ES is always supported in GL ES 2
        rsc->addShaderProfile("glsles");
        LogManager::getSingleton().logMessage("GLSL ES support detected");

        // Vertex/Fragment Programs
        rsc->setCapability(RSC_VERTEX_PROGRAM);
        rsc->setCapability(RSC_FRAGMENT_PROGRAM);

        // Separate shader objects
#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        OGRE_IF_IOS_VERSION_IS_GREATER_THAN(5.0)
        if(0 && checkExtension("GL_EXT_separate_shader_objects"))
            rsc->setCapability(RSC_SEPARATE_SHADER_OBJECTS);
#endif

        // Separate shader objects don't work properly on Tegra
        if (rsc->getVendor() == GPU_NVIDIA)
            rsc->unsetCapability(RSC_SEPARATE_SHADER_OBJECTS);

        GLfloat floatConstantCount = 0;
#if OGRE_NO_GLES3_SUPPORT == 0
        glGetFloatv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &floatConstantCount);
#else
        glGetFloatv(GL_MAX_VERTEX_UNIFORM_VECTORS, &floatConstantCount);
#endif
        rsc->setVertexProgramConstantFloatCount((Ogre::ushort)floatConstantCount);
        rsc->setVertexProgramConstantBoolCount((Ogre::ushort)floatConstantCount);
        rsc->setVertexProgramConstantIntCount((Ogre::ushort)floatConstantCount);

        // Fragment Program Properties
        floatConstantCount = 0;
#if OGRE_NO_GLES3_SUPPORT == 0
        glGetFloatv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &floatConstantCount);
#else
        glGetFloatv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &floatConstantCount);
#endif
        rsc->setFragmentProgramConstantFloatCount((Ogre::ushort)floatConstantCount);
        rsc->setFragmentProgramConstantBoolCount((Ogre::ushort)floatConstantCount);
        rsc->setFragmentProgramConstantIntCount((Ogre::ushort)floatConstantCount);

#if OGRE_NO_GLES3_SUPPORT == 0
        if (hasMinGLVersion(3, 0) || checkExtension("GL_OES_get_program_binary"))
        {
            GLint formats;
            OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats));

            if(formats > 0)
                rsc->setCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER);
        }
#endif

        if (hasMinGLVersion(3, 0) || checkExtension("GL_EXT_instanced_arrays"))
        {
            rsc->setCapability(RSC_VERTEX_BUFFER_INSTANCE_DATA);
        }

        // Check for Float textures
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_texture_float") || checkExtension("GL_OES_texture_half_float"))
            rsc->setCapability(RSC_TEXTURE_FLOAT);

        // Alpha to coverage always 'supported' when MSAA is available
        // although card may ignore it if it doesn't specifically support A2C
        rsc->setCapability(RSC_ALPHA_TO_COVERAGE);
        
#if OGRE_NO_GLES3_SUPPORT == 0
        // Check if render to vertex buffer (transform feedback in OpenGL)
        rsc->setCapability(RSC_HWRENDER_TO_VERTEX_BUFFER);
#endif
        return rsc;
    }

    void GLES2RenderSystem::initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, RenderTarget* primary)
    {
        if(caps->getRenderSystemName() != getName())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Trying to initialize GLES2RenderSystem from RenderSystemCapabilities that do not support OpenGL ES",
                        "GLES2RenderSystem::initialiseFromRenderSystemCapabilities");
        }

        mGpuProgramManager = OGRE_NEW GLES2GpuProgramManager();

        mGLSLESShaderFactory = OGRE_NEW GLSLESShaderFactory();
        HighLevelGpuProgramManager::getSingleton().addFactory(mGLSLESShaderFactory);

        // Set texture the number of texture units
        mFixedFunctionTextureUnits = caps->getNumTextureUnits();

        // Use VBO's by default
        mHardwareBufferManager = OGRE_NEW v1::GLES2HardwareBufferManager();

        // Create FBO manager
        LogManager::getSingleton().logMessage("GL ES 2: Using FBOs for rendering to textures");
        mRTTManager = new GLES2FBOManager();
        caps->setCapability(RSC_RTT_SEPARATE_DEPTHBUFFER);

        Log* defaultLog = LogManager::getSingleton().getDefaultLog();
        if (defaultLog)
        {
            caps->log(defaultLog);
        }

        mGLInitialised = true;
    }

    void GLES2RenderSystem::reinitialise()
    {
        this->shutdown();
        this->_initialise(true);
    }

    void GLES2RenderSystem::shutdown()
    {
        // Deleting the GLSL shader factory
        if (mGLSLESShaderFactory)
        {
            // Remove from manager safely
            if (HighLevelGpuProgramManager::getSingletonPtr())
                HighLevelGpuProgramManager::getSingleton().removeFactory(mGLSLESShaderFactory);
            OGRE_DELETE mGLSLESShaderFactory;
            mGLSLESShaderFactory = 0;
        }

        // Deleting the GPU program manager and hardware buffer manager.  Has to be done before the mGLSupport->stop().
        OGRE_DELETE mGpuProgramManager;
        mGpuProgramManager = 0;

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;

        delete mRTTManager;
        mRTTManager = 0;

        OGRE_DELETE mTextureManager;
        mTextureManager = 0;

        // Delete extra threads contexts
        for (GLES2ContextList::iterator i = mBackgroundContextList.begin();
             i != mBackgroundContextList.end(); ++i)
        {
            GLES2Context* pCurContext = *i;

            pCurContext->releaseContext();

            delete pCurContext;
        }

        mBackgroundContextList.clear();

        if( mNullColourFramebuffer )
        {
            OCGE( glDeleteFramebuffers( 1, &mNullColourFramebuffer ) );
            mNullColourFramebuffer = 0;
        }

        RenderSystem::shutdown();

        mGLSupport->stop();

        if( mGlobalVao )
        {
            OCGE( glBindVertexArray( 0 ) );
            OCGE( glDeleteVertexArrays( 1, &mGlobalVao ) );
            mGlobalVao = 0;
        }

        mGLInitialised = 0;
    }

    RenderWindow* GLES2RenderSystem::_createRenderWindow(const String &name, unsigned int width, unsigned int height,
                                                        bool fullScreen, const NameValuePairList *miscParams)
    {
        if (mRenderTargets.find(name) != mRenderTargets.end())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Window with name '" + name + "' already exists",
                        "GLES2RenderSystem::_createRenderWindow");
        }

        // Log a message
        StringStream ss;
        ss << "GLES2RenderSystem::_createRenderWindow \"" << name << "\", " <<
            width << "x" << height << " ";
        if (fullScreen)
            ss << "fullscreen ";
        else
            ss << "windowed ";

        if (miscParams)
        {
            ss << " miscParams: ";
            NameValuePairList::const_iterator it;
            for (it = miscParams->begin(); it != miscParams->end(); ++it)
            {
                ss << it->first << "=" << it->second << " ";
            }

            LogManager::getSingleton().logMessage(ss.str());
        }

        // Create the window
        RenderWindow* win = mGLSupport->newWindow(name, width, height, fullScreen, miscParams);
        attachRenderTarget((Ogre::RenderTarget&) *win);

        if (!mGLInitialised)
        {
            initialiseContext(win);
            
            mDriverVersion = mGLSupport->getGLVersion();

            if( !mDriverVersion.hasMinVersion( 3, 0 ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "OpenGL ES 3.0 or greater required. Try updating your drivers.",
                             "GLES2RenderSystem::_createRenderWindow" );
            }

            assert( !mVaoManager );
            mVaoManager = OGRE_NEW GLES2VaoManager();

            //Bind the Draw ID
            OCGE( glGenVertexArrays( 1, &mGlobalVao ) );
            OCGE( glBindVertexArray( mGlobalVao ) );
            static_cast<GLES2VaoManager*>( mVaoManager )->bindDrawId();
            OCGE( glBindVertexArray( 0 ) );

            // Get the shader language version
            const char* shadingLangVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
            StringVector tokens = StringUtil::split(shadingLangVersion, ". ");
            size_t i = 0;

            // iOS reports the GLSL version with a whole bunch of non-digit characters so we have to find where the version starts.
            for(; i < tokens.size(); i++)
            {
                if (isdigit(*tokens[i].c_str()))
                    break;
            }
            mNativeShadingLanguageVersion = (StringConverter::parseUnsignedInt(tokens[i]) * 100) + StringConverter::parseUnsignedInt(tokens[i+1]);

            // Initialise GL after the first window has been created
            // TODO: fire this from emulation options, and don't duplicate Real and Current capabilities
            mRealCapabilities = createRenderSystemCapabilities();

            mHasDiscardFramebuffer = hasMinGLVersion(3, 0) || checkExtension( "GL_EXT_discard_framebuffer" );

            // use real capabilities if custom capabilities are not available
            if (!mUseCustomCapabilities)
                mCurrentCapabilities = mRealCapabilities;

            fireEvent("RenderSystemCapabilitiesCreated");

            initialiseFromRenderSystemCapabilities(mCurrentCapabilities, (RenderTarget *) win);

            // Initialise the main context
            _oneTimeContextInitialization();
            if (mCurrentContext)
                mCurrentContext->setInitialized();
        }

        if( win->getDepthBufferPool() != DepthBuffer::POOL_NO_DEPTH )
        {
            // Unlike D3D9, OGL doesn't allow sharing the main depth buffer, so keep them separate.
            // Only Copy does, but Copy means only one depth buffer...
            GLES2Context *windowContext = 0;
            win->getCustomAttribute( "GLCONTEXT", &windowContext );
            GLES2DepthBuffer *depthBuffer = OGRE_NEW GLES2DepthBuffer( DepthBuffer::POOL_DEFAULT, this,
                                                            windowContext, GL_NONE, GL_NONE,
                                                            win->getWidth(), win->getHeight(),
                                                            win->getFSAA(), 0, PF_UNKNOWN,
                                                            false, true );

            mDepthBufferPool[depthBuffer->getPoolId()].push_back( depthBuffer );

            win->attachDepthBuffer( depthBuffer, false );
        }

        return win;
    }

    //---------------------------------------------------------------------
    DepthBuffer* GLES2RenderSystem::_createDepthBufferFor( RenderTarget *renderTarget, bool exactMatchFormat )
    {
        GLES2DepthBuffer *retVal = 0;

        // Only FBOs support different depth buffers, so everything
        // else creates dummy (empty) containers
        // retVal = mRTTManager->_createDepthBufferFor( renderTarget );
        GLES2FrameBufferObject *fbo = 0;
        renderTarget->getCustomAttribute("FBO", &fbo);

        if( fbo || renderTarget->getForceDisableColourWrites() )
        {
            PixelFormat desiredDepthBufferFormat = renderTarget->getDesiredDepthBufferFormat();

            if( !exactMatchFormat )
            {
                if( desiredDepthBufferFormat == PF_D24_UNORM_X8 && renderTarget->prefersDepthTexture() )
                    desiredDepthBufferFormat = PF_D24_UNORM;
                else
                    desiredDepthBufferFormat = PF_D24_UNORM_S8_UINT;
            }

            PixelFormat renderTargetFormat;

            if( fbo )
                renderTargetFormat = fbo->getFormat();
            else
            {
                //Deal with depth textures
                renderTargetFormat = desiredDepthBufferFormat;
            }

            // Presence of an FBO means the manager is an FBO Manager, that's why it's safe to downcast
            // Find best depth & stencil format suited for the RT's format
            GLuint depthFormat, stencilFormat;
            static_cast<GLES2FBOManager*>(mRTTManager)->getBestDepthStencil( desiredDepthBufferFormat,
                                                                             renderTargetFormat,
                                                                             &depthFormat,
                                                                             &stencilFormat );

            // OpenGL specs explicitly disallow depth textures with separate stencil.
            if( stencilFormat == GL_NONE || !renderTarget->prefersDepthTexture() )
            {
                // No "custom-quality" multisample for now in GL
                retVal = OGRE_NEW GLES2DepthBuffer( 0, this, mCurrentContext, depthFormat, stencilFormat,
                                                    renderTarget->getWidth(), renderTarget->getHeight(),
                                                    renderTarget->getFSAA(), 0,
                                                    desiredDepthBufferFormat,
                                                    renderTarget->prefersDepthTexture(), false );
            }
        }

        return retVal;
    }
    //---------------------------------------------------------------------
    void GLES2RenderSystem::_getDepthStencilFormatFor( GLenum internalColourFormat, GLenum *depthFormat,
                                                      GLenum *stencilFormat )
    {
        mRTTManager->getBestDepthStencil( internalColourFormat, depthFormat, stencilFormat );
    }

    MultiRenderTarget* GLES2RenderSystem::createMultiRenderTarget(const String & name)
    {
        MultiRenderTarget *retval = mRTTManager->createMultiRenderTarget(name);
        attachRenderTarget(*retval);
        return retval;
    }

    void GLES2RenderSystem::destroyRenderWindow(RenderWindow* pWin)
    {
        // Find it to remove from list.
        RenderTargetMap::iterator i = mRenderTargets.begin();

        while (i != mRenderTargets.end())
        {
            if (i->second == pWin)
            {
                _destroyDepthBuffer(pWin);
                mRenderTargets.erase(i);
                OGRE_DELETE pWin;
                break;
            }
        }
    }

    void GLES2RenderSystem::_destroyDepthBuffer(RenderWindow* pWin)
    {
        GLES2Context *windowContext = 0;
        pWin->getCustomAttribute("GLCONTEXT", &windowContext);
        
        // 1 Window <-> 1 Context, should be always true
        assert( windowContext );
        
        bool bFound = false;
        // Find the depth buffer from this window and remove it.
        DepthBufferMap::iterator itMap = mDepthBufferPool.begin();
        DepthBufferMap::iterator enMap = mDepthBufferPool.end();
        
        while( itMap != enMap && !bFound )
        {
            DepthBufferVec::iterator itor = itMap->second.begin();
            DepthBufferVec::iterator end  = itMap->second.end();
            
            while( itor != end )
            {
                // A DepthBuffer with no depth & stencil pointers is a dummy one,
                // look for the one that matches the same GL context.
                GLES2DepthBuffer *depthBuffer = static_cast<GLES2DepthBuffer*>(*itor);
                GLES2Context *glContext = depthBuffer->getGLContext();
                
                if( glContext == windowContext &&
                   (depthBuffer->getDepthBuffer() || depthBuffer->getStencilBuffer()) )
                {
                    bFound = true;
                    
                    delete *itor;
                    itMap->second.erase( itor );
                    break;
                }
                ++itor;
            }
            
            ++itMap;
        }
    }
    
    String GLES2RenderSystem::getErrorDescription(long errorNumber) const
    {
        // TODO find a way to get error string
//        const GLubyte *errString = gluErrorString (errCode);
//        return (errString != 0) ? String((const char*) errString) : BLANKSTRING;

        return BLANKSTRING;
    }

    VertexElementType GLES2RenderSystem::getColourVertexElementType() const
    {
        return VET_COLOUR_ABGR;
    }

    void GLES2RenderSystem::_setWorldMatrix(const Matrix4 &m)
    {
        mWorldMatrix = m;
    }

    void GLES2RenderSystem::_setViewMatrix(const Matrix4 &m)
    {
        mViewMatrix = m;

        // Also mark clip planes dirty.
        if (!mClipPlanes.empty())
        {
            mClipPlanesDirty = true;
        }
    }

    void GLES2RenderSystem::_setProjectionMatrix(const Matrix4 &m)
    {
        // Nothing to do but mark clip planes dirty.
        if (!mClipPlanes.empty())
            mClipPlanesDirty = true;
    }

    void GLES2RenderSystem::_setTexture(size_t stage, bool enabled, Texture *texPtr, bool bDepthReadOnly)
    {
        GLES2Texture *tex = static_cast<GLES2Texture*>(texPtr);

        if (!activateGLTextureUnit(stage))
            return;

        if (enabled)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
            mCurTexMipCount = 0;
#endif
            GLuint texID =  0;
            if (tex)
            {
                // Note used
                tex->touch();
                mTextureTypes[stage] = tex->getGLES2TextureTarget();
                texID = tex->getGLID();
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
                mCurTexMipCount = tex->getNumMipmaps();
#endif
            }
            else
            {
                // Assume 2D.
                mTextureTypes[stage] = GL_TEXTURE_2D;
                texID = static_cast<GLES2TextureManager*>(mTextureManager)->getWarningTextureID();
            }

            OGRE_CHECK_GL_ERROR(glBindTexture(mTextureTypes[stage], texID));
        }
        else
        {
            // Bind zero texture
            OGRE_CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, 0));
        }

        activateGLTextureUnit(0);
    }

    void GLES2RenderSystem::_setTextureCoordSet(size_t stage, size_t index)
    {
        mTextureCoordIndex[stage] = index;
    }

    GLint GLES2RenderSystem::getTextureAddressingMode(TextureAddressingMode tam) const
    {
    }

    void GLES2RenderSystem::queueBindUAV( uint32 slot, UavBufferPacked *buffer,
                                          ResourceAccess::ResourceAccess access,
                                          size_t offset, size_t sizeBytes )
    {
    }

    void GLES2RenderSystem::clearUAVs()
    {
    }

    void GLES2RenderSystem::_bindTextureUavCS( uint32 slot, Texture *texture,
                                               ResourceAccess::ResourceAccess _access,
                                               int32 mipmapLevel, int32 textureArrayIndex,
                                               PixelFormat pixelFormat )
    {
    }

    void GLES2RenderSystem::_setTextureCS( uint32 slot, bool enabled, Texture *texPtr )
    {
        this->_setTexture( slot, enabled, texPtr, false );
    }

    void GLES2RenderSystem::_setHlmsSamplerblockCS( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        this->_setHlmsSamplerblock( texUnit, samplerblock );
    }

    GLint GLES2RenderSystem::getTextureAddressingMode(TextureAddressingMode tam) const
    {
        switch (tam)
        {
            case TAM_CLAMP:
            case TAM_BORDER:
                return GL_CLAMP_TO_EDGE;
            case TAM_MIRROR:
                return GL_MIRRORED_REPEAT;
            case TAM_WRAP:
            default:
                return GL_REPEAT;
        }
    }

    GLenum GLES2RenderSystem::getBlendMode(SceneBlendFactor ogreBlend) const
    {
        switch (ogreBlend)
        {
            case SBF_ONE:
                return GL_ONE;
            case SBF_ZERO:
                return GL_ZERO;
            case SBF_DEST_COLOUR:
                return GL_DST_COLOR;
            case SBF_SOURCE_COLOUR:
                return GL_SRC_COLOR;
            case SBF_ONE_MINUS_DEST_COLOUR:
                return GL_ONE_MINUS_DST_COLOR;
            case SBF_ONE_MINUS_SOURCE_COLOUR:
                return GL_ONE_MINUS_SRC_COLOR;
            case SBF_DEST_ALPHA:
                return GL_DST_ALPHA;
            case SBF_SOURCE_ALPHA:
                return GL_SRC_ALPHA;
            case SBF_ONE_MINUS_DEST_ALPHA:
                return GL_ONE_MINUS_DST_ALPHA;
            case SBF_ONE_MINUS_SOURCE_ALPHA:
                return GL_ONE_MINUS_SRC_ALPHA;
        };

        // To keep compiler happy.
        return GL_ONE;
    }

    GLenum GLES2RenderSystem::getBlendOperation(SceneBlendOperation op) const
    {
        switch( op )
        {
        case SBO_ADD:
            return GL_FUNC_ADD;
        case SBO_SUBTRACT:
            return GL_FUNC_SUBTRACT;
        case SBO_REVERSE_SUBTRACT:
            return GL_FUNC_REVERSE_SUBTRACT;
        case SBO_MIN:
            if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                return GL_MIN_EXT;
            break;
        case SBO_MAX:
            if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                return GL_MAX_EXT;
            break;
        }
        // To keep compiler happy.
        return GL_FUNC_ADD;
    }

    void GLES2RenderSystem::_setSceneBlending(SceneBlendFactor sourceFactor, SceneBlendFactor destFactor, SceneBlendOperation op)
    {
        GLenum sourceBlend = getBlendMode(sourceFactor);
        GLenum destBlend = getBlendMode(destFactor);
        if(sourceFactor == SBF_ONE && destFactor == SBF_ZERO)
        {
            OGRE_CHECK_GL_ERROR(glDisable(GL_BLEND));
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glEnable(GL_BLEND));
            OGRE_CHECK_GL_ERROR(glBlendFunc(sourceBlend, destBlend));
        }
        
        GLint func = GL_FUNC_ADD;
        switch(op)
        {
        case SBO_ADD:
            func = GL_FUNC_ADD;
            break;
        case SBO_SUBTRACT:
            func = GL_FUNC_SUBTRACT;
            break;
        case SBO_REVERSE_SUBTRACT:
            func = GL_FUNC_REVERSE_SUBTRACT;
            break;
        case SBO_MIN:
            if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                func = GL_MIN_EXT;
            break;
        case SBO_MAX:
            if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                func = GL_MAX_EXT;
            break;
        }

        OGRE_CHECK_GL_ERROR(glBlendEquation(func));
    }

    void GLES2RenderSystem::_setSeparateSceneBlending(
        SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
        SceneBlendFactor sourceFactorAlpha, SceneBlendFactor destFactorAlpha,
        SceneBlendOperation op, SceneBlendOperation alphaOp )
    {
        GLenum sourceBlend = getBlendMode(sourceFactor);
        GLenum destBlend = getBlendMode(destFactor);
        GLenum sourceBlendAlpha = getBlendMode(sourceFactorAlpha);
        GLenum destBlendAlpha = getBlendMode(destFactorAlpha);
        
        if(sourceFactor == SBF_ONE && destFactor == SBF_ZERO && 
           sourceFactorAlpha == SBF_ONE && destFactorAlpha == SBF_ZERO)
        {
            OGRE_CHECK_GL_ERROR(glDisable(GL_BLEND));
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glEnable(GL_BLEND));
            OGRE_CHECK_GL_ERROR(glBlendFuncSeparate(sourceBlend, destBlend, sourceBlendAlpha, destBlendAlpha));
        }
        
        GLint func = GL_FUNC_ADD, alphaFunc = GL_FUNC_ADD;
        
        switch(op)
        {
            case SBO_ADD:
                func = GL_FUNC_ADD;
                break;
            case SBO_SUBTRACT:
                func = GL_FUNC_SUBTRACT;
                break;
            case SBO_REVERSE_SUBTRACT:
                func = GL_FUNC_REVERSE_SUBTRACT;
                break;
            case SBO_MIN:
                if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                    func = GL_MIN_EXT;
                break;
            case SBO_MAX:
                if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                    func = GL_MAX_EXT;
                break;
        }
        
        switch(alphaOp)
        {
            case SBO_ADD:
                alphaFunc = GL_FUNC_ADD;
                break;
            case SBO_SUBTRACT:
                alphaFunc = GL_FUNC_SUBTRACT;
                break;
            case SBO_REVERSE_SUBTRACT:
                alphaFunc = GL_FUNC_REVERSE_SUBTRACT;
                break;
            case SBO_MIN:
                if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                    alphaFunc = GL_MIN_EXT;
                break;
            case SBO_MAX:
                if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_blend_minmax"))
                    alphaFunc = GL_MAX_EXT;
                break;
        }
        
        OGRE_CHECK_GL_ERROR(glBlendEquationSeparate(func, alphaFunc));
    }

    void GLES2RenderSystem::correctViewport( GLint x, GLint &y, GLint &w, GLint &h, RenderTarget *renderTarget )
    {
        if( !renderTarget->requiresTextureFlipping() )
        {
            // Convert "upper-left" corner to "lower-left"
            y = renderTarget->getHeight() - h - y;
        }

#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        ConfigOptionMap::const_iterator opt;
        ConfigOptionMap::const_iterator end = mGLSupport->getConfigOptions().end();

        if ((opt = mGLSupport->getConfigOptions().find("Orientation")) != end)
        {
            String val = opt->second.currentValue;
            String::size_type pos = val.find("Landscape");

            if (pos != String::npos)
            {
                GLsizei temp = h;
                h = w;
                w = temp;
            }
        }
#endif
    }

    void GLES2RenderSystem::_setViewport(Viewport *vp)
    {
        // Check if viewport is different
        if (!vp)
        {
            mActiveViewport = NULL;
            _setRenderTarget(NULL, VP_RTT_COLOUR_WRITE);
        }
        else if (vp != mActiveViewport || vp->_isUpdated())
        {
            RenderTarget* target;
            
            target = vp->getTarget();
            _setRenderTarget(target, vp->getViewportRenderTargetFlags());
            mActiveViewport = vp;
            
            GLsizei x, y, w, h;
            
            // Calculate the "lower-left" corner of the viewport
            w = vp->getActualWidth();
            h = vp->getActualHeight();
            x = vp->getActualLeft();
            y = vp->getActualTop();
            
            correctViewport( x, y, w, h, target );

            OGRE_CHECK_GL_ERROR(glViewport(x, y, w, h));

            w = vp->getScissorActualWidth();
            h = vp->getScissorActualHeight();
            x = vp->getScissorActualLeft();
            y = vp->getScissorActualTop();

            correctViewport( x, y, w, h, target );

            OGRE_CHECK_GL_ERROR(glScissor(x, y, w, h));
            
            vp->_clearUpdatedFlag();
        }
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        else
        {
            // On iOS RenderWindow is FBO based, renders to multisampled FBO and then resolves
            // to non-multisampled FBO, therefore we need to restore FBO binding even when
            // rendering to the same viewport.
            RenderTarget* target = vp->getTarget();
            mRTTManager->bind(target);
        }
#endif
    }

    bool GLES2RenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newBlock, uint64 deadline )
    {
        GLES2HlmsPso *pso = new GLES2HlmsPso();
        memset( pso, 0, sizeof(GLES2HlmsPso) );
        
        //
        // Macroblock stuff
        //

        pso->depthWrite   = newBlock->macroblock->mDepthWrite ? GL_TRUE : GL_FALSE;
        pso->depthFunc    = convertCompareFunction( newBlock->macroblock->mDepthFunc );

        switch( newBlock->macroblock->mCullMode )
        {
        case CULL_NONE:
            pso->cullMode = 0;
            break;
        default:
        case CULL_CLOCKWISE:
            pso->cullMode = GL_BACK;
            break;
        case CULL_ANTICLOCKWISE:
            pso->cullMode = GL_FRONT;
            break;
        }

        switch( newBlock->macroblock->mPolygonMode )
        {
        case PM_POINTS:
            //pso->polygonMode = GL_POINTS;
            pso->polygonMode = GL_POINT;
            break;
        case PM_WIREFRAME:
            //pso->polygonMode = GL_LINE_STRIP;
            pso->polygonMode = GL_LINE;
            break;
        default:
        case PM_SOLID:
            pso->polygonMode = GL_FILL;
            break;
        }

        //
        // Blendblock stuff
        //
        pso->enableAlphaBlend = newBlock->blendblock->mSourceBlendFactor != SBF_ONE ||
                                newBlock->blendblock->mDestBlendFactor != SBF_ZERO;
        if( newBlock->blendblock->mSeparateBlend )
        {
            pso->enableAlphaBlend |= newBlock->blendblock->mSourceBlendFactorAlpha != SBF_ONE ||
                                     newBlock->blendblock->mDestBlendFactorAlpha != SBF_ZERO;
        }
        pso->sourceBlend        = getBlendMode( newBlock->blendblock->mSourceBlendFactor );
        pso->destBlend          = getBlendMode( newBlock->blendblock->mDestBlendFactor );
        pso->sourceBlendAlpha   = getBlendMode( newBlock->blendblock->mSourceBlendFactorAlpha );
        pso->destBlendAlpha     = getBlendMode( newBlock->blendblock->mDestBlendFactorAlpha );
        pso->blendFunc          = getBlendOperation( newBlock->blendblock->mBlendOperation );
        pso->blendFuncAlpha     = getBlendOperation( newBlock->blendblock->mBlendOperationAlpha );


        //
        // Shader stuff
        //

        if( newBlock->vertexShader )
        {
            pso->vertexShader = static_cast<GLSLESShader*>( newBlock->vertexShader->
                                                          _getBindingDelegate() );
        }
        if( newBlock->pixelShader )
        {
            pso->pixelShader = static_cast<GLSLESShader*>( newBlock->pixelShader->
                                                         _getBindingDelegate() );
        }
        
        newBlock->rsData = pso;
        return true;
    }

    void GLES2RenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        GLES2HlmsPso *glPso = reinterpret_cast<GLES2HlmsPso*>(pso->rsData);
        delete glPso;
        pso->rsData = 0;
    }

    void GLES2RenderSystem::_hlmsMacroblockCreated( HlmsMacroblock *newBlock )
    {
        //Set it to non-zero to get the assert in _setHlmsBlendblock
        //to work correctly (which is a very useful assert)
        newBlock->mRsData = reinterpret_cast<void*>( 1 );
    }

    void GLES2RenderSystem::_hlmsMacroblockDestroyed( HlmsMacroblock *block )
    {
        block->mRsData = 0;
    }

    void GLES2RenderSystem::_hlmsBlendblockCreated( HlmsBlendblock *newBlock )
    {
        //Set it to non-zero to get the assert in _setHlmsBlendblock
        //to work correctly (which is a very useful assert)
        newBlock->mRsData = reinterpret_cast<void*>( 1 );
    }

    void GLES2RenderSystem::_hlmsBlendblockDestroyed( HlmsBlendblock *block )
    {
        block->mRsData = 0;
    }

    void GLES2RenderSystem::_hlmsSamplerblockCreated( HlmsSamplerblock *newBlock )
    {
#if OGRE_NO_GLES3_SUPPORT == 0
        GLuint samplerName;
        glGenSamplers( 1, &samplerName );
#endif

        GLint minFilter, magFilter;
        switch( newBlock->mMinFilter )
        {
        case FO_ANISOTROPIC:
        case FO_LINEAR:
            switch( newBlock->mMipFilter )
            {
            case FO_ANISOTROPIC:
            case FO_LINEAR:
                // linear min, linear mip
                minFilter = GL_LINEAR_MIPMAP_LINEAR;
                break;
            case FO_POINT:
                // linear min, point mip
                minFilter = GL_LINEAR_MIPMAP_NEAREST;
                break;
            case FO_NONE:
                // linear min, no mip
                minFilter = GL_LINEAR;
                break;
            }
            break;
        case FO_POINT:
        case FO_NONE:
            switch( newBlock->mMipFilter )
            {
            case FO_ANISOTROPIC:
            case FO_LINEAR:
                // nearest min, linear mip
                minFilter = GL_NEAREST_MIPMAP_LINEAR;
                break;
            case FO_POINT:
                // nearest min, point mip
                minFilter = GL_NEAREST_MIPMAP_NEAREST;
                break;
            case FO_NONE:
                // nearest min, no mip
                minFilter = GL_NEAREST;
                break;
            }
            break;
        }

        magFilter = newBlock->mMagFilter <= FO_POINT ? GL_NEAREST : GL_LINEAR;

#if OGRE_NO_GLES3_SUPPORT == 0
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_MIN_FILTER, minFilter ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_MAG_FILTER, magFilter ) );

        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_S,
                                   getTextureAddressingMode( newBlock->mU ) ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_T,
                                   getTextureAddressingMode( newBlock->mV ) ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_R,
                                   getTextureAddressingMode( newBlock->mW ) ) );

        /*OCGE( glSamplerParameterfv( samplerName, GL_TEXTURE_BORDER_COLOR,
                                    newBlock->mBorderColour.ptr() ) );
        OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_LOD_BIAS, newBlock->mMipLodBias ) );*/
        OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_MIN_LOD, newBlock->mMinLod ) );
        OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_MAX_LOD, newBlock->mMaxLod ) );

        if( newBlock->mCompareFunction != NUM_COMPARE_FUNCTIONS )
        {
            OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_COMPARE_MODE,
                                       GL_COMPARE_REF_TO_TEXTURE ) );
            OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_COMPARE_FUNC,
                                       convertCompareFunction( newBlock->mCompareFunction ) ) );
        }

        if( mCurrentCapabilities->hasCapability(RSC_ANISOTROPY) )
        {
            OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                       newBlock->mMaxAnisotropy ) );
        }

        newBlock->mRsData = (void*)(intptr_t)samplerName;
#else
        GLES2HlmsSamplerblock *glSamplerblock = new GLES2HlmsSamplerblock();
        glSamplerblock->mInternalId= ++mSamplerblocksInternalIdCount;
        glSamplerblock->mMinFilter = minFilter;
        glSamplerblock->mMagFilter = magFilter;
        glSamplerblock->mU  = getTextureAddressingMode( newBlock->mU );
        glSamplerblock->mV  = getTextureAddressingMode( newBlock->mV );
        glSamplerblock->mW  = getTextureAddressingMode( newBlock->mW );

        glSamplerblock->mAnisotropy = std::min( newBlock->mMaxAnisotropy, mLargestSupportedAnisotropy );

        newBlock->mRsData = glSamplerblock;
#endif
    }

    void GLES2RenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
#if OGRE_NO_GLES3_SUPPORT == 0
        GLuint samplerName = static_cast<GLuint>( reinterpret_cast<intptr_t>( block->mRsData ) );
        glDeleteSamplers( 1, &samplerName );
#else
        GLES2HlmsSamplerblock *glSamplerblock = reinterpret_cast<GLES2HlmsSamplerblock*>(block->mRsData);
        delete glSamplerblock;
        block->mRsData = 0;
#endif
    }

    void GLES2RenderSystem::_setHlmsMacroblock( const HlmsMacroblock *macroblock, const GLES2HlmsPso *pso )
    {
        if( macroblock->mDepthCheck )
        {
            OCGE( glEnable( GL_DEPTH_TEST ) );
        }
        else
        {
            OCGE( glDisable( GL_DEPTH_TEST ) );
        }
        OCGE( glDepthMask( pso->depthWrite ) );
        OCGE( glDepthFunc( pso->depthFunc ) );

        _setDepthBias( macroblock->mDepthBiasConstant, macroblock->mDepthBiasSlopeScale );

        //Cull mode
        if( pso->cullMode == 0 )
        {
            OCGE( glDisable( GL_CULL_FACE ) );
        }
        else
        {
            OCGE( glEnable( GL_CULL_FACE ) );
            OCGE( glCullFace( pso->cullMode ) );
        }

        //Polygon mode
        //OCGE( glPolygonMode( GL_FRONT_AND_BACK, pso->polygonMode ) );

        if( macroblock->mScissorTestEnabled )
        {
            OCGE( glEnable(GL_SCISSOR_TEST) );
        }
        else
        {
            OCGE( glDisable(GL_SCISSOR_TEST) );
        }

        mDepthWrite      = macroblock->mDepthWrite;
        mScissorsEnabled = macroblock->mScissorTestEnabled;
    }

    void GLES2RenderSystem::_setHlmsBlendblock( const HlmsBlendblock *blendblock, const GLES2HlmsPso *pso )
    {
        if( pso->enableAlphaBlend )
        {
            OCGE( glEnable(GL_BLEND) );
            if( blendblock->mSeparateBlend )
            {
                OCGE( glBlendFuncSeparate( pso->sourceBlend, pso->destBlend,
                                           pso->sourceBlendAlpha, pso->destBlendAlpha ) );
                OCGE( glBlendEquationSeparate( pso->blendFunc, pso->blendFuncAlpha ) );
            }
            else
            {
                OCGE( glBlendFunc( pso->sourceBlend, pso->destBlend ) );
                OCGE( glBlendEquation( pso->blendFunc ) );
            }
        }
        else
        {
            OCGE( glDisable(GL_BLEND) );
        }

        if( blendblock->mAlphaToCoverageEnabled )
        {
            OCGE( glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE) );
        }
        else
        {
            OCGE( glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE) );
        }


        if( mBlendChannelMask != blendblock->mBlendChannelMask )
        {
            GLboolean r = (blendblock->mBlendChannelMask & HlmsBlendblock::BlendChannelRed) != 0;
            GLboolean g = (blendblock->mBlendChannelMask & HlmsBlendblock::BlendChannelGreen) != 0;
            GLboolean b = (blendblock->mBlendChannelMask & HlmsBlendblock::BlendChannelBlue) != 0;
            GLboolean a = (blendblock->mBlendChannelMask & HlmsBlendblock::BlendChannelAlpha) != 0;
            OCGE( glColorMask( r, g, b, a ) );

            mBlendChannelMask = blendblock->mBlendChannelMask;
        }
    }

    void GLES2RenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        assert( (!samplerblock || samplerblock->mRsData) &&
                "The block must have been created via HlmsManager::getSamplerblock!" );

#if OGRE_NO_GLES3_SUPPORT == 0
        if( !samplerblock )
            glBindSampler( texUnit, 0 );
        else
            glBindSampler( texUnit, static_cast<GLuint>(
                                    reinterpret_cast<intptr_t>( samplerblock->mRsData ) ) );
#else
        if (!activateGLTextureUnit(texUnit))
            return;

        GLES2HlmsSamplerblock *glSamplerblock = reinterpret_cast<GLES2HlmsSamplerblock*>( samplerblock->mRsData );
        GLenum target = mTextureTypes[texUnit]

        OGRE_CHECK_GL_ERROR(glTexParameteri( target, GL_TEXTURE_MIN_FILTER, glSamplerblock->mMinFilter ));
        OGRE_CHECK_GL_ERROR(glTexParameteri( target, GL_TEXTURE_MAG_FILTER, glSamplerblock->mMagFilter ));

        OGRE_CHECK_GL_ERROR(glTexParameteri( target, GL_TEXTURE_WRAP_S, glSamplerblock->mU ));
        OGRE_CHECK_GL_ERROR(glTexParameteri( target, GL_TEXTURE_WRAP_T, glSamplerblock->mV ));
        //OGRE_CHECK_GL_ERROR(glTexParameteri( target, GL_TEXTURE_WRAP_R_OES, glSamplerblock->mW ));

        if( glSamplerblock->mAnisotropy != 0 )
        {
            OGRE_CHECK_GL_ERROR(glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                                  glSamplerblock->mAnisotropy ));
        }

        /*OGRE_CHECK_GL_ERROR(glTexParameterf( target, GL_TEXTURE_LOD_BIAS, samplerblock->mMipLodBias ));

        OGRE_CHECK_GL_ERROR(glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR,
                                reinterpret_cast<GLfloat*>( &samplerblock->mBorderColour ) ) );

        OGRE_CHECK_GL_ERROR(glTexParameterf( target, GL_TEXTURE_MIN_LOD, samplerblock->mMinLod ));
        OGRE_CHECK_GL_ERROR(glTexParameterf( target, GL_TEXTURE_MAX_LOD, samplerblock->mMaxLod ));*/


        activateGLTextureUnit(0);
#endif
    }

    void GLES2RenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        // Disable previous state
        GLSLESShader::unbindAll();

        RenderSystem::_setPipelineStateObject( pso );

        mUseAdjacency   = false;
        mPso            = 0;

        if( !pso )
            return;
        
        //Set new state
        mPso = reinterpret_cast<GLES2HlmsPso*>( pso->rsData );

        _setHlmsMacroblock( pso->macroblock, mPso );
        _setHlmsBlendblock( pso->blendblock, mPso );

        if( mPso->vertexShader )
        {
            mPso->vertexShader->bind();
            mActiveVertexGpuProgramParameters = mPso->vertexShader->getDefaultParameters();
            mVertexProgramBound = true;
        }
        if( mPso->pixelShader )
        {
            mPso->pixelShader->bind();
            mActiveFragmentGpuProgramParameters = mPso->pixelShader->getDefaultParameters();
            mFragmentProgramBound = true;
        }
        
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
        else
            GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
    }

    void GLES2RenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
    }

    void GLES2RenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
//        if( mVaoManager->supportsIndirectBuffers() )
//        {
//            if( indirectBuffer )
//            {
//                GLES2BufferInterface *bufferInterface = static_cast<GLES2BufferInterface*>(
//                                                            indirectBuffer->getBufferInterface() );
//                OCGE( glBindBuffer( GL_DRAW_INDIRECT_BUFFER, bufferInterface->getVboName() ) );
//            }
//            else
//            {
//                OCGE( glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 ) );
//            }
//        }
//        else
        {
            if( indirectBuffer )
                mSwIndirectBufferPtr = indirectBuffer->getSwBufferPtr();
            else
                mSwIndirectBufferPtr = 0;
        }
    }

    void GLES2RenderSystem::_beginFrame()
    {
    }

    void GLES2RenderSystem::_endFrame()
    {
        // unbind PSO programs at end of frame
        // this is mostly to avoid holding bound programs that might get deleted
        // outside via the resource manager
        _setPipelineStateObject( 0 );

#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        glBindProgramPipelineEXT(0);
#endif
    }

    void GLES2RenderSystem::_setDepthBias(float constantBias, float slopeScaleBias)
    {
        if (constantBias != 0 || slopeScaleBias != 0)
        {
            OGRE_CHECK_GL_ERROR(glEnable(GL_POLYGON_OFFSET_FILL));
            OGRE_CHECK_GL_ERROR(glPolygonOffset(-slopeScaleBias, -constantBias));
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glDisable(GL_POLYGON_OFFSET_FILL));
        }
    }

    void GLES2RenderSystem::_setColourBufferWriteEnabled(bool red, bool green, bool blue, bool alpha)
    {
        mStateCacheManager->setColourMask(red, green, blue, alpha);
    }

    void GLES2RenderSystem::_convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest)
    {
        if( !mReverseDepth )
        {
            // no any conversion request for OpenGL
            dest = matrix;
        }
        else
        {
            RenderSystem::_convertProjectionMatrix( matrix, dest );
        }
    }

    //---------------------------------------------------------------------
    HardwareOcclusionQuery* GLES2RenderSystem::createHardwareOcclusionQuery()
    {
        if(mGLSupport->checkExtension("GL_EXT_occlusion_query_boolean") || gleswIsSupported(3, 0))
        {
            GLES2HardwareOcclusionQuery* ret = new GLES2HardwareOcclusionQuery(); 
            mHwOcclusionQueries.push_back(ret);
            return ret;
        }
        else
        {
            return NULL;
        }
    }

    void GLES2RenderSystem::_applyObliqueDepthProjection(Matrix4& matrix,
                                                      const Plane& plane,
                                                      bool forGpuProgram)
    {
        // Thanks to Eric Lenyel for posting this calculation at www.terathon.com
        
        // Calculate the clip-space corner point opposite the clipping plane
        // as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
        // transform it into camera space by multiplying it
        // by the inverse of the projection matrix

        Vector4 q;
        q.x = (Math::Sign(plane.normal.x) + matrix[0][2]) / matrix[0][0];
        q.y = (Math::Sign(plane.normal.y) + matrix[1][2]) / matrix[1][1];
        q.z = -1.0F;
        q.w = (1.0F + matrix[2][2]) / matrix[2][3];

        // Calculate the scaled plane vector
        Vector4 clipPlane4d(plane.normal.x, plane.normal.y, plane.normal.z, plane.d);
        Vector4 c = clipPlane4d * (2.0F / (clipPlane4d.dotProduct(q)));

        // Replace the third row of the projection matrix
        matrix[2][0] = c.x;
        matrix[2][1] = c.y;
        matrix[2][2] = c.z + 1.0F;
        matrix[2][3] = c.w; 
    }

    void GLES2RenderSystem::setStencilCheckEnabled(bool enabled)
    {
        if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_occlusion_query_boolean"))
        {
            GLES2HardwareOcclusionQuery* ret = new GLES2HardwareOcclusionQuery();
            mHwOcclusionQueries.push_back(ret);
            return ret;
        }
        else
        {
            return NULL;
        }
    }

    void GLES2RenderSystem::setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );
        
        if( mStencilParams.enabled )
        {
            OCGE( glEnable(GL_STENCIL_TEST) );

            OCGE( glStencilMask( mStencilParams.writeMask ) );

            //OCGE( glStencilMaskSeparate( GL_BACK, mStencilParams.writeMask ) );
            //OCGE( glStencilMaskSeparate( GL_FRONT, mStencilParams.writeMask ) );

            if(stencilParams.stencilFront != stencilParams.stencilBack)
            {
                if (!mCurrentCapabilities->hasCapability(RSC_TWO_SIDED_STENCIL))
                    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "2-sided stencils are not supported",
                                "GLES2RenderSystem::setStencilBufferParams");
                
                OCGE( glStencilFuncSeparate( GL_BACK,
                                             convertCompareFunction(stencilParams.stencilBack.compareOp),
                                             refValue, stencilParams.readMask ) );
                OCGE( glStencilOpSeparate( GL_BACK,
                                           convertStencilOp(stencilParams.stencilBack.stencilFailOp),
                                           convertStencilOp(stencilParams.stencilBack.stencilDepthFailOp),
                                           convertStencilOp(stencilParams.stencilBack.stencilPassOp)));

                OCGE( glStencilFuncSeparate( GL_FRONT,
                                             convertCompareFunction(stencilParams.stencilFront.compareOp),
                                             refValue, stencilParams.readMask ) );
                OCGE( glStencilOpSeparate( GL_FRONT,
                                           convertStencilOp(stencilParams.stencilFront.stencilFailOp),
                                           convertStencilOp(stencilParams.stencilFront.stencilDepthFailOp),
                                           convertStencilOp(stencilParams.stencilFront.stencilPassOp)));
            }
            else
            {
                OCGE( glStencilFunc( convertCompareFunction(stencilParams.stencilFront.compareOp),
                                     refValue, stencilParams.readMask ) );
                OCGE( glStencilOp( convertStencilOp(stencilParams.stencilFront.stencilFailOp),
                                   convertStencilOp(stencilParams.stencilFront.stencilDepthFailOp),
                                   convertStencilOp(stencilParams.stencilFront.stencilPassOp)));
            }
        }
        else
        {
            OCGE( glDisable(GL_STENCIL_TEST) );
        }
    }

    GLint GLES2RenderSystem::getCombinedMinMipFilter() const
    {
        switch(mMinFilter)
        {
            case FO_ANISOTROPIC:
            case FO_LINEAR:
                switch (mMipFilter)
                {
                    case FO_ANISOTROPIC:
                    case FO_LINEAR:
                        // linear min, linear mip
                        return GL_LINEAR_MIPMAP_LINEAR;
                    case FO_POINT:
                        // linear min, point mip
                        return GL_LINEAR_MIPMAP_NEAREST;
                    case FO_NONE:
                        // linear min, no mip
                        return GL_LINEAR;
                }
                break;
            case FO_POINT:
            case FO_NONE:
                switch (mMipFilter)
                {
                    case FO_ANISOTROPIC:
                    case FO_LINEAR:
                        // nearest min, linear mip
                        return GL_NEAREST_MIPMAP_LINEAR;
                    case FO_POINT:
                        // nearest min, point mip
                        return GL_NEAREST_MIPMAP_NEAREST;
                    case FO_NONE:
                        // nearest min, no mip
                        return GL_NEAREST;
                }
                break;
        }

        // should never get here
        return 0;
    }

    void GLES2RenderSystem::_render(const v1::RenderOperation& op)
    {
        // Call super class.
        RenderSystem::_render(op);

        // Create variables related to instancing.
        v1::HardwareVertexBufferSharedPtr globalInstanceVertexBuffer;
        v1::VertexDeclaration* globalVertexDeclaration = 0;
        size_t numberOfInstances = 1;
        if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_instanced_arrays"))
        {
            globalInstanceVertexBuffer = getGlobalInstanceVertexBuffer();
            globalVertexDeclaration = getGlobalInstanceVertexBufferVertexDeclaration();

            numberOfInstances = op.numberOfInstances;

            if (op.useGlobalInstancingVertexBufferIsAvailable)
            {
                numberOfInstances *= getGlobalNumberOfInstances();
            }
        }

        const v1::VertexDeclaration::VertexElementList& decl =
            op.vertexData->vertexDeclaration->getElements();
        v1::VertexDeclaration::VertexElementList::const_iterator elemIter, elemEnd;
        elemEnd = decl.end();

        // Bind VAO (set of per-vertex attributes: position, normal, etc.).
        bool updateVAO = true;
        // Till VAO is NYI we use single shared, and update it each time

        // Bind the appropriate VBOs to the active attributes of the VAO.
        for (elemIter = decl.begin(); elemIter != elemEnd; ++elemIter)
        {
            const v1::VertexElement & elem = *elemIter;
            size_t source = elem.getSource();

            if (!op.vertexData->vertexBufferBinding->isBufferBound(source))
                continue; // Skip unbound elements.
 
            v1::HardwareVertexBufferSharedPtr vertexBuffer =
                op.vertexData->vertexBufferBinding->getBuffer(source);
            bindVertexElementToGpu(elem, vertexBuffer, op.vertexData->vertexStart,
                                   mRenderAttribsBound, mRenderInstanceAttribsBound, updateVAO);
        }

        if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_instanced_arrays"))
        {
            if( globalInstanceVertexBuffer && globalVertexDeclaration )
            {
                elemEnd = globalVertexDeclaration->getElements().end();
                for (elemIter = globalVertexDeclaration->getElements().begin(); elemIter != elemEnd; ++elemIter)
                {
                    const v1::VertexElement & elem = *elemIter;
                    bindVertexElementToGpu(elem, globalInstanceVertexBuffer, 0,
                                           mRenderAttribsBound, mRenderInstanceAttribsBound, updateVAO);
                }
            }
        }

        // Determine the correct primitive type to render.
        GLint primType;
        switch (op.operationType)
        {
            case OT_POINT_LIST:
                primType = GL_POINTS;
                break;
            case OT_LINE_LIST:
                primType = GL_LINES;
                break;
            case OT_LINE_STRIP:
                primType = GL_LINE_STRIP;
                break;
            default:
            case OT_TRIANGLE_LIST:
                primType = GL_TRIANGLES;
                break;
            case OT_TRIANGLE_STRIP:
                primType = GL_TRIANGLE_STRIP;
                break;
            case OT_TRIANGLE_FAN:
                primType = GL_TRIANGLE_FAN;
                break;
        }

        if (op.useIndexes)
        {
            OGRE_CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<v1::GLES2HardwareIndexBuffer*>(op.indexData->indexBuffer.get())->getGLBufferId()));

            void* pBufferData = GL_BUFFER_OFFSET(op.indexData->indexStart *
                                            op.indexData->indexBuffer->getIndexSize());

            GLenum indexType = (op.indexData->indexBuffer->getType() == v1::HardwareIndexBuffer::IT_16BIT) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

            do
            {
                // Update derived depth bias.
                if (mDerivedDepthBias && mCurrentPassIterationNum > 0)
                {
                    _setDepthBias(mDerivedDepthBiasBase +
                                  mDerivedDepthBiasMultiplier * mCurrentPassIterationNum,
                                  mDerivedDepthBiasSlopeScale);
                }

                if(numberOfInstances > 1)
                {
                    OGRE_CHECK_GL_ERROR(glDrawElementsInstancedEXT(primType, static_cast<GLsizei>(op.indexData->indexCount), indexType, pBufferData, static_cast<GLsizei>(numberOfInstances)));
                }
                else
                {
                    OGRE_CHECK_GL_ERROR(glDrawElements(primType, static_cast<GLsizei>(op.indexData->indexCount), indexType, pBufferData));
                }

            } while (updatePassIterationRenderState());
        }
        else
        {
            do
            {
                // Update derived depth bias.
                if (mDerivedDepthBias && mCurrentPassIterationNum > 0)
                {
                    _setDepthBias(mDerivedDepthBiasBase +
                                  mDerivedDepthBiasMultiplier * mCurrentPassIterationNum,
                                  mDerivedDepthBiasSlopeScale);
                }

                if(numberOfInstances > 1)
                {
                    OGRE_CHECK_GL_ERROR(glDrawArraysInstancedEXT(primType, 0, static_cast<GLsizei>(op.vertexData->vertexCount), static_cast<GLsizei>(numberOfInstances)));
                }
                else
                {
                    OGRE_CHECK_GL_ERROR(glDrawArrays(primType, 0, static_cast<GLsizei>(op.vertexData->vertexCount)));
                }
            } while (updatePassIterationRenderState());
        }

#if OGRE_NO_GLES2_VAO_SUPPORT == 0
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_vertex_array_object"))
            // Unbind the vertex array object.  Marks the end of what state will be included.
            OGRE_CHECK_GL_ERROR(glBindVertexArrayOES(0));
#endif

        // Unbind all attributes
        for (vector<GLuint>::type::iterator ai = mRenderAttribsBound.begin(); ai != mRenderAttribsBound.end(); ++ai)
        {
            OGRE_CHECK_GL_ERROR(glDisableVertexAttribArray(*ai));
        }

        // Unbind any instance attributes
        for (vector<GLuint>::type::iterator ai = mRenderInstanceAttribsBound.begin(); ai != mRenderInstanceAttribsBound.end(); ++ai)
        {
            glVertexAttribDivisorEXT(*ai, 0);
        }

        mRenderAttribsBound.clear();
        mRenderInstanceAttribsBound.clear();
    }

    void GLES2RenderSystem::_dispatch( const HlmsComputePso &pso )
    {
    }

    void GLES2RenderSystem::_setVertexArrayObject( const VertexArrayObject *_vao )
    {
#if OGRE_NO_GLES2_VAO_SUPPORT == 0
        if(hasMinGLVersion(3, 0) || checkExtension("GL_OES_vertex_array_object"))
        {
            if( _vao )
            {
                const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( _vao );
                OGRE_CHECK_GL_ERROR( glBindVertexArrayOES( vao->mVaoName ) );
            }
            else
            {
                OGRE_CHECK_GL_ERROR( glBindVertexArrayOES( 0 ) );
            }
        }
#endif
    }

    void GLES2RenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
//        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
//        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];
//
//        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
//                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
//
//        OCGE( glMultiDrawElementsIndirect( mode, indexType, cmd->indirectBufferOffset,
//                                           cmd->numDraws, sizeof(CbDrawIndexed) ) );
    }

    void GLES2RenderSystem::_render( const CbDrawCallStrip *cmd )
    {
//        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
//        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];
//
//        OCGE( glMultiDrawArraysIndirect( mode, cmd->indirectBufferOffset,
//                                         cmd->numDraws, sizeof(CbDrawStrip) ) );
    }

    void GLES2RenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
//        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
//        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];
//
//        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
//                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
//
//        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed*>(
//                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );
//
//        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();
//
//        for( uint32 i=cmd->numDraws; i--; )
//        {
//            OCGE( glDrawElementsInstancedBaseVertexBaseInstance(
//                      mode,
//                      drawCmd->primCount,
//                      indexType,
//                      reinterpret_cast<void*>( drawCmd->firstVertexIndex * bytesPerIndexElement ),
//                      drawCmd->instanceCount,
//                      drawCmd->baseVertex,
//                      drawCmd->baseInstance ) );
//            ++drawCmd;
//        }
    }

    void GLES2RenderSystem::_renderEmulated( const CbDrawCallStrip *cmd )
    {
//        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
//        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];
//
//        CbDrawStrip *drawCmd = reinterpret_cast<CbDrawStrip*>(
//                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );
//
//        for( uint32 i=cmd->numDraws; i--; )
//        {
//            OCGE( glDrawArraysInstancedBaseInstance(
//                      mode,
//                      drawCmd->firstVertexIndex,
//                      drawCmd->primCount,
//                      drawCmd->instanceCount,
//                      drawCmd->baseInstance ) );
//            ++drawCmd;
//        }
    }

    void GLES2RenderSystem::_renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd )
    {
        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
        GLenum mode = /* mPso->domainShader ? GL_PATCHES : */ vao->mPrimType[mUseAdjacency];

        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();

        GLSLESProgramCommon* activeProgram = NULL;
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            activeProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
        else
            activeProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();

        for( uint32 i=cmd->numDraws; i--; )
        {
            if(activeProgram->getBaseInstanceLocation() != (GLint)GL_INVALID_INDEX)
                OCGE( glUniform1ui( activeProgram->getBaseInstanceLocation(),
                                    static_cast<GLuint>( drawCmd->baseInstance ) ) );

//          if(hasMinGLVersion(3, 2))
//          {
//            OCGE( glDrawElementsInstancedBaseVertex(
//                    mode,
//                    drawCmd->primCount,
//                    indexType,
//                    reinterpret_cast<void*>( drawCmd->firstVertexIndex * bytesPerIndexElement ),
//                    drawCmd->instanceCount,
//                    drawCmd->baseVertex ) );
//          }
//          else
            {
                assert(drawCmd->baseVertex == 0);
                OCGE( glDrawElementsInstanced(
                        mode,
                        drawCmd->primCount,
                        indexType,
                        reinterpret_cast<void*>( drawCmd->firstVertexIndex * bytesPerIndexElement ),
                        drawCmd->instanceCount ) );
            }
            ++drawCmd;
        }
    }

    void GLES2RenderSystem::_renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd )
    {
        const GLES2VertexArrayObject *vao = static_cast<const GLES2VertexArrayObject*>( cmd->vao );
        GLenum mode = /* mPso->domainShader ? GL_PATCHES : */ vao->mPrimType[mUseAdjacency];

        CbDrawStrip *drawCmd = reinterpret_cast<CbDrawStrip*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        GLSLESProgramCommon* activeProgram = NULL;
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            activeProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
        else
            activeProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();

        for( uint32 i=cmd->numDraws; i--; )
        {
            if(activeProgram->getBaseInstanceLocation() != (GLint)GL_INVALID_INDEX)
                OCGE( glUniform1ui( activeProgram->getBaseInstanceLocation(),
                                    static_cast<GLuint>( drawCmd->baseInstance ) ) );

            OCGE( glDrawArraysInstanced(
                    mode,
                    drawCmd->firstVertexIndex,
                    drawCmd->primCount,
                    drawCmd->instanceCount ) );
            ++drawCmd;
        }
    }

    void GLES2RenderSystem::_startLegacyV1Rendering()
    {
        glBindVertexArray( mGlobalVao );
    }

    void GLES2RenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd )
    {
        mCurrentVertexBuffer    = cmd->vertexData;
        mCurrentIndexBuffer     = cmd->indexData;

        glBindVertexArray( mGlobalVao );

        v1::VertexBufferBinding *vertexBufferBinding = cmd->vertexData->vertexBufferBinding;
        v1::VertexDeclaration *vertexDeclaration     = cmd->vertexData->vertexDeclaration;

        const v1::VertexDeclaration::VertexElementList& elements = vertexDeclaration->getElements();
        v1::VertexDeclaration::VertexElementList::const_iterator itor;
        v1::VertexDeclaration::VertexElementList::const_iterator end;

        itor = elements.begin();
        end  = elements.end();

        while( itor != end )
        {
            const v1::VertexElement &elem = *itor;

            unsigned short source = elem.getSource();

            VertexElementSemantic semantic = elem.getSemantic();
            GLuint attributeIndex = GLES2VaoManager::getAttributeIndexFor( semantic ) +
                                    elem.getIndex();

            if( !vertexBufferBinding->isBufferBound( source ) )
            {
                OCGE( glDisableVertexAttribArray( attributeIndex ) );
                ++itor;
                continue; // Skip unbound elements.
            }

            v1::HardwareVertexBufferSharedPtr vertexBuffer = vertexBufferBinding->getBuffer( source );
            const v1::GLES2HardwareVertexBuffer* hwGlBuffer =
                            static_cast<v1::GLES2HardwareVertexBuffer*>( vertexBuffer.get() );

            OCGE( glBindBuffer( GL_ARRAY_BUFFER, hwGlBuffer->getGLBufferId() ) );
            void *bindOffset = GL_BUFFER_OFFSET( elem.getOffset() );

            VertexElementType vertexElementType = elem.getType();

            GLint typeCount = v1::VertexElement::getTypeCount( vertexElementType );
            GLboolean normalised = v1::VertexElement::isTypeNormalized( vertexElementType ) ? GL_TRUE :
                                                                                              GL_FALSE;
            switch( vertexElementType )
            {
            case VET_COLOUR:
            case VET_COLOUR_ABGR:
            case VET_COLOUR_ARGB:
                // Because GL takes these as a sequence of single unsigned bytes, count needs to be 4
                // VertexElement::getTypeCount treats them as 1 (RGBA)
                // Also need to normalise the fixed-point data
                typeCount = 4;
                normalised = GL_TRUE;
                break;
            default:
                break;
            };

            assert( (semantic != VES_TEXTURE_COORDINATES || elem.getIndex() < 8) &&
                    "Up to 8 UVs are supported." );

            if( semantic == VES_BINORMAL )
            {
                LogManager::getSingleton().logMessage(
                            "WARNING: VES_BINORMAL will not render properly in "
                            "many GPUs where GL_MAX_VERTEX_ATTRIBS = 16. Consider"
                            " changing for VES_TANGENT with 4 components or use"
                            " QTangents", LML_CRITICAL );
            }

            GLenum type = v1::GLES2HardwareBufferManager::getGLType( vertexElementType );

            switch( v1::VertexElement::getBaseType( vertexElementType ) )
            {
            default:
            case VET_FLOAT1:
                OCGE( glVertexAttribPointer( attributeIndex, typeCount,
                                             type,
                                             normalised,
                                             static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                             bindOffset ) );
                break;
            case VET_BYTE4:
            case VET_UBYTE4:
            case VET_SHORT2:
            case VET_USHORT2:
            case VET_UINT1:
            case VET_INT1:
                OCGE( glVertexAttribIPointer( attributeIndex, typeCount,
                                              type,
                                              static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                              bindOffset ) );
                break;
//            case VET_DOUBLE1:
//                OCGE( glVertexAttribLPointer( attributeIndex, typeCount,
//                                              type,
//                                              static_cast<GLsizei>(vertexBuffer->getVertexSize()),
//                                              bindOffset ) );
                break;
            }

            OCGE( glVertexAttribDivisor( attributeIndex, hwGlBuffer->getInstanceDataStepRate() *
                                         hwGlBuffer->getIsInstanceData() ) );
            OCGE( glEnableVertexAttribArray( attributeIndex ) );

            ++itor;
        }

        if( cmd->indexData )
        {
            v1::GLES2HardwareIndexBuffer *indexBuffer = static_cast<v1::GLES2HardwareIndexBuffer*>(
                                                                    cmd->indexData->indexBuffer.get() );
            OCGE( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getGLBufferId() ) );
        }

        mCurrentPolygonMode = GL_TRIANGLES;
        switch( cmd->operationType )
        {
        case OT_POINT_LIST:
            mCurrentPolygonMode = GL_POINTS;
            break;
        case OT_LINE_LIST:
            mCurrentPolygonMode = /*mUseAdjacency ? GL_LINES_ADJACENCY :*/ GL_LINES;
            break;
        case OT_LINE_STRIP:
            mCurrentPolygonMode = /*mUseAdjacency ? GL_LINE_STRIP_ADJACENCY :*/ GL_LINE_STRIP;
            break;
        default:
        case OT_TRIANGLE_LIST:
            mCurrentPolygonMode = /*mUseAdjacency ? GL_TRIANGLES_ADJACENCY :*/ GL_TRIANGLES;
            break;
        case OT_TRIANGLE_STRIP:
            mCurrentPolygonMode = /*mUseAdjacency ? GL_TRIANGLE_STRIP_ADJACENCY :*/ GL_TRIANGLE_STRIP;
            break;
        case OT_TRIANGLE_FAN:
            mCurrentPolygonMode = GL_TRIANGLE_FAN;
            break;
        }
    }

    void GLES2RenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        assert(false);
//        GLenum indexType = mCurrentIndexBuffer->indexBuffer->getType() ==
//                            v1::HardwareIndexBuffer::IT_16BIT ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
//
//        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();
//
//        OCGE( glDrawElementsInstancedBaseVertexBaseInstance(
//                    mCurrentPolygonMode,
//                    cmd->primCount,
//                    indexType,
//                    reinterpret_cast<void*>( cmd->firstVertexIndex * bytesPerIndexElement ),
//                    cmd->instanceCount,
//                    mCurrentVertexBuffer->vertexStart,
//                    cmd->baseInstance ) );
    }

    void GLES2RenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
        assert(false);
//        OCGE( glDrawArraysInstancedBaseInstance(
//                    mCurrentPolygonMode,
//                    cmd->firstVertexIndex,
//                    cmd->primCount,
//                    cmd->instanceCount,
//                    cmd->baseInstance ) );
    }

    void GLES2RenderSystem::_renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd )
    {
        GLenum indexType = mCurrentIndexBuffer->indexBuffer->getType() ==
                            v1::HardwareIndexBuffer::IT_16BIT ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

        GLSLESProgramCommon* activeProgram = NULL;
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            activeProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
        else
            activeProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();

        OCGE( glUniform1ui( activeProgram->getBaseInstanceLocation(),
                            static_cast<GLuint>( cmd->baseInstance ) ) );

//        if(hasMinGLVersion(3, 2))
//        {
//            OCGE( glDrawElementsInstancedBaseVertex(
//                        mCurrentPolygonMode,
//                        cmd->primCount,
//                        indexType,
//                        reinterpret_cast<void*>(cmd->firstVertexIndex * bytesPerIndexElement),
//                        cmd->instanceCount,
//                        mCurrentVertexBuffer->vertexStart ) );
//        }
//        else
        {
            assert(mCurrentVertexBuffer->vertexStart == 0);
            OCGE( glDrawElementsInstanced(
                        mCurrentPolygonMode,
                        cmd->primCount,
                        indexType,
                        reinterpret_cast<void*>(cmd->firstVertexIndex * bytesPerIndexElement),
                        cmd->instanceCount ) );
        }
    }

    void GLES2RenderSystem::_renderNoBaseInstance( const v1::CbDrawCallStrip *cmd )
    {
        GLSLESProgramCommon* activeProgram = NULL;
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            activeProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
        else
            activeProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();

        OCGE( glUniform1ui( activeProgram->getBaseInstanceLocation(),
                            static_cast<GLuint>( cmd->baseInstance ) ) );

        OCGE( glDrawArraysInstanced(
                    mCurrentPolygonMode,
                    cmd->firstVertexIndex,
                    cmd->primCount,
                    cmd->instanceCount ) );
    }

    void GLES2RenderSystem::clearFrameBuffer(unsigned int buffers,
                                            const ColourValue& colour,
                                            Real depth, unsigned short stencil)
    {
        bool colourMask = mBlendChannelMask != HlmsBlendblock::BlendChannelAll;
        
        GLbitfield flags = 0;
        if (buffers & FBT_COLOUR)
        {
            flags |= GL_COLOR_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            if (colourMask)
            {
                OGRE_CHECK_GL_ERROR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
            }
            OGRE_CHECK_GL_ERROR(glClearColor(colour.r, colour.g, colour.b, colour.a));
        }
        if (buffers & FBT_DEPTH)
        {
            flags |= GL_DEPTH_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            if(!mDepthWrite)
            {
                OGRE_CHECK_GL_ERROR(glDepthMask(GL_TRUE));
            }
            OGRE_CHECK_GL_ERROR(glClearDepthf(depth));
        }
        if (buffers & FBT_STENCIL)
        {
            flags |= GL_STENCIL_BUFFER_BIT;
            // Enable buffer for writing if it isn't
            OGRE_CHECK_GL_ERROR(glStencilMask(0xFFFFFFFF));
            OGRE_CHECK_GL_ERROR(glClearStencil(stencil));
        }

        RenderTarget* target = mActiveViewport->getTarget();
        bool scissorsNeeded = mActiveViewport->getActualLeft() != 0 ||
                                mActiveViewport->getActualTop() != 0 ||
                                mActiveViewport->getActualWidth() != (int)target->getWidth() ||
                                mActiveViewport->getActualHeight() != (int)target->getHeight();

        if( scissorsNeeded )
        {
            //We clear the viewport area. The Viewport may not
            //coincide with the current clipping region
            GLsizei x, y, w, h;
            w = mActiveViewport->getActualWidth();
            h = mActiveViewport->getActualHeight();
            x = mActiveViewport->getActualLeft();
            y = mActiveViewport->getActualTop();

            if( !target->requiresTextureFlipping() )
            {
                // Convert "upper-left" corner to "lower-left"
                y = target->getHeight() - h - y;
            }

            OGRE_CHECK_GL_ERROR(glScissor(x, y, w, h));
        }

        if( scissorsNeeded && !mScissorsEnabled )
        {
            // Clear the buffers
            // Subregion clears need scissort tests enabled.
            OGRE_CHECK_GL_ERROR(glEnable(GL_SCISSOR_TEST));
            OGRE_CHECK_GL_ERROR(glClear(flags));
            OGRE_CHECK_GL_ERROR(glDisable(GL_SCISSOR_TEST));
        }
        else
        {
            // Clear the buffers
            // Either clearing the whole screen, or scissor test is already enabled.
            OGRE_CHECK_GL_ERROR(glClear(flags));
        }

        if( scissorsNeeded )
        {
            //Restore the clipping region
            GLsizei x, y, w, h;
            w = mActiveViewport->getScissorActualWidth();
            h = mActiveViewport->getScissorActualHeight();
            x = mActiveViewport->getScissorActualLeft();
            y = mActiveViewport->getScissorActualTop();

            if( !target->requiresTextureFlipping() )
            {
                // Convert "upper-left" corner to "lower-left"
                y = target->getHeight() - h - y;
            }

            OGRE_CHECK_GL_ERROR(glScissor(x, y, w, h));
        }

        // Reset buffer write state
        if (!mDepthWrite && (buffers & FBT_DEPTH))
        {
            OGRE_CHECK_GL_ERROR(glDepthMask(GL_FALSE));
        }

        if (colourMask && (buffers & FBT_COLOUR))
        {
            GLboolean r = (mBlendChannelMask & HlmsBlendblock::BlendChannelRed) != 0;
            GLboolean g = (mBlendChannelMask & HlmsBlendblock::BlendChannelGreen) != 0;
            GLboolean b = (mBlendChannelMask & HlmsBlendblock::BlendChannelBlue) != 0;
            GLboolean a = (mBlendChannelMask & HlmsBlendblock::BlendChannelAlpha) != 0;
            OCGE( glColorMask( r, g, b, a ) );
        }

        if (buffers & FBT_STENCIL)
        {
            OGRE_CHECK_GL_ERROR(glStencilMask(mStencilParams.writeMask));
        }
    }

    void GLES2RenderSystem::discardFrameBuffer( unsigned int buffers )
    {
        //To GLES2 porting note:
        //GL_EXT_discard_framebuffer does not imply a clear.
        //GL_EXT_discard_framebuffer should be called after rendering
        //(Allows to omit writeback of unneeded data e.g. Z-buffers, Stencil)
        //On most renderers, not clearing (and invalidate is not clearing)
        //can put you in slow mode

        //GL_ARB_invalidate_subdata

        assert( mActiveRenderTarget );
        if( !mHasDiscardFramebuffer )
            return;

        GLsizei numAttachments = 0;
        GLenum attachments[OGRE_MAX_MULTIPLE_RENDER_TARGETS+2];

        GLES2FrameBufferObject *fbo = 0;
        mActiveRenderTarget->getCustomAttribute( "FBO", &fbo );

        if( fbo )
        {
            if( buffers & FBT_COLOUR )
            {
                for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
                {
                    const GLES2SurfaceDesc &surfDesc = fbo->getSurface( i );
                    if( surfDesc.buffer )
                        attachments[numAttachments++] = static_cast<GLenum>( GL_COLOR_ATTACHMENT0 + i );
                }
            }

            GLES2DepthBuffer *depthBuffer = static_cast<GLES2DepthBuffer*>(
                                                mActiveRenderTarget->getDepthBuffer() );

            if( depthBuffer )
            {
                if( buffers & FBT_STENCIL && depthBuffer->getStencilBuffer() )
                    attachments[numAttachments++] = GL_STENCIL_ATTACHMENT;
                if( buffers & FBT_DEPTH )
                    attachments[numAttachments++] = GL_DEPTH_ATTACHMENT;
            }
        }
        else
        {
            if( buffers & FBT_COLOUR )
            {
                attachments[numAttachments++] = GL_COLOR;
                /*attachments[numAttachments++] = GL_BACK_LEFT;
                attachments[numAttachments++] = GL_BACK_RIGHT;*/
            }

            if( buffers & FBT_DEPTH )
                attachments[numAttachments++] = GL_DEPTH;
            if( buffers & FBT_STENCIL )
                attachments[numAttachments++] = GL_STENCIL;
        }

        assert( numAttachments && "Bad flags provided" );
        assert( (size_t)numAttachments <= sizeof(attachments) / sizeof(attachments[0]) );
#if OGRE_NO_GLES3_SUPPORT == 1
        OGRE_CHECK_GL_ERROR(glDiscardFramebufferEXT(GL_FRAMEBUFFER, numAttachments, attachments));
#else
        OGRE_CHECK_GL_ERROR(glInvalidateFramebuffer(GL_FRAMEBUFFER, numAttachments, attachments));
#endif
    }

    void GLES2RenderSystem::_switchContext(GLES2Context *context)
    {
        // Unbind GPU programs and rebind to new context later, because
        // scene manager treat render system as ONE 'context' ONLY, and it
        // cached the GPU programs using state.
        if (mCurrentVertexProgram)
            mCurrentVertexProgram->unbindProgram();
        if (mCurrentFragmentProgram)
            mCurrentFragmentProgram->unbindProgram();
        
        // Disable textures
        _disableTextureUnitsFrom(0);

        // It's ready for switching
        if (mCurrentContext!=context)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            // EAGLContext::setCurrentContext does not flush automatically. everybody else does.
            // see https://developer.apple.com/library/content/qa/qa1612/_index.html
            glFlush();
#endif
            mCurrentContext->endCurrent();
            mCurrentContext = context;
        }
        mCurrentContext->setCurrent();

        // Check if the context has already done one-time initialisation
        if (!mCurrentContext->getInitialized())
        {
            _oneTimeContextInitialization();
            mCurrentContext->setInitialized();
        }

        // Rebind GPU programs to new context
        if( mPso )
        {
            if (mPso->vertexShader)
                mPso->vertexShader->bind();
            if (mPso->pixelShader)
                mPso->pixelShader->bind();
        }
        
        // Must reset depth/colour write mask to according with user desired, otherwise,
        // clearFrameBuffer would be wrong because the value we are recorded may be
        // difference with the really state stored in GL context.
        OGRE_CHECK_GL_ERROR(glDepthMask(mDepthWrite));
        {
            GLboolean r = (mBlendChannelMask & HlmsBlendblock::BlendChannelRed) != 0;
            GLboolean g = (mBlendChannelMask & HlmsBlendblock::BlendChannelGreen) != 0;
            GLboolean b = (mBlendChannelMask & HlmsBlendblock::BlendChannelBlue) != 0;
            GLboolean a = (mBlendChannelMask & HlmsBlendblock::BlendChannelAlpha) != 0;
            OCGE( glColorMask( r, g, b, a ) );
        }
        OGRE_CHECK_GL_ERROR(glStencilMask(mStencilParams.writeMask));
    }

    void GLES2RenderSystem::_unregisterContext(GLES2Context *context)
    {
        if (mCurrentContext == context)
        {
            // Change the context to something else so that a valid context
            // remains active. When this is the main context being unregistered,
            // we set the main context to 0.
            if (mCurrentContext != mMainContext)
            {
                _switchContext(mMainContext);
            }
            else
            {
                // No contexts remain
                mCurrentContext->endCurrent();
                mCurrentContext = 0;
                mMainContext = 0;
            }
        }
    }

    void GLES2RenderSystem::_oneTimeContextInitialization()
    {
        OGRE_CHECK_GL_ERROR(glDisable(GL_DITHER));
        static_cast<GLES2TextureManager*>(mTextureManager)->createWarningTexture();

#if OGRE_NO_GLES3_SUPPORT == 0
        // Enable primitive restarting with fixed indices depending upon the data type
        OGRE_CHECK_GL_ERROR(glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX));
#endif

        if( checkExtension("GL_EXT_texture_filter_anisotropic") )
            OCGE( glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &mLargestSupportedAnisotropy) );
        
        OCGE( glGenFramebuffers( 1, &mNullColourFramebuffer ) );
    }

    void GLES2RenderSystem::initialiseContext(RenderWindow* primary)
    {
        // Set main and current context
        mMainContext = 0;
        primary->getCustomAttribute("GLCONTEXT", &mMainContext);
        mCurrentContext = mMainContext;

        // Set primary context as active
        if (mCurrentContext)
            mCurrentContext->setCurrent();

        // Setup GLSupport
        mGLSupport->initialiseExtensions();

        LogManager::getSingleton().logMessage("**************************************");
        LogManager::getSingleton().logMessage("*** OpenGL ES 2.x Renderer Started ***");
        LogManager::getSingleton().logMessage("**************************************");
    }

    void GLES2RenderSystem::_setRenderTarget( RenderTarget *target, uint8 viewportRenderTargetFlags )
    {
        // Unbind frame buffer object
        if(mActiveRenderTarget && mRTTManager)
            mRTTManager->unbind(mActiveRenderTarget);

        mActiveRenderTarget = target;
        if (target && mRTTManager)
        {
            // Switch context if different from current one
            GLES2Context *newContext = 0;
            target->getCustomAttribute("GLCONTEXT", &newContext);
            if (newContext && mCurrentContext != newContext)
            {
                _switchContext(newContext);
            }

            // Check the FBO's depth buffer status
            GLES2DepthBuffer *depthBuffer = static_cast<GLES2DepthBuffer*>(target->getDepthBuffer());

            if( target->getDepthBufferPool() != DepthBuffer::POOL_NO_DEPTH &&
                (!depthBuffer || depthBuffer->getGLContext() != mCurrentContext ) )
            {
                // Depth is automatically managed and there is no depth buffer attached to this RT
                // or the Current context doesn't match the one this Depth buffer was created with
                setDepthBufferFor( target, true );
            }

            depthBuffer = static_cast<GLES2DepthBuffer*>(target->getDepthBuffer());

            if( target->getForceDisableColourWrites() )
                viewportRenderTargetFlags &= ~VP_RTT_COLOUR_WRITE;

            // Bind frame buffer object
            mRTTManager->bind(target);

            if( !(viewportRenderTargetFlags & VP_RTT_COLOUR_WRITE) )
            {
                assert(!target->isRenderWindow());
                assert(depthBuffer);
                
                OCGE( glBindFramebuffer( GL_FRAMEBUFFER, mNullColourFramebuffer ) );

                if( depthBuffer )
                {
                    //Attach the depth buffer to this no-colour framebuffer
                    depthBuffer->bindToFramebuffer();
                }
            }
            else
            {
                // ...
            }
            
        }
    }

    GLint GLES2RenderSystem::convertCompareFunction(CompareFunction func) const
    {
        switch(func)
        {
            case CMPF_ALWAYS_FAIL:
                return GL_NEVER;
            case CMPF_ALWAYS_PASS:
                return GL_ALWAYS;
            case CMPF_LESS:
                return GL_LESS;
            case CMPF_LESS_EQUAL:
                return GL_LEQUAL;
            case CMPF_EQUAL:
                return GL_EQUAL;
            case CMPF_NOT_EQUAL:
                return GL_NOTEQUAL;
            case CMPF_GREATER_EQUAL:
                return GL_GEQUAL;
            case CMPF_GREATER:
                return GL_GREATER;
            case NUM_COMPARE_FUNCTIONS:
                return GL_ALWAYS; // To keep compiler happy
        };
        // To keep compiler happy
        return GL_ALWAYS;
    }

    GLint GLES2RenderSystem::convertStencilOp(StencilOperation op, bool invert) const
    {
        switch(op)
        {
        case SOP_KEEP:
            return GL_KEEP;
        case SOP_ZERO:
            return GL_ZERO;
        case SOP_REPLACE:
            return GL_REPLACE;
        case SOP_INCREMENT:
            return invert ? GL_DECR : GL_INCR;
        case SOP_DECREMENT:
            return invert ? GL_INCR : GL_DECR;
        case SOP_INCREMENT_WRAP:
            return invert ? GL_DECR_WRAP : GL_INCR_WRAP;
        case SOP_DECREMENT_WRAP:
            return invert ? GL_INCR_WRAP : GL_DECR_WRAP;
        case SOP_INVERT:
            return GL_INVERT;
        };
        // to keep compiler happy
        return SOP_KEEP;
    }

    void GLES2RenderSystem::bindGpuProgramParameters(GpuProgramType gptype, GpuProgramParametersSharedPtr params, uint16 mask)
    {
        // Just copy
        params->_copySharedParams();
        switch (gptype)
        {
            case GPT_VERTEX_PROGRAM:
                mActiveVertexGpuProgramParameters = params;
                mPso->vertexShader->bindSharedParameters(params, mask);
                break;
            case GPT_FRAGMENT_PROGRAM:
                mActiveFragmentGpuProgramParameters = params;
                mPso->pixelShader->bindSharedParameters(params, mask);
                break;
            default:
                break;
        }

        switch (gptype)
        {
            case GPT_VERTEX_PROGRAM:
                mActiveVertexGpuProgramParameters = params;
                mPso->vertexShader->bindParameters(params, mask);
                break;
            case GPT_FRAGMENT_PROGRAM:
                mActiveFragmentGpuProgramParameters = params;
                mPso->pixelShader->bindParameters(params, mask);
                break;
            default:
                break;
        }
    }

    void GLES2RenderSystem::bindGpuProgramPassIterationParameters(GpuProgramType gptype)
    {
        switch (gptype)
        {
            case GPT_VERTEX_PROGRAM:
                mPso->vertexShader->bindPassIterationParameters(mActiveVertexGpuProgramParameters);
                break;
            case GPT_FRAGMENT_PROGRAM:
                mPso->pixelShader->bindPassIterationParameters(mActiveFragmentGpuProgramParameters);
                break;
            default:
                break;
        }
    }

    void GLES2RenderSystem::registerThread()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // This is only valid once we've created the main context
        if (!mMainContext)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Cannot register a background thread before the main context "
                        "has been created.",
                        "GLES2RenderSystem::registerThread");
        }

        // Create a new context for this thread. Cloning from the main context
        // will ensure that resources are shared with the main context
        // We want a separate context so that we can safely create GL
        // objects in parallel with the main thread
        GLES2Context* newContext = mMainContext->clone();
        mBackgroundContextList.push_back(newContext);

        // Bind this new context to this thread.
        newContext->setCurrent();

        _oneTimeContextInitialization();
        newContext->setInitialized();
    }

    void GLES2RenderSystem::unregisterThread()
    {
        // nothing to do here?
        // Don't need to worry about active context, just make sure we delete
        // on shutdown.
    }

    void GLES2RenderSystem::preExtraThreadsStarted()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // free context, we'll need this to share lists
        if(mCurrentContext)
            mCurrentContext->endCurrent();
    }

    void GLES2RenderSystem::postExtraThreadsStarted()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // reacquire context
        if(mCurrentContext)
            mCurrentContext->setCurrent();
    }

    unsigned int GLES2RenderSystem::getDisplayMonitorCount() const
    {
        return 1;
    }

    //---------------------------------------------------------------------
    void GLES2RenderSystem::beginProfileEvent( const String &eventName )
    {
        if(checkExtension("GL_EXT_debug_marker"))
            glPushGroupMarkerEXT(0, eventName.c_str());
    }
    //---------------------------------------------------------------------
    void GLES2RenderSystem::endProfileEvent()
    {
        if(checkExtension("GL_EXT_debug_marker"))
            glPopGroupMarkerEXT();
    }
    //---------------------------------------------------------------------
    void GLES2RenderSystem::markProfileEvent( const String &eventName )
    {
        if( eventName.empty() )
            return;

        if(checkExtension("GL_EXT_debug_marker"))
           glInsertEventMarkerEXT(0, eventName.c_str());
    }
    //---------------------------------------------------------------------
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    void GLES2RenderSystem::resetRenderer(RenderWindow* win)
    {
        LogManager::getSingleton().logMessage("********************************************");
        LogManager::getSingleton().logMessage("*** OpenGL ES 2.x Reset Renderer Started ***");
        LogManager::getSingleton().logMessage("********************************************");
                
        initialiseContext(win);
        
        static_cast<GLES2FBOManager*>(mRTTManager)->_reload();
        
        _destroyDepthBuffer(win);
        
        GLES2DepthBuffer *depthBuffer = OGRE_NEW GLES2DepthBuffer( DepthBuffer::POOL_DEFAULT, this,
                                                                  mMainContext, GL_NONE, GL_NONE,
                                                                  win->getWidth(), win->getHeight(),
                                                                  win->getFSAA(), 0, PF_UNKNOWN,
                                                                  false, true );
        
        mDepthBufferPool[depthBuffer->getPoolId()].push_back( depthBuffer );
        win->attachDepthBuffer( depthBuffer, false );
        
        GLES2RenderSystem::mResourceManager->notifyOnContextReset();
        
        _setViewport(NULL);
        _setRenderTarget(win);
    }
    
    GLES2ManagedResourceManager* GLES2RenderSystem::getResourceManager()
    {
        return GLES2RenderSystem::mResourceManager;
    }
#endif

    void GLES2RenderSystem::initGPUProfiling()
    {
    }

    void GLES2RenderSystem::deinitGPUProfiling()
    {
    }

    void GLES2RenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache )
    {
    }

    void GLES2RenderSystem::endGPUSampleProfile( const String &name )
    {
    }


    bool GLES2RenderSystem::activateGLTextureUnit(size_t unit)
    {
        // Always return true for the currently bound texture unit
        if (mActiveTextureUnit == unit)
            return true;
        
        if (unit < dynamic_cast<GLES2RenderSystem*>(Root::getSingleton().getRenderSystem())->getCapabilities()->getNumTextureUnits())
        {
            OGRE_CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0 + unit));
            
            mActiveTextureUnit = unit;
            
            return true;
        }
        else
        {
            return false;
        }
    }
    
    void GLES2RenderSystem::bindVertexElementToGpu( const v1::VertexElement &elem,
                                                    v1::HardwareVertexBufferSharedPtr vertexBuffer,
                                                    const size_t vertexStart,
                                                    vector<GLuint>::type &attribsBound,
                                                    vector<GLuint>::type &instanceAttribsBound,
                                                    bool updateVAO)
    {
        const v1::GLES2HardwareVertexBuffer* hwGlBuffer = static_cast<const v1::GLES2HardwareVertexBuffer*>(vertexBuffer.get());

        // FIXME: Having this commented out fixes some rendering issues but leaves VAO's useless
        if (updateVAO)
        {
            OGRE_CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER,
                                             hwGlBuffer->getGLBufferId()));
            void* pBufferData = GL_BUFFER_OFFSET(elem.getOffset());

            if (vertexStart)
            {
                pBufferData = static_cast<char*>(pBufferData) + vertexStart * vertexBuffer->getVertexSize();
            }

            VertexElementSemantic sem = elem.getSemantic();
            unsigned short typeCount = v1::VertexElement::getTypeCount(elem.getType());
            GLboolean normalised = GL_FALSE;
            GLuint attrib = 0;
            unsigned short elemIndex = elem.getIndex();

            if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                GLSLESProgramPipeline* programPipeline =
                GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
                if (!programPipeline || !programPipeline->isAttributeValid(sem, elemIndex))
                {
                    return;
                }

                attrib = (GLuint)programPipeline->getAttributeIndex(sem, elemIndex);
            }
            else
            {
                GLSLESLinkProgram* linkProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
                if (!linkProgram || !linkProgram->isAttributeValid(sem, elemIndex))
                {
                    return;
                }

                attrib = (GLuint)linkProgram->getAttributeIndex(sem, elemIndex);
            }

            if(hasMinGLVersion(3, 0) || checkExtension("GL_EXT_instanced_arrays"))
            {
                if (mPso->vertexShader)
                {
                    if (hwGlBuffer->getIsInstanceData())
                    {
                        OGRE_CHECK_GL_ERROR(glVertexAttribDivisorEXT(attrib, static_cast<GLuint>(hwGlBuffer->getInstanceDataStepRate())));
                        instanceAttribsBound.push_back(attrib);
                    }
                }
            }

            switch(elem.getType())
            {
                case VET_COLOUR:
                case VET_COLOUR_ABGR:
                case VET_COLOUR_ARGB:
                    // Because GL takes these as a sequence of single unsigned bytes, count needs to be 4
                    // VertexElement::getTypeCount treats them as 1 (RGBA)
                    // Also need to normalise the fixed-point data
                    typeCount = 4;
                    normalised = GL_TRUE;
                    break;
                default:
                    break;
            };

            OGRE_CHECK_GL_ERROR(glVertexAttribPointer(attrib,
                                                      typeCount,
                                                      v1::GLES2HardwareBufferManager::getGLType(elem.getType()),
                                                      normalised,
                                                      static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                                      pBufferData));

            // If this attribute hasn't been enabled, do so and keep a record of it.
            OGRE_CHECK_GL_ERROR(glEnableVertexAttribArray(attrib));
            
            attribsBound.push_back(attrib);
        }
    }

    const PixelFormatToShaderType* GLES2RenderSystem::getPixelFormatToShaderType() const
    {
        return &mPixelFormatToShaderType;
    }
}
