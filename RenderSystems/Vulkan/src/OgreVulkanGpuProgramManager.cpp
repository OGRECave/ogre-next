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

#include "OgreVulkanGpuProgramManager.h"

#include "OgreVulkanDevice.h"
#include "OgreVulkanRootLayout.h"
#include "OgreVulkanUtils.h"

#include "OgreLogManager.h"
// #include "OgreVulkanProgram.h"

namespace Ogre
{
    bool operator<( const VkDescriptorSetLayoutBinding &a, const VkDescriptorSetLayoutBinding &b )
    {
        if( a.binding != b.binding )
            return a.binding < b.binding;
        if( a.descriptorType != b.descriptorType )
            return a.descriptorType < b.descriptorType;
        if( a.descriptorCount != b.descriptorCount )
            return a.descriptorCount < b.descriptorCount;
        if( a.stageFlags != b.stageFlags )
            return a.stageFlags < b.stageFlags;
        return a.pImmutableSamplers < b.pImmutableSamplers;
    }

    bool operator!=( const VkDescriptorSetLayoutBinding &a, const VkDescriptorSetLayoutBinding &b )
    {
        return a.binding != b.binding ||                                                  //
               a.descriptorType != b.descriptorType ||                                    //
               a.descriptorCount != b.descriptorCount || a.stageFlags != b.stageFlags ||  //
               a.pImmutableSamplers < b.pImmutableSamplers;
    }

    bool operator<( const VulkanSingleSetLayoutDesc &a, const VulkanSingleSetLayoutDesc &b )
    {
        const size_t aSize = a.size();
        const size_t bSize = b.size();
        if( aSize != bSize )
            return aSize < bSize;

        for( size_t i = 0u; i < aSize; ++i )
        {
            if( a[i] != b[i] )
                return a[i] < b[i];
        }

        // If we're here then a and b are equals, thus a < b returns false
        return false;
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    VulkanGpuProgramManager::VulkanGpuProgramManager( VulkanDevice *device ) :
        mTmpRootLayout( new VulkanRootLayout( this ) ),
        mDevice( device )
    {
        // Superclass sets up members

        // Register with resource group manager
        ResourceGroupManager::getSingleton()._registerResourceManager( mResourceType, this );
    }
    //-------------------------------------------------------------------------
    VulkanGpuProgramManager::~VulkanGpuProgramManager()
    {
        delete mTmpRootLayout;
        mTmpRootLayout = 0;

        {
            VulkanRootLayoutSet::const_iterator itor = mRootLayouts.begin();
            VulkanRootLayoutSet::const_iterator endt = mRootLayouts.end();

            while( itor != endt )
            {
                delete *itor;
                ++itor;
            }

            mRootLayouts.clear();
        }

        destroyDescriptorSetLayouts();

        // Unregister with resource group manager
        ResourceGroupManager::getSingleton()._unregisterResourceManager( mResourceType );
    }
    //-------------------------------------------------------------------------
    void VulkanGpuProgramManager::destroyDescriptorSetLayouts()
    {
        DescriptorSetMap::const_iterator itor = mDescriptorSetMap.begin();
        DescriptorSetMap::const_iterator endt = mDescriptorSetMap.end();

        while( itor != endt )
        {
            vkDestroyDescriptorSetLayout( mDevice->mDevice, itor->second, 0 );
            ++itor;
        }

        mDescriptorSetMap.clear();
    }
    //-------------------------------------------------------------------------
    void VulkanGpuProgramManager::notifyDeviceLost() { destroyDescriptorSetLayouts(); }
    //-------------------------------------------------------------------------
    void VulkanGpuProgramManager::notifyDeviceRestored( unsigned pass ) {}
    //-------------------------------------------------------------------------
    bool VulkanGpuProgramManager::registerProgramFactory( const String &syntaxCode,
                                                          CreateGpuProgramCallback createFn )
    {
        return mProgramMap.insert( ProgramMap::value_type( syntaxCode, createFn ) ).second;
    }
    //-------------------------------------------------------------------------
    bool VulkanGpuProgramManager::unregisterProgramFactory( const String &syntaxCode )
    {
        return mProgramMap.erase( syntaxCode ) != 0;
    }
    //-------------------------------------------------------------------------
    VkDescriptorSetLayout VulkanGpuProgramManager::getCachedSet( const VulkanSingleSetLayoutDesc &set )
    {
        ScopedLock lock( mMutexRootLayouts );
        VkDescriptorSetLayout retVal = 0;

        DescriptorSetMap::const_iterator itor = mDescriptorSetMap.find( set );

        if( itor == mDescriptorSetMap.end() )
        {
            VkDescriptorSetLayoutCreateInfo descSetLayoutCi;
            makeVkStruct( descSetLayoutCi, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO );
            descSetLayoutCi.bindingCount = static_cast<uint32>( set.size() );
            descSetLayoutCi.pBindings = set.begin();

            VkResult result =
                vkCreateDescriptorSetLayout( mDevice->mDevice, &descSetLayoutCi, 0, &retVal );
            checkVkResult( mDevice, result, "vkCreateDescriptorSetLayout" );
            mDescriptorSetMap[set] = retVal;
        }
        else
        {
            retVal = itor->second;
        }

        return retVal;
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout *VulkanGpuProgramManager::getRootLayout( const RootLayout &rootLayout )
    {
        ScopedLock lock( mMutexRootLayouts );
        VulkanRootLayout *retVal = 0;

        mTmpRootLayout->copyFrom( rootLayout );

        VulkanRootLayoutSet::const_iterator itor = mRootLayouts.find( mTmpRootLayout );

        if( itor == mRootLayouts.end() )
        {
            retVal = mTmpRootLayout;
            mRootLayouts.insert( mTmpRootLayout );
            mTmpRootLayout = new VulkanRootLayout( this );
        }
        else
        {
            retVal = *itor;
        }

        return retVal;
    }
    //-------------------------------------------------------------------------
    VulkanRootLayout *VulkanGpuProgramManager::getRootLayout( const char *rootLayout,
                                                              const bool bCompute,
                                                              const String &filename )
    {
        ScopedLock lock( mMutexRootLayouts );
        VulkanRootLayout *retVal = 0;

        mTmpRootLayout->parseRootLayout( rootLayout, bCompute, filename );

        VulkanRootLayoutSet::const_iterator itor = mRootLayouts.find( mTmpRootLayout );

        if( itor == mRootLayouts.end() )
        {
            retVal = mTmpRootLayout;
            mRootLayouts.insert( mTmpRootLayout );
            mTmpRootLayout = new VulkanRootLayout( this );
        }
        else
        {
            retVal = *itor;
        }

        return retVal;
    }
    //-------------------------------------------------------------------------
    Resource *VulkanGpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                   const String &group, bool isManual,
                                                   ManualResourceLoader *loader,
                                                   const NameValuePairList *params )
    {
        NameValuePairList::const_iterator paramSyntax, paramType;

        if( !params || ( paramSyntax = params->find( "syntax" ) ) == params->end() ||
            ( paramType = params->find( "type" ) ) == params->end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "You must supply 'syntax' and 'type' parameters",
                         "VulkanGpuProgramManager::createImpl" );
        }

        ProgramMap::const_iterator iter = mProgramMap.find( paramSyntax->second );
        if( iter == mProgramMap.end() )
        {
            // No factory, this is an unsupported syntax code, probably for another rendersystem
            // Create a basic one, it doesn't matter what it is since it won't be used
            // return new VulkanProgram( this, name, handle, group, isManual, loader, mDevice );
            return 0;
        }

        GpuProgramType gpt;
        if( paramType->second == "vertex_program" )
        {
            gpt = GPT_VERTEX_PROGRAM;
        }
        else
        {
            gpt = GPT_FRAGMENT_PROGRAM;
        }

        return ( iter->second )( this, name, handle, group, isManual, loader, gpt, paramSyntax->second );
    }
    //-------------------------------------------------------------------------
    Resource *VulkanGpuProgramManager::createImpl( const String &name, ResourceHandle handle,
                                                   const String &group, bool isManual,
                                                   ManualResourceLoader *loader, GpuProgramType gptype,
                                                   const String &syntaxCode )
    {
        ProgramMap::const_iterator iter = mProgramMap.find( syntaxCode );
        if( iter == mProgramMap.end() )
        {
            // No factory, this is an unsupported syntax code, probably for another rendersystem
            // Create a basic one, it doesn't matter what it is since it won't be used
            // return new VulkanProgram( this, name, handle, group, isManual, loader, mDevice );
            return 0;
        }

        return ( iter->second )( this, name, handle, group, isManual, loader, gptype, syntaxCode );
    }
    //-------------------------------------------------------------------------
    bool VulkanGpuProgramManager::SortByVulkanRootLayout::operator()( const VulkanRootLayout *a,
                                                                      const VulkanRootLayout *b ) const
    {
        return *a < *b;
    }
}  // namespace Ogre
