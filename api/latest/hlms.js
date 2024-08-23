var hlms =
[
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
      [ "Multithreaded Shader Compilation", "hlms.html#autotoc_md108", null ]
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
    [ "Precision / Quality", "hlms.html#autotoc_md109", null ],
    [ "Multithreaded Shader Compilation", "_hlms_threading.html", [
      [ "CMake Options", "_hlms_threading.html#HlmsThreading_CMakeOptions", null ],
      [ "The tid (Thread ID) argument", "_hlms_threading.html#HlmsThreading_tidArgument", [
        [ "API when OGRE_SHADER_COMPILATION_THREADING_MODE = 1", "_hlms_threading.html#autotoc_md97", null ]
      ] ],
      [ "How does threaded Hlms work?", "_hlms_threading.html#autotoc_md98", [
        [ "What is the range of tid argument?", "_hlms_threading.html#autotoc_md99", null ]
      ] ]
    ] ]
];