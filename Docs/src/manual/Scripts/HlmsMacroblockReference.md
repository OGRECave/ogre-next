
Reference Guide: HLMS Macroblock {#hlmsmacroblockref}
================================

- Named like that because most entities end up using the macroblock. Except for transparents, we sort by macroblock first. These contain information like depth check & depth write, culling mode, polygon mode (point, wireframe, solid). They're quite analogous to D3D11_RASTERIZER_DESC. And not without reason: under the hood Macroblocks hold a ID3D11RasterizerState, and thanks to render queue's sorting, we change them as little as possible. In other words, reduce API overhead. On GL backends, we just change the individual states on each block change. Macroblocks can be shared by many Datablocks.
- Macroblocks are located in JSON material filename: `[name].material.json`
- A macro block contains settings that will rarely change, and thus are common to many materials.
- This is very analogous to D3D11_RASTERIZER_DESC.
- Up to 32 different blocks are allowed.
- All parameters are optional. Defaults are provided.

@tableofcontents

# Parameters: {#mbParameters}

## Parameter: scissor_test {#mbParamScissorText}
- Sets the state of scissor testing. Viewports hold the information about the scissor rectangle (see Viewport::setScissors).
- Value is true or false
- **DEFAULT=false**

## Parameter: depth_check {#mbParamDepthCheck}
- Sets whether or not this pass renders with depth-buffer checking on or not.
- If depth-buffer checking is on, whenever a pixel is about to be written to the frame buffer the depth buffer is checked to see if the pixel is in front of all other pixels written at that point. If not, the pixel is not written.
- If depth checking is off, pixels are written no matter what has been rendered before. Also see depth_function for more advanced depth check configuration. 
- Value is true or false
- **DEFAULT=true**

## Parameter: depth_write {#mbParamDepthWrite}
- Sets whether or not this pass renders with depth-buffer writing on or not.
- If depth-buffer writing is on, whenever a pixel is written to the frame buffer the depth buffer is updated with the depth value of that new pixel, thus affecting future rendering operations if future pixels are behind this one.
- If depth writing is off, pixels are written without updating the depth buffer Depth writing should normally be on but can be turned off when rendering static backgrounds or when rendering a collection of transparent objects at the end of a scene so that they overlap each other correctly.
- Value is true or false
- **DEFAULT=true**

## Parameter: depth_function {#mbParamDepthFunction}
- Sets the function used to compare depth values when depth checking is on.
- If depth checking is enabled a comparison occurs between the depth value of the pixel to be written and the current contents of the buffer. This comparison is normally Ogre::CMPF_LESS_EQUAL.
- Value of type string. Valid values are:
    - `less`: Write if (new_Z < existing_Z)
    - `less_equal`: Write if (new_Z <= existing_Z) 
    - `equal`: Write if (new_Z == existing_Z) 
    - `not_equal`: Write if (new_Z != existing_Z) 
    - `greater_equal`: Write if (new_Z >= existing_Z)
    - `greater`: Write if (new_Z >= existing_Z)
    - `never`: Never writes a pixel to the render target. 
    - `always`: Always writes a pixel to the render target
    - `disabled`:
- **Default="less_equal"**

## Parameter: depth_bias_constant {#mbParamDepthBiasConstant}
- Sets the bias applied to the depth value of this pass. 
- When polygons are coplanar, you can get problems with 'depth fighting' where the pixels from the two polys compete for the same screen pixel. This is particularly a problem for decals (polys attached to another surface to represent details such as bulletholes etc.).
- A way to combat this problem is to use a depth bias to adjust the depth buffer value used for the decal such that it is slightly higher than the true value, ensuring that the decal appears on top.
- The constant bias value, expressed as a factor of the minimum observable depth 
- Value of type float
- **Default=0**

## Parameter: depth_bias_slope_scale {#mbParamDepthBiasSlopeScale}
- Sets the bias applied to the depth value of this pass. 
- When polygons are coplanar, you can get problems with 'depth fighting' where the pixels from the two polys compete for the same screen pixel. This is particularly a problem for decals (polys attached to another surface to represent details such as bulletholes etc.).
- A way to combat this problem is to use a depth bias to adjust the depth buffer value used for the decal such that it is slightly higher than the true value, ensuring that the decal appears on top.
- Slope-relative biasing value varies according to the maximum depth slope relative to the camera: `finalBias = maxSlope * slopeScaleBias + constantBias`
- Note that slope scale bias, whilst more accurate, may be ignored by old hardware.
- Value of type float
- **Default=0**

## Parameter: cull_mode {#mbParamCullMode}
- Culling mode based on the 'vertex winding'.
- A typical way for the rendering engine to cull triangles is based on the 'vertex winding' of triangles. Vertex winding refers to the direction in which the vertices are passed or indexed to in the rendering operation as viewed from the camera, and will wither be clockwise or anticlockwise. The default is clockwise i.e. that only triangles whose vertices are passed/indexed in anticlockwise order are rendered - this is a common approach. You can alter this culling mode if you wish but it is not advised unless you know what you are doing. You may wish to use the CULL_NONE option for mesh data that you cull yourself where the vertex winding is uncertain.
- Value of type string. Valid values are: `none`, `clockwise` and `anticlockwise`
- **Default="clockwise"**

## Parameter: polygon_mode {#mbParamPolygonMode}
- Sets how polygons should be rasterised, i.e. whether they should be filled in, or just drawn as lines or points.
- Value of type string. Valid values are: `points`, `wireframe` and `solid`
- **Default="solid"**

# Example Macroblock Definition: {#mbExample}
```
{
    "macroblocks" :
    {
        "[macroblock definition name]" :
  	     {
             "scissor_test" : false,
             "depth_check" : true,
             "depth_write" : true,
             "depth_function" : "less_equal",
             "depth_bias_constant" : 0,
             "depth_bias_slope_scale" : 0,
             "cull_mode" : "clockwise",
             "polygon_mode" : "solid"
         }
     }
}
```

# Links {#mbLinks}
- [Ogre 2.3 HLMS Manual](https://ogrecave.github.io/ogre-next/api/2.3/hlms.html)
- [Ogre 13.1 Material Script Manual](https://ogrecave.github.io/ogre/api/latest/_material-_scripts.html#SEC23)