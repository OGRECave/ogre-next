/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2015 Torus Knot Software Ltd

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
#include "OgreVulkanDeviceResource.h"

#include "ogrestd/vector.h"

namespace Ogre
{
    VulkanDeviceResource::VulkanDeviceResource()
    {
        VulkanDeviceResourceManager::get()->notifyResourceCreated( this );
    }

    VulkanDeviceResource::~VulkanDeviceResource()
    {
        VulkanDeviceResourceManager::get()->notifyResourceDestroyed( this );
    }

    // ------------------------------------------------------------------------
    static VulkanDeviceResourceManager *gs_VulkanDeviceResourceManager = nullptr;

    VulkanDeviceResourceManager *VulkanDeviceResourceManager::get()
    {
        return gs_VulkanDeviceResourceManager;
    }

    VulkanDeviceResourceManager::VulkanDeviceResourceManager()
    {
        assert( gs_VulkanDeviceResourceManager == nullptr );
        gs_VulkanDeviceResourceManager = this;
    }

    VulkanDeviceResourceManager::~VulkanDeviceResourceManager()
    {
        assert( mResources.empty() );
        assert( gs_VulkanDeviceResourceManager == this );
        gs_VulkanDeviceResourceManager = nullptr;
    }

    void VulkanDeviceResourceManager::notifyResourceCreated( VulkanDeviceResource *deviceResource )
    {
        std::lock_guard<std::recursive_mutex> guard( mResourcesMutex );

        assert( std::find( mResources.begin(), mResources.end(), deviceResource ) == mResources.end() );
        mResources.push_back( deviceResource );
    }

    void VulkanDeviceResourceManager::notifyResourceDestroyed( VulkanDeviceResource *deviceResource )
    {
        std::lock_guard<std::recursive_mutex> guard( mResourcesMutex );

        vector<VulkanDeviceResource *>::type::iterator it =
            std::find( mResources.begin(), mResources.end(), deviceResource );
        assert( it != mResources.end() );
        mResources.erase( it );

        vector<VulkanDeviceResource *>::type::iterator itCopy =
            std::find( mResourcesCopy.begin(), mResourcesCopy.end(), deviceResource );
        if( itCopy != mResourcesCopy.end() )
            *itCopy = NULL;
    }

    void VulkanDeviceResourceManager::notifyDeviceLost()
    {
        std::lock_guard<std::recursive_mutex> guard( mResourcesMutex );

        assert( mResourcesCopy.empty() );  // reentrancy is not expected nor supported
        mResourcesCopy = mResources;

        for( VulkanDeviceResource *deviceResource : mResourcesCopy )
            if( deviceResource != nullptr )
                deviceResource->notifyDeviceLost();  // notifyResourceDestroyed() can be called inside

        mResourcesCopy.clear();
    }

    void VulkanDeviceResourceManager::notifyDeviceRestored()
    {
        std::lock_guard<std::recursive_mutex> guard( mResourcesMutex );

        assert( mResourcesCopy.empty() );  // reentrancy is not expected nor supported
        mResourcesCopy = mResources;

        for( unsigned pass = 0; pass < 2; ++pass )
            for( VulkanDeviceResource *deviceResource : mResourcesCopy )
                if( deviceResource != nullptr )
                    deviceResource->notifyDeviceRestored( pass );  // subresources could be created

        mResourcesCopy.clear();
    }

}  // namespace Ogre
