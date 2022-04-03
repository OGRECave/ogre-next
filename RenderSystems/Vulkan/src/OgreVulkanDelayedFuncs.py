
functions = \
[
	["vkDestroyImage", "VkDevice", "device", "VkImage", "image", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyImageView", "VkDevice", "device", "VkImageView", "pView", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyPipeline", "VkDevice", "device", "VkPipeline", "pipeline", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroySampler", "VkDevice", "device", "VkSampler", "sampler", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyBufferView", "VkDevice", "device", "VkBufferView", "bufferView", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyShaderModule", "VkDevice", "device", "VkShaderModule", "shaderModule", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyFramebuffer", "VkDevice", "device", "VkFramebuffer", "framebuffer", "VkAllocationCallbacks const *", "pAllocator"],
	["vkDestroyRenderPass", "VkDevice", "device", "VkRenderPass", "renderPass", "VkAllocationCallbacks const *", "pAllocator"],
]

headerFileTemplate = \
"""// FILE AUTOMATICALLY GENERATED. Run python3 OgreVulkanDelayedFuncs.py

// Fix for UNITY builds
#pragma once

namespace Ogre
{{
    class VulkanDelayedFuncBase
    {{
    public:
        uint32 frameIdx;

		virtual ~VulkanDelayedFuncBase();
        virtual void execute() = 0;
    }};

	{derivedClasses}
}}  // namespace Ogre
"""

headerCmdTemplate = \
"""
class VulkanDelayed_{funcName} final : public VulkanDelayedFuncBase
    {{
    public:
        {argsClassDecl}

        void execute() override;
    }};
    void delayed_{funcName}( VaoManager *vaoMgr, {argsFuncDecl} );
"""

cppTemplate = \
"""// FILE AUTOMATICALLY GENERATED. Run python3 OgreVulkanDelayedFuncs.py
#include "Vao/OgreVulkanVaoManager.h"

#include "vulkan/vulkan_core.h"

#include "OgreVulkanDelayedFuncs.h"

namespace Ogre
{{
	VulkanDelayedFuncBase::~VulkanDelayedFuncBase() {{}}

    {srcImpl}
}}  // namespace Ogre
"""

cppCmdTemplate = \
"""
	void VulkanDelayed_{funcName}::execute() {{ {funcName}( {argsCall} ); }}

    void delayed_{funcName}( VaoManager *vaoMgr, {argsFuncDecl} )
    {{
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_{funcName} *cmd = new VulkanDelayed_{funcName}();
        {argCopy}
        vkVaoMgr->addDelayedFunc( cmd );
    }}
"""

header = ""
src = ""
for func in functions:
	funcName = func[0]

	args = func[1:]
	argsClassDecl = ""
	argsFuncDecl = ""
	argCopy = ""
	argsCall = ""
	for i in range( int( len(args) / 2 ) ):
		argsClassDecl += args[i * 2] + " " + args[i * 2 + 1] + ";\n"
		if i != 0:
			argsFuncDecl += ", "
			argsCall += ", "
		argsFuncDecl += args[i * 2] + " " + args[i * 2 + 1]
		argsCall += args[i * 2 + 1]
		argCopy += "cmd->" + args[i * 2 + 1] + " = " + args[i * 2 + 1] + ";\n"
	
	header += headerCmdTemplate.format( funcName = funcName, argsClassDecl = argsClassDecl, argsFuncDecl = argsFuncDecl )
	src += cppCmdTemplate.format( funcName = funcName, argsFuncDecl = argsFuncDecl, argsCall = argsCall, argCopy = argCopy )

newFile = open( "OgreVulkanDelayedFuncs.h", 'w' )
newFile.write( headerFileTemplate.format( derivedClasses = header ) )

newFile = open( "OgreVulkanDelayedFuncs.cpp", 'w' )
newFile.write( cppTemplate.format( srcImpl = src ) )
