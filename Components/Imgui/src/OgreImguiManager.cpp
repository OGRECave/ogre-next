/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2025 Torus Knot Software Ltd

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

Original implementation by Crashy.
Further enhancements by edherbert.
General fixes: Various people https://forums.ogre3d.org/viewtopic.php?t=93889
Updated for Ogre-Next 2.3 by Vian https://forums.ogre3d.org/viewtopic.php?t=96798
Reworked for Ogre-Next 4.0 by Matias N. Goldberg.

Based on the implementation in https://github.com/edherbert/ogre-next-imgui
-----------------------------------------------------------------------------
*/

#include "OgreImguiManager.h"

#include "CommandBuffer/OgreCbDrawCall.h"
#include "CommandBuffer/OgreCbPipelineStateObject.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreImguiRenderable.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreRenderQueue.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTechnique.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManager.h"
#include "OgreUnifiedHighLevelGpuProgram.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <imgui.h>

using namespace Ogre;

namespace Ogre
{
    const HlmsCache c_dummyCache( 0, HLMS_MAX, HLMS_CACHE_FLAGS_NONE, HlmsPso() );

    class ImguiDummyMO final : public MovableObject
    {
    public:
        ImguiDummyMO( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager,
                      uint8 renderQueueId ) :
            MovableObject( id, objectMemoryManager, manager, renderQueueId )
        {
        }
        ~ImguiDummyMO() override {}

        // Overrides from MovableObject
        const String &getMovableType() const override { return BLANKSTRING; }
    };
}  // namespace Ogre

//-----------------------------------------------------------------------------
ImguiManager::ImguiManager() :
    mDrawData( 0 ),
    mIndirectBuffer( 0 ),
    mCommandBuffer( 0 ),
    mFontTex( 0 ),
    mDummyMovableObject( 0 )
{
    mCommandBuffer = new CommandBuffer();
    createPrograms();
}
//-----------------------------------------------------------------------------
ImguiManager::~ImguiManager()
{
    destroyAllResources();
    delete mDummyMovableObject;
    mDummyMovableObject = 0;
    delete mCommandBuffer;
    mCommandBuffer = 0;
}
//-----------------------------------------------------------------------------
void ImguiManager::destroyAllResources()
{
    VaoManager *vaoManager = Root::getSingleton().getRenderSystem()->getVaoManager();

    for( ImguiRenderable *renderable : mScheduledRenderables )
    {
        renderable->destroyBuffers( vaoManager );
        delete renderable;
    }
    mScheduledRenderables.clear();

    ImguiRenderableMap::const_iterator itor = mAvailableRenderables.begin();
    ImguiRenderableMap::const_iterator endt = mAvailableRenderables.end();

    while( itor != endt )
    {
        for( ImguiRenderable *renderable : itor->second )
        {
            renderable->destroyBuffers( vaoManager );
            delete renderable;
        }
        const String materialName = "!!OgreImgui_" + itor->first->getName().getReleaseText();
        MaterialPtr material = MaterialManager::getSingleton().getByName(
            materialName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );
        MaterialManager::getSingleton().remove( material );
        ++itor;
    }
    mAvailableRenderables.clear();

    if( mIndirectBuffer )
    {
        if( mIndirectBuffer->getMappingState() != MS_UNMAPPED )
            mIndirectBuffer->unmap( UO_UNMAP_ALL );
        vaoManager->destroyIndirectBuffer( mIndirectBuffer );
        mIndirectBuffer = 0;
    }

    mCommandBuffer->clear();
}
//-----------------------------------------------------------------------------
Matrix4 ImguiManager::getProjectionMatrix( RenderSystem *rs, const bool bRequiresTextureFlipping,
                                           const Camera *currentCamera ) const
{
    const ImGuiIO &io = ImGui::GetIO();
    Matrix4 projectionMatrix( 2.0f / io.DisplaySize.x, 0.0f, 0.0f, -1.0f,  //
                              0.0f, -2.0f / io.DisplaySize.y, 0.0f, 1.0f,  //
                              0.0f, 0.0f, -1.0f, 0.0f,                     //
                              0.0f, 0.0f, 0.0f, 1.0f );
    // Still need to take RS depth into account.
    rs->_convertProjectionMatrix( projectionMatrix, projectionMatrix );
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
    projectionMatrix =
        projectionMatrix * Quaternion( currentCamera->getOrientationModeAngle(), Vector3::UNIT_Z );
#endif

    if( bRequiresTextureFlipping )
    {
        // Invert transformed y.
        projectionMatrix[1][0] = -projectionMatrix[1][0];
        projectionMatrix[1][1] = -projectionMatrix[1][1];
        projectionMatrix[1][2] = -projectionMatrix[1][2];
        projectionMatrix[1][3] = -projectionMatrix[1][3];
    }
    return projectionMatrix;
}
//-----------------------------------------------------------------------------
void ImguiManager::createPrograms()
{
    static const char *vertexShaderSrcD3D11 = {
        "uniform float4x4 ProjectionMatrix;\n"
        "struct VS_INPUT\n"
        "{\n"
        "float2 pos : POSITION;\n"
        "float4 col : COLOR0;\n"
        "float2 uv  : TEXCOORD0;\n"
        "};\n"
        "struct PS_INPUT\n"
        "{\n"
        "float4 pos : SV_POSITION;\n"
        "float4 col : COLOR0;\n"
        "float2 uv  : TEXCOORD0;\n"
        "};\n"
        "PS_INPUT main(VS_INPUT input)\n"
        "{\n"
        "PS_INPUT output;\n"
        "output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
        "output.col = input.col;\n"
        "output.uv  = input.uv;\n"
        "return output;\n"
        "}"
    };
    static const char *pixelShaderSrcD3D11 = {
        "struct PS_INPUT\n"
        "{\n"
        "float4 pos : SV_POSITION;\n"
        "float4 col : COLOR0;\n"
        "float2 uv  : TEXCOORD0;\n"
        "};\n"
        "sampler sampler0: register(s0);\n"
        "Texture2D texture0: register(t0);\n"
        "\n"
        "float4 main(PS_INPUT input) : SV_Target\n"
        "{\n"
        "float4 out_col = input.col * texture0.Sample(sampler0, input.uv);\n"
        "return out_col; \n"
        "}"
    };

    static const char *vertexShaderSrcGLSL = {
        "#version 150\n"
        "uniform mat4 ProjectionMatrix; \n"
        "in vec2 vertex;\n"
        "in vec2 uv0;\n"
        "in vec4 colour;\n"
        "out vec2 Texcoord;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
        "Texcoord  = uv0;\n"
        "col = colour;\n"
        "}"
    };
    static const char *pixelShaderSrcGLSL = {
        "#version 150\n"
        "in vec2 Texcoord;\n"
        "in vec4 col;\n"
        "uniform sampler2D sampler0;\n"
        "out vec4 out_col;\n"
        "void main()\n"
        "{\n"
        "out_col = col * texture(sampler0, Texcoord);\n"
        "}"
    };
    static const char *vertexShaderSrcVK = {
        "vulkan( layout( ogre_P0 ) uniform Params { )\n"
        "    uniform mat4 ProjectionMatrix; \n"
        "vulkan( }; )\n"
        "vulkan_layout( OGRE_POSITION ) in vec2 vertex;\n"
        "vulkan_layout( OGRE_TEXCOORD0 ) in vec2 Texcoord;\n"
        "vulkan_layout( OGRE_DIFFUSE ) in vec4 colour;\n"
        "vulkan_layout( location = 1 )\n"
        "out block\n"
        "{\n"
        "    vec2 uv0;\n"
        "    vec4 col;\n"
        "} outVs;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
        "    outVs.uv0 = Texcoord;\n"
        "    outVs.col = colour;\n"
        "}"
    };
    static const char *pixelShaderSrcVK = {
        "vulkan_layout( ogre_t0 ) uniform texture2D sampler0;\n"
        "vulkan( layout( ogre_s0 ) uniform sampler texSampler );\n"
        "vulkan_layout( location = 0 )\n"
        "out vec4 out_col;\n"
        "vulkan_layout( location = 1 )\n"
        "in block\n"
        "{\n"
        "    vec2 uv0;\n"
        "    vec4 col;\n"
        "} inPs;\n"
        "void main()\n"
        "{\n"
        "    out_col = inPs.col * texture( vkSampler2D( sampler0, texSampler ), inPs.uv0 );"
        "}"
    };
    static const char *fragmentShaderSrcMetal = {
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "    float2 texCoords;\n"
        "    float4 colour;\n"
        "};\n"
        "\n"
        "fragment float4 main_metal(VertexOut in [[stage_in]],\n"
        "                             texture2d<float> texture [[texture(0)]]) {\n"
        "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, "
        "mip_filter::linear);\n"
        "    float4 texColour = texture.sample(linearSampler, in.texCoords);\n"
        "    return in.colour * texColour;\n"
        "}\n"
    };

    static const char *vertexShaderSrcMetal = {
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct Constant {\n"
        "    float4x4 ProjectionMatrix;\n"
        "};\n"
        "\n"
        "struct VertexIn {\n"
        "    float2 position  [[attribute(VES_POSITION)]];\n"
        "    float2 texCoords [[attribute(VES_TEXTURE_COORDINATES0)]];\n"
        "    float4 colour     [[attribute(VES_DIFFUSE)]];\n"
        "};\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "    float2 texCoords;\n"
        "    float4 colour;\n"
        "};\n"
        "\n"
        "vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],\n"
        "                             constant Constant &uniforms [[buffer(PARAMETER_SLOT)]]) {\n"
        "    VertexOut out;\n"
        "    out.position = uniforms.ProjectionMatrix * float4(in.position, 0, 1);\n"

        "    out.texCoords = in.texCoords;\n"
        "    out.colour = in.colour;\n"

        "    return out;\n"
        "}\n"
    };

    // create the default shadows material
    HighLevelGpuProgramManager &mgr = HighLevelGpuProgramManager::getSingleton();

    HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName( "imgui/VP" );
    HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName( "imgui/FP" );

    HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName( "imgui/VP/D3D11" );
    HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName( "imgui/FP/D3D11" );

    HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName( "imgui/VP/GL150" );
    HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName( "imgui/FP/GL150" );

    HighLevelGpuProgramPtr vertexShaderVK = mgr.getByName( "imgui/VP/VK" );
    HighLevelGpuProgramPtr pixelShaderVK = mgr.getByName( "imgui/FP/VK" );

    HighLevelGpuProgramPtr vertexShaderMetal = mgr.getByName( "imgui/VP/Metal" );
    HighLevelGpuProgramPtr pixelShaderMetal = mgr.getByName( "imgui/FP/Metal" );

    if( !vertexShaderUnified )
    {
        vertexShaderUnified =
            mgr.createProgram( "imgui/VP", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, "unified",
                               GPT_VERTEX_PROGRAM );
    }
    if( !pixelShaderUnified )
    {
        pixelShaderUnified =
            mgr.createProgram( "imgui/FP", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, "unified",
                               GPT_FRAGMENT_PROGRAM );
    }

    UnifiedHighLevelGpuProgram *vertexShaderPtr =
        static_cast<UnifiedHighLevelGpuProgram *>( vertexShaderUnified.get() );
    UnifiedHighLevelGpuProgram *pixelShaderPtr =
        static_cast<UnifiedHighLevelGpuProgram *>( pixelShaderUnified.get() );

    if( !vertexShaderD3D11 )
    {
        vertexShaderD3D11 =
            mgr.createProgram( "imgui/VP/D3D11", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "hlsl", GPT_VERTEX_PROGRAM );
        vertexShaderD3D11->setParameter( "target", "vs_5_0 vs_4_0" );
        vertexShaderD3D11->setParameter( "entry_point", "main" );
        vertexShaderD3D11->setSource( vertexShaderSrcD3D11 );
        vertexShaderD3D11->load();

        vertexShaderPtr->addDelegateProgram( vertexShaderD3D11->getName() );
    }
    if( !pixelShaderD3D11 )
    {
        pixelShaderD3D11 =
            mgr.createProgram( "imgui/FP/D3D11", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "hlsl", GPT_FRAGMENT_PROGRAM );
        pixelShaderD3D11->setParameter( "target", "ps_5_0 ps_4_0" );
        pixelShaderD3D11->setParameter( "entry_point", "main" );
        pixelShaderD3D11->setSource( pixelShaderSrcD3D11 );
        pixelShaderD3D11->load();

        pixelShaderPtr->addDelegateProgram( pixelShaderD3D11->getName() );
    }

    if( !vertexShaderMetal )
    {
        vertexShaderMetal =
            mgr.createProgram( "imgui/VP/Metal", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "metal", GPT_VERTEX_PROGRAM );
        vertexShaderMetal->setParameter( "entry_point", "vertex_main" );
        vertexShaderMetal->setSource( vertexShaderSrcMetal );
        vertexShaderMetal->load();
        vertexShaderPtr->addDelegateProgram( vertexShaderMetal->getName() );
    }
    if( !pixelShaderMetal )
    {
        pixelShaderMetal =
            mgr.createProgram( "imgui/FP/Metal", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "metal", GPT_FRAGMENT_PROGRAM );
        vertexShaderMetal->setParameter( "entry_point", "fragment_main" );
        pixelShaderMetal->setSource( fragmentShaderSrcMetal );
        pixelShaderMetal->load();
        pixelShaderPtr->addDelegateProgram( pixelShaderMetal->getName() );
    }

    if( !vertexShaderGL )
    {
        vertexShaderGL =
            mgr.createProgram( "imgui/VP/GL150", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "glsl", GPT_VERTEX_PROGRAM );
        vertexShaderGL->setSource( vertexShaderSrcGLSL );
        vertexShaderGL->load();
        vertexShaderPtr->addDelegateProgram( vertexShaderGL->getName() );
    }
    if( !pixelShaderGL )
    {
        pixelShaderGL =
            mgr.createProgram( "imgui/FP/GL150", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "glsl", GPT_FRAGMENT_PROGRAM );
        pixelShaderGL->setSource( pixelShaderSrcGLSL );
        pixelShaderGL->load();

        pixelShaderPtr->addDelegateProgram( pixelShaderGL->getName() );
    }

    if( !vertexShaderVK )
    {
        vertexShaderVK =
            mgr.createProgram( "imgui/VP/vulkan", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "glslvk", GPT_VERTEX_PROGRAM );
        vertexShaderVK->setSource( vertexShaderSrcVK );
        vertexShaderVK->setPrefabRootLayout( PrefabRootLayout::Standard );
        vertexShaderVK->load();
        vertexShaderPtr->addDelegateProgram( vertexShaderVK->getName() );
    }
    if( !pixelShaderVK )
    {
        pixelShaderVK =
            mgr.createProgram( "imgui/FP/vulkan", ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
                               "glslvk", GPT_FRAGMENT_PROGRAM );
        pixelShaderVK->setSource( pixelShaderSrcVK );
        pixelShaderVK->setPrefabRootLayout( PrefabRootLayout::Standard );
        pixelShaderVK->load();

        pixelShaderPtr->addDelegateProgram( pixelShaderVK->getName() );
    }
}
//-----------------------------------------------------------------------------
MaterialPtr ImguiManager::createMaterialFor( TextureGpu *texture )
{
    const String materialName = "!!OgreImgui_" + texture->getName().getReleaseText();
    MaterialPtr imguiMaterial = MaterialManager::getSingleton().create(
        materialName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME );

    HlmsBlendblock blendblock;
    blendblock.mSourceBlendFactor = SBF_SOURCE_ALPHA;
    blendblock.mDestBlendFactor = SBF_ONE_MINUS_SOURCE_ALPHA;
    blendblock.mSourceBlendFactorAlpha = SBF_ONE_MINUS_SOURCE_ALPHA;
    blendblock.mDestBlendFactorAlpha = SBF_ZERO;
    blendblock.mBlendOperation = SBO_ADD;
    blendblock.mBlendOperationAlpha = SBO_ADD;
    blendblock.mSeparateBlend = true;
    blendblock.mIsTransparent = true;

    HlmsMacroblock macroblock;
    macroblock.mCullMode = CULL_NONE;
    macroblock.mDepthFunc = CMPF_ALWAYS_PASS;
    macroblock.mDepthCheck = false;
    macroblock.mDepthWrite = false;
    macroblock.mScissorTestEnabled = true;

    Pass *pass = imguiMaterial->getTechnique( 0 )->getPass( 0 );
    pass->setFragmentProgram( "imgui/FP" );
    pass->setVertexProgram( "imgui/VP" );

    pass->setBlendblock( blendblock );
    pass->setMacroblock( macroblock );

    pass->createTextureUnitState()->setTexture( texture );

    return imguiMaterial;
}
//-----------------------------------------------------------------------------
ImguiRenderable *ImguiManager::getAvailableRenderable( TextureGpu *texture )
{
    ImguiRenderable *retVal = 0;

    ImguiRenderableMap::iterator itor = mAvailableRenderables.find( texture );
    if( itor != mAvailableRenderables.end() )
    {
        if( itor->second.empty() )
        {
            ImguiRenderable *renderable = new ImguiRenderable();
            renderable->setMaterial( createMaterialFor( texture ) );
            itor->second.push_back( renderable );
        }
        retVal = itor->second.back();
        itor->second.pop_back();
    }
    else
    {
        ImguiRenderable *renderable = new ImguiRenderable();
        renderable->setMaterial( createMaterialFor( texture ) );
        mAvailableRenderables.insert( { texture, {} } );
        retVal = renderable;
    }

    mScheduledRenderables.push_back( retVal );

    return retVal;
}
//-----------------------------------------------------------------------------
void ImguiManager::setupFont( ImFontAtlas *fontAtlas, bool bUseSynchronous )
{
    OGRE_ASSERT_LOW( !mFontTex );
    unsigned char *pixels;
    int iwidth, iheight;
    fontAtlas->GetTexDataAsRGBA32( &pixels, &iwidth, &iheight );
    const uint32 width = uint32( iwidth );
    const uint32 height = uint32( iheight );

    TextureGpuManager *textureManager = Root::getSingleton().getRenderSystem()->getTextureGpuManager();

    mFontTex = textureManager->createTexture(
        "imgui/fontTex", GpuPageOutStrategy::AlwaysKeepSystemRamCopy,
        bUseSynchronous ? TextureFlags::ManualTexture : 0u, TextureTypes::Type2D );
    mFontTex->setResolution( width, height );
    mFontTex->setPixelFormat( PixelFormatGpu::PFG_RGBA8_UNORM_SRGB );

    fontAtlas->SetTexID( mFontTex );

    Image2 image;
    image.createEmptyImageLike( mFontTex );
    TextureBox dstBox = image.getData( 0u );
    for( uint32_t y = 0u; y < dstBox.height; ++y )
    {
        void *dstRaw = dstBox.at( 0u, y, 0u );
        memcpy( dstRaw, &pixels[y * width * 4u], width * 4u );
    }

    if( bUseSynchronous )
    {
        mFontTex->scheduleTransitionTo( GpuResidency::Resident );
        image.uploadTo( mFontTex, 0u, mFontTex->getNumMipmaps() - 1u );
    }
    else
    {
        // Tweak via _setAutoDelete so the internal data is copied as a pointer
        // instead of performing a deep copy of the data; while leaving the responsability
        // of freeing memory to imagePtr instead.
        image._setAutoDelete( false );
        Image2 *imagePtr = new Image2( image );
        imagePtr->_setAutoDelete( true );

        if( mFontTex->getNextResidencyStatus() == GpuResidency::Resident )
            mFontTex->scheduleTransitionTo( GpuResidency::OnStorage );
        // Ogre will call "delete imagePtr" when done, because we're passing
        // true to autoDeleteImage argument in scheduleTransitionTo.
        mFontTex->scheduleTransitionTo( GpuResidency::Resident, imagePtr, true );
    }
}
//-----------------------------------------------------------------------------
void ImguiManager::prepareForRender( SceneManager *sceneManager )
{
    // Tell ImGui to create the buffers.
    ImGui::Render();

    const ImDrawData *drawData = ImGui::GetDrawData();
    mDrawData = drawData;

    for( ImguiRenderable *renderable : mScheduledRenderables )
    {
        TextureGpu *texture = renderable->getMaterial()
                                  ->getTechnique( 0u )
                                  ->getPass( 0u )
                                  ->getTextureUnitState( 0u )
                                  ->_getTexturePtr( 0u );
        mAvailableRenderables[texture].push_back( renderable );
    }
    mScheduledRenderables.clear();

    size_t numNeededDraws = 0u;
    for( int n = 0; n < drawData->CmdListsCount; n++ )
    {
        const ImDrawList *drawList = drawData->CmdLists[n];
        numNeededDraws += size_t( drawList->CmdBuffer.Size );
    }

    VaoManager *vaoManager = sceneManager->getDestinationRenderSystem()->getVaoManager();
    if( !mDummyMovableObject )
    {
        mDummyMovableObject = OGRE_NEW ImguiDummyMO(
            Id::generateNewId<Ogre::MovableObject>(),
            &sceneManager->_getEntityMemoryManager( SCENE_STATIC ), sceneManager, 254u );
        sceneManager->getRootSceneNode( SCENE_STATIC )->attachObject( mDummyMovableObject );
        mDummyMovableObject->setVisible( false );
        mDummyMovableObject->setCastShadows( false );
    }

    const bool supportsIndirectBuffers = vaoManager->supportsIndirectBuffers();

    unsigned char *indirectDraw = 0;
    if( numNeededDraws > 0u )
    {
        if( !mIndirectBuffer ||
            ( numNeededDraws * sizeof( CbDrawIndexed ) ) > mIndirectBuffer->getNumElements() )
        {
            if( mIndirectBuffer )
            {
                if( mIndirectBuffer->getMappingState() != MS_UNMAPPED )
                    mIndirectBuffer->unmap( UO_UNMAP_ALL );
                vaoManager->destroyIndirectBuffer( mIndirectBuffer );
            }
            mIndirectBuffer = vaoManager->createIndirectBuffer( numNeededDraws * sizeof( CbDrawIndexed ),
                                                                BT_DYNAMIC_PERSISTENT, 0, false );
        }

        if( supportsIndirectBuffers )
        {
            indirectDraw = static_cast<unsigned char *>(
                mIndirectBuffer->map( 0, mIndirectBuffer->getNumElements() ) );
        }
        else
        {
            indirectDraw = mIndirectBuffer->getSwBufferPtr();
        }
    }

    // iterate through all lists (at the moment every window has its own)
    for( int n = 0; n < drawData->CmdListsCount; n++ )
    {
        const ImDrawList *drawList = drawData->CmdLists[n];
        const ImDrawVert *vtxBuf = drawList->VtxBuffer.Data;
        const ImDrawIdx *idxBuf = drawList->IdxBuffer.Data;

        unsigned int startIdx = 0;

        for( int i = 0; i < drawList->CmdBuffer.Size; i++ )
        {
            const ImDrawCmd *drawCmd = &drawList->CmdBuffer[i];

            TextureGpu *texture = reinterpret_cast<TextureGpu *>( drawCmd->GetTexID() );
            ImguiRenderable *renderable = getAvailableRenderable( texture );

            // update their vertex buffers.
            renderable->updateVertexData( vtxBuf, &idxBuf[startIdx],
                                          (unsigned int)drawList->VtxBuffer.Size, drawCmd->ElemCount,
                                          vaoManager );

            VertexArrayObject *vao = renderable->getVaos( VpNormal ).back();

            CbDrawIndexed *cmd = reinterpret_cast<CbDrawIndexed *>( indirectDraw );
            indirectDraw += sizeof( CbDrawIndexed );
            cmd->primCount = vao->getPrimitiveCount();
            cmd->instanceCount = 1u;
            cmd->firstVertexIndex =
                uint32_t( vao->getIndexBuffer()->_getFinalBufferStart() + vao->getPrimitiveStart() );
            cmd->baseVertex = uint32_t( vao->getBaseVertexBuffer()->_getFinalBufferStart() );
            cmd->baseInstance = 0u;

            startIdx += drawCmd->ElemCount;
        }
    }

    if( indirectDraw && supportsIndirectBuffers )
        mIndirectBuffer->unmap( UO_KEEP_PERSISTENT );

    // Delete unused renderables, but leaving just one element.
    ImguiRenderableMap::iterator itor = mAvailableRenderables.begin();
    ImguiRenderableMap::iterator endt = mAvailableRenderables.end();

    while( itor != endt )
    {
        while( itor->second.size() > 1u )
        {
            ImguiRenderable *renderable = itor->second.back();
            renderable->destroyBuffers( vaoManager );
            delete renderable;
            itor->second.pop_back();
        }
        ++itor;
    }
}
//-----------------------------------------------------------------------------
void ImguiManager::drawIntoCompositor( RenderPassDescriptor *renderPassDesc,
                                       TextureGpu *anyTargetTexture, SceneManager *sceneManager,
                                       const Camera *currentCamera )
{
    HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
    Hlms *hlms = hlmsManager->getHlms( HLMS_LOW_LEVEL );

    RenderSystem *renderSystem = sceneManager->getDestinationRenderSystem();
    mCommandBuffer->setCurrentRenderSystem( renderSystem );

    const bool bWasReadyForPresent = renderPassDesc->mReadyWindowForPresent;

    const VaoManager *vaoManager = renderSystem->getVaoManager();

    int baseInstanceAndIndirectBuffers = 0;
    if( vaoManager->supportsIndirectBuffers() )
        baseInstanceAndIndirectBuffers = 2;
    else if( vaoManager->supportsBaseInstance() )
        baseInstanceAndIndirectBuffers = 1;

    // In theory we should call preparePassHash() after every executeRenderPassDescriptorDelayedActions,
    // but it doesn't seem like it's needed and has a performance cost.
    HlmsCache passCache = hlms->preparePassHash( 0, false, false, sceneManager );

    const int vpWidth = int( anyTargetTexture->getWidth() );
    const int vpHeight = int( anyTargetTexture->getHeight() );

    const Vector4 viewportSize( 0, 0, 1, 1 );

    mCommandBuffer->setCurrentRenderSystem( renderSystem );

    const Matrix4 projMatrix =
        getProjectionMatrix( renderSystem, renderPassDesc->requiresTextureFlipping(), currentCamera );

    RenderingMetrics stats;

    const ImDrawData *drawData = mDrawData;
    ImguiRenderableVec::const_iterator renderableBegin = mScheduledRenderables.begin();
    ImguiRenderableVec::const_iterator renderableIt = renderableBegin;
    for( int n = 0; n < drawData->CmdListsCount; n++ )
    {
        const ImDrawList *drawList = drawData->CmdLists[n];

        for( int i = 0; i < drawList->CmdBuffer.Size; i++ )
        {
            ImguiRenderable *renderable = *renderableIt;

            const ImDrawCmd *drawCmd = &drawList->CmdBuffer[i];

            // Set scissoring.
            int scLeft = static_cast<int>( drawCmd->ClipRect.x );  // Obtain bounds
            int scTop = static_cast<int>( drawCmd->ClipRect.y );
            int scRight = static_cast<int>( drawCmd->ClipRect.z );
            int scBottom = static_cast<int>( drawCmd->ClipRect.w );

            // Clamp bounds to viewport dimensions.
            scLeft = Math::Clamp( scLeft, 0, vpWidth );
            scRight = Math::Clamp( scRight, 0, vpWidth );
            scTop = Math::Clamp( scTop, 0, vpHeight );
            scBottom = Math::Clamp( scBottom, 0, vpHeight );

            const float left = (float)scLeft / (float)vpWidth;
            const float top = (float)scTop / (float)vpHeight;
            const float width = (float)( scRight - scLeft ) / (float)vpWidth;
            const float height = (float)( scBottom - scTop ) / (float)vpHeight;

            if( bWasReadyForPresent )
            {
                const bool bShouldBeReadyForPresent =
                    ( n + 1 ) == drawData->CmdListsCount && ( i + 1 ) == drawList->CmdBuffer.Size;
                if( bShouldBeReadyForPresent != renderPassDesc->mReadyWindowForPresent )
                {
                    renderSystem->endRenderPassDescriptor();
                    renderPassDesc->mReadyWindowForPresent = bShouldBeReadyForPresent;
                    renderPassDesc->entriesModified( RenderPassDescriptor::Colour );
                }
            }

            const Vector4 scissors = Vector4( left, top, width, height );
            renderSystem->beginRenderPassDescriptor( renderPassDesc, anyTargetTexture, 0u, &viewportSize,
                                                     &scissors, 1u, false, false );
            renderSystem->executeRenderPassDescriptorDelayedActions();

            QueuedRenderable queuedRenderable( 0u, renderable, mDummyMovableObject );

            renderable->getMaterial()
                ->getTechnique( 0u )
                ->getPass( 0u )
                ->getVertexProgramParameters()
                ->setNamedConstant( "ProjectionMatrix", projMatrix );

            // We can't cache anything because we're calling beginRenderPassDescriptor() on every
            // iteration.
            const HlmsCache *hlmsCache =
                hlms->getMaterial( &c_dummyCache, passCache, queuedRenderable, false, nullptr );

            CbPipelineStateObject *psoCmd = mCommandBuffer->addCommand<CbPipelineStateObject>();
            *psoCmd = CbPipelineStateObject( &hlmsCache->pso );

            hlms->fillBuffersForV2( hlmsCache, queuedRenderable, false, 0u, mCommandBuffer );

            VertexArrayObject *vao = renderable->getVaos( VpNormal ).back();

            OGRE_ASSERT_MEDIUM( vao->getVaoName() != 0u &&
                                "Invalid Vao name! This can happen if a BT_IMMUTABLE buffer was "
                                "recently created and VaoManager::_beginFrame() wasn't called" );

            *mCommandBuffer->addCommand<CbVao>() = CbVao( vao );
            *mCommandBuffer->addCommand<CbIndirectBuffer>() = CbIndirectBuffer( mIndirectBuffer );

            void *offset = reinterpret_cast<void *>( mIndirectBuffer->_getFinalBufferStart() +
                                                     sizeof( CbDrawIndexed ) *
                                                         size_t( renderableIt - renderableBegin ) );

            CbDrawCallIndexed *drawCall = mCommandBuffer->addCommand<CbDrawCallIndexed>();
            *drawCall = CbDrawCallIndexed( baseInstanceAndIndirectBuffers, vao, offset );
            drawCall->numDraws = 1u;
            stats.mDrawCount += 1u;
            stats.mInstanceCount += 1u;
            stats.mFaceCount += vao->getPrimitiveCount() / 3u;
            stats.mVertexCount += vao->getPrimitiveCount();

            hlms->preCommandBufferExecution( mCommandBuffer );
            mCommandBuffer->execute();
            hlms->postCommandBufferExecution( mCommandBuffer );

            ++renderableIt;
        }
    }

    renderSystem->_addMetrics( stats );

    // There was nothing for imgui to draw. We must still prepare the window for presenting.
    if( bWasReadyForPresent && !stats.mDrawCount )
    {
        Vector4 scissors( 0, 0, 1, 1 );
        renderSystem->beginRenderPassDescriptor( renderPassDesc, anyTargetTexture, 0u, &viewportSize,
                                                 &scissors, 1u, false, false );
        renderSystem->executeRenderPassDescriptorDelayedActions();
    }
}
