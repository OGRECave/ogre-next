var _scripts =
[
    [ "Loading scripts", "_scripts.html#autotoc_md118", null ],
    [ "Format", "_scripts.html#Format", [
      [ "Script Inheritance", "_scripts.html#Script-Inheritance", [
        [ "Advanced Script Inheritance", "_scripts.html#Advanced-Script-Inheritance", null ]
      ] ],
      [ "Script Variables", "_scripts.html#Script-Variables", null ],
      [ "Script Import Directive", "_scripts.html#Script-Import-Directive", null ]
    ] ],
    [ "Custom Translators", "_scripts.html#custom-translators", null ],
    [ "JSON Materials", "_j_s_o_n-_materials.html", "_j_s_o_n-_materials" ],
    [ "Compositor", "compositor.html", [
      [ "Nodes", "compositor.html#CompositorNodes", [
        [ "Input & output channels and RTTs", "compositor.html#CompositorNodesChannelsAndRTTs", [
          [ "Locally declared textures", "compositor.html#CompositorNodesChannelsAndRTTsLocalTextures", null ],
          [ "Textures coming from input channels", "compositor.html#CompositorNodesChannelsAndRTTsFromInputChannel", null ],
          [ "Global Textures", "compositor.html#CompositorNodesChannelsAndRTTsGlobal", null ],
          [ "compositor_node parameters", "compositor.html#autotoc_md111", [
            [ "in", "compositor.html#CompositorNode_in", null ],
            [ "out", "compositor.html#CompositorNode_out", null ],
            [ "in_buffer", "compositor.html#CompositorNode_in_buffer", null ],
            [ "out_buffer", "compositor.html#CompositorNode_out_buffer", null ],
            [ "custom_id", "compositor.html#CompositorNode_custom_id", null ]
          ] ],
          [ "Main RenderTarget", "compositor.html#CompositorNodesChannelsAndRTTsMainRenderTarget", null ]
        ] ],
        [ "Target", "compositor.html#CompositorNodesTarget", [
          [ "target parameters", "compositor.html#autotoc_md112", [
            [ "target_level_barrier", "compositor.html#CompositorTarget_target_level_barrier", null ]
          ] ]
        ] ],
        [ "Passes", "compositor.html#CompositorNodesPasses", [
          [ "pass parameters", "compositor.html#autotoc_md113", [
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
        [ "What is MSAA?", "compositor.html#autotoc_md114", [
          [ "Supersampling Antialiasing (SSAA) vs MSAA", "compositor.html#autotoc_md115", null ],
          [ "MSAA approach to the problem", "compositor.html#autotoc_md116", [
            [ "technique", "compositor.html#CompositorShadowNodesSetup_technique", null ],
            [ "num_splits", "compositor.html#CompositorShadowNodesSetup_num_splits", null ],
            [ "num_stable_splits", "compositor.html#CompositorShadowNodesSetup_num_stable_splits", null ],
            [ "normal_offset_bias", "compositor.html#CompositorShadowNodesSetup_normal_offset_bias", null ],
            [ "constant_bias_scale", "compositor.html#CompositorShadowNodesSetup_constant_bias_scale", null ],
            [ "pssm_lambda", "compositor.html#CompositorShadowNodesSetup_pssm_lambda", null ],
            [ "pssm_split_blend", "compositor.html#CompositorShadowNodesSetup_pssm_split_blend", null ],
            [ "pssm_split_fade", "compositor.html#CompositorShadowNodesSetup_pssm_split_fade", null ],
            [ "shadow_map", "compositor.html#CompositorShadowNodesSetup_shadow_map", null ],
            [ "connect", "compositor.html#CompositorWorkspaces_connect", null ],
            [ "connect_external", "compositor.html#CompositorWorkspaces_connect_external", null ],
            [ "alias", "compositor.html#CompositorWorkspaces_alias", null ],
            [ "buffer", "compositor.html#CompositorWorkspaces_buffer", null ],
            [ "connect_buffer", "compositor.html#CompositorWorkspaces_connect_buffer", null ],
            [ "connect_buffer_external", "compositor.html#CompositorWorkspaces_connect_buffer_external", null ],
            [ "Resources", "compositor.html#CompositorNodesTexturesMsaaResources", null ]
          ] ]
        ] ],
        [ "Ogre + MSAA with Implicit Resolves", "compositor.html#autotoc_md117", null ],
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
        [ "Common Emitter Attributes", "_particle-_scripts.html#autotoc_md110", null ],
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
];