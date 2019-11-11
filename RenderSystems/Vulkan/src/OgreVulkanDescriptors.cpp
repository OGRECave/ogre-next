/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#include "OgreVulkanDescriptors.h"

#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanProgram.h"
#include "OgreVulkanUtils.h"

#include "OgreException.h"
#include "OgreLwString.h"

#include "SPIRV-Reflect/spirv_reflect.h"

namespace Ogre
{
    //-------------------------------------------------------------------------
    static String getSpirvReflectError( SpvReflectResult spirvReflectResult )
    {
        switch( spirvReflectResult )
        {
        case SPV_REFLECT_RESULT_SUCCESS:
            return "SPV_REFLECT_RESULT_SUCCESS";
        case SPV_REFLECT_RESULT_NOT_READY:
            return "SPV_REFLECT_RESULT_NOT_READY";
        case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED:
            return "SPV_REFLECT_RESULT_ERROR_PARSE_FAILED";
        case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED:
            return "SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED";
        case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED:
            return "SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED";
        case SPV_REFLECT_RESULT_ERROR_NULL_POINTER:
            return "SPV_REFLECT_RESULT_ERROR_NULL_POINTER";
        case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR:
            return "SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR";
        case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH:
            return "SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH";
        case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND:
            return "SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE:
            return "SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE";
        }

        return "SPV_REFLECT_INVALID_ERROR_CODE";
    }
    //-------------------------------------------------------------------------
    bool VulkanDescriptors::areBindingsCompatible( const VkDescriptorSetLayoutBinding &a,
                                                   const VkDescriptorSetLayoutBinding &b )
    {
        OGRE_ASSERT_HIGH( a.binding == b.binding && "Comparding bindings from different slots!" );
        return a.descriptorCount == 0u || b.descriptorCount == 0 ||
               ( a.descriptorType == b.descriptorType &&  //
                 a.descriptorCount == b.descriptorCount &&
                 a.pImmutableSamplers == b.pImmutableSamplers );
    }
    //-------------------------------------------------------------------------
    bool VulkanDescriptors::canMergeDescriptorSets( const DescriptorSetLayoutArray &a,
                                                    const DescriptorSetLayoutArray &b )
    {
        const size_t minSize = std::min( a.size(), b.size() );

        for( size_t i = 0u; i < minSize; ++i )
        {
            if( a[i].empty() || b[i].empty() )
                continue;

            const size_t minBindings = std::min( a[i].size(), b[i].size() );
            for( size_t j = 0u; j < minBindings; ++j )
            {
                if( !areBindingsCompatible( a[i][j], b[i][j] ) )
                {
                    char tmpBuffer[256];
                    LwString logMsg( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
                    logMsg.a( "Descriptor set ", (uint32)i, " binding ", (uint32)j,
                              " of two shader stages (e.g. vertex and pixel shader?) are not "
                              "compatible. These shaders cannot be used together" );
                    LogManager::getSingleton().logMessage( logMsg.c_str() );
                }
            }
        }

        return true;
    }
    //-------------------------------------------------------------------------
    void VulkanDescriptors::mergeDescriptorSets( DescriptorSetLayoutArray &a, const String &shaderB,
                                                 const DescriptorSetLayoutArray &b )
    {
        if( canMergeDescriptorSets( a, b ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Descriptor Sets from shader '" + shaderB +
                             "' cannot be merged because they contradict what's already been defined by "
                             "the previous shader stages. See Ogre.log for more details",
                         "VulkanDescriptors::mergeDescriptorSets" );
        }

        const size_t minSize = std::min( a.size(), b.size() );

        for( size_t i = 0u; i < minSize; ++i )
        {
            if( a[i].empty() )
            {
                // a doesn't use bindings at set i. We can copy all of them straight from b
                a[i] = b[i];
            }
            else if( !b[i].empty() )
            {
                const size_t minBindings = std::min( a[i].size(), b[i].size() );
                for( size_t j = 0u; j < minBindings; ++j )
                {
                    OGRE_ASSERT_HIGH( a[i][j].binding == j );
                    OGRE_ASSERT_HIGH( b[i][j].binding == j );
                    a[i][j].stageFlags |= b[i][j].stageFlags;
                }

                a[i].appendPOD( b[i].begin() + minBindings, b[i].end() );
            }
            // else
            //{
            //    b[i].empty() == true;
            //    There's nothing to merge from b
            //}
        }

        a.append( b.begin() + minSize, b.end() );
    }
    //-------------------------------------------------------------------------
    void VulkanDescriptors::generateDescriptorSets( const String &shaderName,
                                                    const std::vector<uint32> &spirv,
                                                    DescriptorSetLayoutArray &outputSets )
    {
        if( spirv.empty() )
            return;

        SpvReflectShaderModule module;
        memset( &module, 0, sizeof( module ) );
        SpvReflectResult result = spvReflectCreateShaderModule( spirv.size(), &spirv[0], &module );
        if( result == SPV_REFLECT_RESULT_SUCCESS )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "spvReflectCreateShaderModule failed on shader " + shaderName +
                             " error code: " + getSpirvReflectError( result ),
                         "VulkanDescriptors::generateDescriptorSet" );
        }

        uint32 numDescSets = 0;
        result = spvReflectEnumerateDescriptorSets( &module, &numDescSets, 0 );
        if( result == SPV_REFLECT_RESULT_SUCCESS )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "spvReflectEnumerateDescriptorSets failed on shader " + shaderName +
                             " error code: " + getSpirvReflectError( result ),
                         "VulkanDescriptors::generateDescriptorSet" );
        }

        FastArray<SpvReflectDescriptorSet *> sets;
        sets.resize( numDescSets );
        result = spvReflectEnumerateDescriptorSets( &module, &numDescSets, sets.begin() );
        if( result == SPV_REFLECT_RESULT_SUCCESS )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "spvReflectEnumerateDescriptorSets failed on shader " + shaderName +
                             " error code: " + getSpirvReflectError( result ),
                         "VulkanDescriptors::generateDescriptorSet" );
        }

        size_t numSets = 0u;
        FastArray<SpvReflectDescriptorSet *>::const_iterator itor = sets.begin();
        FastArray<SpvReflectDescriptorSet *>::const_iterator endt = sets.end();

        while( itor != endt )
        {
            numSets = std::max<size_t>( ( *itor )->set + 1u, numSets );
            ++itor;
        }

        outputSets.resize( numSets );

        itor = sets.begin();

        while( itor != endt )
        {
            const SpvReflectDescriptorSet &reflSet = **itor;

            FastArray<VkDescriptorSetLayoutBinding> &bindings = outputSets[reflSet.set];
            const size_t numUsedBindings = reflSet.binding_count;

            size_t numBindings = 0;
            for( size_t i = 0; i < numUsedBindings; ++i )
                numBindings = std::max<size_t>( reflSet.bindings[i]->binding + 1u, numBindings );

            bindings.resize( numBindings );
            memset( bindings.begin(), 0, sizeof( VkDescriptorSetLayoutBinding ) * bindings.size() );

            for( size_t i = 0; i < numUsedBindings; ++i )
            {
                const SpvReflectDescriptorBinding &reflBinding = *( reflSet.bindings[i] );
                VkDescriptorSetLayoutBinding descSetLayoutBinding;
                memset( &descSetLayoutBinding, 0, sizeof( descSetLayoutBinding ) );

                const size_t bindingIdx = reflBinding.binding;

                bindings[bindingIdx].binding = reflBinding.binding;
                bindings[bindingIdx].descriptorType =
                    static_cast<VkDescriptorType>( reflBinding.descriptor_type );
                bindings[bindingIdx].descriptorCount = 1u;
                for( uint32_t i_dim = 0; i_dim < reflBinding.array.dims_count; ++i_dim )
                    bindings[bindingIdx].descriptorCount *= reflBinding.array.dims[i_dim];
                bindings[bindingIdx].stageFlags =
                    static_cast<VkShaderStageFlagBits>( module.shader_stage );
                bindings.push_back( descSetLayoutBinding );
            }

            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanDescriptors::generateAndMergeDescriptorSets( VulkanProgram *shader,
                                                            DescriptorSetLayoutArray &outputSets )
    {
        DescriptorSetLayoutArray sets;
        generateDescriptorSets( shader->getName(), shader->getSpirv(), outputSets );
        if( outputSets.empty() )
            outputSets.swap( sets );
        else
            mergeDescriptorSets( outputSets, shader->getName(), sets );
    }
    //-------------------------------------------------------------------------
    void VulkanDescriptors::optimizeDescriptorSets( DescriptorSetLayoutArray &sets )
    {
        const size_t numSets = sets.size();
        for( size_t i = numSets; i--; )
        {
            const size_t numBindings = sets[i].size();
            for( size_t j = numBindings; j--; )
            {
                if( sets[i][j].descriptorCount == 0 )
                    sets[i].erase( sets[i].begin() + j );
            }
        }
    }
    //-------------------------------------------------------------------------
    VkPipelineLayout VulkanDescriptors::generateVkDescriptorSets( const DescriptorSetLayoutArray &sets )
    {
        VkDescriptorSetLayoutArray vkSets;

        VulkanGpuProgramManager *vulkanProgramManager =
            static_cast<VulkanGpuProgramManager *>( VulkanGpuProgramManager::getSingletonPtr() );

        vkSets.reserve( sets.size() );

        DescriptorSetLayoutArray::const_iterator itor = sets.begin();
        DescriptorSetLayoutArray::const_iterator endt = sets.end();

        while( itor != endt )
        {
            vkSets.push_back( vulkanProgramManager->getCachedSet( *itor ) );
            ++itor;
        }

        VkPipelineLayout retVal = vulkanProgramManager->getCachedSets( vkSets );
        return retVal;
    }
    //-------------------------------------------------------------------------
}  // namespace Ogre
