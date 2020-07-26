#include "OgreVulkanHlmsPso.h"

Ogre::VulkanHlmsPso::VulkanHlmsPso( VkPipeline _pso, VulkanProgram *_vertexShader,
                                    VulkanProgram *_pixelShader,
                                    const DescriptorSetLayoutBindingArray &_descriptorLayoutBindingSets,
                                    const DescriptorSetLayoutArray &_descriptorSets,
                                    VkPipelineLayout _layout ) :
    pso( _pso ),
    vertexShader( _vertexShader ),
    pixelShader( _pixelShader ),
    descriptorLayoutBindingSets( _descriptorLayoutBindingSets ),
    descriptorSets( _descriptorSets ),
    pipelineLayout( _layout )
{
    /*DescriptorSetLayoutBindingArray::const_iterator itor = descriptorLayoutBindingSets.begin();
    DescriptorSetLayoutBindingArray::const_iterator endt = descriptorLayoutBindingSets.end();

    size_t i = 0;

    while( itor != endt )
    {
        descriptorLayoutSets.emplace_back( descriptorSets[i++], * itor );
        ++itor;
    }*/
}

Ogre::VulkanHlmsPso::~VulkanHlmsPso()
{
    // std::vector<VulkanDescriptorSetLayout *>::iterator itor = descriptorLayoutSets.begin();
    // std::vector<VulkanDescriptorSetLayout *>::iterator end = descriptorLayoutSets.end();
    //
    // while( itor != end )
    // {
    //     delete *itor;
    //     ++itor;
    // }
}
