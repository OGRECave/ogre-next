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

#ifndef _OgreVulkanGpuProgramManager_H_
#define _OgreVulkanGpuProgramManager_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreGpuProgramManager.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    class VulkanProgram;

    // Sets' descriptor
    typedef FastArray<VkDescriptorSetLayoutBinding> VkDescriptorSetLayoutBindingArray;
    // Sets' opaque handles
    typedef FastArray<VkDescriptorSetLayout> DescriptorSetLayoutArray;

    bool operator<( const VkDescriptorSetLayoutBindingArray &a,
                    const VkDescriptorSetLayoutBindingArray &b );
    bool operator<( const DescriptorSetLayoutArray &a, const DescriptorSetLayoutArray &b );

    class _OgreVulkanExport VulkanGpuProgramManager : public GpuProgramManager
    {
    public:
        typedef GpuProgram *( *CreateGpuProgramCallback )( ResourceManager *creator, const String &name,
                                                           ResourceHandle handle, const String &group,
                                                           bool isManual, ManualResourceLoader *loader,
                                                           GpuProgramType gptype,
                                                           const String &syntaxCode );

    private:
        typedef map<String, CreateGpuProgramCallback>::type ProgramMap;
        ProgramMap mProgramMap;

        typedef map<VkDescriptorSetLayoutBindingArray, VkDescriptorSetLayout>::type DescriptorSetMap;
        DescriptorSetMap mDescriptorSetMap;

        typedef map<DescriptorSetLayoutArray, VkPipelineLayout>::type DescriptorSetsVkMap;
        DescriptorSetsVkMap mDescriptorSetsVkMap;

        VulkanDevice *mDevice;

    protected:
        /// @copydoc ResourceManager::createImpl
        Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                              bool isManual, ManualResourceLoader *loader,
                              const NameValuePairList *createParams );
        /// Specialised create method with specific parameters
        Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                              bool isManual, ManualResourceLoader *loader, GpuProgramType gptype,
                              const String &syntaxCode );

    public:
        VulkanGpuProgramManager( VulkanDevice *device );
        virtual ~VulkanGpuProgramManager();
        bool registerProgramFactory( const String &syntaxCode, CreateGpuProgramCallback createFn );
        bool unregisterProgramFactory( const String &syntaxCode );

        VkDescriptorSetLayout getCachedSet( const VkDescriptorSetLayoutBindingArray &set );
        VkPipelineLayout getCachedSets( const DescriptorSetLayoutArray &vkSets );
    };
}  // namespace Ogre

#endif
