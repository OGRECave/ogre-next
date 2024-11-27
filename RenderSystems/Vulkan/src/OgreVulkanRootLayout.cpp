/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreVulkanRootLayout.h"

#include "OgreVulkanDescriptorPool.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanGlobalBindingTable.h"
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanUtils.h"
#include "Vao/OgreVulkanVaoManager.h"

#include "OgreException.h"
#include "OgreLwString.h"
#include "OgreStringConverter.h"

#if defined( __clang__ )
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#    pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#if defined( __clang__ )
#    pragma clang diagnostic pop
#endif

#include "vulkan/vulkan_core.h"

#define TODO_limit_NUM_BIND_TEXTURES  // and co.

namespace Ogre
{
    static const char c_bufferTypes[] = "PBRTtsUu";
    static const char c_HLSLBufferTypesMap[] = "ccuttsuu";
    //-------------------------------------------------------------------------
    uint32 toVkDescriptorType( DescBindingTypes::DescBindingTypes type )
    {
        switch( type )
        {
            // clang-format off
        case DescBindingTypes::ParamBuffer:       return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescBindingTypes::ConstBuffer:       return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescBindingTypes::ReadOnlyBuffer:    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescBindingTypes::TexBuffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case DescBindingTypes::Texture:           return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescBindingTypes::Sampler:           return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescBindingTypes::UavTexture:        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescBindingTypes::UavBuffer:         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescBindingTypes::NumDescBindingTypes:   return VK_DESCRIPTOR_TYPE_MAX_ENUM;
            // clang-format on
        }
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout::VulkanRootLayout( VulkanGpuProgramManager *programManager ) :
        mRootLayout( 0 ),
        mProgramManager( programManager )
    {
        memset( mArrayedSlots, 0, sizeof( mArrayedSlots ) );
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout::~VulkanRootLayout()
    {
        if( mRootLayout )
        {
            // This should not need to be delayed, since VulkanRootLayouts persists until shutdown
            vkDestroyPipelineLayout( mProgramManager->getDevice()->mDevice, mRootLayout, 0 );
            mRootLayout = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::copyFrom( const RootLayout &rootLayout, bool bIncludeArrayBindings )
    {
        OGRE_ASSERT_LOW( !mRootLayout && "Cannot call parseRootLayout after createVulkanHandles" );
        RootLayout::copyFrom( rootLayout, bIncludeArrayBindings );
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::copyTo( RootLayout &outRootLayout, bool bIncludeArrayBindings )
    {
        outRootLayout.copyFrom( *this, bIncludeArrayBindings );
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::parseRootLayout( const char *rootLayout, const bool bCompute,
                                            const String &filename )
    {
        OGRE_ASSERT_LOW( !mRootLayout && "Cannot call parseRootLayout after createVulkanHandles" );
        RootLayout::parseRootLayout( rootLayout, bCompute, filename );
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::generateRootLayoutMacros( uint32 shaderStage, ShaderSyntax shaderType,
                                                     String &inOutString ) const
    {
        String macroStr;
        macroStr.swap( inOutString );

        char tmpBuffer[256];
        LwString textStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        textStr.a( "#define ogre_" );
        const size_t prefixSize0 = textStr.size();

        for( size_t i = 0u; i < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            uint32 bindingIdx = 0u;
            for( size_t j = 0u; j < DescBindingTypes::NumDescBindingTypes; ++j )
            {
                textStr.resize( prefixSize0 );
                textStr.aChar( c_bufferTypes[j] );
                const size_t prefixSize1 = textStr.size();

                if( j == DescBindingTypes::ParamBuffer )
                {
                    // ParamBuffer is special because it's always ogre_P0, but we still
                    // must still account all the other stages' binding indices while counting
                    if( mDescBindingRanges[i][j].isInUse() )
                    {
                        if( mParamsBuffStages & ( 1u << shaderStage ) )
                        {
                            textStr.resize( prefixSize1 );  // #define ogre_P
                            uint32 numPrevStagesWithParams = 0u;
                            if( !mCompute )
                            {
                                for( size_t k = 0u; k < shaderStage; ++k )
                                {
                                    if( mParamsBuffStages & ( 1u << k ) )
                                        ++numPrevStagesWithParams;
                                }
                            }

                            if( shaderType == GLSL )
                            {
                                // #define ogre_P0 set = 1, binding = 6
                                textStr.a( "0", " set = ", (uint32)i,
                                           ", binding = ", numPrevStagesWithParams, "\n" );
                            }
                            else
                            {
                                // #define ogre_B3 c3
                                textStr.a( "0 " );
                                textStr.aChar( c_HLSLBufferTypesMap[j] );
                                textStr.a( numPrevStagesWithParams, "\n" );
                            }

                            macroStr += textStr.c_str();
                        }

                        const uint32 numSlots = mDescBindingRanges[i][j].getNumUsedSlots();
                        bindingIdx += numSlots;
                    }
                }
                else
                {
                    uint32 emulatedSlot = mDescBindingRanges[i][j].start;
                    const uint32 numSlots = mDescBindingRanges[i][j].getNumUsedSlots();

                    FastArray<uint32>::const_iterator arrayRangesEnd = mArrayRanges[j].end();
                    FastArray<uint32>::const_iterator arrayRanges =
                        std::lower_bound( mArrayRanges[j].begin(), arrayRangesEnd,
                                          ArrayDesc( static_cast<uint16>( emulatedSlot ), 0u ).toKey() );

                    for( size_t k = 0u; k < numSlots; ++k )
                    {
                        textStr.resize( prefixSize1 );  // #define ogre_B
                        if( shaderType == GLSL )
                        {
                            // #define ogre_B3 set = 1, binding = 6
                            textStr.a( emulatedSlot, " set = ", (uint32)i, ", binding = ", bindingIdx,
                                       "\n" );
                        }
                        else
                        {
                            // #define ogre_B3 c3
                            textStr.a( emulatedSlot, " " );
                            textStr.aChar( c_HLSLBufferTypesMap[j] );
                            textStr.a( bindingIdx, "\n" );
                        }

                        bool bIsArray = false;
                        if( arrayRanges != arrayRangesEnd )
                        {
                            const ArrayDesc arrayDesc = ArrayDesc::fromKey( *arrayRanges );
                            if( arrayDesc.bindingIdx == emulatedSlot )
                            {
                                ++bindingIdx;
                                emulatedSlot += arrayDesc.arraySize;
                                k += arrayDesc.arraySize - 1u;
                                ++arrayRanges;
                                bIsArray = true;
                            }
                        }

                        if( !bIsArray )
                        {
                            ++bindingIdx;
                            ++emulatedSlot;
                        }

                        macroStr += textStr.c_str();
                    }
                }
            }
        }
        macroStr.swap( inOutString );
    }
    //-------------------------------------------------------------------------
    VkPipelineLayout VulkanRootLayout::createVulkanHandles()
    {
        {
            // If mRootLayout is nullptr but being created in another thread, we will check again
            // but with a proper lock. If it's not a nullptr, it means it's properly been created.
            VkPipelineLayout retVal = mRootLayout.load( std::memory_order::memory_order_relaxed );
            if( retVal )
                return retVal;
        }

        ScopedLock lock( mProgramManager->getMutexRootLayoutHandles() );

        if( mRootLayout )
            return mRootLayout;  // Checking again as previous check was unsafe.

        FastArray<FastArray<VkDescriptorSetLayoutBinding> > rootLayoutDesc;

        const size_t numSets = calculateNumUsedSets();
        rootLayoutDesc.resize( numSets );
        mSets.resize( numSets );
        mPools.resize( numSets );

        for( size_t i = 0u; i < numSets; ++i )
        {
            rootLayoutDesc[i].resize( calculateNumBindings( i ) );

            size_t bindingIdx = 0u;
            size_t bindingSlot = 0u;

            for( size_t j = 0u; j < DescBindingTypes::NumDescBindingTypes; ++j )
            {
                const uint16 descSlotStart = mDescBindingRanges[i][j].start;
                const FastArray<uint32>::const_iterator arrayRangesBeg = mArrayRanges[j].begin();
                const FastArray<uint32>::const_iterator arrayRangesEnd = mArrayRanges[j].end();

                FastArray<uint32>::const_iterator arrayRanges = std::lower_bound(
                    arrayRangesBeg, arrayRangesEnd, ArrayDesc( descSlotStart, 0u ).toKey() );

                const size_t numSlots = mDescBindingRanges[i][j].getNumUsedSlots();
                for( size_t k = 0u; k < numSlots; ++k )
                {
                    rootLayoutDesc[i][bindingIdx].binding = static_cast<uint32_t>( bindingSlot );
                    rootLayoutDesc[i][bindingIdx].descriptorType = static_cast<VkDescriptorType>(
                        toVkDescriptorType( static_cast<DescBindingTypes::DescBindingTypes>( j ) ) );

                    rootLayoutDesc[i][bindingIdx].descriptorCount = 1u;
                    rootLayoutDesc[i][bindingIdx].stageFlags =
                        mCompute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;

                    if( !mCompute && j == DescBindingTypes::ParamBuffer &&
                        ( mParamsBuffStages & c_allGraphicStagesMask ) != c_allGraphicStagesMask )
                    {
                        if( mParamsBuffStages & ( 1u << VertexShader ) )
                            rootLayoutDesc[i][bindingIdx].stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
                        if( mParamsBuffStages & ( 1u << PixelShader ) )
                            rootLayoutDesc[i][bindingIdx].stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
                        if( mParamsBuffStages & ( 1u << GeometryShader ) )
                            rootLayoutDesc[i][bindingIdx].stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
                        if( mParamsBuffStages & ( 1u << HullShader ) )
                        {
                            rootLayoutDesc[i][bindingIdx].stageFlags |=
                                VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                        }
                        if( mParamsBuffStages & ( 1u << DomainShader ) )
                        {
                            rootLayoutDesc[i][bindingIdx].stageFlags |=
                                VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                        }
                    }
                    rootLayoutDesc[i][bindingIdx].pImmutableSamplers = 0;

                    if( arrayRanges != arrayRangesEnd )
                    {
                        const ArrayDesc arrayDesc = ArrayDesc::fromKey( *arrayRanges );
                        if( arrayDesc.bindingIdx == descSlotStart + k )
                        {
                            rootLayoutDesc[i][bindingIdx].descriptorCount = arrayDesc.arraySize;
                            ++arrayRanges;
                            k += arrayDesc.arraySize - 1u;

                            mArrayedSlots[i][j] += arrayDesc.arraySize - 1u;
                        }
                    }

                    ++bindingIdx;
                    ++bindingSlot;
                }
            }

            // If there's arrays, we created more descriptors than needed. Trim them
            rootLayoutDesc[i].resizePOD( bindingIdx );

            mSets[i] = mProgramManager->getCachedSet( rootLayoutDesc[i] );
        }

        VulkanDevice *device = mProgramManager->getDevice();

        VkPipelineLayoutCreateInfo pipelineLayoutCi;
        makeVkStruct( pipelineLayoutCi, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO );
        pipelineLayoutCi.setLayoutCount = static_cast<uint32>( numSets );
        pipelineLayoutCi.pSetLayouts = mSets.begin();

        VkPipelineLayout rootLayoutResult;
        VkResult result =
            vkCreatePipelineLayout( device->mDevice, &pipelineLayoutCi, 0, &rootLayoutResult );
        checkVkResult( result, "vkCreatePipelineLayout" );

        mRootLayout.store( rootLayoutResult, std::memory_order::memory_order_relaxed );

        return mRootLayout;
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindCommon( VkWriteDescriptorSet &writeDescSet,
                                              size_t &numWriteDescSets, uint32 &currBinding,
                                              VkDescriptorSet descSet,
                                              const DescBindingRange &bindRanges,
                                              const uint32 arrayedSlots )
    {
        makeVkStruct( writeDescSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
        writeDescSet.dstSet = descSet;
        writeDescSet.dstBinding = currBinding;
        writeDescSet.dstArrayElement = 0u;
        writeDescSet.descriptorCount = static_cast<uint32_t>( bindRanges.getNumUsedSlots() );
        currBinding += uint32( bindRanges.getNumUsedSlots() - arrayedSlots );
        ++numWriteDescSets;
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindParamsBuffer( VkWriteDescriptorSet *writeDescSets,
                                                    size_t &numWriteDescSets, uint32 &currBinding,
                                                    VkDescriptorSet descSet,
                                                    const DescBindingRange *descBindingRanges,
                                                    const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::ParamBuffer];

        if( !bindRanges.isInUse() )
            return;

        if( mCompute )
        {
            VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
            bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges, 0u );
            writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescSet.pBufferInfo = &table.paramsBuffer[GPT_COMPUTE_PROGRAM];
        }
        else
        {
            for( size_t i = 0u; i < NumShaderTypes; ++i )
            {
                if( mParamsBuffStages & ( 1u << i ) )
                {
                    VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];

                    makeVkStruct( writeDescSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
                    writeDescSet.dstSet = descSet;
                    writeDescSet.dstBinding = currBinding;
                    writeDescSet.dstArrayElement = 0u;
                    writeDescSet.descriptorCount = 1u;
                    ++currBinding;
                    ++numWriteDescSets;

                    writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    writeDescSet.pBufferInfo = &table.paramsBuffer[i];
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindConstBuffers( VkWriteDescriptorSet *writeDescSets,
                                                    size_t &numWriteDescSets, uint32 &currBinding,
                                                    VkDescriptorSet descSet,
                                                    const DescBindingRange *descBindingRanges,
                                                    const uint32 *arrayedSlots,
                                                    const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::ConstBuffer];

        if( !bindRanges.isInUse() )
            return;

        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
        bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges,
                    arrayedSlots[DescBindingTypes::ConstBuffer] );
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescSet.pBufferInfo = &table.constBuffers[bindRanges.start];
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindReadOnlyBuffers( VkWriteDescriptorSet *writeDescSets,
                                                       size_t &numWriteDescSets, uint32 &currBinding,
                                                       VkDescriptorSet descSet,
                                                       const DescBindingRange *descBindingRanges,
                                                       const uint32 *arrayedSlots,
                                                       const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::ReadOnlyBuffer];

        if( !bindRanges.isInUse() )
            return;

        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
        bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges,
                    arrayedSlots[DescBindingTypes::ReadOnlyBuffer] );
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescSet.pBufferInfo = &table.readOnlyBuffers[bindRanges.start];
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindTexBuffers( VkWriteDescriptorSet *writeDescSets,
                                                  size_t &numWriteDescSets, uint32 &currBinding,
                                                  VkDescriptorSet descSet,
                                                  const DescBindingRange *descBindingRanges,
                                                  const uint32 *arrayedSlots,
                                                  const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::TexBuffer];

        if( !bindRanges.isInUse() )
            return;

        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
        bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges,
                    arrayedSlots[DescBindingTypes::TexBuffer] );
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        writeDescSet.pTexelBufferView = &table.texBuffers[bindRanges.start];
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindTextures( VkWriteDescriptorSet *writeDescSets,
                                                size_t &numWriteDescSets, uint32 &currBinding,
                                                VkDescriptorSet descSet,
                                                const DescBindingRange *descBindingRanges,
                                                const uint32 *arrayedSlots,
                                                const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::Texture];

        if( !bindRanges.isInUse() )
            return;

        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
        bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges,
                    arrayedSlots[DescBindingTypes::Texture] );
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescSet.pImageInfo = &table.textures[bindRanges.start];
    }
    //-------------------------------------------------------------------------
    inline void VulkanRootLayout::bindSamplers( VkWriteDescriptorSet *writeDescSets,
                                                size_t &numWriteDescSets, uint32 &currBinding,
                                                VkDescriptorSet descSet,
                                                const DescBindingRange *descBindingRanges,
                                                const uint32 *arrayedSlots,
                                                const VulkanGlobalBindingTable &table )
    {
        const DescBindingRange &bindRanges = descBindingRanges[DescBindingTypes::Sampler];

        if( !bindRanges.isInUse() )
            return;

        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
        bindCommon( writeDescSet, numWriteDescSets, currBinding, descSet, bindRanges,
                    arrayedSlots[DescBindingTypes::Sampler] );
        writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writeDescSet.pImageInfo = &table.samplers[bindRanges.start];
    }
    //-------------------------------------------------------------------------
    uint32 VulkanRootLayout::calculateFirstDirtySet( const VulkanGlobalBindingTable &table ) const
    {
        uint32 firstDirtySet = 0u;
        const size_t numSets = mSets.size();
        for( size_t i = 0u; i < numSets; ++i )
        {
            bool bDirty = false;

            const DescBindingRange *ranges = mDescBindingRanges[i];

            if( !mBaked[i] )
            {
                bDirty |= table.dirtyParamsBuffer & ranges[DescBindingTypes::ParamBuffer].isInUse();
                bDirty |= ranges[DescBindingTypes::ConstBuffer].isDirty( table.minDirtySlotConst );
                bDirty |= ranges[DescBindingTypes::TexBuffer].isDirty( table.minDirtySlotTexBuffer );
                bDirty |= ranges[DescBindingTypes::Texture].isDirty( table.minDirtySlotTextures );
                bDirty |= ranges[DescBindingTypes::Sampler].isDirty( table.minDirtySlotSamplers );
                bDirty |=
                    ranges[DescBindingTypes::ReadOnlyBuffer].isDirty( table.minDirtySlotReadOnlyBuffer );
            }
            else
            {
                bDirty |= (int)table.dirtyBakedTextures &
                          ( (int)ranges[DescBindingTypes::ReadOnlyBuffer].isInUse() |
                            (int)ranges[DescBindingTypes::TexBuffer].isInUse() |
                            (int)ranges[DescBindingTypes::Texture].isInUse() );
                bDirty |= (int)table.dirtyBakedSamplers & ranges[DescBindingTypes::Sampler].isInUse();
                bDirty |=
                    (int)table.dirtyBakedUavs & ( (int)ranges[DescBindingTypes::UavBuffer].isInUse() |
                                                  (int)ranges[DescBindingTypes::UavTexture].isInUse() );
            }

            if( bDirty )
                return firstDirtySet;

            ++firstDirtySet;
        }

        return firstDirtySet;
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::bind( VulkanDevice *device, VulkanVaoManager *vaoManager,
                                 const VulkanGlobalBindingTable &table )
    {
        VkDescriptorSet descSets[OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS];

        const size_t numSets = mSets.size();

        const uint32 firstDirtySet = calculateFirstDirtySet( table );

        for( size_t i = firstDirtySet; i < numSets; ++i )
        {
            if( !mPools[i] || !mPools[i]->isAvailableInCurrentFrame() )
                mPools[i] = vaoManager->getDescriptorPool( this, i, mSets[i] );

            VkDescriptorSet descSet = mPools[i]->allocate( device, mSets[i] );

            const DescBindingRange *descBindingRanges = mDescBindingRanges[i];
            const uint32 *arrayedSlots = mArrayedSlots[i];

            size_t numWriteDescSets = 0u;
            uint32 currBinding = 0u;
            // ParamBuffer consumes up to 1 per stage (not just 1)
            VkWriteDescriptorSet writeDescSets[DescBindingTypes::Sampler + uint32( NumShaderTypes )];

            // Note: We must bind in the same order as DescBindingTypes
            if( !mBaked[i] )
            {
                bindParamsBuffer( writeDescSets, numWriteDescSets, currBinding, descSet,
                                  descBindingRanges, table );
                bindConstBuffers( writeDescSets, numWriteDescSets, currBinding, descSet,
                                  descBindingRanges, arrayedSlots, table );
                bindReadOnlyBuffers( writeDescSets, numWriteDescSets, currBinding, descSet,
                                     descBindingRanges, arrayedSlots, table );
                bindTexBuffers( writeDescSets, numWriteDescSets, currBinding, descSet, descBindingRanges,
                                arrayedSlots, table );
                bindTextures( writeDescSets, numWriteDescSets, currBinding, descSet, descBindingRanges,
                              arrayedSlots, table );
                bindSamplers( writeDescSets, numWriteDescSets, currBinding, descSet, descBindingRanges,
                              arrayedSlots, table );
            }
            else
            {
                for( size_t j = 0u; j < BakedDescriptorSets::NumBakedDescriptorSets; ++j )
                {
                    const DescBindingRange &bindRanges =
                        descBindingRanges[j + DescBindingTypes::ReadOnlyBuffer];
                    if( bindRanges.isInUse() )
                    {
                        OGRE_ASSERT_MEDIUM(
                            table.bakedDescriptorSets[j] &&
                            "No DescriptorSetTexture/Sampler/Uav bound when expected!\n"
                            "Forgot a call to "
                            "RenderSystem::_setTextures/_setTextures2/_setSamplers/etc?" );
                        OGRE_ASSERT_MEDIUM( table.bakedDescriptorSets[j]->descriptorCount ==
                                                bindRanges.getNumUsedSlots() &&
                                            "DescriptorSetTexture/Sampler/Uav provided is incompatible "
                                            "with active RootLayout. e.g. you can't bind a set of "
                                            "7 textures when the shader expects 4" );

                        VkWriteDescriptorSet &writeDescSet = writeDescSets[numWriteDescSets];
                        writeDescSet = *table.bakedDescriptorSets[j];
                        writeDescSet.dstSet = descSet;
                        writeDescSet.dstBinding = currBinding;
                        currBinding += writeDescSet.descriptorCount -
                                       arrayedSlots[j + DescBindingTypes::ReadOnlyBuffer];
                        ++numWriteDescSets;
                    }
                }
            }

            vkUpdateDescriptorSets( device->mDevice, static_cast<uint32_t>( numWriteDescSets ),
                                    writeDescSets, 0u, 0 );
            descSets[i] = descSet;
        }

        // The table may have been dirty, but nothing the root layout actually uses was dirty
        if( firstDirtySet < mSets.size() )
        {
            vkCmdBindDescriptorSets(
                device->mGraphicsQueue.getCurrentCmdBuffer(),
                mCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, mRootLayout,
                firstDirtySet, static_cast<uint32_t>( mSets.size() ) - firstDirtySet,
                &descSets[firstDirtySet], 0u, 0 );
        }
    }
    //-------------------------------------------------------------------------
    bool VulkanRootLayout::findBindingIndex( const uint32 setIdx, const uint32 targetBindingIdx,
                                             DescBindingTypes::DescBindingTypes &outType,
                                             size_t &outRelativeSlotIndex ) const
    {
        size_t currBindingIdx = 0u;
        for( size_t i = 0u; i < DescBindingTypes::NumDescBindingTypes; ++i )
        {
            if( mDescBindingRanges[setIdx][i].isInUse() )
            {
                if( mArrayRanges[i].empty() )
                {
                    // Fast path, N emulated binding = N binding slot
                    if( targetBindingIdx <= currBindingIdx )
                    {
                        outType = static_cast<DescBindingTypes::DescBindingTypes>( i );
                        outRelativeSlotIndex =
                            currBindingIdx - targetBindingIdx + mDescBindingRanges[setIdx][i].start;
                        return true;
                    }

                    const size_t nextBindingIdx =
                        currBindingIdx + mDescBindingRanges[setIdx][i].getNumUsedSlots();

                    if( targetBindingIdx < nextBindingIdx )
                    {
                        outType = static_cast<DescBindingTypes::DescBindingTypes>( i );
                        outRelativeSlotIndex =
                            targetBindingIdx - currBindingIdx + mDescBindingRanges[setIdx][i].start;
                        return true;
                    }

                    currBindingIdx = nextBindingIdx;
                }
                else
                {
                    // Slow path, N emulated bindings > M binding slots
                    uint32 emulatedSlot = mDescBindingRanges[setIdx][i].start;
                    const size_t numSlots = mDescBindingRanges[setIdx][i].getNumUsedSlots();

                    FastArray<uint32>::const_iterator arrayRangesEnd = mArrayRanges[i].end();
                    FastArray<uint32>::const_iterator arrayRanges =
                        std::lower_bound( mArrayRanges[i].begin(), arrayRangesEnd,
                                          ArrayDesc( static_cast<uint16>( emulatedSlot ), 0u ).toKey() );

                    for( size_t k = 0u; k < numSlots; ++k )
                    {
                        if( currBindingIdx == targetBindingIdx )
                        {
                            outType = static_cast<DescBindingTypes::DescBindingTypes>( i );
                            outRelativeSlotIndex = emulatedSlot;
                            return true;
                        }

                        bool bIsArray = false;
                        if( arrayRanges != arrayRangesEnd )
                        {
                            const ArrayDesc arrayDesc = ArrayDesc::fromKey( *arrayRanges );
                            if( arrayDesc.bindingIdx == emulatedSlot )
                            {
                                ++currBindingIdx;
                                emulatedSlot += arrayDesc.arraySize;
                                k += arrayDesc.arraySize - 1u;
                                ++arrayRanges;
                                bIsArray = true;
                            }
                        }

                        if( !bIsArray )
                        {
                            ++currBindingIdx;
                            ++emulatedSlot;
                        }
                    }
                }
            }
        }

        return false;
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout *VulkanRootLayout::findBest( VulkanRootLayout *a, VulkanRootLayout *b )
    {
        if( !b )
            return a;
        if( !a )
            return b;
        if( a == b )
            return a;

        VulkanRootLayout *best = 0;

        for( size_t i = DescBindingTypes::ParamBuffer + 1u; i < DescBindingTypes::NumDescBindingTypes;
             ++i )
        {
            VulkanRootLayout *newBest = 0;
            if( !a->mArrayRanges[i].empty() )
                newBest = a;

            if( !b->mArrayRanges[i].empty() )
                newBest = b;

            if( newBest )
            {
                if( best && best != newBest )
                {
                    // In theory we could try merging and remove duplicates and perhaps arrive return
                    // a 3rd RootLayout that is compatible but that's a lot of work. It's easier to let
                    // the user specify a custom RootLayout in the source code for both shaders
                    return 0;  // a and b are incompatible
                }

                best = newBest;
            }
        }

        for( size_t i = 0u; i < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            VulkanRootLayout *other = 0;

            bool bDiverged = false;
            size_t numSlotsA = 0u;
            size_t numSlotsB = 0u;
            for( size_t j = 0u; j < DescBindingTypes::NumDescBindingTypes; ++j )
            {
                numSlotsA += a->mDescBindingRanges[i][j].getNumUsedSlots();
                numSlotsB += b->mDescBindingRanges[i][j].getNumUsedSlots();

                if( !bDiverged )
                {
                    if( numSlotsA != numSlotsB )
                    {
                        VulkanRootLayout *newBest = 0;
                        if( numSlotsA > numSlotsB )
                        {
                            newBest = a;
                            other = b;
                        }
                        else
                        {
                            newBest = b;
                            other = a;
                        }

                        if( best && best != newBest )
                        {
                            // This is the first divergence within this set idx
                            // However a previous set diverged; and the 'best' one
                            // is not this time's winner.
                            //
                            // a and b are incompatible
                            return 0;
                        }

                        best = newBest;
                        bDiverged = true;
                    }
                }
                else
                {
                    // a and b were already on a path to being incompatible
                    if( other->mDescBindingRanges[i][j].isInUse() )
                        return 0;  // a and b are incompatible
                }
            }
        }

        if( !best )
        {
            // If we're here then a and b are equivalent? We should not arrive here due to
            // VulkanGpuProgramManager::getRootLayout always returning the same layout.
            // Anyway, pick any just in case
            best = a;
        }

        return best;
    }
    //-------------------------------------------------------------------------
    bool VulkanRootLayout::operator<( const VulkanRootLayout &other ) const
    {
        if( this->mCompute != other.mCompute )
            return this->mCompute < other.mCompute;
        if( this->mParamsBuffStages != other.mParamsBuffStages )
            return this->mParamsBuffStages < other.mParamsBuffStages;

        for( size_t i = 0u; i < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            if( this->mBaked[i] != other.mBaked[i] )
                return this->mBaked[i] < other.mBaked[i];

            for( size_t j = 0u; j < DescBindingTypes::NumDescBindingTypes; ++j )
            {
                if( this->mDescBindingRanges[i][j].start != other.mDescBindingRanges[i][j].start )
                    return this->mDescBindingRanges[i][j].start < other.mDescBindingRanges[i][j].start;
                if( this->mDescBindingRanges[i][j].end != other.mDescBindingRanges[i][j].end )
                    return this->mDescBindingRanges[i][j].end < other.mDescBindingRanges[i][j].end;
            }
        }

        for( size_t i = 0u; i < DescBindingTypes::NumDescBindingTypes; ++i )
        {
            if( this->mArrayRanges[i].size() != other.mArrayRanges[i].size() )
            {
                return this->mArrayRanges[i].size() < other.mArrayRanges[i].size();
            }
            else
            {
                const size_t numArrayRanges = this->mArrayRanges[i].size();
                for( size_t j = 0u; j < numArrayRanges; ++j )
                {
                    if( this->mArrayRanges[i][j] != other.mArrayRanges[i][j] )
                        return this->mArrayRanges[i][j] < other.mArrayRanges[i][j];
                }
            }
        }

        // If we're here then a and b are equals, thus a < b returns false
        return false;
    }
}  // namespace Ogre
