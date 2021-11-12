
Reference Guide: HLMS Samplerblock {#hlmssamplerref}
==================================

- Samplerblocks hold information about texture units, like filtering options, addressing modes (wrap, clamp, etc), Lod bias, anisotropy, border colour, etc. They're analogous to D3D11_SAMPLER_DESC.
- Samplers are located in JSON material filename: `[name].material.json`
- A sampler block contains settings that go hand in hand with a texture, and thus are common to many textures. This is very analogous to D3D11_SAMPLER_DESC.
- Up to 32 different sampler blocks are allowed.
- All parameters are optional. Defaults are provided.

@tableofcontents

# Parameters: {#sbParameters}

## Parameter: min {#sbParamMin}
- The texture filtering used when shrinking a texture.
- Value of type string. Valid values are:
    - `none`: No filtering
    - `point`: Use the closest pixel
    - `linear`: Average of a 2x2 pixel area, denotes bilinear for MIN.
    - `anisotropic`: Similar to FO_LINEAR, but compensates for the angle of the texture plane
- **Default="linear"**

## Parameter: mag {#sbParamMag}
- The texture filtering used when magnifying a texture
- Value of type string. Valid values are:
    - `none`: No filtering
    - `point`: Use the closest pixel
    - `linear`: Average of a 2x2 pixel area, denotes bilinear for MAG.
    - `anisotropic`: Similar to FO_LINEAR, but compensates for the angle of the texture plane
- **Default="linear"**

## Parameter: mip {#sbParamMip}
- The texture filtering used for mip filters.
- Value of type string. Valid values are:
    - `none`: No filtering
    - `point`: Use the closest pixel
    - `linear`: Average of a 2x2 pixel area, denotes trilinear for MIP.
    - `anisotropic`: Similar to FO_LINEAR, but compensates for the angle of the texture plane
- **Default="linear"**

## Parameter: u {#sbParamU}
- Defines what happens when the U texture coordinates exceed 1.0
- Value of type string. Valid values are:
    - `wrap`: Texture wraps at values over 1.0
    - `mirror`: Texture mirrors (flips) at joins over 1.0
    - `clamp`: Texture clamps at 1.0
    - `border`: Texture coordinates outside the range [0.0, 1.0] are set to the border colour
- **Default="clamp"**

## Parameter: v {#sbParamV}
- Defines what happens when the V texture coordinates exceed 1.0
- Value of type string. Valid values are:
  - `wrap`: Texture wraps at values over 1.0
  - `mirror`: Texture mirrors (flips) at joins over 1.0
  - `clamp`: Texture clamps at 1.0
  - `border`: Texture coordinates outside the range [0.0, 1.0] are set to the border colour
- **Default="clamp"**
  
## Parameter: w {#sbParamW}
- Defines what happens when the W texture coordinates exceed 1.0
- Value of type string. Valid values are:
  - `wrap`: Texture wraps at values over 1.0
  - `mirror`: Texture mirrors (flips) at joins over 1.0
  - `clamp`: Texture clamps at 1.0
  - `border`: Texture coordinates outside the range [0.0, 1.0] are set to the border colour
- **Default="clamp"**
  
## Parameter: miplodbias {#sbParamMiplodbias}
- You can alter the mipmap calculation by biasing the result with a single floating point value. After the mip level has been calculated, this bias value is added to the result to give the final mip level. Lower mip levels are larger (higher detail), so a negative bias will force the larger mip levels to be used, and a positive bias will cause smaller mip levels to be used. The bias values are in mip levels, so a -1 bias will force mip levels one larger than by the default calculation.
- Value of type float
- **Default=0**
  
## Parameter: max_anisotropic {#sbParamMaxAnisotropic}
- Sets the anisotropy level to be used for this texture level
- The degree of anisotropy is the ratio between the height of the texture segment visible in a screen space region versus the width - so for example a floor plane, which stretches on into the distance and thus the vertical texture coordinates change much faster than the horizontal ones, has a higher anisotropy than a wall which is facing you head on (which has an anisotropy of 1 if your line of sight is perfectly perpendicular to it)
- Minimum value of 1. Value is normally a power of 2. The maximum value is determined by the hardware, but it is usually 8 or 16
- For this to be used, `min`, `mag` or `mip` need to be "anisotropic"
- Value of type float.
- **Default=1**
  
## Parameter: compare_function {#sbParamComparefunction}
- Value of type string. Comparison functions used for the depth/stencil buffer operations and others. Valid values are:
    - `less`: Write if (new_Z < existing_Z)
    - `less_equal`: Write if (new_Z <= existing_Z) 
    - `equal`: Write if (new_Z == existing_Z) 
    - `not_equal`: Write if (new_Z != existing_Z) 
    - `greater_equal`: Write if (new_Z >= existing_Z)
    - `greater`: Write if (new_Z >= existing_Z)
    - `never`: Never writes a pixel to the render target. 
    - `always`: Always writes a pixel to the render target
    - `disabled`:
- **Default="disabled"**

## Parameter: border {#sbParamBorder}
- Sets the border colour. Related to the U,V & W modes.
- Value of type float array. Type vector4 array that defines a colour (RGBA).
- **Default=[1, 1, 1, 1]**

## Parameter: min_lod {#sbParamMinlod}
- Clamps the minimum texture lod used
- Value of type float.
- **Default=-3.40282347E+38**

## Parameter: max_lod {#sbParamMaxlod}
- Clamps the maximum texture lod used
- Value of type float.
- **Default=3.40282347E+38**

# Example Samplerblock Definition: {#sbExample}
```
{
    "samplers" :
    {
        "[sampler definition name]" :
        {
            "min" : "linear",
            "mag" : "linear",
            "mip" : "linear",
            "u" : "clamp",
            "v" : "clamp",
            "w" :  "clamp",
            "miplodbias" : 0,
            "max_anisotropic" : 1,
            "compare_function" : "disabled",
            "border" : [1, 1, 1, 1],
            "min_lod" : -3.40282347E+38,
            "max_lod" : 3.40282347E+38
        }
    }
}
```

# Links {#sbLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)