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
#ifndef __GLSLESProgramCommon_H__
#define __GLSLESProgramCommon_H__

#include "OgreGLES2Prerequisites.h"
#include "OgreGpuProgram.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreHardwareUniformBuffer.h"

namespace Ogre {
    /// Structure used to keep track of named uniforms in the linked program object
    struct GLUniformReference
    {
        /// GL location handle
        GLint mLocation;
        /// Which type of program params will this value come from?
        GpuProgramType mSourceProgType;
        /// The constant definition it relates to
        const GpuConstantDefinition* mConstantDef;
    };

    typedef vector<GLUniformReference>::type GLUniformReferenceList;
    typedef GLUniformReferenceList::iterator GLUniformReferenceIterator;
    typedef vector<v1::HardwareUniformBufferSharedPtr>::type GLUniformBufferList;

    typedef GLUniformBufferList::iterator GLUniformBufferIterator;

    /** C++ encapsulation of GLSL ES Program Object
     
     */

    class _OgreGLES2Export GLSLESProgramCommon
    {
    public:
        /// Constructor should only be used by GLSLESLinkProgramManager and GLSLESProgramPipelineManager
        GLSLESProgramCommon(GLSLESShader* vertexProgram, GLSLESShader* fragmentProgram);
        virtual ~GLSLESProgramCommon();

        /** Makes a program object active by making sure it is linked and then putting it in use.
         */
        virtual void activate() = 0;

        /** Updates program object uniforms using data from GpuProgramParameters.
         Normally called by GLSLESShader::bindParameters() just before rendering occurs.
         */
        virtual void updateUniforms(GpuProgramParametersSharedPtr params, uint16 mask, GpuProgramType fromProgType) = 0;
        /** Updates program object uniform blocks using data from GpuProgramParameters.
         Normally called by GLSLESShader::bindParameters() just before rendering occurs.
         */
        virtual void updateUniformBlocks(GpuProgramParametersSharedPtr params, uint16 mask, GpuProgramType fromProgType) = 0;
        /** Updates program object uniforms using data from pass iteration GpuProgramParameters.
         Normally called by GLSLESShader::bindMultiPassParameters() just before multi pass rendering occurs.
         */
        virtual void updatePassIterationUniforms(GpuProgramParametersSharedPtr params) = 0;
        /// Finds layout qualifiers in the shader source and sets attribute indices appropriately
        virtual void extractLayoutQualifiers();
        /// Get the GL Handle for the program object
        GLuint getGLProgramHandle() const { return mGLProgramHandle; }
        /** Sets whether the linked program includes the required instructions
         to perform skeletal animation.
         @remarks
         If this is set to true, OGRE will not blend the geometry according to
         skeletal animation, it will expect the vertex program to do it.
         */
        void setSkeletalAnimationIncluded(bool included) { mSkeletalAnimation = included; }

        /** Returns whether the linked program includes the required instructions
         to perform skeletal animation.
         @remarks
         If this returns true, OGRE will not blend the geometry according to
         skeletal animation, it will expect the vertex program to do it.
         */
        bool isSkeletalAnimationIncluded() const { return mSkeletalAnimation; }

        /// Get the index of a non-standard attribute bound in the linked code
        virtual GLint getAttributeIndex(VertexElementSemantic semantic, uint index);
        /// Is a non-standard attribute bound in the linked code?
        bool isAttributeValid(VertexElementSemantic semantic, uint index);

        GLSLESShader* getVertexShader() const { return mVertexShader; }
        GLSLESShader* getFragmentShader() const { return mFragmentShader; }

        GLint getBaseInstanceLocation() const { return mBaseInstanceLocation; }
    protected:
        GLint mBaseInstanceLocation;
        /// Container of uniform references that are active in the program object
        GLUniformReferenceList mGLUniformReferences;
        /// Container of uniform buffer references that are active in the program object
        GLUniformBufferList mGLUniformBufferReferences;

        /// Linked vertex shader.
        GLSLESShader* mVertexShader;
        /// Linked fragment shader.
        GLSLESShader* mFragmentShader;
        /// Flag to indicate that uniform references have already been built
        bool mUniformRefsBuilt;
        /// GL handle for the program object
        GLuint mGLProgramHandle;
        /// Flag indicating that the program or pipeline object has been successfully linked
        GLint mLinked;
        /// Flag indicating that the program or pipeline object has tried to link and failed
        bool mTriedToLinkAndFailed;
        /// Flag indicating skeletal animation is being performed
        bool mSkeletalAnimation;

        /// Build uniform references from active named uniforms
        void buildGLUniformReferences();
        typedef set<GLuint>::type AttributeSet;

        /// An array to hold the attributes indexes
        GLint mCustomAttributesIndexes[VES_COUNT][OGRE_MAX_TEXTURE_COORD_SETS];
        /// A value to define the case we didn't look for the attributes since the contractor
#define NULL_CUSTOM_ATTRIBUTES_INDEX -2
        /// A value to define the attribute has not been found (this is also the result when glGetAttribLocation fails)
#define NOT_FOUND_CUSTOM_ATTRIBUTES_INDEX -1

        Ogre::String getCombinedName();
        Ogre::String getCombinedSource() const;
        /// Get the the binary data of a program from the microcode cache
        void getMicrocodeFromCache();
        /// Compiles and links the vertex and fragment programs
        virtual void compileAndLink() = 0;
        // /// Put a program in use
        // virtual void _useProgram() = 0;

        typedef map<String, VertexElementSemantic>::type SemanticToStringMap;
        SemanticToStringMap mSemanticTypeMap;

        VertexElementSemantic getAttributeSemanticEnum(String type);
        const char * getAttributeSemanticString(VertexElementSemantic semantic);

        void bindFixedAttributes( GLuint programName );
        void setupBaseInstance( GLuint programName );
    };
}

#endif // __GLSLESProgramCommon_H__
