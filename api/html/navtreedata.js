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
  [ "OGRE", "index.html", [
    [ "API Reference Start Page", "index.html", "index" ],
    [ "Manual", "manual.html", [
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
      [ "Compositor", "compositor.html", [
        [ "Nodes", "compositor.html#CompositorNodes", [
          [ "Input & output channels and RTTs", "compositor.html#CompositorNodesChannelsAndRTTs", [
            [ "Locally declared textures", "compositor.html#CompositorNodesChannelsAndRTTsLocalTextures", null ],
            [ "It comes from an input channel", "compositor.html#CompositorNodesChannelsAndRTTsFromInputChannel", null ],
            [ "It is a global texture", "compositor.html#CompositorNodesChannelsAndRTTsGlobal", null ],
            [ "Main RenderTarget", "compositor.html#CompositorNodesChannelsAndRTTsMainRenderTarget", null ]
          ] ],
          [ "Target", "compositor.html#CompositorNodesTarget", null ],
          [ "Passes", "compositor.html#CompositorNodesPasses", [
            [ "clear", "compositor.html#CompositorNodesPassesClear", null ],
            [ "generate_mipmaps", "compositor.html#CompositorNodesPassesGenerateMipmaps", null ],
            [ "quad", "compositor.html#CompositorNodesPassesQuad", null ],
            [ "resolve", "compositor.html#CompositorNodesPassesResolve", null ],
            [ "render_scene", "compositor.html#CompositorNodesPassesRenderScene", null ],
            [ "stencil", "compositor.html#CompositorNodesPassesStencil", null ],
            [ "uav_queue", "compositor.html#CompositorNodesPassesUavQueue", [
              [ "Synchronization", "compositor.html#CompositorNodesPassesUavQueueSync", null ]
            ] ],
            [ "compute", "compositor.html#CompositorNodesPassesCompute", null ]
          ] ],
          [ "Textures", "compositor.html#CompositorNodesTextures", [
            [ "MSAA: Explicit vs Implicit resolves", "compositor.html#CompositorNodesTexturesMsaa", [
              [ "Implicit resolves", "compositor.html#CompositorNodesTexturesMsaaImplicit", null ],
              [ "Explicit resolves", "compositor.html#CompositorNodesTexturesMsaaExplicit", null ],
              [ "Resources", "compositor.html#CompositorNodesTexturesMsaaResources", null ]
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
          ] ],
          [ "Writing shaders", "compositor.html#CompositorShadowNodesShaders", null ]
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
        [ "What technique should I choose?", "_gi_methods.html#GiWhatTechniqueChoose", null ]
      ] ],
      [ "Instancing", "instancing.html", [
        [ "What is instancing?", "instancing.html#WhatIsInstancing", null ],
        [ "Instancing 101", "instancing.html#Instancing101", [
          [ "Instances per batch", "instancing.html#InstancesPerBatch", null ]
        ] ],
        [ "Techniques", "instancing.html#InstancingTechniques", [
          [ "ShaderBased", "instancing.html#InstancingTechniquesShaderBased", null ],
          [ "VTF (Software)", "instancing.html#InstancingTechniquesVTFSoftware", null ],
          [ "HW VTF", "instancing.html#InstancingTechniquesHWVTF", [
            [ "HW VTF LUT", "instancing.html#InstancingTechniquesHW", null ]
          ] ],
          [ "HW Basic", "instancing.html#InstancingTechniquesHWBasic", null ]
        ] ],
        [ "Custom parameters", "instancing.html#InstancingCustomParameters", null ],
        [ "Supporting multiple submeshes", "instancing.html#InstancingMultipleSubmeshes", null ],
        [ "Defragmenting batches", "instancing.html#InstancingDefragmentingBatches", [
          [ "What is batch fragmentation?", "instancing.html#InstancingDefragmentingBatchesIntro", null ],
          [ "Prevention: Avoiding fragmentation", "instancing.html#InstancingDefragmentingBatchesPrevention", null ],
          [ "Cure: Defragmenting on the fly", "instancing.html#InstancingDefragmentingBatchesOnTheFly", null ]
        ] ],
        [ "Troubleshooting", "instancing.html#InstancingTroubleshooting", null ]
      ] ],
      [ "Threading", "threading.html", [
        [ "Initializing", "threading.html#ThreadingInitializing", [
          [ "Ideal number of threads", "threading.html#ThreadingInitializingNumberOfThreads", null ],
          [ "More info about InstancingThreadedCullingMethod", "threading.html#ThreadingInitializingCullingMethod", null ]
        ] ],
        [ "What tasks are threaded in Ogre", "threading.html#ThreadingInOgre", null ],
        [ "Using Ogre's threading system for custom tasks", "threading.html#ThreadingCustomTasks", null ],
        [ "Thread safety of SceneNodes", "threading.html#ThreadSafetySceneNodes", null ]
      ] ],
      [ "Performance Hints", "performance.html", null ],
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
          [ "fillBuffersFor", "hlms.html#HlmsRuntimeRenderingFillBuffersFor", null ]
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
        ] ]
      ] ],
      [ "AZDO changes (Aproaching Zero Driver Overhead)", "azdo.html", [
        [ "V2 and v1 objects", "azdo.html#V2AndV1Objects", [
          [ "Longevity of the v1 objects and deprecation", "azdo.html#V2AndV1ObjectsV1Longevity", null ]
        ] ],
        [ "Render Queue", "azdo.html#RenderQueue", null ],
        [ "The VaoManager", "azdo.html#VaoMaanger", null ]
      ] ],
      [ "The Command Buffer", "commandbuffer.html", [
        [ "Adding a command", "commandbuffer.html#CommandBufferAddCommand", null ],
        [ "Structure of a command", "commandbuffer.html#CommandBufferCommandStructure", null ],
        [ "Execution table", "commandbuffer.html#CommandBufferExecutionTable", [
          [ "Hacks and Tricks", "commandbuffer.html#CommandBufferExecutionTableHacks", null ]
        ] ],
        [ "Post-processing the command buffer", "commandbuffer.html#CommandBufferPostProcessing", null ]
      ] ],
      [ "What's new in Ogre 2.2", "_ogre22_changes.html", [
        [ "Load Store semantics", "_ogre22_changes.html#autotoc_md17", [
          [ "Now that we’ve explained how TBDRs work, we can explain load and store actions", "_ogre22_changes.html#autotoc_md18", null ]
        ] ],
        [ "More control over MSAA", "_ogre22_changes.html#autotoc_md19", null ],
        [ "Porting to Ogre 2.2 from 2.1", "_ogre22_changes.html#autotoc_md20", [
          [ "PixelFormats", "_ogre22_changes.html#autotoc_md21", [
            [ "Common pixel format equivalencies", "_ogre22_changes.html#autotoc_md22", null ]
          ] ],
          [ "Useful code snippets", "_ogre22_changes.html#autotoc_md23", [
            [ "Create a TextureGpu based on a file", "_ogre22_changes.html#autotoc_md24", null ],
            [ "Create a TextureGpu based that you manually fill", "_ogre22_changes.html#autotoc_md25", null ],
            [ "Uploading data to a TextureGpu", "_ogre22_changes.html#autotoc_md26", null ],
            [ "Upload streaming", "_ogre22_changes.html#autotoc_md27", null ],
            [ "Downloading data from TextureGpu into CPU", "_ogre22_changes.html#autotoc_md28", null ],
            [ "Downloading streaming", "_ogre22_changes.html#autotoc_md29", null ]
          ] ]
        ] ],
        [ "Difference between depth, numSlices and depthOrSlices", "_ogre22_changes.html#autotoc_md30", null ],
        [ "Memory layout of textures and images", "_ogre22_changes.html#autotoc_md31", null ],
        [ "Troubleshooting errors", "_ogre22_changes.html#autotoc_md32", null ],
        [ "RenderPassDescriptors", "_ogre22_changes.html#autotoc_md33", null ],
        [ "DescriptorSetTexture & co.", "_ogre22_changes.html#autotoc_md34", null ],
        [ "Does 2.2 interoperate well with the HLMS texture arrays?", "_ogre22_changes.html#autotoc_md35", null ],
        [ "Hlms porting", "_ogre22_changes.html#autotoc_md36", null ],
        [ "Things to watch out when porting", "_ogre22_changes.html#autotoc_md37", null ]
      ] ],
      [ "Behavor of StagingTexture in D3D11", "_behavor_staging_texture_d3_d11.html", [
        [ "Attempting to be contiguous", "_behavor_staging_texture_d3_d11.html#autotoc_md6", null ],
        [ "Slicing in 3", "_behavor_staging_texture_d3_d11.html#autotoc_md7", null ],
        [ "Slicing in the middle", "_behavor_staging_texture_d3_d11.html#autotoc_md8", null ]
      ] ]
    ] ],
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
        [ "Install cmake", "_setting_up_ogre_mac_o_s.html#autotoc_md38", null ],
        [ "Install dependencies", "_setting_up_ogre_mac_o_s.html#autotoc_md39", null ],
        [ "Build Ogre", "_setting_up_ogre_mac_o_s.html#autotoc_md40", null ],
        [ "Build offline version of the docs", "_setting_up_ogre_mac_o_s.html#autotoc_md41", null ]
      ] ]
    ] ],
    [ "Using Ogre in your App", "_using_ogre_in_your_app.html", [
      [ "Overview", "_using_ogre_in_your_app.html#UsingOgreInYourAppOverview", null ],
      [ "Speeding things up", "_using_ogre_in_your_app.html#SpeedingThingsUp", [
        [ "Small note about iOS", "_using_ogre_in_your_app.html#SmallNoteAbout_iOS", null ],
        [ "Creating your application with 'EmptyProject' script", "_using_ogre_in_your_app.html#CreatingYourApplication", [
          [ "A note about copied files from Samples/2.0/Common", "_using_ogre_in_your_app.html#AnoteaboutcopiedfilesfromSamples_20_Common", null ]
        ] ],
        [ "Supporting Multithreading loops from the get go", "_using_ogre_in_your_app.html#SupportingMultithreadingLoopsFromTheGetGo", null ]
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
"_ogre_camera_8h.html",
"_ogre_d3_d11_render_window_8h.html",
"_ogre_g_l_e_s2_prerequisites_8h.html#a3d3b7e52f4521f60f2793bfc1273c6f2",
"_ogre_hlms_pbs_prerequisites_8h.html#ae13b0f0b5217cbcc8aa6107daac70dae",
"_ogre_metal_texture_gpu_8h.html",
"_ogre_polygon_8h.html",
"_ogre_ribbon_trail_8h.html",
"_ogre_texture_gpu_manager_8h.html#a3a978c834ce00930123468c9a64047b4",
"_ogre_vertex_elements_8h.html#a027109503a988ba85f4c63b55082907fa1a6f5599d29d19a96f284c76be2d9965",
"class_ogre_1_1_a_p_k_file_system_archive.html#a3bf1d801c67ee89e48d8a3299cfc0a7e",
"class_ogre_1_1_android_e_g_l_context.html#a08612fd3eea707121938bb0197e454c6",
"class_ogre_1_1_android_e_g_l_window.html#a8a464e98db9c09042b866bdb95b210f4",
"class_ogre_1_1_anti_portal.html#a015c3801632c98bca5dbb5aa3da1aa24",
"class_ogre_1_1_anti_portal.html#aed7b889a1e10f9816c83127d0e13f09f",
"class_ogre_1_1_area_emitter.html#a51159bb05720260843eb80fde34261fd",
"class_ogre_1_1_array_matrix4.html#a32e1a78690ef8d554e10fa937d5a6f31",
"class_ogre_1_1_array_sphere.html#a7670729740e21cdd1d5ae3e554ba3cf5",
"class_ogre_1_1_async_texture_ticket.html#a6c76b44f26be0d76c764fe2d5597a26f",
"class_ogre_1_1_auto_param_data_source.html#ace31465a839948a9c08f67049e2059a1",
"class_ogre_1_1_bone.html#addcf86004230191108a13f3db29b8d04",
"class_ogre_1_1_box_emitter.html#ab39d2d327c694ef553fc3159b680d1db",
"class_ogre_1_1_bsp_level.html#a74138c0203835513613af73cdb08890d",
"class_ogre_1_1_bsp_resource_manager.html#a7913c36c02ef03007ff1a1c76ddf7710",
"class_ogre_1_1_bsp_scene_manager.html#a4ac962924e49d9686471e3e639177129",
"class_ogre_1_1_bsp_scene_manager.html#aa9509d9c69659cfa5ee4a09532e461ea",
"class_ogre_1_1_bsp_scene_manager_plugin.html#adce32528577d71b228692df906ecd17c",
"class_ogre_1_1_bsp_scene_node.html#aeff8334b9da08af47e6bddd0f5042b31",
"class_ogre_1_1_camera.html#a19bf2951b97744064509c13068143f3d",
"class_ogre_1_1_camera.html#a865cb46cc09cbb8bd560abeca1d0d31e",
"class_ogre_1_1_camera.html#afb58624fb567bb790c55d8a716b472da",
"class_ogre_1_1_codec_1_1_codec_data.html#a8d1d3dc4a466a4e992aad72c14bf81eb",
"class_ogre_1_1_colour_fader_affector_1_1_cmd_green_adjust.html",
"class_ogre_1_1_colour_value.html#a21875c1d92c3ad0cba9cafd5776f42a0",
"class_ogre_1_1_compositor_manager2.html#aedef19dd7de5ab286f880e505cb3f834",
"class_ogre_1_1_compositor_pass_clear_def.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_compositor_pass_ibl_specular_def.html#a19d4bd88e211650df13db9ffd314e680",
"class_ogre_1_1_compositor_pass_quad_def.html#aec8f6ba4b673d0032866e19837785c87",
"class_ogre_1_1_compositor_pass_uav.html#afe11bd546cf7315f4526488e7078cc7dabb2a61a0002ccca6afbde7588787e58e",
"class_ogre_1_1_compositor_shadow_node_def.html#adce32528577d71b228692df906ecd17c",
"class_ogre_1_1_compute_tools.html#a5c706a12694c079c70a4d633f3d6c329",
"class_ogre_1_1_const_map_iterator.html#abded82ac05c8d0d3e56c6651eff3cad8",
"class_ogre_1_1_cubemap_probe.html#a9bc62d893b4df689a1abe1c9c2875556",
"class_ogre_1_1_cylinder_emitter.html#aeb086f851624a470e1b5592b81418d39",
"class_ogre_1_1_d3_d11_compat_buffer_interface.html#a1b7b1f47a858a1b14ac3c7b0e8bb9007",
"class_ogre_1_1_d3_d11_depth_texture.html#a1686b8aef3b2c8d3892ae453086073c4",
"class_ogre_1_1_d3_d11_depth_texture_target.html#a15f59cd1b4a2b2e65fcee349d78ae490a5ee29312d754f2244987dc0d1f5894e8",
"class_ogre_1_1_d3_d11_driver_list.html#a42da70f21a3c50454df5801e860481ad",
"class_ogre_1_1_d3_d11_h_l_s_l_program.html#a1aa750a4b994f2b1066220936cdfdb94a70589e0b87b09b0a4fd98ce490dc9782",
"class_ogre_1_1_d3_d11_h_l_s_l_program.html#aff5bc7363bd68cfdf653e380e68904d4",
"class_ogre_1_1_d3_d11_multi_render_target.html#a8e1a9f2c62e6524921b2983413e01e67",
"class_ogre_1_1_d3_d11_null_texture.html#a94c448129841867a63e2336f31d6fb8f",
"class_ogre_1_1_d3_d11_null_texture_target.html#af8ca9d240bd48662c37dbdf8fa881bad",
"class_ogre_1_1_d3_d11_render_system.html#a5bc6482cb17b2519658302a4aa0b2415",
"class_ogre_1_1_d3_d11_render_system.html#adceb84c1be629865821a15c74d4f0c4f",
"class_ogre_1_1_d3_d11_render_window_base.html#a30ad5ebed94e14efecc178706807ac33",
"class_ogre_1_1_d3_d11_render_window_swap_chain_based.html#a552ded694f3bd7013e8d1d8d14f144ba",
"class_ogre_1_1_d3_d11_stereo_driver_a_m_d.html#a0e7cf0d5efd1e52eaa23b31e0e2013a0",
"class_ogre_1_1_d3_d11_texture.html#a07e619aa09d3bc6789a6667cb4fc572cab43c913411f4a96722902f17b88f10fb",
"class_ogre_1_1_d3_d11_texture_gpu.html#a2afd94abec3a7a91c656188fc6d7cdf5",
"class_ogre_1_1_d3_d11_texture_gpu_manager.html#ab9065d7ce750a69d07f0d315d623b2cc",
"class_ogre_1_1_d3_d11_texture_gpu_render_target.html#aeebb419ba824d0128f437bbe20cc81f9",
"class_ogre_1_1_d3_d11_texture_manager.html#a117a3887bbf9e83e0585777beb0501dc",
"class_ogre_1_1_d3_d11_vao_manager.html#a21be4e3d6b7ad0490c59eb9db1cdcdf7",
"class_ogre_1_1_d3_d11_window.html#aabfaa3d20b4ad5f0ad0b30ccaf6f2ef7",
"class_ogre_1_1_d3_d9_device.html",
"class_ogre_1_1_d3_d9_gpu_fragment_program.html#a1aa750a4b994f2b1066220936cdfdb94a70589e0b87b09b0a4fd98ce490dc9782",
"class_ogre_1_1_d3_d9_gpu_program.html#a1bea65fa2b3c2ac5a9f3be59b46a80e8",
"class_ogre_1_1_d3_d9_gpu_program_manager.html#a4d3289d4636168230da11c1c9794db59",
"class_ogre_1_1_d3_d9_gpu_vertex_program.html#a699d1c851f494a8d5d23d23b695ceb47",
"class_ogre_1_1_d3_d9_h_l_s_l_program.html#a5d3962a2bef3bd77de412c8207975caf",
"class_ogre_1_1_d3_d9_hardware_index_buffer.html#a6d817fd480541f9a5e855c5453c1a7de",
"class_ogre_1_1_d3_d9_multi_render_target.html#a32f2e646a09dcdaef810ea402bc92155",
"class_ogre_1_1_d3_d9_render_system.html#a23cc0a7d87b77ec3d0a34e92335bec1a",
"class_ogre_1_1_d3_d9_render_system.html#a9b63f3873cf44d872b67399fa10e5d9c",
"class_ogre_1_1_d3_d9_render_texture.html#a2bdfaa76a186e49b807b06c7a92b720e",
"class_ogre_1_1_d3_d9_render_window.html#a8c07a9eff234ae9ce99360d7b0feb41b",
"class_ogre_1_1_d3_d9_stereo_driver_bridge.html#ac5972a083e75b8501ec2cf8af94994b6",
"class_ogre_1_1_d3_d9_texture.html#aa38c4f67ab5413135d1830dd164a68e6",
"class_ogre_1_1_d3_d9_vertex_declaration.html#a842f42e52d16d44819abb342fc182cad",
"class_ogre_1_1_decal.html#a57123ae4fec1b472f25270c1dd6abe59",
"class_ogre_1_1_default_axis_aligned_box_scene_query.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_default_ray_scene_query.html#aee26bdd5505fbcad6a077124810738af",
"class_ogre_1_1_default_scene_manager.html#a5fdfd9e790d2630ce07fffd8fb5a84f1",
"class_ogre_1_1_default_scene_manager.html#ab8c1c6962ad94e28ac4a202f1fd16772",
"class_ogre_1_1_default_sphere_scene_query.html#aec3aa0b8b16c41fae990ee78150f8d2a",
"class_ogre_1_1_default_zone.html#abc8ebac2f46f22bb525856ec91be771e",
"class_ogre_1_1_deflector_plane_affector_factory.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1_distance_lod_sphere_strategy.html#afcbca1aed7b649a98adc3393ce177fc7",
"class_ogre_1_1_e_g_l_support.html#aee9a5957c98ac70592dc9ec82a862d68",
"class_ogre_1_1_e_g_l_window.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1_egl_p_buffer_window.html#a3ff24dc15ec59e058aadea1e1381b8ed",
"class_ogre_1_1_ellipsoid_emitter.html#aac8039cf60bcb0fe8a0ca19d149948d3",
"class_ogre_1_1_emitter_commands_1_1_cmd_velocity.html#a50410db5dcffc17ff480db30ad3a9c51",
"class_ogre_1_1_emscripten_e_g_l_window.html#a83616cbcf3c7ec12efb8c9a8bc2affb0",
"class_ogre_1_1_external_texture_source_manager.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_file_system_layer.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_forward3_d.html#aa169a9da93f7be67ef0315e0db6200de",
"class_ogre_1_1_frustum.html#a2d458a92423f3b26d0904d02e85798fd",
"class_ogre_1_1_g_l3_plus_async_texture_ticket.html#afe11bd546cf7315f4526488e7078cc7da55066351a26967068b944c2170e51af1",
"class_ogre_1_1_g_l3_plus_depth_buffer.html#a9854c57374adef41216bae68544a5b7ead40798189fef10b6b2e171253046cd84",
"class_ogre_1_1_g_l3_plus_depth_texture.html#ae229b1f58092109adef55bfc6f864d79",
"class_ogre_1_1_g_l3_plus_f_b_o_multi_render_target.html",
"class_ogre_1_1_g_l3_plus_f_b_o_render_texture.html#a62a35f19422bd10282b044909e757b2e",
"class_ogre_1_1_g_l3_plus_null_texture.html#a1aa750a4b994f2b1066220936cdfdb94ab49695ac949df3a7d2e2f9a03267097c",
"class_ogre_1_1_g_l3_plus_null_texture_target.html#a4b42772d32712767391c749f4745397aaa9d982cf6f703ba525577fdd487ae5e8",
"class_ogre_1_1_g_l3_plus_render_system.html#a051be33bcc0b33744f7f507e65d0bce1",
"class_ogre_1_1_g_l3_plus_render_system.html#a91e7ee7dd1f244ffa9a0681efb6fae6c",
"class_ogre_1_1_g_l3_plus_render_texture.html#a552ded694f3bd7013e8d1d8d14f144ba",
"class_ogre_1_1_g_l3_plus_support.html#a813134d9af879a2c2e7e93b03379e513",
"class_ogre_1_1_g_l3_plus_texture.html#a1aa750a4b994f2b1066220936cdfdb94ae5f2eca654bd810da4c774570a5b4b77",
"class_ogre_1_1_g_l3_plus_texture_gpu.html#a4d6bc1fdda7b86b12d3b3656ec53ce77",
"class_ogre_1_1_g_l3_plus_texture_gpu_headless_window.html#a65a6949bdace3ca3e22d9cdb8cc77411",
"class_ogre_1_1_g_l3_plus_texture_gpu_render_target.html#a31a1ba96b03d3bab1a3c5943d03531fc",
"class_ogre_1_1_g_l3_plus_texture_gpu_window.html#a4c0edfcf7c6354c686d2ff580e7a5011",
"class_ogre_1_1_g_l3_plus_texture_manager.html#a9e645d37d4428ad9258a0572c4aecb37",
"class_ogre_1_1_g_l3_plus_vao_manager.html#ad0260436ff6254a7f977f308148fa964",
"class_ogre_1_1_g_l_e_s2_depth_buffer.html#a9854c57374adef41216bae68544a5b7ea821faf32a09ac92606a6585fc61e24f7",
"class_ogre_1_1_g_l_e_s2_f_b_o_manager.html",
"class_ogre_1_1_g_l_e_s2_frame_buffer_object.html#aba944aa9d091e9214f73318d6e6ee19d",
"class_ogre_1_1_g_l_e_s2_null_texture.html#a80f0523a2ba230ba6e637e34a7b35463",
"class_ogre_1_1_g_l_e_s2_r_t_t_manager.html#a270434b8c3c065d0abb8c27aa2a91885",
"class_ogre_1_1_g_l_e_s2_render_system.html#a80605199eceb6f34f01230242d1f8c41",
"class_ogre_1_1_g_l_e_s2_render_system.html#afdea9fc18369642840cc829cd00bfddf",
"class_ogre_1_1_g_l_e_s2_support.html#a3bb7ba22015a2517d13d48fb94d7c236",
"class_ogre_1_1_g_l_e_s2_texture_manager.html#ab422c67a2c8844f410fe822a357cba3b",
"class_ogre_1_1_g_l_s_l_e_s_program_manager_common.html",
"class_ogre_1_1_g_l_s_l_program.html#a1a0fc5acd82fdb82ddfa6c046dad1473",
"class_ogre_1_1_g_l_s_l_shader.html#a5172618ce54ccd45b89769b3e8ae1d51",
"class_ogre_1_1_g_l_s_l_shader_manager.html#a2a3ef36f60dc007920eb9bba20ef9125",
"class_ogre_1_1_g_l_x_g_l_support.html#af444cffc272f1d56cc33325cae194e18",
"class_ogre_1_1_g_t_k_window.html#a32f2e646a09dcdaef810ea402bc92155",
"class_ogre_1_1_gpu_program.html#a104d9db8bfcff1f37b29157bbe070f7a",
"class_ogre_1_1_gpu_program_parameters.html#a155c886f15e0c10d2c33c224f0d43ce3a5a35ece35857f86f44c16c95511100c7",
"class_ogre_1_1_gpu_program_translator.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_hashed_vector.html#a126b57c573ec7654ed6a848ff6818303",
"class_ogre_1_1_high_level_gpu_program_manager.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_hlms_compute_job.html#a260ffa4d1c79eb09cc51480f9c79b5b1",
"class_ogre_1_1_hlms_low_level.html#a6e629e7a87a40040e12d2bfb4ff80d58a1aba1d888e0aa8e695416e9ad2af44b9",
"class_ogre_1_1_hlms_pbs.html#ab422c67a2c8844f410fe822a357cba3b",
"class_ogre_1_1_hlms_pbs_mobile_datablock.html#abfdafe7756d34fd2e4992caba072b248",
"class_ogre_1_1_hlms_unlit_mobile.html#a6e629e7a87a40040e12d2bfb4ff80d58a792d2cdce23b4018b9c149540ebb1a93",
"class_ogre_1_1_hollow_ellipsoid_emitter.html#aa4cc84bb842666d1a69d25375dac5d12",
"class_ogre_1_1_ifd_probe_visualizer.html#a38f8241458458e4f1235eae7d8f850c3",
"class_ogre_1_1_image.html#a5312a67dfaeb1716ae9ab72c1af766cb",
"class_ogre_1_1_index_buffer_packed.html#a29ff95c6bebe87dcfe2d43a02cc0ba6c",
"class_ogre_1_1_internal_cubemap_probe.html#a7d491859c987cf03cd2b62121e9bfea0",
"class_ogre_1_1_invalid_call_exception.html#ab4518b52083342bc207ef3b677114e38",
"class_ogre_1_1_item.html#a78b7d68ed7d7f0010c80328cc61f459a",
"class_ogre_1_1_kf_transform_array_memory_manager.html#afb8eb119d1603f70bd3d2f333f129e20",
"class_ogre_1_1_light.html#ab703ec1f1cf82763b0ac9c4b1e51a17b",
"class_ogre_1_1_linear_force_affector.html#a8357fe4fb4849772b94baa4bf47c7ded",
"class_ogre_1_1_lod_output_provider_buffer.html#a4de90dd96a42ac8ed0f3a8c170073835",
"class_ogre_1_1_lw_const_string.html#a811b55e86aebefb4d3b7ffe9b80f5e3c",
"class_ogre_1_1_manual_object.html#a8b4d68a9227896fdf1ab39568ae57f89",
"class_ogre_1_1_manual_object_1_1_manual_object_section.html#adf2a2570b7d6c5dbdb80d8f98e620cbd",
"class_ogre_1_1_material_manager.html#a0e655f4afb2eb68bbcf280f2879e20b6",
"class_ogre_1_1_mathlib_c.html#a3c6672b2e6b4420e0e70fa2f5786a719",
"class_ogre_1_1_mesh.html#a07e619aa09d3bc6789a6667cb4fc572ca4228d390b3a9ed614001db177d1f441e",
"class_ogre_1_1_mesh_serializer_impl__v2__1___r0.html#a595ea4c05da8aa987d3800e65d23355d",
"class_ogre_1_1_metal_const_buffer_packed.html#aac73dd5b3bfea12dca1c5a64e6e95c7a",
"class_ogre_1_1_metal_depth_texture.html#a6ed49ed8701feb087b04b730ce1d682f",
"class_ogre_1_1_metal_depth_texture_target.html#acfb338c927b445144253ce6893bbbb55",
"class_ogre_1_1_metal_gpu_program_manager.html#a964ed591329cc8046c9fbab0713a35ce",
"class_ogre_1_1_metal_multi_render_target.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_metal_null_texture.html#ab7e6d7b70a6256eca73709ab487c8407",
"class_ogre_1_1_metal_plugin.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_metal_program.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1_metal_render_system.html#a4675ea90e37a28ebcb60f0aeeea0dce8",
"class_ogre_1_1_metal_render_system.html#adacd8a9cdfaf24b2aab87289de657189",
"class_ogre_1_1_metal_render_texture.html#af8c3f070c7c1f59e4be242531b1ec9b4",
"class_ogre_1_1_metal_texture.html#a104d9db8bfcff1f37b29157bbe070f7a",
"class_ogre_1_1_metal_texture_gpu.html#a3db88cd9e4286f47a56fe7f507cd5c1b",
"class_ogre_1_1_metal_texture_gpu_manager.html#afe11bd546cf7315f4526488e7078cc7d",
"class_ogre_1_1_metal_texture_gpu_window.html#a107372aa3256baf8a80484b7f2412dd6",
"class_ogre_1_1_metal_texture_manager.html#a42ae3de59dd5ccfd449645ec4af1143c",
"class_ogre_1_1_metal_vao_manager.html#a4d56b87f51c489bdc2df07d1ce040512",
"class_ogre_1_1_movable_object.html#a4a40a12d7cd0f7fa4c4c0fc31bbf9a40",
"class_ogre_1_1_movable_plane.html#a3f97462d3822f6add8c070048f6c2d92",
"class_ogre_1_1_multi_render_target.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_n_u_l_l_const_buffer_packed.html#a8078a50b48347290ef01e1c3e037dc0e",
"class_ogre_1_1_n_u_l_l_render_system.html#a5b95f508351c5eabab5bd6efa15741dc",
"class_ogre_1_1_n_u_l_l_render_system.html#aed5a59574d64e3e1f95ecd8cab66870c",
"class_ogre_1_1_n_u_l_l_render_window.html#a4b42772d32712767391c749f4745397aa632020df02d4d37ee90ff4ff0a5b7b86",
"class_ogre_1_1_n_u_l_l_staging_texture.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_n_u_l_l_texture.html#aa561d8d03383bb59ecce5c4278ed8da6",
"class_ogre_1_1_n_u_l_l_texture_gpu_manager.html#afe11bd546cf7315f4526488e7078cc7da2b732a317de0915527e291485ca42e82",
"class_ogre_1_1_n_u_l_l_texture_manager.html#ad9b4efbeacb17f07710c30bd5048e42f",
"class_ogre_1_1_n_u_l_l_window.html#a595ea4c05da8aa987d3800e65d23355d",
"class_ogre_1_1_na_cl_window.html#a74f922747c11f98718be00c2fa86165c",
"class_ogre_1_1_node.html#ab57c31b84135b19d0ef12f4a9024e711",
"class_ogre_1_1_null_entity.html#a9a72ade896561e23c32cf8659f1640e4",
"class_ogre_1_1_obj_cmd_buffer.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_octree_axis_aligned_box_scene_query.html#aec3aa0b8b16c41fae990ee78150f8d2a",
"class_ogre_1_1_octree_camera.html#a6e80ce34c7f4ceb5b2deecf2517c57e0",
"class_ogre_1_1_octree_camera.html#ade9a67cb126367dd48883bf0df695679",
"class_ogre_1_1_octree_node.html#a5370ed8f834c105884e96913ecb0c356",
"class_ogre_1_1_octree_plane_bounded_volume_list_scene_query.html#ae52b991bb3e6ecdb688c3d06d245c468",
"class_ogre_1_1_octree_scene_manager.html#a366788e2422530c20281ff6c5afd68e2",
"class_ogre_1_1_octree_scene_manager.html#a935347e3d8a5af785e819abb901882f6",
"class_ogre_1_1_octree_scene_manager.html#af5714312a90c6f699d901ca41933ee6a",
"class_ogre_1_1_octree_zone.html#ae4fbfcc16340e4624692a434ead9cb5a",
"class_ogre_1_1_p_c_z_axis_aligned_box_scene_query.html#ac3b60af8a02139c85520a92a34920004",
"class_ogre_1_1_p_c_z_camera.html#a6260eefc06a0bb2946a5c58d79750f3d",
"class_ogre_1_1_p_c_z_camera.html#ad22289842e973a7a6b727c8cd32e9be9",
"class_ogre_1_1_p_c_z_light.html#a0a7c23a753bf2c7b1ded3b6b874a82a5",
"class_ogre_1_1_p_c_z_light.html#ac4305df204604a027ca27ef77efff295",
"class_ogre_1_1_p_c_z_plugin.html#aceaa39473e822631dcfbc016f106d8da",
"class_ogre_1_1_p_c_z_scene_manager.html#a3a720414f68274bb9c5026911e27135d",
"class_ogre_1_1_p_c_z_scene_manager.html#a91dee910ef9412d09800e11b4c5f569c",
"class_ogre_1_1_p_c_z_scene_manager.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_p_c_z_scene_node.html#a7ed3d23f3069df5e3356369fc5174e9d",
"class_ogre_1_1_p_c_zone.html#a34886e1892df8103abd009e9fd4985e7",
"class_ogre_1_1_p_v_r_t_c_codec.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_page_strategy_data.html#ab422c67a2c8844f410fe822a357cba3b",
"class_ogre_1_1_parallax_corrected_cubemap_auto.html#a091333b335b9fa21ab6859145bb87e26",
"class_ogre_1_1_particle.html#ac9741b2623435c312819c9d3903c915e",
"class_ogre_1_1_particle_system.html#a1a048b8ace0a21cd2ed2c3008941ce3d",
"class_ogre_1_1_particle_system_renderer.html#a135d07c57490da7dccb849e0058c2b24",
"class_ogre_1_1_pass.html#af2643f7594c827d2f1b8785029de280f",
"class_ogre_1_1_pixel_util.html#ae127c3e6145af784e7d2d3734681a75a",
"class_ogre_1_1_point_emitter.html#a23a30a598c10f44f59ad4cfd78bf2a10",
"class_ogre_1_1_portal.html#a02110b4b9d0ddb498d384cf6f3ec8951",
"class_ogre_1_1_portal.html#aed7b889a1e10f9816c83127d0e13f09f",
"class_ogre_1_1_portal_base.html#ac4305df204604a027ca27ef77efff295",
"class_ogre_1_1_profiler.html#a8357fe4fb4849772b94baa4bf47c7ded",
"class_ogre_1_1_quake3_level.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1_r_t_shader_1_1_const_parameter.html#a4fa6286f097d19e92a8ed91645fbfbe7a36c0e0abf19306fa11f2304fa5c4f6a9",
"class_ogre_1_1_r_t_shader_1_1_f_f_p_colour.html#a874e97a7115c55b572e2efd471ba11e8ae05b2e49b207bcbe06646b598ce09d22",
"class_ogre_1_1_r_t_shader_1_1_function.html#ac305bd18bfe6edfcaf8bca3cec5df9fd",
"class_ogre_1_1_r_t_shader_1_1_integrated_p_s_s_m3_factory.html#a3628bf00c2ad7cd5ad955a749a82ed2a",
"class_ogre_1_1_r_t_shader_1_1_normal_map_lighting_factory.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_r_t_shader_1_1_parameter_factory.html#ad0461fc33fb6bbb8ebb1d732f2ccb17a",
"class_ogre_1_1_r_t_shader_1_1_shader_generator.html#a320c232700a45276ec30b54bcc7922c7",
"class_ogre_1_1_r_t_shader_1_1_texture_atlas_sampler_factory.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1_radial_density_mask.html#a63fc6bdc2eb6aeac24898a77ec5e2023",
"class_ogre_1_1_rectangle2_d.html#a41b58fe7a0d317cbc00052e6668cf4ef",
"class_ogre_1_1_rectangle2_d_factory.html#a8357fe4fb4849772b94baa4bf47c7ded",
"class_ogre_1_1_render_system.html#a6a34f2ded473d4cbaeda6e954f76dbaf",
"class_ogre_1_1_render_system_capabilities_serializer.html#a1c727e879a260c37b00ce5505fe8e144",
"class_ogre_1_1_render_window.html#a95768aeda225df73c3636f228f9f770b",
"class_ogre_1_1_renderable_animated.html#ad4d284fd219351af7f1ef9f9e9acab1e",
"class_ogre_1_1_resource_group_manager.html#afb3d5943565b32c927981e141c52c671",
"class_ogre_1_1_ring_emitter.html#aae174ed689d7748e3047bb4242440993",
"class_ogre_1_1_rotation_affector.html#a9dffcf1a2d754e63e98770e5cf2cd9ee",
"class_ogre_1_1_s_d_l_window.html#a595ea4c05da8aa987d3800e65d23355d",
"class_ogre_1_1_scale_affector_factory.html#a1c727e879a260c37b00ce5505fe8e144",
"class_ogre_1_1_scene_manager.html#a330ae308e614a06f201e1c460014b8c7",
"class_ogre_1_1_scene_manager.html#a99f32c5dc4717bdaea4dfa6f13eec151",
"class_ogre_1_1_scene_manager_enumerator.html#a1c727e879a260c37b00ce5505fe8e144",
"class_ogre_1_1_scene_node.html#ab085e833deecddee7b293fc5e91439f4",
"class_ogre_1_1_script_compiler.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_shadow_volume_extrude_program.html#a1c727e879a260c37b00ce5505fe8e144",
"class_ogre_1_1_singleton.html#ad77eda7e12f151aa36dcdf1286421de2",
"class_ogre_1_1_small_vector.html",
"class_ogre_1_1_small_vector_3_01_t_00_010_01_4.html#a08f0d82975d336032f922f602ac0f35f",
"class_ogre_1_1_small_vector_impl.html#a1bc36fd4f486783f4a5255e911c4178c",
"class_ogre_1_1_small_vector_template_base.html#a4b86aea9ff25e7749483c2431e02db41",
"class_ogre_1_1_small_vector_template_common.html",
"class_ogre_1_1_stack_vector.html#a7ed4677316369b6ae8b35beaf73ffd47",
"class_ogre_1_1_string_converter.html#a0356f16ae84fb3a715c7a5cd98fb5ff3",
"class_ogre_1_1_sub_mesh.html#abef0947bb29f195d87d822b5b2f8ee8a",
"class_ogre_1_1_technique.html#aed27a55db91366ee2f95a608f946bc86",
"class_ogre_1_1_terrain.html#afce38d08848608e3bf4642678e289181",
"class_ogre_1_1_terrain_lod_manager.html#a70d6a9494e245040a45b5ee000334b1e",
"class_ogre_1_1_terrain_paged_world_section_1_1_terrain_definer.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1_texture.html#a8ed6a95810f21f53aa8cbc780b680164",
"class_ogre_1_1_texture_filter_1_1_generate_sw_mipmaps.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_texture_gpu_manager.html#a763c9420839cf914f7a5c1901c53539b",
"class_ogre_1_1_texture_unit_state.html#a72a88b1cf11053da98fa25f98689931e",
"class_ogre_1_1_uav_buffer_packed.html#a8357fe4fb4849772b94baa4bf47c7ded",
"class_ogre_1_1_vao_manager.html#a1b746e3cfc05da2d1fded8949f27b0e4",
"class_ogre_1_1_vct_voxelizer.html#a47eabebe58c2532c9115e9022b9d462d",
"class_ogre_1_1_vector_iterator.html#a704ce78ef948a2730b4774b4ba0cea69",
"class_ogre_1_1_volume_1_1_c_s_g_intersection_source.html",
"class_ogre_1_1_volume_1_1_cache_source.html#a3f107f5949b7e9de9bd713f3a21f655a",
"class_ogre_1_1_volume_1_1_iso_surface_m_c.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1_voxel_visualizer.html#a040f6de2d0aa0b483acac3e69608c47c",
"class_ogre_1_1_voxel_visualizer.html#ae47c1fe914431bbbd47d5d2a3ba258d0",
"class_ogre_1_1_win32_e_g_l_window.html#a3649200bc76f575f6241beff14dd80d0",
"class_ogre_1_1_win32_g_l_support.html#a6141fd1becc6a161d37d08204d4a0f27",
"class_ogre_1_1_wire_aabb.html#a1c727e879a260c37b00ce5505fe8e144",
"class_ogre_1_1_wire_aabb.html#af630e6230261ecca2ecbfcf0b0ee9ce9",
"class_ogre_1_1_x11_e_g_l_window.html#a15f59cd1b4a2b2e65fcee349d78ae490",
"class_ogre_1_1_x11_e_g_l_window.html#ae1603d5b0e983523ea1c1ba3401f6506",
"class_ogre_1_1v1_1_1_animation.html#ab12e71ce35ae8afb8d50731e46bedd39",
"class_ogre_1_1v1_1_1_billboard_chain.html#a5456ecd630d55c91b2fd1b19969d29f5",
"class_ogre_1_1v1_1_1_billboard_set.html#a1a048b8ace0a21cd2ed2c3008941ce3d",
"class_ogre_1_1v1_1_1_border_panel_overlay_element.html#a3c600369ab5d6957524125c93915019f",
"class_ogre_1_1v1_1_1_d3_d11_depth_pixel_buffer.html#a0ff8419b84db268e64231f2748ade783",
"class_ogre_1_1v1_1_1_d3_d11_hardware_buffer_manager.html#ab422c67a2c8844f410fe822a357cba3b",
"class_ogre_1_1v1_1_1_d3_d11_hardware_pixel_buffer.html#a787c4f5ca31d533e63f089f6d35a5e8e",
"class_ogre_1_1v1_1_1_d3_d11_null_pixel_buffer.html",
"class_ogre_1_1v1_1_1_default_hardware_counter_buffer.html#ac0c56e93ce6f29bf39fb48c3d5dc64cf",
"class_ogre_1_1v1_1_1_entity.html#a347c70bfad5b0c1e1d804172d3e5f4e8",
"class_ogre_1_1v1_1_1_entity.html#af19ca18abcd6ca7bceb2f55a814e1c1b",
"class_ogre_1_1v1_1_1_g_l3_plus_default_hardware_counter_buffer.html#a9f7b1a5624f5a89781b0870f919152e4",
"class_ogre_1_1v1_1_1_g_l3_plus_default_hardware_vertex_buffer.html#a5caebd4b5db7696029eb412351d7767f",
"class_ogre_1_1v1_1_1_g_l3_plus_hardware_counter_buffer.html",
"class_ogre_1_1v1_1_1_g_l3_plus_hardware_pixel_buffer.html#a87632db14ce9c10e113f1966c6a97c6dab5fe6ef729ea615265721a10ff7d9e57",
"class_ogre_1_1v1_1_1_g_l3_plus_hardware_vertex_buffer.html#a4674ce75bfc84fa7e6ac03a059f0dbb5",
"class_ogre_1_1v1_1_1_g_l3_plus_texture_buffer.html#a4c89e8f3e555b5baf18bcae16d5fb8dd",
"class_ogre_1_1v1_1_1_g_l_e_s2_default_hardware_buffer_manager_base.html#adce32528577d71b228692df906ecd17c",
"class_ogre_1_1v1_1_1_g_l_e_s2_default_hardware_vertex_buffer.html#ac0c56e93ce6f29bf39fb48c3d5dc64cfa364f94b757045261468e68fe09c36cca",
"class_ogre_1_1v1_1_1_g_l_e_s2_hardware_pixel_buffer.html#a787c4f5ca31d533e63f089f6d35a5e8e",
"class_ogre_1_1v1_1_1_g_l_e_s2_render_buffer.html#ab60da8e622506487a7d44200a1c704b6",
"class_ogre_1_1v1_1_1_hardware_buffer_licensee.html#a79993f3120fffb36d8c97cafc84ea938",
"class_ogre_1_1v1_1_1_hardware_pixel_buffer.html#ac0c56e93ce6f29bf39fb48c3d5dc64cfae160717b1f864d2d91aa2ba24ac38771",
"class_ogre_1_1v1_1_1_manual_object.html#a3a0931d4fb9b2dfb348257b40d15110e",
"class_ogre_1_1v1_1_1_manual_object_1_1_manual_object_section.html#a3bee104c7359ba6c41d65d88cfc3d418",
"class_ogre_1_1v1_1_1_mesh.html#a7fc68fb27ecf6df13e48f55dc2bf63c6",
"class_ogre_1_1v1_1_1_mesh_serializer_impl__v1__2.html#a595ea4c05da8aa987d3800e65d23355d",
"class_ogre_1_1v1_1_1_metal_hardware_buffer_manager_base.html#aaec7f834aff615ec9efd3169658950b0",
"class_ogre_1_1v1_1_1_metal_hardware_vertex_buffer.html#a314bdb43214d6d0ec151d9eb628fe6c3",
"class_ogre_1_1v1_1_1_metal_texture_buffer.html#a87632db14ce9c10e113f1966c6a97c6da1de64eadc98dfb60e9dcdf7fe6287c09",
"class_ogre_1_1v1_1_1_numeric_animation_track.html#ad1f1a983f9e2dfddee802c47bb0b1ca9",
"class_ogre_1_1v1_1_1_old_node.html#acb46d4b0a597156d9ba5abc39d127792",
"class_ogre_1_1v1_1_1_old_skeleton_instance.html#a9dffcf1a2d754e63e98770e5cf2cd9ee",
"class_ogre_1_1v1_1_1_overlay.html#aed1464ac3de21ff20db8f9f1f8608dc7",
"class_ogre_1_1v1_1_1_overlay_element.html#a82ffa73dd4cd2aaaff532429e5e26e14",
"class_ogre_1_1v1_1_1_panel_overlay_element.html#a40f54cb11284c8e13a04511539a2f3ea",
"class_ogre_1_1v1_1_1_patch_mesh.html#a8e9e4d491069483506d008369076aef1",
"class_ogre_1_1v1_1_1_rectangle2_d.html#a65246f7bdb08b9b4fb169ff5d0235eaa",
"class_ogre_1_1v1_1_1_ribbon_trail.html#a01f48775e48b672d7796311f3ffe2930",
"class_ogre_1_1v1_1_1_simple_renderable.html#a27e11639e69472211e847cbe59389c02",
"class_ogre_1_1v1_1_1_simple_renderable.html#af85238872bb197dea36acf21a578ecec",
"class_ogre_1_1v1_1_1_static_geometry_1_1_geometry_bucket.html#a0f81bbb1fd282d641035a19a9432175f",
"class_ogre_1_1v1_1_1_static_geometry_1_1_optimised_sub_mesh_geometry.html#ab7d95354f1f6655034211af3984878b6",
"class_ogre_1_1v1_1_1_sub_entity.html#a1cc35cf8e670d08912fc52fb51330759",
"class_ogre_1_1v1_1_1_tag_point.html#a7b932b5d1e72cc70eda5309e0a93fe27",
"class_ogre_1_1v1_1_1_text_area_overlay_element_1_1_cmd_alignment.html#a90af55100e3482cbb2271eb78037db61",
"class_ogre_1_1v1_1_1_vertex_element.html#a988815e754f24add2e6529c744e5f4fb",
"class_ogre_1_1v1_1_1_wire_bounding_box.html#a9a72ade896561e23c32cf8659f1640e4",
"dir_83c3d6697dd0c53f763f0f7468a75e59.html",
"globals_defs_f.html",
"group___general.html#gga3970484e048d1352a7e59b61186b5f2aadae32e4e84712917731325c903fa14dd",
"group___general.html#gga828fd7e6454d2a188dc27c3df3627244a824a2c2d59619c736a3db29ed92d51a9",
"group___image.html#ga4239da7c9c8c6d4ce977663844dfcae1",
"group___image.html#gga7e0353e7d36d4c2e8468641b7303d39ca2542d39f6712cb6f69907330a31a9342",
"group___materials.html#gga9c5b2950be06ff56a6ee0bace240d447ae5d0f0b506f6443725dfa2ee729d2b8b",
"group___property.html#gga0883a5af9197151407965df0bacc4f3aaa5b410ea31e3954eeba233240dfb67ec",
"group___render_system.html#gga3d2965b7f378ebdcfe8a4a6cf74c3de7ab4f91cac8782a94b7628efde67bb3a89",
"group___scene.html#ga1d0db569747448aeff27f36ecd717686",
"namespace_ogre.html#a11c9152a33c78bdc13a9cfdc41833d35",
"namespace_ogre.html#aba516afc0c3b91e55136e823cb02fed0",
"namespace_ogre_1_1_scene_flags.html#af3190017d83d94455e645c506aa1681b",
"struct_ogre_1_1_aabb.html#ac6f3f88357056e62dbd427a9c486c76c",
"struct_ogre_1_1_cb_shader_buffer.html#af17e61e5739bfe1d4e35520a870f1003",
"struct_ogre_1_1_d3_d11_frame_buffer_desc_value.html#a92045338d83c538dcafda06bcf72240e",
"struct_ogre_1_1_entity_material_lod_changed_event.html",
"struct_ogre_1_1_g_l_e_s2_vertex_array_object.html#a18c774a4e47939b8d4ce980af254c40d",
"struct_ogre_1_1_hlms_base_prop.html#ad20efb9f96fcd8abff55af3369051f0d",
"struct_ogre_1_1_id_string.html#aa879e3cd371deb2979f1f5d4b8977d44",
"struct_ogre_1_1_lod_data_1_1_triangle.html#ab43218cda5bc70a6c6e5f084a1a5ec7b",
"struct_ogre_1_1_metal_hlms_pso.html#a0649b518d6c05f67c1732d3397d0118d",
"struct_ogre_1_1_pbs_property.html#a04fcad673e38920530d9300fe0b748f7",
"struct_ogre_1_1_r_t_shader_1_1_function_invocation_1_1_function_invocation_compare.html",
"struct_ogre_1_1_scene_query_result.html#acb46d4b0a597156d9ba5abc39d127792",
"struct_ogre_1_1_terrain_group_1_1_terrain_slot.html#aa0cb112bc24303af1881f7849b7c0585",
"struct_ogre_1_1_unlit_mobile_prop.html#a57de8ec760b9fa28206c2e9a1544b402",
"struct_ogre_1_1_vao_manager_1_1_memory_stats_entry.html#a5e9e1ef044989c8fc9fc88bd4cfbc303",
"struct_ogre_1_1_vector_set.html#adf2eb6c75c5a21936b9d658de1a3e132",
"struct_ogre_1_1_volume_1_1_dual_cell.html#a3da89c366371d8f8f24c95307a897caf",
"struct_ogre_1_1v1_1_1_cb_draw_call_strip.html",
"structbsp__face__t.html#ae18796a76c1cc01a88908a91ffbb18b1"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';