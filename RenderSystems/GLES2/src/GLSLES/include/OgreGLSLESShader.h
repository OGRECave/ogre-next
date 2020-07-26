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
#ifndef __GLSLESShader_H__
#define __GLSLESShader_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreHighLevelGpuProgram.h"
#include "OgreGLES2ManagedResource.h"

namespace Ogre {
    /** Specialisation of HighLevelGpuProgram to encapsulate shader
        objects obtained from compiled shaders written in the OpenGL 
        Shader Language (GLSL ES) for OpenGL ES 2.0.
    @remarks
        GLSL ES has no target assembler or entry point specification
        like DirectX 9 HLSL.  Vertex and
        Fragment shaders only have one entry point called "main".
        When a shader is compiled, microcode is generated but can not
        be accessed by the application.  GLSL ES also does not provide
        assembler low level output after compiling.
    */
    class _OgreGLES2Export GLSLESShader : public HighLevelGpuProgram MANAGED_RESOURCE
    {
    public:
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
        /// Command object for running the GLSL optimiser 
        class CmdOptimisation : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };
#endif
        /// Command object for setting macro defines
        class CmdPreprocessorDefines : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };
        
        GLSLESShader(ResourceManager* creator,
            const String& name, ResourceHandle handle,
            const String& group, bool isManual, ManualResourceLoader* loader);
        ~GLSLESShader();

        GLuint getGLShaderHandle() const { return mGLShaderHandle; }
        GLuint getGLProgramHandle();
        void attachToProgramObject( const GLuint programObject );
        void detachFromProgramObject( const GLuint programObject );
        /// Get OpenGL GLSL shader type from OGRE GPU program type.
        GLenum getGLShaderType(GpuProgramType programType);
        /// Get a string containing the name of the GLSL shader type
        /// correspondening to the OGRE GPU program type.
        String getShaderTypeLabel(GpuProgramType programType);

        /// Overridden
        bool getPassTransformStates(void) const;
        bool getPassSurfaceAndLightStates(void) const;
        bool getPassFogStates(void) const;

        /// Sets the preprocessor defines use to compile the program.
        void setPreprocessorDefines(const String& defines) { mPreprocessorDefines = defines; }
        /// Sets the preprocessor defines use to compile the program.
        const String& getPreprocessorDefines(void) const { return mPreprocessorDefines; }

#if !OGRE_NO_GLES2_GLSL_OPTIMISER
        /// Sets if the GLSL optimiser is enabled.
        void setOptimiserEnabled(bool enabled);
        /// Gets if the GLSL optimiser is enabled.
        bool getOptimiserEnabled(void) const { return mOptimiserEnabled; }
        
        /// Sets if the GLSL source has been optimised successfully
        void setIsOptimised(bool flag) { mIsOptimised = flag; }
        /// Gets if the GLSL source has been optimised successfully
        bool getIsOptimised(void) { return mIsOptimised; }

        /// Sets the optimised GLSL source 
        void setOptimisedSource(const String& src) { mOptimisedSource = src; }
        /// Gets he optimised GLSL source 
        String getOptimisedSource(void) { return mOptimisedSource; }
#endif

        /// Overridden from GpuProgram
        const String& getLanguage(void) const;
        /// Overridden from GpuProgram
        GpuProgramParametersSharedPtr createParameters(void);

        /// Compile source into shader object
        bool compile( const bool checkErrors = false);


        /// Bind the shader in OpenGL.
        void bind(void);
        /// Unbind the shader in OpenGL.
        void unbind(void);
        static void unbindAll(void);
        /// Execute the param binding functions for this shader.
        void bindParameters(GpuProgramParametersSharedPtr params, uint16 mask);
        /// Execute the pass iteration param binding functions for this shader.
        void bindPassIterationParameters(GpuProgramParametersSharedPtr params);
        /// Execute the shared param binding functions for this shader.
        void bindSharedParameters(GpuProgramParametersSharedPtr params, uint16 mask);
        
        /** Return the shader link status.
            Only used for separable programs.
        */
        GLint isLinked(void) { return mLinked; }

        /** Set the shader link status.
            Only used for separable programs.
        */
        void setLinked(GLint flag) { mLinked = flag; }

        /// Get the OGRE assigned shader ID.
        GLuint getShaderID(void) const { return mShaderID; }
        
        /// Since GLSL has no assembly, use this shader for binding.
        GpuProgram* _getBindingDelegate(void) { return this; }

    protected:
        static CmdPreprocessorDefines msCmdPreprocessorDefines;
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
        static CmdOptimisation msCmdOptimisation;
#endif

        /** Internal load implementation, must be implemented by subclasses.
        */
        void loadFromSource(void);
        /** Internal method for creating a dummy low-level program for
            this high-level program. GLSL ES does not give access to the
            low level implementation of the shader so this method
            creates an object sub-classed from GLSLESShader just to
            be compatible with GLES2RenderSystem.
        */
        void createLowLevelImpl(void);
        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl(void);
        /// Overridden from HighLevelGpuProgram
        void unloadImpl(void);

        /// Populate the passed parameters with name->index map
        void populateParameterNames(GpuProgramParametersSharedPtr params);
        /// Populate the passed parameters with name->index map, must be overridden
        void buildConstantDefinitions() const;
        /** Check the compile result for an error with default
            precision - and recompile if needed.  some glsl compilers
            return an error default precision is set to types other
            then int or float, this function test a failed compile
            result for the error, delete the needed lines from the
            source if needed then try to re-compile.
        */
        void checkAndFixInvalidDefaultPrecisionError( String &message );

        virtual void setUniformBlockBinding( const char *blockName, uint32 bindingSlot );

#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID || OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
        /** See AndroidResource. */
        virtual void notifyOnContextLost();
#endif
        
        /// OGRE assigned shader ID.
        GLuint mShaderID;

    private:
        /// GL handle for shader object.
        GLuint mGLShaderHandle;

        /// GL handle for program object the shader is bound to.
        GLuint mGLProgramHandle;

        /// Flag indicating if shader object successfully compiled.
        GLint mCompiled;
        /// Preprocessor options.
        String mPreprocessorDefines;
#if !OGRE_NO_GLES2_GLSL_OPTIMISER
        /// Flag indicating if shader has been successfully optimised.
        bool mIsOptimised;
        bool mOptimiserEnabled;
        /// The optmised source of the program (may be blank until the shader is optmisied)
        String mOptimisedSource;
#endif

        /// Keep track of the number of shaders created.
        static GLuint mShaderCount;

        /** Flag indicating that the shader has been successfully
            linked.
            Only used for separable programs. */
        GLint mLinked;
    };
}

#endif // __GLSLESShader_H__
