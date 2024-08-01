# Root Layouts {#RootLayouts}

@tableofcontents

Ogre 2.3 introduced the concept of root layouts, and they need to be setup for Vulkan.

If you're familiar with D3D12, you may have noted we borrowed this concept.

# Old APIs (i.e. D3D11 and GL) {#RootLayoutsOldAPIs}

Older APIs like D3D11 and GL (and Metal 1) use a 'table binding' model. It is very simple to understand:

D3D11 offers 128 texture slots, thus textures can be bound to each slot:

```cpp
table[textures][0] = textureA;         // Bind to slot 0
table[textures][10] = textureB;        // Bind to slot 10
table[textures][50] = nullptr;         // Bind nothing to slot 50
table[textures][127] = anotherTexture; // Bind to slot 127
```

And its corresponding shader declaration:

```hlsl
uniform Texture2D textureA : register( t0 );
uniform Texture2D textureB : register( t10 );
uniform Texture2D textureA : register( t127 );
```

IHVs recommend to not leave gaps because they flush based on min-max. touched slots, e.g. if we have 10 textures and bind the first 9 at slots [0; 8] and the last one at slot 127, the driver will flush all 127 slots instead of just flushing 10.

The same table model is used for other resources: Const buffers, UAV buffers, UAV textures:

```cpp
table[const_buffers][2] = materialConstBufferA; // Bind to slot 2
```

```hlsl
cbuffer myObject : register(b2)
{       
    float4x4 matWorld;
    float3   vObjectPosition;
    int      arrayIndex;
}
```

This table model is simple to understand, although not all APIs agree on what goes together, e.g. in Metal regular textures and UAV textures share the same table. In D3D11 they have separate tables, thus regularTexture can be bound to slot `t1` and uavTexture to slot `u1`; while on Metal only one of them can be bound to slot `[[ texture( 1 ) ]]`

Ogre tries to abstract these differences.

# New APIs and Root Layouts {#RootLayoutsNewAPIs}

In newer APIs, these tables don't exist. Developers can layout resources in arbitrary ways and then describe the API what is in each offset. A binding can contain ANYTHING: sampler, const buffer, textures, etc

e.g. in pseudo code:

```hlsl
// Example A
uniform Texture2D textureA : register( slot 0 );
uniform Texture2D textureB : register( slot 1 );
cbuffer myObject : register( slot 2 ) {};
uniform Texture2D textureD : register( slot 3 );


// Example B
uniform Texture2D textureA : register( slot 0 );
uniform Texture2D textureB : register( slot 1 );
uniform Texture2D textureC : register( slot 2 );
uniform Texture2D textureD : register( slot 3 );
```

Two shaders that have the same root layout (even if they don't need to use all the resources declared) are said to be compatible.

In the example A & B are not compatible because slot 2 uses a different type. This means their size in bytes could be different, so even though `textureD` is in slot 3 in both shaders, their offset in bytes could be completely different.

Thus when switching from A to B we'd need to rebind everything again (or at best rebind slots 2 and 3, not just slot 2)

Sharing root layouts maximize CPU & GPU performance by lowering the amount of switching.
Conversely, very big root layouts can hurt GPU performance due to their size and amount of registers consumed.

Ultimately we want to share as much as possible but not make the Root Layout gigantic to just to achieve maximum sharing.

The fact that resource declaration can be so arbitrary gives a lot of power and flexibility but it can be difficult to setup, easy to mess up; and how to approach the problem can be overwhelming.

There are three ways to approach it:

 1. The shader is first compiled, the layout is manually and carefully maintained. The C++ and shader code assume the layout is preserved as is. Ogre cannot do this as we have to preserve compatibility with other APIs and we cannot enforce a layout to nor from our user either. This is too low level.
 2. The shader is first compiled, reflected, their slots extracted, a Root Layout is created out of it, the Root Layout is modified to work with how Ogre expects to bind data, and then the shader is recompiled again with patched binding slots.
 3. The root layout is first specified, then the shader is built with patched binding slots

Ogre follows the 3rd approach (except when arrays of textures are used, which use the 2nd approach).

Root Layouts can have up to 4 sets (which is the minimum guaranteed by Vulkan).

It is common practice in Ogre to leave set 0 for resources bound in the traditional way (i.e. like a table model in D3D11 / OpenGL) while set 1 is set to 'baked' where DescriptorSetTexture/Sampler/Texture2/Uav are bound to it.

**Another way to look at Ogre's RootLayout is that it basically tells Ogre what resources will the shader use so that we can properly emulate tables and compile the shader using slot locations calculated by us while the shader author can use binding slot index that have the same number as the shader code for other APIs (i.e. D3D11, GL)**

# Setting up root layouts {#RootLayoutsSettingUp}

An HLSL shader that ONLY declares and uses the following resources:

```hlsl
cbuffer myConstBuffer0      : register(b4) {};
cbuffer myConstBuffer1      : register(b5) {};
cbuffer myConstBuffer2      : register(b6) {};
Buffer<float4> myTexBuffer  : register(t1);
Texture2D myTex0            : register(t3);
Texture2D myTex1            : register(t4);
SamplerState mySampler      : register(s1);
RWTexture3D<float> myUavTex : register(u0);
RWBuffer<float> myUavBuffer : register(u1);
```

Would work using the following RootLayout:

```json
{
	"0" :
	{
		"const_buffers"     : [4,7],
		"tex_buffers"       : [1,2],
		"textures"          : [3,4],
		"samplers"          : [1,2],
		"uav_buffers"       : [1,2],
		"uav_textures"      : [0,1]
	}
}
```

That is, explicitly declare that you're using const buffer range [4; 7),
tex buffer range [1; 2) etc.

In Vulkan we will automatically generate macros for use in bindings
(buffers are uppercase letter, textures lowercase):

```cpp
// Const buffers
#define ogre_B4 set = 0, binding = 0
#define ogre_B5 set = 0, binding = 1
#define ogre_B6 set = 0, binding = 2

// Texture buffers
#define ogre_T1 set = 0, binding = 3

// Textures
#define ogre_t3 set = 0, binding = 4

// Samplers
#define ogre_s1 set = 0, binding = 5

// UAV buffers
#define ogre_U1 set = 0, binding = 6

// UAV textures
#define ogre_u0 set = 0, binding = 7
```

Thus a GLSL shader can use it like this:

```glsl
layout( ogre_B4 ) uniform bufferName
{
	uniform float myParam;
	uniform float2 myParam2;
};

layout( ogre_t3 ) uniform texture2D myTexture;
```

## Could you have used e.g. "const_buffers" : [0,7] instead of [4,7]?

i.e. declare slots in range [0;4) even though they won't be used?

Yes. But you would be consuming more memory.

Note that if your vertex shader uses slots [0; 3) and pixel shader uses range
[4; 7) then BOTH shaders must use a RootLayout that at least declares range [0; 7)
so they can be paired together

RootLayouts have a memory vs performance trade off:

  - Declaring unused slots helps reusing RootLayout. Reuse leads to fewer descriptor swapping
  - Unused slots waste RAM and VRAM

That's why low level materials provide prefab RootLayouts, in order to maximize
RootLayout reuse while also keeping reasonable memory consumption.

See `GpuProgram::setPrefabRootLayout`

# Declaring Root Layouts in shaders {#RootLayoutsDeclaringInShaders}

Shaders can declare their Root Layouts in JSON in comments as long as it starts with `## ROOT LAYOUT BEGIN` and end with `## ROOT LAYOUT END`:

```glsl
/*
## ROOT LAYOUT BEGIN
{
	"0" :
	{
		"has_params" : true
		"const_buffers" : [2, 4]
		"tex_buffers" : [0, 1]
		"samplers" : [1, 2],
		"textures" : [1, 2]
	}
}
## ROOT LAYOUT END
*/

#version 450

layout( ogre_P0 ) uniform Params {
	uniform float myParams;
};
layout( ogre_B2 ) uniform MyUBO {
	uniform float anotherParam;
};
layout( ogre_t1 ) texture2d myTexA;

void main()
{
	// glsl code...
}
```

Or it can be declared in C++. This is the preferred method for Hlms shader code to maximize performance.

`HlmsUnlit::setupRootLayout` has a simple example:

```cpp
void HlmsUnlit::setupRootLayout( RootLayout &rootLayout )
{
    DescBindingRange *descBindingRanges = rootLayout.mDescBindingRanges[0];

	// We have up to 3 const buffers at slots 0, 1 and 2
    descBindingRanges[DescBindingTypes::ConstBuffer].end = 3u;

	// When there's texture matrix animations, we must consume one extra buffer slot
    if( getProperty( UnlitProperty::TextureMatrix ) == 0 )
        descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 1u;
    else
        descBindingRanges[DescBindingTypes::ReadOnlyBuffer].end = 2u;

	// Baked sets are the ones that use DescriptorSetTexture/Sampler/Texture2/Uav
    rootLayout.mBaked[1] = true;
    DescBindingRange *bakedRanges = rootLayout.mDescBindingRanges[1];
    bakedRanges[DescBindingTypes::Sampler].start = (uint16)mSamplerUnitSlotStart;
    bakedRanges[DescBindingTypes::Sampler].end =
        (uint16)mSamplerUnitSlotStart + (uint16)getProperty( UnlitProperty::NumSamplers );

    bakedRanges[DescBindingTypes::Texture].start = (uint16)mTexUnitSlotStart;
    bakedRanges[DescBindingTypes::Texture].end =
        (uint16)mTexUnitSlotStart + (uint16)getProperty( UnlitProperty::NumTextures );

    mListener->setupRootLayout( rootLayout, mSetProperties );
}
```

# Baked sets {#RootLayoutsBakedSets}

Non-baked sets are meant to behave very similarly to table models in D3D11 and OpenGL.

Baked sets on the other hand are meant exclusively for binding `DescriptorSetTexture`, `DescriptorSetSampler`, `DescriptorSetTexture2` and `DescriptorSetUav`

The size of the DescriptorSet\* must match *exactly* the amount of bindings slots in the RootLayout

# Prefab Root Layouts for low level materials {#RootLayoutPrefabs}

To ease porting of low level materials (i.e. *.material and *.program scripts), most low level materials don't need to declare Root Layouts because we have prefabs for them:

| Prefab   | Description                                       |
|----------|---------------------------------------------------|
| None     | Defined in shader source or externally via C++    |
| Standard | 4 textures per material, VS and PS only (default) |
| High     | 8 textures per material, VS and PS only           |
| Max      | 32 textures per material, all shader stages       |

The majority of low level materials are fine with Standard, and this allows us to maximize Root Layout reuse.

If you need something different, you can either:

Change the prefab with `root_layout standard|high|max|none` in the shader program script definition, or declare the root layout inside the shader code.

# Arrays of Textures {#RootLayoutsArraysOfTextures}

When a shader contains arrays of textures or samplers with an array length > 1, e.g.

```glsl
layout( ogre_t0 ) texture2d myTexA[5];
layout( ogre_t6 ) texture2d myTexB[4];

layout( ogre_s0 ) texture2d mySamplers[3];
layout( ogre_s4 ) texture2d anotherSampler;
```

We need one of the following to specify that these slots are array, and what's their length:

## C++ {#RootLayoutsAoTCpp}

```cpp
rootLayout.addArrayBinding( DescBindingTypes::Texture, ArrayDesc( 0, 5 ) );
rootLayout.addArrayBinding( DescBindingTypes::Texture, ArrayDesc( 6, 4 ) );
rootLayout.addArrayBinding( DescBindingTypes::Sampler, ArrayDesc( 0, 3 ) );
```

## Inline shader declaration {#RootLayoutsAoTInlineShader}

```json
{
    "0" :
    {
        "textures" : [0, 10],
        "samplers" : [0, 4],
        "baked" : false
    },

    "arrays" :
    {
        "textures" : [[0, 5], [0, 4]],
		"samplers" : [[0, 3]],
    }
}
```

## Automatic {#RootLayoutsAoTAuto}

In automatic, arrays are not declared. But the shader will be compiled, reflected, **and then compiled again** with a patched Root Layout (unless there were no arrays).

On Debug builds, Ogre will always reflect shaders to check the declared arrays in root layouts match the arrays used by the shader.

Automatic is the default behavior for low level materials. For Hlms shaders it's turned off

Automatic can be turned on or off via `GpuProgram::setAutoReflectArrayBindingsInRootLayout` or its script counterpart `uses_array_bindings`

Compute Shaders can turn on automatic mode by setting the `uses_array_bindings` property via Hlms, e.g.

```
@pset( uses_array_bindings, 1 )
```

## Making GLSL shaders compatible with both Vulkan and OpenGL {#RootLayoutsGLSLForGLandVK}

Vulkan and OpenGL both use GLSL. However there are a few differences mostly because of the different binding model.

As a result we provide a few abstractions to separate these differences:

| Expression | Vulkan | OpenGL |
|---|---|---|
| `vulkan()` macro | Anything inside is kept | Anything inside is removed |
| `vulkan_layout()` macro | It is converted to `layout()` | It is removed |
| `#version ogre_glsl_ver_xxx` | Always converted to `#version 450` | The `ogre_glsl_ver_` part is removed and will be translated to `#version xxx` |

### Example: {#RootLayoutsGLSLForGLandVKExample}

```glsl
#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D tex;
vulkan( layout( ogre_s0 ) uniform sampler texSampler );

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float4x4 worldViewProjMatrix;
    uniform uint vertexBase;
    uniform uint3 bandMaskPower;
    uniform float2 sectionsBandArc;

    uniform uint3 numProbes;
    uniform float3 aspectRatioFixer;
vulkan( }; )
```

#### OpenGL

```glsl
#version 330

uniform texture2D tex;

in block
{
	vec2 uv0;
} inPs;

    uniform float4x4 worldViewProjMatrix;
    uniform uint vertexBase;
    uniform uint3 bandMaskPower;
    uniform float2 sectionsBandArc;

    uniform uint3 numProbes;
    uniform float3 aspectRatioFixer;
```

#### Vulkan

```glsl
#version 450

layout( ogre_t0 ) uniform texture2D tex;
layout( ogre_s0 ) uniform sampler texSampler;

layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

layout( ogre_P0 ) uniform Params {
    uniform float4x4 worldViewProjMatrix;
    uniform uint vertexBase;
    uniform uint3 bandMaskPower;
    uniform float2 sectionsBandArc;

    uniform uint3 numProbes;
    uniform float3 aspectRatioFixer;
};
```