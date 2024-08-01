
Reference Guide: HLMS Blendblock {#hlmsblendblockref}
================================

- Blendblocks are like Macroblocks, but they hold alpha blending operation information (blend factors: One, One_Minus_Src_Alpha; blending modes: add, substract, min, max. etc). They're analogous to D3D11_BLEND_DESC. We also sort by blendblocks to reduce state changes.
- Blendblocks are located in JSON material filename: `[name].material.json`
- A blend block contains settings that rarely change, and thus are common to many materials. The reasons this structure isn't joined with HlmsMacroblock is that: The D3D11 API makes this distinction (much higher API overhead if we change i.e. depth settings) due to D3D11_RASTERIZER_DESC.
- This block contains information of whether the material is transparent. Transparent materials are sorted differently than opaque ones.
- Up to 32 different blend blocks are allowed.
- All parameters are optional. Defaults are provided.

@tableofcontents

# Parameters: {#bbParameters}

## Parameter: alpha_to_coverage {#bbParamAlphaToCoverage}
- Alpha to coverage performs multisampling on the edges of alpha-rejected textures to produce a smoother result. It is only supported when multisampling is already enabled on the render target, and when the hardware supports alpha to coverage. The common use for alpha to coverage is foliage rendering and chain-link fence style textures.
- Value is true, false or "msaa_only". The latter means it is only enabled if the render target is MSAA 2x or higher.
- **DEFAULT=false**

## Parameter: blendmask {#bbParamBlendmask}
- Masks which colour channels will be writing to. Value is a string up to four characters long and will contain the characters `r`, `g`, `b` and `a` corresponding to colour channels. For some advanced effects, you may wish to turn off the writing of certain colour channels, or even all of the colour channels so that only the depth buffer is updated in a rendering pass (if depth writes are on; may be you want to only update the stencil buffer)
- **DEFAULT="rgba"**

## Parameter: separate_blend {#bbParamSeperateBlend}
- Used to determine if separate alpha blending should be used for colour and alpha channels
- Value is true or false
- **DEFAULT=false**

## Parameter: src_blend_factor {#bbParamSorceBlendFactor}
- Source blending factor for blending objects with the scene
- Value is a string. Valid values:
    - `one`: Constant value of 1.0
    - `zero`: Constant value of 0.0
    - `dst_colour`: The existing pixel colour 
    - `src_colour`: The texture pixel (texel) colour
    - `one_minus_dst_colour`: 1 - SBF_DEST_COLOUR
    - `one_minus_src_colour`: 1 - SBF_SOURCE_COLOUR 
    - `dst_alpha`: The existing pixel alpha value
    - `src_alpha`: The texel alpha value
    - `one_minus_dst_alpha`: 1 - SBF_DEST_ALPHA
    - `one_minus_src_alpha`: 1 - SBF_SOURCE_ALPHA 
- **DEFAULT="one"**

## Parameter: dst_blend_factor {#bbParamDestBlendFactor}
- Destination blending factor for blending objects with the scene
- Value is a string. Valid values:
    - `one`, `zero`, `dst_colour`, `src_colour`, `one_minus_dst_colour`, `one_minus_src_colour`, `dst_alpha`, `src_alpha`, `one_minus_dst_alpha` and `one_minus_src_alpha`
    - See "src_blend_factor" for full description.
- **DEFAULT="zero"**

## Parameter: src_alpha_blend_factor {#bbParamSourceAlphaBlendFactor}
- Source blending factors for blending objects with the scene
- Value is a string. Valid values:
    - `one`, `zero`, `dst_colour`, `src_colour`, `one_minus_dst_colour`, `one_minus_src_colour`, `dst_alpha`, `src_alpha`, `one_minus_dst_alpha` and `one_minus_src_alpha`
    - See "src_blend_factor" for full description.
- **DEFAULT="one"**

## Parameter: dst_alpha_blend_factor {#bbParamDestinationAlphaBlendFactor}
- Destination blending factor for blending objects with the scene
- Value is a string. Valid values:
    - `one`, `zero`, `dst_colour`, `src_colour`, `one_minus_dst_colour`, `one_minus_src_colour`, `dst_alpha`, `src_alpha`, `one_minus_dst_alpha` and `one_minus_src_alpha`
    - See "src_blend_factor" for full description.
- **DEFAULT="zero"**

## Parameter: blend_operation {#bbParamBlendOperation}
- Sets the operation which is applied between the two components of the scene blending equation
- Value is a string. Valid values:
    - `add`, `subtract`, `reverse_subtract`, `min` and `max`
- **DEFAULT="add"**

## Parameter: blend_operation_alpha {#bbParamBlendOperationAlpha}
- Sets the operation which is applied between the two components of the scene blending equation except this sets the alpha separately.
- Value is a string. Valid values:
    - `add`, `subtract`, `reverse_subtract`, `min` and `max`
- **DEFAULT="add"**

# Example Blendblock Definition: {#bbExample}
```
{
    "blendblocks" :
    {
        "[blendblock definition name]" :
        {
            "alpha_to_coverage" : false,
            "blendmask" : "rgba",
            "separate_blend" : false,
            "src_blend_factor" : "one",
            "dst_blend_factor" : "zero",
            "src_alpha_blend_factor" : "one",
            "dst_alpha_blend_factor" : "zero",
            "blend_operation" : "add",
            "blend_operation_alpha" : "add"
        }
    }
}
```
# Scene blend type settings: {#bbBlendtypes}
These are some settings for common blending types.

## Type: Transparent Alpha {#btTransparentAlpha}
- Make the object transparent based on the final alpha values in the texture. Settings:
  - "src_blend_factor" : "src_alpha"
  - "dst_blend_factor" : "one_minus_src_alpha"
  - "src_alpha_blend_factor" : "src_alpha"
  - "dst_alpha_blend_factor" : "one_minus_src_alpha"

## Type: Transparent Colour {#btTransparentColour}
- Make the object transparent based on the colour values in the texture (brighter = more opaque) Settings:
  - "src_blend_factor" : "src_colour"
  - "dst_blend_factor" : "one_minus_src_colour"
  - "src_alpha_blend_factor" : "src_colour"
  - "dst_alpha_blend_factor" : "one_minus_src_colour"

## Type: Add {#btAdd}
- Add the texture values to the existing scene content. Settings:
  - "src_blend_factor" : "one"
  - "dst_blend_factor" : "one"
  - "src_alpha_blend_factor" : "one"
  - "dst_alpha_blend_factor" : "one"

## Type: Modulate {#btModulate}
- Multiply the 2 colours together. Settings:
  - "src_blend_factor" : "dst_colour"
  - "dst_blend_factor" : "zero"
  - "src_alpha_blend_factor" : "dst_colour"
  - "dst_alpha_blend_factor" : "zero"

## Type: Replace {#btReplace}
- The default blend mode where source replaces destination. Settings:
  - "src_blend_factor" : "one"
  - "dst_blend_factor" : "zero"
  - "src_alpha_blend_factor" : "one"
  - "dst_alpha_blend_factor" : "zero"

# Links {#bbLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)
