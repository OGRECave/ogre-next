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
        [ "Advanced Users", "_rendering.html#autotoc_md106", null ],
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
            [ "Customizing an existing implementation", "hlms.html#HlmsCreationOfShadersCustomizing", [
              [ "Examples:", "hlms.html#autotoc_md110", null ]
            ] ]
          ] ],
          [ "Run-time rendering", "hlms.html#HlmsRuntimeRendering", [
            [ "preparePassHash", "hlms.html#HlmsRuntimeRenderingPreparePassHash", null ],
            [ "fillBuffersFor", "hlms.html#HlmsRuntimeRenderingFillBuffersFor", null ],
            [ "Multithreaded Shader Compilation", "hlms.html#autotoc_md111", null ]
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
          [ "Precision / Quality", "hlms.html#autotoc_md112", null ],
          [ "Multithreaded Shader Compilation", "_hlms_threading.html", [
            [ "CMake Options", "_hlms_threading.html#HlmsThreading_CMakeOptions", null ],
            [ "The tid (Thread ID) argument", "_hlms_threading.html#HlmsThreading_tidArgument", [
              [ "API when OGRE_SHADER_COMPILATION_THREADING_MODE = 1", "_hlms_threading.html#autotoc_md99", null ]
            ] ],
            [ "How does threaded Hlms work?", "_hlms_threading.html#autotoc_md100", [
              [ "What is the range of tid argument?", "_hlms_threading.html#autotoc_md101", null ]
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
                [ "Non-researched solutions:", "_image_voxel_cone_tracing.html#autotoc_md102", null ]
              ] ],
              [ "Trivia", "_image_voxel_cone_tracing.html#autotoc_md103", null ]
            ] ],
            [ "Step 2: Row Translation", "_image_voxel_cone_tracing.html#IVCT_Step2", null ],
            [ "Step 3: Cascades", "_image_voxel_cone_tracing.html#IVCT_Step3", null ],
            [ "Wait isn't this what UE5's Lumen does?", "_image_voxel_cone_tracing.html#autotoc_md104", null ],
            [ "Wait isn't this what Godot does?", "_image_voxel_cone_tracing.html#autotoc_md105", null ]
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
            [ "Could you have used e.g. \"const_buffers\" : [0,7] instead of [4,7]?", "_root_layouts.html#autotoc_md107", null ]
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
                [ "OpenGL", "_root_layouts.html#autotoc_md108", null ],
                [ "Vulkan", "_root_layouts.html#autotoc_md109", null ]
              ] ]
            ] ]
          ] ]
        ] ]
      ] ],
      [ "Scripts", "_scripts.html", [
        [ "Loading scripts", "_scripts.html#autotoc_md122", null ],
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
              [ "Parameter: refraction_strength", "hlmspbsdatablockref.html#autotoc_md113", null ],
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
              [ "compositor_node parameters", "compositor.html#autotoc_md115", [
                [ "in", "compositor.html#CompositorNode_in", null ],
                [ "out", "compositor.html#CompositorNode_out", null ],
                [ "in_buffer", "compositor.html#CompositorNode_in_buffer", null ],
                [ "out_buffer", "compositor.html#CompositorNode_out_buffer", null ],
                [ "custom_id", "compositor.html#CompositorNode_custom_id", null ]
              ] ],
              [ "Main RenderTarget", "compositor.html#CompositorNodesChannelsAndRTTsMainRenderTarget", null ]
            ] ],
            [ "Target", "compositor.html#CompositorNodesTarget", [
              [ "target parameters", "compositor.html#autotoc_md116", [
                [ "target_level_barrier", "compositor.html#CompositorTarget_target_level_barrier", null ]
              ] ]
            ] ],
            [ "Passes", "compositor.html#CompositorNodesPasses", [
              [ "pass parameters", "compositor.html#autotoc_md117", [
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
            [ "What is MSAA?", "compositor.html#autotoc_md118", [
              [ "Supersampling Antialiasing (SSAA) vs MSAA", "compositor.html#autotoc_md119", null ],
              [ "MSAA approach to the problem", "compositor.html#autotoc_md120", [
                [ "Resources", "compositor.html#CompositorNodesTexturesMsaaResources", null ]
              ] ]
            ] ],
            [ "Ogre + MSAA with Implicit Resolves", "compositor.html#autotoc_md121", null ],
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
            [ "Common Emitter Attributes", "_particle-_scripts.html#autotoc_md114", null ],
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
            [ "Vulkan and <tt>TEXTURES_OPTIMAL</tt>", "_tuning_memory_resources.html#autotoc_md79", null ]
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
          [ "Thread Safe RandomValueProvider", "_particle_system2.html#autotoc_md80", null ],
          [ "New settings", "_particle_system2.html#autotoc_md81", null ]
        ] ],
        [ "Terra System", "_terra_system.html", [
          [ "Vertex-bufferless rendering", "_terra_system.html#autotoc_md82", null ],
          [ "Vertex Trick in Terra", "_terra_system.html#autotoc_md83", null ],
          [ "Terra cells", "_terra_system.html#autotoc_md84", [
            [ "First layer, the 4x4 block", "_terra_system.html#autotoc_md85", null ],
            [ "Outer layers", "_terra_system.html#autotoc_md86", null ]
          ] ],
          [ "Skirts", "_terra_system.html#autotoc_md87", null ],
          [ "Shadows", "_terra_system.html#autotoc_md88", null ],
          [ "Shading", "_terra_system.html#autotoc_md89", null ],
          [ "Why is it not a component?", "_terra_system.html#autotoc_md90", null ]
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
            [ "Notes:", "_resolving_merge_conflicts30.html#autotoc_md77", null ],
            [ "Batch Script", "_resolving_merge_conflicts30.html#autotoc_md78", null ]
          ] ],
          [ "What's new in Ogre-Next 3.0", "_ogre30_changes.html", [
            [ "Ogre to OgreNext name migration", "_ogre30_changes.html#autotoc_md52", null ],
            [ "PBS Changes in 3.0", "_ogre30_changes.html#autotoc_md53", null ],
            [ "Hlms Shader piece changes", "_ogre30_changes.html#autotoc_md54", null ],
            [ "AbiCookie", "_ogre30_changes.html#autotoc_md55", null ],
            [ "Move to C++11 and general cleanup", "_ogre30_changes.html#autotoc_md56", null ]
          ] ],
          [ "PBR / PBS Changes in 3.0", "_p_b_s_changes_in30.html", [
            [ "Short version", "_p_b_s_changes_in30.html#autotoc_md64", null ],
            [ "Long version", "_p_b_s_changes_in30.html#autotoc_md65", [
              [ "Direct Lighting", "_p_b_s_changes_in30.html#autotoc_md66", [
                [ "Fresnel Diffuse is no longer considered", "_p_b_s_changes_in30.html#autotoc_md67", [
                  [ "Raffaele's comments:", "_p_b_s_changes_in30.html#autotoc_md68", null ],
                  [ "Default-enable to diffuse fresnel", "_p_b_s_changes_in30.html#autotoc_md69", null ]
                ] ],
                [ "Geometric Term change", "_p_b_s_changes_in30.html#autotoc_md70", null ],
                [ "Metalness change", "_p_b_s_changes_in30.html#autotoc_md71", null ]
              ] ],
              [ "IBL", "_p_b_s_changes_in30.html#autotoc_md72", [
                [ "IBL Diffuse", "_p_b_s_changes_in30.html#autotoc_md73", [
                  [ "Multiplication by PI", "_p_b_s_changes_in30.html#autotoc_md74", null ]
                ] ],
                [ "IBL Specular", "_p_b_s_changes_in30.html#autotoc_md75", null ]
              ] ]
            ] ],
            [ "Hemisphere Ambient Lighting changes", "_p_b_s_changes_in30.html#autotoc_md76", null ]
          ] ]
        ] ],
        [ "Migrating from 3.0 to 4.0", "_migrating_30_to_40.html", [
          [ "What's new in Ogre-Next 4.0", "_ogre40_changes.html", [
            [ "Threaded Hlms", "_ogre40_changes.html#autotoc_md57", null ],
            [ "Hlms implementations and listeners", "_ogre40_changes.html#autotoc_md58", [
              [ "Porting tips (from <= 3.0)", "_ogre40_changes.html#Ogre40Changes_PortingTips", null ]
            ] ],
            [ "Trivial Hlms changes", "_ogre40_changes.html#autotoc_md59", null ],
            [ "Compositor Script changes", "_ogre40_changes.html#autotoc_md60", null ],
            [ "New initialization step", "_ogre40_changes.html#autotoc_md61", null ],
            [ "HlmsUnlit changes", "_ogre40_changes.html#autotoc_md62", null ],
            [ "Header renames", "_ogre40_changes.html#autotoc_md63", null ]
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
"_ogre_billboard_8h.html",
"_ogre_d3_d11_hlms_pso_8h.html",
"_ogre_g_l_e_s2_prerequisites_8h.html#a3bc2e39f623dcbb488c3d305a448afef",
"_ogre_hlms_unlit_prerequisites_8h.html#a90397bac6ee36d3380f98a549eada0daa791f3a122a8095938ff14d703b660eab",
"_ogre_o_s_version_helpers_8h.html#ad8a247a955766c3f4905be6c3021f93c",
"_ogre_quaternion_8h.html",
"_ogre_texture_gpu_8h.html#a88628e41c3e6b360f8b1653a1fc6697aafa04688137f25d1b66e3e5c9b6429172",
"_ogre_vertex_elements_8h.html#a027109503a988ba85f4c63b55082907fa9875117bd933d3d7b009c52c003ca8f1",
"_ogre_x11_e_g_l_support_8h.html#a7494bfafd106d56df9ed438c0a03d9ad",
"_tuning_memory_resources.html#autotoc_md79",
"class_ogre_1_1_archive.html#a331471a22b46882d08cd1db7bfc28611",
"class_ogre_1_1_array_matrix_af4x3.html#ab2a741f543ffa1cc9a8ad78960d36541",
"class_ogre_1_1_array_vector2.html#a529309849a1e8365f7c5c7427fbd08df",
"class_ogre_1_1_array_vector3.html#a805a9b00aaa5a7814c5223c889ec5444",
"class_ogre_1_1_async_texture_ticket.html#a11ff659e0f1e167bc53c2cdf754700ab",
"class_ogre_1_1_auto_param_data_source.html#afce9502bdbbcf8f135cf0d90d1b35bb6",
"class_ogre_1_1_boolean_mask4.html#ac672bed1a841eeae9b85cc529dfceaa0",
"class_ogre_1_1_camera.html#a54542bfe56c8a09949d35d6c75a5045c",
"class_ogre_1_1_colour_fader_affector.html#ac2a68a0de0951830620221bf56084ea1",
"class_ogre_1_1_colour_fader_affector_f_x2.html#ae097a0d5d95caf42dac6bd565e6edac6",
"class_ogre_1_1_command_buffer.html#ad0a7964504a07565079ff6fb17609ebb",
"class_ogre_1_1_compositor_pass_def.html#a8c3285eeb1ee8a3538f327ff8d556c53",
"class_ogre_1_1_compositor_shadow_node.html#a227313a84e19bafb0d69f287273a9562",
"class_ogre_1_1_const_buffer_packed.html#aa4ebc58e115b22366cd335aae75ac32b",
"class_ogre_1_1_cylinder_emitter2.html",
"class_ogre_1_1_d3_d11_dynamic_buffer.html#ad41be9f885e62b4f1c92b487b9d216bb",
"class_ogre_1_1_d3_d11_render_system.html#a01b0ebb4b78cc24bc526b268d049a5ee",
"class_ogre_1_1_d3_d11_staging_buffer.html",
"class_ogre_1_1_d3_d11_vao_manager.html#ab0e89c58d15112edd5ef5a01a7c5587a",
"class_ogre_1_1_default_scene_format_listener.html#a776b2b37b7c71d9c145205a88f8828d4",
"class_ogre_1_1_direction_randomiser_affector.html#a87d352225a07b2f0557454a6681320b0",
"class_ogre_1_1_e_g_l_window.html#a6a2b76539652e6c2be9b402ba7f6fbf3",
"class_ogre_1_1_emitter_commands_1_1_cmd_min_duration.html#a94e03ce87b7140556eb36f8fab080148",
"class_ogre_1_1_fast_array.html#aa0d93fe6f29272129139bcf8f304fcff",
"class_ogre_1_1_free_image_codec2.html#a6787ab70d39111d63489f18a838a6144",
"class_ogre_1_1_g_l3_plus_read_only_buffer_emulated_packed.html#a9808af38fb1cc2a907965eb8ef588b78",
"class_ogre_1_1_g_l3_plus_render_system.html#ac0be0700a31d7da27d45efb1162535b7",
"class_ogre_1_1_g_l3_plus_texture_gpu_render_target.html#a0b727294bf20b09915c0881245b969a6",
"class_ogre_1_1_g_l_e_s2_f_b_o_multi_render_target.html#a18b12f41d6a01de56be76a28378e40d7",
"class_ogre_1_1_g_l_e_s2_render_system.html#a423431e7f9058deffc6bba9b7673f50b",
"class_ogre_1_1_g_l_e_s2_tex_buffer_emulated_packed.html#a5c7bc40c22709f92dddd626f62465487",
"class_ogre_1_1_g_l_s_l_e_s_shader_1_1_cmd_preprocessor_defines.html",
"class_ogre_1_1_g_l_x_context.html#ae0ea65602fcbf7008e8be6d204110eac",
"class_ogre_1_1_gpu_program_parameters.html#a123cf81ee91123f9b978de694e9e3e3b",
"class_ogre_1_1_gpu_program_parameters.html#ade8223cfd36836dde883ee33329415cc",
"class_ogre_1_1_high_level_gpu_program_1_1_cmd_use_hlms_parser.html#a01d21d968931dc0678a6e178c04348eb",
"class_ogre_1_1_hlms_datablock.html#af1ac88fb51276fb7145e0b9d7ce77acd",
"class_ogre_1_1_hlms_pbs_datablock.html#a4c5eb8d3bea9b3b472c6f9b3c26e4499",
"class_ogre_1_1_image2.html#a2b8dc803cebdeef4ae91231aebf483a9",
"class_ogre_1_1_irradiance_field.html#af6451e6b39a8750f281b8be035e3a5f8",
"class_ogre_1_1_light.html#a8b09bac9505986355e0e18ca31d67c6d",
"class_ogre_1_1_lod_config_serializer.html#a90888570c5ea11c98e7ea8cfdab52fae",
"class_ogre_1_1_lw_const_string.html#ab4c78ea798071a91f6719c4de82fcb05",
"class_ogre_1_1_material.html#a2c26e56f120a9de63a5a85329ce95844",
"class_ogre_1_1_mathlib_c.html#a5d2b8b11bd795ba12d10eddd5d1263d7",
"class_ogre_1_1_mesh.html#ac6bbae69849cb43ae40afd8eb94767d8",
"class_ogre_1_1_metal_mappings.html#a925f2c069505cfe57d2695efb3376333",
"class_ogre_1_1_metal_render_system.html#a93322773ea466cc280564450686dec18",
"class_ogre_1_1_metal_uav_buffer_packed.html#aac4131429575ce425eb70924871ba351",
"class_ogre_1_1_movable_object.html#ae9081d3b9fe88c2fddb5a44bc325ceff",
"class_ogre_1_1_n_u_l_l_render_system.html#a7e9e9de4c6de5690fa3157ec681bc6f4",
"class_ogre_1_1_node.html#a60c305942b550d110b85c5da64a0a5f6",
"class_ogre_1_1_obj_cmd_buffer_1_1_upload_from_staging_tex.html",
"class_ogre_1_1_page_manager.html#a7b7093e25ba68dcc52ef3e7981dff7ba",
"class_ogre_1_1_parallax_corrected_cubemap_base.html#a039657d1c26eee4e9a7c412d5be12ed9",
"class_ogre_1_1_particle_f_x2_plugin.html",
"class_ogre_1_1_particle_system_factory.html#a1f54d5597df615dc84661ad6a77878c4",
"class_ogre_1_1_pass.html#ab5f997abc5c89099c537d4e7c6a59e84",
"class_ogre_1_1_plane_bounded_volume.html#ac92a3d627da4cadbc9ebe7331b368e6e",
"class_ogre_1_1_property_set.html",
"class_ogre_1_1_rectangle2_d.html#aed2d4167bff4f2238fde042eb24a0fae",
"class_ogre_1_1_render_system.html#aa9a9c316aaa35ecdd939550b56500279",
"class_ogre_1_1_renderable.html#a77230eea8136b7c296cc0a5db1dba9ee",
"class_ogre_1_1_resource_manager.html#a2883e5999ded4031bd421fc5d045aaf7",
"class_ogre_1_1_rotation_affector.html#a8211b7cdae1a7db079ebc9b6a123e045",
"class_ogre_1_1_scale_interpolator_affector_factory.html",
"class_ogre_1_1_scene_manager.html#a5380b5752fbdc38c0476d715f3e963d7",
"class_ogre_1_1_scene_manager.html#ab73819442f7f3540f52431064e6b1a97",
"class_ogre_1_1_scene_node.html#a3c9bd3fb58e49cac76060914e4242423",
"class_ogre_1_1_shadow_texture_definition.html#a9415b07e2e36a433b61767818fa214d8",
"class_ogre_1_1_small_vector.html#a1ddbda6070d0d513d43de087c90e76d3",
"class_ogre_1_1_stack_vector.html#ad7b30816b65462b8d691c914b8251104",
"class_ogre_1_1_sub_item.html#a45267a8b4917f3bbe76a2dc14f7122be",
"class_ogre_1_1_texture_definition_base.html#ad7f9edb910ffb0e955310b4faf5fc400",
"class_ogre_1_1_texture_gpu_manager.html#abf7a7a36c00587fb8298f4100606c627",
"class_ogre_1_1_timer.html#ae9d0299693765d37b2f0b75c013755d2",
"class_ogre_1_1_vct_cascaded_voxelizer.html#a6b8d462f8ae1d8378b883868926ac76a",
"class_ogre_1_1_vector2.html#aed2661adf58895a08a1b38054d8d5743",
"class_ogre_1_1_volume_1_1_c_s_g_cube_source.html#afd9f39e86eb922cad95ee3e49718b5fe",
"class_ogre_1_1_volume_1_1_mesh_builder_callback.html#a27c03be369113b0379191143c852d766",
"class_ogre_1_1_vulkan_async_ticket.html#a6e3a0f87816be62282520cd82a55649f",
"class_ogre_1_1_vulkan_gpu_program_manager.html#a2bd0bbd53b5ba09b11a9217e129ec0d7",
"class_ogre_1_1_vulkan_queue.html#a8ab7d4343e87b2eae286dd7a95e63b4a",
"class_ogre_1_1_vulkan_render_system.html#aa7810530cfd3855165e14d7a032b5308",
"class_ogre_1_1_vulkan_texture_gpu.html#af9632a253899ed155a62dbcc14a7f551",
"class_ogre_1_1_vulkan_window_swap_chain_based.html#a09ff80100ca52995038ce8b404330c24",
"class_ogre_1_1_win32_window.html#af21d51eaae34cbe6b114338ef81f00a8",
"class_ogre_1_1cbitset_n.html#a00c867d4c0978ef88d1e789438b86051",
"class_ogre_1_1v1_1_1_billboard_chain_1_1_element.html#a5cbc242e4abc62b3e73b0117d7b311a0",
"class_ogre_1_1v1_1_1_d3_d11_hardware_buffer.html#a91ed4f52178d37386137d1bd86d3aaeaa59f2e0cce9c5347cd7159c918e606bb1",
"class_ogre_1_1v1_1_1_entity.html#aede39750aa3a679a245b9deb5fcbd45c",
"class_ogre_1_1v1_1_1_g_l_e_s2_hardware_index_buffer.html",
"class_ogre_1_1v1_1_1_manual_object.html#a3a0931d4fb9b2dfb348257b40d15110e",
"class_ogre_1_1v1_1_1_mesh_manager.html#a6fa9b72294531a03a8cb91d4975cab7f",
"class_ogre_1_1v1_1_1_old_node.html#a70c8f84893f2752c5e7b5f7852004e5b",
"class_ogre_1_1v1_1_1_overlay_element.html#a19a3d3d9af49a2b353cbc819ac728623",
"class_ogre_1_1v1_1_1_rectangle2_d.html",
"class_ogre_1_1v1_1_1_sub_mesh.html#a48305c54a01e0c7a3376ea0a077c467f",
"class_ogre_1_1v1_1_1_vertex_element.html#a08b51f30c760159fca490c4b753c9317",
"compositor.html#CompositorPass_identifier",
"functions_func_u.html",
"group___general.html#ga39063dbfddacad4998c6ea0ab722cd11",
"group___general.html#gga30d5439896c2a2362024ec689b1e181ca4313cab3aaae35e47d234a863ebf01ba",
"group___general.html#gga30d5439896c2a2362024ec689b1e181cae6abdb32967bca8e3f54a31b85c7e07b",
"group___image.html#gaea67ccdf688eff24dd1792c185a6f227",
"group___materials.html#gga9c5b2950be06ff56a6ee0bace240d447a4ed9a26ac9822e4f3c0e623b1a64c63b",
"group___property.html#gga0883a5af9197151407965df0bacc4f3aabb2c30d9d8af27d83d4aafed018aa84d",
"group___resources.html#gga7b904fc5463a8ef1e61f6de39b603fc4ad2d5c64333e75efb676c25cf7413fbe7",
"namespace_ogre.html#a027109503a988ba85f4c63b55082907fa078e7c3b0702b607e3e18a3daf261e0f",
"namespace_ogre.html#ac036050d60e78ed73d02e95fe79884a7a5026f97516f2b1de37d0fb5235916ee9",
"namespace_ogre_1_1_pixel_format_data_types.html#a320bd6a6b273a3abe76add6b64ff8540afbc604739db739ea551e000fe2c30930",
"struct_d_x_g_i___s_w_a_p___c_h_a_i_n___d_e_s_c1.html#ab079b5448d5b0c0b540a00e7cc85775e",
"struct_ogre_1_1_cb_draw_indexed.html#a0aa8233200babb247d5985cef0eb55d5",
"struct_ogre_1_1_d3_d11_vao_manager_1_1_block.html",
"struct_ogre_1_1_entity_material_lod_changed_event.html",
"struct_ogre_1_1_hidden_area_vr_settings.html#a8ab0b721d456556fcf54305f9c0c481c",
"struct_ogre_1_1_hlms_macroblock.html#a4b9208c9dd4efb98a7d3e4508a9636afa0e7a97aa4d7afbf214753253ac8d62e9",
"struct_ogre_1_1_lod_config.html#a7eecc87589b5a3eacfa363850c2fe4c0",
"struct_ogre_1_1_lod_work_queue_request.html#a7b36867cf35d5bbfc3c43c00c7bf3884",
"struct_ogre_1_1_particle_gpu_data.html#a4a9d3e83054c0f8881bb5176ab2d4b89",
"struct_ogre_1_1_render_target_view_entry.html#ad9074a1aa80f3ef03f6fea820439a87e",
"struct_ogre_1_1_sub_mesh_1_1_source_data.html",
"struct_ogre_1_1_unlit_property.html#a589b56a42bd6867d756be461af9ccee7",
"struct_ogre_1_1_vertex_array_object.html#a9f2065bd198fb37576ae64c292d1bd74",
"struct_ogre_1_1_vr_data.html#a36bc3390e5f1e33d07cf195cf417f452",
"struct_ogre_1_1_vulkan_physical_device.html#a9f5a76f131bfae4dedeeeb6946a1089a",
"struct_ogre_1_1v1_1_1_cb_start_v1_legacy_rendering.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';