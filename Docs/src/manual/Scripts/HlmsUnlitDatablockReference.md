
Reference Guide: HLMS Unlit Datablock {#hlmsunlitdatablockref}
=====================================

- Reference for a datablock for the Unlit HLMS implementation
- A Datablock is a "material" from the user's perspective. It is the only mutable block. It holds data (i.e. material properties) that will be passed directly to the shaders, and also holds which Macroblock, Blendblocks and Samplerblocks are assigned to it.
- Datablocks are located in JSON material filename: `[name].material.json`
- When loading a texture, if no sampler is provided, a default one from the HLMS is supplied
- All parameters are optional. Defaults are provided.

@tableofcontents

# Common Datablock Parameters: {#dbulCommonParameters}

## Parameter: alpha_test {#dbulParamAlphaTest}
- Sets the alpha test to the given compare function
- Alpha_test value is type array:
    - First value is a string and sets the compare function.
        - Valid values for the compare function: `less`, `less_equal`, `equal`, `not_equal`, `greater_equal`, `greater`, `never`, `always` and `disabled`
    - The second is a float and sets the test's threshold. Value typically in the range [0; 1) 
    - Optional third is a bool that sets weather to alpha test shadow caster only
- **DEFAULT=["disabled",0.5,false]** 

## Parameter: blendblock {#dbulParamBlendBlock}
- Sets the blendblock definition
- Value is type string, the name of a blendblock definition. Example: `"blendblock" : "blendblock_name"`

## Parameter: macroblock {#dbulParamMacroBlock}
- Sets the macroblock definition
- Value is type string, the name of a macroblock definition. Example: `"macroblock" : "macroblock_name"`

# Unlit Datablock Parameters {#dbulUnlitParameters}

## Parameter: diffuse {#dbulParamDiffuse}
- Sets the diffuse colour
- Type vector4 array (RGBA)
- This parameter is only used if it is called in the material file
- **DEFAULT="[1, 1, 1, 1]"**

## Parameter: diffuse_map[X] {#dbulParamDetailNormal}
- Sets a diffuse map
- Can contain up to 16 diffuse_map blocks (`diffuse_map0` to `diffuse_map15`)
- Block definition:
    - `animate` : Type float array (Matrix 4). **Default = [[1, 0, 0, 0],**
                                                            **[0, 1, 0, 0],** 
                                                            **[0, 0, 1, 0],**  
                                                            **[0, 0, 0, 1]]**
    - `blendmode` : Sets the blending mode (how the texture unit gets layered on top of the previous texture units). `diffuse_map0` ignores this parameter. **DEFAULT="NormalNonPremul".** Valid values:
        - `NormalNonPremul` : Regular alpha blending
        - `NormalPremul` : Premultiplied alpha blending
        - `Add` : 
        - `Subtract` :
        - `Multiply` :
        - `Multiply2x` : 
        - `Screen` : 
        - `Overlay` : 
        - `Lighten` :
        - `Darken` :
        - `GrainExtract` :
        - `GrainMerge` :
        - `Difference` :
    - `sampler`: Type string. The name of the sampler block definition to apply to the normal map texture. **OPTIONAL parameter**
    - `texture`: **OPTIONAL parameter**. Four value variations:
        - Type 1: Value is type string. Value is the name of a texture. The texture alias name is the same as the texture name. Example: `"texture" : "texture.tga"`
        - Type 2: Value is type string array. The first value is the name of a texture file. The second value is the alias name for the texture. Example: `"texture" : ["texture.tga","Texture_Alias_Name"]`
        - Type 3: Value is type string array. The first value is the name of a texture file. The second value is a boolean to specify if mipmaps should be generated automatically (only if the texture hasn't already been loaded and the file doesn't have mipmaps). Example: `"texture" : ["texture.tga", true]`
        - Type 4: Combination of Types 2 + 3. Example: `"texture" : ["texture.tga", "Texture_Alias_Name", true]`
    - `uv`: Type integer. Sets which UV set to use for the given texture. Value must be between in range [0, 8). **DEFAULT=0**

# Example Unlit Datablock Definition: {#dbulExample}

```json
{ 
    "unlit" : 
    {
        "[Material definition name]" :
        {
            "alpha_test" : ["disabled",0.5,false],
            "blendblock" : "blendblock_name",
            "macroblock" : "macroblock_name",
            "diffuse" : [1, 1, 1, 1],
            "diffuse_map0" :
            {
                "animate" : [[1, 0, 0, 0],
                            [0, 1, 0, 0],
                            [0, 0, 1, 0],
                            [0, 0, 0, 1]],
                "blendmode" : "NormalNonPremul",
                "sampler" : "unique_name",
                "texture" : "texture.tga",
                "uv" : 0.0
            }
        }
    }
}
```

## Links {#dbulLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)

