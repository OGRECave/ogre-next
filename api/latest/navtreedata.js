/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "OGRE-Next", "index.html", [
    [ "API Reference Start Page", "index.html", "index" ],
    [ "Setting Up Ogre", "_setting_up_ogre.html", [
      [ "Windows", "_setting_up_ogre_windows.html", [
        [ "Requirements", "_setting_up_ogre_windows.html#RequirementsWindows", null ],
        [ "Downloading Ogre", "_setting_up_ogre_windows.html#DownloadingOgreWindows", null ],
        [ "Building Dependencies", "_setting_up_ogre_windows.html#BuildingDependenciesWindows", null ],
        [ "Building Ogre", "_setting_up_ogre_windows.html#BuildingOgreWindows", null ]
      ] ],
      [ "Linux", "_setting_up_ogre_linux.html", [
        [ "Requirements", "_setting_up_ogre_linux.html#RequirementsLinux", null ],
        [ "Downloading Ogre", "_setting_up_ogre_linux.html#DownloadingOgreLinux", null ],
        [ "Building Dependencies", "_setting_up_ogre_linux.html#BuildingDependenciesLinux", null ],
        [ "Building Ogre", "_setting_up_ogre_linux.html#BuildingOgreLinux", null ],
        [ "Setting Up Ogre with QtCreator", "_setting_up_ogre_linux.html#SettingUpOgreWithQtCreatorLinux", null ]
      ] ],
      [ "iOS", "_setting_up_ogre_i_o_s.html", [
        [ "Requirements", "_setting_up_ogre_i_o_s.html#Requirements_iOS", null ],
        [ "Downloading Ogre", "_setting_up_ogre_i_o_s.html#DownloadingOgre_iOS", null ],
        [ "Building Ogre", "_setting_up_ogre_i_o_s.html#BuildingOgre_iOS", null ]
      ] ],
      [ "macOS", "_setting_up_ogre_mac_o_s.html", [
        [ "Install cmake", "_setting_up_ogre_mac_o_s.html#autotoc_md12", null ],
        [ "Install dependencies", "_setting_up_ogre_mac_o_s.html#autotoc_md13", [
          [ "Install SDL2 using brew", "_setting_up_ogre_mac_o_s.html#autotoc_md14", null ]
        ] ],
        [ "Build Ogre", "_setting_up_ogre_mac_o_s.html#autotoc_md15", null ],
        [ "Build offline version of the docs", "_setting_up_ogre_mac_o_s.html#autotoc_md16", null ]
      ] ],
      [ "Android", "_setting_up_ogre_android.html", [
        [ "Requirements", "_setting_up_ogre_android.html#RequirementsAndroid", null ],
        [ "Downloading and building Ogre", "_setting_up_ogre_android.html#DownloadingOgreAndroid", null ],
        [ "Building Samples", "_setting_up_ogre_android.html#BuildingSamplesAndroid", null ]
      ] ]
    ] ],
    [ "Using Ogre in your App", "_using_ogre_in_your_app.html", [
      [ "Overview", "_using_ogre_in_your_app.html#UsingOgreInYourAppOverview", null ],
      [ "Speeding things up", "_using_ogre_in_your_app.html#SpeedingThingsUp", [
        [ "Small note about iOS", "_using_ogre_in_your_app.html#SmallNoteAbout_iOS", null ],
        [ "Apple specific", "_using_ogre_in_your_app.html#autotoc_md17", null ],
        [ "Creating your application with 'EmptyProject' script", "_using_ogre_in_your_app.html#CreatingYourApplication", [
          [ "A note about copied files from Samples/2.0/Common", "_using_ogre_in_your_app.html#AnoteaboutcopiedfilesfromSamples_20_Common", null ]
        ] ],
        [ "Supporting Multithreading loops from the get go", "_using_ogre_in_your_app.html#SupportingMultithreadingLoopsFromTheGetGo", null ]
      ] ]
    ] ],
    [ "Behavior of StagingTexture in D3D11", "_behavior_staging_texture_d3_d11.html", [
      [ "Attempting to be contiguous", "_behavior_staging_texture_d3_d11.html#autotoc_md18", null ],
      [ "Slicing in 3", "_behavior_staging_texture_d3_d11.html#autotoc_md19", null ],
      [ "Slicing in the middle", "_behavior_staging_texture_d3_d11.html#autotoc_md20", null ]
    ] ],
    [ "Manual", "manual.html", [
      [ "Introduction & Changes from Ogre 1.x", "_introduction_and_changes.html", [
        [ "Technical Overview", "_technical_overview.html", [
          [ "Overview", "_technical_overview.html#TechnicalOverviewOverview", [
            [ "SIMD Coherence", "_technical_overview.html#SIMDCoherence", null ]
          ] ],
          [ "Memory Managers usage patterns", "_technical_overview.html#MemoryManagerUsagePatterns", [
            [ "Cleanups", "_technical_overview.html#MemoryManagerCleanups", null ]
          ] ],
          [ "Memory preallocation", "_technical_overview.html#MemoryPreallocation", null ],
          [ "Configuring memory managers", "_technical_overview.html#ConfiguringMemoryManagers", null ],
          [ "Where is RenderTarget::update? Why do I get errors in Viewport?", "_technical_overview.html#RenderTargetUpdate", null ],
          [ "Porting from 1.x to 2.0", "_technical_overview.html#PortingV1ToV2", null ],
          [ "Porting from 2.0 to 2.1", "_technical_overview.html#PortingV20ToV21", null ]
        ] ],
        [ "Changes: Objects, Scene & Nodes", "_ogre20_changes.html", [
          [ "Names are now optional", "_ogre20_changes.html#NamesAreNowOptional", null ],
          [ "How to debug MovableObjects' (and Nodes) data", "_ogre20_changes.html#HowToDebugMovableObjectsData", [
            [ "Interpreting ArrayVector3", "_ogre20_changes.html#InterpretingArrayVector3", null ],
            [ "Dummy pointers instead of NULL", "_ogre20_changes.html#DummyPointers", null ]
          ] ],
          [ "Attachment and Visibility", "_ogre20_changes.html#AttachmentAndVisibility", null ],
          [ "Attaching/Detaching is more expensive than hiding", "_ogre20_changes.html#AttachingDetachingIsMoreExpensive", null ],
          [ "All MovableObjects require a SceneNode (Lights & Cameras)", "_ogre20_changes.html#AllMovableObjectsRequireSceneNode", null ],
          [ "Obtaining derived transforms", "_ogre20_changes.html#DerivedTransforms", null ],
          [ "SCENE_STATIC and SCENE_DYNAMIC", "_ogre20_changes.html#SceneStaticSceneDynamic", [
            [ "What means a Node to be SCENE_STATIC", "_ogre20_changes.html#SceneStaticNode", null ],
            [ "What means a Entities (and InstancedEntities) to be SCENE_STATIC", "_ogre20_changes.html#SceneStaticEntities", null ],
            [ "General", "_ogre20_changes.html#SceneStaticGeneral", null ]
          ] ],
          [ "Ogre asserts mCachedAabbOutOfDate or mCachedTransformOutOfDate while in debug mode", "_ogre20_changes.html#AssersionCachedOutOfDate", null ],
          [ "Custom classes derived from Renderable or MovableObject", "_ogre20_changes.html#DerivingRenderable", null ],
          [ "How do I get the vertex information from the new v2 Mesh classes?", "_ogre20_changes.html#V2MeshInformation", null ],
          [ "How do I set the element offsets, vertex buffer's source and index?", "_ogre20_changes.html#V2MeshElementOffset", null ],
          [ "My scene looks too dark or dull!", "_ogre20_changes.html#SceneLooksDarkDull", null ],
          [ "I activated gamma correction, but now my GUI textures look washed out!", "_ogre20_changes.html#GUIWashedOut", null ]
        ] ],
        [ "AZDO changes (Aproaching Zero Driver Overhead)", "azdo.html", [
          [ "V2 and v1 objects", "azdo.html#V2AndV1Objects", [
            [ "Longevity of the v1 objects and deprecation", "azdo.html#V2AndV1ObjectsV1Longevity", null ]
          ] ],
          [ "Render Queue", "azdo.html#RenderQueue", null ],
          [ "The VaoManager", "azdo.html#VaoMaanger", null ]
        ] ]
      ] ],
      [ "Rendering", "_rendering.html", [
        [ "Advanced Users", "_rendering.html#autotoc_md105", null ],
        [ "HLMS: High Level Material System", "hlms.html", [
          [ "Fundamental changes", "hlms.html#HlmsChanges", [
            [ "Viewports and Scissor tests", "hlms.html#HlmsChangesViewports", null ],
            [ "A lot of data is stored in \"Blocks\"", "hlms.html#HlmsChangesBlocks", null ],
            [ "Materials are still alive", "hlms.html#HlmsChangesMaterialsAlive", null ],
            [ "Fixed Function has been removed", "hlms.html#HlmsChangesFFP", null ]
          ] ],
          [ "The three components", "hlms.html#HlmsComponents", null ],
          [ "Blocks", "hlms.html#HlmsBlocks", [
            [ "Datablocks", "hlms.html#HlmsBlocksDatablocks", null ],
            [ "Macroblocks", "hlms.html#HlmsBlocksMacroblocks", null ],
            [ "Blendblocks", "hlms.html#HlmsBlocksBlendblocks", null ],
            [ "Samplerblocks", "hlms.html#HlmsBlocksSampleblocks", null ]
          ] ],
          [ "Hlms templates", "hlms.html#HlmsTemplates", null ],
          [ "The Hlms preprocessor", "hlms.html#HlmsPreprocessor", [
            [ "Preprocessor syntax", "hlms.html#HlmsPreprocessorSyntax", [
              [ "@property( expression )", "hlms.html#HlmsPreprocessorSyntaxProperty", null ],
              [ "@foreach( count, scopedVar, [start] )", "hlms.html#HlmsPreprocessorSyntaxForeach", null ],
              [ "@counter( variable )", "hlms.html#HlmsPreprocessorSyntaxCounter", null ],
              [ "@value( variable )", "hlms.html#HlmsPreprocessorSyntaxValue", null ],
              [ "@set add sub mul div mod min max", "hlms.html#HlmsPreprocessorSyntaxSetEtc", null ],
              [ "@piece( nameOfPiece )", "hlms.html#HlmsPreprocessorSyntaxPiece", null ],
              [ "@insertpiece( nameOfPiece )", "hlms.html#HlmsPreprocessorSyntaxInsertpiece", null ],
              [ "@undefpiece( nameOfPiece )", "hlms.html#HlmsPreprocessorSyntaxUndefpiece", null ],
              [ "@pset padd psub pmul pdiv pmod pmin pmax", "hlms.html#HlmsPreprocessorSyntaxPsetEtc", null ]
            ] ]
          ] ],
          [ "Creation of shaders", "hlms.html#HlmsCreationOfShaders", [
            [ "C++ interaction with shader templates", "hlms.html#HlmsCreationOfShadersCpp", null ],
            [ "Common conventions", "hlms.html#HlmsCreationOfShadersConventions", null ],
            [ "Hot reloading", "hlms.html#HlmsCreationOfShadersHotReloading", null ],
            [ "Disabling a stage", "hlms.html#HlmsCreationOfShadersDisablingStage", null ],
            [ "Customizing an existing implementation", "hlms.html#HlmsCreationOfShadersCustomizing", null ]
          ] ],
          [ "Run-time rendering", "hlms.html#HlmsRuntimeRendering", [
            [ "preparePassHash", "hlms.html#HlmsRuntimeRenderingPreparePassHash", null ],
            [ "fillBuffersFor", "hlms.html#HlmsRuntimeRenderingFillBuffersFor", null ],
            [ "Multithreaded Shader Compilation", "hlms.html#autotoc_md109", null ]
          ] ],
          [ "Using the HLMS implementations", "hlms.html#UsingHlmsImplementation", [
            [ "Initialization", "hlms.html#UsingHlmsImplementationInitialization", null ],
            [ "Deinitialization", "hlms.html#UsingHlmsImplementationDeinitilization", null ],
            [ "Creating a datablock", "hlms.html#UsingHlmsImplementationCreatingDatablock", null ]
          ] ],
          [ "The Hlms Texture Manager", "hlms.html#HlmsTextureManager", [
            [ "Automatic batching", "hlms.html#HlmsTextureManagerAutomaticBatching", [
              [ "Texture types", "hlms.html#HlmsTextureManagerAutomaticBatchingTextureTypes", null ],
              [ "Automatic parameters", "hlms.html#HlmsTextureManagerAutomaticBatchingAutoParams", null ],
              [ "Loading a texture twice (i.e. with a different format) via aliasing", "hlms.html#HlmsTextureManagerAutomaticBatchingLoadingTwice", null ]
            ] ],
            [ "Manual: Texture packs", "hlms.html#HlmsTextureManagerTexturePacks", null ],
            [ "Watching out for memory consumption", "hlms.html#HlmsTextureManagerWatchOutMemory", [
              [ "Additional memory considerations", "hlms.html#HlmsTextureManagerWatchOutMemoryConsiderations", null ],
              [ "setWorkerThreadMinimumBudget warning", "hlms.html#setWorkerThreadMinimumBudget", null ]
            ] ]
          ] ],
          [ "Troubleshooting", "hlms.html#HlmsTroubleshooting", [
            [ "My shadows don't show up or are very glitchy", "hlms.html#HlmsTroubleshootingShadow", null ]
          ] ],
          [ "Precision / Quality", "hlms.html#autotoc_md110", null ],
          [ "Multithreaded Shader Compilation", "_hlms_threading.html", [
            [ "CMake Options", "_hlms_threading.html#HlmsThreading_CMakeOptions", null ],
            [ "The tid (Thread ID) argument", "_hlms_threading.html#HlmsThreading_tidArgument", [
              [ "API when OGRE_SHADER_COMPILATION_THREADING_MODE = 1", "_hlms_threading.html#autotoc_md98", null ]
            ] ],
            [ "How does threaded Hlms work?", "_hlms_threading.html#autotoc_md99", [
              [ "What is the range of tid argument?", "_hlms_threading.html#autotoc_md100", null ]
            ] ]
          ] ]
        ] ],
        [ "Global Illumination Methods", "_gi_methods.html", [
          [ "Ambient Lighting", "_gi_methods.html#GiAmbientLighting", [
            [ "Flat", "_gi_methods.html#GiAmbientLightingFlat", null ],
            [ "Hemisphere", "_gi_methods.html#GiAmbientLightingHemisphere", null ],
            [ "Spherical Harmonics", "_gi_methods.html#GiAmbientLightingSH", null ]
          ] ],
          [ "Parallax Corrected Cubemaps (PCC)", "_gi_methods.html#GiPCC", [
            [ "Auto PCC", "_gi_methods.html#GiPCCAuto", null ],
            [ "Manual PCC", "_gi_methods.html#GiPCCManual", null ]
          ] ],
          [ "Per Pixel PCC", "_gi_methods.html#GiPPPCC", [
            [ "PCC Per Pixel Grid Placement", "_gi_methods.html#GiPPGridPlacement", null ]
          ] ],
          [ "Instant Radiosity", "_gi_methods.html#GiInstantRadiosity", null ],
          [ "Irradiance Volume", "_gi_methods.html#GiIrradianceVolume", null ],
          [ "Voxel Cone Tracing (aka VCT)", "_gi_methods.html#GiVCT", [
            [ "VCT + Per Pixel PCC Hybrid", "_gi_methods.html#GiVCTPlusPPPCC", null ]
          ] ],
          [ "Irradiance Field with Depth (IFD)", "_gi_methods.html#GiIFD", null ],
          [ "Cascaded Image Voxel Cone Tracing (CIVCT)", "_gi_methods.html#GiCIVCT", null ],
          [ "What technique should I choose?", "_gi_methods.html#GiWhatTechniqueChoose", null ],
          [ "Image Voxel Cone Tracing", "_image_voxel_cone_tracing.html", [
            [ "Step 1: Image Voxelizer", "_image_voxel_cone_tracing.html#IVCT_Step1", [
              [ "Downside", "_image_voxel_cone_tracing.html#IVCT_Step1_Downside", [
                [ "Non-researched solutions:", "_image_voxel_cone_tracing.html#autotoc_md101", null ]
              ] ],
              [ "Trivia", "_image_voxel_cone_tracing.html#autotoc_md102", null ]
            ] ],
            [ "Step 2: Row Translation", "_image_voxel_cone_tracing.html#IVCT_Step2", null ],
            [ "Step 3: Cascades", "_image_voxel_cone_tracing.html#IVCT_Step3", null ],
            [ "Wait isn't this what UE5's Lumen does?", "_image_voxel_cone_tracing.html#autotoc_md103", null ],
            [ "Wait isn't this what Godot does?", "_image_voxel_cone_tracing.html#autotoc_md104", null ]
          ] ]
        ] ],
        [ "The Command Buffer", "commandbuffer.html", [
          [ "Adding a command", "commandbuffer.html#CommandBufferAddCommand", null ],
          [ "Structure of a command", "commandbuffer.html#CommandBufferCommandStructure", null ],
          [ "Execution table", "commandbuffer.html#CommandBufferExecutionTable", [
            [ "Hacks and Tricks", "commandbuffer.html#CommandBufferExecutionTableHacks", null ]
          ] ],
          [ "Post-processing the command buffer", "commandbuffer.html#CommandBufferPostProcessing", null ]
        ] ],
        [ "Root Layouts", "_root_layouts.html", [
          [ "Old APIs (i.e. D3D11 and GL)", "_root_layouts.html#RootLayoutsOldAPIs", null ],
          [ "New APIs and Root Layouts", "_root_layouts.html#RootLayoutsNewAPIs", null ],
          [ "Setting up root layouts", "_root_layouts.html#RootLayoutsSettingUp", [
            [ "Could you have used e.g. \"const_buffers\" : [0,7] instead of [4,7]?", "_root_layouts.html#autotoc_md106", null ]
          ] ],
          [ "Declaring Root Layouts in shaders", "_root_layouts.html#RootLayoutsDeclaringInShaders", null ],
          [ "Baked sets", "_root_layouts.html#RootLayoutsBakedSets", null ],
          [ "Prefab Root Layouts for low level materials", "_root_layouts.html#RootLayoutPrefabs", null ],
          [ "Arrays of Textures", "_root_layouts.html#RootLayoutsArraysOfTextures", [
            [ "C++", "_root_layouts.html#RootLayoutsAoTCpp", null ],
            [ "Inline shader declaration", "_root_layouts.html#RootLayoutsAoTInlineShader", null ],
            [ "Automatic", "_root_layouts.html#RootLayoutsAoTAuto", null ],
            [ "Making GLSL shaders compatible with both Vulkan and OpenGL", "_root_layouts.html#RootLayoutsGLSLForGLandVK", [
              [ "Example:", "_root_layouts.html#RootLayoutsGLSLForGLandVKExample", [
                [ "OpenGL", "_root_layouts.html#autotoc_md107", null ],
                [ "Vulkan", "_root_layouts.html#autotoc_md108", null ]
              ] ]
            ] ]
          ] ]
        ] ]
      ] ],
      [ "Scripts", "_scripts.html", [
        [ "Loading scripts", "_scripts.html#autotoc_md120", null ],
        [ "Format", "_scripts.html#Format", [
          [ "Script Inheritance", "_scripts.html#Script-Inheritance", [
            [ "Advanced Script Inheritance", "_scripts.html#Advanced-Script-Inheritance", null ]
          ] ],
          [ "Script Variables", "_scripts.html#Script-Variables", null ],
          [ "Script Import Directive", "_scripts.html#Script-Import-Directive", null ]
        ] ],
        [ "Custom Translators", "_scripts.html#custom-translators", null ],
        [ "JSON Materials", "_j_s_o_n-_materials.html", [
          [ "Reference Guide: HLMS Blendblock", "hlmsblendblockref.html", [
            [ "Parameters:", "hlmsblendblockref.html#bbParameters", [
              [ "Parameter: alpha_to_coverage", "hlmsblendblockref.html#bbParamAlphaToCoverage", null ],
              [ "Parameter: blendmask", "hlmsblendblockref.html#bbParamBlendmask", null ],
              [ "Parameter: separate_blend", "hlmsblendblockref.html#bbParamSeperateBlend", null ],
              [ "Parameter: src_blend_factor", "hlmsblendblockref.html#bbParamSorceBlendFactor", null ],
              [ "Parameter: dst_blend_factor", "hlmsblendblockref.html#bbParamDestBlendFactor", null ],
              [ "Parameter: src_alpha_blend_factor", "hlmsblendblockref.html#bbParamSourceAlphaBlendFactor", null ],
              [ "Parameter: dst_alpha_blend_factor", "hlmsblendblockref.html#bbParamDestinationAlphaBlendFactor", null ],
              [ "Parameter: blend_operation", "hlmsblendblockref.html#bbParamBlendOperation", null ],
              [ "Parameter: blend_operation_alpha", "hlmsblendblockref.html#bbParamBlendOperationAlpha", null ]
            ] ],
            [ "Example Blendblock Definition:", "hlmsblendblockref.html#bbExample", null ],
            [ "Scene blend type settings:", "hlmsblendblockref.html#bbBlendtypes", [
              [ "Type: Transparent Alpha", "hlmsblendblockref.html#btTransparentAlpha", null ],
              [ "Type: Transparent Colour", "hlmsblendblockref.html#btTransparentColour", null ],
              [ "Type: Add", "hlmsblendblockref.html#btAdd", null ],
              [ "Type: Modulate", "hlmsblendblockref.html#btModulate", null ],
              [ "Type: Replace", "hlmsblendblockref.html#btReplace", null ]
            ] ],
            [ "Links", "hlmsblendblockref.html#bbLinks", null ]
          ] ],
          [ "Reference Guide: HLMS Macroblock", "hlmsmacroblockref.html", [
            [ "Parameters:", "hlmsmacroblockref.html#mbParameters", [
              [ "Parameter: scissor_test", "hlmsmacroblockref.html#mbParamScissorText", null ],
              [ "Parameter: depth_check", "hlmsmacroblockref.html#mbParamDepthCheck", null ],
              [ "Parameter: depth_write", "hlmsmacroblockref.html#mbParamDepthWrite", null ],
              [ "Parameter: depth_function", "hlmsmacroblockref.html#mbParamDepthFunction", null ],
              [ "Parameter: depth_bias_constant", "hlmsmacroblockref.html#mbParamDepthBiasConstant", null ],
              [ "Parameter: depth_bias_slope_scale", "hlmsmacroblockref.html#mbParamDepthBiasSlopeScale", null ],
              [ "Parameter: cull_mode", "hlmsmacroblockref.html#mbParamCullMode", null ],
              [ "Parameter: polygon_mode", "hlmsmacroblockref.html#mbParamPolygonMode", null ]
            ] ],
            [ "Example Macroblock Definition:", "hlmsmacroblockref.html#mbExample", null ],
            [ "Links", "hlmsmacroblockref.html#mbLinks", null ]
          ] ],
          [ "Reference Guide: HLMS Samplerblock", "hlmssamplerref.html", [
            [ "Parameters:", "hlmssamplerref.html#sbParameters", [
              [ "Parameter: min", "hlmssamplerref.html#sbParamMin", null ],
              [ "Parameter: mag", "hlmssamplerref.html#sbParamMag", null ],
              [ "Parameter: mip", "hlmssamplerref.html#sbParamMip", null ],
              [ "Parameter: u", "hlmssamplerref.html#sbParamU", null ],
              [ "Parameter: v", "hlmssamplerref.html#sbParamV", null ],
              [ "Parameter: w", "hlmssamplerref.html#sbParamW", null ],
              [ "Parameter: miplodbias", "hlmssamplerref.html#sbParamMiplodbias", null ],
              [ "Parameter: max_anisotropic", "hlmssamplerref.html#sbParamMaxAnisotropic", null ],
              [ "Parameter: compare_function", "hlmssamplerref.html#sbParamComparefunction", null ],
              [ "Parameter: border", "hlmssamplerref.html#sbParamBorder", null ],
              [ "Parameter: min_lod", "hlmssamplerref.html#sbParamMinlod", null ],
              [ "Parameter: max_lod", "hlmssamplerref.html#sbParamMaxlod", null ]
            ] ],
            [ "Example Samplerblock Definition:", "hlmssamplerref.html#sbExample", null ],
            [ "Links", "hlmssamplerref.html#sbLinks", null ]
          ] ],
          [ "Reference Guide: HLMS PBS Datablock", "hlmspbsdatablockref.html", [
            [ "Common Datablock Parameters:", "hlmspbsdatablockref.html#dbCommonParameters", [
              [ "Parameter: accurate_non_uniform_normal_scaling", "hlmspbsdatablockref.html#dbParamAccurateNonUniformNormalScaling", null ],
              [ "Parameter: alpha_test", "hlmspbsdatablockref.html#dbParamAlphaTest", null ],
              [ "Parameter: blendblock", "hlmspbsdatablockref.html#dbParamBlendBlock", null ],
              [ "Parameter: macroblock", "hlmspbsdatablockref.html#dbParamMacroBlock", null ],
              [ "Parameter: shadow_const_bias", "hlmspbsdatablockref.html#dbParamShadowConstBias", null ]
            ] ],
            [ "PBS Datablock Parameters", "hlmspbsdatablockref.html#dbPBSParameters", [
              [ "Parameter: brdf", "hlmspbsdatablockref.html#dbParamBRDF", null ],
              [ "Parameter: refraction_strength", "hlmspbsdatablockref.html#autotoc_md111", null ],
              [ "Parameter: detail_diffuse[X]", "hlmspbsdatablockref.html#dbParamDetailDiffuse", null ],
              [ "Parameter: detail_normal[X]", "hlmspbsdatablockref.html#dbParamDetailNormal", null ],
              [ "Parameter: detail_weight", "hlmspbsdatablockref.html#dbParamDetailWeight", null ],
              [ "Parameter: diffuse", "hlmspbsdatablockref.html#dbParamDiffuse", null ],
              [ "Parameter: emissive", "hlmspbsdatablockref.html#dbParamEmissive", null ],
              [ "Parameter: fresnel", "hlmspbsdatablockref.html#dbParamFresnel", null ],
              [ "Parameter: metalness", "hlmspbsdatablockref.html#dbParamMetalness", null ],
              [ "Parameter: normal", "hlmspbsdatablockref.html#dbParamNormal", null ],
              [ "Parameter: receive_shadows", "hlmspbsdatablockref.html#dbParamReceiveShadows", null ],
              [ "Parameter: reflection", "hlmspbsdatablockref.html#dbParamReflection", null ],
              [ "Parameter: roughness", "hlmspbsdatablockref.html#dbParamRoughness", null ],
              [ "Parameter: specular", "hlmspbsdatablockref.html#dbParamSpecular", null ],
              [ "Parameter: transparency", "hlmspbsdatablockref.html#dbParamTransparency", null ],
              [ "Parameter: two_sided", "hlmspbsdatablockref.html#dbParamTwoSided", null ],
              [ "Parameter: workflow", "hlmspbsdatablockref.html#dbParamWorkflow", null ]
            ] ],
            [ "Example PBS Datablock Definition:", "hlmspbsdatablockref.html#dbExample", [
              [ "Links", "hlmspbsdatablockref.html#dbLinks", null ]
            ] ]
          ] ],
          [ "Reference Guide: HLMS Unlit Datablock", "hlmsunlitdatablockref.html", [
            [ "Common Datablock Parameters:", "hlmsunlitdatablockref.html#dbulCommonParameters", [
              [ "Parameter: alpha_test", "hlmsunlitdatablockref.html#dbulParamAlphaTest", null ],
              [ "Parameter: blendblock", "hlmsunlitdatablockref.html#dbulParamBlendBlock", null ],
              [ "Parameter: macroblock", "hlmsunlitdatablockref.html#dbulParamMacroBlock", null ]
            ] ],
            [ "Unlit Datablock Parameters", "hlmsunlitdatablockref.html#dbulUnlitParameters", [
              [ "Parameter: diffuse", "hlmsunlitdatablockref.html#dbulParamDiffuse", null ],
              [ "Parameter: diffuse_map[X]", "hlmsunlitdatablockref.html#dbulParamDetailNormal", null ]
            ] ],
            [ "Example Unlit Datablock Definition:", "hlmsunlitdatablockref.html#dbulExample", [
              [ "Links", "hlmsunlitdatablockref.html#dbulLinks", null ]
            ] ]
          ] ],
          [ "Reference Guide: HLMS Terra Datablock", "hlmsterradatablockref.html", [
            [ "Common Datablock Parameters:", "hlmsterradatablockref.html#dbtCommonParameters", null ],
            [ "Terra Datablock Parameters", "hlmsterradatablockref.html#dbtTerraParameters", [
              [ "Parameter: brdf", "hlmsterradatablockref.html#dbtParamBRDF", null ],
              [ "Parameter: detail[X]", "hlmsterradatablockref.html#dbtParamDetailDiffuse", null ],
              [ "Parameter: detail_weight", "hlmsterradatablockref.html#dbtParamDetailWeight", null ],
              [ "Parameter: diffuse", "hlmsterradatablockref.html#dbtParamDiffuse", null ],
              [ "Parameter: reflection", "hlmsterradatablockref.html#dbtParamReflection", null ]
            ] ],
            [ "Example Terra Datablock Definition:", "hlmsterradatablockref.html#dbtExample", [
              [ "Links", "hlmsterradatablockref.html#dbtLinks", null ]
            ] ]
          ] ]
        ] ],
        [ "Compositor", "compositor.html", [
          [ "Nodes", "compositor.html#CompositorNodes", [
            [ "Input & output channels and RTTs", "compositor.html#CompositorNodesChannelsAndRTTs", [
              [ "Locally declared textures", "compositor.html#CompositorNodesChannelsAndRTTsLocalTextures", null ],
              [ "Textures coming from input channels", "compositor.html#CompositorNodesChannelsAndRTTsFromInputChannel", null ],
              [ "Global Textures", "compositor.html#CompositorNodesChannelsAndRTTsGlobal", null ],
              [ "compositor_node parameters", "compositor.html#autotoc_md113", [
                [ "in", "compositor.html#CompositorNode_in", null ],
                [ "out", "compositor.html#CompositorNode_out", null ],
                [ "in_buffer", "compositor.html#CompositorNode_in_buffer", null ],
                [ "out_buffer", "compositor.html#CompositorNode_out_buffer", null ],
                [ "custom_id", "compositor.html#CompositorNode_custom_id", null ]
              ] ],
              [ "Main RenderTarget", "compositor.html#CompositorNodesChannelsAndRTTsMainRenderTarget", null ]
            ] ],
            [ "Target", "compositor.html#CompositorNodesTarget", [
              [ "target parameters", "compositor.html#autotoc_md114", [
                [ "target_level_barrier", "compositor.html#CompositorTarget_target_level_barrier", null ]
              ] ]
            ] ],
            [ "Passes", "compositor.html#CompositorNodesPasses", [
              [ "pass parameters", "compositor.html#autotoc_md115", [
                [ "pass", "compositor.html#CompositorPass_pass", null ],
                [ "num_initial", "compositor.html#CompositorPass_num_initial", null ],
                [ "flush_command_buffers", "compositor.html#CompositorPass_flush_command_buffers", null ],
                [ "identifier", "compositor.html#CompositorPass_identifier", null ],
                [ "execution_mask", "compositor.html#CompositorPass_execution_mask", null ],
                [ "viewport_modifier_mask", "compositor.html#CompositorPass_viewport_modifier_mask", null ],
                [ "colour_write", "compositor.html#CompositorPass_colour_write", null ],
                [ "profiling_id", "compositor.html#CompositorPass_profiling_id", null ],
                [ "viewport", "compositor.html#CompositorPass_viewport", null ],
                [ "expose", "compositor.html#CompositorPass_expose", null ],
                [ "skip_load_store_semantics", "compositor.html#CompositorPass_skip_load_store_semantics", null ],
                [ "load", "compositor.html#CompositorPass_load", null ],
                [ "all (load)", "compositor.html#CompositorPass_load_all", null ],
                [ "colour (load)", "compositor.html#CompositorPass_load_colour", null ],
                [ "depth (load)", "compositor.html#CompositorPass_load_depth", null ],
                [ "stencil (load)", "compositor.html#CompositorPass_load_stencil", null ],
                [ "clear_colour / colour_value (load)", "compositor.html#CompositorPass_load_clear_colour", null ],
                [ "clear_colour_reverse_depth_aware (load)", "compositor.html#CompositorPass_load_clear_colour_reverse_depth_aware", null ],
                [ "clear_depth (load)", "compositor.html#CompositorPass_load_clear_depth", null ],
                [ "clear_stencil (load)", "compositor.html#CompositorPass_load_clear_stencil", null ],
                [ "warn_if_rtv_was_flushed (load)", "compositor.html#CompositorPass_load_warn_if_rtv_was_flushed", null ],
                [ "store", "compositor.html#CompositorPass_store", null ],
                [ "all (store)", "compositor.html#CompositorPass_store_all", null ],
                [ "colour (store)", "compositor.html#CompositorPass_store_colour", null ],
                [ "depth (store)", "compositor.html#CompositorPass_store_depth", null ],
                [ "stencil (store)", "compositor.html#CompositorPass_store_stencil", null ]
              ] ],
              [ "clear", "compositor.html#CompositorNodesPassesClear", [
                [ "non_tilers_only", "compositor.html#CompositorPass_clear_non_tilers_only", null ],
                [ "buffers", "compositor.html#CompositorPass_clear_buffers", null ]
              ] ],
              [ "generate_mipmaps", "compositor.html#CompositorNodesPassesGenerateMipmaps", [
                [ "mipmap_method", "compositor.html#CompositorNodesPassesGenerateMipmaps_mipmap_method", null ],
                [ "kernel_radius", "compositor.html#CompositorNodesPassesGenerateMipmaps_kernel_radius", null ],
                [ "gauss_deviation", "compositor.html#CompositorNodesPassesGenerateMipmaps_gauss_deviation", null ]
              ] ],
              [ "render_quad", "compositor.html#CompositorNodesPassesQuad", [
                [ "use_quad", "compositor.html#CompositorNodesPassesQuad_use_quad", null ],
                [ "material", "compositor.html#CompositorNodesPassesQuad_material", null ],
                [ "hlms", "compositor.html#CompositorNodesPassesQuad_hlms", null ],
                [ "input", "compositor.html#CompositorNodesPassesQuad_input", null ],
                [ "quad_normals", "compositor.html#CompositorNodesPassesQuad_quad_normals", null ],
                [ "camera", "compositor.html#CompositorNodesPassesQuad_camera", null ],
                [ "camera_cubemap_reorient", "compositor.html#CompositorNodesPassesQuad_camera_cubemap_reorient", null ]
              ] ],
              [ "render_scene", "compositor.html#CompositorNodesPassesRenderScene", [
                [ "rq_first", "compositor.html#CompositorNodesPassesRenderScene_rq_first", null ],
                [ "rq_last", "compositor.html#CompositorNodesPassesRenderScene_rq_last", null ],
                [ "lod_bias", "compositor.html#CompositorNodesPassesRenderScene_lod_bias", null ],
                [ "lod_update_list", "compositor.html#CompositorNodesPassesRenderScene_lod_update_list", null ],
                [ "cull_reuse_data", "compositor.html#CompositorNodesPassesRenderScene_cull_reuse_data", null ],
                [ "visibility_mask", "compositor.html#CompositorNodesPassesRenderScene_visibility_mask", null ],
                [ "light_visibility_mask", "compositor.html#CompositorNodesPassesRenderScene_light_visibility_mask", null ],
                [ "shadows", "compositor.html#CompositorNodesPassesRenderScene_shadows", null ],
                [ "overlays", "compositor.html#CompositorNodesPassesRenderScene_overlays", null ],
                [ "camera", "compositor.html#CompositorNodesPassesRenderScene_camera", null ],
                [ "lod_camera", "compositor.html#CompositorNodesPassesRenderScene_lod_camera", null ],
                [ "cull_camera", "compositor.html#CompositorNodesPassesRenderScene_cull_camera", null ],
                [ "camera_cubemap_reorient", "compositor.html#CompositorNodesPassesRenderScene_camera_cubemap_reorient", null ],
                [ "enable_forwardplus", "compositor.html#CompositorNodesPassesRenderScene_enable_forwardplus", null ],
                [ "flush_command_buffers_after_shadow_node", "compositor.html#CompositorNodesPassesRenderScene_flush_command_buffers_after_shadow_node", null ],
                [ "is_prepass", "compositor.html#CompositorNodesPassesRenderScene_is_prepass", null ],
                [ "use_prepass", "compositor.html#CompositorNodesPassesRenderScene_use_prepass", null ],
                [ "gen_normals_gbuffer", "compositor.html#CompositorNodesPassesRenderScene_gen_normals_gbuffer", null ]
              ] ],
              [ "use_refractions", "compositor.html#CompositorNodesPassesRenderScene_use_refractions", [
                [ "uv_baking", "compositor.html#CompositorNodesPassesRenderScene_uv_baking", null ],
                [ "uv_baking_offset", "compositor.html#CompositorNodesPassesRenderScene_uv_baking_offset", null ],
                [ "bake_lighting_only", "compositor.html#CompositorNodesPassesRenderScene_bake_lighting_only", null ],
                [ "instanced_stereo", "compositor.html#CompositorNodesPassesRenderScene_instanced_stereo", null ]
              ] ],
              [ "shadows", "compositor.html#CompositorNodesPassesShadows", null ],
              [ "stencil", "compositor.html#CompositorNodesPassesStencil", null ],
              [ "uav_queue", "compositor.html#CompositorNodesPassesUavQueue", [
                [ "starting_slot", "compositor.html#CompositorNodesPassesUavQueue_starting_slot", null ],
                [ "uav", "compositor.html#CompositorNodesPassesUavQueue_uav", null ],
                [ "uav_external", "compositor.html#CompositorNodesPassesUavQueue_uav_external", null ],
                [ "uav_buffer", "compositor.html#CompositorNodesPassesUavQueue_uav_buffer", null ],
                [ "keep_previous_uavs", "compositor.html#CompositorNodesPassesUavQueue_keep_previous_uavs", null ]
              ] ],
              [ "compute", "compositor.html#CompositorNodesPassesCompute", [
                [ "job", "compositor.html#CompositorPassCompute_job", null ],
                [ "uav", "compositor.html#CompositorPassCompute_uav", null ],
                [ "uav_buffer", "compositor.html#CompositorPassCompute_uav_buffer", null ],
                [ "input", "compositor.html#CompositorPassCompute_input", null ]
              ] ],
              [ "texture_copy / depth_copy", "compositor.html#CompositorNodesPassesDepthCopy", [
                [ "in", "compositor.html#CompositorPassDepthCopy_in", null ],
                [ "out", "compositor.html#CompositorPassDepthCopy_out", null ],
                [ "mip_range", "compositor.html#CompositorPassDepthCopy_mip_range", null ]
              ] ]
            ] ],
            [ "texture", "compositor.html#CompositorNodesTextures", [
              [ "MSAA: Explicit vs Implicit resolves", "compositor.html#CompositorNodesTexturesMsaa", [
                [ "Implicit resolves", "compositor.html#CompositorNodesTexturesMsaaImplicit", null ],
                [ "Explicit resolves", "compositor.html#CompositorNodesTexturesMsaaExplicit", null ]
              ] ],
              [ "Depth Textures", "compositor.html#CompositorNodesTexturesDepth", null ]
            ] ]
          ] ],
          [ "Shadow Nodes", "compositor.html#CompositorShadowNodes", [
            [ "Setting up shadow nodes", "compositor.html#CompositorShadowNodesSetup", null ],
            [ "Example", "compositor.html#CompositorShadowNodesExample", null ],
            [ "Shadow map atlas & Point Lights", "compositor.html#CompositorShadowNodesAtlasAndPointLights", null ],
            [ "Reuse, recalculate and first", "compositor.html#CompositorShadowNodesReuseEtc", null ],
            [ "Shadow mapping setup types", "compositor.html#CompositorShadowNodesTypes", [
              [ "Uniform shadow mapping", "compositor.html#CompositorShadowNodesTypesUniform", null ],
              [ "Focused", "compositor.html#CompositorShadowNodesTypesFocused", null ],
              [ "PSSM / CSM", "compositor.html#CompositorShadowNodesTypesPssm", null ],
              [ "Plane Optimal", "compositor.html#CompositorShadowNodesTypesPlaneOptimal", null ]
            ] ]
          ] ],
          [ "Workspaces", "compositor.html#CompositorWorkspaces", [
            [ "Data dependencies between nodes and circular dependencies", "compositor.html#CompositorWorkspacesDataDependencies", null ]
          ] ],
          [ "Setting up code", "compositor.html#CompositorSetupCode", [
            [ "Initializing the workspace", "compositor.html#CompositorWorkspacesSetupInitialize", null ],
            [ "Simple bootstrap for beginners", "compositor.html#CompositorWorkspacesSetupSimple", null ],
            [ "Advanced C++ users", "compositor.html#CompositorWorkspacesSetupAdvanced", null ]
          ] ],
          [ "Stereo and Split-Screen Rendering", "compositor.html#StereoAndSplitScreenRendering", [
            [ "Per-Workspace offset and scale", "compositor.html#CompositorWorkspacesStereoPerWorkspace", null ],
            [ "Viewport modifier mask", "compositor.html#CompositorWorkspacesStereoViewportMask", null ],
            [ "Execution mask", "compositor.html#CompositorWorkspacesStereoExecutionMask", null ],
            [ "Default values", "compositor.html#CompositorWorkspacesStereoDefaultValues", null ]
          ] ],
          [ "Advanced MSAA", "compositor.html#AdvancedMSAA", [
            [ "What is MSAA?", "compositor.html#autotoc_md116", [
              [ "Supersampling Antialiasing (SSAA) vs MSAA", "compositor.html#autotoc_md117", null ],
              [ "MSAA approach to the problem", "compositor.html#autotoc_md118", [
                [ "Resources", "compositor.html#CompositorNodesTexturesMsaaResources", null ]
              ] ]
            ] ],
            [ "Ogre + MSAA with Implicit Resolves", "compositor.html#autotoc_md119", null ],
            [ "Ogre + MSAA with Explicit Resolves", "compositor.html#MSAAExplicitResolves", null ]
          ] ],
          [ "RTV (RenderTargetView)", "compositor.html#CompositorRTV", [
            [ "What is an RTV", "compositor.html#CompositorWhatisAnRtv", null ],
            [ "RTV settings", "compositor.html#CompositorRtvSettings", null ],
            [ "RTV Examples", "compositor.html#CompositorRtv_Examples", null ]
          ] ]
        ] ],
        [ "Particle Scripts", "_particle-_scripts.html", [
          [ "Particle System Attributes", "_particle-_scripts.html#Particle-System-Attributes", [
            [ "quota", "_particle-_scripts.html#ParticleSystemAttributes_quota", null ],
            [ "material", "_particle-_scripts.html#ParticleSystemAttributes_material", null ],
            [ "particle_width", "_particle-_scripts.html#ParticleSystemAttributes_particle_width", null ],
            [ "particle_height", "_particle-_scripts.html#ParticleSystemAttributes_particle_height", null ],
            [ "cull_each", "_particle-_scripts.html#ParticleSystemAttributes_cull_each", null ],
            [ "renderer", "_particle-_scripts.html#ParticleSystemAttributes_renderer", null ],
            [ "sorted", "_particle-_scripts.html#ParticleSystemAttributes_sorted", null ],
            [ "local_space", "_particle-_scripts.html#ParticleSystemAttributes_local_space", null ],
            [ "iteration_interval", "_particle-_scripts.html#ParticleSystemAttributes_iteration_interval", null ],
            [ "nonvisible_update_timeout", "_particle-_scripts.html#ParticleSystemAttributes_nonvisible_update_timeout", null ]
          ] ],
          [ "Billboard Renderer Attributes", "_particle-_scripts.html#Billboard-Renderer-Attributes", [
            [ "billboard_type", "_particle-_scripts.html#BillboardRendererAttributes_billboard_type", null ],
            [ "billboard_origin", "_particle-_scripts.html#BillboardRendererAttributes_origin", null ],
            [ "billboard_rotation_type", "_particle-_scripts.html#BillboardRendererAttributes_billboard_rotation_type", null ],
            [ "common_direction", "_particle-_scripts.html#BillboardRendererAttributes_common_direction", null ],
            [ "common_up_vector", "_particle-_scripts.html#BillboardRendererAttributes_common_up_vector", null ],
            [ "point_rendering", "_particle-_scripts.html#BillboardRendererAttributes_point_rendering", null ],
            [ "accurate_facing", "_particle-_scripts.html#BillboardRendererAttributes_accurate_facing", null ],
            [ "texture_sheet_size", "_particle-_scripts.html#BillboardRendererAttributes_texture_sheet_size", null ]
          ] ],
          [ "Particle Emitters", "_particle-_scripts.html#Particle-Emitters", [
            [ "Emitting Emitters", "_particle-_scripts.html#Emitting-Emitters", null ],
            [ "Common Emitter Attributes", "_particle-_scripts.html#autotoc_md112", null ],
            [ "angle", "_particle-_scripts.html#ParticleEmitterAttributes_angle", null ],
            [ "colour", "_particle-_scripts.html#ParticleEmitterAttributes_colour", null ],
            [ "colour_range_start & colour_range_end", "_particle-_scripts.html#ParticleEmitterAttributes_colour_range_start", null ],
            [ "direction", "_particle-_scripts.html#ParticleEmitterAttributes_direction", null ],
            [ "direction_position_reference", "_particle-_scripts.html#ParticleEmitterAttributes_direction_position_reference", null ],
            [ "emission_rate", "_particle-_scripts.html#ParticleEmitterAttributes_emission_rate", null ],
            [ "position", "_particle-_scripts.html#ParticleEmitterAttributes_position", null ],
            [ "velocity", "_particle-_scripts.html#ParticleEmitterAttributes_velocity", null ],
            [ "velocity_min & velocity_max", "_particle-_scripts.html#ParticleEmitterAttributes_velocity_min_max", null ],
            [ "time_to_live", "_particle-_scripts.html#ParticleEmitterAttributes_time_to_live", null ],
            [ "time_to_live_min & time_to_live_max", "_particle-_scripts.html#ParticleEmitterAttributes_time_to_live_min_max", null ],
            [ "duration", "_particle-_scripts.html#ParticleEmitterAttributes_duration", null ],
            [ "duration_min & duration_max", "_particle-_scripts.html#ParticleEmitterAttributes_duration_min_max", null ],
            [ "repeat_delay", "_particle-_scripts.html#ParticleEmitterAttributes_repeat_delay", null ],
            [ "repeat_delay_min & repeat_delay_max", "_particle-_scripts.html#ParticleEmitterAttributes_repeat_delay_min_max", null ]
          ] ],
          [ "Standard Particle Emitters", "_particle-_scripts.html#Standard-Particle-Emitters", [
            [ "Point Emitter", "_particle-_scripts.html#Point-Emitter", null ],
            [ "Box Emitter", "_particle-_scripts.html#Box-Emitter", null ],
            [ "Cylinder Emitter", "_particle-_scripts.html#Cylinder-Emitter", null ],
            [ "Ellipsoid Emitter", "_particle-_scripts.html#Ellipsoid-Emitter", null ],
            [ "Hollow Ellipsoid Emitter", "_particle-_scripts.html#Hollow-Ellipsoid-Emitter", null ],
            [ "Ring Emitter", "_particle-_scripts.html#Ring-Emitter", null ]
          ] ],
          [ "Particle Affectors", "_particle-_scripts.html#Particle-Affectors", null ],
          [ "Standard Particle Affectors", "_particle-_scripts.html#Standard-Particle-Affectors", [
            [ "Linear Force Affector", "_particle-_scripts.html#Linear-Force-Affector", null ],
            [ "ColourFader Affector", "_particle-_scripts.html#ColourFader-Affector", null ],
            [ "ColourFader2 Affector", "_particle-_scripts.html#ColourFader2-Affector", null ],
            [ "Scaler Affector", "_particle-_scripts.html#Scaler-Affector", null ],
            [ "Rotator Affector", "_particle-_scripts.html#Rotator-Affector", null ],
            [ "ColourInterpolator Affector", "_particle-_scripts.html#ColourInterpolator-Affector", null ],
            [ "ColourImage Affector", "_particle-_scripts.html#ColourImage-Affector", null ],
            [ "DeflectorPlane Affector", "_particle-_scripts.html#DeflectorPlane-Affector", null ],
            [ "DirectionRandomiser Affector", "_particle-_scripts.html#DirectionRandomiser-Affector", null ]
          ] ]
        ] ],
        [ "Overlays", "v1__overlays.html", null ]
      ] ],
      [ "Performance", "_performance_page.html", [
        [ "Threading", "threading.html", [
          [ "Initializing", "threading.html#ThreadingInitializing", [
            [ "Ideal number of threads", "threading.html#ThreadingInitializingNumberOfThreads", null ]
          ] ],
          [ "What tasks are threaded in Ogre", "threading.html#ThreadingInOgre", null ],
          [ "Using Ogre's threading system for custom tasks", "threading.html#ThreadingCustomTasks", null ],
          [ "Thread safety of SceneNodes", "threading.html#ThreadSafetySceneNodes", null ]
        ] ],
        [ "Performance Hints", "performance.html", null ],
        [ "Tunning memory consumption and resources", "_tuning_memory_resources.html", [
          [ "Grouping textures by type", "_tuning_memory_resources.html#GroupingTexturesByType", null ],
          [ "Dynamic vs Default buffers", "_tuning_memory_resources.html#DynamicVsDefaultBuffers", null ],
          [ "Tweaking default memory consumption by VaoManager", "_tuning_memory_resources.html#TweakingVaoManager", [
            [ "Vulkan and <tt>TEXTURES_OPTIMAL</tt>", "_tuning_memory_resources.html#autotoc_md78", null ]
          ] ]
        ] ]
      ] ],
      [ "Ogre Next Samples (Feature demonstrations)", "_samples.html", [
        [ "Showcase: Forward3D", "_samples.html#forward3d", null ],
        [ "Showcase: HDR", "_samples.html#hdr", null ],
        [ "Showcase: HDR/SMAA", "_samples.html#hdrsmaa", null ],
        [ "Showcase: PBS Materials", "_samples.html#pbsmaterials", null ],
        [ "Showcase: Post Processing", "_samples.html#postprocessing", null ],
        [ "API Usage: Animation tag points", "_samples.html#animationtagpoints", null ],
        [ "API Usage: Area Light Approximation", "_samples.html#arealighapprox", null ],
        [ "API Usage: Custom Renderable", "_samples.html#customrenderable", null ],
        [ "API Usage: Decals", "_samples.html#decals", null ],
        [ "API Usage: Dynamic Geometry", "_samples.html#dynamicgeometry", null ],
        [ "API Usage: IES Photometric Profiles", "_samples.html#iesprofiles", null ],
        [ "API Usage: Shared Skeleton", "_samples.html#sharedskeleton", null ],
        [ "API Usage: Instanced Stereo", "_samples.html#instancedstero", null ],
        [ "API Usage: Instant Radiosity", "_samples.html#instantradiosity", null ],
        [ "API Usage: Local Reflections Using Parallax Corrected Cubemaps", "_samples.html#localcubemaps", null ],
        [ "API Usage: Local Reflections Using Parallax Corrected Cubemaps With Manual Probes", "_samples.html#localcubemapsmp", null ],
        [ "API Usage: Automatic LOD Generation", "_samples.html#autolod", null ],
        [ "API Usage: Morph Animations", "_samples.html#morphanimation", null ],
        [ "API Usage: Automatically Placed Parallax Corrected Cubemap Probes Via PccPerPixelGridPlacement", "_samples.html#autopcc", null ],
        [ "API Usage: Planar Reflections", "_samples.html#planarreflections", null ],
        [ "API Usage: Refractions", "_samples.html#refractions", null ],
        [ "API Usage: SceneFormat Export / Import Sample", "_samples.html#sceneformat", null ],
        [ "API Usage: Screen Space Reflections", "_samples.html#ssreflections", null ],
        [ "API Usage: Shadow Map Debugging", "_samples.html#shadowdebug", null ],
        [ "API Usage: Shadow Map From Code", "_samples.html#shadowfromcode", null ],
        [ "API Usage: Static Shadow Map", "_samples.html#staticshadowmap", null ],
        [ "API Usage: Stencil Test", "_samples.html#stenciltest", null ],
        [ "API Usage: Stereo Rendering", "_samples.html#stereorendering", null ],
        [ "API Usage: Updating Decals And Area Lights' Textures", "_samples.html#updatedecal", null ],
        [ "API Usage: Ogre V1 Interfaces", "_samples.html#v1interface", null ],
        [ "API Usage: Ogre Next Manual Object", "_samples.html#onmanualobject", null ],
        [ "API Usage: Ogre Next V2 Meshes", "_samples.html#v2Mesh", null ],
        [ "Tutorial: Tutorial 00 - Basic", "_samples.html#tutorial00", null ],
        [ "Tutorial: Tutorial 01 - Initialisation", "_samples.html#tutorial01", null ],
        [ "Tutorial: Tutorial 02 - Variable Framerate", "_samples.html#tutorial02", null ],
        [ "Tutorial: Tutorial 03 - Deterministic Loop", "_samples.html#tutorial03", null ],
        [ "Tutorial: Tutorial 04 - Interpolation Loop", "_samples.html#tutorial04", null ],
        [ "Tutorial: Tutorial 05 - Multithreading Basics", "_samples.html#tutorial05", null ],
        [ "Tutorial: Tutorial 06 - Multithreading", "_samples.html#tutorial06", null ],
        [ "Tutorial: Compute 01 - UAV Textures", "_samples.html#compute01", null ],
        [ "Tutorial: Compute 02 - UAV Buffers", "_samples.html#compute02", null ],
        [ "Tutorial: Distortion", "_samples.html#tut_distortion", null ],
        [ "Tutorial: Dynamic Cube Map", "_samples.html#dyncubemap", null ],
        [ "Tutorial: EGL Headless", "_samples.html#eglheadless", null ],
        [ "Tutorial: Open VR", "_samples.html#openVR", null ],
        [ "Tutorial: Reconstructing Position From Depth", "_samples.html#reconpfromd", null ],
        [ "Tutorial: Rendering Sky As A Postprocess With A Single Shader", "_samples.html#skypost", null ],
        [ "Tutorial: SMAA", "_samples.html#SMAA", null ],
        [ "Tutorial: SSAO", "_samples.html#ssao", null ],
        [ "Tutorial: Terra Terrain", "_samples.html#terrain", null ],
        [ "Tutorial: Texture Baking", "_samples.html#texturebaking", null ],
        [ "Tutorial: UAV Setup 1 Example", "_samples.html#uav1", null ],
        [ "Tutorial: UAV Setup 2 Example", "_samples.html#uav2", null ]
      ] ],
      [ "Plugins", "_plugins.html", [
        [ "ParticleFX2 / Particle System 2 Documentation", "_particle_system2.html", [
          [ "Reasons to use ParticleFX2", "_particle_system2.html#ParticleSystem2ReasonsPFX2", null ],
          [ "Reasons not to use ParticleFX2", "_particle_system2.html#ParticleSystem2ReasonsNotPFX2", null ],
          [ "Differences with ParticleFX2", "_particle_system2.html#ParticleSystem2DiffWithPFX2", [
            [ "Distance to Camera", "_particle_system2.html#ParticleSystem2DistToCamera", null ]
          ] ],
          [ "Using OIT (Order Independent Transparency)", "_particle_system2.html#ParticleSystem2Oit", [
            [ "Alpha Hashing: Blue Noise vs White Noise", "_particle_system2.html#AlphaHashingBlueNoiseSetup", null ]
          ] ],
          [ "Thread Safe RandomValueProvider", "_particle_system2.html#autotoc_md79", null ],
          [ "New settings", "_particle_system2.html#autotoc_md80", null ]
        ] ],
        [ "Terra System", "_terra_system.html", [
          [ "Vertex-bufferless rendering", "_terra_system.html#autotoc_md81", null ],
          [ "Vertex Trick in Terra", "_terra_system.html#autotoc_md82", null ],
          [ "Terra cells", "_terra_system.html#autotoc_md83", [
            [ "First layer, the 4x4 block", "_terra_system.html#autotoc_md84", null ],
            [ "Outer layers", "_terra_system.html#autotoc_md85", null ]
          ] ],
          [ "Skirts", "_terra_system.html#autotoc_md86", null ],
          [ "Shadows", "_terra_system.html#autotoc_md87", null ],
          [ "Shading", "_terra_system.html#autotoc_md88", null ],
          [ "Why is it not a component?", "_terra_system.html#autotoc_md89", null ]
        ] ]
      ] ],
      [ "Migrating Series", "_migrating_series.html", [
        [ "Migrating from 2.1 to 2.2", "_migrating_21_to_22.html", [
          [ "What's new in Ogre 2.2", "_ogre22_changes.html", [
            [ "Load Store semantics", "_ogre22_changes.html#Ogre22LoadStoreSemantics", [
              [ "Now that we’ve explained how TBDRs work, we can explain load and store actions", "_ogre22_changes.html#autotoc_md23", null ]
            ] ],
            [ "More control over MSAA", "_ogre22_changes.html#autotoc_md24", null ],
            [ "Porting to Ogre 2.2 from 2.1", "_ogre22_changes.html#autotoc_md25", [
              [ "PixelFormats", "_ogre22_changes.html#autotoc_md26", [
                [ "Common pixel format equivalencies", "_ogre22_changes.html#autotoc_md27", null ]
              ] ],
              [ "Useful code snippets", "_ogre22_changes.html#autotoc_md28", [
                [ "Create a TextureGpu based on a file", "_ogre22_changes.html#autotoc_md29", null ],
                [ "Create a TextureGpu based that you manually fill", "_ogre22_changes.html#autotoc_md30", null ],
                [ "Uploading data to a TextureGpu", "_ogre22_changes.html#autotoc_md31", null ],
                [ "Upload streaming", "_ogre22_changes.html#autotoc_md32", null ],
                [ "Downloading data from TextureGpu into CPU", "_ogre22_changes.html#autotoc_md33", null ],
                [ "Downloading streaming", "_ogre22_changes.html#autotoc_md34", null ]
              ] ]
            ] ],
            [ "Difference between depth, numSlices and depthOrSlices", "_ogre22_changes.html#autotoc_md35", null ],
            [ "Memory layout of textures and images", "_ogre22_changes.html#autotoc_md36", null ],
            [ "Troubleshooting errors", "_ogre22_changes.html#autotoc_md37", null ],
            [ "RenderPassDescriptors", "_ogre22_changes.html#autotoc_md38", null ],
            [ "DescriptorSetTexture & co.", "_ogre22_changes.html#autotoc_md39", null ],
            [ "Does 2.2 interoperate well with the HLMS texture arrays?", "_ogre22_changes.html#autotoc_md40", null ],
            [ "Hlms porting", "_ogre22_changes.html#autotoc_md41", null ],
            [ "Things to watch out when porting", "_ogre22_changes.html#autotoc_md42", null ]
          ] ]
        ] ],
        [ "Migrating from 2.2 to 2.3", "_migrating_22_to_23.html", [
          [ "What's new in Ogre 2.3", "_ogre23_changes.html", [
            [ "Switch importV1 to createByImportingV1", "_ogre23_changes.html#autotoc_md43", null ],
            [ "Shadow's Normal Offset Bias", "_ogre23_changes.html#autotoc_md44", null ],
            [ "Unlit vertex and pixel shaders unified", "_ogre23_changes.html#autotoc_md45", null ],
            [ "Added HlmsMacroblock::mDepthClamp", "_ogre23_changes.html#autotoc_md46", null ],
            [ "Added shadow pancaking", "_ogre23_changes.html#autotoc_md47", null ],
            [ "PluginOptional", "_ogre23_changes.html#autotoc_md48", null ],
            [ "Other relevant information when porting", "_ogre23_changes.html#autotoc_md49", [
              [ "Do not call notifyDataIsReady more than needed", "_ogre23_changes.html#autotoc_md50", null ],
              [ "Global changes for Vulkan compatibility:", "_ogre23_changes.html#autotoc_md51", null ]
            ] ]
          ] ]
        ] ],
        [ "Migrating from 2.3 to 3.0", "_migrating_23_to_30.html", [
          [ "Resolving Merge Conflicts in Ogre-Next 3.0", "_resolving_merge_conflicts30.html", [
            [ "Notes:", "_resolving_merge_conflicts30.html#autotoc_md76", null ],
            [ "Batch Script", "_resolving_merge_conflicts30.html#autotoc_md77", null ]
          ] ],
          [ "What's new in Ogre-Next 3.0", "_ogre30_changes.html", [
            [ "Ogre to OgreNext name migration", "_ogre30_changes.html#autotoc_md52", null ],
            [ "PBS Changes in 3.0", "_ogre30_changes.html#autotoc_md53", null ],
            [ "Hlms Shader piece changes", "_ogre30_changes.html#autotoc_md54", null ],
            [ "AbiCookie", "_ogre30_changes.html#autotoc_md55", null ],
            [ "Move to C++11 and general cleanup", "_ogre30_changes.html#autotoc_md56", null ]
          ] ],
          [ "PBR / PBS Changes in 3.0", "_p_b_s_changes_in30.html", [
            [ "Short version", "_p_b_s_changes_in30.html#autotoc_md63", null ],
            [ "Long version", "_p_b_s_changes_in30.html#autotoc_md64", [
              [ "Direct Lighting", "_p_b_s_changes_in30.html#autotoc_md65", [
                [ "Fresnel Diffuse is no longer considered", "_p_b_s_changes_in30.html#autotoc_md66", [
                  [ "Raffaele's comments:", "_p_b_s_changes_in30.html#autotoc_md67", null ],
                  [ "Default-enable to diffuse fresnel", "_p_b_s_changes_in30.html#autotoc_md68", null ]
                ] ],
                [ "Geometric Term change", "_p_b_s_changes_in30.html#autotoc_md69", null ],
                [ "Metalness change", "_p_b_s_changes_in30.html#autotoc_md70", null ]
              ] ],
              [ "IBL", "_p_b_s_changes_in30.html#autotoc_md71", [
                [ "IBL Diffuse", "_p_b_s_changes_in30.html#autotoc_md72", [
                  [ "Multiplication by PI", "_p_b_s_changes_in30.html#autotoc_md73", null ]
                ] ],
                [ "IBL Specular", "_p_b_s_changes_in30.html#autotoc_md74", null ]
              ] ]
            ] ],
            [ "Hemisphere Ambient Lighting changes", "_p_b_s_changes_in30.html#autotoc_md75", null ]
          ] ]
        ] ],
        [ "Migrating from 3.0 to 4.0", "_migrating_30_to_40.html", [
          [ "What's new in Ogre-Next 4.0", "_ogre40_changes.html", [
            [ "Threaded Hlms", "_ogre40_changes.html#autotoc_md57", null ],
            [ "Hlms implementations and listeners", "_ogre40_changes.html#autotoc_md58", [
              [ "Porting tips (from <= 3.0)", "_ogre40_changes.html#Ogre40Changes_PortingTips", null ]
            ] ],
            [ "Compositor Script changes", "_ogre40_changes.html#autotoc_md59", null ],
            [ "New initialization step", "_ogre40_changes.html#autotoc_md60", null ],
            [ "HlmsUnlit changes", "_ogre40_changes.html#autotoc_md61", null ],
            [ "Header renames", "_ogre40_changes.html#autotoc_md62", null ]
          ] ]
        ] ]
      ] ],
      [ "Deprecation Plan", "_deprecation_plan.html", [
        [ "CMake Option", "_deprecation_plan.html#autotoc_md21", null ],
        [ "Functions marked with <tt>OGRE_DEPRECATED_VER</tt>", "_deprecation_plan.html#autotoc_md22", null ]
      ] ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", "namespacemembers_type" ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", "namespacemembers_eval" ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", "functions_type" ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", "functions_eval" ],
        [ "Properties", "functions_prop.html", null ],
        [ "Related Symbols", "functions_rela.html", "functions_rela" ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", "globals_defs" ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_android_2_ogre_config_dialog_imp_8h.html",
"_ogre_billboard_chain_8h.html",
"_ogre_d3_d11_legacy_s_d_k_emulation_8h.html#a38aca83993b2756cc7a2f374905ab196",
"_ogre_g_l_e_s2_prerequisites_8h.html#a45bbdef2e602a1d12b999a837df8fb89",
"_ogre_id_string_8h.html#a65d81f8d195ea976dbed9daee4cc31f0",
"_ogre_overlay_prerequisites_8h.html",
"_ogre_render_queue_8h.html",
"_ogre_texture_gpu_manager_8h.html#a3a978c834ce00930123468c9a64047b4a46cdc74c21d464c0330b5bcb57b22462",
"_ogre_vertex_elements_8h.html#a0a62b3f2ede8992ff365bb013a8bc00da6f167783a26ed5d1c146fbeae995af0b",
"_ogre_x11_e_g_l_support_8h.html#ae55322407855ab36edf909cb81daf20d",
"azdo.html#V2AndV1Objects",
"class_ogre_1_1_archive_manager.html#a2774a9f593600e518b1b5252d5186c61",
"class_ogre_1_1_array_memory_manager.html#a443f00a249cbbc4b6edf56632c5e2756",
"class_ogre_1_1_array_vector2.html#a761c437e17ca38570b7f850652b53cae",
"class_ogre_1_1_array_vector3.html#aa1da9a8f7332070b10426a64bc80f36d",
"class_ogre_1_1_async_texture_ticket.html#a742f8f485fa6e28247881ac5637d4a10",
"class_ogre_1_1_axis_aligned_box.html#a569d00c84979da765abfc66a64b25b68",
"class_ogre_1_1_box_emitter_factory.html#ae3aa33b7a73f6a78c241db3c175c8178",
"class_ogre_1_1_camera.html#a6b0e4f7b871eca1c11097e50780cf6be",
"class_ogre_1_1_colour_fader_affector2.html#a2d6b1d11318b3dd2bf45ea2c2f6ae598",
"class_ogre_1_1_colour_image_affector2.html#a05abf40cb721d114e7decb01eafcbd6c",
"class_ogre_1_1_command_buffer.html#afe98b4d1feeb505dbde58b32cbff8b80",
"class_ogre_1_1_compositor_pass_depth_copy_def.html#af8dce88fce2b1540d453ff2fa212163e",
"class_ogre_1_1_compositor_target_def.html#a8633b1cdc6a1e9a0d3bd31e083ac20c5",
"class_ogre_1_1_controller_manager.html#a7dc1048c68490f5f907d790d74f51f11",
"class_ogre_1_1_d3_d11_buffer_interface.html#ae50b6620b70f2c29ed6241216473d513",
"class_ogre_1_1_d3_d11_h_l_s_l_program.html#ac2413285a4ce23815075691179c17740",
"class_ogre_1_1_d3_d11_render_system.html#a429bda5fbb42b3426e8ec9ac5f08835b",
"class_ogre_1_1_d3_d11_stereo_driver_impl.html#a6c1cbe69a997fe98a0a935b1c330a2de",
"class_ogre_1_1_d3_d11_window.html",
"class_ogre_1_1_default_work_queue.html#ad78e6bebd4280b2bd8c430291925d381",
"class_ogre_1_1_distance_lod_strategy_base.html#aeb9db4da4774ed395054a0ce5c87fce1",
"class_ogre_1_1_egl_p_buffer_support.html#a1d8351d5b2f05fd0608d9850d38d5787",
"class_ogre_1_1_emitter_def_data.html#a948511990daa032a357e873921fa2683",
"class_ogre_1_1_file_system_archive.html#aa55394ed0fa2e4df48363e038344d8c9",
"class_ogre_1_1_frustum.html#ae8a1b90cb3f81ef66987032777681629",
"class_ogre_1_1_g_l3_plus_render_system.html#a03d3c66268325c8a3a5fbab233bfe537",
"class_ogre_1_1_g_l3_plus_render_system.html#af877d991762cdda3025d69dc0cc3bf50",
"class_ogre_1_1_g_l3_plus_vao_manager.html#a79c5a9e410625780fcc610f3243be960",
"class_ogre_1_1_g_l_e_s2_gpu_program_manager.html#ab114cbd4a6240acb7093b2a795f05af5",
"class_ogre_1_1_g_l_e_s2_render_system.html#a9f6db584e4cc57ee997294d14ef3a69f",
"class_ogre_1_1_g_l_e_s2_vao_manager.html#af6396319a56f1705c5843971b5d38af9",
"class_ogre_1_1_g_l_s_l_program.html#a7033f73d15946c90a612690beb5c8a2f",
"class_ogre_1_1_g_l_x_utils.html#a4c9525dd103e3b3facd31b9fca10970d",
"class_ogre_1_1_gpu_program_parameters.html#a155c886f15e0c10d2c33c224f0d43ce3a7252f693f607f47c7b68cf6a4a4f16cb",
"class_ogre_1_1_gpu_resource.html#aa747c8f9b2503e09c167d885e9dcbdee",
"class_ogre_1_1_hlms.html#a7a0ec84d5e6078c5627f40e95d8d8ca3",
"class_ogre_1_1_hlms_json_unlit.html#aff82115381fe9ddc34ebf8bb9fb3840f",
"class_ogre_1_1_hlms_unlit.html#a7cbae042f1c24d201432bf8ec1c33ff9",
"class_ogre_1_1_indirect_buffer_packed.html",
"class_ogre_1_1_item_identity_exception.html#ae98c4a38b709e81ef21722be30db5060",
"class_ogre_1_1_lightweight_mutex.html#a1876f974fd17bb2885d73747e1e7364c",
"class_ogre_1_1_lod_outside_marker.html#a29dd4b0f1d314671a8eb66a6ae260f93",
"class_ogre_1_1_manual_object.html#a2172149d38b421e863db25a2f4fb7617",
"class_ogre_1_1_math.html#a12f1cd41b467cd21dfa192025553ed46",
"class_ogre_1_1_matrix3.html#a9c4cbf94ffb5400519b60dd4620b595a",
"class_ogre_1_1_metal_buffer_interface.html#a74ff031ab12ba2c8403088160ab14c9d",
"class_ogre_1_1_metal_program_factory.html#ac846a84ce5b42f445baa099e3e99e0f3",
"class_ogre_1_1_metal_staging_buffer.html#a2a5852c0c0255267898f48ace9c8b124",
"class_ogre_1_1_movable_object.html#a158e972be87282325850f001cee635a8",
"class_ogre_1_1_n_u_l_l_const_buffer_packed.html#a52a6f2d065480ac4eeb292907052c3f8",
"class_ogre_1_1_n_u_l_l_texture_gpu_manager.html#a3b2eb464f34ca01c6e0f60b3a7e837cf",
"class_ogre_1_1_node.html#ae98ec089fda531aa72b7200d4bc98f03",
"class_ogre_1_1_p_s_s_m_shadow_camera_setup.html#a9375773578b060cca8cb221cfffc387f",
"class_ogre_1_1_paged_world_section.html#acd4ecd952e295a94f5649a6064c8531a",
"class_ogre_1_1_particle.html#a6a2bc4941c3c346f6d9936e447dde3c9",
"class_ogre_1_1_particle_system2.html#a60b19fea59f49de1008191a04753c815",
"class_ogre_1_1_particle_system_renderer.html#a2e0b651fd9d845b9bc70e9aaf64e5915",
"class_ogre_1_1_pixel_format_gpu_utils.html#a19c5422c171a259ffdc5967d1aead7c0a8bc36763e7a6de3687a555662ae1bcfc",
"class_ogre_1_1_pool.html#af399adb462aa706c25e1e961cdfb5a1e",
"class_ogre_1_1_radian.html#a6b34aca5c2b5cbc483dde8f22d3de0a1",
"class_ogre_1_1_render_queue_listener.html#a338dd1abf82317692ce7504978c01a5c",
"class_ogre_1_1_render_system_capabilities.html#a73c185f1029507fec309eb0b781a621e",
"class_ogre_1_1_resource.html#aa5d6f3f7e73b77b233fc62b11f270e4d",
"class_ogre_1_1_root.html#a06b786d8e0847dfe9cc15e1dea308fcd",
"class_ogre_1_1_s_t_b_i_image_codec.html#a27a2d8beba5370e1b0efd9cc56855c0e",
"class_ogre_1_1_scene_manager.html#a17f57d1485b6f28bed9a7ead8d59b8e1",
"class_ogre_1_1_scene_manager.html#a7eb12e77377019771c3b8faa001c2062",
"class_ogre_1_1_scene_manager.html#af2d4879678c141d5acd892a4188aa221",
"class_ogre_1_1_screen_ratio_pixel_count_lod_strategy.html#acce5c9f640628e3eb05cca6fb0333b06",
"class_ogre_1_1_skeleton_animation_def.html#a5bb015b43352f666bcd35cf45e88001a",
"class_ogre_1_1_small_vector_template_common.html#a6f3019eb35333a8ca4e10f14923298d4",
"class_ogre_1_1_stream_serialiser.html#a2aae82219abe1e37ea5f1c1adf8bdd1c",
"class_ogre_1_1_technique.html#a63da4eb10f7272486a9806374b09bab6",
"class_ogre_1_1_texture_gpu.html#a5e9b67097af3ad1a470065fc9dce032d",
"class_ogre_1_1_texture_unit_state.html#a942a1ce66d0909d29ed09c98e9e6d82da6e85f00afb893c2bace9a394e5ac9f95",
"class_ogre_1_1_vao_manager.html#a0aa4278b6061752264f2887aa3dff7ed",
"class_ogre_1_1_vct_material.html#a5d673fc0669ae3c41af972725f5411ea",
"class_ogre_1_1_vector4.html#aa175d851733658eecdb7e70f678df8ff",
"class_ogre_1_1_volume_1_1_chunk.html#a6b7ccb2109873a50c86ee4fcc51cce46",
"class_ogre_1_1_volume_1_1_source.html#a348e2692fb3b2b65223b1331a42e91dd",
"class_ogre_1_1_vulkan_delayed__vk_destroy_sampler.html#a3a7086e671c29fef3ac3f0413bd8ba3d",
"class_ogre_1_1_vulkan_program.html#a00839f5e6dec10ac32f9544b114db2f9",
"class_ogre_1_1_vulkan_render_system.html#a35f73464824f50dd3f0927cced397e99",
"class_ogre_1_1_vulkan_root_layout.html#aade6b1af85f8aef53ff225c7d8034fe8",
"class_ogre_1_1_vulkan_vao_manager.html#a96bb866e33ee675aa532142c05b7c864",
"class_ogre_1_1_waitable_event.html#a1a0392b8ecd2b3cecb4b70f0065993bd",
"class_ogre_1_1_work_queue.html#afe710c55a31c661f738abb2cc5c7303e",
"class_ogre_1_1v1_1_1_animation_state.html#ab7148c4b64b729a17fc68e62986d0330",
"class_ogre_1_1v1_1_1_billboard_set.html#ac4c2607fc42750775e4b29aa03dffe05",
"class_ogre_1_1v1_1_1_entity.html#a1f79f6686a6939329193619e6bd8e3b0",
"class_ogre_1_1v1_1_1_g_l3_plus_hardware_vertex_buffer.html#a0acfcd37484c21c7674fd5da4b565132",
"class_ogre_1_1v1_1_1_hardware_buffer.html#ac0c56e93ce6f29bf39fb48c3d5dc64cfa688a3812b47db689eebdbfefca66df87",
"class_ogre_1_1v1_1_1_mesh.html#a20a94eb3120dbc1a14fb3fc3f5a29eb1",
"class_ogre_1_1v1_1_1_metal_hardware_vertex_buffer.html#a467071b2d6acfe339374bcb8c4064280",
"class_ogre_1_1v1_1_1_old_node_animation_track.html#a93f0c61431c33b4391c28c7513c561d8",
"class_ogre_1_1v1_1_1_overlay_manager.html#a0ab7251905e3a85159f6b8217a39dc5a",
"class_ogre_1_1v1_1_1_skeleton.html#a86961f2e5bfb6b2403d58dc05fef0f62",
"class_ogre_1_1v1_1_1_time_index.html#a673ac8cf9bced53b97cc5377e494f2f6",
"class_ogre_1_1v1_1_1_vulkan_hardware_vertex_buffer.html#a41f5dded3179c3bb078756ca4bdf1ca5",
"dir_9243a22ceb45bc468b0483f197836e23.html",
"group___animation.html#ga4393e2afffe021cb323b896779de3d17",
"group___general.html#gga0ef99399e9e670e7bb69dc373968a666ace4d60a17763d9ba4121e716cdae4c63",
"group___general.html#gga30d5439896c2a2362024ec689b1e181ca9f7aa52dae209b16e10b405138fdf51b",
"group___general.html#ggae8bb1cb809660de802d1d592ba4adb61a7faeb919637b85bed8650d8f1baf2ada",
"group___image.html#gga71f09fe41a1db41186262f1aa5814a18ac3621ecf1c858eb4cc140b47543ed41e",
"group___optional.html#ga2762e5a38c2a8f887236920efb6dc840",
"group___render_system.html#ggac4c251bcc05376f701348bfe0f4a53b4a5753a78e9d99c0d1c878d1c6adc5a83b",
"hlmsmacroblockref.html",
"namespace_ogre.html#a725a35f2df65645c39ef694fe468091d",
"namespace_ogre_1_1_lamp_cone_type.html",
"namespace_ogre_1_1v1.html#ada7f44b0e95a262656a6b5b715c05377a8270af4f4f39b583d53d34dd510055ba",
"struct_ogre_1_1_bone_transform.html#a1ba3bd60d7b62b8bf22106fff26b302a",
"struct_ogre_1_1_compositor_pass_uav_def_1_1_texture_source.html#a07c2879ff66406db5824d0a5915da359",
"struct_ogre_1_1_descriptor_set_texture2_1_1_texture_slot.html#a5e4ca5e4934489665c75aff247d1b949",
"struct_ogre_1_1_g_l_e_s2_vao_manager_1_1_block.html",
"struct_ogre_1_1_hlms_base_prop.html#aff59c800dc28ed12f86cf40cfa576ad1",
"struct_ogre_1_1_irradiance_field_settings.html#a664c3c0430ef4fbe2a877b98bd97fe80",
"struct_ogre_1_1_lod_input_buffer.html#a8804206bc5ce9cd384d9d3e358fe1bbf",
"struct_ogre_1_1_object_data.html#a3c419f2e030fb2cd289a77ef773c4fd0",
"struct_ogre_1_1_ray_scene_query_result_entry.html",
"struct_ogre_1_1_skeleton_def_1_1_bone_data.html",
"struct_ogre_1_1_transform.html#ab130604cfbbebb88805dd147aa7a3e11",
"struct_ogre_1_1_vct_material_1_1_datablock_conversion_result.html#aa1a11d3db79c954048038ff273576652",
"struct_ogre_1_1_volume_1_1_dual_cell.html#a0ad4661acc2db15251656a1fdf8e8bc1",
"struct_ogre_1_1_vulkan_frame_buffer_desc_value.html",
"struct_ogre_1_1is_pod_like_3_01unsigned_01short_01_4.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';