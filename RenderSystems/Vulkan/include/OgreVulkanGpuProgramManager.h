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

#ifndef _OgreVulkanGpuProgramManager_H_
#define _OgreVulkanGpuProgramManager_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreVulkanDeviceResource.h"
#include "OgreGpuProgramManager.h"

#include "vulkan/vulkan_core.h"

namespace Ogre
{
    class VulkanProgram;

    // Sets' descriptor
    typedef FastArray<VkDescriptorSetLayoutBinding> VulkanSingleSetLayoutDesc;

    bool operator<( const VulkanSingleSetLayoutDesc &a, const VulkanSingleSetLayoutDesc &b );

    class _OgreVulkanExport VulkanGpuProgramManager final : public GpuProgramManager,
                                                            protected VulkanDeviceResource
    {
    public:
        typedef GpuProgram *( *CreateGpuProgramCallback )( ResourceManager *creator, const String &name,
                                                           ResourceHandle handle, const String &group,
                                                           bool isManual, ManualResourceLoader *loader,
                                                           GpuProgramType gptype,
                                                           const String &syntaxCode );

    private:
        struct SortByVulkanRootLayout
        {
            bool operator()( const VulkanRootLayout *a, const VulkanRootLayout *b ) const;
        };

        typedef map<String, CreateGpuProgramCallback>::type ProgramMap;
        ProgramMap mProgramMap;

        typedef map<VulkanSingleSetLayoutDesc, VkDescriptorSetLayout>::type DescriptorSetMap;
        LightweightMutex mMutexDescriptorSet;
        DescriptorSetMap mDescriptorSetMap;  // GUARDED_BY( mMutexDescriptorSet )

        typedef set<VulkanRootLayout *, SortByVulkanRootLayout>::type VulkanRootLayoutSet;
        LightweightMutex mMutexRootLayouts;
        VulkanRootLayoutSet mRootLayouts;  // GUARDED_BY( mMutexRootLayouts )

        VulkanRootLayout *mTmpRootLayout;  // GUARDED_BY( mMutexRootLayouts )

        /// Guards VulkanRootLayout::createVulkanHandles
        LightweightMutex mMutexRootLayoutHandles;

        VulkanDevice *mDevice;

    protected:
        /// @copydoc ResourceManager::createImpl
        Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                              bool isManual, ManualResourceLoader *loader,
                              const NameValuePairList *createParams ) override;
        /// Specialised create method with specific parameters
        Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                              bool isManual, ManualResourceLoader *loader, GpuProgramType gptype,
                              const String &syntaxCode ) override;

        void destroyDescriptorSetLayouts();

        void notifyDeviceLost() override;
        void notifyDeviceRestored( unsigned pass ) override;

    public:
        VulkanGpuProgramManager( VulkanDevice *device );
        ~VulkanGpuProgramManager() override;

        bool registerProgramFactory( const String &syntaxCode, CreateGpuProgramCallback createFn );
        bool unregisterProgramFactory( const String &syntaxCode );

        VulkanDevice *getDevice() const { return mDevice; }

        VkDescriptorSetLayout getCachedSet( const VulkanSingleSetLayoutDesc &set );

        /// Finds a cached root layout from a programmatically-generated structure,
        /// creates a new one if not found.
        ///
        /// Assumes RootLayout::validate has already been called
        ///
        /// @see    RootLayout::parseRootLayout
        VulkanRootLayout *getRootLayout( const RootLayout &rootLayout );

        /// Finds a cached root layout from the given JSON data, creates a new one if not found.
        ///
        /// filename is only for error/debugging purposes
        ///
        /// @see    RootLayout::parseRootLayout
        VulkanRootLayout *getRootLayout( const char *rootLayout, const bool bCompute,
                                         const String &filename );

        LightweightMutex &getMutexRootLayoutHandles() { return mMutexRootLayoutHandles; }
    };
}  // namespace Ogre

#endif
