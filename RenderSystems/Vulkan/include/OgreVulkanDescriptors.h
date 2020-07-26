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
#ifndef _OgreVulkanDescriptors_H_
#define _OgreVulkanDescriptors_H_

#include "OgreVulkanPrerequisites.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    typedef FastArray<FastArray<VkDescriptorSetLayoutBinding> > DescriptorSetLayoutBindingArray;

    typedef FastArray<VkDescriptorSetLayout> DescriptorSetLayoutArray;

    class _OgreVulkanExport VulkanDescriptors
    {
    protected:
        static bool areBindingsCompatible( const VkDescriptorSetLayoutBinding &a,
                                           const VkDescriptorSetLayoutBinding &b );
        static bool canMergeDescriptorSets( const DescriptorSetLayoutBindingArray &a,
                                            const DescriptorSetLayoutBindingArray &b );

    public:
        /// Merges b into a. Raises an exception if these two cannot be merged
        /// There's no 'shaderA' string because a could be an amalgamation of stuff
        static void mergeDescriptorSets( DescriptorSetLayoutBindingArray &a, const String &shaderB,
                                         const DescriptorSetLayoutBindingArray &b );

        /// Uses SPIRV reflection to generate Descriptor Sets from the given SPIRV binary.
        /// Empty bindings are included as... empty. This means that if there are gaps, these gaps
        /// will be included in outputSets
        static void generateDescriptorSets( const String &shaderName, const std::vector<uint32> &spirv,
                                            DescriptorSetLayoutBindingArray &outputSets );

        static void generateAndMergeDescriptorSets( VulkanProgram *shader,
                                                    DescriptorSetLayoutBindingArray &outputSets );

        /// Removes all empty bindings. Do this **after** the last mergeDescriptorSets call
        /// This operation can cause gaps to appear in the bindings.
        static void optimizeDescriptorSets( DescriptorSetLayoutBindingArray &sets );

        static VkPipelineLayout generateVkDescriptorSets(
            const DescriptorSetLayoutBindingArray &bindingSets, DescriptorSetLayoutArray &sets );
    };
}  // namespace Ogre

#endif  // __VulkanProgram_H__
