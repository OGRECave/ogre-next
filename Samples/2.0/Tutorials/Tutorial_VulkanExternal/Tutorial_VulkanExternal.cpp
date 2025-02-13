
//-----------------------------------------------------------------------------
// See README.md
//-----------------------------------------------------------------------------

#include "OgreAbiUtils.h"
#include "OgreArchiveManager.h"
#include "OgreCamera.h"
#include "OgreConfigFile.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreWindow.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreWindowEventUtilities.h"

#include "OgreVulkanBuildSettings.h"

#include "OgreVulkanDevice.h"

#include <vulkan/vulkan_core.h>
#ifdef OGRE_VULKAN_WINDOW_WIN32
#    define VK_USE_PLATFORM_WIN32_KHR
#    include "vulkan/vulkan_win32.h"
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
#    include <xcb/randr.h>
#    include <xcb/xcb.h>

#    include <vulkan/vulkan_xcb.h>
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
#    include "vulkan/vulkan_android.h"
#endif

// Macros borrowed (modified) from the VulkanRenderSystem since they're convenient
#define MY_VK_EXCEPT( code, num, desc, src ) \
    OGRE_EXCEPT( code, desc + ( "\nVkResult = " + std::to_string( num ) ), src )

#define myCheckVkResult( result, functionName ) \
    do \
    { \
        if( result != VK_SUCCESS ) \
        { \
            MY_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, functionName " failed", \
                          OGRE_CURRENT_FUNCTION ); \
        } \
    } while( 0 )

/**
@brief createVulkanInstance
    Creates a Vulkan instance that is ideal for Ogre Next.

    This follows VulkanRenderSystem::initializeVkInstance quite closely
@param outExternalInstance[out]

*/
static void createVulkanInstance( Ogre::VulkanExternalInstance &outExternalInstance )
{
    using namespace Ogre;

    // See VulkanRenderSystem::initializeVkInstance

    uint32_t numExtensions = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties( 0, &numExtensions, 0 );
    myCheckVkResult( result, "vkEnumerateInstanceExtensionProperties" );

    FastArray<VkExtensionProperties> availableExtensions;
    availableExtensions.resize( numExtensions );
    result = vkEnumerateInstanceExtensionProperties( 0, &numExtensions, availableExtensions.begin() );
    myCheckVkResult( result, "vkEnumerateInstanceExtensionProperties" );

    const char *wantedExtensions[] = {
#ifdef OGRE_VULKAN_WINDOW_WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif

#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    const size_t kNumWantedExtensions = sizeof( wantedExtensions ) / sizeof( wantedExtensions[0] );

    // Check supported extensions we may want
    FastArray<const char *> reqInstanceExtensions;
    for( size_t i = 0u; i < numExtensions; ++i )
    {
        const String extensionName = availableExtensions[i].extensionName;
        LogManager::getSingleton().logMessage( "SAMPLE: Found instance extension: " + extensionName );

        for( size_t j = 0u; j < kNumWantedExtensions; ++j )
        {
            if( extensionName == wantedExtensions[j] )
                reqInstanceExtensions.push_back( wantedExtensions[j] );
        }
    }

    const char *wantedLayers[] = {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_HIGH
        "VK_LAYER_KHRONOS_validation",
#endif
        "UNUSED",  // Avoid empty arrays
    };
    const size_t kNumWantedLayers = sizeof( wantedLayers ) / sizeof( wantedLayers[0] ) - 1u;

    // Check supported layers we may want
    uint32 numInstanceLayers = 0u;
    result = vkEnumerateInstanceLayerProperties( &numInstanceLayers, 0 );
    myCheckVkResult( result, "vkEnumerateInstanceLayerProperties" );

    FastArray<VkLayerProperties> instanceLayerProps;
    instanceLayerProps.resize( numInstanceLayers );
    result = vkEnumerateInstanceLayerProperties( &numInstanceLayers, instanceLayerProps.begin() );
    myCheckVkResult( result, "vkEnumerateInstanceLayerProperties" );

    FastArray<const char *> instanceLayers;
    for( size_t i = 0u; i < numInstanceLayers; ++i )
    {
        const String layerName = instanceLayerProps[i].layerName;
        LogManager::getSingleton().logMessage( "SAMPLE: Found instance layer: " + layerName );

        for( size_t j = 0u; j < kNumWantedLayers; ++j )
        {
            if( layerName == wantedLayers[j] )
                instanceLayers.push_back( wantedLayers[j] );
        }
    }

    VkInstanceCreateInfo createInfo;
    VkApplicationInfo appInfo;
    memset( &createInfo, 0, sizeof( createInfo ) );
    memset( &appInfo, 0, sizeof( appInfo ) );

    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "My External Vulkan App";
    appInfo.pEngineName = "Ogre3D Vulkan Engine";
    appInfo.engineVersion = OGRE_VERSION;
    appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 2 );

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledLayerCount = static_cast<uint32>( instanceLayers.size() );
    createInfo.ppEnabledLayerNames = instanceLayers.begin();

    reqInstanceExtensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

    createInfo.enabledExtensionCount = static_cast<uint32>( reqInstanceExtensions.size() );
    createInfo.ppEnabledExtensionNames = reqInstanceExtensions.begin();

    // Tell OgreNext about our VkInstance we are creating.
    result = vkCreateInstance( &createInfo, 0, &outExternalInstance.instance );
    myCheckVkResult( result, "vkCreateInstance" );

    {
        // Tell OgreNext what extensions we tried to create.
        // OgreNext will figure out which ones were unsupported and thus failed.
        outExternalInstance.instanceExtensions.reserve( kNumWantedExtensions );

        for( size_t j = 0u; j < kNumWantedExtensions; ++j )
        {
            VkExtensionProperties ext{};
            strncpy( ext.extensionName, wantedExtensions[j], VK_MAX_EXTENSION_NAME_SIZE );
            ext.extensionName[VK_MAX_EXTENSION_NAME_SIZE - 1u] = 0;
            outExternalInstance.instanceExtensions.push_back( ext );
        }
    }

    {
        // Tell OgreNext what layers we tried to create.
        // OgreNext will figure out which ones were unsupported and thus failed.
        outExternalInstance.instanceLayers.reserve( kNumWantedLayers );

        for( size_t j = 0u; j < kNumWantedLayers; ++j )
        {
            VkLayerProperties ext{};
            strncpy( ext.layerName, wantedLayers[j], VK_MAX_EXTENSION_NAME_SIZE );
            ext.layerName[VK_MAX_EXTENSION_NAME_SIZE - 1u] = 0;
            outExternalInstance.instanceLayers.push_back( ext );
        }
    }
}

/**
@brief createVulkanDevice
    Creates a VkDevice ideal for OgreNext to use.
    Follows VulkanDevice::createDevice quite closely.
@param externalInstance
    An already created instance
@param outExternalDevice[out]
    The device we are creating
*/
static void createVulkanDevice( const Ogre::VulkanExternalInstance &externalInstance,
                                Ogre::VulkanExternalDevice &outExternalDevice )
{
    using namespace Ogre;

    {
        // Select the 1st device and hope it's the best one
        uint32_t numDevices = 0u;
        VkResult result = vkEnumeratePhysicalDevices( externalInstance.instance, &numDevices, NULL );

        FastArray<VkPhysicalDevice> pd;
        pd.resize( numDevices );
        result = vkEnumeratePhysicalDevices( externalInstance.instance, &numDevices, pd.begin() );
        myCheckVkResult( result, "vkEnumeratePhysicalDevices" );
        outExternalDevice.physicalDevice = pd[0];
    }

    uint32_t numQueueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties( outExternalDevice.physicalDevice, &numQueueFamilies,
                                              NULL );
    if( numQueueFamilies == 0u )
    {
        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, "Vulkan device is reporting 0 queues!",
                     "VulkanDevice::createDevice" );
    }
    FastArray<VkQueueFamilyProperties> queueProps;
    queueProps.resize( numQueueFamilies );
    vkGetPhysicalDeviceQueueFamilyProperties( outExternalDevice.physicalDevice, &numQueueFamilies,
                                              &queueProps[0] );

    // Find a Graphics Queue
    uint32_t queueFamilyIdx = std::numeric_limits<uint32_t>::max();
    for( uint32_t i = 0u; i < numQueueFamilies; ++i )
    {
        if( queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            queueFamilyIdx = i;
            break;
        }
    }

    if( queueFamilyIdx == std::numeric_limits<uint32_t>::max() )
    {
        LogManager::getSingleton().logMessage(
            "SAMPLE: No graphics queue found. Trying a compute one. Things may not work as expected." );
        for( uint32_t i = 0u; i < numQueueFamilies; ++i )
        {
            if( queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                queueFamilyIdx = i;
                break;
            }
        }

        if( queueFamilyIdx == std::numeric_limits<uint32_t>::max() )
        {
            LogManager::getSingleton().logMessage( "SAMPLE: No graphics or compute queue found!" );
            abort();
        }
    }

    const float queuePriorities[1] = { 1.0f };

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIdx;
    queueCreateInfo.queueCount = 1u;
    queueCreateInfo.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = 1u;
    createInfo.pQueueCreateInfos = &queueCreateInfo;

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures( outExternalDevice.physicalDevice, &deviceFeatures );
    createInfo.pEnabledFeatures = &deviceFeatures;

    // We skip VK_KHR_16bit_storage because it depends on us checkin on
    // VK_KHR_get_physical_device_properties2 (which follows a slightly device
    // creation path, see README.md) and would make this tutorial harder to follow.
    //
    // Also users rarely want to use Midf16 in combination with an external device anyway.
    // Qt wouldn't also know how to create the device if VK_KHR_get_physical_device_properties2
    // is requested anyway (and this is the main reason for this functionality).
    const char *wantedExtensions[] = { VK_KHR_MAINTENANCE2_EXTENSION_NAME,
                                       VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
                                       VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
                                       VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    const size_t kNumWantedExtensions = sizeof( wantedExtensions ) / sizeof( wantedExtensions[0] );

    FastArray<const char *> deviceExtensions;
    {
        uint32_t numExtensions = 0;
        VkResult result = vkEnumerateDeviceExtensionProperties( outExternalDevice.physicalDevice, 0,
                                                                &numExtensions, 0 );
        myCheckVkResult( result, "vkCreateInstance" );

        FastArray<VkExtensionProperties> availableExtensions;
        availableExtensions.resize( numExtensions );
        result = vkEnumerateDeviceExtensionProperties( outExternalDevice.physicalDevice, 0,
                                                       &numExtensions, availableExtensions.begin() );
        myCheckVkResult( result, "vkCreateInstance" );

        for( size_t i = 0u; i < numExtensions; ++i )
        {
            const String extensionName = availableExtensions[i].extensionName;
            LogManager::getSingleton().logMessage( "SAMPLE: Found device extension: " + extensionName );

            for( size_t j = 0u; j < kNumWantedExtensions; ++j )
            {
                if( extensionName == wantedExtensions[j] )
                    deviceExtensions.push_back( wantedExtensions[j] );
            }
        }
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.begin();

    VkResult result =
        vkCreateDevice( outExternalDevice.physicalDevice, &createInfo, NULL, &outExternalDevice.device );
    myCheckVkResult( result, "vkCreateDevice" );

    vkGetDeviceQueue( outExternalDevice.device, queueFamilyIdx, 0u, &outExternalDevice.graphicsQueue );
    // We assume they're the same (even OgreNext does that...)
    outExternalDevice.presentQueue = outExternalDevice.graphicsQueue;

    {
        // Tell OgreNext what extensions we tried to create.
        // OgreNext will figure out which ones were unsupported and thus failed.
        outExternalDevice.deviceExtensions.reserve( kNumWantedExtensions );

        for( size_t j = 0u; j < kNumWantedExtensions; ++j )
        {
            VkExtensionProperties ext{};
            strncpy( ext.extensionName, wantedExtensions[j], VK_MAX_EXTENSION_NAME_SIZE );
            ext.extensionName[VK_MAX_EXTENSION_NAME_SIZE - 1u] = 0;
            outExternalDevice.deviceExtensions.push_back( ext );
        }
    }
}

class MyWindowEventListener final : public Ogre::WindowEventListener
{
    bool mQuit;

public:
    MyWindowEventListener() : mQuit( false ) {}
    void windowClosed( Ogre::Window * ) override { mQuit = true; }

    bool getQuit() const { return mQuit; }
};

int main( int /*argc*/, const char * /*argv*/[] )
{
    using namespace Ogre;

    const String pluginsCfgFolder = "./";
    const String writeAccessFolder = "./";

#ifndef OGRE_STATIC_LIB
#    if OGRE_DEBUG_MODE
    const char *pluginsFile = "plugins_d.cfg";
#    else
    const char *pluginsFile = "plugins.cfg";
#    endif

    Ogre::ConfigFile pluginConfigFile;
    pluginConfigFile.load( pluginsCfgFolder + pluginsFile );

    Ogre::String vulkanPluginLocation = pluginConfigFile.getSetting( "PluginFolder" );
    if( !vulkanPluginLocation.empty() && vulkanPluginLocation.back() != '/' &&
        vulkanPluginLocation.back() != '\\' )
    {
        vulkanPluginLocation += '/';
    }

#    if OGRE_DEBUG_MODE
    vulkanPluginLocation += "RenderSystem_Vulkan_d";
#    else
    vulkanPluginLocation += "RenderSystem_Vulkan";
#    endif

#else
    const char *pluginsFile = 0;  // TODO
#endif
    const Ogre::AbiCookie abiCookie = Ogre::generateAbiCookie();
    Root *root = OGRE_NEW Root( &abiCookie,                      //
                                "",                              //
                                writeAccessFolder + "ogre.cfg",  //
                                writeAccessFolder + "Ogre.log" );

    // We can't call root->showConfigDialog() because that needs a GUI
    // We can try calling root->restoreConfig() though

    Ogre::VulkanExternalInstance externalInstance;
    Ogre::VulkanExternalDevice externalDevice;
    createVulkanInstance( externalInstance );
    createVulkanDevice( externalInstance, externalDevice );

    NameValuePairList pluginParams;
    pluginParams["external_instance"] =
        StringConverter::toString( reinterpret_cast<uintptr_t>( &externalInstance ) );

    root->loadPlugin( vulkanPluginLocation, false, &pluginParams );

    root->setRenderSystem( root->getRenderSystemByName( "Vulkan Rendering Subsystem" ) );
    root->getRenderSystem()->setConfigOption( "sRGB Gamma Conversion", "Yes" );

    // Initialize Root
    root->initialise( false, "Tutorial Vulkan External" );

    NameValuePairList miscParams;
    miscParams["external_device"] =
        StringConverter::toString( reinterpret_cast<uintptr_t>( &externalDevice ) );
    miscParams["gamma"] = "Yes";
    Window *window =
        root->createRenderWindow( "Tutorial Vulkan External", 1280u, 720u, false, &miscParams );

    // Create SceneManager
    const size_t numThreads = 1u;
    SceneManager *sceneManager = root->createSceneManager( ST_GENERIC, numThreads, "ExampleSMInstance" );

    // Create & setup camera
    Camera *camera = sceneManager->createCamera( "Main Camera" );

    // Position it at 500 in Z direction
    camera->setPosition( Vector3( 0, 5, 15 ) );
    // Look back along -Z
    camera->lookAt( Vector3( 0, 0, 0 ) );
    camera->setNearClipDistance( 0.2f );
    camera->setFarClipDistance( 1000.0f );
    camera->setAutoAspectRatio( true );

    // Setup a basic compositor with a blue clear colour
    CompositorManager2 *compositorManager = root->getCompositorManager2();
    const String workspaceName( "Demo Workspace" );
    const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
    compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
    compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName, true );

    MyWindowEventListener myWindowEventListener;
    WindowEventUtilities::addWindowEventListener( window, &myWindowEventListener );

    bool bQuit = false;

    while( !bQuit )
    {
        WindowEventUtilities::messagePump();
        bQuit |= myWindowEventListener.getQuit();
        if( !bQuit )
            bQuit |= !root->renderOneFrame();
    }

    WindowEventUtilities::removeWindowEventListener( window, &myWindowEventListener );

    OGRE_DELETE root;
    root = 0;

    vkDestroyDevice( externalDevice.device, nullptr );
    vkDestroyInstance( externalInstance.instance, nullptr );

    return 0;
}
