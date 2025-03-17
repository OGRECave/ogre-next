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
#ifndef _OgreVulkanDeviceResource_H_
#define _OgreVulkanDeviceResource_H_

#include "OgreVulkanPrerequisites.h"

#include <mutex>
#include "ogrestd/vector.h"

namespace Ogre
{
    /** Represents a Vulkan device depended rendering resource.
    Provide unified interface to handle various device states.
    This class is intended to be used as protected base.
    */
    class VulkanDeviceResource
    {
    public:
        // Called immediately after the Vulkan device is lost.
        // This is the place to release device depended resources.
        virtual void notifyDeviceLost() = 0;

        // Called immediately after the Vulkan device has been recreated.
        // This is the place to create device depended resources.
        virtual void notifyDeviceRestored( unsigned pass ) = 0;

    protected:
        VulkanDeviceResource();
        ~VulkanDeviceResource();  // protected and non-virtual
    };

    /** Singleton that is used to propagate device state changed notifications.
    This class is intended to be used as protected base.
    */
    class VulkanDeviceResourceManager
    {
    public:
        void notifyResourceCreated( VulkanDeviceResource *deviceResource );
        void notifyResourceDestroyed( VulkanDeviceResource *deviceResource );

        void notifyDeviceLost();
        void notifyDeviceRestored();

        static VulkanDeviceResourceManager *get();

    protected:
        VulkanDeviceResourceManager();
        ~VulkanDeviceResourceManager();  // protected and non-virtual

    private:
        std::recursive_mutex mResourcesMutex;

        // Originally the order in which resources were restored mattered (e.g. Meshes being
        // destroyed before their vertex & index buffers).
        // If anyone wants to change this vector for something else (e.g. a hashset), make
        // sure to test it extensively to see if order no longer matters. And if so, a more robust
        // system would be needed to handle these order dependencies.
        vector<VulkanDeviceResource *>::type mResources, mResourcesCopy;
    };

}  // namespace Ogre
#endif
