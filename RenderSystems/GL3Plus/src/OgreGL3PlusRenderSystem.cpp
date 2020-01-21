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

#include "OgreGL3PlusRenderSystem.h"
#include "OgreRenderSystem.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreLight.h"
#include "OgreCamera.h"
#include "OgreGL3PlusHardwareCounterBuffer.h"
#include "OgreGL3PlusHardwareUniformBuffer.h"
#include "OgreGL3PlusHardwareShaderStorageBuffer.h"
#include "OgreGL3PlusHardwareVertexBuffer.h"
#include "OgreGL3PlusHardwareIndexBuffer.h"
#include "OgreGL3PlusDefaultHardwareBufferManager.h"
#include "OgreGL3PlusUtil.h"
#include "OgreGLSLShader.h"
#include "OgreGLSLShaderManager.h"
#include "OgreException.h"
#include "OgreGLSLExtSupport.h"
#include "OgreGL3PlusDescriptorSetTexture.h"
#include "OgreGL3PlusHardwareOcclusionQuery.h"
#include "OgreGL3PlusContext.h"
#include "OgreGLSLShaderFactory.h"
#include "OgreGL3PlusHardwareBufferManager.h"
#include "OgreGLSLSeparableProgramManager.h"
#include "OgreGLSLSeparableProgram.h"
#include "OgreGLSLMonolithicProgramManager.h"
#include "OgreGL3PlusVertexArrayObject.h"
#include "OgreGL3PlusTextureGpuManager.h"
#include "OgreGL3PlusTextureGpu.h"
#include "OgreGL3PlusMappings.h"
#include "OgreGL3PlusRenderPassDescriptor.h"
#include "OgreGL3PlusHlmsPso.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsSamplerblock.h"
#include "Vao/OgreGL3PlusVaoManager.h"
#include "Vao/OgreGL3PlusVertexArrayObject.h"
#include "Vao/OgreGL3PlusBufferInterface.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreUavBufferPacked.h"
#include "CommandBuffer/OgreCbDrawCall.h"
#include "OgreRoot.h"
#include "OgreConfig.h"
#include "OgreViewport.h"
#include "OgreDepthBuffer.h"
#include "OgreWindow.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreProfiler.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
extern "C" void glFlushRenderAPPLE();
#endif

#if OGRE_DEBUG_MODE
static void APIENTRY GLDebugCallback(GLenum source,
                                     GLenum type,
                                     GLuint id,
                                     GLenum severity,
                                     GLsizei length,
                                     const GLchar* message,
                                     const void* userParam)
{
    if( type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP )
        return; //Ignore these

    char debSource[32], debType[32], debSev[32];

    if (source == GL_DEBUG_SOURCE_API)
        strcpy(debSource, "OpenGL");
    else if (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM)
        strcpy(debSource, "Windows");
    else if (source == GL_DEBUG_SOURCE_SHADER_COMPILER)
        strcpy(debSource, "Shader Compiler");
    else if (source == GL_DEBUG_SOURCE_THIRD_PARTY)
        strcpy(debSource, "Third Party");
    else if (source == GL_DEBUG_SOURCE_APPLICATION)
        strcpy(debSource, "Application");
    else if (source == GL_DEBUG_SOURCE_OTHER)
        strcpy(debSource, "Other");

    if (type == GL_DEBUG_TYPE_ERROR)
        strcpy(debType, "error");
    else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
        strcpy(debType, "deprecated behavior");
    else if (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        strcpy(debType, "undefined behavior");
    else if (type == GL_DEBUG_TYPE_PORTABILITY)
        strcpy(debType, "portability");
    else if (type == GL_DEBUG_TYPE_PERFORMANCE)
        strcpy(debType, "performance");
    else if (type == GL_DEBUG_TYPE_OTHER)
        strcpy(debType, "message");
    else if (type == GL_DEBUG_TYPE_PUSH_GROUP)
        strcpy(debType, "push");
    else if (type == GL_DEBUG_TYPE_POP_GROUP)
        strcpy(debType, "pop");

    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        strcpy(debSev, "high");
    }
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        strcpy(debSev, "medium");
    else if (severity == GL_DEBUG_SEVERITY_LOW)
        strcpy(debSev, "low");
    else if( severity == GL_DEBUG_SEVERITY_NOTIFICATION )
    {
        strcpy(debSev, "notification");

#if OGRE_PROFILING == 0
        //Filter notification debug GL messages as they can
        //be quite noisy, annoying and useless on NVIDIA.
        return;
#endif
    }
    else
        strcpy(debSev, "unknown");

    Ogre::LogManager::getSingleton().stream() << debSource << ":" << debType << "(" << debSev << ") " << id << ": " << message;
}
#endif

namespace Ogre {

    static bool g_hasDebugObjectLabel = false;
    void ogreGlObjectLabel( GLenum identifier, GLuint name, GLsizei length, const GLchar *label )
    {
        if( g_hasDebugObjectLabel )
        {
            OCGE( glObjectLabel( identifier, name, length, label ) );
        }
    }
    void ogreGlObjectLabel( GLenum identifier, GLuint name, const String &label )
    {
        ogreGlObjectLabel( identifier, name, label.size(), label.c_str() );
    }

    GL3PlusRenderSystem::GL3PlusRenderSystem()
        : mBlendChannelMask( HlmsBlendblock::BlendChannelAll ),
          mDepthWrite(true),
          mScissorsEnabled(false),
          mGlobalVao( 0 ),
          mCurrentVertexBuffer( 0 ),
          mCurrentIndexBuffer( 0 ),
          mCurrentPolygonMode( GL_TRIANGLES ),
          mShaderManager(0),
          mGLSLShaderFactory(0),
          mHardwareBufferManager(0),
          mActiveTextureUnit(0),
          mHasArbInvalidateSubdata( false ),
          mNullColourFramebuffer( 0 )
    {
        size_t i;

        LogManager::getSingleton().logMessage(getName() + " created.");

        mRenderAttribsBound.reserve(100);
        mRenderInstanceAttribsBound.reserve(100);

        // Get our GLSupport
        mGLSupport = Ogre::getGLSupport();

        mWorldMatrix = Matrix4::IDENTITY;
        mViewMatrix = Matrix4::IDENTITY;

        initConfigOptions();

        for (i = 0; i < OGRE_MAX_TEXTURE_LAYERS; i++)
        {
            // Dummy value
            mTextureTypes[i] = GL_TEXTURE_2D;
        }

        mCurrentContext = 0;
        mMainContext = 0;
        mGLInitialised = false;
        mUseAdjacency = false;
        mTextureMipmapCount = 0;
        mMinFilter = FO_LINEAR;
        mMipFilter = FO_POINT;
        mSwIndirectBufferPtr = 0;
        mFirstUavBoundSlot = 255;
        mLastUavBoundPlusOne = 0;
        mClipDistances = 0;
        mPso = 0;
        mCurrentComputeShader = 0;
        mLargestSupportedAnisotropy = 1;
    }

    GL3PlusRenderSystem::~GL3PlusRenderSystem()
    {
        shutdown();

        if (mGLSupport)
            OGRE_DELETE mGLSupport;
    }

    const String& GL3PlusRenderSystem::getName(void) const
    {
        static String strName("OpenGL 3+ Rendering Subsystem");
        return strName;
    }

    const String& GL3PlusRenderSystem::getFriendlyName(void) const
    {
        static String strName("OpenGL 3+");
        return strName;
    }

    void GL3PlusRenderSystem::initConfigOptions(void)
    {
        mGLSupport->addConfig();
    }

    ConfigOptionMap& GL3PlusRenderSystem::getConfigOptions(void)
    {
        return mGLSupport->getConfigOptions();
    }

    void GL3PlusRenderSystem::setConfigOption(const String &name, const String &value)
    {
        mGLSupport->setConfigOption(name, value);
    }

    String GL3PlusRenderSystem::validateConfigOptions(void)
    {
        // XXX Return an error string if something is invalid
        return mGLSupport->validateConfig();
    }

    Window* GL3PlusRenderSystem::_initialise( bool autoCreateWindow,
                                              const String &windowTitle )
    {
        mGLSupport->start();

        Window *autoWindow = mGLSupport->createWindow( autoCreateWindow, this, windowTitle );
        RenderSystem::_initialise(autoCreateWindow, windowTitle);
        return autoWindow;
    }

    RenderSystemCapabilities* GL3PlusRenderSystem::createRenderSystemCapabilities() const
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

        bool hasGL40 = mGLSupport->hasMinGLVersion(4, 0);
        bool hasGL41 = mGLSupport->hasMinGLVersion(4, 1);
        bool hasGL42 = mGLSupport->hasMinGLVersion(4, 2);

        // Check for hardware mipmapping support.
        bool disableAutoMip = false;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || \
    OGRE_PLATFORM == OGRE_PLATFORM_LINUX || \
    OGRE_PLATFORM == OGRE_PLATFORM_FREEBSD
        // Apple & Linux ATI drivers have faults in hardware mipmap generation
        // TODO: Test this with GL3+
        if (rsc->getVendor() == GPU_AMD)
            disableAutoMip = true;
#endif
        // The Intel 915G frequently corrupts textures when using hardware mip generation
        // I'm not currently sure how many generations of hardware this affects,
        // so for now, be safe.
        if (rsc->getVendor() == GPU_INTEL)
            disableAutoMip = true;

        if (!disableAutoMip)
            rsc->setCapability(RSC_AUTOMIPMAP);

        // Check for blending support
        rsc->setCapability(RSC_BLENDING);

        // Multitexturing support and set number of texture units
        GLint units;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units));
        rsc->setNumTextureUnits(std::min<ushort>(16, units));

        // Check for Anisotropy support
        if (mGLSupport->checkExtension("GL_EXT_texture_filter_anisotropic"))
            rsc->setCapability(RSC_ANISOTROPY);

        // DOT3 support is standard
        rsc->setCapability(RSC_DOT3);

        // Cube map
        rsc->setCapability(RSC_CUBEMAPPING);

        // Point sprites
        rsc->setCapability(RSC_POINT_SPRITES);
        rsc->setCapability(RSC_POINT_EXTENDED_PARAMETERS);

        // Check for hardware stencil support and set bit depth
        rsc->setCapability(RSC_HWSTENCIL);
        rsc->setCapability(RSC_TWO_SIDED_STENCIL);
        rsc->setStencilBufferBitDepth(8);

        rsc->setCapability(RSC_HW_GAMMA);
        rsc->setCapability(RSC_TEXTURE_SIGNED_INT);

        // Vertex Buffer Objects are always supported
        rsc->setCapability(RSC_VBO);
        rsc->setCapability(RSC_32BIT_INDEX);

        // Vertex Array Objects are supported in 3.0
        rsc->setCapability(RSC_VAO);

        // Check for texture compression
        rsc->setCapability(RSC_TEXTURE_COMPRESSION);

        // Check for dxt compression
        if (mGLSupport->checkExtension("GL_EXT_texture_compression_s3tc"))
        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_DXT);
        }

        // Check for etc compression
        if (mGLSupport->checkExtension("GL_ARB_ES3_compatibility") || mHasGL43)
        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_ETC2);
        }

        if( mGLSupport->checkExtension( "GL_KHR_texture_compression_astc_ldr" ) )
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_ASTC);

        // Check for vtc compression
        if (mGLSupport->checkExtension("GL_NV_texture_compression_vtc"))
        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_VTC);
        }

        // RGTC(BC4/BC5) is supported by the 3.0 spec
        rsc->setCapability(RSC_TEXTURE_COMPRESSION_BC4_BC5);

        // BPTC(BC6H/BC7) is supported by the extension or OpenGL 4.2 or higher
        if (mGLSupport->checkExtension("GL_ARB_texture_compression_bptc") || hasGL42)
        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7);
        }

        // Supported since GL 3.2; our minimum is GL 3.3
        rsc->setCapability(RSC_MSAA_2D_ARRAY);

        //Technically D3D10.1 hardware (GL3) supports gather and exposes this extension.
        //However we have bug reports that textureGather isn't working properly, and
        //sadly these cards no longer receive updates. So, assume modern cards and
        //modern drivers are needed.
        //https://bitbucket.org/sinbad/ogre/commits/c76e1bedfed65d0f9dc45353d432a26f278cc968#comment-1776416
        //if( mGLSupport->checkExtension("GL_ARB_texture_gather") || mHasGL40 )
        if( mHasGL43 )
            rsc->setCapability(RSC_TEXTURE_GATHER);

        if( mHasGL43 || (mGLSupport->checkExtension("GL_ARB_shader_image_load_store") &&
                         mGLSupport->checkExtension("GL_ARB_shader_storage_buffer_object")) )
        {
            rsc->setCapability(RSC_UAV);
            rsc->setCapability(RSC_TYPED_UAV_LOADS);
        }

        rsc->setCapability(RSC_FBO);
        rsc->setCapability(RSC_HWRENDER_TO_TEXTURE);
        // Probe number of draw buffers
        // Only makes sense with FBO support, so probe here
        GLint buffers;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_MAX_DRAW_BUFFERS, &buffers));
        rsc->setNumMultiRenderTargets(std::min<int>(buffers, (GLint)OGRE_MAX_MULTIPLE_RENDER_TARGETS));
        rsc->setCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS);

        // Stencil wrapping
        rsc->setCapability(RSC_STENCIL_WRAP);

        // GL always shares vertex and fragment texture units (for now?)
        rsc->setVertexTextureUnitsShared(true);

        // Blending support
        rsc->setCapability(RSC_BLENDING);

        // Check for non-power-of-2 texture support
        rsc->setCapability(RSC_NON_POWER_OF_2_TEXTURES);

        // Check for atomic counter support
        if (mGLSupport->checkExtension("GL_ARB_shader_atomic_counters") || hasGL42)
            rsc->setCapability(RSC_ATOMIC_COUNTERS);

        // As are user clipping planes
        rsc->setCapability(RSC_USER_CLIP_PLANES);

        // So are 1D & 3D textures
        rsc->setCapability(RSC_TEXTURE_1D);
        rsc->setCapability(RSC_TEXTURE_3D);

        rsc->setCapability(RSC_TEXTURE_2D_ARRAY);

        if( mDriverVersion.major >= 4 || mGLSupport->checkExtension("GL_ARB_texture_cube_map_array") )
            rsc->setCapability(RSC_TEXTURE_CUBE_MAP_ARRAY);

        // UBYTE4 always supported
        rsc->setCapability(RSC_VERTEX_FORMAT_UBYTE4);

        // Infinite far plane always supported
        rsc->setCapability(RSC_INFINITE_FAR_PLANE);

        // Check for hardware occlusion support
        rsc->setCapability(RSC_HWOCCLUSION);

        GLint maxRes2d, maxRes3d, maxResCube;
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_TEXTURE_SIZE,            &maxRes2d ) );
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_3D_TEXTURE_SIZE,         &maxRes3d ) );
        OGRE_CHECK_GL_ERROR( glGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE,   &maxResCube ) );

        rsc->setMaximumResolutions( static_cast<ushort>(maxRes2d), static_cast<ushort>(maxRes3d),
                                    static_cast<ushort>(maxResCube) );

        // Point size
        GLfloat psRange[2] = {0.0, 0.0};
        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_POINT_SIZE_RANGE, psRange));
        rsc->setMaxPointSize(psRange[1]);

        // GLSL is always supported in GL
        // TODO: Deprecate this profile name in favor of versioned names
        rsc->addShaderProfile("glsl");

        // Support for specific shader profiles
        if (getNativeShadingLanguageVersion() >= 440)
            rsc->addShaderProfile("glsl440");
        if (getNativeShadingLanguageVersion() >= 430)
            rsc->addShaderProfile("glsl430");
        if (getNativeShadingLanguageVersion() >= 420)
            rsc->addShaderProfile("glsl420");
        if (getNativeShadingLanguageVersion() >= 410)
            rsc->addShaderProfile("glsl410");
        if (getNativeShadingLanguageVersion() >= 400)
            rsc->addShaderProfile("glsl400");
        if (getNativeShadingLanguageVersion() >= 330)
            rsc->addShaderProfile("glsl330");
        if (getNativeShadingLanguageVersion() >= 150)
            rsc->addShaderProfile("glsl150");
        if (getNativeShadingLanguageVersion() >= 140)
            rsc->addShaderProfile("glsl140");
        if (getNativeShadingLanguageVersion() >= 130)
            rsc->addShaderProfile("glsl130");

        // FIXME: This isn't working right yet in some rarer cases
        /*if (mGLSupport->checkExtension("GL_ARB_separate_shader_objects") || hasGL41)
            rsc->setCapability(RSC_SEPARATE_SHADER_OBJECTS);*/

        // Vertex/Fragment Programs
        rsc->setCapability(RSC_VERTEX_PROGRAM);
        rsc->setCapability(RSC_FRAGMENT_PROGRAM);

        GLfloat floatConstantCount = 0;
        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &floatConstantCount));
        rsc->setVertexProgramConstantFloatCount((Ogre::ushort)floatConstantCount);
        rsc->setVertexProgramConstantBoolCount((Ogre::ushort)floatConstantCount);
        rsc->setVertexProgramConstantIntCount((Ogre::ushort)floatConstantCount);

        // Fragment Program Properties
        floatConstantCount = 0;
        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &floatConstantCount));
        rsc->setFragmentProgramConstantFloatCount((Ogre::ushort)floatConstantCount);
        rsc->setFragmentProgramConstantBoolCount((Ogre::ushort)floatConstantCount);
        rsc->setFragmentProgramConstantIntCount((Ogre::ushort)floatConstantCount);

        // Geometry Program Properties
        rsc->setCapability(RSC_GEOMETRY_PROGRAM);

        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &floatConstantCount));
        rsc->setGeometryProgramConstantFloatCount(floatConstantCount);

        GLint maxOutputVertices;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxOutputVertices));
        rsc->setGeometryProgramNumOutputVertices(maxOutputVertices);

        //FIXME Is this correct?
        OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &floatConstantCount));
        rsc->setGeometryProgramConstantFloatCount(floatConstantCount);
        rsc->setGeometryProgramConstantBoolCount(floatConstantCount);
        rsc->setGeometryProgramConstantIntCount(floatConstantCount);

        // Tessellation Program Properties
        if (mGLSupport->checkExtension("GL_ARB_tessellation_shader") || hasGL40)
        {
            rsc->setCapability(RSC_TESSELLATION_HULL_PROGRAM);
            rsc->setCapability(RSC_TESSELLATION_DOMAIN_PROGRAM);

            OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, &floatConstantCount));
            // 16 boolean params allowed
            rsc->setTessellationHullProgramConstantBoolCount(floatConstantCount);
            // 16 integer params allowed, 4D
            rsc->setTessellationHullProgramConstantIntCount(floatConstantCount);
            // float params, always 4D
            rsc->setTessellationHullProgramConstantFloatCount(floatConstantCount);

            OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, &floatConstantCount));
            // 16 boolean params allowed
            rsc->setTessellationDomainProgramConstantBoolCount(floatConstantCount);
            // 16 integer params allowed, 4D
            rsc->setTessellationDomainProgramConstantIntCount(floatConstantCount);
            // float params, always 4D
            rsc->setTessellationDomainProgramConstantFloatCount(floatConstantCount);
        }

        // Compute Program Properties
        if (mGLSupport->checkExtension("GL_ARB_compute_shader") || mHasGL43)
        {
            rsc->setCapability(RSC_COMPUTE_PROGRAM);

            //FIXME Is this correct?
            OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &floatConstantCount));
            rsc->setComputeProgramConstantFloatCount(floatConstantCount);
            rsc->setComputeProgramConstantBoolCount(floatConstantCount);
            rsc->setComputeProgramConstantIntCount(floatConstantCount);

            //TODO we should also check max workgroup count & size
            // OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_COMPUTE_WORK_GROUP_SIZE, &workgroupCount));
            // OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workgroupInvocations));
        }

        if (mGLSupport->checkExtension("GL_ARB_get_program_binary") || hasGL41)
        {
            GLint formats;
            OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats));

            if (formats > 0)
                rsc->setCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER);
        }

        rsc->setCapability(RSC_VERTEX_BUFFER_INSTANCE_DATA);

        // Check for Float textures
        rsc->setCapability(RSC_TEXTURE_FLOAT);

        // OpenGL 3.0 requires a minimum of 16 texture image units
        units = std::max<GLint>(16, units);

        rsc->setNumVertexTextureUnits(static_cast<ushort>(units));
        if (units > 0)
        {
            rsc->setCapability(RSC_VERTEX_TEXTURE_FETCH);
        }

        // Mipmap LOD biasing?
        rsc->setCapability(RSC_MIPMAP_LOD_BIAS);

        // Alpha to coverage always 'supported' when MSAA is available
        // although card may ignore it if it doesn't specifically support A2C
        rsc->setCapability(RSC_ALPHA_TO_COVERAGE);

        // Check if render to vertex buffer (transform feedback in OpenGL)
        rsc->setCapability(RSC_HWRENDER_TO_VERTEX_BUFFER);

        rsc->setCapability(RSC_EXPLICIT_FSAA_RESOLVE);

        if( mDriverVersion.hasMinVersion( 4, 2 ) ||
            mGLSupport->checkExtension("GL_ARB_shading_language_420pack" ) )
        {
            rsc->setCapability(RSC_CONST_BUFFER_SLOTS_IN_SHADER);
        }

        return rsc;
    }

    void GL3PlusRenderSystem::initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps,
                                                                     Window* primary)
    {
        if (caps->getRenderSystemName() != getName())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Trying to initialize GL3PlusRenderSystem from RenderSystemCapabilities that do not support OpenGL 3+",
                        "GL3PlusRenderSystem::initialiseFromRenderSystemCapabilities");
        }

        mShaderManager = OGRE_NEW GLSLShaderManager();

        // Create GLSL shader factory
        mGLSLShaderFactory = new GLSLShaderFactory(*mGLSupport);
        HighLevelGpuProgramManager::getSingleton().addFactory(mGLSLShaderFactory);

        // Set texture the number of texture units
        mFixedFunctionTextureUnits = caps->getNumTextureUnits();

        // Use VBO's by default
        mHardwareBufferManager = new v1::GL3PlusHardwareBufferManager();

        caps->setCapability(RSC_RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL);

        Log* defaultLog = LogManager::getSingleton().getDefaultLog();
        if (defaultLog)
        {
            caps->log(defaultLog);
            defaultLog->logMessage(
                " * Using Reverse Z: " + StringConverter::toString( mReverseDepth, true ) );
        }

        /*if (caps->hasCapability(RSC_CAN_GET_COMPILED_SHADER_BUFFER))
        {
            // Enable microcache
            mShaderManager->setSaveMicrocodesToCache(true);
        }*/

        mGLInitialised = true;
    }

    void GL3PlusRenderSystem::reinitialise(void)
    {
        this->shutdown();
        this->_initialise(true);
    }

    void GL3PlusRenderSystem::shutdown(void)
    {
        RenderSystem::shutdown();

        // Deleting the GLSL program factory
        if (mGLSLShaderFactory)
        {
            // Remove from manager safely
            if (HighLevelGpuProgramManager::getSingletonPtr())
                HighLevelGpuProgramManager::getSingleton().removeFactory(mGLSLShaderFactory);
            OGRE_DELETE mGLSLShaderFactory;
            mGLSLShaderFactory = 0;
        }

        // Deleting the GPU program manager and hardware buffer manager.  Has to be done before the mGLSupport->stop().
        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;

        // Delete extra threads contexts
        for (GL3PlusContextList::iterator i = mBackgroundContextList.begin();
             i != mBackgroundContextList.end(); ++i)
        {
            GL3PlusContext* pCurContext = *i;

            pCurContext->releaseContext();

            OGRE_DELETE pCurContext;
        }
        mBackgroundContextList.clear();

        if( mNullColourFramebuffer )
        {
            OCGE( glDeleteFramebuffers( 1, &mNullColourFramebuffer ) );
            mNullColourFramebuffer = 0;
        }

        mGLSupport->stop();
        mStopRendering = true;

        // delete mTextureManager;
        // mTextureManager = 0;

        if( mGlobalVao )
        {
            OCGE( glBindVertexArray( 0 ) );
            OCGE( glDeleteVertexArrays( 1, &mGlobalVao ) );
            mGlobalVao = 0;
        }

        mGLInitialised = 0;

        // RenderSystem::shutdown();
    }

    bool GL3PlusRenderSystem::_createRenderWindows(
            const RenderWindowDescriptionList& renderWindowDescriptions,
            WindowList &createdWindows )
    {
        // Call base render system method.
        if (false == RenderSystem::_createRenderWindows(renderWindowDescriptions, createdWindows))
            return false;

        // Simply call _createRenderWindow in a loop.
        for (size_t i = 0; i < renderWindowDescriptions.size(); ++i)
        {
            const RenderWindowDescription& curRenderWindowDescription = renderWindowDescriptions[i];
            Window* curWindow = NULL;

            curWindow = _createRenderWindow(curRenderWindowDescription.name,
                                            curRenderWindowDescription.width,
                                            curRenderWindowDescription.height,
                                            curRenderWindowDescription.useFullScreen,
                                            &curRenderWindowDescription.miscParams);

            createdWindows.push_back(curWindow);
        }

        return true;
    }

    Window* GL3PlusRenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                      bool fullScreen,
                                                      const NameValuePairList *miscParams )
    {
        // Log a message
        StringStream ss;
        ss << "GL3PlusRenderSystem::_createRenderWindow \"" << name << "\", " <<
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
        Ogre::Window *win = mGLSupport->newWindow( name, width, height, fullScreen, miscParams );
        mWindows.insert( win );

        if( !mGLInitialised )
        {
            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

            initialiseContext(win);

            mDriverVersion = mGLSupport->getGLVersion();

            if( !mDriverVersion.hasMinVersion( 3, 3 ) )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "OpenGL 3.3 or greater required. Try updating your drivers.",
                             "GL3PlusRenderSystem::_createRenderWindow" );
            }

            assert( !mVaoManager );
            bool supportsArbBufferStorage   = mDriverVersion.hasMinVersion( 4, 4 ) ||
                    mGLSupport->checkExtension("GL_ARB_buffer_storage");
            bool emulateTexBuffers          = !(mHasGL43 ||
                    mGLSupport->checkExtension("GL_ARB_texture_buffer_range"));
            bool supportsIndirectBuffers    = mDriverVersion.hasMinVersion( 4, 6 ) ||
                    mGLSupport->checkExtension("GL_ARB_multi_draw_indirect");
            bool supportsBaseInstance       = mDriverVersion.hasMinVersion( 4, 2 ) ||
                    mGLSupport->checkExtension("GL_ARB_base_instance");
            bool supportsSsbo               = mHasGL43 ||
                    mGLSupport->checkExtension("GL_ARB_shader_storage_buffer_object");
            mVaoManager = OGRE_NEW GL3PlusVaoManager( supportsArbBufferStorage, emulateTexBuffers,
                                                      supportsIndirectBuffers, supportsBaseInstance,
                                                      supportsSsbo, miscParams );

            //Bind the Draw ID
            OCGE( glGenVertexArrays( 1, &mGlobalVao ) );
            OCGE( glBindVertexArray( mGlobalVao ) );
            static_cast<GL3PlusVaoManager*>( mVaoManager )->bindDrawId();
            OCGE( glBindVertexArray( 0 ) );

            const char* shadingLangVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
            StringVector tokens = StringUtil::split(shadingLangVersion, ". ");
            mNativeShadingLanguageVersion = (StringConverter::parseUnsignedInt(tokens[0]) * 100) + StringConverter::parseUnsignedInt(tokens[1]);

            // Initialise GL after the first window has been created
            // TODO: fire this from emulation options, and don't duplicate Real and Current capabilities
            mRealCapabilities = createRenderSystemCapabilities();

            mHasArbInvalidateSubdata = mHasGL43 ||
                                        mGLSupport->checkExtension( "GL_ARB_invalidate_subdata" );

            // use real capabilities if custom capabilities are not available
            if (!mUseCustomCapabilities)
                mCurrentCapabilities = mRealCapabilities;

            //On AMD's GCN cards, there is no performance or memory difference between
            //PF_D24_UNORM_S8_UINT & PF_D32_FLOAT_X24_S8_UINT, so prefer the latter
            //on modern cards (GL >= 4.3) and that also claim to support this format.
            //NVIDIA's preference? Dunno, they don't tell. But at least the quality
            //will be consistent.
            if( mDriverVersion.hasMinVersion( 4, 0 ) )
                DepthBuffer::DefaultDepthBufferFormat = PFG_D32_FLOAT_S8X24_UINT;
            else
                DepthBuffer::DefaultDepthBufferFormat = PFG_D24_UNORM_S8_UINT;

            mTextureGpuManager = OGRE_NEW GL3PlusTextureGpuManager( mVaoManager, this, *mGLSupport );

            fireEvent("RenderSystemCapabilitiesCreated");

            initialiseFromRenderSystemCapabilities(mCurrentCapabilities, win);

            // Initialise the main context
            _oneTimeContextInitialization();
            if (mCurrentContext)
                mCurrentContext->setInitialized();

            mTextureGpuManager->_update( true );
        }

        win->_initialize( mTextureGpuManager );

        return win;
    }
    //-----------------------------------------------------------------------------------
    extern const IdString CustomAttributeIdString_GLCONTEXT;
    const IdString CustomAttributeIdString_GLCONTEXT( "GLCONTEXT" );
    void GL3PlusRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture )
    {
        GL3PlusContext *newContext = 0;
        texture->getCustomAttribute( CustomAttributeIdString_GLCONTEXT, &newContext );
        if( newContext && mCurrentContext != newContext )
            _switchContext( newContext );
    }
    //-----------------------------------------------------------------------------------
    RenderPassDescriptor* GL3PlusRenderSystem::createRenderPassDescriptor(void)
    {
        RenderPassDescriptor *retVal = OGRE_NEW GL3PlusRenderPassDescriptor( this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderSystem::beginRenderPassDescriptor( RenderPassDescriptor *desc,
                                                         TextureGpu *anyTarget, uint8 mipLevel,
                                                         const Vector4 *viewportSizes,
                                                         const Vector4 *scissors,
                                                         uint32 numViewports,
                                                         bool overlaysEnabled,
                                                         bool warnIfRtvWasFlushed )
    {
        if( desc->mInformationOnly && desc->hasSameAttachments( mCurrentRenderPassDescriptor ) )
            return;

        flushUAVs();

        GL3PlusRenderPassDescriptor *currPassDesc =
                static_cast<GL3PlusRenderPassDescriptor*>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSizes, scissors,
                                                 numViewports, overlaysEnabled, warnIfRtvWasFlushed );

        GL3PlusRenderPassDescriptor *newPassDesc =
                static_cast<GL3PlusRenderPassDescriptor*>( desc );

        //Determine whether:
        //  1. We need to store current active RenderPassDescriptor
        //  2. We need to perform clears when loading the new RenderPassDescriptor
        uint32 entriesToFlush = 0;
        if( currPassDesc )
        {
            entriesToFlush = currPassDesc->willSwitchTo( newPassDesc, warnIfRtvWasFlushed );

            if( entriesToFlush != 0 )
                currPassDesc->performStoreActions( mHasArbInvalidateSubdata, entriesToFlush );
        }
        else
        {
            entriesToFlush = RenderPassDescriptor::All;
        }

        if( entriesToFlush )
        {
            //If we clear, we need the whole viewport
            const GLsizei w = static_cast<GLsizei>( anyTarget->getWidth() );
            const GLsizei h = static_cast<GLsizei>( anyTarget->getHeight() );
            OCGE( glViewport( 0, 0, w, h ) );
            OCGE( glScissor( 0, 0, w, h ) );
        }

        newPassDesc->performLoadActions( mBlendChannelMask, mDepthWrite,
                                         mStencilParams.writeMask, entriesToFlush );

        {
            GLfloat xywhVp[16][4];
            GLint xywhSc[16][4];
            for( size_t i=0; i<numViewports; ++i )
            {
                xywhVp[i][0] = mCurrentRenderViewport[i].getActualLeft();
                xywhVp[i][1] = mCurrentRenderViewport[i].getActualTop();
                xywhVp[i][2] = mCurrentRenderViewport[i].getActualWidth();
                xywhVp[i][3] = mCurrentRenderViewport[i].getActualHeight();

                xywhSc[i][0] = mCurrentRenderViewport[i].getScissorActualLeft();
                xywhSc[i][1] = mCurrentRenderViewport[i].getScissorActualTop();
                xywhSc[i][2] = mCurrentRenderViewport[i].getScissorActualWidth();
                xywhSc[i][3] = mCurrentRenderViewport[i].getScissorActualHeight();

                if( !desc->requiresTextureFlipping() )
                {
                    // Convert "upper-left" corner to "lower-left"
                    xywhVp[i][1] = anyTarget->getHeight() - xywhVp[i][3] - xywhVp[i][1];
                    xywhSc[i][1] = anyTarget->getHeight() - xywhSc[i][3] - xywhSc[i][1];
                }
            }
            glViewportArrayv( 0u, numViewports, reinterpret_cast<GLfloat*>( xywhVp ) );
            glScissorArrayv( 0u, numViewports, reinterpret_cast<GLint*>( xywhVp ) );
        }
        /*else
        {
            GLsizei x, y, w, h;
            w = mCurrentRenderViewport[0].getActualWidth();
            h = mCurrentRenderViewport[0].getActualHeight();
            x = mCurrentRenderViewport[0].getActualLeft();
            y = mCurrentRenderViewport[0].getActualTop();

            if( !desc->requiresTextureFlipping() )
            {
                // Convert "upper-left" corner to "lower-left"
                y = anyTarget->getHeight() - h - y;
            }
            OCGE( glViewport( x, y, w, h ) );

            w = mCurrentRenderViewport[0].getScissorActualWidth();
            h = mCurrentRenderViewport[0].getScissorActualHeight();
            x = mCurrentRenderViewport[0].getScissorActualLeft();
            y = mCurrentRenderViewport[0].getScissorActualTop();

            if( !desc->requiresTextureFlipping() )
            {
                // Convert "upper-left" corner to "lower-left"
                y = anyTarget->getHeight() - h - y;
            }

            // Configure the viewport clipping
            OCGE( glScissor( x, y, w, h ) );
        }*/
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusRenderSystem::endRenderPassDescriptor(void)
    {
        if( mCurrentRenderPassDescriptor )
        {
            GL3PlusRenderPassDescriptor *passDesc =
                    static_cast<GL3PlusRenderPassDescriptor*>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions( mHasArbInvalidateSubdata, RenderPassDescriptor::All );
        }
        OCGE( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );

        RenderSystem::endRenderPassDescriptor();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* GL3PlusRenderSystem::createDepthBufferFor( TextureGpu *colourTexture,
                                                           bool preferDepthTexture,
                                                           PixelFormatGpu depthBufferFormat )
    {
        if( depthBufferFormat == PFG_UNKNOWN )
        {
            //GeForce 8 & 9 series are faster using 24-bit depth buffers. Likely
            //other HW from that era has the same issue. Assume GL <4.0 is old
            //HW that prefers 24-bit.
            depthBufferFormat = DepthBuffer::DefaultDepthBufferFormat;
        }

        return RenderSystem::createDepthBufferFor( colourTexture, preferDepthTexture,
                                                   depthBufferFormat );
    }

    String GL3PlusRenderSystem::getErrorDescription(long errorNumber) const
    {
        return BLANKSTRING;
    }

    VertexElementType GL3PlusRenderSystem::getColourVertexElementType(void) const
    {
        return VET_COLOUR_ABGR;
    }

    void GL3PlusRenderSystem::_setWorldMatrix(const Matrix4 &m)
    {
        mWorldMatrix = m;
    }

    void GL3PlusRenderSystem::_setViewMatrix(const Matrix4 &m)
    {
        mViewMatrix = m;

        // Also mark clip planes dirty.
        if (!mClipPlanes.empty())
        {
            mClipPlanesDirty = true;
        }
    }

    void GL3PlusRenderSystem::_setProjectionMatrix(const Matrix4 &m)
    {
        // Nothing to do but mark clip planes dirty.
        if (!mClipPlanes.empty())
            mClipPlanesDirty = true;
    }

    void GL3PlusRenderSystem::_setPointParameters(Real size,
                                                  bool attenuationEnabled, Real constant, Real linear, Real quadratic,
                                                  Real minSize, Real maxSize)
    {

        if (attenuationEnabled)
        {
            // Point size is still calculated in pixels even when attenuation is
            // enabled, which is pretty awkward, since you typically want a viewport
            // independent size if you're looking for attenuation.
            // So, scale the point size up by viewport size (this is equivalent to
            // what D3D does as standard).
            size = size * mCurrentRenderViewport[0].getActualHeight();

            // XXX: why do I need this for results to be consistent with D3D?
            // Equations are supposedly the same once you factor in vp height.
            // Real correction = 0.005;
            // Scaling required.
            // float val[4] = {1, 0, 0, 1};
            // val[1] = linear * correction;
            // val[2] = quadratic * correction;
            // val[3] = 1;

            if (mCurrentCapabilities->hasCapability(RSC_VERTEX_PROGRAM))
            {
                OGRE_CHECK_GL_ERROR(glEnable(GL_PROGRAM_POINT_SIZE));
            }
        }
        else
        {
            if (mCurrentCapabilities->hasCapability(RSC_VERTEX_PROGRAM))
            {
                OGRE_CHECK_GL_ERROR(glDisable(GL_PROGRAM_POINT_SIZE));
            }
        }

        //FIXME Points do not seem affected by setting this.
        // OGRE_CHECK_GL_ERROR(glPointSize(size));
        OGRE_CHECK_GL_ERROR(glPointSize(30.0));

        //OGRE_CHECK_GL_ERROR(glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 64.0));
    }

    void GL3PlusRenderSystem::_setPointSpritesEnabled(bool enabled)
    {
        // Point sprites are always on in OpenGL 3.2 and up.
    }

    void GL3PlusRenderSystem::_setTexture( size_t stage, TextureGpu *texPtr )
    {
        if( !activateGLTextureUnit( stage ) )
            return;

        if( texPtr )
        {
            GL3PlusTextureGpu *tex = static_cast<GL3PlusTextureGpu*>( texPtr );
            GLenum target = tex->getGlTextureTarget();
            GLuint textureName = tex->getDisplayTextureName();
            OCGE( glBindTexture( target, textureName ) );
        }
        else
        {
            // Bind zero texture. GL_TEXTURE_2D & GL_TEXTURE_2D_ARRAY are
            //the most common but not necessarily correct.
            OCGE( glBindTexture( GL_TEXTURE_2D, 0 ) );
            OCGE( glBindTexture( GL_TEXTURE_2D_ARRAY, 0 ) );
        }

        activateGLTextureUnit( 0 );
    }

    void GL3PlusRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                            uint32 hazardousTexIdx )
    {
        //Ignore hazardousTexIdx, GL doesn't care. It will happily let us do hazards.
        uint32 texUnit = slotStart;

        FastArray<const TextureGpu*>::const_iterator itor = set->mTextures.begin();
        FastArray<const TextureGpu*>::const_iterator end  = set->mTextures.end();

        while( itor != end )
        {
            OCGE( glActiveTexture( static_cast<uint32>( GL_TEXTURE0 + texUnit ) ) );

            if( *itor )
            {
                const GL3PlusTextureGpu *textureGpu = static_cast<const GL3PlusTextureGpu*>( *itor );
                const GLenum texTarget  = textureGpu->getGlTextureTarget();
                const GLuint texName    = textureGpu->getDisplayTextureName();
                OCGE( glBindTexture( texTarget, texName ) );
                mTextureTypes[texUnit] = texTarget;
            }
            else
            {
                OCGE( glBindTexture( mTextureTypes[texUnit], 0 ) );
            }

            ++texUnit;
            ++itor;
        }

        OCGE( glActiveTexture( GL_TEXTURE0 ) );
    }

    void GL3PlusRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        uint32 texUnit = slotStart;

        GL3PlusDescriptorSetTexture2 *srvList =
                reinterpret_cast<GL3PlusDescriptorSetTexture2*>( set->mRsData );
        FastArray<DescriptorSetTexture2::Slot>::const_iterator itor = set->mTextures.begin();
        FastArray<DescriptorSetTexture2::Slot>::const_iterator end  = set->mTextures.end();

        while( itor != end )
        {
            OCGE( glActiveTexture( static_cast<uint32>( GL_TEXTURE0 + texUnit ) ) );

            if( itor->isBuffer() )
            {
                //Bind buffer
                const DescriptorSetTexture2::BufferSlot &bufferSlot = itor->getBuffer();
                if( bufferSlot.buffer )
                    bufferSlot.buffer->_bindBufferDirectly( bufferSlot.offset, bufferSlot.sizeBytes );
            }
            else
            {
                //Bind texture
                const DescriptorSetTexture2::TextureSlot &texSlot = itor->getTexture();
                if( texSlot.texture )
                {
                    const size_t idx = texUnit - slotStart;
                    mTextureTypes[texUnit] = srvList[idx].target;
                    OCGE( glBindTexture( srvList[idx].target, srvList[idx].texName ) );
                }
                else
                {
                    OCGE( glBindTexture( mTextureTypes[texUnit], 0 ) );
                }
            }

            ++texUnit;
            ++itor;
        }

        OCGE( glActiveTexture( GL_TEXTURE0 ) );
    }

    void GL3PlusRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set )
    {
        uint32 texUnit = slotStart;

        FastArray<const HlmsSamplerblock*>::const_iterator itor = set->mSamplers.begin();
        FastArray<const HlmsSamplerblock*>::const_iterator end  = set->mSamplers.end();

        while( itor != end )
        {
            const HlmsSamplerblock *samplerblock = *itor;

            assert( (!samplerblock || samplerblock->mRsData) &&
                    "The block must have been created via HlmsManager::getSamplerblock!" );

            if( !samplerblock )
            {
                glBindSampler( texUnit, 0 );
            }
            else
            {
                glBindSampler( texUnit, static_cast<GLuint>(
                                   reinterpret_cast<intptr_t>( samplerblock->mRsData ) ) );
            }

            ++texUnit;
            ++itor;
        }
    }

    void GL3PlusRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set )
    {
        _setTextures( slotStart, set, std::numeric_limits<uint32>::max() );
    }

    void GL3PlusRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        _setTextures( slotStart, set );
    }

    void GL3PlusRenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set )
    {
        _setSamplers( slotStart, set );
    }

    void GL3PlusRenderSystem::setBufferUavCS( uint32 slot,
                                              const DescriptorSetUav::BufferSlot &bufferSlot )
    {
        if( bufferSlot.buffer )
        {
            bufferSlot.buffer->bindBufferCS( slot, bufferSlot.offset, bufferSlot.sizeBytes );
        }
        else
        {
            OCGE( glBindImageTexture( slot, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI ) );
            OCGE( glBindBufferRange( GL_SHADER_STORAGE_BUFFER, slot, 0, 0, 0 ) );
        }
    }

    void GL3PlusRenderSystem::setTextureUavCS( uint32 slot, const DescriptorSetUav::TextureSlot &texSlot,
                                               GLuint srvView )
    {
        if( texSlot.texture )
        {
            GLenum access;
            switch( texSlot.access )
            {
            case ResourceAccess::Read:
                access = GL_READ_ONLY;
                break;
            case ResourceAccess::Write:
                access = GL_WRITE_ONLY;
                break;
            case ResourceAccess::ReadWrite:
                access = GL_READ_WRITE;
                break;
            default:
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid ResourceAccess parameter '" +
                             StringConverter::toString( texSlot.access ) + "'",
                             "GL3PlusRenderSystem::setTextureUavCS" );
                break;
            }

            PixelFormatGpu pixelFormat = texSlot.pixelFormat;
            if( pixelFormat == PFG_UNKNOWN )
                pixelFormat = texSlot.texture->getPixelFormat();

            GLboolean isArrayTexture;
            const TextureTypes::TextureTypes textureType = texSlot.texture->getTextureType();
            if( textureType == TextureTypes::Type1DArray ||
                textureType == TextureTypes::Type2DArray ||
                textureType == TextureTypes::Type3D ||
                textureType == TextureTypes::TypeCube ||
                textureType == TextureTypes::TypeCubeArray )
            {
                isArrayTexture = GL_TRUE;
            }
            else
            {
                isArrayTexture = GL_FALSE;
            }
            const GLenum format = GL3PlusMappings::get( pixelFormat );

            OCGE( glBindImageTexture( slot, srvView, texSlot.mipmapLevel, isArrayTexture,
                                      texSlot.textureArrayIndex, access, format ) );
        }
        else
        {
            OCGE( glBindImageTexture( slot, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI ) );
            OCGE( glBindBufferRange( GL_SHADER_STORAGE_BUFFER, slot, 0, 0, 0 ) );
        }
    }

    void GL3PlusRenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set )
    {
        if( !set )
            return;

        GLuint *srvList = reinterpret_cast<GLuint*>( set->mRsData );
        size_t elementIdx = 0;

        FastArray<DescriptorSetUav::Slot>::const_iterator itor = set->mUavs.begin();
        FastArray<DescriptorSetUav::Slot>::const_iterator end  = set->mUavs.end();

        while( itor != end )
        {
            if( itor->isBuffer() )
                setBufferUavCS( slotStart, itor->getBuffer() );
            else
                setTextureUavCS( slotStart, itor->getTexture(), srvList[elementIdx] );

            ++elementIdx;
            ++slotStart;
            ++itor;
        }

        mFirstUavBoundSlot   = std::min<uint8>( mFirstUavBoundSlot, slotStart );
        mLastUavBoundPlusOne = std::max<uint8>( mLastUavBoundPlusOne, slotStart + set->mUavs.size() );
    }

    void GL3PlusRenderSystem::_setVertexTexture( size_t unit, TextureGpu *tex )
    {
        _setTexture(unit, tex);
    }

    void GL3PlusRenderSystem::_setGeometryTexture( size_t unit, TextureGpu *tex )
    {
        _setTexture(unit, tex);
    }

    void GL3PlusRenderSystem::_setTessellationHullTexture( size_t unit, TextureGpu *tex )
    {
        _setTexture(unit, tex);
    }

    void GL3PlusRenderSystem::_setTessellationDomainTexture( size_t unit, TextureGpu *tex )
    {
        _setTexture(unit, tex);
    }

    void GL3PlusRenderSystem::flushUAVs(void)
    {
        if( mUavRenderingDirty )
        {
            //Unbind in range [mFirstUavBoundSlot; mUavStartingSlot)
            if( mFirstUavBoundSlot < mUavStartingSlot )
            {
                const size_t startingSlot = mUavStartingSlot;
                for( size_t i=mFirstUavBoundSlot; i<startingSlot; ++i )
                {
                    OCGE( glBindImageTexture( i, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI ) );
                    OCGE( glBindBufferRange( GL_SHADER_STORAGE_BUFFER, i, 0, 0, 0 ) );
                }

                mFirstUavBoundSlot = 255;
            }

            //Unbind in range [lastUavToBindPlusOne, mLastUavBoundPlusOne)
            if( !mUavRenderingDescSet ||
                mLastUavBoundPlusOne > (mUavStartingSlot + mUavRenderingDescSet->mUavs.size()) )
            {
                size_t lastUavToBindPlusOne = mUavStartingSlot;
                if( mUavRenderingDescSet )
                    lastUavToBindPlusOne += mUavRenderingDescSet->mUavs.size();

                const size_t lastUavBoundPlusOne = mLastUavBoundPlusOne;

                for( size_t i=lastUavToBindPlusOne; i<lastUavBoundPlusOne; ++i )
                {
                    OCGE( glBindImageTexture( i, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI ) );
                    OCGE( glBindBufferRange( GL_SHADER_STORAGE_BUFFER, i, 0, 0, 0 ) );
                }

                mLastUavBoundPlusOne = 0;
            }

            _setUavCS( mUavStartingSlot, mUavRenderingDescSet );
            mUavRenderingDirty = false;
        }
    }

    GLint GL3PlusRenderSystem::getTextureAddressingMode(TextureAddressingMode tam) const
    {
        switch (tam)
        {
        default:
        case TAM_WRAP:
            return GL_REPEAT;
        case TAM_MIRROR:
            return GL_MIRRORED_REPEAT;
        case TAM_CLAMP:
            return GL_CLAMP_TO_EDGE;
        case TAM_BORDER:
            return GL_CLAMP_TO_BORDER;
        }
    }

    GLenum GL3PlusRenderSystem::getBlendMode(SceneBlendFactor ogreBlend)
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

    GLenum GL3PlusRenderSystem::getBlendOperation( SceneBlendOperation op )
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
            return GL_MIN;
        case SBO_MAX:
            return GL_MAX;
        }

        // To keep compiler happy.
        return GL_FUNC_ADD;
    }

    void GL3PlusRenderSystem::_setSceneBlending(SceneBlendFactor sourceFactor, SceneBlendFactor destFactor, SceneBlendOperation op)
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
            func = GL_MIN;
            break;
        case SBO_MAX:
            func = GL_MAX;
            break;
        }

        OGRE_CHECK_GL_ERROR(glBlendEquation(func));
    }

    void GL3PlusRenderSystem::_setSeparateSceneBlending(
        SceneBlendFactor sourceFactor, SceneBlendFactor destFactor,
        SceneBlendFactor sourceFactorAlpha, SceneBlendFactor destFactorAlpha,
        SceneBlendOperation op, SceneBlendOperation alphaOp )
    {
        GLenum sourceBlend = getBlendMode(sourceFactor);
        GLenum destBlend = getBlendMode(destFactor);
        GLenum sourceBlendAlpha = getBlendMode(sourceFactorAlpha);
        GLenum destBlendAlpha = getBlendMode(destFactorAlpha);

        if (sourceFactor == SBF_ONE && destFactor == SBF_ZERO &&
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
            func = GL_MIN;
            break;
        case SBO_MAX:
            func = GL_MAX;
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
            alphaFunc = GL_MIN;
            break;
        case SBO_MAX:
            alphaFunc = GL_MAX;
            break;
        }

        OGRE_CHECK_GL_ERROR(glBlendEquationSeparate(func, alphaFunc));
    }

    void GL3PlusRenderSystem::_resourceTransitionCreated( ResourceTransition *resTransition )
    {
        assert( sizeof(void*) >= sizeof(GLbitfield) );

        assert( (resTransition->readBarrierBits || resTransition->writeBarrierBits) &&
                "A zero-bit memory barrier is invalid!" );

        GLbitfield barriers = 0;

        //TODO:
        //GL_QUERY_BUFFER_BARRIER_BIT is nearly impossible to determine
        //specifically
        //Should be used in all barriers? Since we don't yet support them,
        //we don't include it in case it brings performance down.
        //Or should we use 'All' instead for these edge cases?

        if( resTransition->readBarrierBits & ReadBarrier::CpuRead ||
            resTransition->writeBarrierBits & WriteBarrier::CpuWrite )
        {
            barriers |= GL_PIXEL_BUFFER_BARRIER_BIT|GL_TEXTURE_UPDATE_BARRIER_BIT|
                        GL_BUFFER_UPDATE_BARRIER_BIT|GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT;
        }

        if( resTransition->readBarrierBits & ReadBarrier::Indirect )
            barriers |= GL_COMMAND_BARRIER_BIT;

        if( resTransition->readBarrierBits & ReadBarrier::VertexBuffer )
            barriers |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_TRANSFORM_FEEDBACK_BARRIER_BIT;

        if( resTransition->readBarrierBits & ReadBarrier::IndexBuffer )
            barriers |= GL_ELEMENT_ARRAY_BARRIER_BIT;

        if( resTransition->readBarrierBits & ReadBarrier::ConstBuffer )
            barriers |= GL_UNIFORM_BARRIER_BIT;

        if( resTransition->readBarrierBits & ReadBarrier::Texture )
            barriers |= GL_TEXTURE_FETCH_BARRIER_BIT;

        if( resTransition->readBarrierBits & ReadBarrier::Uav ||
            resTransition->writeBarrierBits & WriteBarrier::Uav )
        {
            barriers |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_SHADER_STORAGE_BARRIER_BIT|
                        GL_ATOMIC_COUNTER_BARRIER_BIT;
        }

        if( resTransition->readBarrierBits & (ReadBarrier::RenderTarget|ReadBarrier::DepthStencil) ||
            resTransition->writeBarrierBits & (WriteBarrier::RenderTarget|WriteBarrier::DepthStencil) )
        {
            barriers |= GL_FRAMEBUFFER_BARRIER_BIT;
        }

        if( resTransition->readBarrierBits == ReadBarrier::All ||
            resTransition->writeBarrierBits == WriteBarrier::All )
        {
            barriers = GL_ALL_BARRIER_BITS;
        }

        resTransition->mRsData = reinterpret_cast<void*>( barriers );
    }

    void GL3PlusRenderSystem::_resourceTransitionDestroyed( ResourceTransition *resTransition )
    {
        assert( resTransition->mRsData ); //A zero-bit memory barrier is invalid
        resTransition->mRsData = 0;
    }

    void GL3PlusRenderSystem::_executeResourceTransition( ResourceTransition *resTransition )
    {
        if( !glMemoryBarrier )
            return;

        GLbitfield barriers = static_cast<GLbitfield>( reinterpret_cast<intptr_t>(
                                                           resTransition->mRsData ) );

        assert( barriers && "A zero-bit memory barrier is invalid" );
        glMemoryBarrier( barriers );
    }

    void GL3PlusRenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newBlock )
    {
        GL3PlusHlmsPso *pso = new GL3PlusHlmsPso();
        memset( pso, 0, sizeof(GL3PlusHlmsPso) );

        //
        // Macroblock stuff
        //
        pso->depthWrite = newBlock->macroblock->mDepthWrite ? GL_TRUE : GL_FALSE;
        CompareFunction depthFunc = newBlock->macroblock->mDepthFunc;
        if( mReverseDepth )
            depthFunc = reverseCompareFunction( depthFunc );
        pso->depthFunc  = convertCompareFunction( depthFunc );

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

        if( !newBlock->vertexShader.isNull() )
        {
            pso->vertexShader = static_cast<GLSLShader*>( newBlock->vertexShader->
                                                          _getBindingDelegate() );
        }
        if( !newBlock->geometryShader.isNull() )
        {
            pso->geometryShader = static_cast<GLSLShader*>( newBlock->geometryShader->
                                                            _getBindingDelegate() );
        }
        if( !newBlock->tesselationHullShader.isNull() )
        {
            pso->hullShader = static_cast<GLSLShader*>( newBlock->tesselationHullShader->
                                                        _getBindingDelegate() );
        }
        if( !newBlock->tesselationDomainShader.isNull() )
        {
            pso->domainShader = static_cast<GLSLShader*>( newBlock->tesselationDomainShader->
                                                          _getBindingDelegate() );
        }
        if( !newBlock->pixelShader.isNull() )
        {
            pso->pixelShader = static_cast<GLSLShader*>( newBlock->pixelShader->
                                                         _getBindingDelegate() );
        }

        newBlock->rsData = pso;
    }

    void GL3PlusRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        GL3PlusHlmsPso *glPso = reinterpret_cast<GL3PlusHlmsPso*>(pso->rsData);
        delete glPso;
        pso->rsData = 0;
    }

    void GL3PlusRenderSystem::_hlmsMacroblockCreated( HlmsMacroblock *newBlock )
    {
        //Set it to non-zero to get the assert in _setHlmsBlendblock
        //to work correctly (which is a very useful assert)
        newBlock->mRsData = reinterpret_cast<void*>( 1 );
    }

    void GL3PlusRenderSystem::_hlmsMacroblockDestroyed( HlmsMacroblock *block )
    {
        block->mRsData = 0;
    }

    void GL3PlusRenderSystem::_hlmsBlendblockCreated( HlmsBlendblock *newBlock )
    {
        //Set it to non-zero to get the assert in _setHlmsBlendblock
        //to work correctly (which is a very useful assert)
        newBlock->mRsData = reinterpret_cast<void*>( 1 );
    }

    void GL3PlusRenderSystem::_hlmsBlendblockDestroyed( HlmsBlendblock *block )
    {
        block->mRsData = 0;
    }

    void GL3PlusRenderSystem::_hlmsSamplerblockCreated( HlmsSamplerblock *newBlock )
    {
        GLuint samplerName = 0;
        OCGE( glGenSamplers( 1, &samplerName ) );

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

        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_MIN_FILTER, minFilter ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_MAG_FILTER, magFilter ) );

        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_S,
                                   getTextureAddressingMode( newBlock->mU ) ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_T,
                                   getTextureAddressingMode( newBlock->mV ) ) );
        OCGE( glSamplerParameteri( samplerName, GL_TEXTURE_WRAP_R,
                                   getTextureAddressingMode( newBlock->mW ) ) );

        float borderColour[4] = { (float)newBlock->mBorderColour.r, (float)newBlock->mBorderColour.g,
                                  (float)newBlock->mBorderColour.b, (float)newBlock->mBorderColour.a };
        OCGE( glSamplerParameterfv( samplerName, GL_TEXTURE_BORDER_COLOR,
                                    borderColour ) );
        OCGE( glSamplerParameterf( samplerName, GL_TEXTURE_LOD_BIAS, newBlock->mMipLodBias ) );
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

        newBlock->mRsData = reinterpret_cast<void*>( samplerName );

        /*GL3PlusHlmsSamplerblock *glSamplerblock = new GL3PlusHlmsSamplerblock();

        switch( newBlock->mMinFilter )
        {
        case FO_ANISOTROPIC:
        case FO_LINEAR:
            switch( newBlock->mMipFilter )
            {
            case FO_ANISOTROPIC:
            case FO_LINEAR:
                // linear min, linear mip
                glSamplerblock->mMinFilter = GL_LINEAR_MIPMAP_LINEAR;
                break;
            case FO_POINT:
                // linear min, point mip
                glSamplerblock->mMinFilter = GL_LINEAR_MIPMAP_NEAREST;
                break;
            case FO_NONE:
                // linear min, no mip
                glSamplerblock->mMinFilter = GL_LINEAR;
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
                glSamplerblock->mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
                break;
            case FO_POINT:
                // nearest min, point mip
                glSamplerblock->mMinFilter = GL_NEAREST_MIPMAP_NEAREST;
                break;
            case FO_NONE:
                // nearest min, no mip
                glSamplerblock->mMinFilter = GL_NEAREST;
                break;
            }
            break;
        }

        glSamplerblock->mMagFilter = newBlock->mMagFilter <= FO_POINT ? GL_NEAREST : GL_LINEAR;
        glSamplerblock->mU  = getTextureAddressingMode( newBlock->mU );
        glSamplerblock->mV  = getTextureAddressingMode( newBlock->mV );
        glSamplerblock->mW  = getTextureAddressingMode( newBlock->mW );

        glSamplerblock->mAnisotropy = std::min( newBlock->mMaxAnisotropy, mLargestSupportedAnisotrop );

        newBlock->mRsData = glSamplerblock;*/
    }

    void GL3PlusRenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        GLuint samplerName = static_cast<GLuint>( reinterpret_cast<intptr_t>( block->mRsData ) );
        glDeleteSamplers( 1, &samplerName );
    }

    void GL3PlusRenderSystem::_descriptorSetTexture2Created( DescriptorSetTexture2 *newSet )
    {
        const size_t numElements = newSet->mTextures.size();
        GL3PlusDescriptorSetTexture2 *srvList = new GL3PlusDescriptorSetTexture2[numElements];
        newSet->mRsData = srvList;

        FastArray<DescriptorSetTexture2::Slot>::const_iterator itor = newSet->mTextures.begin();

        for( size_t i=0u; i<numElements; ++i )
        {
            srvList[i].target = 0;
            srvList[i].texName = 0;
            if( itor->isTexture() && itor->getTexture().texture )
            {
                const DescriptorSetTexture2::TextureSlot &texSlot = itor->getTexture();
                const GL3PlusTextureGpu *textureGpu =
                        static_cast<const GL3PlusTextureGpu*>( itor->getTexture().texture );
                GLuint displayTex = textureGpu->getDisplayTextureName();

                if( texSlot.needsDifferentView() )
                {
                    glGenTextures( 1u, &srvList[i].texName );

                    PixelFormatGpu pixelFormat = texSlot.pixelFormat;
                    if( pixelFormat == PFG_UNKNOWN )
                        pixelFormat = textureGpu->getPixelFormat();

                    const GLenum format = GL3PlusMappings::get( pixelFormat );
                    srvList[i].target = GL3PlusMappings::get( textureGpu->getTextureType(),
                                                              texSlot.cubemapsAs2DArrays );

                    uint8 numMipmaps = texSlot.numMipmaps;
                    if( !texSlot.numMipmaps )
                        numMipmaps = textureGpu->getNumMipmaps() - texSlot.mipmapLevel;

                    OGRE_ASSERT_LOW( numMipmaps <= textureGpu->getNumMipmaps() - texSlot.mipmapLevel &&
                                     "Asking for more mipmaps than the texture has!" );

                    glTextureView( srvList[i].texName, srvList[i].target, displayTex, format,
                                   texSlot.mipmapLevel, numMipmaps,
                                   texSlot.textureArrayIndex,
                                   textureGpu->getNumSlices() - texSlot.textureArrayIndex );
                }
                else
                {
                    srvList[i].texName = displayTex;
                    srvList[i].target = textureGpu->getGlTextureTarget();
                }
            }

            ++itor;
        }
    }

    void GL3PlusRenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        assert( set->mRsData );
        GL3PlusDescriptorSetTexture2 *srvList =
                reinterpret_cast<GL3PlusDescriptorSetTexture2*>( set->mRsData );

        const size_t numElements = set->mTextures.size();
        FastArray<DescriptorSetTexture2::Slot>::const_iterator itor = set->mTextures.begin();

        for( size_t i=0u; i<numElements; ++i )
        {
            if( itor->isTexture() && itor->getTexture().texture )
            {
                const DescriptorSetTexture2::TextureSlot &texSlot = itor->getTexture();
                if( texSlot.needsDifferentView() )
                    glDeleteTextures( 1u, &srvList[i].texName );
            }
        }

        delete [] srvList;
        set->mRsData = 0;
    }

    void GL3PlusRenderSystem::_descriptorSetUavCreated( DescriptorSetUav *newSet )
    {
        const size_t numElements = newSet->mUavs.size();
        GLuint *srvList = new GLuint[numElements];
        newSet->mRsData = srvList;

        FastArray<DescriptorSetUav::Slot>::const_iterator itor = newSet->mUavs.begin();

        for( size_t i=0u; i<numElements; ++i )
        {
            srvList[i] = 0;
            if( itor->isTexture() && itor->getTexture().texture )
            {
                const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();

                const GL3PlusTextureGpu *textureGpu =
                        static_cast<const GL3PlusTextureGpu*>( itor->getTexture().texture );
                srvList[i] = textureGpu->getDisplayTextureName();

                if( texSlot.needsDifferentView() &&
                    texSlot.pixelFormat != PFG_UNKNOWN &&
                    PixelFormatGpuUtils::isSRgb( texSlot.texture->getPixelFormat() ) )
                {
                    OCGE( glGenTextures( 1u, &srvList[i] ) );

                    PixelFormatGpu pixelFormat = texSlot.pixelFormat;

                    const GLenum format = GL3PlusMappings::get( pixelFormat );

                    OCGE( glTextureView( srvList[i], textureGpu->getGlTextureTarget(),
                                         textureGpu->getDisplayTextureName(), format,
                                         0, textureGpu->getNumMipmaps(),
                                         0, textureGpu->getNumSlices() ) );
                }
            }

            ++itor;
        }
    }

    void GL3PlusRenderSystem::_descriptorSetUavDestroyed( DescriptorSetUav *set )
    {
        OGRE_ASSERT_LOW( set->mRsData );
        GLuint *srvList = reinterpret_cast<GLuint*>( set->mRsData );

        const size_t numElements = set->mUavs.size();
        FastArray<DescriptorSetUav::Slot>::const_iterator itor = set->mUavs.begin();

        for( size_t i=0u; i<numElements; ++i )
        {
            if( itor->isTexture() && itor->getTexture().texture )
            {
                const DescriptorSetUav::TextureSlot &texSlot = itor->getTexture();
                if( texSlot.needsDifferentView() &&
                    texSlot.pixelFormat != PFG_UNKNOWN &&
                    PixelFormatGpuUtils::isSRgb( texSlot.texture->getPixelFormat() ) )
                {
                    glDeleteTextures( 1u, &srvList[i] );
                }
            }
        }

        delete [] srvList;
        set->mRsData = 0;
    }

    void GL3PlusRenderSystem::_setHlmsMacroblock( const HlmsMacroblock *macroblock,
                                                  const GL3PlusHlmsPso *pso )
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
        OCGE( glPolygonMode( GL_FRONT_AND_BACK, pso->polygonMode ) );

        if( macroblock->mScissorTestEnabled )
        {
            OCGE( glEnable(GL_SCISSOR_TEST) );
        }
        else
        {
            OCGE( glDisable(GL_SCISSOR_TEST) );
        }

        mDepthWrite         = macroblock->mDepthWrite;
        mScissorsEnabled    = macroblock->mScissorTestEnabled;
    }

    void GL3PlusRenderSystem::_setHlmsBlendblock( const HlmsBlendblock *blendblock,
                                                  const GL3PlusHlmsPso *pso )
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

    void GL3PlusRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        assert( (!samplerblock || samplerblock->mRsData) &&
                "The block must have been created via HlmsManager::getSamplerblock!" );

        if( !samplerblock )
        {
            glBindSampler( texUnit, 0 );
        }
        else
        {
            glBindSampler( texUnit, static_cast<GLuint>(
                                    reinterpret_cast<intptr_t>( samplerblock->mRsData ) ) );
        }
        /*if (!activateGLTextureUnit(texUnit))
            return;

        GL3PlusHlmsSamplerblock *glSamplerblock = reinterpret_cast<GL3PlusHlmsSamplerblock*>(
                                                                                samplerblock->mRsData );

        OGRE_CHECK_GL_ERROR(glTexParameteri( mTextureTypes[unit], GL_TEXTURE_MIN_FILTER,
                                             glSamplerblock->mMinFilter) );
        OGRE_CHECK_GL_ERROR(glTexParameteri( mTextureTypes[unit], GL_TEXTURE_MAG_FILTER,
                                             glSamplerblock->mMagFilter));

        OGRE_CHECK_GL_ERROR(glTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_S, glSamplerblock->mU ));
        OGRE_CHECK_GL_ERROR(glTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_T, glSamplerblock->mV ));
        OGRE_CHECK_GL_ERROR(glTexParameteri( mTextureTypes[stage], GL_TEXTURE_WRAP_R, glSamplerblock->mW ));

        OGRE_CHECK_GL_ERROR(glTexParameterf( mTextureTypes[stage], GL_TEXTURE_LOD_BIAS,
                                             samplerblock->mMipLodBias ));

        OGRE_CHECK_GL_ERROR(glTexParameterfv( mTextureTypes[stage], GL_TEXTURE_BORDER_COLOR,
                                              reinterpret_cast<GLfloat*>(
                                                    &samplerblock->mBorderColour ) ));

        OGRE_CHECK_GL_ERROR(glTexParameterf( mTextureTypes[stage], GL_TEXTURE_MIN_LOD,
                                             samplerblock->mMinLod ));
        OGRE_CHECK_GL_ERROR(glTexParameterf( mTextureTypes[stage], GL_TEXTURE_MAX_LOD,
                                             samplerblock->mMaxLod ));

        activateGLTextureUnit(0);*/
    }

    void GL3PlusRenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        //Disable previous state
        GLSLShader::unbindAll();

        RenderSystem::_setPipelineStateObject( pso );
        _setComputePso( 0 );

        uint8 newClipDistances = 0;
        if( pso )
            newClipDistances = pso->clipDistances;

        if( mClipDistances != newClipDistances )
        {
            for( size_t i=0; i<8u; ++i )
            {
                const uint8 bitFlag = 1u << i;
                bool oldClipSet = (mClipDistances & bitFlag) != 0;
                bool newClipSet = (newClipDistances & bitFlag) != 0;
                if( oldClipSet != newClipSet )
                {
                    if( newClipSet )
                        glEnable( GL_CLIP_DISTANCE0 + i );
                    else
                        glDisable( GL_CLIP_DISTANCE0 + i );
                }
            }

            mClipDistances = newClipDistances;
        }

        mUseAdjacency   = false;
        mPso            = 0;

        if( !pso )
            return;

        //Set new state
        mPso = reinterpret_cast<GL3PlusHlmsPso*>( pso->rsData );

        _setHlmsMacroblock( pso->macroblock, mPso );
        _setHlmsBlendblock( pso->blendblock, mPso );

        if( mPso->vertexShader )
        {
            mPso->vertexShader->bind();
            mActiveVertexGpuProgramParameters = mPso->vertexShader->getDefaultParameters();
            mVertexProgramBound = true;
        }
        if( mPso->geometryShader )
        {
            mPso->geometryShader->bind();
            mActiveGeometryGpuProgramParameters = mPso->geometryShader->getDefaultParameters();
            mGeometryProgramBound = true;

            mUseAdjacency = mPso->geometryShader->isAdjacencyInfoRequired();
        }
        if( mPso->hullShader )
        {
            mPso->hullShader->bind();
            mActiveTessellationHullGpuProgramParameters = mPso->hullShader->getDefaultParameters();
            mTessellationHullProgramBound = true;
        }
        if( mPso->domainShader )
        {
            mPso->domainShader->bind();
            mActiveTessellationDomainGpuProgramParameters = mPso->domainShader->getDefaultParameters();
            mTessellationDomainProgramBound = true;
        }
        if( mPso->pixelShader )
        {
            mPso->pixelShader->bind();
            mActiveFragmentGpuProgramParameters = mPso->pixelShader->getDefaultParameters();
            mFragmentProgramBound = true;
        }

        GLSLSeparableProgramManager* separableProgramMgr =
                GLSLSeparableProgramManager::getSingletonPtr();

        if( separableProgramMgr )
        {
            GLSLSeparableProgram* separableProgram = separableProgramMgr->getCurrentSeparableProgram();
            if (separableProgram)
                separableProgram->activate();
        }
        else
        {
            GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
        }
    }

    void GL3PlusRenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        GLSLShader *newComputeShader = 0;

        if( pso )
        {
            newComputeShader = reinterpret_cast<GLSLShader*>( pso->rsData );

            if( mCurrentComputeShader == newComputeShader )
                return;
        }

        //Disable previous state
        GLSLShader::unbindAll();

        RenderSystem::_setPipelineStateObject( (HlmsPso*)0 );

        mUseAdjacency   = false;
        mPso            = 0;
        mCurrentComputeShader = 0;

        if( !pso )
            return;

        mCurrentComputeShader = newComputeShader;
        mCurrentComputeShader->bind();
        mActiveComputeGpuProgramParameters = pso->computeParams;
        mComputeProgramBound = true;

        GLSLSeparableProgramManager* separableProgramMgr =
                GLSLSeparableProgramManager::getSingletonPtr();

        if( separableProgramMgr )
        {
            GLSLSeparableProgram* separableProgram = separableProgramMgr->getCurrentSeparableProgram();
            if (separableProgram)
                separableProgram->activate();
        }
        else
        {
            GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
        }
    }

    void GL3PlusRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
        if( mVaoManager->supportsIndirectBuffers() )
        {
            if( indirectBuffer )
            {
                GL3PlusBufferInterface *bufferInterface = static_cast<GL3PlusBufferInterface*>(
                                                            indirectBuffer->getBufferInterface() );
                OCGE( glBindBuffer( GL_DRAW_INDIRECT_BUFFER, bufferInterface->getVboName() ) );
            }
            else
            {
                OCGE( glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 ) );
            }
        }
        else
        {
            if( indirectBuffer )
                mSwIndirectBufferPtr = indirectBuffer->getSwBufferPtr();
            else
                mSwIndirectBufferPtr = 0;
        }
    }

    void GL3PlusRenderSystem::_hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso )
    {
        newPso->rsData = reinterpret_cast<void*>( static_cast<GLSLShader*>(
                                                      newPso->computeShader->_getBindingDelegate() ) );
    }

    void GL3PlusRenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *newPso )
    {
        newPso->rsData = 0;
    }

    void GL3PlusRenderSystem::_beginFrame(void)
    {
    }

    void GL3PlusRenderSystem::_endFrame(void)
    {
        OGRE_CHECK_GL_ERROR(glDisable(GL_DEPTH_CLAMP));

        // unbind PSO programs at end of frame
        // this is mostly to avoid holding bound programs that might get deleted
        // outside via the resource manager
        _setPipelineStateObject( 0 );
        _setComputePso( 0 );

        glBindProgramPipeline( 0 );
    }

    void GL3PlusRenderSystem::_setDepthBias(float constantBias, float slopeScaleBias)
    {
        //FIXME glPolygonOffset currently is buggy in GL3+ RS but not GL RS.
        if (constantBias != 0 || slopeScaleBias != 0)
        {
            const float biasSign = mReverseDepth ? 1.0f : -1.0f;
            OGRE_CHECK_GL_ERROR(glEnable(GL_POLYGON_OFFSET_FILL));
            OGRE_CHECK_GL_ERROR(glEnable(GL_POLYGON_OFFSET_POINT));
            OGRE_CHECK_GL_ERROR(glEnable(GL_POLYGON_OFFSET_LINE));
            OCGE( glPolygonOffset( slopeScaleBias * biasSign, constantBias * biasSign ) );
        }
        else
        {
            OGRE_CHECK_GL_ERROR(glDisable(GL_POLYGON_OFFSET_FILL));
            OGRE_CHECK_GL_ERROR(glDisable(GL_POLYGON_OFFSET_POINT));
            OGRE_CHECK_GL_ERROR(glDisable(GL_POLYGON_OFFSET_LINE));
        }
    }

    void GL3PlusRenderSystem::_makeRsProjectionMatrix( const Matrix4& matrix,
                                                       Matrix4& dest, Real nearPlane,
                                                       Real farPlane, ProjectionType projectionType )
    {
        if( !mReverseDepth )
        {
            // no any conversion request for OpenGL
            dest = matrix;
        }
        else
        {
            RenderSystem::_makeRsProjectionMatrix( matrix, dest, nearPlane, farPlane, projectionType );
        }
    }

    void GL3PlusRenderSystem::_convertProjectionMatrix( const Matrix4& matrix, Matrix4& dest )
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

    void GL3PlusRenderSystem::_convertOpenVrProjectionMatrix( const Matrix4& matrix, Matrix4& dest )
    {
        if( !mReverseDepth )
        {
            dest = matrix;

            // Convert depth range from [0,1] to [-1,1]
            dest[2][0] = (dest[2][0] + dest[3][0]) * 2.0f;
            dest[2][1] = (dest[2][1] + dest[3][1]) * 2.0f;
            dest[2][2] = (dest[2][2] + dest[3][2]) * 2.0f;
            dest[2][3] = (dest[2][3] + dest[3][3]) * 2.0f;
        }
        else
        {
            RenderSystem::_convertProjectionMatrix( matrix, dest );
        }
    }

    Real GL3PlusRenderSystem::getRSDepthRange(void) const
    {
        return mReverseDepth ? 1.0f : 2.0f;
    }

    HardwareOcclusionQuery* GL3PlusRenderSystem::createHardwareOcclusionQuery(void)
    {
        GL3PlusHardwareOcclusionQuery* ret = new GL3PlusHardwareOcclusionQuery();
        mHwOcclusionQueries.push_back(ret);
        return ret;
    }

    Real GL3PlusRenderSystem::getMinimumDepthInputValue(void)
    {
        // Range [-1.0f, 1.0f] or range [0.0f; 1.0f]
        return mReverseDepth ? 0.0f : -1.0f;
    }

    Real GL3PlusRenderSystem::getMaximumDepthInputValue(void)
    {
        // Range [-1.0f, 1.0f]
        return 1.0f;
    }

    void GL3PlusRenderSystem::setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );

        if( mStencilParams.enabled )
        {
            OCGE( glEnable(GL_STENCIL_TEST) );

            OCGE( glStencilMask( mStencilParams.writeMask ) );

            //OCGE( glStencilMaskSeparate( GL_BACK, mStencilParams.writeMask ) );
            //OCGE( glStencilMaskSeparate( GL_FRONT, mStencilParams.writeMask ) );

            OCGE( glStencilFuncSeparate( GL_BACK,
                                         convertCompareFunction( stencilParams.stencilBack.compareOp ),
                                         refValue, stencilParams.readMask ) );
            OCGE( glStencilOpSeparate( GL_BACK,
                                       convertStencilOp( stencilParams.stencilBack.stencilFailOp ),
                                       convertStencilOp( stencilParams.stencilBack.stencilDepthFailOp ),
                                       convertStencilOp( stencilParams.stencilBack.stencilPassOp ) ) );

            OCGE( glStencilFuncSeparate( GL_FRONT,
                                         convertCompareFunction( stencilParams.stencilFront.compareOp ),
                                         refValue, stencilParams.readMask ) );
            OCGE( glStencilOpSeparate( GL_FRONT,
                                       convertStencilOp( stencilParams.stencilFront.stencilFailOp ),
                                       convertStencilOp( stencilParams.stencilFront.stencilDepthFailOp ),
                                       convertStencilOp( stencilParams.stencilFront.stencilPassOp ) ) );
        }
        else
        {
            OCGE( glDisable(GL_STENCIL_TEST) );
        }
    }

    GLint GL3PlusRenderSystem::getCombinedMinMipFilter(void) const
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

    void GL3PlusRenderSystem::_render(const v1::RenderOperation& op)
    {
        // Call super class.
        RenderSystem::_render(op);

        // Create variables related to instancing.
        v1::HardwareVertexBufferSharedPtr globalInstanceVertexBuffer = getGlobalInstanceVertexBuffer();
        v1::VertexDeclaration* globalVertexDeclaration = getGlobalInstanceVertexBufferVertexDeclaration();
        bool hasInstanceData = (op.useGlobalInstancingVertexBufferIsAvailable &&
                                !globalInstanceVertexBuffer.isNull() && (globalVertexDeclaration != NULL))
            || op.vertexData->vertexBufferBinding->getHasInstanceData();

        size_t numberOfInstances = op.numberOfInstances;

        if (op.useGlobalInstancingVertexBufferIsAvailable)
        {
            numberOfInstances *= getGlobalNumberOfInstances();
        }

        // Get vertex array organization.
        const v1::VertexDeclaration::VertexElementList& decl =
            op.vertexData->vertexDeclaration->getElements();
        v1::VertexDeclaration::VertexElementList::const_iterator elemIter, elemEnd;
        elemEnd = decl.end();

        // Bind VAO (set of per-vertex attributes: position, normal, etc.).
        bool updateVAO = true;
        if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            GLSLSeparableProgram* separableProgram =
                GLSLSeparableProgramManager::getSingleton().getCurrentSeparableProgram();
            if (separableProgram)
            {
                if (!op.renderToVertexBuffer)
                {
                    separableProgram->activate();
                }

                updateVAO = !separableProgram->getVertexArrayObject()->isInitialised();

                separableProgram->getVertexArrayObject()->bind();
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(
                    "ERROR: Failed to create separable program.", LML_CRITICAL);
            }
        }
        else
        {
            GLSLMonolithicProgram* monolithicProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
            if (monolithicProgram)
            {
                updateVAO = !monolithicProgram->getVertexArrayObject()->isInitialised();

                monolithicProgram->getVertexArrayObject()->bind();
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(
                    "ERROR: Failed to create monolithic program.", LML_CRITICAL);
            }
        }

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

        if ( !globalInstanceVertexBuffer.isNull() && globalVertexDeclaration != NULL )
        {
            elemEnd = globalVertexDeclaration->getElements().end();
            for (elemIter = globalVertexDeclaration->getElements().begin(); elemIter != elemEnd; ++elemIter)
            {
                const v1::VertexElement & elem = *elemIter;
                bindVertexElementToGpu(elem, globalInstanceVertexBuffer, 0,
                                       mRenderAttribsBound, mRenderInstanceAttribsBound, updateVAO);
            }
        }

        activateGLTextureUnit(0);

        // Determine the correct primitive type to render.
        GLint primType;
        // Use adjacency if there is a geometry program and it requested adjacency info.
        bool useAdjacency = (mGeometryProgramBound && mPso->geometryShader && mPso->geometryShader->isAdjacencyInfoRequired());
        switch (op.operationType)
        {
        case OT_POINT_LIST:
            primType = GL_POINTS;
            break;
        case OT_LINE_LIST:
            primType = useAdjacency ? GL_LINES_ADJACENCY : GL_LINES;
            break;
        case OT_LINE_STRIP:
            primType = useAdjacency ? GL_LINE_STRIP_ADJACENCY : GL_LINE_STRIP;
            break;
        default:
        case OT_TRIANGLE_LIST:
            primType = useAdjacency ? GL_TRIANGLES_ADJACENCY : GL_TRIANGLES;
            break;
        case OT_TRIANGLE_STRIP:
            primType = useAdjacency ? GL_TRIANGLE_STRIP_ADJACENCY : GL_TRIANGLE_STRIP;
            break;
        case OT_TRIANGLE_FAN:
            primType = GL_TRIANGLE_FAN;
            break;
        }


        // Bind atomic counter buffers.
        // if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_ATOMIC_COUNTERS))
        // {
        //     GLuint atomicsBuffer = 0;

        //     glGenBuffers(1, &atomicsBuffer);
        //     glBindBuffer(GL_ATOMIC_COUNTER_BUFFER,
        //                  static_cast<GL3PlusHardwareCounterBuffer*>(HardwareBufferManager::getSingleton().getCounterBuffer().getGLBufferId()));
        //                  //static_cast<GL3PlusHardwareCounterBuffer*>(op..getCounterBuffer().getGLBufferId()));
        //     // glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint) * 3, NULL, GL_DYNAMIC_DRAW);
        //     // glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        // }
        //TODO: Reset atomic counters somewhere


        // Render to screen!
        if (mPso->domainShader)
        {
            // Tessellation shader special case.
            // Note: Only evaluation (domain) shaders are required.

            // GLuint primCount = 0;
            // // Useful primitives for tessellation
            // switch( op.operationType )
            // {
            // case OT_LINE_LIST:
            //     primCount = (GLuint)(op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount) / 2;
            //     break;

            // case OT_LINE_STRIP:
            //     primCount = (GLuint)(op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount) - 1;
            //     break;

            // case OT_TRIANGLE_LIST:
            //     primCount = (GLuint)(op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount);
            //     //primCount = (GLuint)(op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount) / 3;
            //     break;

            // case OT_TRIANGLE_STRIP:
            //     primCount = (GLuint)(op.useIndexes ? op.indexData->indexCount : op.vertexData->vertexCount) - 2;
            //     break;
            // default:
            //     break;
            // }

            // These are set via shader in DX11, SV_InsideTessFactor and SV_OutsideTessFactor
            // Hardcoding for the sample
            // float patchLevel(1.f);
            // OGRE_CHECK_GL_ERROR(glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, &patchLevel));
            // OGRE_CHECK_GL_ERROR(glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, &patchLevel));
            // OGRE_CHECK_GL_ERROR(glPatchParameteri(GL_PATCH_VERTICES, op.vertexData->vertexCount));

            if (op.useIndexes)
            {
                OGRE_CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                                 static_cast<v1::GL3PlusHardwareIndexBuffer*>(op.indexData->indexBuffer.get())->getGLBufferId()));
                void *pBufferData = GL_BUFFER_OFFSET(op.indexData->indexStart *
                                                     op.indexData->indexBuffer->getIndexSize());
                GLenum indexType = (op.indexData->indexBuffer->getType() == v1::HardwareIndexBuffer::IT_16BIT) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
                OGRE_CHECK_GL_ERROR(glDrawElements(GL_PATCHES, op.indexData->indexCount, indexType, pBufferData));
                //OGRE_CHECK_GL_ERROR(glDrawElements(GL_PATCHES, op.indexData->indexCount, indexType, pBufferData));
                //                OGRE_CHECK_GL_ERROR(glDrawArraysInstanced(GL_PATCHES, 0, primCount, 1));
            }
            else
            {
                OGRE_CHECK_GL_ERROR(glDrawArrays(GL_PATCHES, 0, op.vertexData->vertexCount));
                //OGRE_CHECK_GL_ERROR(glDrawArrays(GL_PATCHES, 0, primCount));
                //                OGRE_CHECK_GL_ERROR(glDrawArraysInstanced(GL_PATCHES, 0, primCount, 1));
            }
        }
        else if (op.useIndexes)
        {
            OGRE_CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                             static_cast<v1::GL3PlusHardwareIndexBuffer*>(op.indexData->indexBuffer.get())->getGLBufferId()));

            void *pBufferData = GL_BUFFER_OFFSET(op.indexData->indexStart *
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

                if(hasInstanceData)
                {
                    OGRE_CHECK_GL_ERROR(glDrawElementsInstancedBaseVertex(primType, op.indexData->indexCount, indexType, pBufferData, numberOfInstances, op.vertexData->vertexStart));
                }
                else
                {
                    OGRE_CHECK_GL_ERROR( glDrawElementsBaseVertex( primType, op.indexData->indexCount, indexType,
                                                                   pBufferData, op.vertexData->vertexStart ) );
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

                if (hasInstanceData)
                {
                    OGRE_CHECK_GL_ERROR(glDrawArraysInstanced(primType, 0, op.vertexData->vertexCount, numberOfInstances));
                }
                else
                {
                    OGRE_CHECK_GL_ERROR(glDrawArrays(primType, 0, op.vertexData->vertexCount));
                }
            } while (updatePassIterationRenderState());
        }

        // Unbind VAO (if updated).
        if (updateVAO)
        {
            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                GLSLSeparableProgram* separableProgram =
                    GLSLSeparableProgramManager::getSingleton().getCurrentSeparableProgram();
                if (separableProgram)
                {
                    separableProgram->getVertexArrayObject()->setInitialised(true);
                }
            }
            else
            {
                GLSLMonolithicProgram* monolithicProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
                if (monolithicProgram)
                {
                    monolithicProgram->getVertexArrayObject()->setInitialised(true);
                }
            }

            // Unbind the vertex array object.
            // Marks the end of what state will be included.
            OGRE_CHECK_GL_ERROR(glBindVertexArray(0));
        }


        mRenderAttribsBound.clear();
        mRenderInstanceAttribsBound.clear();
    }

    void GL3PlusRenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        glDispatchCompute( pso.mNumThreadGroups[0], pso.mNumThreadGroups[1], pso.mNumThreadGroups[2] );
    }

    void GL3PlusRenderSystem::_setVertexArrayObject( const VertexArrayObject *_vao )
    {
        if( _vao )
        {
            const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( _vao );
            OGRE_CHECK_GL_ERROR( glBindVertexArray( vao->mVaoName ) );
        }
        else
        {
            OGRE_CHECK_GL_ERROR( glBindVertexArray( 0 ) );
        }
    }

    void GL3PlusRenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];

        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        OCGE( glMultiDrawElementsIndirect( mode, indexType, cmd->indirectBufferOffset,
                                           cmd->numDraws, sizeof(CbDrawIndexed) ) );
    }

    void GL3PlusRenderSystem::_render( const CbDrawCallStrip *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];
        
        OCGE( glMultiDrawArraysIndirect( mode, cmd->indirectBufferOffset,
                                         cmd->numDraws, sizeof(CbDrawStrip) ) );
    }

    void GL3PlusRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];

        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();
        
        for( uint32 i=cmd->numDraws; i--; )
        {
            OCGE( glDrawElementsInstancedBaseVertexBaseInstance(
                      mode,
                      drawCmd->primCount,
                      indexType,
                      reinterpret_cast<void*>( drawCmd->firstVertexIndex * bytesPerIndexElement ),
                      drawCmd->instanceCount,
                      drawCmd->baseVertex,
                      drawCmd->baseInstance ) );
            ++drawCmd;
        }
    }

    void GL3PlusRenderSystem::_renderEmulated( const CbDrawCallStrip *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];

        CbDrawStrip *drawCmd = reinterpret_cast<CbDrawStrip*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );
        
        for( uint32 i=cmd->numDraws; i--; )
        {
            OCGE( glDrawArraysInstancedBaseInstance(
                      mode,
                      drawCmd->firstVertexIndex,
                      drawCmd->primCount,
                      drawCmd->instanceCount,
                      drawCmd->baseInstance ) );
            ++drawCmd;
        }
    }

    void GL3PlusRenderSystem::_renderEmulatedNoBaseInstance( const CbDrawCallIndexed *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];

        GLenum indexType = vao->mIndexBuffer->getIndexType() == IndexBufferPacked::IT_16BIT ?
                                                            GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();

        GLSLMonolithicProgram *activeLinkProgram =
                GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();

        for( uint32 i=cmd->numDraws; i--; )
        {
            OCGE( glUniform1ui( activeLinkProgram->mBaseInstanceLocation,
                                static_cast<GLuint>( drawCmd->baseInstance ) ) );

            OCGE( glDrawElementsInstancedBaseVertex(
                    mode,
                    drawCmd->primCount,
                    indexType,
                    reinterpret_cast<void*>( drawCmd->firstVertexIndex * bytesPerIndexElement ),
                    drawCmd->instanceCount,
                    drawCmd->baseVertex ) );
            ++drawCmd;
        }
    }

    void GL3PlusRenderSystem::_renderEmulatedNoBaseInstance( const CbDrawCallStrip *cmd )
    {
        const GL3PlusVertexArrayObject *vao = static_cast<const GL3PlusVertexArrayObject*>( cmd->vao );
        GLenum mode = mPso->domainShader ? GL_PATCHES : vao->mPrimType[mUseAdjacency];

        CbDrawStrip *drawCmd = reinterpret_cast<CbDrawStrip*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        GLSLMonolithicProgram *activeLinkProgram =
                GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();

        for( uint32 i=cmd->numDraws; i--; )
        {
            OCGE( glUniform1ui( activeLinkProgram->mBaseInstanceLocation,
                                static_cast<GLuint>( drawCmd->baseInstance ) ) );

            OCGE( glDrawArraysInstanced(
                    mode,
                    drawCmd->firstVertexIndex,
                    drawCmd->primCount,
                    drawCmd->instanceCount ) );
            ++drawCmd;
        }
    }

    void GL3PlusRenderSystem::_startLegacyV1Rendering(void)
    {
        glBindVertexArray( mGlobalVao );
    }

    void GL3PlusRenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd )
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
            GLuint attributeIndex = GL3PlusVaoManager::getAttributeIndexFor( semantic ) +
                                    elem.getIndex();

            if( !vertexBufferBinding->isBufferBound( source ) )
            {
                OCGE( glDisableVertexAttribArray( attributeIndex ) );
                ++itor;
                continue; // Skip unbound elements.
            }

            v1::HardwareVertexBufferSharedPtr vertexBuffer = vertexBufferBinding->getBuffer( source );
            const v1::GL3PlusHardwareVertexBuffer* hwGlBuffer =
                            static_cast<v1::GL3PlusHardwareVertexBuffer*>( vertexBuffer.get() );

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

            GLenum type = v1::GL3PlusHardwareBufferManager::getGLType( vertexElementType );

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
            case VET_DOUBLE1:
                OCGE( glVertexAttribLPointer( attributeIndex, typeCount,
                                              type,
                                              static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                              bindOffset ) );
                break;
            }

            OCGE( glVertexAttribDivisor( attributeIndex, hwGlBuffer->getInstanceDataStepRate() *
                                         hwGlBuffer->getIsInstanceData() ) );
            OCGE( glEnableVertexAttribArray( attributeIndex ) );

            ++itor;
        }

        if( cmd->indexData )
        {
            v1::GL3PlusHardwareIndexBuffer *indexBuffer = static_cast<v1::GL3PlusHardwareIndexBuffer*>(
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
            mCurrentPolygonMode = mUseAdjacency ? GL_LINES_ADJACENCY : GL_LINES;
            break;
        case OT_LINE_STRIP:
            mCurrentPolygonMode = mUseAdjacency ? GL_LINE_STRIP_ADJACENCY : GL_LINE_STRIP;
            break;
        default:
        case OT_TRIANGLE_LIST:
            mCurrentPolygonMode = mUseAdjacency ? GL_TRIANGLES_ADJACENCY : GL_TRIANGLES;
            break;
        case OT_TRIANGLE_STRIP:
            mCurrentPolygonMode = mUseAdjacency ? GL_TRIANGLE_STRIP_ADJACENCY : GL_TRIANGLE_STRIP;
            break;
        case OT_TRIANGLE_FAN:
            mCurrentPolygonMode = GL_TRIANGLE_FAN;
            break;
        }
    }

    void GL3PlusRenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        GLenum indexType = mCurrentIndexBuffer->indexBuffer->getType() ==
                            v1::HardwareIndexBuffer::IT_16BIT ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

        OCGE( glDrawElementsInstancedBaseVertexBaseInstance(
                    mCurrentPolygonMode,
                    cmd->primCount,
                    indexType,
                    reinterpret_cast<void*>( cmd->firstVertexIndex * bytesPerIndexElement ),
                    cmd->instanceCount,
                    mCurrentVertexBuffer->vertexStart,
                    cmd->baseInstance ) );
    }

    void GL3PlusRenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
        OCGE( glDrawArraysInstancedBaseInstance(
                    mCurrentPolygonMode,
                    cmd->firstVertexIndex,
                    cmd->primCount,
                    cmd->instanceCount,
                    cmd->baseInstance ) );
    }

    void GL3PlusRenderSystem::_renderNoBaseInstance( const v1::CbDrawCallIndexed *cmd )
    {
        GLenum indexType = mCurrentIndexBuffer->indexBuffer->getType() ==
                            v1::HardwareIndexBuffer::IT_16BIT ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

        GLSLMonolithicProgram *activeLinkProgram =
                GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();

        OCGE( glUniform1ui( activeLinkProgram->mBaseInstanceLocation,
                            static_cast<GLuint>( cmd->baseInstance ) ) );

        OCGE( glDrawElementsInstancedBaseVertex(
                    mCurrentPolygonMode,
                    cmd->primCount,
                    indexType,
                    reinterpret_cast<void*>(cmd->firstVertexIndex * bytesPerIndexElement),
                    cmd->instanceCount,
                    mCurrentVertexBuffer->vertexStart ) );
    }

    void GL3PlusRenderSystem::_renderNoBaseInstance( const v1::CbDrawCallStrip *cmd )
    {
        GLSLMonolithicProgram *activeLinkProgram =
                GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();

        OCGE( glUniform1ui( activeLinkProgram->mBaseInstanceLocation,
                            static_cast<GLuint>( cmd->baseInstance ) ) );

        OCGE( glDrawArraysInstanced(
                    mCurrentPolygonMode,
                    cmd->firstVertexIndex,
                    cmd->primCount,
                    cmd->instanceCount ) );
    }

    void GL3PlusRenderSystem::clearFrameBuffer( RenderPassDescriptor *desc,
                                                TextureGpu *anyTarget, uint8 mipLevel )
    {
        Vector4 fullVp( 0, 0, 1, 1 );
        beginRenderPassDescriptor( desc, anyTarget, mipLevel, &fullVp, &fullVp, 1u, false, false );
    }

    void GL3PlusRenderSystem::_switchContext(GL3PlusContext *context)
    {
        // Unbind GPU programs and rebind to new context later, because
        // scene manager treat render system as ONE 'context' ONLY, and it
		// cached the GPU programs using state.
        if( mPso )
        {
            if (mPso->vertexShader)
                mPso->vertexShader->unbind();
            if (mPso->geometryShader)
                mPso->geometryShader->unbind();
            if (mPso->pixelShader)
                mPso->pixelShader->unbind();
            if (mPso->hullShader)
                mPso->hullShader->unbind();
            if (mPso->domainShader)
                mPso->domainShader->unbind();
        }
        if (mCurrentComputeShader)
            mCurrentComputeShader->unbind();

        // It's ready for switching
        if (mCurrentContext!=context)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // NSGLContext::makeCurrentContext does not flush automatically. everybody else does.
            glFlushRenderAPPLE();
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
            if (mPso->geometryShader)
                mPso->geometryShader->bind();
            if (mPso->pixelShader)
                mPso->pixelShader->bind();
            if (mPso->hullShader)
                mPso->hullShader->bind();
            if (mPso->domainShader)
                mPso->domainShader->bind();
        }

        if (mCurrentComputeShader)
            mCurrentComputeShader->bind();

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

    void GL3PlusRenderSystem::_unregisterContext(GL3PlusContext *context)
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
                /// No contexts remain
                mCurrentContext->endCurrent();
                mCurrentContext = 0;
                mMainContext = 0;
            }
        }
    }

    void GL3PlusRenderSystem::_oneTimeContextInitialization()
    {
        OGRE_CHECK_GL_ERROR(glDisable(GL_DITHER));

        if( mReverseDepth &&
            (mGLSupport->hasMinGLVersion(4, 5) || mGLSupport->checkExtension( "GL_ARB_clip_control" )) )
        {
            OCGE( glClipControl( GL_LOWER_LEFT, GL_ZERO_TO_ONE ) );
        }
        else
        {
            mReverseDepth = false;
        }

        // Check for FSAA
        // Enable the extension if it was enabled by the GL3PlusSupport
        int fsaa_active = false;
        OGRE_CHECK_GL_ERROR(glGetIntegerv(GL_SAMPLE_BUFFERS, (GLint*)&fsaa_active));
        if (fsaa_active)
        {
            OGRE_CHECK_GL_ERROR(glEnable(GL_MULTISAMPLE));
            LogManager::getSingleton().logMessage("Using FSAA.");
        }

        if (mGLSupport->checkExtension("GL_EXT_texture_filter_anisotropic"))
        {
            OGRE_CHECK_GL_ERROR(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &mLargestSupportedAnisotropy));
        }

        OCGE( glGenFramebuffers( 1, &mNullColourFramebuffer ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        // Some Apple NVIDIA hardware can't handle seamless cubemaps
        if (mCurrentCapabilities->getVendor() != GPU_NVIDIA)
#endif
            // Enable seamless cube maps
            OGRE_CHECK_GL_ERROR(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));

        // Set provoking vertex convention
        OGRE_CHECK_GL_ERROR(glProvokingVertex(GL_FIRST_VERTEX_CONVENTION));

        g_hasDebugObjectLabel = false;
        if (mGLSupport->checkExtension("GL_KHR_debug") || mHasGL43)
        {
#if OGRE_DEBUG_MODE
            OGRE_CHECK_GL_ERROR(glDebugMessageCallbackARB(&GLDebugCallback, NULL));
            OGRE_CHECK_GL_ERROR(glDebugMessageControlARB(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, NULL, GL_TRUE));
            OGRE_CHECK_GL_ERROR(glEnable(GL_DEBUG_OUTPUT));
            OGRE_CHECK_GL_ERROR(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
#endif
            g_hasDebugObjectLabel = true;
        }
    }

    void GL3PlusRenderSystem::initialiseContext( Window *primary )
    {
        // Set main and current context
        mMainContext = 0;
        primary->getCustomAttribute( "GLCONTEXT", &mMainContext );
        mCurrentContext = mMainContext;

        // Set primary context as active
        if (mCurrentContext)
            mCurrentContext->setCurrent();

        // Initialise GL3W
		bool gl3wFailed = gl3wInit() != 0;
        if( gl3wFailed )
        {
            LogManager::getSingleton().logMessage("Failed to initialize GL3W", LML_CRITICAL);
        }
        else
        {
            // Setup GL3PlusSupport
            mGLSupport->initialiseExtensions();
        }

        // Make sure that OpenGL 3.3+ is supported in this context
        if( gl3wFailed || !mGLSupport->hasMinGLVersion(3, 3) )
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "OpenGL 3.3 is not supported. Please update your graphics card drivers.",
                        "GL3PlusRenderSystem::initialiseContext");
        }

        mHasGL43 = mGLSupport->hasMinGLVersion(4, 3);

        LogManager::getSingleton().logMessage("**************************************");
        LogManager::getSingleton().logMessage("***   OpenGL 3+ Renderer Started   ***");
        LogManager::getSingleton().logMessage("**************************************");
    }

    GLint GL3PlusRenderSystem::convertCompareFunction(CompareFunction func) const
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
        };
        // To keep compiler happy
        return GL_ALWAYS;
    }

    GLint GL3PlusRenderSystem::convertStencilOp(StencilOperation op) const
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
            return GL_INCR;
        case SOP_DECREMENT:
            return GL_DECR;
        case SOP_INCREMENT_WRAP:
            return GL_INCR_WRAP;
        case SOP_DECREMENT_WRAP:
            return GL_DECR_WRAP;
        case SOP_INVERT:
            return GL_INVERT;
        };
        // to keep compiler happy
        return SOP_KEEP;
    }

    void GL3PlusRenderSystem::bindGpuProgramParameters(GpuProgramType gptype, GpuProgramParametersSharedPtr params, uint16 mask)
    {
        //              if (mask & (uint16)GPV_GLOBAL)
        //              {
        //TODO We could maybe use GL_EXT_bindable_uniform here to produce Dx10-style
        // shared constant buffers, but GPU support seems fairly weak?
        // check the match to constant buffers & use rendersystem data hooks to store
        // for now, just copy
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
        case GPT_GEOMETRY_PROGRAM:
            mActiveGeometryGpuProgramParameters = params;
            mPso->geometryShader->bindSharedParameters(params, mask);
            break;
        case GPT_HULL_PROGRAM:
            mActiveTessellationHullGpuProgramParameters = params;
            mPso->hullShader->bindSharedParameters(params, mask);
            break;
        case GPT_DOMAIN_PROGRAM:
            mActiveTessellationDomainGpuProgramParameters = params;
            mPso->domainShader->bindSharedParameters(params, mask);
            break;
        case GPT_COMPUTE_PROGRAM:
            mActiveComputeGpuProgramParameters = params;
            mCurrentComputeShader->bindSharedParameters(params, mask);
            break;
        default:
            break;
        }
        //              }
        //        else
        //        {
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
        case GPT_GEOMETRY_PROGRAM:
            mActiveGeometryGpuProgramParameters = params;
            mPso->geometryShader->bindParameters(params, mask);
            break;
        case GPT_HULL_PROGRAM:
            mActiveTessellationHullGpuProgramParameters = params;
            mPso->hullShader->bindParameters(params, mask);
            break;
        case GPT_DOMAIN_PROGRAM:
            mActiveTessellationDomainGpuProgramParameters = params;
            mPso->domainShader->bindParameters(params, mask);
            break;
        case GPT_COMPUTE_PROGRAM:
            mActiveComputeGpuProgramParameters = params;
            mCurrentComputeShader->bindParameters(params, mask);
            break;
        default:
            break;
        }
        //        }

        //FIXME This needs to be moved somewhere texture specific.
        // Update image bindings for image load/store
        // static_cast<GL3PlusTextureManager*>(mTextureManager)->bindImages();
    }

    void GL3PlusRenderSystem::bindGpuProgramPassIterationParameters(GpuProgramType gptype)
    {
        switch (gptype)
        {
        case GPT_VERTEX_PROGRAM:
            mPso->vertexShader->bindPassIterationParameters(mActiveVertexGpuProgramParameters);
            break;
        case GPT_FRAGMENT_PROGRAM:
            mPso->pixelShader->bindPassIterationParameters(mActiveFragmentGpuProgramParameters);
            break;
        case GPT_GEOMETRY_PROGRAM:
            mPso->geometryShader->bindPassIterationParameters(mActiveGeometryGpuProgramParameters);
            break;
        case GPT_HULL_PROGRAM:
            mPso->hullShader->bindPassIterationParameters(mActiveTessellationHullGpuProgramParameters);
            break;
        case GPT_DOMAIN_PROGRAM:
            mPso->domainShader->bindPassIterationParameters(mActiveTessellationDomainGpuProgramParameters);
            break;
        case GPT_COMPUTE_PROGRAM:
            mCurrentComputeShader->bindPassIterationParameters(mActiveComputeGpuProgramParameters);
            break;
        default:
            break;
        }
    }

    void GL3PlusRenderSystem::setClipPlanesImpl(const Ogre::PlaneList& planeList)
    {
        OGRE_CHECK_GL_ERROR(glEnable(GL_DEPTH_CLAMP));
    }

    void GL3PlusRenderSystem::registerThread()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // This is only valid once we've created the main context
        if (!mMainContext)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                        "Cannot register a background thread before the main context "
                        "has been created.",
                        "GL3PlusRenderSystem::registerThread");
        }

        // Create a new context for this thread. Cloning from the main context
        // will ensure that resources are shared with the main context
        // We want a separate context so that we can safely create GL
        // objects in parallel with the main thread
        GL3PlusContext* newContext = mMainContext->clone();
        mBackgroundContextList.push_back(newContext);

        // Bind this new context to this thread.
        newContext->setCurrent();

        _oneTimeContextInitialization();
        newContext->setInitialized();
    }

    void GL3PlusRenderSystem::unregisterThread()
    {
        // nothing to do here?
        // Don't need to worry about active context, just make sure we delete
        // on shutdown.
    }

    void GL3PlusRenderSystem::preExtraThreadsStarted()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // free context, we'll need this to share lists
        if (mCurrentContext)
            mCurrentContext->endCurrent();
    }

    void GL3PlusRenderSystem::postExtraThreadsStarted()
    {
        OGRE_LOCK_MUTEX(mThreadInitMutex);
        // reacquire context
        if (mCurrentContext)
            mCurrentContext->setCurrent();
    }

    unsigned int GL3PlusRenderSystem::getDisplayMonitorCount() const
    {
        return mGLSupport->getDisplayMonitorCount();
    }


    void GL3PlusRenderSystem::beginProfileEvent( const String &eventName )
    {
        markProfileEvent("Begin Event: " + eventName);
        if( mHasGL43 || mGLSupport->checkExtension("ARB_debug_group") )
        {
            OCGE( glPushDebugGroup( GL_DEBUG_SOURCE_THIRD_PARTY, 0,
                                    static_cast<GLint>( eventName.length() ),
                                    eventName.c_str() ) );
        }
    }


    void GL3PlusRenderSystem::endProfileEvent( void )
    {
        markProfileEvent("End Event");
        if( mHasGL43 || mGLSupport->checkExtension("ARB_debug_group") )
            OCGE( glPopDebugGroup() );
    }


    void GL3PlusRenderSystem::markProfileEvent( const String &eventName )
    {
        if( eventName.empty() )
            return;

        if( mHasGL43 || mGLSupport->checkExtension("GL_KHR_debug") )
        {
            glDebugMessageInsert( GL_DEBUG_SOURCE_THIRD_PARTY,
                                  GL_DEBUG_TYPE_PERFORMANCE,
                                  0,
                                  GL_DEBUG_SEVERITY_LOW,
                                  static_cast<GLint>( eventName.length() ),
                                  eventName.c_str() );
        }
    }

    void GL3PlusRenderSystem::initGPUProfiling(void)
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_BindOpenGL();
#endif
    }

    void GL3PlusRenderSystem::deinitGPUProfiling(void)
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_UnbindOpenGL();
#endif
    }

    void GL3PlusRenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_BeginOpenGLSample( name.c_str(), hashCache );
#endif
    }

    void GL3PlusRenderSystem::endGPUSampleProfile( const String &name )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_EndOpenGLSample();
#endif
    }

    bool GL3PlusRenderSystem::activateGLTextureUnit(size_t unit)
    {
        if (mActiveTextureUnit != unit)
        {
            if (unit < getCapabilities()->getNumTextureUnits())
            {
                OGRE_CHECK_GL_ERROR(glActiveTexture(static_cast<uint32>(GL_TEXTURE0 + unit)));
                mActiveTextureUnit = static_cast<GLenum>(unit);
                return true;
            }
            else if (!unit)
            {
                //FIXME If the above case fails, should this case ever be taken?
                // Also switch to (unit == number) unless not operation is actually
                // faster on some architectures.

                // Always OK to use the first unit.
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return true;
        }
    }

    void GL3PlusRenderSystem::bindVertexElementToGpu( const v1::VertexElement &elem,
                                                      v1::HardwareVertexBufferSharedPtr vertexBuffer,
                                                      const size_t vertexStart,
                                                      vector<GLuint>::type &attribsBound,
                                                      vector<GLuint>::type &instanceAttribsBound,
                                                      bool updateVAO)
    {
        const v1::GL3PlusHardwareVertexBuffer* hwGlBuffer = static_cast<const v1::GL3PlusHardwareVertexBuffer*>(
                                                                                            vertexBuffer.get());

        // FIXME: Having this commented out fixes some rendering issues but leaves VAO's useless
        // if (updateVAO)
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

            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                GLSLSeparableProgram* separableProgram =
                    GLSLSeparableProgramManager::getSingleton().getCurrentSeparableProgram();
                if (!separableProgram || !separableProgram->isAttributeValid(sem, elemIndex))
                {
                    return;
                }

                attrib = (GLuint)separableProgram->getAttributeIndex(sem, elemIndex);
            }
            else
            {
                GLSLMonolithicProgram* monolithicProgram = GLSLMonolithicProgramManager::getSingleton().getActiveMonolithicProgram();
                if (!monolithicProgram || !monolithicProgram->isAttributeValid(sem, elemIndex))
                {
                    return;
                }

                attrib = (GLuint)monolithicProgram->getAttributeIndex(sem, elemIndex);
            }

            if (mPso->vertexShader)
            {
                if (hwGlBuffer->getIsInstanceData())
                {
                    OGRE_CHECK_GL_ERROR(glVertexAttribDivisor(attrib, hwGlBuffer->getInstanceDataStepRate()));
                    instanceAttribsBound.push_back(attrib);
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

            switch(elem.getBaseType(elem.getType()))
            {
            default:
            case VET_FLOAT1:
                OGRE_CHECK_GL_ERROR(glVertexAttribPointer(attrib,
                                                          typeCount,
                                                          v1::GL3PlusHardwareBufferManager::getGLType(elem.getType()),
                                                          normalised,
                                                          static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                                          pBufferData));
                break;
            case VET_DOUBLE1:
                OGRE_CHECK_GL_ERROR(glVertexAttribLPointer(attrib,
                                                           typeCount,
                                                           v1::GL3PlusHardwareBufferManager::getGLType(elem.getType()),
                                                           static_cast<GLsizei>(vertexBuffer->getVertexSize()),
                                                           pBufferData));
                break;
            }

            // If this attribute hasn't been enabled, do so and keep a record of it.
            OGRE_CHECK_GL_ERROR(glEnableVertexAttribArray(attrib));

            attribsBound.push_back(attrib);
        }
    }
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
	bool GL3PlusRenderSystem::setDrawBuffer(ColourBufferType colourBuffer)
	{
		bool result = true;

		switch (colourBuffer)
		{
            case CBT_BACK:
                OGRE_CHECK_GL_ERROR(glDrawBuffer(GL_BACK));
                break;
            case CBT_BACK_LEFT:
                OGRE_CHECK_GL_ERROR(glDrawBuffer(GL_BACK_LEFT));
                break;
            case CBT_BACK_RIGHT:
                OGRE_CHECK_GL_ERROR(glDrawBuffer(GL_BACK_RIGHT));
//                break;
            default:
                result = false;
		}

		return result;
	}
#endif
    bool GL3PlusRenderSystem::checkExtension( const String &ext ) const
    {
        return mGLSupport->checkExtension( ext );
    }

    const PixelFormatToShaderType* GL3PlusRenderSystem::getPixelFormatToShaderType(void) const
    {
        return &mPixelFormatToShaderType;
    }
    //---------------------------------------------------------------------
    void GL3PlusRenderSystem::_clearStateAndFlushCommandBuffer(void)
    {
        OgreProfileExhaustive( "GL3PlusRenderSystem::_clearStateAndFlushCommandBuffer" );
        OCGE( glFlush() );
    }
    //---------------------------------------------------------------------
    void GL3PlusRenderSystem::flushCommands(void)
    {
        OgreProfileExhaustive( "GL3PlusRenderSystem::flushCommands" );
        OCGE( glFlush() );
    }
}
