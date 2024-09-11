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