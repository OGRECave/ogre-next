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

#include "OgreGLSLProgramManager.h"
#include "OgreGL3PlusHardwareBufferManager.h"
#include "OgreGL3PlusUtil.h"
#include "OgreGLSLShader.h"
#include "OgreGpuProgramManager.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreString.h"
#include "OgreStringConverter.h"

#include <iostream>

namespace Ogre
{
    const size_t c_numTypeQualifiers = 8;

    const String c_typeQualifiers[c_numTypeQualifiers] = { "lowp",     "mediump",  "highp",
                                                           "restrict", "coherent", "volatile",
                                                           "readonly", "writeonly" };

    GLSLProgramManager::GLSLProgramManager( const GL3PlusSupport &support ) :
        mActiveVertexShader( NULL ),
        mActiveHullShader( NULL ),
        mActiveDomainShader( NULL ),
        mActiveGeometryShader( NULL ),
        mActiveFragmentShader( NULL ),
        mActiveComputeShader( NULL ),
        mGLSupport( support )
    {
        // Fill in the relationship between type names and enums
        mTypeEnumMap.emplace( "float", GL_FLOAT );
        mTypeEnumMap.emplace( "vec2", GL_FLOAT_VEC2 );
        mTypeEnumMap.emplace( "vec3", GL_FLOAT_VEC3 );
        mTypeEnumMap.emplace( "vec4", GL_FLOAT_VEC4 );
        mTypeEnumMap.emplace( "sampler1D", GL_SAMPLER_1D );
        mTypeEnumMap.emplace( "sampler2D", GL_SAMPLER_2D );
        mTypeEnumMap.emplace( "sampler3D", GL_SAMPLER_3D );
        mTypeEnumMap.emplace( "samplerCube", GL_SAMPLER_CUBE );
        mTypeEnumMap.emplace( "sampler1DShadow", GL_SAMPLER_1D_SHADOW );
        mTypeEnumMap.emplace( "sampler2DShadow", GL_SAMPLER_2D_SHADOW );
        mTypeEnumMap.emplace( "int", GL_INT );
        mTypeEnumMap.emplace( "ivec2", GL_INT_VEC2 );
        mTypeEnumMap.emplace( "ivec3", GL_INT_VEC3 );
        mTypeEnumMap.emplace( "ivec4", GL_INT_VEC4 );
        mTypeEnumMap.emplace( "bool", GL_BOOL );
        mTypeEnumMap.emplace( "bvec2", GL_BOOL_VEC2 );
        mTypeEnumMap.emplace( "bvec3", GL_BOOL_VEC3 );
        mTypeEnumMap.emplace( "bvec4", GL_BOOL_VEC4 );
        mTypeEnumMap.emplace( "mat2", GL_FLOAT_MAT2 );
        mTypeEnumMap.emplace( "mat3", GL_FLOAT_MAT3 );
        mTypeEnumMap.emplace( "mat4", GL_FLOAT_MAT4 );

        // GL 2.1
        mTypeEnumMap.emplace( "mat2x2", GL_FLOAT_MAT2 );
        mTypeEnumMap.emplace( "mat3x3", GL_FLOAT_MAT3 );
        mTypeEnumMap.emplace( "mat4x4", GL_FLOAT_MAT4 );
        mTypeEnumMap.emplace( "mat2x3", GL_FLOAT_MAT2x3 );
        mTypeEnumMap.emplace( "mat3x2", GL_FLOAT_MAT3x2 );
        mTypeEnumMap.emplace( "mat3x4", GL_FLOAT_MAT3x4 );
        mTypeEnumMap.emplace( "mat4x3", GL_FLOAT_MAT4x3 );
        mTypeEnumMap.emplace( "mat2x4", GL_FLOAT_MAT2x4 );
        mTypeEnumMap.emplace( "mat4x2", GL_FLOAT_MAT4x2 );

        // GL 3.0
        mTypeEnumMap.emplace( "uint", GL_UNSIGNED_INT );
        mTypeEnumMap.emplace( "uvec2", GL_UNSIGNED_INT_VEC2 );
        mTypeEnumMap.emplace( "uvec3", GL_UNSIGNED_INT_VEC3 );
        mTypeEnumMap.emplace( "uvec4", GL_UNSIGNED_INT_VEC4 );
        mTypeEnumMap.emplace( "samplerCubeShadow", GL_SAMPLER_CUBE_SHADOW );
        mTypeEnumMap.emplace( "sampler1DArray", GL_SAMPLER_1D_ARRAY );
        mTypeEnumMap.emplace( "sampler2DArray", GL_SAMPLER_2D_ARRAY );
        mTypeEnumMap.emplace( "sampler1DArrayShadow", GL_SAMPLER_1D_ARRAY_SHADOW );
        mTypeEnumMap.emplace( "sampler2DArrayShadow", GL_SAMPLER_2D_ARRAY_SHADOW );
        mTypeEnumMap.emplace( "isampler1D", GL_INT_SAMPLER_1D );
        mTypeEnumMap.emplace( "isampler2D", GL_INT_SAMPLER_2D );
        mTypeEnumMap.emplace( "isampler3D", GL_INT_SAMPLER_3D );
        mTypeEnumMap.emplace( "isamplerCube", GL_INT_SAMPLER_CUBE );
        mTypeEnumMap.emplace( "isampler1DArray", GL_INT_SAMPLER_1D_ARRAY );
        mTypeEnumMap.emplace( "isampler2DArray", GL_INT_SAMPLER_2D_ARRAY );
        mTypeEnumMap.emplace( "usampler1D", GL_UNSIGNED_INT_SAMPLER_1D );
        mTypeEnumMap.emplace( "usampler2D", GL_UNSIGNED_INT_SAMPLER_2D );
        mTypeEnumMap.emplace( "usampler3D", GL_UNSIGNED_INT_SAMPLER_3D );
        mTypeEnumMap.emplace( "usamplerCube", GL_UNSIGNED_INT_SAMPLER_CUBE );
        mTypeEnumMap.emplace( "usampler1DArray", GL_UNSIGNED_INT_SAMPLER_1D_ARRAY );
        mTypeEnumMap.emplace( "usampler2DArray", GL_UNSIGNED_INT_SAMPLER_2D_ARRAY );

        // GL 3.1
        mTypeEnumMap.emplace( "sampler2DRect", GL_SAMPLER_2D_RECT );
        mTypeEnumMap.emplace( "sampler2DRectShadow", GL_SAMPLER_2D_RECT_SHADOW );
        mTypeEnumMap.emplace( "isampler2DRect", GL_INT_SAMPLER_2D_RECT );
        mTypeEnumMap.emplace( "usampler2DRect", GL_UNSIGNED_INT_SAMPLER_2D_RECT );
        mTypeEnumMap.emplace( "samplerBuffer", GL_SAMPLER_BUFFER );
        mTypeEnumMap.emplace( "isamplerBuffer", GL_INT_SAMPLER_BUFFER );
        mTypeEnumMap.emplace( "usamplerBuffer", GL_UNSIGNED_INT_SAMPLER_BUFFER );

        // GL 3.2
        mTypeEnumMap.emplace( "sampler2DMS", GL_SAMPLER_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "isampler2DMS", GL_INT_SAMPLER_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "usampler2DMS", GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "sampler2DMSArray", GL_SAMPLER_2D_MULTISAMPLE_ARRAY );
        mTypeEnumMap.emplace( "isampler2DMSArray", GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY );
        mTypeEnumMap.emplace( "usampler2DMSArray", GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY );

        // GL 4.0
        mTypeEnumMap.emplace( "double", GL_DOUBLE );
        mTypeEnumMap.emplace( "dmat2", GL_DOUBLE_MAT2 );
        mTypeEnumMap.emplace( "dmat3", GL_DOUBLE_MAT3 );
        mTypeEnumMap.emplace( "dmat4", GL_DOUBLE_MAT4 );
        mTypeEnumMap.emplace( "dmat2x2", GL_DOUBLE_MAT2 );
        mTypeEnumMap.emplace( "dmat3x3", GL_DOUBLE_MAT3 );
        mTypeEnumMap.emplace( "dmat4x4", GL_DOUBLE_MAT4 );
        mTypeEnumMap.emplace( "dmat2x3", GL_DOUBLE_MAT2x3 );
        mTypeEnumMap.emplace( "dmat3x2", GL_DOUBLE_MAT3x2 );
        mTypeEnumMap.emplace( "dmat3x4", GL_DOUBLE_MAT3x4 );
        mTypeEnumMap.emplace( "dmat4x3", GL_DOUBLE_MAT4x3 );
        mTypeEnumMap.emplace( "dmat2x4", GL_DOUBLE_MAT2x4 );
        mTypeEnumMap.emplace( "dmat4x2", GL_DOUBLE_MAT4x2 );
        mTypeEnumMap.emplace( "dvec2", GL_DOUBLE_VEC2 );
        mTypeEnumMap.emplace( "dvec3", GL_DOUBLE_VEC3 );
        mTypeEnumMap.emplace( "dvec4", GL_DOUBLE_VEC4 );
        mTypeEnumMap.emplace( "samplerCubeArray", GL_SAMPLER_CUBE_MAP_ARRAY );
        mTypeEnumMap.emplace( "samplerCubeArrayShadow", GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW );
        mTypeEnumMap.emplace( "isamplerCubeArray", GL_INT_SAMPLER_CUBE_MAP_ARRAY );
        mTypeEnumMap.emplace( "usamplerCubeArray", GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY );

        mTypeEnumMap.emplace( "image1D", GL_IMAGE_1D );
        mTypeEnumMap.emplace( "iimage1D", GL_INT_IMAGE_1D );
        mTypeEnumMap.emplace( "uimage1D", GL_UNSIGNED_INT_IMAGE_1D );
        mTypeEnumMap.emplace( "image2D", GL_IMAGE_2D );
        mTypeEnumMap.emplace( "iimage2D", GL_INT_IMAGE_2D );
        mTypeEnumMap.emplace( "uimage2D", GL_UNSIGNED_INT_IMAGE_2D );
        mTypeEnumMap.emplace( "image3D", GL_IMAGE_3D );
        mTypeEnumMap.emplace( "iimage3D", GL_INT_IMAGE_3D );
        mTypeEnumMap.emplace( "uimage3D", GL_UNSIGNED_INT_IMAGE_3D );
        mTypeEnumMap.emplace( "image2DRect", GL_IMAGE_2D_RECT );
        mTypeEnumMap.emplace( "iimage2DRect", GL_INT_IMAGE_2D_RECT );
        mTypeEnumMap.emplace( "uimage2DRect", GL_UNSIGNED_INT_IMAGE_2D_RECT );
        mTypeEnumMap.emplace( "imageCube", GL_IMAGE_CUBE );
        mTypeEnumMap.emplace( "iimageCube", GL_INT_IMAGE_CUBE );
        mTypeEnumMap.emplace( "uimageCube", GL_UNSIGNED_INT_IMAGE_CUBE );
        mTypeEnumMap.emplace( "imageBuffer", GL_IMAGE_BUFFER );
        mTypeEnumMap.emplace( "iimageBuffer", GL_INT_IMAGE_BUFFER );
        mTypeEnumMap.emplace( "uimageBuffer", GL_UNSIGNED_INT_IMAGE_BUFFER );
        mTypeEnumMap.emplace( "image1DArray", GL_IMAGE_1D_ARRAY );
        mTypeEnumMap.emplace( "iimage1DArray", GL_INT_IMAGE_1D_ARRAY );
        mTypeEnumMap.emplace( "uimage1DArray", GL_UNSIGNED_INT_IMAGE_1D_ARRAY );
        mTypeEnumMap.emplace( "image2DArray", GL_IMAGE_2D_ARRAY );
        mTypeEnumMap.emplace( "iimage2DArray", GL_INT_IMAGE_2D_ARRAY );
        mTypeEnumMap.emplace( "uimage2DArray", GL_UNSIGNED_INT_IMAGE_2D_ARRAY );
        mTypeEnumMap.emplace( "imageCubeArray", GL_IMAGE_CUBE_MAP_ARRAY );
        mTypeEnumMap.emplace( "iimageCubeArray", GL_INT_IMAGE_CUBE_MAP_ARRAY );
        mTypeEnumMap.emplace( "uimageCubeArray", GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY );
        mTypeEnumMap.emplace( "image2DMS", GL_IMAGE_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "iimage2DMS", GL_INT_IMAGE_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "uimage2DMS", GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE );
        mTypeEnumMap.emplace( "image2DMSArray", GL_IMAGE_2D_MULTISAMPLE_ARRAY );
        mTypeEnumMap.emplace( "iimage2DMSArray", GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY );
        mTypeEnumMap.emplace( "uimage2DMSArray", GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY );

        // GL 4.2
        mTypeEnumMap.emplace( "atomic_uint", GL_UNSIGNED_INT_ATOMIC_COUNTER );
    }

    void GLSLProgramManager::convertGLUniformtoOgreType( GLenum gltype,
                                                         GpuConstantDefinition &defToUpdate )
    {
        // Note GLSL never packs rows into float4's (from an API perspective anyway)
        // therefore all values are tight in the buffer.
        // TODO Should the rest of the above enum types be included here?
        switch( gltype )
        {
        case GL_FLOAT:
            defToUpdate.constType = GCT_FLOAT1;
            break;
        case GL_FLOAT_VEC2:
            defToUpdate.constType = GCT_FLOAT2;
            break;
        case GL_FLOAT_VEC3:
            defToUpdate.constType = GCT_FLOAT3;
            break;
        case GL_FLOAT_VEC4:
            defToUpdate.constType = GCT_FLOAT4;
            break;
        case GL_SAMPLER_1D:
        case GL_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_1D:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            // need to record samplers for GLSL
            defToUpdate.constType = GCT_SAMPLER1D;
            break;
        case GL_SAMPLER_2D:
        case GL_SAMPLER_2D_RECT:  // TODO: Move these to a new type??
        case GL_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            defToUpdate.constType = GCT_SAMPLER2D;
            break;
        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
            defToUpdate.constType = GCT_SAMPLER3D;
            break;
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
            defToUpdate.constType = GCT_SAMPLERCUBE;
            break;
        case GL_SAMPLER_CUBE_MAP_ARRAY:
        case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
        case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
            defToUpdate.constType = GCT_SAMPLERCUBE_ARRAY;
            break;
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
            defToUpdate.constType = GCT_SAMPLER1DSHADOW;
            break;
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_2D_RECT_SHADOW:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            defToUpdate.constType = GCT_SAMPLER2DSHADOW;
            break;
        case GL_INT:
        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            defToUpdate.constType = GCT_INT1;
            break;
        case GL_INT_VEC2:
            defToUpdate.constType = GCT_INT2;
            break;
        case GL_INT_VEC3:
            defToUpdate.constType = GCT_INT3;
            break;
        case GL_INT_VEC4:
            defToUpdate.constType = GCT_INT4;
            break;
        case GL_FLOAT_MAT2:
            defToUpdate.constType = GCT_MATRIX_2X2;
            break;
        case GL_FLOAT_MAT3:
            defToUpdate.constType = GCT_MATRIX_3X3;
            break;
        case GL_FLOAT_MAT4:
            defToUpdate.constType = GCT_MATRIX_4X4;
            break;
        case GL_FLOAT_MAT2x3:
            defToUpdate.constType = GCT_MATRIX_2X3;
            break;
        case GL_FLOAT_MAT3x2:
            defToUpdate.constType = GCT_MATRIX_3X2;
            break;
        case GL_FLOAT_MAT2x4:
            defToUpdate.constType = GCT_MATRIX_2X4;
            break;
        case GL_FLOAT_MAT4x2:
            defToUpdate.constType = GCT_MATRIX_4X2;
            break;
        case GL_FLOAT_MAT3x4:
            defToUpdate.constType = GCT_MATRIX_3X4;
            break;
        case GL_FLOAT_MAT4x3:
            defToUpdate.constType = GCT_MATRIX_4X3;
            break;
        case GL_DOUBLE:
            defToUpdate.constType = GCT_DOUBLE1;
            break;
        case GL_DOUBLE_VEC2:
            defToUpdate.constType = GCT_DOUBLE2;
            break;
        case GL_DOUBLE_VEC3:
            defToUpdate.constType = GCT_DOUBLE3;
            break;
        case GL_DOUBLE_VEC4:
            defToUpdate.constType = GCT_DOUBLE4;
            break;
        case GL_DOUBLE_MAT2:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_2X2;
            break;
        case GL_DOUBLE_MAT3:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_3X3;
            break;
        case GL_DOUBLE_MAT4:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_4X4;
            break;
        case GL_DOUBLE_MAT2x3:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_2X3;
            break;
        case GL_DOUBLE_MAT3x2:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_3X2;
            break;
        case GL_DOUBLE_MAT2x4:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_2X4;
            break;
        case GL_DOUBLE_MAT4x2:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_4X2;
            break;
        case GL_DOUBLE_MAT3x4:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_3X4;
            break;
        case GL_DOUBLE_MAT4x3:
            defToUpdate.constType = GCT_MATRIX_DOUBLE_4X3;
            break;
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_INT_ATOMIC_COUNTER:  // TODO should be its own type?
            defToUpdate.constType = GCT_UINT1;
            break;
        case GL_UNSIGNED_INT_VEC2:
            defToUpdate.constType = GCT_UINT2;
            break;
        case GL_UNSIGNED_INT_VEC3:
            defToUpdate.constType = GCT_UINT3;
            break;
        case GL_UNSIGNED_INT_VEC4:
            defToUpdate.constType = GCT_UINT4;
            break;
        case GL_BOOL:
            defToUpdate.constType = GCT_BOOL1;
            break;
        case GL_BOOL_VEC2:
            defToUpdate.constType = GCT_BOOL2;
            break;
        case GL_BOOL_VEC3:
            defToUpdate.constType = GCT_BOOL3;
            break;
        case GL_BOOL_VEC4:
            defToUpdate.constType = GCT_BOOL4;
            break;
        default:
            if( gltype >= GL_IMAGE_1D && gltype <= GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY )
                defToUpdate.constType = GCT_INT1;
            else
                defToUpdate.constType = GCT_UNKNOWN;
            break;
        }

        // GL doesn't pad
        defToUpdate.elementSize = GpuConstantDefinition::getElementSize( defToUpdate.constType, false );
    }

    bool GLSLProgramManager::findUniformDataSource( const String &paramName,
                                                    const GpuConstantDefinitionMap *vertexConstantDefs,
                                                    const GpuConstantDefinitionMap *hullConstantDefs,
                                                    const GpuConstantDefinitionMap *domainConstantDefs,
                                                    const GpuConstantDefinitionMap *geometryConstantDefs,
                                                    const GpuConstantDefinitionMap *fragmentConstantDefs,
                                                    const GpuConstantDefinitionMap *computeConstantDefs,
                                                    GLUniformReference &refToUpdate )
    {
        if( vertexConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = vertexConstantDefs->find( paramName );
            if( parami != vertexConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_VERTEX_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        if( geometryConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = geometryConstantDefs->find( paramName );
            if( parami != geometryConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_GEOMETRY_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        if( fragmentConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = fragmentConstantDefs->find( paramName );
            if( parami != fragmentConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_FRAGMENT_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        if( hullConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = hullConstantDefs->find( paramName );
            if( parami != hullConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_HULL_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        if( domainConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = domainConstantDefs->find( paramName );
            if( parami != domainConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_DOMAIN_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        if( computeConstantDefs )
        {
            GpuConstantDefinitionMap::const_iterator parami = computeConstantDefs->find( paramName );
            if( parami != computeConstantDefs->end() )
            {
                refToUpdate.mSourceProgType = GPT_COMPUTE_PROGRAM;
                refToUpdate.mConstantDef = &( parami->second );
                return true;
            }
        }
        return false;
    }

    void GLSLProgramManager::extractUniformsFromProgram(
        GLuint programObject, const GpuConstantDefinitionMap *vertexConstantDefs,
        const GpuConstantDefinitionMap *hullConstantDefs,
        const GpuConstantDefinitionMap *domainConstantDefs,
        const GpuConstantDefinitionMap *geometryConstantDefs,
        const GpuConstantDefinitionMap *fragmentConstantDefs,
        const GpuConstantDefinitionMap *computeConstantDefs, GLUniformReferenceList &uniformList )
    {
        // Scan through the active uniforms and add them to the reference list.
        GLint uniformCount = 0;
#define uniformLength 200
        //              GLint uniformLength = 0;
        //        glGetProgramiv(programObject, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformLength);

        char uniformName[uniformLength];
        GLUniformReference newGLUniformReference;

        // Get the number of active uniforms, including atomic
        // counters and uniforms contained in uniform blocks.
        OGRE_CHECK_GL_ERROR( glGetProgramiv( programObject, GL_ACTIVE_UNIFORMS, &uniformCount ) );

        // Loop over each active uniform and add it to the reference
        // container.
        for( GLuint index = 0; index < (GLuint)uniformCount; index++ )
        {
            GLint arraySize;
            GLenum glType;
            OGRE_CHECK_GL_ERROR( glGetActiveUniform( programObject, index, uniformLength, NULL,
                                                     &arraySize, &glType, uniformName ) );

            // Don't add built in uniforms, atomic counters, or uniform block parameters.
            OGRE_CHECK_GL_ERROR( newGLUniformReference.mLocation =
                                     glGetUniformLocation( programObject, uniformName ) );
            if( newGLUniformReference.mLocation >= 0 )
            {
                // User defined uniform found, add it to the reference list.
                String paramName = String( uniformName );

                // Current ATI drivers (Catalyst 7.2 and earlier) and
                // older NVidia drivers will include all array
                // elements as uniforms but we only want the root
                // array name and location. Also note that ATI Catalyst
                // 6.8 to 7.2 there is a bug with glUniform that does
                // not allow you to update a uniform array past the
                // first uniform array element ie you can't start
                // updating an array starting at element 1, must
                // always be element 0.

                // If the uniform name has a "[" in it then its an array element uniform.
                String::size_type arrayStart = paramName.find( "[" );
                if( arrayStart != String::npos )
                {
                    // if not the first array element then skip it and continue to the next uniform
                    if( paramName.compare( arrayStart, paramName.size() - 1, "[0]" ) != 0 )
                        continue;
                    paramName = paramName.substr( 0, arrayStart );
                }

                // Find out which params object this comes from
                bool foundSource = findUniformDataSource(
                    paramName, vertexConstantDefs, geometryConstantDefs, fragmentConstantDefs,
                    hullConstantDefs, domainConstantDefs, computeConstantDefs, newGLUniformReference );

                // Only add this parameter if we found the source
                if( foundSource )
                {
                    assert( size_t( arraySize ) == newGLUniformReference.mConstantDef->arraySize &&
                            "GL doesn't agree with our array size!" );
                    uniformList.push_back( newGLUniformReference );
                }

                // Don't bother adding individual array params, they will be
                // picked up in the 'parent' parameter can copied all at once
                // anyway, individual indexes are only needed for lookup from
                // user params
            }  // end if
        }  // end for
    }

    void GLSLProgramManager::extractUniformsFromGLSL( const String &src, GpuNamedConstants &defs,
                                                      const String &filename )
    {
        // Parse the output string and collect all uniforms
        // NOTE this relies on the source already having been preprocessed
        // which is done in GLSLShader::loadFromSource
        String line;
        String::size_type currPos = src.find( "uniform" );
        while( currPos != String::npos )
        {
            // Now check for using the word 'uniform' in a larger string & ignore
            bool inLargerString = false;
            if( currPos != 0 )
            {
                char prev = src.at( currPos - 1 );
                if( prev != ' ' && prev != '\t' && prev != '\r' && prev != '\n' && prev != ';' )
                    inLargerString = true;
            }
            if( !inLargerString && currPos + 7 < src.size() )
            {
                char next = src.at( currPos + 7 );
                if( next != ' ' && next != '\t' && next != '\r' && next != '\n' )
                    inLargerString = true;
            }

            // skip 'uniform'
            currPos += 7;

            if( !inLargerString )
            {
                String::size_type endPos;
                String typeString;
                GpuSharedParametersPtr blockSharedParams;

                // Check for a type. If there is one, then the
                // semicolon is missing.  Otherwise treat as if it is
                // a uniform block.
                String::size_type lineEndPos = src.find_first_of( "\n\r", currPos );
                line = src.substr( currPos, lineEndPos - currPos );
                StringVector parts = StringUtil::split( line, " \t" );

                size_t partStart = 0;
                for( size_t i = 0; i < c_numTypeQualifiers && partStart < parts.size(); ++i )
                {
                    if( StringUtil::match( parts[partStart], c_typeQualifiers[i] ) )
                    {
                        i = 0;
                        ++partStart;
                    }
                }

                if( partStart < parts.size() )
                    typeString = parts[partStart];
                else
                    typeString = parts[0];

                StringToEnumMap::iterator typei = mTypeEnumMap.find( typeString );
                if( typei == mTypeEnumMap.end() )
                {
                    // Now there should be an opening brace
                    String::size_type openBracePos = src.find( "{", currPos );
                    if( openBracePos != String::npos )
                    {
                        currPos = openBracePos + 1;
                    }
                    else
                    {
                        LogManager::getSingleton().logMessage(
                            "Missing opening brace in GLSL Uniform Block in file " + filename,
                            LML_CRITICAL );
                        break;
                    }

                    // First we need to find the internal name for the uniform block
                    String::size_type endBracePos = src.find( "}", currPos );

                    // Find terminating semicolon
                    currPos = endBracePos + 1;
                    endPos = src.find( ";", currPos );
                    if( endPos == String::npos )
                    {
                        // problem, missing semicolon, abort
                        break;
                    }

                    // TODO: We don't need the internal name. Just skip over to the end of the block
                    // But we do need to know if this is an array of blocks. Is that legal?

                    // // Find the internal name.
                    // // This can be an array.
                    // line = src.substr(currPos, endPos - currPos);
                    // StringVector internalParts = StringUtil::split(line, ", \t\r\n");
                    // String internalName = "";
                    // uint16 arraySize = 0;
                    // for (StringVector::iterator i = internalParts.begin(); i != internalParts.end();
                    // ++i)
                    // {
                    //     StringUtil::trim(*i);
                    //     String::size_type arrayStart = i->find("[", 0);
                    //     if (arrayStart != String::npos)
                    //     {
                    //         // potential name (if butted up to array)
                    //         String name = i->substr(0, arrayStart);
                    //         StringUtil::trim(name);
                    //         if (!name.empty())
                    //             internalName = name;

                    //         String::size_type arrayEnd = i->find("]", arrayStart);
                    //         String arrayDimTerm = i->substr(arrayStart + 1, arrayEnd - arrayStart -
                    //         1); StringUtil::trim(arrayDimTerm); arraySize =
                    //         StringConverter::parseUnsignedInt(arrayDimTerm);
                    //     }
                    //     else
                    //     {
                    //         internalName = *i;
                    //     }
                    // }

                    // // Ok, now rewind and parse the individual uniforms in this block
                    // currPos = openBracePos + 1;
                    // blockSharedParams =
                    // GpuProgramManager::getSingleton().getSharedParameters(externalName);
                    // if(!blockSharedParams)
                    //     blockSharedParams =
                    //     GpuProgramManager::getSingleton().createSharedParameters(externalName);
                    // do
                    // {
                    //     lineEndPos = src.find_first_of("\n\r", currPos);
                    //     endPos = src.find(";", currPos);
                    //     line = src.substr(currPos, endPos - currPos);

                    //     // TODO: Give some sort of block id
                    //     // Parse the normally structured uniform
                    //     parseIndividualConstant(src, defs, currPos, filename, blockSharedParams);
                    //     currPos = lineEndPos + 1;
                    // } while (endBracePos > currPos);
                }
                else
                {
                    // find terminating semicolon
                    endPos = src.find( ";", currPos );
                    if( endPos == String::npos )
                    {
                        // problem, missing semicolon, abort
                        break;
                    }

                    parseGLSLUniform( src, defs, currPos, filename, blockSharedParams );
                }
                line = src.substr( currPos, endPos - currPos );
            }  // not commented or a larger symbol

            // Find next one
            currPos = src.find( "uniform", currPos );
        }
    }

    void GLSLProgramManager::parseGLSLUniform( const String &src, GpuNamedConstants &defs,
                                               String::size_type currPos, const String &filename,
                                               GpuSharedParametersPtr sharedParams )
    {
        GpuConstantDefinition def;
        String paramName = "";
        String::size_type endPos = src.find( ";", currPos );
        String line = src.substr( currPos, endPos - currPos );

        // Remove spaces before opening square braces, otherwise
        // the following split() can split the line at inappropriate
        // places (e.g. "vec3 something [3]" won't work).
        // FIXME What are valid ways of including spaces in GLSL
        // variable declarations?  May need regex.
        for( String::size_type sqp = line.find( " [" ); sqp != String::npos; sqp = line.find( " [" ) )
            line.erase( sqp, 1 );
        // Split into tokens
        StringVector parts = StringUtil::split( line, ", \t\r\n" );

        for( StringVector::iterator i = parts.begin(); i != parts.end(); ++i )
        {
            // Is this a type?
            StringToEnumMap::iterator typei = mTypeEnumMap.find( *i );
            if( typei != mTypeEnumMap.end() )
            {
                convertGLUniformtoOgreType( typei->second, def );
            }
            else
            {
                // if this is not a type, and not empty, it should be a name
                StringUtil::trim( *i );
                if( i->empty() )
                    continue;

                {
                    // Skip over precision keywords & other similar type qualifiers
                    bool skipElement = false;
                    for( size_t j = 0; j < c_numTypeQualifiers && !skipElement; ++j )
                        skipElement = StringUtil::match( *i, c_typeQualifiers[j] );

                    if( skipElement )
                        continue;
                }

                String::size_type arrayStart = i->find( "[", 0 );
                if( arrayStart != String::npos )
                {
                    // potential name (if butted up to array)
                    String name = i->substr( 0, arrayStart );
                    StringUtil::trim( name );
                    if( !name.empty() )
                        paramName = name;

                    def.arraySize = 1;

                    // N-dimensional arrays
                    while( arrayStart != String::npos )
                    {
                        String::size_type arrayEnd = i->find( "]", arrayStart );
                        String arrayDimTerm = i->substr( arrayStart + 1, arrayEnd - arrayStart - 1 );
                        StringUtil::trim( arrayDimTerm );
                        // TODO
                        // the array term might be a simple number or it might be
                        // an expression (e.g. 24*3) or refer to a constant expression
                        // we'd have to evaluate the expression which could get nasty
                        def.arraySize *= StringConverter::parseUnsignedInt( arrayDimTerm );
                        arrayStart = i->find( "[", arrayEnd );
                    }
                }
                else
                {
                    paramName = *i;
                    def.arraySize = 1;
                }

                // Name should be after the type, so complete def and add
                // We do this now so that comma-separated params will do
                // this part once for each name mentioned
                if( def.constType == GCT_UNKNOWN )
                {
                    LogManager::getSingleton().logMessage(
                        "Problem parsing the following GLSL Uniform: '" + line + "' in file " + filename,
                        LML_CRITICAL );
                    // next uniform
                    break;
                }

                // Special handling for shared parameters
                if( !sharedParams )
                {
                    // Complete def and add
                    // increment physical buffer location
                    def.logicalIndex = 0;  // not valid in GLSL
                    if( def.isFloat() )
                    {
                        def.physicalIndex = defs.floatBufferSize;
                        defs.floatBufferSize += def.arraySize * def.elementSize;
                    }
                    else if( def.isDouble() )
                    {
                        def.physicalIndex = defs.doubleBufferSize;
                        defs.doubleBufferSize += def.arraySize * def.elementSize;
                    }
                    else if( def.isInt() || def.isSampler() )
                    {
                        def.physicalIndex = defs.intBufferSize;
                        defs.intBufferSize += def.arraySize * def.elementSize;
                    }
                    else if( def.isUnsignedInt() || def.isBool() )
                    {
                        def.physicalIndex = defs.uintBufferSize;
                        defs.uintBufferSize += def.arraySize * def.elementSize;
                    }
                    // else if (def.isBool())
                    // {
                    //     def.physicalIndex = defs.boolBufferSize;
                    //     defs.boolBufferSize += def.arraySize * def.elementSize;
                    // }
                    else
                    {
                        LogManager::getSingleton().logMessage(
                            "Could not parse type of GLSL Uniform: '" + line + "' in file " + filename );
                    }
                    defs.map.emplace( paramName, def );

                    // Generate array accessors
                    defs.generateConstantDefinitionArrayEntries( paramName, def );
                }
                else
                {
                    try
                    {
                        const GpuConstantDefinition &sharedDef =
                            sharedParams->getConstantDefinition( paramName );
                        (void)sharedDef;  // Silence warning
                    }
                    catch( Exception & )
                    {
                        // This constant doesn't exist so we'll create a new one
                        sharedParams->addConstantDefinition( paramName, def.constType );
                    }
                }
            }
        }
    }
}  // namespace Ogre
