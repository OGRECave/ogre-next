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
        bool isValid( void ) const { return start <= end; }
    };

    namespace VulkanDescBindingTypes
    {
        /// The order is important as it affects compatibility between root layouts
        ///
        /// If the relative order is changed, then the following needs to be modified:
        ///     - VulkanRootLayout::bind
        ///     - VulkanRootLayout::findParamsBuffer (if ParamBuffer stops being the 1st)
        ///     - c_rootLayoutVarNames
        ///     - c_bufferTypes
        enum VulkanDescBindingTypes
        {
            ParamBuffer,
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
        uint8 mParamsBuffStages;
        VulkanDescBindingRange mDescBindingRanges[OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS]
                                                 [VulkanDescBindingTypes::NumDescBindingTypes];

        /// One handle per binding set (up to OGRE_VULKAN_MAX_NUM_BOUND_DESCRIPTOR_SETS)
        /// Doesn't have gaps (e.g. if mDescBindingRanges[3] is not empty, then mSets[3] must exist)
        /// Won't be initialized if this VulkanRootLayout never makes it into a PSO.
        FastArray<VkDescriptorSetLayout> mSets;

        /// The Vulkan Handle to our root layout. Won't be initialized if this
        /// VulkanRootLayout never makes it into a PSO.
        VkPipelineLayout mRootLayout;

        /// There's one VulkanDescriptorPool per binding set
        FastArray<VulkanDescriptorPool *> mPools;

        VulkanGpuProgramManager *mProgramManager;

        void validate( const String &filename ) const;
        void parseSet( const rapidjson::Value &jsonValue, const size_t setIdx, const String &filename );

        size_t calculateNumUsedSets( void ) const;
        size_t calculateNumBindings( const size_t setIdx ) const;

        inline void bindCommon( VkWriteDescriptorSet &writeDescSet, size_t &numWriteDescSets,
                                uint32 &currBinding, VkDescriptorSet descSet,
                                const VulkanDescBindingRange &bindRanges );
        inline void bindParamsBuffer( VkWriteDescriptorSet *writeDescSets, size_t &numWriteDescSets,
                                      uint32 &currBinding, VkDescriptorSet descSet,
                                      const VulkanDescBindingRange *descBindingRanges,
                                      const VulkanGlobalBindingTable &table );
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

        /** Parses a root layout definition from a JSON string
            The JSON string:

            @code
                {
                    "0" :
                    {
                        "has_params" : ["all", "vs", "gs", "hs", "ds", "ps", "cs"],
                        "const_buffers" : [0, 16],
                        "tex_buffers" : [1, 16],
                        "textures" : [0, 1],
                        "samplers" : [0, 1],
                        "uav_buffers" : [0, 16]
                        "uav_textures" : [16, 32]
                    }
                }
            @endcode

            has_params can establish which shader stages allow parameters.
                - "all" means all shader stages (vs through ps for graphics, cs for compute)

            Note that for compatibility with other APIs, textures and tex_buffers cannot overlap
            Same with uav_buffers and uav_textures
        @param rootLayout
            JSON string containing root layout
        @param bCompute
            True if this is meant for compute. False for graphics
        @param filename
            Filename for logging purposes if errors are found
        */
        void parseRootLayout( const char *rootLayout, const bool bCompute, const String &filename );

        /** Generates all the macros for compiling shaders, based on our layout

            e.g. a layout like this:

            @code
                ## ROOT LAYOUT BEGIN
                {
                    "0" :
                    {
                        "has_params" : true
                        "const_buffers" : [2, 4]
                        "tex_buffers" : [0, 1]
                        "samplers" : [1, 2],
                        "textures" : [1, 2]
                    }
                }
                ## ROOT LAYOUT END
            @endcode

            will generate the following:
            @code
                #define ogre_P0 set = 0, binding = 0 // Params buffer
                #define ogre_B2 set = 0, binding = 1 // Const buffer at slot 2
                #define ogre_B3 set = 0, binding = 2
                #define ogre_T0 set = 0, binding = 3 // Tex buffer at slot 0
                #define ogre_t1 set = 0, binding = 4 // Texture at slot 1
                                                     // (other APIs share tex buffer & texture slots)
                #define ogre_s1 set = 0, binding = 5 // Sampler at slot 1
            @endcode
        @param shaderStage
            See GpuProgramType
        @param inOutString [in/out]
            String to output our macros
        */
        void generateRootLayoutMacros( uint32 shaderStage, String &inOutString ) const;

        /** Creates most of our Vulkan handles required to build a PSO.

            This function is not called by parseRootLayout because if two Root Layouts are identical,
            then after calling a->parseRootLayout(), instead of calling a->createVulkanHandles()
            we first must look for an already existing VulkanRootLayout and reuse it.
        @return
            VkPipelineLayout handle for building the PSO.
        */
        VkPipelineLayout createVulkanHandles( void );

        /** Takes an emulated D3D11/Metal-style table and binds it according to this layout's rules

            This updates N descriptors (1 for each set) and binds them
        @param device
        @param vaoManager
            The VaoManager so we can grab new VulkanDescriptorPools shall we need them
        @param table
            The emulated table to bind it
        */
        void bind( VulkanDevice *device, VulkanVaoManager *vaoManager,
                   const VulkanGlobalBindingTable &table );

        /** Retrieves the set and binding idx of the params buffer
        @param shaderStage
            See GpuProgramType
        @param outSetIdx [out]
            Set in which it is located
            Value will not be modified if we return false
        @param outBindingIdx [out]
            Binding index in which it is located
            Value will not be modified if we return false
        @return
            True if there is a params buffer
            False otherwise and output params won't be modified
        */
        bool findParamsBuffer( uint32 shaderStage, size_t &outSetIdx, size_t &outBindingIdx ) const;

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
