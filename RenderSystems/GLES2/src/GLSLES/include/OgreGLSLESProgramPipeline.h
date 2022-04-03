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
#ifndef __GLSLESProgramPipeline_H__
#define __GLSLESProgramPipeline_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreGLSLESProgramCommon.h"

namespace Ogre
{
    /** Specialisation of GLSLESProgramCommon to provide support for separable
        programs via the Open GL program pipeline.

        Separable programs consist of shader objects which have been
        individually linked. This allows for individual shaders in a
        program pipeline to be swapped without recompiling the program
        object, as would be necessary with monolithic programs. This
        is especially useful in the common case of only entities which
        change only a few of the shaders in the pipeline while leaving
        the rest the same.

        @remarks
        GLSL ES has no target assembler or entry point specification like
        DirectX 9 HLSL. Vertex and Fragment shaders only have one
        entry point called "main". When a shader is compiled,
        microcode is generated but can not be accessed by the
        application.

        GLSL ES also does not provide assembler low level output after
        compiling.  The GL ES Render system assumes that the Gpu program
        is a GL Gpu program so GLSLESProgramPipeline will create a
        GLSLESShader for the low level implementation. The GLES2Program
        class will create a shader and program object and compile the
        source but will not create a pipeline object.  It's up to
        GLES2GpuProgram class to request a program pipeline object to
        link the program object to.

        @note
        GLSL ES supports multiple modular shader objects that can be
        attached to one program object to form a single shader.  This
        is supported through the "attach" material script command.
        All the modules to be attached are listed on the same line as
        the attach command separated by white space.
     */
    class _OgreGLES2Export GLSLESProgramPipeline : public GLSLESProgramCommon
    {
    public:
        /// Constructor should only be used by GLSLESProgramPipelineManager
        GLSLESProgramPipeline(GLSLESShader* vertexProgram, GLSLESShader* fragmentProgram);
        virtual ~GLSLESProgramPipeline();

        /// GL Program Pipeline Handle
        GLuint getGLProgramPipelineHandle() const { return mGLProgramPipelineHandle; }

        /** Updates program pipeline object uniforms using named and
         indexed parameter data from GpuProgramParameters.
         normally called by GLSLESShader::bindParameters()
         just before rendering occurs.
         */
        void updateUniforms(GpuProgramParametersSharedPtr params,
                            uint16 mask, GpuProgramType fromProgType);
        /** Updates program object uniform blocks using shared
         parameter data from GpuProgramParameters. Normally called
         by GLSLESShader::bindParameters() just before
         rendering occurs.
         */
        void updateUniformBlocks(GpuProgramParametersSharedPtr params,
                                 uint16 mask, GpuProgramType fromProgType);
        /** Updates program pipeline object uniforms using data from
         pass iteration GpuProgramParameters. Normally called by
         GLSLESShader::bindMultiPassParameters() just
         before multi pass rendering occurs.
         */
        void updatePassIterationUniforms(GpuProgramParametersSharedPtr params);

        /** Makes a program pipeline object active by making sure it
        is linked and then putting it in use.
         */
        void activate();

        /** Get the index of a non-standard attribute bound in the
         linked code.
         */
        GLint getAttributeIndex(VertexElementSemantic semantic, uint index);

    protected:
        /// GL handle for pipeline object.
        GLuint mGLProgramPipelineHandle;

        /// Compiles and links the separate programs.
        void compileAndLink();
        void loadIndividualProgram(GLSLESShader *program);
        /// Put a program pipeline in use
        void _useProgram();
        /// Build uniform references from active named uniforms
        void buildGLUniformReferences();
    };
}

#endif
