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
#ifndef _OgreVulkanRootLayout_H_
#define _OgreVulkanRootLayout_H_

#include "OgreVulkanPrerequisites.h"

struct VkDescriptorSetLayoutBinding;
struct VkWriteDescriptorSet;

// Forward declaration for |Document|.
namespace rapidjson
{
    class CrtAllocator;
    template <typename>
    class MemoryPoolAllocator;
    template <typename>
    struct UTF8;

    template <typename BaseAllocator>
    class MemoryPoolAllocator;
    template <typename Encoding, typename>
    class GenericValue;
    typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;
}  // namespace rapidjson

namespace Ogre
{
#define OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS 4
    struct VulkanDescBindingRange
    {
        uint16 start;  // Inclusive
        uint16 end;    // Exclusive
        VulkanDescBindingRange();

        size_t getNumUsedSlots( void ) const { return end - start; }
        bool isInUse( void ) const { return start < end; }
    };

    namespace VulkanDescBindingTypes
    {
        /// The order is important as it affects compatibility between root layouts
        ///
        /// If the relative order is changed, then VulkanRootLayout::bind needs to be modified
        enum VulkanDescBindingTypes
        {
            ConstBuffer,
            TexBuffer,
            Texture,
            Sampler,
            UavTexture,
            UavBuffer,
            NumDescBindingTypes
        };
    }  // namespace VulkanDescBindingTypes

    uint32 toVkDescriptorType( VulkanDescBindingTypes::VulkanDescBindingTypes type );

    class _OgreVulkanExport VulkanRootLayout : public ResourceAlloc
    {
        bool mCompute;
        VulkanDescBindingRange mDescBindingRanges[OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS]
                                                 [VulkanDescBindingTypes::NumDescBindingTypes];

        /// One handle per binding set (up to OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS)
        /// Doesn't have gaps (e.g. if mDescBindingRanges[3] is not empty, then mSets[3] must exist)
        /// Won't be initialized if this VulkanRootLayout never makes it into a PSO.
        FastArray<VkDescriptorSetLayout> mSets;

        /// The Vulkan Handle to our root layout. Won't be initialized if this
        /// VulkanRootLayout never makes it into a PSO.
        VkPipelineLayout mRootLayout;

        /// There's one VulkanDescriptorPool per binding set, per frame index
        /// mPools[setIdx][frameNum]
        FastArray<VulkanDescriptorPool> mPools[OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS];

        VulkanGpuProgramManager *mProgramManager;

        void validate( const String &filename ) const;
        void parseSet( const rapidjson::Value &jsonValue, const size_t setIdx, const String &filename );

        size_t calculateNumUsedSets( void ) const;
        size_t calculateNumBindings( const size_t setIdx ) const;

        inline void bindCommon( VkWriteDescriptorSet &writeDescSet, size_t &numWriteDescSets,
                                uint32 &currBinding, VkDescriptorSet descSet,
                                const VulkanDescBindingRange &bindRanges );
        inline void bindConstBuffers( VkWriteDescriptorSet *writeDescSets, size_t &numWriteDescSets,
                                      uint32 &currBinding, VkDescriptorSet descSet,
                                      const VulkanDescBindingRange *descBindingRanges,
                                      const VulkanGlobalBindingTable &table );
        inline void bindTexBuffers( VkWriteDescriptorSet *writeDescSets, size_t &numWriteDescSets,
                                    uint32 &currBinding, VkDescriptorSet descSet,
                                    const VulkanDescBindingRange *descBindingRanges,
                                    const VulkanGlobalBindingTable &table );
        inline void bindTextures( VkWriteDescriptorSet *writeDescSets, size_t &numWriteDescSets,
                                  uint32 &currBinding, VkDescriptorSet descSet,
                                  const VulkanDescBindingRange *descBindingRanges,
                                  const VulkanGlobalBindingTable &table );
        inline void bindSamplers( VkWriteDescriptorSet *writeDescSets, size_t &numWriteDescSets,
                                  uint32 &currBinding, VkDescriptorSet descSet,
                                  const VulkanDescBindingRange *descBindingRanges,
                                  const VulkanGlobalBindingTable &table );

    public:
        VulkanRootLayout( VulkanGpuProgramManager *programManager );
        ~VulkanRootLayout();

        void parseRootLayout( const char *rootLayout, const bool bCompute, const String &filename );
        void generateRootLayoutMacros( String &outString ) const;

        VkPipelineLayout createVulkanHandles( void );

        void bind( VulkanDevice *device, const VulkanGlobalBindingTable &table );

        /// Two root layouts can be incompatible. If so, we return nullptr
        ///
        /// If a and b are compatible, we return the best one (one that can satisfy both a and b).
        ///
        /// a or b can be nullptr
        static VulkanRootLayout *findBest( VulkanRootLayout *a, VulkanRootLayout *b );

        const VulkanDescBindingRange *getDescBindingRanges( size_t setIdx ) const
        {
            return mDescBindingRanges[setIdx];
        }

        bool operator<( const VulkanRootLayout &other ) const;
    };
}  // namespace Ogre

#endif  // __VulkanProgram_H__
