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

#include "OgreGLSLESProgramPipeline.h"
#include "OgreStringConverter.h"
#include "OgreGLSLESShader.h"
#include "OgreGLSLESProgramPipelineManager.h"
#include "OgreGpuProgramManager.h"
#include "OgreGLES2RenderSystem.h"
#include "OgreGLES2Util.h"
#include "OgreRoot.h"

#include <sstream>

namespace Ogre
{
    GLSLESProgramPipeline::GLSLESProgramPipeline(GLSLESShader* vertexShader, GLSLESShader* fragmentShader)
        : GLSLESProgramCommon(vertexShader, fragmentShader)
    {
    }

    GLSLESProgramPipeline::~GLSLESProgramPipeline()
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        OGRE_CHECK_GL_ERROR(glDeleteProgramPipelinesEXT(1, &mGLProgramPipelineHandle));
#endif
    }

    void GLSLESProgramPipeline::compileAndLink()
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        // Ensure no monolithic programs are in use.
        OGRE_CHECK_GL_ERROR(glUseProgram(0));

        OGRE_CHECK_GL_ERROR(glGenProgramPipelinesEXT(1, &mGLProgramPipelineHandle));
        //OGRE_CHECK_GL_ERROR(glBindProgramPipelineEXT(mGLProgramPipelineHandle));

        loadIndividualProgram(mVertexShader);
        loadIndividualProgram(mFragmentShader);

        if(mLinked)
        {
            if(mVertexShader && mVertexShader->isLinked())
            {
                OGRE_CHECK_GL_ERROR(glUseProgramStagesEXT(mGLProgramPipelineHandle, GL_VERTEX_SHADER_BIT_EXT, mVertexShader->getGLProgramHandle()));
            }
            if(mFragmentShader && mFragmentShader->isLinked())
            {
                OGRE_CHECK_GL_ERROR(glUseProgramStagesEXT(mGLProgramPipelineHandle, GL_FRAGMENT_SHADER_BIT_EXT, mFragmentShader->getGLProgramHandle()));
            }

            // Validate pipeline
            OGRE_CHECK_GL_ERROR(glValidateProgramPipelineEXT(mGLProgramPipelineHandle));
            logObjectInfo( getCombinedName() + String("GLSL program pipeline validation result : "), mGLProgramPipelineHandle );

            if( mVertexShader && mVertexShader->isLinked() )
                setupBaseInstance( mGLProgramPipelineHandle );

#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
            GLES2RenderSystem* rs = getGLES2RenderSystem();
            if(mVertexShader && mFragmentShader && rs->checkExtension("GL_EXT_debug_label"))
            {
                glLabelObjectEXT(GL_PROGRAM_PIPELINE_OBJECT_EXT, mGLProgramPipelineHandle, 0,
                             (mVertexShader->getName() + "/" + mFragmentShader->getName()).c_str());
            }
#endif
        }
#endif
    }

    void GLSLESProgramPipeline::loadIndividualProgram(GLSLESShader *program)
    {
        if(program && !program->isLinked())
        {
            GLint linkStatus = 0;

            String programSource = program->getSource();

            GLuint programHandle = program->getGLProgramHandle();

            OGRE_CHECK_GL_ERROR(glProgramParameteriEXT(programHandle, GL_PROGRAM_SEPARABLE_EXT, GL_TRUE));
            //if (GpuProgramManager::getSingleton().getSaveMicrocodesToCache())
            OGRE_CHECK_GL_ERROR(glProgramParameteri(programHandle, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE));

            // Use precompiled program if possible.
            bool microcodeAvailableInCache = GpuProgramManager::getSingleton().isMicrocodeAvailableInCache(programSource);
            if (microcodeAvailableInCache)
            {
                GpuProgramManager::Microcode cacheMicrocode =
                    GpuProgramManager::getSingleton().getMicrocodeFromCache(programSource);
                cacheMicrocode->seek(0);

                GLenum binaryFormat = 0;
                cacheMicrocode->read(&binaryFormat, sizeof(GLenum));

                GLint binaryLength = cacheMicrocode->size() - sizeof(GLenum);

                OGRE_CHECK_GL_ERROR(glProgramBinary(programHandle,
                                                    binaryFormat,
                                                    cacheMicrocode->getPtr() + sizeof(GLenum),
                                                    binaryLength));

                OGRE_CHECK_GL_ERROR(glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus));
                if (!linkStatus)
                    logObjectInfo("Could not use cached binary " + program->getName(), programHandle);
            }

            // Compilation needed if precompiled program is
            // unavailable or failed to link.
            if (!linkStatus)
            {
                try
                {
                    program->compile(true);
                }
                catch (Exception& e)
                {
                    LogManager::getSingleton().stream() << e.getDescription();
                    mTriedToLinkAndFailed = true;
                    return;
                }

                if( program->getType() == GPT_VERTEX_PROGRAM )
                    bindFixedAttributes( programHandle );

                program->attachToProgramObject(programHandle);
                OGRE_CHECK_GL_ERROR(glLinkProgram(programHandle));
                OGRE_CHECK_GL_ERROR(glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus));

                // Binary cache needs an update.
                microcodeAvailableInCache = false;
            }

            program->setLinked(linkStatus);
            mLinked = linkStatus;
            
            mTriedToLinkAndFailed = !linkStatus;
            
            logObjectInfo( getCombinedName() + String("GLSL program result : "), programHandle );

            if (program->getType() == GPT_VERTEX_PROGRAM)
                setSkeletalAnimationIncluded(program->isSkeletalAnimationIncluded());

            // Add the microcode to the cache.
            if (!microcodeAvailableInCache && mLinked &&
                GpuProgramManager::getSingleton().getSaveMicrocodesToCache() )
            {
                // Get buffer size.
                GLint binaryLength = 0;

                OGRE_CHECK_GL_ERROR(glGetProgramiv(programHandle, GL_PROGRAM_BINARY_LENGTH, &binaryLength));

                // Create microcode.
                GpuProgramManager::Microcode newMicrocode =
                    GpuProgramManager::getSingleton().createMicrocode((unsigned long)binaryLength + sizeof(GLenum));

                // Get binary.
                OGRE_CHECK_GL_ERROR(glGetProgramBinary(programHandle, binaryLength, NULL, (GLenum *)newMicrocode->getPtr(), newMicrocode->getPtr() + sizeof(GLenum)));

                GpuProgramManager::getSingleton().addMicrocodeToCache(programSource, newMicrocode);
            }
        }
    }

    // void GLSLESProgramPipeline::_useProgram(void)
    // {
    //     if (mLinked)
    //     {
    // #if OGRE_PLATFORM != OGRE_PLATFORM_NACL
    //         OGRE_CHECK_GL_ERROR(glBindProgramPipelineEXT(mGLProgramPipelineHandle));
    // #endif
    //     }
    // }

    GLint GLSLESProgramPipeline::getAttributeIndex(VertexElementSemantic semantic, uint index)
    {
        GLint res = mCustomAttributesIndexes[semantic-1][index];
        if (res == NULL_CUSTOM_ATTRIBUTES_INDEX)
        {
            GLuint handle = mVertexShader->getGLProgramHandle();
            const char * attString = getAttributeSemanticString(semantic);
            GLint attrib;
            OGRE_CHECK_GL_ERROR(attrib = glGetAttribLocation(handle, attString));

            // Sadly position is a special case.
            if (attrib == NOT_FOUND_CUSTOM_ATTRIBUTES_INDEX && semantic == VES_POSITION)
            {
                OGRE_CHECK_GL_ERROR(attrib = glGetAttribLocation(handle, "position"));
            }
            
            // For UV and other cases the index is a part of the name.
            if (attrib == NOT_FOUND_CUSTOM_ATTRIBUTES_INDEX)
            {
                String attStringWithSemantic = String(attString) + StringConverter::toString(index);
                OGRE_CHECK_GL_ERROR(attrib = glGetAttribLocation(handle, attStringWithSemantic.c_str()));
            }
            
            // Update mCustomAttributesIndexes with the index we found (or didn't find) 
            mCustomAttributesIndexes[semantic-1][index] = attrib;
            res = attrib;
        }
        
        return res;
    }

    void GLSLESProgramPipeline::activate(void)
    {
        if (!mLinked && !mTriedToLinkAndFailed)
        {
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
            // Check CmdParams for each shader type to see if we should optimise
            if(mVertexShader)
            {
                String paramStr = mVertexShader->getParameter("use_optimiser");
                if((paramStr == "true") || paramStr.empty())
                {
                    GLSLESProgramPipelineManager::getSingleton().optimiseShaderSource(mVertexShader);
                }
            }

            if(mFragmentShader)
            {
                String paramStr = mFragmentShader->getParameter("use_optimiser");
                if((paramStr == "true") || paramStr.empty())
                {
                    GLSLESProgramPipelineManager::getSingleton().optimiseShaderSource(mFragmentShader);
                }
            }
#endif
            compileAndLink();

            extractLayoutQualifiers();

            buildGLUniformReferences();
        }

        // _useProgram();
        
#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        if(mLinked)
        {
            OGRE_CHECK_GL_ERROR(glBindProgramPipelineEXT(mGLProgramPipelineHandle));
        }
#endif
    }

    void GLSLESProgramPipeline::buildGLUniformReferences(void)
    {
        if (!mUniformRefsBuilt)
        {
            const GpuConstantDefinitionMap* vertParams = 0;
            const GpuConstantDefinitionMap* fragParams = 0;
            if (mVertexShader)
            {
                vertParams = &(mVertexShader->getConstantDefinitions().map);
                GLSLESProgramPipelineManager::getSingleton().extractUniformsFromProgram(mVertexShader->getGLProgramHandle(),
                                                                                        vertParams, NULL,
                                                                                        mGLUniformReferences, mGLUniformBufferReferences);
            }
            if (mFragmentShader)
            {
                fragParams = &(mFragmentShader->getConstantDefinitions().map);
                GLSLESProgramPipelineManager::getSingleton().extractUniformsFromProgram(mFragmentShader->getGLProgramHandle(),
                                                                                        NULL, fragParams,
                                                                                        mGLUniformReferences, mGLUniformBufferReferences);
            }

            mUniformRefsBuilt = true;
        }
    }

    void GLSLESProgramPipeline::updateUniforms(GpuProgramParametersSharedPtr params,
                                           uint16 mask, GpuProgramType fromProgType)
    {
        // Iterate through uniform reference list and update uniform values
        GLUniformReferenceIterator currentUniform = mGLUniformReferences.begin();
        GLUniformReferenceIterator endUniform = mGLUniformReferences.end();

#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
        // determine if we need to transpose matrices when binding
        int transpose = GL_TRUE;

        GLuint progID = 0;
        if(fromProgType == GPT_VERTEX_PROGRAM)
        {
            progID = mVertexShader->getGLProgramHandle();
        }
        else if(fromProgType == GPT_FRAGMENT_PROGRAM)
        {
            progID = mFragmentShader->getGLProgramHandle();
        }

        for (;currentUniform != endUniform; ++currentUniform)
        {
            // Only pull values from buffer it's supposed to be in (vertex or fragment)
            // This method will be called once per shader stage.
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
                        OGRE_CHECK_GL_ERROR(glProgramUniform1fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT2:
                        OGRE_CHECK_GL_ERROR(glProgramUniform2fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT3:
                        OGRE_CHECK_GL_ERROR(glProgramUniform3fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_FLOAT4:
                        OGRE_CHECK_GL_ERROR(glProgramUniform4fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_2X2:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                         transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X3:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                         transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X4:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                         transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_INT1:
                        OGRE_CHECK_GL_ERROR(glProgramUniform1ivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT2:
                        OGRE_CHECK_GL_ERROR(glProgramUniform2ivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT3:
                        OGRE_CHECK_GL_ERROR(glProgramUniform3ivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getIntPointer(def->physicalIndex)));
                        break;
                    case GCT_INT4:
                        OGRE_CHECK_GL_ERROR(glProgramUniform4ivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getIntPointer(def->physicalIndex)));
                        break;
#if OGRE_NO_GLES3_SUPPORT == 0
                    case GCT_MATRIX_2X3:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x3fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_2X4:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix2x4fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X2:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x2fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_3X4:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix3x4fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X2:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x2fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
                    case GCT_MATRIX_4X3:
                        OGRE_CHECK_GL_ERROR(glProgramUniformMatrix4x3fvEXT(progID, currentUniform->mLocation, glArraySize,
                                                                        transpose, params->getFloatPointer(def->physicalIndex)));
                        break;
#else
                    case GCT_MATRIX_2X3:
                    case GCT_MATRIX_2X4:
                    case GCT_MATRIX_3X2:
                    case GCT_MATRIX_3X4:
                    case GCT_MATRIX_4X2:
                    case GCT_MATRIX_4X3:
#endif
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
                        break;

#if 0 // TODO: update glew
                    case GCT_UINT1:
                    case GCT_BOOL1:
                        OGRE_CHECK_GL_ERROR(glProgramUniform1uivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                 params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT2:
                    case GCT_BOOL2:
                        OGRE_CHECK_GL_ERROR(glProgramUniform2uivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                 params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT3:
                    case GCT_BOOL3:
                        OGRE_CHECK_GL_ERROR(glProgramUniform3uivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                 params->getUnsignedIntPointer(def->physicalIndex)));
                        break;
                    case GCT_UINT4:
                    case GCT_BOOL4:
                        OGRE_CHECK_GL_ERROR(glProgramUniform4uivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                 params->getUnsignedIntPointer(def->physicalIndex)));
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
                        OGRE_CHECK_GL_ERROR(glProgramUniform1ivEXT(progID, currentUniform->mLocation, glArraySize,
                                                                   params->getIntPointer(def->physicalIndex)));
                        break;

                    default:
                        break;
                        
                    } // End switch
                } // Variability & mask
            } // fromProgType == currentUniform->mSourceProgType
            
        } // End for
#endif
    }

    void GLSLESProgramPipeline::updateUniformBlocks(GpuProgramParametersSharedPtr params,
                                                  uint16 mask, GpuProgramType fromProgType)
    {
#if 0 //OGRE_NO_GLES3_SUPPORT == 0
        //TODO Support uniform block arrays - need to figure how to do this via material.

        // Iterate through the list of uniform blocks and update them as needed.
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

    void GLSLESProgramPipeline::updatePassIterationUniforms(GpuProgramParametersSharedPtr params)
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_NACL
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
                    GLuint progID = 0;
                    if (mVertexShader && currentUniform->mSourceProgType == GPT_VERTEX_PROGRAM)
                    {
                        progID = mVertexShader->getGLProgramHandle();
                    }
                    
                    if (mFragmentShader && currentUniform->mSourceProgType == GPT_FRAGMENT_PROGRAM)
                    {
                        progID = mFragmentShader->getGLProgramHandle();
                    }

                    OGRE_CHECK_GL_ERROR(glProgramUniform1fvEXT(progID, currentUniform->mLocation, 1, params->getFloatPointer(index)));

                    // There will only be one multipass entry
                    return;
                }
            }
        }
#endif
    }
}
