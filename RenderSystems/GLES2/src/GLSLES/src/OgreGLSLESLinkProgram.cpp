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

#include "OgreGLSLESLinkProgram.h"
#include "OgreGLSLESShader.h"
#include "OgreGLSLESLinkProgramManager.h"
#include "OgreGLES2HardwareUniformBuffer.h"
#include "OgreLogManager.h"
#include "OgreGpuProgramManager.h"
#include "OgreStringConverter.h"
#include "OgreRoot.h"
#include "OgreGLES2Util.h"
#include "OgreGLES2RenderSystem.h"

#include <sstream>

namespace Ogre {

    GLSLESLinkProgram::GLSLESLinkProgram(GLSLESShader* vertexProgram, GLSLESShader* fragmentProgram)
        : GLSLESProgramCommon(vertexProgram, fragmentProgram)
    {
        if ((!mVertexShader || !mFragmentShader))
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                        "Attempted to create a shader program without both a vertex and fragment program.",
                        "GLSLESLinkProgram::GLSLESLinkProgram");
        }
    }

    GLSLESLinkProgram::~GLSLESLinkProgram()
    {
        OGRE_CHECK_GL_ERROR(glDeleteProgram(mGLProgramHandle));
        mGLProgramHandle = 0;
    }

    void GLSLESLinkProgram::_useProgram()
    {
        if (mLinked)
        {
            OGRE_CHECK_GL_ERROR(glUseProgram( mGLProgramHandle ));
        }
    }

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
    void GLSLESLinkProgram::notifyOnContextLost()
    {
        OGRE_CHECK_GL_ERROR(glDeleteProgram(mGLProgramHandle));
        mGLProgramHandle = 0;
        mLinked = false;
        mTriedToLinkAndFailed = false;
        mUniformRefsBuilt = false;
        mUniformCache->clearCache();
    }
    
    void GLSLESLinkProgram::notifyOnContextReset()
    {
        activate();
    }
#endif

    void GLSLESLinkProgram::activate()
    {
        if (!mLinked && !mTriedToLinkAndFailed)
        {
            OGRE_CHECK_GL_ERROR(mGLProgramHandle = glCreateProgram());

            if ( GpuProgramManager::getSingleton().canGetCompiledShaderBuffer() &&
                GpuProgramManager::getSingleton().isMicrocodeAvailableInCache(getCombinedSource()) )
            {
                getMicrocodeFromCache();
            }
            else
            {
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
                // Check CmdParams for each shader type to see if we should optimize
                if(mVertexShader)
                {
                    String paramStr = mVertexShader->getParameter("use_optimiser");
                    if((paramStr == "true") || paramStr.empty())
                    {
                        GLSLESLinkProgramManager::getSingleton().optimiseShaderSource(mVertexShader);
                    }
                }

                if(mFragmentShader)
                {
                    String paramStr = mFragmentShader->getParameter("use_optimiser");
                    if((paramStr == "true") || paramStr.empty())
                    {
                        GLSLESLinkProgramManager::getSingleton().optimiseShaderSource(mFragmentShader);
                    }
                }
#endif
                compileAndLink();

#if !OGRE_NO_GLES2_GLSL_OPTIMISER
                // Try it again when we used the optimised versions
                if(mTriedToLinkAndFailed && 
                    mVertexShader->getOptimiserEnabled() &&
                    mFragmentShader->getOptimiserEnabled())
                {
                    LogManager::getSingleton().stream() << "Try not optimised shader.";
                    mTriedToLinkAndFailed = false;
                    mVertexShader->setOptimiserEnabled(false);
                    mFragmentShader->setOptimiserEnabled(false);
                    compileAndLink();
                }
#endif
            }

            extractLayoutQualifiers();
            buildGLUniformReferences();
        }

        _useProgram();
    }

    void GLSLESLinkProgram::compileAndLink()
    {
        // Compile and attach Vertex Program
        if (mVertexShader)
        {
            try
            {
                mVertexShader->compile(true);
            }
            catch (Exception& e)
            {
                LogManager::getSingleton().stream() << e.getDescription();
                mTriedToLinkAndFailed = true;
                return;
            }

            mVertexShader->attachToProgramObject(mGLProgramHandle);
            setSkeletalAnimationIncluded(mVertexShader->isSkeletalAnimationIncluded());
        }

        // Compile and attach Fragment Program
        if (mFragmentShader)
        {
            try
            {
                mFragmentShader->compile(true);
            }
            catch (Exception& e)
            {
                LogManager::getSingleton().stream() << e.getDescription();
                mTriedToLinkAndFailed = true;
                return;
            }
            mFragmentShader->attachToProgramObject(mGLProgramHandle);
        }

        bindFixedAttributes(mGLProgramHandle);

        // the link
        OGRE_CHECK_GL_ERROR(glLinkProgram( mGLProgramHandle ));
        OGRE_CHECK_GL_ERROR(glGetProgramiv( mGLProgramHandle, GL_LINK_STATUS, &mLinked ));

        mTriedToLinkAndFailed = !mLinked;

        logObjectInfo( getCombinedName() + String("GLSL link result : "), mGLProgramHandle );

#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_SEPARATE_SHADER_OBJECTS))
        {
            if(glIsProgramPipelineEXT(mGLProgramHandle))
                glValidateProgramPipelineEXT(mGLProgramHandle);
        }
        else if(glIsProgram(mGLProgramHandle))
#else
        if(glIsProgram(mGLProgramHandle))
#endif
        {
            OGRE_CHECK_GL_ERROR(glValidateProgram(mGLProgramHandle));
        }

        logObjectInfo( getCombinedName() + String(" GLSL validation result : "), mGLProgramHandle );

        if(mLinked)
        {
            setupBaseInstance( mGLProgramHandle );
            if ( GpuProgramManager::getSingleton().getSaveMicrocodesToCache() )
            {
                // Add to the microcode to the cache
                String source;
                source = getCombinedSource();

                // Get buffer size
                GLint binaryLength = 0;
                GLES2RenderSystem* rs = getGLES2RenderSystem();
                if(rs->hasMinGLVersion(3, 0) || rs->checkExtension("GL_OES_get_program_binary"))
                    OGRE_CHECK_GL_ERROR(glGetProgramiv(mGLProgramHandle, GL_PROGRAM_BINARY_LENGTH_OES, &binaryLength));

                // Create microcode
                GpuProgramManager::Microcode newMicrocode = 
                    GpuProgramManager::getSingleton().createMicrocode(static_cast<uint32>(binaryLength + sizeof(GLenum)));

                // Get binary
                if(rs->hasMinGLVersion(3, 0) || rs->checkExtension("GL_OES_get_program_binary"))
                    OGRE_CHECK_GL_ERROR(glGetProgramBinaryOES(mGLProgramHandle, binaryLength, NULL, (GLenum *)newMicrocode->getPtr(),
                                                          newMicrocode->getPtr() + sizeof(GLenum)));

                // Add to the microcode to the cache
                GpuProgramManager::getSingleton().addMicrocodeToCache(source, newMicrocode);
            }
        }
    }

    void GLSLESLinkProgram::buildGLUniformReferences()
    {
        if (!mUniformRefsBuilt)
        {
            const GpuConstantDefinitionMap* vertParams = 0;
            const GpuConstantDefinitionMap* fragParams = 0;
            if (mVertexShader)
            {
                vertParams = &(mVertexShader->getConstantDefinitions().map);
            }
            if (mFragmentShader)
            {
                fragParams = &(mFragmentShader->getConstantDefinitions().map);
            }

            // Do we know how many shared params there are yet? Or if there are any blocks defined?
            GLSLESLinkProgramManager::getSingleton().extractUniformsFromProgram(
                mGLProgramHandle, vertParams, fragParams, mGLUniformReferences, mGLUniformBufferReferences);

            mUniformRefsBuilt = true;
        }
    }

    void GLSLESLinkProgram::updateUniforms(GpuProgramParametersSharedPtr params,
        uint16 mask, GpuProgramType fromProgType)
    {
        // Iterate through uniform reference list and update uniform values
        GLUniformReferenceIterator currentUniform = mGLUniformReferences.begin();
        GLUniformReferenceIterator endUniform = mGLUniformReferences.end();

        // determine if we need to transpose matrices when binding
        int transpose = GL_TRUE;

        for (;currentUniform != endUniform; ++currentUniform)
        {
            // Only pull values from buffer it's supposed to be in (vertex or fragment)
            // This method will be called twice, once for vertex program params, 
            // and once for fragment program params.
            if (fromProgType == currentUniform->mSourceProgType)
            {
                const GpuConstantDefinition* def = currentUniform->mConstantDef;
                if (def->variability & mask)
                {
                    GLsizei glArraySize = (GLsizei)def->arraySize;

                    // Get the index in the parameter real list
                    switch (def->constType)
                    {
                    case GCT_FLOAT1:
                        OGRE_CHECK_GL_ERROR(glUniform1fv(currentUniform->mLocation, glArraySize, 
                                                         params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT2:
                        OGRE_CHECK_GL_ERROR(glUniform2fv(currentUniform->mLocation, glArraySize, 
                                                         params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT3:
                        OGRE_CHECK_GL_ERROR(glUniform3fv(currentUniform->mLocation, glArraySize, 
                                                         params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT4:
                        OGRE_CHECK_GL_ERROR(glUniform4fv(currentUniform->mLocation, glArraySize, 
                                                         params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_2X2:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix2fv(currentUniform->mLocation, glArraySize, 
                                                               transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X3:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix3fv(currentUniform->mLocation, glArraySize, 
                                                               transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X4:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix4fv(currentUniform->mLocation, glArraySize, 
                                                               transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
#if OGRE_NO_GLES3_SUPPORT == 0
                    case GCT_MATRIX_2X3:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix2x3fv(currentUniform->mLocation, glArraySize,
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_2X4:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix2x4fv(currentUniform->mLocation, glArraySize,
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X2:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix3x2fv(currentUniform->mLocation, glArraySize,
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X4:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix3x4fv(currentUniform->mLocation, glArraySize,
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X2:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix4x2fv(currentUniform->mLocation, glArraySize,
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X3:
                        OGRE_CHECK_GL_ERROR(glUniformMatrix4x3fv(currentUniform->mLocation, glArraySize, 
                                                                 transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
#else
                    case GCT_MATRIX_2X3:
                    case GCT_MATRIX_2X4:
                    case GCT_MATRIX_3X2:
                    case GCT_MATRIX_3X4:
                    case GCT_MATRIX_4X2:
                    case GCT_MATRIX_4X3:
                        break;
#endif
                    case GCT_INT1:
                        OGRE_CHECK_GL_ERROR(glUniform1iv(currentUniform->mLocation, glArraySize,
                                                         (GLint*)params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT2:
                        OGRE_CHECK_GL_ERROR(glUniform2iv(currentUniform->mLocation, glArraySize, 
                                                         (GLint*)params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT3:
                        OGRE_CHECK_GL_ERROR(glUniform3iv(currentUniform->mLocation, glArraySize, 
                                                         (GLint*)params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT4:
                        OGRE_CHECK_GL_ERROR(glUniform4iv(currentUniform->mLocation, glArraySize, 
                                                         (GLint*)params->getIntPointer(def->physicalIndex)));
                        break;
#if OGRE_NO_GLES3_SUPPORT == 0
                    case GCT_UINT1:
                    case GCT_BOOL1:
                        OGRE_CHECK_GL_ERROR(glUniform1uiv(currentUniform->mLocation, glArraySize,
                                                          (GLuint*)params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT2:
                    case GCT_BOOL2:
                        OGRE_CHECK_GL_ERROR(glUniform2uiv(currentUniform->mLocation, glArraySize,
                                                          (GLuint*)params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT3:
                    case GCT_BOOL3:
                        OGRE_CHECK_GL_ERROR(glUniform3uiv(currentUniform->mLocation, glArraySize,
                                                          (GLuint*)params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT4:
                    case GCT_BOOL4:
                        OGRE_CHECK_GL_ERROR(glUniform4uiv(currentUniform->mLocation, glArraySize,
                                                          (GLuint*)params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
#endif

                    case GCT_SAMPLER1D:
                    case GCT_SAMPLER1DSHADOW:
                    case GCT_SAMPLER2D:
                    case GCT_SAMPLER2DSHADOW:
                    case GCT_SAMPLER2DARRAY:
                    case GCT_SAMPLER3D:
                    case GCT_SAMPLERCUBE:
                    case GCT_SAMPLERRECT:
                        // Samplers handled like 1-element ints
                        OGRE_CHECK_GL_ERROR(glUniform1iv(currentUniform->mLocation, glArraySize,
                                                         (GLint*)params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UNKNOWN:
                    case GCT_SUBROUTINE:
                    case GCT_DOUBLE1:
                    case GCT_DOUBLE2:
                    case GCT_DOUBLE3:
                    case GCT_DOUBLE4:
                    case GCT_MATRIX_DOUBLE_2X2:
                    case GCT_MATRIX_DOUBLE_2X3:
                    case GCT_MATRIX_DOUBLE_2X4:
                    case GCT_MATRIX_DOUBLE_3X2:
                    case GCT_MATRIX_DOUBLE_3X3:
                    case GCT_MATRIX_DOUBLE_3X4:
                    case GCT_MATRIX_DOUBLE_4X2:
                    case GCT_MATRIX_DOUBLE_4X3:
                    case GCT_MATRIX_DOUBLE_4X4:
                    default:
                        break;

                    } // End switch
                } // Variability & mask
            } // fromProgType == currentUniform->mSourceProgType
  
        } // End for
    }

    void GLSLESLinkProgram::updateUniformBlocks(GpuProgramParametersSharedPtr params,
                                              uint16 mask, GpuProgramType fromProgType)
    {
#if OGRE_NO_GLES3_SUPPORT == 0
        // Iterate through the list of uniform buffers and update them as needed
        GLUniformBufferIterator currentBuffer = mGLUniformBufferReferences.begin();
        GLUniformBufferIterator endBuffer = mGLUniformBufferReferences.end();

        const GpuProgramParameters::GpuSharedParamUsageList& sharedParams = params->getSharedParameters();

        GpuProgramParameters::GpuSharedParamUsageList::const_iterator it, end = sharedParams.end();
        for (it = sharedParams.begin(); it != end; ++it)
        {
            for (;currentBuffer != endBuffer; ++currentBuffer)
            {
                v1::GLES2HardwareUniformBuffer* hwGlBuffer = static_cast<v1::GLES2HardwareUniformBuffer*>(currentBuffer->get());
                GpuSharedParametersPtr paramsPtr = it->getSharedParams();

                // Block name is stored in mSharedParams->mName of GpuSharedParamUsageList items
                GLint UniformTransform;
                OGRE_CHECK_GL_ERROR(UniformTransform = glGetUniformBlockIndex(mGLProgramHandle, it->getName().c_str()));
                OGRE_CHECK_GL_ERROR(glUniformBlockBinding(mGLProgramHandle, UniformTransform, hwGlBuffer->getGLBufferBinding()));

                hwGlBuffer->writeData(0, hwGlBuffer->getSizeInBytes(), &paramsPtr->getFloatConstantList().front());
            }
        }
#endif
    }

    void GLSLESLinkProgram::updatePassIterationUniforms(GpuProgramParametersSharedPtr params)
    {
        if (params->hasPassIterationNumber())
        {
            size_t index = params->getPassIterationNumberIndex();

            GLUniformReferenceIterator currentUniform = mGLUniformReferences.begin();
            GLUniformReferenceIterator endUniform = mGLUniformReferences.end();

            // Need to find the uniform that matches the multi pass entry
            for (;currentUniform != endUniform; ++currentUniform)
            {
                // Get the index in the parameter real list
                if (index == currentUniform->mConstantDef->physicalIndex)
                {
                    OGRE_CHECK_GL_ERROR(glUniform1fv(currentUniform->mLocation, 1, params->getFloatPointer(index)));
                    // There will only be one multipass entry
                    return;
                }
            }
        }
    }
} // namespace Ogre
