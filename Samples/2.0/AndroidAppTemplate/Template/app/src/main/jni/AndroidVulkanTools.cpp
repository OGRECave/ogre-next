
#include "AndroidVulkanTools.h"

#include "OgrePrerequisites.h"
#include "OgreWorkarounds.h"

#include <android/log.h>

#include <dlfcn.h>
#include <vector>

#include "vulkan/vulkan.h"

void testEarlyVulkanWorkarounds()
{
#ifdef OGRE_VK_WORKAROUND_SYNC_WINDOW_INIT
    void *libVulkan = dlopen( "libvulkan.so.1", RTLD_NOW | RTLD_LOCAL );
    if( !libVulkan )
        libVulkan = dlopen( "libvulkan.so", RTLD_NOW | RTLD_LOCAL );
    if( !libVulkan )
        return;

    PFN_vkCreateInstance vkCreateInstance =
        reinterpret_cast<PFN_vkCreateInstance>( dlsym( libVulkan, "vkCreateInstance" ) );
    PFN_vkDestroyInstance vkDestroyInstance =
        reinterpret_cast<PFN_vkDestroyInstance>( dlsym( libVulkan, "vkDestroyInstance" ) );

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DO_NOT_OVERRIDE";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "DO_NOT_OVERRIDE";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Create Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices =
        reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
            dlsym( libVulkan, "vkEnumeratePhysicalDevices" ) );
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties =
        reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
            dlsym( libVulkan, "vkGetPhysicalDeviceProperties" ) );

    VkInstance instance;
    vkCreateInstance( &instanceCreateInfo, nullptr, &instance );

    uint32_t numDevices = 1u;
    vkEnumeratePhysicalDevices( instance, &numDevices, NULL );
    if( numDevices == 0 )
    {
        __android_log_print( ANDROID_LOG_ERROR, "testEarlyVulkanWorkarounds", "NO VK DEVICES!" );
    }
    else
    {
        __android_log_print( ANDROID_LOG_INFO, "testEarlyVulkanWorkarounds", "YES VK DEVICES!" );

        std::vector<VkPhysicalDevice> pd;
        pd.resize( numDevices );
        vkEnumeratePhysicalDevices( instance, &numDevices, pd.data() );

        for( uint32_t i = 0u; i < numDevices; ++i )
        {
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties( pd[i], &deviceProps );

            if( deviceProps.vendorID == 0x5143 )
            {
                // Qualcomm.
                const uint32_t knownGoodQualcommVersion = VK_MAKE_API_VERSION( 0, 512, 502, 0 );
                if( deviceProps.driverVersion < knownGoodQualcommVersion )
                    Ogre::Workarounds::mVkSyncWindowInit = true;
            }
            else if( deviceProps.vendorID == 0x13B5 )
            {
                // ARM / Mali. We know at least version 21.0.0 is bad.
                const uint32_t knownBadArmVersion = VK_MAKE_API_VERSION( 0, 21, 0, 0 );
                if( deviceProps.driverVersion <= knownBadArmVersion )
                    Ogre::Workarounds::mVkSyncWindowInit = true;
                else
                {
                    const int apiLevel = android_get_device_api_level();
                    if( apiLevel > 0 && apiLevel <= 28 )
                    {
                        // Always broken. Older drivers used a driver scheme 8xx.x.x and 9xx.x.x
                        // which would pass the previous checks. But they're actually ancient.
                        Ogre::Workarounds::mVkSyncWindowInit = true;
                    }
                }
            }
            else if( deviceProps.vendorID == 0x1010 )
            {
                // IMGTEC / PowerVR. We strongly suspect at least version 1.300.2589.0 is bad.
                // Version 1.386.1368 is good and is the most popular one.
                const uint32_t knownGoodVersion = VK_MAKE_API_VERSION( 0, 1, 386, 1368 );
                if( deviceProps.driverVersion < knownGoodVersion )
                    Ogre::Workarounds::mVkSyncWindowInit = true;
            }
        }
    }

    vkDestroyInstance( instance, nullptr );
    instance = 0;
#endif
}
