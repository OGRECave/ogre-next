
#ifndef _OgreVulkanGlslangHeader_H_
#    define _OgreVulkanGlslangHeader_H_

// clang-format off
#if __cplusplus <= 199711L
    #ifndef nullptr
        #define OgreVulkanNullptrDefined
        #define nullptr (0)
    #endif
    #ifndef override
        #define OgreVulkanOverrideDefined
        #define override
    #endif
#endif

#include "glslang/Public/ShaderLang.h"
#if __cplusplus <= 199711L
    #ifdef OgreVulkanNullptrDefined
        #undef OgreVulkanNullptrDefined
        #undef nullptr
    #endif
    #ifdef OgreVulkanOverrideDefined
        #undef OgreVulkanOverrideDefined
        #undef override
    #endif
#endif
// clang-format on

#endif
