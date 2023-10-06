
Reference Guide: HLMS Terra Datablock {#hlmsterradatablockref}
=====================================

- Reference for a datablock for the Terra HLMS implementation
- A Datablock is a "material" from the user's perspective. It is the only mutable block. It holds data (i.e. material properties) that will be passed directly to the shaders, and also holds which Macroblock, Blendblocks and Samplerblocks are assigned to it.
- Datablocks are located in JSON material filename: `[name].material.json`
- When loading a texture, if no sampler is provided, a default one from the HLMS is supplied
- All parameters are optional. Defaults are provided.

@tableofcontents

# Common Datablock Parameters: {#dbtCommonParameters}

# Terra Datablock Parameters {#dbtTerraParameters}

## Parameter: brdf {#dbtParamBRDF}
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
- **DEFAULT="default"**

## Parameter: detail[X] {#dbtParamDetailDiffuse}
- Name of the detail map to be used
- Can contain up to 4 detail blocks (`detail0` to `detail3`)
- Block definition:
    - `diffuse_map`: Type string. The name of a diffuse map texture. Example: `"diffuse_map" : "texture.tga"` **OPTIONAL parameter**
    - `metalness` :  Type float. Sets the metalness value of the detail texture. **DEFAULT=1.0**
    - `metalness_map`: Type string. The name of a metalness map texture. Example: `"metalness_map" : "texture.tga"` **OPTIONAL parameter**
    - `normal_map`: Type string. The name of a normal map texture. Example: `"normal_map" : "texture.tga"` **OPTIONAL parameter**
    - `offset`: Type Vector2 array. Sets the UV offset of the detail map. **DEFAULT=[0,0]**
    - `scale`: Type Vector2 array. Sets the UV scale of the detail map. **DEFAULT=[1,1]**
    - `roughness` : Type float. Sets the roughness value of the detail texture. Very low roughness values can cause NaNs in the pixel shader. **DEFAULT=1.0**
    - `roughness_map`: Type string. The name of a roughness map texture. Example: `"roughness_map" : "texture.tga"` **OPTIONAL parameter**
- In this block you can specifiy a `texture` and `sampler` parameter OR a `diffuse_map` parameter. 
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: Type string. The name of a texture. Example: `"texture" : "texture.tga"` **OPTIONAL parameter** 

## Parameter: detail_weight {#dbtParamDetailWeight}
- Defines a material's detail map properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: Type string. The name of a texture. Example: `"texture" : "texture.tga"` **OPTIONAL parameter** 

## Parameter: diffuse {#dbtParamDiffuse}
- Defines a material's diffuse properties
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: Type string. The name of a texture. Example: `"texture" : "texture.tga"` **OPTIONAL parameter** 
    - `value`: Type vector3 array (RGB). Sets the diffuse colour (final multiplier). The colour will be divided by PI for energy conservation. **DEFAULT="[1, 1, 1]"**

## Parameter: reflection {#dbtParamReflection}
- Defines a material's reflection map. Must be a cubemap. Doesn't use an UV set because the texture coordinates are automatically calculated
- Block definition:
    - `sampler`: Type string. The name of the sampler block definition to apply to the diffuse texture. **OPTIONAL parameter**
    - `texture`: Type string. The name of a texture. Example: `"texture" : "texture.tga"` **OPTIONAL parameter** 

# Example Terra Datablock Definition: {#dbtExample}

```json
{ 
    "Terra" : 
    {
        "[Material definition name]" :
        {
            "brdf" : "default",
            "detail0" :
            {
                "offset" : [0, 0],
                "scale" : [256, 256],
				"roughness" : 1,
				"metalness" : 1,
                "diffuse_map" : "texture.png",
				"roughness_map" : "texture.png",
				"metalness_map" : "texture.png",
                "sampler" : "default_sampler",
                "texture" : "texture.png"
            },
            "detail_weight" :
            {
                "sampler" : "default_sampler",
                "texture" : "detail_weight_texture.tga",
            },
            "diffuse" :
            {
                "sampler" : "default_sampler",
                "texture" : "diffuse_texture.tga",
                "value" : [1, 1, 1]
            },
            "reflection" :
            {
                "sampler" : "default_sampler",
                "texture" : "reflective_texture.tga",
            }
        }
    }
}
```

## Links {#dbtLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)

