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
#include "OgreGpuProgram.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreStringConverter.h"

#include "OgreGLSLESShader.h"
#include "OgreGLSLESLinkProgramManager.h"
#include "OgreGLSLESProgramPipelineManager.h"
#include "OgreGLSLESPreprocessor.h"
#include "OgreGLES2Util.h"
#include "OgreGLES2RenderSystem.h"

namespace Ogre {

    GLSLESShader::CmdPreprocessorDefines GLSLESShader::msCmdPreprocessorDefines;
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
    GLSLESShader::CmdOptimisation GLSLESShader::msCmdOptimisation;
#endif

    GLuint GLSLESShader::mShaderCount = 0;

    GLSLESShader::GLSLESShader(
        ResourceManager* creator,
        const String& name, ResourceHandle handle,
        const String& group, bool isManual, ManualResourceLoader* loader)
        : HighLevelGpuProgram(creator, name, handle, group, isManual, loader) 
        , mGLShaderHandle(0)
        , mGLProgramHandle(0)
        , mCompiled(0)
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
        , mIsOptimised(false)
        , mOptimiserEnabled(false)
#endif
    {
        if (createParamDictionary("GLSLESShader"))
        {
            setupBaseParamDictionary();
            ParamDictionary* dict = getParamDictionary();

            dict->addParameter(ParameterDef("preprocessor_defines", 
                                            "Preprocessor defines use to compile the program.",
                                            PT_STRING),&msCmdPreprocessorDefines);
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
            dict->addParameter(ParameterDef("use_optimiser", 
                                            "Should the GLSL optimiser be used. Default is false.",
                                            PT_BOOL),&msCmdOptimisation);
#endif
        }

        mType = GPT_VERTEX_PROGRAM; // default value, to be corrected after the constructor with GpuProgram::setType()
        mSyntaxCode = "glsles"; // Manually assign language now since we use it immediately

        mLinked = 0;
        // Increase shader counter and use as ID
        mShaderID = ++mShaderCount;
        
        // Transfer skeletal animation status from parent
        mSkeletalAnimation = isSkeletalAnimationIncluded();
        // There is nothing to load
        mLoadFromFile = false;
    }

    GLSLESShader::~GLSLESShader()
    {
        // Have to call this here rather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if (isLoaded())
        {
            unload();
        }
        else
        {
            unloadHighLevel();
        }
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    void GLSLESShader::notifyOnContextLost()
    {
        unloadHighLevelImpl();
    }
#endif

    void GLSLESShader::loadFromSource(void)
    {
        // Preprocess the GLSL ES shader in order to get a clean source
        CPreprocessor cpp;

        // Define "predefined" macros.
        // TODO: decide, should we define __VERSION__, and with what value.
        if(getLanguage() == "glsles")
            cpp.Define("GL_ES", 5, 1);

        // Pass all user-defined macros to preprocessor
        if (!mPreprocessorDefines.empty ())
        {
            String::size_type pos = 0;
            while (pos != String::npos)
            {
                // Find delims
                String::size_type endPos = mPreprocessorDefines.find_first_of(";,=", pos);
                if (endPos != String::npos)
                {
                    String::size_type macro_name_start = pos;
                    size_t macro_name_len = endPos - pos;
                    pos = endPos;

                    // Check definition part
                    if (mPreprocessorDefines[pos] == '=')
                    {
                        // Set up a definition, skip delim
                        ++pos;
                        String::size_type macro_val_start = pos;
                        size_t macro_val_len;

                        endPos = mPreprocessorDefines.find_first_of(";,", pos);
                        if (endPos == String::npos)
                        {
                            macro_val_len = mPreprocessorDefines.size () - pos;
                            pos = endPos;
                        }
                        else
                        {
                            macro_val_len = endPos - pos;
                            pos = endPos+1;
                        }
                        cpp.Define (
                            mPreprocessorDefines.c_str () + macro_name_start, macro_name_len,
                            mPreprocessorDefines.c_str () + macro_val_start, macro_val_len);
                    }
                    else
                    {
                        // No definition part, define as "1"
                        ++pos;
                        cpp.Define (
                            mPreprocessorDefines.c_str () + macro_name_start, macro_name_len, 1);
                    }
                }
                else
                {
                    if(pos < mPreprocessorDefines.size())
                         cpp.Define (mPreprocessorDefines.c_str () + pos, mPreprocessorDefines.size() - pos, 1);
 
                    pos = endPos;
                }
            }
        }

        size_t out_size = 0;
        const char *src = mSource.c_str ();
        size_t src_len = mSource.size ();
        char *out = cpp.Parse (src, src_len, out_size);
        if (!out || !out_size)
        {
            mCompileError = true;
            // Failed to preprocess, break out
            OGRE_EXCEPT (Exception::ERR_RENDERINGAPI_ERROR,
                         "Failed to preprocess shader " + mName,
                         __FUNCTION__);
        }

        mSource = String (out, out_size);
        if (out < src || out > src + src_len)
            free (out);
    }

    bool GLSLESShader::compile(const bool checkErrors)
    {
        if (mCompiled == 1)
        {
            return true;
        }

        // Only create a shader object if glsl es is supported
        if (isSupported())
        {
            // Create shader object.
            GLenum GLShaderType = getGLShaderType(mType);
            OGRE_CHECK_GL_ERROR(mGLShaderHandle = glCreateShader(GLShaderType));

#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
            GLES2RenderSystem* rs = getGLES2RenderSystem();
            if(rs->checkExtension("GL_EXT_debug_label"))
            {
                glLabelObjectEXT(GL_SHADER_OBJECT_EXT, mGLShaderHandle, 0, mName.c_str());
            }
#endif

            // if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            // {
            //     OGRE_CHECK_GL_ERROR(mGLProgramHandle = glCreateProgram());
            // #if OGRE_PLATFORM != OGRE_PLATFORM_NACL
            //     if(rs->checkExtension("GL_EXT_debug_label"))
            //     {
            //         glLabelObjectEXT(GL_PROGRAM_OBJECT_EXT, mGLProgramHandle, 0, mName.c_str());
            //     }
            // #endif
            // }
        }

        // Add boiler plate code and preprocessor extras, then
        // submit shader source to OpenGL.
        if (!mSource.empty())
        {
            // Fix up the source in case someone forgot to redeclare gl_Position
            if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS) &&
                mType == GPT_VERTEX_PROGRAM)
            {
                size_t versionPos = mSource.find("#version");
                int shaderVersion = StringConverter::parseInt(mSource.substr(versionPos+9, 3));

                // Check that it's missing and that this shader has a main function, ie. not a child shader.
                if(mSource.find("out highp vec4 gl_Position") == String::npos)
                {
                    if(shaderVersion >= 300)
                        mSource.insert(versionPos+16, "out highp vec4 gl_Position;\nout highp float gl_PointSize;\n");
                }
                if(mSource.find("#extension GL_EXT_separate_shader_objects : require") == String::npos)
                {
                    if(shaderVersion >= 300)
                        mSource.insert(versionPos+16, "#extension GL_EXT_separate_shader_objects : require\n");
                }
            }
    
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
            const char *source = (getOptimiserEnabled() && getIsOptimised()) ? mOptimisedSource.c_str() : mSource.c_str();
#else
            const char *source = mSource.c_str();
#endif
            OGRE_CHECK_GL_ERROR(glShaderSource(mGLShaderHandle, 1, &source, NULL));
        }

        if (checkErrors)
            logObjectInfo("GLSL ES compiling: " + mName, mGLShaderHandle);

        OGRE_CHECK_GL_ERROR(glCompileShader(mGLShaderHandle));

        // Check for compile errors
        OGRE_CHECK_GL_ERROR(glGetShaderiv(mGLShaderHandle, GL_COMPILE_STATUS, &mCompiled));
        if(!mCompiled && checkErrors)
        {
            String message = logObjectInfo("GLSL ES compile log: " + mName, mGLShaderHandle);
            checkAndFixInvalidDefaultPrecisionError(message);
        }

        // Log a message that the shader compiled successfully.
        if (mCompiled && checkErrors)
            logObjectInfo("GLSL ES compiled: " + mName, mGLShaderHandle);

        if(!mCompiled)
        {
            mCompileError = true;

            String shaderType = getShaderTypeLabel(mType);
            StringUtil::toTitleCase(shaderType);
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        shaderType + " Program " + mName +
                        " failed to compile. See compile log above for details.",
                        "GLSLESShader::compile");
        }

        return (mCompiled == 1);
    }

#if !OGRE_NO_GLES2_GLSL_OPTIMISER   
    void GLSLESShader::setOptimiserEnabled(bool enabled)
    { 
        if(mOptimiserEnabled != enabled && mOptimiserEnabled && mCompiled == 1)
        {
            OGRE_CHECK_GL_ERROR(glDeleteShader(mGLShaderHandle));

            if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS) && mGLProgramHandle)
            {
                OGRE_CHECK_GL_ERROR(glDeleteProgram(mGLProgramHandle));
            }
            
            mGLShaderHandle = 0;
            mGLProgramHandle = 0;
            mCompiled = 0;
        }
        mOptimiserEnabled = enabled; 
    }
#endif

    void GLSLESShader::createLowLevelImpl(void)
    {
        // mAssemblerProgram = GpuProgramPtr(OGRE_NEW GLSLESShader( this ));
    }

    void GLSLESShader::unloadImpl()
    {   
        // We didn't create mAssemblerProgram through a manager, so override this
        // implementation so that we don't try to remove it from one. Since getCreator()
        // is used, it might target a different matching handle!
        // mAssemblerProgram.setNull();

        unloadHighLevel();
    }

    void GLSLESShader::unloadHighLevelImpl(void)
    {
        if (isSupported())
        {
            OGRE_CHECK_GL_ERROR(glDeleteShader(mGLShaderHandle));

            if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS) && mGLProgramHandle)
            {
                OGRE_CHECK_GL_ERROR(glDeleteProgram(mGLProgramHandle));
            }
            
            mGLShaderHandle = 0;
            mGLProgramHandle = 0;
            mCompiled = 0;
        }
    }

    void GLSLESShader::populateParameterNames(GpuProgramParametersSharedPtr params)
    {
        getConstantDefinitions();
        params->_setNamedConstants(mConstantDefs);
        // Don't set logical / physical maps here, as we can't access parameters by logical index in GLSL.
    }

    void GLSLESShader::buildConstantDefinitions() const
    {
        // We need an accurate list of all the uniforms in the shader, but we
        // can't get at them until we link all the shaders into a program object.

        // Therefore instead parse the source code manually and extract the uniforms.
        createParameterMappingStructures(true);
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            GLSLESProgramPipelineManager::getSingleton().extractUniformsFromGLSL(mSource, *mConstantDefs.get(), mName);
        }
        else
        {
            GLSLESLinkProgramManager::getSingleton().extractUniformsFromGLSL(mSource, *mConstantDefs.get(), mName);
        }
    }

    inline bool GLSLESShader::getPassSurfaceAndLightStates(void) const
    {
        // Scenemanager should pass on light & material state to the rendersystem.
        return true;
    }

    inline bool GLSLESShader::getPassTransformStates(void) const
    {
        // Scenemanager should pass on transform state to the rendersystem.
        return true;
    }

    inline bool GLSLESShader::getPassFogStates(void) const
    {
        // Scenemanager should pass on fog state to the rendersystem.
        return true;
    }

    String GLSLESShader::CmdPreprocessorDefines::doGet(const void *target) const
    {
        return static_cast<const GLSLESShader*>(target)->getPreprocessorDefines();
    }
    void GLSLESShader::CmdPreprocessorDefines::doSet(void *target, const String& val)
    {
        static_cast<GLSLESShader*>(target)->setPreprocessorDefines(val);
    }

#if !OGRE_NO_GLES2_GLSL_OPTIMISER
    String GLSLESShader::CmdOptimisation::doGet(const void *target) const
    {
        return StringConverter::toString(static_cast<const GLSLESShader*>(target)->getOptimiserEnabled());
    }
    void GLSLESShader::CmdOptimisation::doSet(void *target, const String& val)
    {
        static_cast<GLSLESShader*>(target)->setOptimiserEnabled(StringConverter::parseBool(val));
    }
#endif

    void GLSLESShader::attachToProgramObject( const GLuint programObject )
    {
        OGRE_CHECK_GL_ERROR(glAttachShader(programObject, mGLShaderHandle));
    }

    void GLSLESShader::detachFromProgramObject( const GLuint programObject )
    {
        OGRE_CHECK_GL_ERROR(glDetachShader(programObject, mGLShaderHandle));
    }

    const String& GLSLESShader::getLanguage(void) const
    {
        static const String language = "glsles";

        return language;
    }

    Ogre::GpuProgramParametersSharedPtr GLSLESShader::createParameters( void )
    {
        GpuProgramParametersSharedPtr params = HighLevelGpuProgram::createParameters();
        return params;
    }

    void GLSLESShader::checkAndFixInvalidDefaultPrecisionError(String &message)
    {
        String precisionQualifierErrorString = ": 'Default Precision Qualifier' : invalid type Type for default precision qualifier can be only float or int";
        vector< String >::type linesOfSource = StringUtil::split(mSource, "\n");
        if( message.find(precisionQualifierErrorString) != String::npos )
        {
            LogManager::getSingleton().logMessage("Fixing invalid type Type for default precision qualifier by deleting bad lines the re-compiling", LML_CRITICAL);

            // remove relevant lines from source
            vector< String >::type errors = StringUtil::split(message, "\n");

            // going from the end so when we delete a line the numbers of the lines before will not change
            for(int i = (int)errors.size() - 1 ; i != -1 ; i--)
            {
                String & curError = errors[i];
                size_t foundPos = curError.find(precisionQualifierErrorString);
                if(foundPos != String::npos)
                {
                    String lineNumber = curError.substr(0, foundPos);
                    size_t posOfStartOfNumber = lineNumber.find_last_of(':');
                    if (posOfStartOfNumber != String::npos)
                    {
                        lineNumber = lineNumber.substr(posOfStartOfNumber + 1, lineNumber.size() - (posOfStartOfNumber + 1));
                        if (StringConverter::isNumber(lineNumber))
                        {
                            int iLineNumber = StringConverter::parseInt(lineNumber);
                            linesOfSource.erase(linesOfSource.begin() + iLineNumber - 1);
                        }
                    }
                }
            }   
            // rebuild source
            StringStream newSource; 
            for(size_t i = 0; i < linesOfSource.size()  ; i++)
            {
                newSource << linesOfSource[i] << "\n";
            }
            mSource = newSource.str();

            const char *source = mSource.c_str();
            OGRE_CHECK_GL_ERROR(glShaderSource(mGLShaderHandle, 1, &source, NULL));

            // Check for load errors
            if (compile(true))
            {
                LogManager::getSingleton().logMessage("The removing of the lines fixed the invalid type Type for default precision qualifier error.", LML_CRITICAL);
            }
            else
            {
                LogManager::getSingleton().logMessage("The removing of the lines didn't help.", LML_CRITICAL);
            }
        }
    }

    void GLSLESShader::setUniformBlockBinding( const char *blockName, uint32 bindingSlot )
    {
        GLuint programHandle = 0;

        const RenderSystemCapabilities *caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if( caps->hasCapability( RSC_SEPARATE_SHADER_OBJECTS ) )
        {
            GLSLESProgramPipeline *activeLinkProgram =
                    GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
            programHandle = activeLinkProgram->getGLProgramHandle();
        }
        else
        {
            GLSLESLinkProgram *activeLinkProgram =
                    GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
            programHandle = activeLinkProgram->getGLProgramHandle();
        }

        GLuint blockIdx = glGetUniformBlockIndex( programHandle, blockName );
        if( blockIdx != GL_INVALID_INDEX )
        {
            OCGE( glUniformBlockBinding( programHandle, blockIdx, bindingSlot ) );
        }
#if ENABLE_GL_CHECK
        else
        {
            // Clear errors
            glGetError();
        }
#endif
    }

    GLenum GLSLESShader::getGLShaderType(GpuProgramType programType)
    {
        //TODO Convert to map, or is speed different negligible?
        switch (programType)
        {
        case GPT_VERTEX_PROGRAM:
            return GL_VERTEX_SHADER;
        case GPT_HULL_PROGRAM:
            return 0; //GL_TESS_CONTROL_SHADER;
        case GPT_DOMAIN_PROGRAM:
            return 0; //GL_TESS_EVALUATION_SHADER;
        case GPT_GEOMETRY_PROGRAM:
            return 0; //GL_GEOMETRY_SHADER;
        case GPT_FRAGMENT_PROGRAM:
            return GL_FRAGMENT_SHADER;
        case GPT_COMPUTE_PROGRAM:
            return 0; //GL_COMPUTE_SHADER;
        }

        //TODO add warning or error
        return 0;
    }

    String GLSLESShader::getShaderTypeLabel(GpuProgramType programType)
    {
        switch (programType)
        {
        case GPT_VERTEX_PROGRAM:
            return "vertex";
            break;
        case GPT_DOMAIN_PROGRAM:
            return "tessellation evaluation";
            break;
        case GPT_HULL_PROGRAM:
            return "tessellation control";
            break;
        case GPT_GEOMETRY_PROGRAM:
            return "geometry";
            break;
        case GPT_FRAGMENT_PROGRAM:
            return "fragment";
            break;
        case GPT_COMPUTE_PROGRAM:
            return "compute";
            break;
        }

        //TODO add warning or error
        return 0;
    }

    GLuint GLSLESShader::getGLProgramHandle() {
        //TODO This should be removed and the compile() function
        // should use glCreateShaderProgramv
        // for separable programs which includes creating a program.
        if (mGLProgramHandle == 0)
        {
            OGRE_CHECK_GL_ERROR(mGLProgramHandle = glCreateProgram());
            if (mGLProgramHandle == 0)
            {
                //TODO error handling
            }
        }
        return mGLProgramHandle;
    }

    void GLSLESShader::bind(void)
    {
        if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Tell the Program Pipeline Manager what pipeline is to become active.
            switch (mType)
            {
            case GPT_VERTEX_PROGRAM:
                GLSLESProgramPipelineManager::getSingleton().setActiveVertexShader(this);
                break;
            case GPT_FRAGMENT_PROGRAM:
                GLSLESProgramPipelineManager::getSingleton().setActiveFragmentShader(this);
                break;
            case GPT_GEOMETRY_PROGRAM:
            case GPT_HULL_PROGRAM:
            case GPT_DOMAIN_PROGRAM:
            case GPT_COMPUTE_PROGRAM:
            default:
                break;
            }
        }
        else
        {
            // Tell the Link Program Manager what shader is to become active.
            switch (mType)
            {
            case GPT_VERTEX_PROGRAM:
                GLSLESLinkProgramManager::getSingleton().setActiveVertexShader(this);
                break;
            case GPT_FRAGMENT_PROGRAM:
                GLSLESLinkProgramManager::getSingleton().setActiveFragmentShader(this);
                break;
            case GPT_GEOMETRY_PROGRAM:
            case GPT_HULL_PROGRAM:
            case GPT_DOMAIN_PROGRAM:
            case GPT_COMPUTE_PROGRAM:
            default:
                break;
            }
        }
    }

    void GLSLESShader::unbind(void)
    {
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Tell the Program Pipeline Manager what pipeline is to become inactive.
            if (mType == GPT_VERTEX_PROGRAM)
            {
                GLSLESProgramPipelineManager::getSingleton().setActiveVertexShader(NULL);
            }
            else if (mType == GPT_FRAGMENT_PROGRAM)
            {
                GLSLESProgramPipelineManager::getSingleton().setActiveFragmentShader(NULL);
            }
        }
        else
        {
            // Tell the Link Program Manager what shader is to become inactive.
            if (mType == GPT_VERTEX_PROGRAM)
            {
                GLSLESLinkProgramManager::getSingleton().setActiveVertexShader(NULL);
            }
            else if (mType == GPT_FRAGMENT_PROGRAM)
            {
                GLSLESLinkProgramManager::getSingleton().setActiveFragmentShader(NULL);
            }
        }
    }

    void GLSLESShader::unbindAll(void)
    {
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            GLSLESProgramPipelineManager &glslManager = GLSLESProgramPipelineManager::getSingleton();
            glslManager.setActiveVertexShader(NULL);
            glslManager.setActiveFragmentShader(NULL);
        }
        else
        {
            GLSLESLinkProgramManager &glslManager = GLSLESLinkProgramManager::getSingleton();
            glslManager.setActiveVertexShader(NULL);
            glslManager.setActiveFragmentShader(NULL);
        }
    }

    void GLSLESShader::bindParameters(GpuProgramParametersSharedPtr params, uint16 mask)
    {
        // Link can throw exceptions, ignore them at this point.
        try
        {
            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                // Activate the program pipeline object.
                GLSLESProgramPipeline* separableProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
                // Pass on parameters from params to program object uniforms.
                separableProgram->updateUniforms(params, mask, mType);
                //separableProgram->updateAtomicCounters(params, mask, mType);
            }
            else
            {
                // Activate the link program object.
                GLSLESLinkProgram* monolithicProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
                // Pass on parameters from params to program object uniforms.
                monolithicProgram->updateUniforms(params, mask, mType);
                //TODO add atomic counter support
                //monolithicProgram->updateAtomicCounters(params, mask, mType);
            }
        }
        catch (Exception&) {}
    }


    void GLSLESShader::bindPassIterationParameters(GpuProgramParametersSharedPtr params)
    {
        if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            // Activate the program pipeline object.
            GLSLESProgramPipeline* separableProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
            // Pass on parameters from params to program object uniforms.
            separableProgram->updatePassIterationUniforms(params);
        }
        else
        {
            // Activate the link program object.
            GLSLESLinkProgram* monolithicProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
            // Pass on parameters from params to program object uniforms.
            monolithicProgram->updatePassIterationUniforms(params);
        }
    }


    void GLSLESShader::bindSharedParameters(GpuProgramParametersSharedPtr params, uint16 mask)
    {
        // Link can throw exceptions, ignore them at this point.
        try
        {
            if (Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
            {
                // Activate the program pipeline object.
                GLSLESProgramPipeline* separableProgram = GLSLESProgramPipelineManager::getSingleton().getActiveProgramPipeline();
                // Pass on parameters from params to program object uniforms.
                separableProgram->updateUniformBlocks(params, mask, mType);
                // separableProgram->updateShaderStorageBlock(params, mask, mType);
            }
            else
            {
                // Activate the link program object.
                GLSLESLinkProgram* monolithicProgram = GLSLESLinkProgramManager::getSingleton().getActiveLinkProgram();
                // Pass on parameters from params to program object uniforms.
                monolithicProgram->updateUniformBlocks(params, mask, mType);
            }
        }
        catch (Exception&) {}
    }
}
