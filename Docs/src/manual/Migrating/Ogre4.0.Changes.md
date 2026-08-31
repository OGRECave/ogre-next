# What's new in Ogre-Next 4.0 {#Ogre40Changes}

@tableofcontents

## Threaded Hlms

We introduced threaded Hlms shader generation, shader compilation, and PSO compilation.

This is used by the new `warm_up` compositor pass (see Ogre::CompositorPassWarmUp and Ogre::CompositorPassWarmUpDef), the Hlms Disk Cache, and during regular rendering.

This means that most Hlms operations gained a `tid` parameter, which contains the thread index in range `[0; num_threads)`.

Operations done in Ogre::Hlms::preparePassHash and Ogre::Hlms::calculateHashFor are single threaded and thus should use Ogre::Hlms::kNoTid.

## Hlms implementations and listeners

Custom Hlms implementations and listeners must update their virtual overload functions to accommodate the `tid` parameter.

This is explained in detail the new [Multithreaded Shader Compilation](@ref HlmsThreading) section.

### Porting tips (from <= 3.0) {#Ogre40Changes_PortingTips}

Watch out for calls to `mSetProperties.clear();` which now must be changed to either `mSetProperties[kNoTid].clear();` or `mSetProperties[tid].clear();`.

The same happens with `mSetProperties.empty();`.

We strongly recommend you use `override` C++ keyword to locate the API signatures that have changed (which now require you to add the tid argument).

Once that's done, most Hlms-related compiler errors (if setting [`OGRE_SHADER_COMPILATION_THREADING_MODE = 2`](@ref HlmsThreading_CMakeOptions)) can be fixed by doing a Find & Replace:

| Find:           | Replace with:           |
|-----------------|-------------------------|
| `setProperty( ` | `setProperty( tid, `    |
| `getProperty( ` | `getProperty( tid, `    |
| `setProperty( ` | `setProperty( kNoTid, ` |
| `getProperty( ` | `getProperty( kNoTid, ` |

You should Find & Replace in that order. If you first find all `setProperty( ` and replace them with `setProperty( kNoTid, `; you will end up with code that compiles but introduces race conditions and no way to identify them.

However if you first find all `setProperty( ` and replace them with `setProperty( tid, `, you will end up with code that does not compile wherever `kNoTid` should be used (still exercise care when replacing `tid` with `kNoTid`, make sure to be conscious of it. See [The tid (Thread ID) argument](@ref HlmsThreading_tidArgument) for details).

## Trivial Hlms changes

There are changes that will prevent compilation but can be easily fixed by adding a default value and ignoring them, as their behavior doesn't fundamentally change:

**Trivial Change:** `HlmsCache` now requires a new argument.

```cpp
// Old:
HlmsCache retVal( hash, mType, HlmsPso() );
// New:
HlmsCache retVal( hash, mType, HLMS_CACHE_FLAGS_NONE, HlmsPso() );
```

**Trivial Change:** Signature change in `notifyPropertiesMergedPreGenerationStep`.

```cpp
// Before (version A):
void notifyPropertiesMergedPreGenerationStep() override
{
	// ..
}

// After (version A):
PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep( size_t tid, PiecesMap *inOutPieces ) override
{
	// ..
	return PropertiesMergeStatusOk;
}

// Before (version B):
void notifyPropertiesMergedPreGenerationStep() override
{
	HlmsPbs::notifyPropertiesMergedPreGenerationStep();
	// ... your code ...
}

// After (version B):
PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep( size_t tid, PiecesMap *inOutPieces ) override
{
	PropertiesMergeStatus status =
		HlmsPbs::notifyPropertiesMergedPreGenerationStep( tid, inOutPieces );

	// It's not strictly necessary to return early. But on error
	// we know for certain this Hlms cannot be compiled.
	if( status == PropertiesMergeStatusError )
		return status;

	// ... your code ...
	return status; // Important, because status may be PropertiesMergeStatusWarning.
}
```

## Compositor Script changes

Added the `not_texture` keyword. This can improve performance in scenarios where you don't intend to sample from this texture i.e. usually in conjuntion with either the `uav` or `explicit_resolve` keywords:

```
compositor_node RenderingNodeMsaa
{
	in 0 rt_renderwindow

	texture msaaSurface target_width target_height target_format target_orientation_mode msaa 4 not_texture explicit_resolve

	rtv mainRtv
	{
		colour 0 msaaSurface resolve rt_renderwindow
	}

	target mainRtv
	{
		pass render_scene
		{
		}
	}

	out 0 rt_renderwindow
}
```

## New initialization step

If you plan on using Alpha Hashing, we provide both blue and white noise.

But blue noise requires you to call `mRoot->getHlmsManager()->loadBlueNoise()` during start up.
[See its new section in the manual](@ref AlphaHashingBlueNoiseSetup) for more information.

## HlmsUnlit changes

[HlmsUnlit now behaves like HlmsPbs](https://github.com/OGRECave/ogre-next/commit/9ee6dd793481b5378e9a68fd445a34435b802e1b) when it comes to the use of mReservedTexBufferSlots & mReservedTexSlots.

The variable `HlmsUnlit::mSamplerUnitSlotStart` was removed and `HlmsUnlit::mTexUnitSlotStart` is now autocalculated every pass, which means users must not rely on overriding this value.

Users deriving from HlmsUnlit must set mReservedTexBufferSlots & mReservedTexSlots instead, like it is done for HlmsPbs.

See [Colibri project's commit](https://github.com/darksylinc/colibrigui/commit/87e74824973007ee9f7f3f46719d2a6ba4948678) for an example of how the change was ported.

## Header renames

Due to issues with different IDEs and build systems, renamed several headers so that all header files have a unique name, even if they live in a different folder.

For example `Animation/OgreTagPoint.h` is now `Animation/OgreTagPoint2.h` because `OgreTagPoint.h` already existed. This can cause build errors when upgrading to 4.0 that are easily fix by switching to their new names.

See commits [75801f33df72844384549b11de96b10421584bce](https://github.com/OGRECave/ogre-next/commit/75801f33df72844384549b11de96b10421584bce) and [67f9fddd4292877dbedb2507fba7719c42aad97c](https://github.com/OGRECave/ogre-next/commit/67f9fddd4292877dbedb2507fba7719c42aad97c) for affected files.
The message of the commit was:

> Avoid identically named source files - some build systems have problems with it, even if CMake have none (headermaps in XCode, shared intermediate folder in Visual Studio)

## New AmbientLightMode values: AmbientHemisphereRim and AmbientHemisphereRimSquared

Two new ambient lighting modes have been added to `Ogre::HlmsPbs::AmbientLightMode`:

- `AmbientHemisphereRim`: Applies rim-like, high-contrast ambient lighting
- `AmbientHemisphereRimSquared`: Same as `AmbientHemisphereRim`, but the rim/contrast effect is even stronger

See the [Rim-Based Ambient Lighting](@ref GiAmbientLightingRim) section in the Global Illumination documentation for more details.

## New HlmsPbs::setEncodedLightmaps setting

A new pass-level setting has been added to `Ogre::HlmsPbs` to improve baked lighting workflows:

```cpp
void setEncodedLightmaps( bool bEncodedLightmaps );
```

When enabled, this setting treats the emissive map as an **encoded lightmap**, which allows fitting HDR baked lighting results into RGBA8_UNORM textures at the cost of some possible banding or quality loss.

The emissive texture stores both the baked lighting result and an encoded intensity value:

- The **RGB channels** store the baked lighting result (RGB) normalized to [0; 1].
- The **A channel** stores the maximum intensity value.

**When baking:**
- The shader uses the encoded intensity to normalize the baked lighting result, allowing for a wider dynamic range in low BPP textures.

**When rendering:**
- The shader decodes the lightmap in the reverse way.

### Usage Example

```cpp
// Enable encoded lightmaps for baked lighting
hlmsPbs->setEncodedLightmaps( true );
```

### Requirements

Either `CompositorPassSceneDef::mBakeLightingOnly` (see [bake_lighting_only](@ref CompositorNodesPassesRenderScene_bake_lighting_only)) or `HlmsPbsDatablock::setUseEmissiveAsLightmap` must be true for this setting to be relevant.

### Note

If your lightmap texture is `RGBA16_FLOAT`, this setting is likely a waste of performance and should not be enabled.
