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
#ifndef _OgreVulkanPrerequisites_H_
#define _OgreVulkanPrerequisites_H_

#include "OgrePrerequisites.h"

#include "OgreLogManager.h"

typedef struct VkInstance_T *VkInstance;
typedef struct VkPhysicalDevice_T *VkPhysicalDevice;
typedef struct VkDevice_T *VkDevice;

typedef struct VkSurfaceKHR_T *VkSurfaceKHR;
typedef struct VkSwapchainKHR_T *VkSwapchainKHR;
typedef struct VkImage_T *VkImage;
typedef struct VkSemaphore_T *VkSemaphore;
typedef struct VkFence_T *VkFence;

typedef struct VkRenderPass_T *VkRenderPass;
typedef struct VkFramebuffer_T *VkFramebuffer;

typedef struct VkShaderModule_T *VkShaderModule;
typedef struct VkDescriptorSetLayout_T *VkDescriptorSetLayout;

struct VkPipelineShaderStageCreateInfo;

namespace Ogre
{
    // Forward declarations
    class VulkanCache;
    struct VulkanDevice;
    class VulkanGpuProgramManager;
    class VulkanProgram;
    class VulkanQueue;
    class VulkanStagingBuffer;
    class VulkanRenderSystem;
    class VulkanVaoManager;
    class VulkanWindow;

    typedef FastArray<VkSemaphore> VkSemaphoreArray;
    typedef FastArray<VkFence> VkFenceArray;
}  // namespace Ogre

#define OGRE_VK_EXCEPT( code, num, desc, src ) \
    OGRE_EXCEPT( code, desc + ( "\nVkResult = " + vkResultToString( num ) ), src )

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    define checkVkResult( result, functionName ) \
        if( result != VK_SUCCESS ) \
        { \
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, functionName " failed", \
                            __FUNCSIG__ ); \
        }
#else
#    define checkVkResult( result, functionName ) \
        if( result != VK_SUCCESS ) \
        { \
            OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, functionName " failed", \
                            __PRETTY_FUNCTION__ ); \
        }
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    if !defined( __MINGW32__ )
#        define WIN32_LEAN_AND_MEAN
#        ifndef NOMINMAX
#            define NOMINMAX  // required to stop windows.h messing up std::min
#        endif
#    endif
#endif

// Lots of generated code in here which triggers the new VC CRT security warnings
#if !defined( _CRT_SECURE_NO_DEPRECATE )
#    define _CRT_SECURE_NO_DEPRECATE
#endif

#if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 ) && !defined( __MINGW32__ ) && !defined( OGRE_STATIC_LIB )
#    ifdef RenderSystem_Vulkan_EXPORTS
#        define _OgreVulkanExport __declspec( dllexport )
#    else
#        if defined( __MINGW32__ )
#            define _OgreVulkanExport
#        else
#            define _OgreVulkanExport __declspec( dllimport )
#        endif
#    endif
#elif defined( OGRE_GCC_VISIBILITY )
#    define _OgreVulkanExport __attribute__( ( visibility( "default" ) ) )
#else
#    define _OgreVulkanExport
#endif

#endif  //#ifndef _OgreVulkanPrerequisites_H_
