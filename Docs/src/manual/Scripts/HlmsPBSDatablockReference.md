
Reference Guide: HLMS PBS Datablock {#hlmspbsdatablockref}
===============================

- Reference for a datablock for the PBS HLMS implementation
- A Datablock is a "material" from the user's perspective. It is the only mutable block. It holds data (i.e. material properties) that will be passed directly to the shaders, and also holds which Macroblock, Blendblocks and Samplerblocks are assigned to it.
- Datablocks are located in JSON material filename: `[name].material.json`
- When loading a texture, if no sampler is provided, a default one from the HLMS is supplied
- All parameters are optional. Defaults are provided.

@tableofcontents

# Common Datablock Parameters: {#dbCommonParameters}

## Parameter: accurate_non_uniform_normal_scaling {#dbParamAccurateNonUniformNormalScaling}
- See Ogre::HlmsDatablock::setAccurateNonUniformNormalScaling
- Value of type bool
- **DEFAULT=false** 

## Parameter: alpha_test {#dbParamAlphaTest}
- Sets the alpha test to the given compare function
- Alpha_test value is type array:
    - First value is a string and sets the compare function.
        - Valid values for the compare function: `less`, `less_equal`, `equal`, `not_equal`, `greater_equal`, `greater`, `never`, `always` and `disabled`
    - The second is a float and sets the test's threshold. Value typically in the range [0; 1) 
    - Optional third is a bool that sets weather to alpha test shadow caster only
- **DEFAULT=["disabled",0.5,false]** 

## Parameter: blendblock {#dbParamBlendBlock}
- Sets the blendblock definition
- Two value variations:
    - Type 1: Value is type string, the name of a blendblock definition. Example: `"blendblock" : "blendblock_name"`
    - Type 2: Value is type string array:
        - First array value is the name of a blendblock definition
        - Second array value is the name of a blendblock definition for shadows
        - Example: `"blendblock" : ["blendblock_name","blendblock_name_for_shadows"]`

## Parameter: macroblock {#dbParamMacroBlock}
- Sets the macroblock definition
- Two value variations:
    - Type 1: Value is type string, the name of a macroblock definition. Example: `"macroblock" : "macroblock_name"`
    - Type 2: Value is type string array:
        - First array value is the name of a macroblock definition
        - Second array value is the name of a macroblock definition for shadows
        - Example: `"macroblock" : ["macroblock_name","macroblock_name_for_shadows"]`

## Parameter: shadow_const_bias {#dbParamShadowConstBias}
- Sets the shadow_const_bias value
- Value of type float
- **DEFAULT=0.01**

# PBS Datablock Parameters {#dbPBSParameters}

## Parameter: brdf {#dbParamBRDF}
- Defines the brdf mode
- Valid values:
    - `default`: Most physically accurate BRDF Ogre has. 
        - Uses: Good for representing the majority of materials.
    - `cook_torrance`: Cook-Torrance implementation of BRDF.
        - Uses: Ideal for silk (use high roughness values) and synthetic fabrics.
    - `blinn_phong`: Implements Normalized Blinn Phong using a normalization factor of (n + 8) / (8 * pi). Looks similar to `default` but has increased performance.
    - `default_uncorrelated`: Same as Default, but the geometry term is not height-correlated which most notably causes edges to be dimmer and is less correct.
        - Uses: Closest resemblance to Unity if an exchangeable pipeline is advantageous.
    - `default_separate_diffuse_fresnel`: Same as Default but the fresnel of the diffuse is calculated differently. When using this BRDF, the diffuse fresnel will be calculated differently, causing the diffuse component to still affect the colour even when the fresnel = 1 (although subtly). To achieve a perfect mirror you will have to set the fresnel to 1 and the diffuse colour to black; which can be unintuitive for artists.
        - Uses: Materials with complex refractions and reflections like glass, transparent plastics, fur, and surface with refractions and multiple rescattering that cannot be represented well with the default BRDF. 
    - `cook_torrance_separate_diffuse_fresnel`: Similar to 'DefaultSeparateDiffuseFresnel' but the Cook Torrance model is used instead.
        - Uses: Shiny objects like glass, toy marbles, some types of rubber, silk and synthetic fabric.
    - `blinn_phong_separate_diffuse_fresnel`: Similar to 'DefaultSeparateDiffuseFresnel' but uses BlinnPhong as base. 
    - `blinn_phong_legacy_math`: Implements traditional / the original non-PBR blinn phong. Notes:
        - Results in graphical look of 2000-2005 game
        - Ignores fresnel completely
        - Works with Roughness in range (0, 1] (converted to shininess)
        - Assumes your Light power is set to PI (or a multiple)
        - Diffuse & Specular will automatically be multiplied/divided by PI
        - One of the fastest BRDF implementations
    - `blinn_phong_full_legacy`: Implements traditional / the original non-PBR blinn phong. Notes:
        - Results in graphical look of 2000-2005 game
        - Ignores fresnel completely
        - Roughness is actually the shininess parameter in range (0, inf) although most used ranges are in (0, 500]
            - Assumes your Light power is set to 1.0.
            - Diffuse & Specular is unmodified
            - The fastest BRDF implementations
            - Use this implementation for ease of porting from Ogre 1.X and to maintain the fixed-function look. 
            - If switching from the 'default' BRDF implementation, the scene will be too bright. If so, divide light power by PI.
- **DEFAULT="default"**

## Parameter: refraction_strength

- Value of type float
- See Ogre::HlmsPbsDatablock::setRefractionStrength
- **DEFAULT=0.075**

## Parameter: detail_diffuse[X] {#dbParamDetailDiffuse}
- Name of the detail map to be used on top of the diffuse colour
- Can contain up to 4 detail_diffuse blocks (`detail_diffuse0` to `detail_diffuse3`)
- There can be gaps (i.e. set detail maps 0 and 2 but not 1)
- Block definition:
    - `mode`: Type string. Blend mode to use for each detail map. This parameter only affects the diffuse detail map. **DEFAULT=`NormalNonPremul`.** Valid values:
        - `NormalNonPremul`: Regular alpha blending. 
        - `NormalPremul`: Premultiplied alpha blending. 
        - `Add`: 
        - `Subtract`: 
        - `Multiply`: 
        - `Multiply2x`: 
        - `Screen`: 
        - `Overlay`: 
        - `Lighten`: 
        - `Darken`: 
        - `GrainExtract`: 
        - `GrainMerge`: 
        - `Difference`: 
    - `offset`: Type Vector2 array. Sets the UV offset of the detail map. **DEFAULT=[0,0]**
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `scale`: Type Vector2 array. Sets the UV scale of the detail map. **DEFAULT=[1,1]**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" :  ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type float. The weight for the detail map. Usual values are in range [0, 1] but any value is accepted and valid. **DEFAULT=1**
  
## Parameter: detail_normal[X] {#dbParamDetailNormal}
- Name of the detail map's normal map
- It's not affected by blend mode. Can contain up to 4 detail_normal blocks (`detail_normal0` to `detail_normal3`)
- May be used even if there is no detail_diffuse map
- Block definition:
    - `offset`: Type Vector2 array. Sets the UV offset of the detail normal map. **DEFAULT=[0,0]**
    - `sampler`: Type string. The name of the sampler block definition to apply to the normal map texture. **OPTIONAL parameter**
    - `scale`: Type Vector2 array. Sets the UV scale of the detail normal map. **DEFAULT=[1,1]**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type float. Sets the normal mapping weight. Usual values are in range [0, 1] but any value is accepted and valid. Value of 0 means no effect. **DEFAULT=1** 

## Parameter: detail_weight {#dbParamDetailWeight}
- Defines a material's detail map properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: Texture that when present, will be used as mask/weight for the 4 detail maps. The R channel is used for detail map #0; the G for detail map #1, B for #2, and Alpha for #3. This affects both the diffuse and normal-mapped detail maps. **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" :  ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**

## Parameter: diffuse {#dbParamDiffuse}
- Defines a material's diffuse properties
- Block definition:
    - `background`: Type vector4 array (RGBA). Sets the diffuse background colour. When no diffuse texture is present, this solid colour replaces it, and can act as a background for the detail maps. Does not replace `value`. **DEFAULT="[1, 1, 1, 1]"**
    - `grayscale`: Value is either true or false. When true, it treats the diffuse map as a grayscale map which means it will spread red component to all RGB channels. With this option you can use PFG_R8_UNORM for diffuse map in the same way as old PF_L8 format. **DEFAULT=false**      
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type vector3 array (RGB). Sets the diffuse colour (final multiplier). The colour will be divided by PI for energy conservation. **DEFAULT="[1, 1, 1]"**

## Parameter: emissive {#dbParamEmissive}
- Defines a material's emissive properties
- Block definition:
    - `lightmap`: Value is either true or false. When set, it treats the emissive map as a lightmap; which means it will be multiplied against the diffuse component. **DEFAULT=false**  
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`    
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type vector3 array (RGB). Sets the emissive colour. Emissive colour has no physical basis. Though in HDR, if you're working in lumens, this value should probably be in lumens too. To disable emissive, setEmissive( Vector3::ZERO ) and unset any texture in PBSM_EMISSIVE slot.  **DEFAULT="[0, 0, 0]"**

## Parameter: fresnel {#dbParamFresnel}
- Defines a material's fresnel properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `mode`: Type string. Valid values:
        - `coeff`: Sets the fresnel (F0) directly, instead of using the IOR.
        - `ior`: Calculates fresnel (F0 in most books) based on the IOR. The formula used is ( (1 - idx) / (1 + idx) )². Can't be using metallic workflow.
        - `coloured`: Sets the fresnel (F0) directly, instead of using the IOR. Fresnel value uses RGB values from `value` parameter.
        - `coloured_ior`: Calculates fresnel (F0 in most books) based on the IOR. The formula used is ( (1 - idx) / (1 + idx) )². Fresnel value uses RGB values from `value` parameter. Can't be using metallic workflow.
        - **DEFAULT="coeff"**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Two value variations:
        - Type 1: Value is float. **DEFAULT=0.818**
        - Type 2: Type vector3 array. Sets individual fresnel values for RGB channels. If the mode doesn't need individual channels, the y and z values are discarded. **DEFAULT=[0.818,0.818,0.818]**

## Parameter: metalness {#dbParamMetalness}
- Defines a material's metalness properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Note: Only the Red channel will be used, and the texture will be converted to an efficient monochrome representation. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type float, Range [0, 1]. Sets the metalness in a metallic workflow. Overrides any fresnel values. Should be in "metallic" workflow. **DEFAULT=0.818**

## Parameter: normal {#dbParamNormal}
- Defines a material's normal map properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Note: Only the Red channel will be used, and the texture will be converted to an efficient monochrome representation. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type float, Range is normally [0, 1] but doesn't need to be. Sets the normal map weight. **DEFAULT=1**

## Parameter: receive_shadows {#dbParamReceiveShadows}
- Value of true or false. When false, no shadows will be cast on this material. **DEFAULT=true**

## Parameter: reflection {#dbParamReflection}
- Defines a material's reflection map. Must be a cubemap. Doesn't use an UV set because the texture coordinates are automatically calculated
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`   
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**

## Parameter: roughness {#dbParamRoughness}
- Defines a material's roughness properties. Only used by certain BRDF implementations
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Note: Only the Red channel will be used, and the texture will be converted to an efficient monochrome representation. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type float, Range (0, inf). Sets the roughness of a material.  Note: Values extremely close to zero could cause NaNs and INFs in the pixel shader, also depends on the GPU's precision. **DEFAULT=1**

## Parameter: specular {#dbParamSpecular}
- Defines a material's specular properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter.**
    - `texture`: **OPTIONAL parameter**. Two value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**
    - `value`: Type vector3 array (RGB). Sets the specular colour of the material. **DEFAULT="[1, 1, 1]"**

## Parameter: transparency {#dbParamTransparency}
- Defines the transparency of the material
- Block definition:
    - `value`: Value range [0, 1]. 0 = fully transparent and 1 = fully opaque. **DEFAULT=1**
    - `mode`: Transparency mode. **DEFAULT="None"** Valid values:  
        - `None`: No alpha blending. 
        - `Fade`: Normal alpha blending. Fade object out until it disappears.
        - `Refractive `: Similar to transparent, but also performs refractions. Note: The compositor scene pass must be set to render refractive objects in its own pass.
        - `Transparent`: Realistic transparency that preserves lighting reflections. Good for glass, transparent plastic. Note: When transparency = 0, the object may not be fully invisible.
    - `use_alpha_from_textures`: When false, the alpha channel of the diffuse and detail maps will be ignored. GPU optimisation. Value can be true or false. **DEFAULT=true**

## Parameter: two_sided {#dbParamTwoSided}
- Enables support for two sided lighting. Value can be true or false. False is faster
- **DEFAULT=false**
- Note: There are two extra parameters that can be modified programmatically:
    - `changeMacroblock` type [bool]: Whether to change the current macroblock for one that has cullingMode = CULL_NONE or set it to false to leave the current macroblock as is. Set to true in the json interpreter.
    - `oneSidedShadowCast` type [CullingMode]: 	If changeMacroblock == true; this parameter controls the culling mode of the shadow caster (the setting of HlmsManager::setShadowMappingUseBackFaces is ignored!). While oneSidedShadowCast == CULL_NONE is usually the "correct" option, setting oneSidedShadowCast=CULL_ANTICLOCKWISE can prevent ugly self-shadowing on interiors. In the json interpreter, this value is set to the CullingMode of the Macroblock.


## Parameter: workflow {#dbParamWorkflow}
- Defines the workflow of the material
- Valid values:
    - `specular_ogre`: Ogre implementation of specular workflow. Specular texture is used as a specular texture, and is expected to be either coloured or monochrome. setMetalness should not be called in this mode. 
    - `specular_fresnel`: Equivalent to what other implementations often refer to as "specular workflow". Specular texture is used as a specular texture, and is expected to be either coloured or monochrome. setMetalness should not be called in this mode.
    - `metallic`: Metallic workflow. The texture is used as a metallic texture and expected to be monochrome. Fresnel setting should not be used.
- **DEFAULT="specular_ogre"**

# Example PBS Datablock Definition: {#dbExample}

```json
{ 
    "pbs" : 
    {
        "[Material definition name]" :
        {
            "alpha_test" : ["disabled",0.5,false],
            "blendblock" : "blendblock_name",
            "blendblock" : ["blendblock_name", "blendblock_name_for_shadows"],
            "macroblock" : "macroblock_name",
            "macroblock" : ["macroblock_name", "macroblock_name_for_shadows"],
            "shadow_const_bias" : 0.01,
            
            "refraction_strength" : 0.2,
      
            "brdf" : "default",
            "detail_diffuse0" :
            {
                "mode" : "NormalNonPremul",
                "offset" : [0,0],
                "sampler" : "default_sampler",
                "scale" : [1,1],
                "texture" : "diffuse_texture.tga",
                "texture" : ["diffuse_texture.tga","Diffuse_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 1
            },
            "detail_normal0" :
            {
                "offset" : [0,0],
                "sampler" : "default_sampler",
                "scale" : [1,1],
                "texture" : "diffuse_texture.tga",
                "texture" : ["diffuse_texture.tga","Diffuse_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 1
            },
            "detail_weight" :
            {
                "sampler" : "default_sampler",
                "texture" : "detail_weight_texture.tga",
                "texture" : ["detail_weight_texture.tga","DetailWeight_Texture_Alias_Name"],
                "uv" : 0
            },
            "diffuse" :
            {
                "background" : [1, 1, 1, 1],
                "grayscale" : false,
                "sampler" : "default_sampler",
                "texture" : "diffuse_texture.tga",
                "texture" : ["diffuse_texture.tga","Diffuse_Texture_Alias_Name"],
                "uv" : 0,
                "value" : [1, 1, 1]
            },
            "emissive" :
            {
                "lightmap" : false,
                "sampler" : "default_sampler",
                "texture" : "emissive_texture.tga",
                "texture" : ["emissive_texture.tga","Emissive_Texture_Alias_Name"],
                "uv" : 0,
                "value" : [0, 0, 0]
            },
            "fresnel" :
            {
                "mode" : "coeff",
                "sampler" : "default_sampler",
                "texture" : "diffuse_texture.tga",
                "texture" : ["diffuse_texture.tga","Diffuse_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 0.818,
                "value" : [0.818,0.818,0.818]
            },
            "metalness" :
            {
                "sampler" : "default_sampler",
                "texture" : "metal_texture.tga",
                "texture" : ["metal_texture.tga","Metal_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 0.818
            },
            "normal" :
            {
                "sampler" : "default_sampler",
                "texture" : "normalmap_texture.tga",
                "texture" : ["normalmap_texture.tga","NormalMap_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 1
            },
            "receive_shadows" : true,
            "reflection" :
            {
                "sampler" : "default_sampler",
                "texture" : "reflective_texture.tga",
                "texture" : ["reflective_texture.tga","Reflective_Texture_Alias_Name"],
                "uv" : 0
            },
            "roughness" : 
            {
                "sampler" : "default_sampler",
                "texture" : "rough_texture.tga",
                "texture" : ["rough_texture.tga","Roughness_Texture_Alias_Name"],
                "uv" : 0,
                "value" : 1
            },
            "specular" :
            {
                "sampler" : "default_sampler",
                "texture" : "specular_texture.tga",
                "texture" : ["specular_texture.tga","Specular_Texture_Alias_Name"],
                "uv" : 0,
                "value" : [1, 1, 1]
            },
            "transparency" :
            {
                "value" : 1,
                "mode" : "Transparent",        
                "use_alpha_from_textures" : true
            },
            "two_sided" : false,
            "workflow" : "metallic"
        }
    }
}
```

## Links {#dbLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)

