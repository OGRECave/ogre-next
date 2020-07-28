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

#include "OgreVulkanRootLayout.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanGpuProgramManager.h"
#include "OgreVulkanUtils.h"

#include "OgreException.h"
#include "OgreLwString.h"
#include "OgreStringConverter.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    static const char *c_rootLayoutVarNames[VulkanDescBindingTypes::NumDescBindingTypes] = {
        "const_buffers",  //
        "tex_buffers",    //
        "textures",       //
        "samplers",       //
        "uav_textures",   //
        "uav_buffers",    //
    };
    //-------------------------------------------------------------------------
    static VkDescriptorType toVkDescriptorType( VulkanDescBindingTypes::VulkanDescBindingTypes type )
    {
        switch( type )
        {
            // clang-format off
        case VulkanDescBindingTypes::ConstBuffer:       return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case VulkanDescBindingTypes::TexBuffer:         return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case VulkanDescBindingTypes::Texture:           return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case VulkanDescBindingTypes::Sampler:           return VK_DESCRIPTOR_TYPE_SAMPLER;
        case VulkanDescBindingTypes::UavTexture:        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case VulkanDescBindingTypes::UavBuffer:         return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case VulkanDescBindingTypes::NumDescBindingTypes:   return VK_DESCRIPTOR_TYPE_MAX_ENUM;
            // clang-format on
        }
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
    //-------------------------------------------------------------------------
    VulkanDescBindingRange::VulkanDescBindingRange() : start( 0u ), end( 0u ) {}
    //-------------------------------------------------------------------------
    VulkanRootLayout::VulkanRootLayout( VulkanGpuProgramManager *programManager ) :
        mCompute( false ),
        mRootLayout( 0 ),
        mProgramManager( programManager )
    {
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout::~VulkanRootLayout()
    {
        if( mRootLayout )
        {
            vkDestroyPipelineLayout( mProgramManager->getDevice()->mDevice, mRootLayout, 0 );
            mRootLayout = 0;
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::validate( const String &filename ) const
    {
        for( size_t i = 0u; i < VulkanDescBindingTypes::NumDescBindingTypes; ++i )
        {
            for( size_t j = 0u; j < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS - 1u; ++j )
            {
                if( mDescBindingRanges[j][i].end > mDescBindingRanges[j + 1][i].start )
                {
                    char tmpBuffer[1024];
                    LwString tmpStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

                    tmpStr.a( "Descriptors cannot overlap across sets. However set ", (uint32)j,
                              " specifies ", c_rootLayoutVarNames[i], " in range [",
                              mDescBindingRanges[j][i].start );
                    tmpStr.a( ", ", (uint32)mDescBindingRanges[j][i].end, ") but set ",
                              ( uint32 )( j + 1u ), " is in range [",
                              mDescBindingRanges[j + 1][i].start );
                    tmpStr.a( ", ", mDescBindingRanges[j + 1][i].end, ")" );

                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Error at file " + filename + ":\n" + tmpStr.c_str(),
                                 "VulkanRootLayout::parseSet" );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::parseSet( const rapidjson::Value &jsonValue, const size_t setIdx,
                                     const String &filename )
    {
        rapidjson::Value::ConstMemberIterator itor;

        for( size_t i = 0u; i < VulkanDescBindingTypes::NumDescBindingTypes; ++i )
        {
            itor = jsonValue.FindMember( c_rootLayoutVarNames[i] );
            if( itor != jsonValue.MemberEnd() && jsonValue.IsArray() && jsonValue.Size() == 2u &&
                jsonValue[0].IsUint() && jsonValue[1].IsUint() )
            {
                if( jsonValue[0].GetUint() > 65535 || jsonValue[1].GetUint() > 65535 )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Error at file " + filename +
                                     ":\n"
                                     "Root Layout descriptors must be in range [0; 65535]",
                                 "VulkanRootLayout::parseSet" );
                }

                mDescBindingRanges[setIdx][i].start = static_cast<uint16>( jsonValue[0].GetUint() );
                mDescBindingRanges[setIdx][i].end = static_cast<uint16>( jsonValue[1].GetUint() );

                if( mDescBindingRanges[setIdx][i].start > mDescBindingRanges[setIdx][i].end )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Error at file " + filename +
                                     ":\n"
                                     "Root Layout descriptors must satisfy start < end",
                                 "VulkanRootLayout::parseSet" );
                }
            }
        }
    }
    //-------------------------------------------------------------------------
    void VulkanRootLayout::parseRootLayout( const char *rootLayout, const bool bCompute,
                                            const String &filename )
    {
        OGRE_ASSERT_LOW( !mRootLayout && "Cannot call parseRootLayout after createVulkanHandles" );

        mCompute = bCompute;

        rapidjson::Document d;
        d.Parse( rootLayout );

        if( d.HasParseError() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Invalid JSON string in file " + filename + " at line " +
                             StringConverter::toString( d.GetErrorOffset() ) +
                             " Reason: " + rapidjson::GetParseError_En( d.GetParseError() ),
                         "VulkanRootLayout::parseRootLayout" );
        }

        char tmpBuffer[16];
        LwString tmpStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        for( size_t i = 0u; i < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
                mDescBindingRanges[i][j] = VulkanDescBindingRange();

            tmpStr.clear();
            tmpStr.a( static_cast<uint32_t>( i ) );

            rapidjson::Value::ConstMemberIterator itor;

            itor = d.FindMember( tmpStr.c_str() );
            if( itor != d.MemberEnd() && itor->value.IsObject() )
                parseSet( itor->value, i, filename );
        }

        validate( filename );
    }
    //-------------------------------------------------------------------------
    const char c_bufferTypes[] = "BTtsuU";
    void VulkanRootLayout::generateRootLayoutMacros( String &outString ) const
    {
        String macroStr;
        macroStr.swap( outString );

        char tmpBuffer[256];
        LwString textStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        uint32 bindingCounts[VulkanDescBindingTypes::NumDescBindingTypes];
        memset( bindingCounts, 0, sizeof( bindingCounts ) );

        textStr.a( "#define ogre_" );
        const size_t prefixSize0 = textStr.size();

        for( size_t i = 0u; i < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
            {
                textStr.resize( prefixSize0 );
                textStr.aChar( c_bufferTypes[j] );
                const size_t prefixSize1 = textStr.size();

                const size_t numSlots = mDescBindingRanges[i][j].getNumUsedSlots();
                for( size_t k = 0u; k < numSlots; ++k )
                {
                    textStr.resize( prefixSize1 );  // #define ogre_B
                    // #define ogre_B3 set = 1, binding = 6
                    textStr.a( bindingCounts[j], " set = ", (uint32)i, ", binding = ", (uint32)k );
                    ++bindingCounts[j];

                    macroStr += textStr.c_str();
                }
            }
        }
        macroStr.swap( outString );
    }
    //-------------------------------------------------------------------------
    size_t VulkanRootLayout::calculateNumUsedSets( void ) const
    {
        size_t numSets = 0u;
        for( size_t i = 0u; i < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
            {
                if( mDescBindingRanges[i][j].isInUse() )
                    numSets = i;
            }
        }
        return numSets;
    }
    //-------------------------------------------------------------------------
    size_t VulkanRootLayout::calculateNumBindings( const size_t setIdx ) const
    {
        size_t numBindings = 0u;
        for( size_t i = 0u; i < VulkanDescBindingTypes::NumDescBindingTypes; ++i )
            numBindings += mDescBindingRanges[setIdx][i].getNumUsedSlots();

        return numBindings;
    }
    //-------------------------------------------------------------------------
    VkPipelineLayout VulkanRootLayout::createVulkanHandles( void )
    {
        if( mRootLayout )
            return mRootLayout;

        FastArray<FastArray<VkDescriptorSetLayoutBinding> > rootLayoutDesc;

        const size_t numSets = calculateNumUsedSets();
        rootLayoutDesc.resize( numSets );
        mSets.resize( numSets );

        for( size_t i = 0u; i < numSets; ++i )
        {
            rootLayoutDesc[i].resize( calculateNumBindings( i ) );

            size_t bindingIdx = 0u;

            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
            {
                const size_t numSlots = mDescBindingRanges[i][j].getNumUsedSlots();
                for( size_t k = 0u; k < numSlots; ++k )
                {
                    rootLayoutDesc[i][bindingIdx].binding = static_cast<uint32_t>( bindingIdx );
                    rootLayoutDesc[i][bindingIdx].descriptorType = toVkDescriptorType(
                        static_cast<VulkanDescBindingTypes::VulkanDescBindingTypes>( j ) );
                    rootLayoutDesc[i][bindingIdx].descriptorCount = 1u;
                    rootLayoutDesc[i][bindingIdx].stageFlags =
                        mCompute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;
                    rootLayoutDesc[i][bindingIdx].pImmutableSamplers = 0;
                }

                ++bindingIdx;
            }

            mSets[i] = mProgramManager->getCachedSet( rootLayoutDesc[i] );
        }

        VulkanDevice *device = mProgramManager->getDevice();

        VkPipelineLayoutCreateInfo pipelineLayoutCi;
        makeVkStruct( pipelineLayoutCi, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO );
        pipelineLayoutCi.setLayoutCount = static_cast<uint32>( numSets );
        pipelineLayoutCi.pSetLayouts = mSets.begin();

        VkResult result = vkCreatePipelineLayout( device->mDevice, &pipelineLayoutCi, 0, &mRootLayout );
        checkVkResult( result, "vkCreatePipelineLayout" );

        return mRootLayout;
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

        for( size_t i = 0u; i < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            VulkanRootLayout *other = 0;

            bool bDiverged = false;
            size_t numSlotsA = 0u;
            size_t numSlotsB = 0u;
            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
            {
                numSlotsA += a->mDescBindingRanges[i][j].getNumUsedSlots();
                numSlotsB += b->mDescBindingRanges[i][j].getNumUsedSlots();

                if( !bDiverged )
                {
                    if( numSlotsA != numSlotsB )
                    {
                        VulkanRootLayout *newBest = 0;
                        if( numSlotsA > numSlotsB )
                            newBest = a;
                        else
                            newBest = b;

                        if( best != newBest )
                        {
                            // This is the first divergence within this set idx
                            // However a previous set diverged; and the 'best' one
                            // is not this time's winner.
                            //
                            // a and b are incompatible
                            return 0;
                        }

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
        for( size_t i = 0u; i < OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++i )
        {
            for( size_t j = 0u; j < VulkanDescBindingTypes::NumDescBindingTypes; ++j )
            {
                if( this->mDescBindingRanges[i][j].start != other.mDescBindingRanges[i][j].start )
                    return this->mDescBindingRanges[i][j].start < other.mDescBindingRanges[i][j].start;
                if( this->mDescBindingRanges[i][j].end != other.mDescBindingRanges[i][j].end )
                    return this->mDescBindingRanges[i][j].end < other.mDescBindingRanges[i][j].end;
            }
        }

        // If we're here then a and b are equals, thus a < b returns false
        return false;
    }
}  // namespace Ogre
