
#include "Utils/HdrUtils.h"

#include "OgreVector4.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "OgreRoot.h"

#include "OgreMaterial.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreTechnique.h"

namespace Demo
{
    void HdrUtils::init( Ogre::uint8 fsaa )
    {
        if( fsaa <= 1 )
            return;

        Ogre::String preprocessorDefines = "MSAA_INITIALIZED=1,";

        preprocessorDefines += "MSAA_SUBSAMPLE_WEIGHT=";
        preprocessorDefines += Ogre::StringConverter::toString( 1.0f / (float)fsaa );
        preprocessorDefines += ",MSAA_NUM_SUBSAMPLES=";
        preprocessorDefines += Ogre::StringConverter::toString( fsaa );

        Ogre::MaterialPtr material =
            std::static_pointer_cast<Ogre::Material>( Ogre::MaterialManager::getSingleton().load(
                "HDR/Resolve_4xFP32_HDR_Box",
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ) );

        Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );

        Ogre::GpuProgram *shader = 0;
        Ogre::GpuProgramParametersSharedPtr oldParams;

        // Save old manual & auto params
        oldParams = pass->getFragmentProgramParameters();
        // Retrieve the HLSL/GLSL/Metal shader and rebuild it with the right settings.
        shader = pass->getFragmentProgram()->_getBindingDelegate();
        shader->setParameter( "preprocessor_defines", preprocessorDefines );
        pass->getFragmentProgram()->reload();
        // Restore manual & auto params to the newly compiled shader
        pass->getFragmentProgramParameters()->copyConstantsFrom( *oldParams );
    }
    //-----------------------------------------------------------------------------------
    void HdrUtils::setSkyColour( const Ogre::ColourValue &colour, float multiplier,
                                 Ogre::CompositorWorkspace *workspace )
    {
        Ogre::CompositorNode *node = workspace->findNode( "HdrRenderingNode" );

        if( !node )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INVALIDPARAMS,
                         "No node 'HdrRenderingNode' in provided workspace ", "HdrUtils::setSkyColour" );
        }

        const Ogre::CompositorPassVec passes = node->_getPasses();

        assert( passes.size() >= 1 );
        Ogre::CompositorPass *pass = passes[0];

        Ogre::RenderPassDescriptor *renderPassDesc = pass->getRenderPassDesc();
        renderPassDesc->setClearColour( colour * multiplier );

        // Set the definition as well, although this isn't strictly necessary.
        Ogre::CompositorManager2 *compositorManager = workspace->getCompositorManager();
        Ogre::CompositorNodeDef *nodeDef =
            compositorManager->getNodeDefinitionNonConst( "HdrRenderingNode" );

        assert( nodeDef->getNumTargetPasses() >= 1 );

        Ogre::CompositorTargetDef *targetDef = nodeDef->getTargetPass( 0 );
        const Ogre::CompositorPassDefVec &passDefs = targetDef->getCompositorPasses();

        assert( passDefs.size() >= 1 );
        Ogre::CompositorPassDef *passDef = passDefs[0];

        passDef->setAllClearColours( colour * multiplier );
    }
    //-----------------------------------------------------------------------------------
    void HdrUtils::setExposure( float exposure, float minAutoExposure, float maxAutoExposure )
    {
        assert( minAutoExposure <= maxAutoExposure );

        Ogre::MaterialPtr material =
            std::static_pointer_cast<Ogre::Material>( Ogre::MaterialManager::getSingleton().load(
                "HDR/DownScale03_SumLumEnd",
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ) );

        Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );
        Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

        const Ogre::Vector3 exposureParams( 1024.0f * expf( exposure - 2.0f ), 7.5f - maxAutoExposure,
                                            7.5f - minAutoExposure );

        psParams = pass->getFragmentProgramParameters();
        psParams->setNamedConstant( "exposure", exposureParams );
    }
    //-----------------------------------------------------------------------------------
    void HdrUtils::setBloomThreshold( float minThreshold, float fullColourThreshold )
    {
        assert( minThreshold < fullColourThreshold );

        Ogre::MaterialPtr material =
            std::static_pointer_cast<Ogre::Material>( Ogre::MaterialManager::getSingleton().load(
                "HDR/BrightPass_Start", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ) );

        Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );

        Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
        psParams->setNamedConstant(
            "brightThreshold",
            Ogre::Vector4( minThreshold, 1.0f / ( fullColourThreshold - minThreshold ), 0, 0 ) );
    }
}  // namespace Demo
