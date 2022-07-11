# Vulkan External Sample

It is possible to initialize Vulkan with an externally-provided VkDevice.

In a similar way to how `currentGLContext` in GL3+ can be used to provide
OgreNext with an externally-created OpenGL context.

However due to how Vulkan works, we need:

1. VkInstance (optional, but highly recommended)
   - All the extensions used to create that VkInstance
   - All the layers used to create that VkInstance
2. VkDevice
3. VkPhysicalDevice
4. All extensions used to create that VkDevice
5. VkQueue

The most troublesome part is providing the VkInstance, because
it must be provided while the Vulkan RenderSystem plugin is loaded.

This mean the Vulkan plugin must be installed by hand and
not through plugins.cfg

This sample shows how to do all that.

## About extensions

OgreNext will verify these extensions are actually supported by the VkInstance/VkDevice.

It's not that we don't trust you, it's that sometimes this information is lost
(such as what happens with Qt 5 & 6: You can request Qt to load certain extensions
and layers but it will silently ignore those unsupported ones)

See the comments in `VulkanExternalInstance` and `VulkanExternalDevice`

### What happens if there are extensions missing that OgreNext wants?

All extensions are optional; however if they're absent such functionality will be missing.

Debug layers & extensions are useful for debugging; but they're not strictly required.
And you can force them on through [vkconfig](https://vulkan.lunarg.com/doc/view/1.2.148.1/windows/vkconfig.html) on Windows & Linux (Android too but it's more complex to set).

 - `VK_EXT_shader_subgroup_vote` is needed by VCT (Voxel Cone Tracing) and possibly other algorithms. Without it, such technique won't be available and trying to use it could crash.
 - `VK_EXT_shader_viewport_index_layer` is needed for Instanced Stereo.
 - `VK_KHR_maintenance2` is needed to avoid some debug layer false positives.
 - `VK_KHR_16bit_storage` (and various others it depends on) is needed if you want to use `PrecisionMode::PrecisionMidf16`

The following are obvious extensions needed to render to a window:

 - `VK_KHR_win32_surface` Windows support.
 - `VK_KHR_xcb_surface` X11 support on Linux.
 - `VK_KHR_android_surface` Android support.

Without them, you can use Vulkan for offscreen rendering and Compute,
but OgreNext will be uncapable of creating a window to display Vulkan content.

### What if I create the device with an extension, but I don't tell OgreNext about it?

That depends on the extension.

Most of the time nothing bad will happen and if OgreNext makes use of that feature,
it just won't be available because it doesn't know it's available.

But in some rare cases the extension may cause Vulkan to behave differently
because it expects the application to set something extra; and since OgreNext
won't know about it; it's possible that Vulkan will misbehave or crash.

The same happens if an extension is present, you tell OgreNext about it;
but OgreNext has no idea what to do about it.

For example, enabling `VK_KHR_get_physical_device_properties2` means you MUST
provide `VkPhysicalDeviceFeatures2` into `VkDeviceCreateInfo::pNext` while
leaving `VkPhysicalDeviceFeatures::pEnabledFeatures` as a nullptr.

## Supported platforms

This technique can be used in all platforms where our Vulkan RenderSystem runs
(e.g. including Android) but for simplicity this sample only runs on Windows and Linux.

For simplicity, we don't initialize the Hlms either.
See [Tutorial00_Basic](../Tutorial00_Basic/) on how to do that.

## What cannot be supported

Some Debug Layers' callbacks must be provided when creating the instance/device.

Since OgreNext did not create the VkInstance nor the VkDevice,
we cannot register those callbacks.