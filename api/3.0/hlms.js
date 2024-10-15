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
    ] ],
    [ "Precision / Quality", "hlms.html#autotoc_md101", null ],
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
    [ "Reference Guide: HLMS PBS Datablock", "hlmspbsdatablockref.html", [
      [ "Common Datablock Parameters:", "hlmspbsdatablockref.html#dbCommonParameters", [
        [ "Parameter: alpha_test", "hlmspbsdatablockref.html#dbParamAlphaTest", null ],
        [ "Parameter: blendblock", "hlmspbsdatablockref.html#dbParamBlendBlock", null ],
        [ "Parameter: macroblock", "hlmspbsdatablockref.html#dbParamMacroBlock", null ],
        [ "Parameter: shadow_const_bias", "hlmspbsdatablockref.html#dbParamShadowConstBias", null ]
      ] ],
      [ "PBS Datablock Parameters", "hlmspbsdatablockref.html#dbPBSParameters", [
        [ "Parameter: brdf", "hlmspbsdatablockref.html#dbParamBRDF", null ],
        [ "Parameter: refraction_strength", "hlmspbsdatablockref.html#autotoc_md27", null ],
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
      [ "Example PBS Datablock Definition:", "hlmspbsdatablockref.html#dbExample", null ]
    ] ],
    [ "Reference Guide: HLMS Unlit Datablock", "hlmsunlitdatablockref.html", [
      [ "Common Datablock Parameters:", "hlmsunlitdatablockref.html#dbulCommonParameters", null ],
      [ "Unlit Datablock Parameters", "hlmsunlitdatablockref.html#dbulUnlitParameters", [
        [ "Parameter: diffuse", "hlmsunlitdatablockref.html#dbulParamDiffuse", null ],
        [ "Parameter: diffuse_map[X]", "hlmsunlitdatablockref.html#dbulParamDetailNormal", null ]
      ] ],
      [ "Example Unlit Datablock Definition:", "hlmsunlitdatablockref.html#dbulExample", null ]
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
      [ "Example Terra Datablock Definition:", "hlmsterradatablockref.html#dbtExample", null ]
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
    ] ]
];