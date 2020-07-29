#include "OgreVulkanHlmsPso.h"

Ogre::VulkanHlmsPso::VulkanHlmsPso( VkPipeline _pso, VulkanProgram *_vertexShader,
                                    VulkanProgram *_pixelShader, VulkanRootLayout *_rootLayout ) :
    pso( _pso ),
    vertexShader( _vertexShader ),
    pixelShader( _pixelShader ),
    rootLayout( _rootLayout )
{
}

Ogre::VulkanHlmsPso::~VulkanHlmsPso()
{
}
