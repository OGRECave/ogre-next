var manual =
[
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
      ] ],
      [ "Advanced MSAA", "compositor.html#autotoc_md9", [
        [ "What is MSAA?", "compositor.html#autotoc_md10", [
          [ "Supersampling Antialiasing (SSAA) vs MSAA", "compositor.html#autotoc_md11", null ],
          [ "MSAA approach to the problem", "compositor.html#autotoc_md12", null ]
        ] ],
        [ "Ogre + MSAA with Implicit Resolves", "compositor.html#autotoc_md13", null ],
        [ "Ogre + MSAA with Explicit Resolves", "compositor.html#autotoc_md14", null ]
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
          [ "Pros", "_gi_methods.html#autotoc_md15", null ],
          [ "Cons", "_gi_methods.html#autotoc_md16", null ],
          [ "Pros", "_gi_methods.html#autotoc_md17", null ],
          [ "Cons", "_gi_methods.html#autotoc_md18", null ],
          [ "Pros", "_gi_methods.html#autotoc_md19", null ],
          [ "Cons", "_gi_methods.html#autotoc_md20", null ],
          [ "Pros", "_gi_methods.html#autotoc_md21", null ],
          [ "Cons", "_gi_methods.html#autotoc_md22", null ],
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
      [ "Load Store semantics", "_ogre22_changes.html#autotoc_md23", [
        [ "Now that we’ve explained how TBDRs work, we can explain load and store actions", "_ogre22_changes.html#autotoc_md24", null ]
      ] ],
      [ "More control over MSAA", "_ogre22_changes.html#autotoc_md25", null ],
      [ "Porting to Ogre 2.2 from 2.1", "_ogre22_changes.html#autotoc_md26", [
        [ "PixelFormats", "_ogre22_changes.html#autotoc_md27", [
          [ "Common pixel format equivalencies", "_ogre22_changes.html#autotoc_md28", null ]
        ] ],
        [ "Useful code snippets", "_ogre22_changes.html#autotoc_md29", [
          [ "Create a TextureGpu based on a file", "_ogre22_changes.html#autotoc_md30", null ],
          [ "Create a TextureGpu based that you manually fill", "_ogre22_changes.html#autotoc_md31", null ],
          [ "Uploading data to a TextureGpu", "_ogre22_changes.html#autotoc_md32", null ],
          [ "Upload streaming", "_ogre22_changes.html#autotoc_md33", null ],
          [ "Downloading data from TextureGpu into CPU", "_ogre22_changes.html#autotoc_md34", null ],
          [ "Downloading streaming", "_ogre22_changes.html#autotoc_md35", null ]
        ] ]
      ] ],
      [ "Difference between depth, numSlices and depthOrSlices", "_ogre22_changes.html#autotoc_md36", null ],
      [ "Memory layout of textures and images", "_ogre22_changes.html#autotoc_md37", null ],
      [ "Troubleshooting errors", "_ogre22_changes.html#autotoc_md38", null ],
      [ "RenderPassDescriptors", "_ogre22_changes.html#autotoc_md39", null ],
      [ "DescriptorSetTexture & co.", "_ogre22_changes.html#autotoc_md40", null ],
      [ "Does 2.2 interoperate well with the HLMS texture arrays?", "_ogre22_changes.html#autotoc_md41", null ],
      [ "Hlms porting", "_ogre22_changes.html#autotoc_md42", null ],
      [ "Things to watch out when porting", "_ogre22_changes.html#autotoc_md43", null ]
    ] ],
    [ "Behavor of StagingTexture in D3D11", "_behavor_staging_texture_d3_d11.html", [
      [ "Attempting to be contiguous", "_behavor_staging_texture_d3_d11.html#autotoc_md6", null ],
      [ "Slicing in 3", "_behavor_staging_texture_d3_d11.html#autotoc_md7", null ],
      [ "Slicing in the middle", "_behavor_staging_texture_d3_d11.html#autotoc_md8", null ]
    ] ]
];