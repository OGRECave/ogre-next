
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

            // Qualcomm.
            if( deviceProps.vendorID == 0x5143 )
            {
                const uint32_t knownGoodQualcommVersion = VK_MAKE_API_VERSION( 0, 512, 502, 0 );
                if( deviceProps.driverVersion < knownGoodQualcommVersion )
                    Ogre::Workarounds::mVkSyncWindowInit = true;
            }
        }
    }

    vkDestroyInstance( instance, nullptr );
    instance = 0;
#endif
}
