# What's new in Ogre 2.3 {#Ogre23Changes}

@tableofcontents

Since Ogre 2.2 there are multiple changes that warrant bumping the minor version to 2.3:

 - Mesh::importV1 deprecated over createByImportingV1
 - Unlit shaders unification can cause a few minor headaches if the old
 - Shader templates mix with the new ones
 - Upcoming Normal Offset Bias
 - Vulkan RenderSystem 

## Switch importV1 to createByImportingV1

In 2.2.2 and earlier we had a function called **Mesh::importV1** which would populate a v2 mesh by filling it with data from a v1 mesh, effectively importing it.

In 2.2.3 users should use **MeshManager::createByImportingV1** instead. This function 'remembers' which meshes have been created through a conversion process, which allows device lost handling to repeat this import process and recreate the resources.

Aside from this little difference, there are no major functionality changes and the function arguments are the same.

# Shadow's Normal Offset Bias

We've had a [couple complaints](https://forums.ogre3d.org/viewtopic.php?f=25&t=95723), but it wasn't until user [SolarPortal](https://forums.ogre3d.org/viewtopic.php?p=548193#p548193) made a more exhaustive research where we realized we were not using state of the art shadow mapping techniques.

We were relying on `hlmsManager->setShadowMappingUseBackFaces( true )` to hide most self-occlussion errors, but this caused other visual errors.

[Normal Offset Bias](https://github.com/godotengine/godot/issues/17260) is a technique from 2011 (yes, it's old!) which **drastically** improves self occlussion and shadow acne while improving overall shadow quality; and is much more robust than using inverted-culling during the caster pass.

Therefore this technique replaced the old one and the function `HlmsManager::setShadowMappingUseBackFaces()` **has been removed**.

Users can globally control normal-offset and constant biases per cascade by tweaking `ShadowTextureDefinition::normalOffsetBias` and `ShadowTextureDefinition::constantBiasScale` respectively.

You can also control them via compositors scripts in the shadow node declaration, using the new keywords **constant\_bias\_scale** and **normal\_offset\_bias**

Users porting from 2.2.x may notice their shadows are a bit different (for the better!), but may encounter some self shadowing artifacts. Thus they may have to adjust these two biases if they need to.

# Unlit vertex and pixel shaders unified

Unlit shaders were still duplicating its code 3 times (one for each RenderSystem) and all of its vertex & pixel shader code has been unified into a single .any file.

Although this shouldn't impact you at all, users porting from 2.2.x need to make sure old Hlms shader templates from Unlit don't linger and get mixed with the new files.

Pay special attention the files from `Samples/Media/Hlms/Unlit` match 1:1 the ones in your project and there aren't stray .glsl/.hlsl/.metal files from an older version.

If you have customized the Unlit implementation, you may find your customizations to be broken. But they're easy to fix. For reference look at Colibri's [two](https://github.com/darksylinc/colibrigui/commit/55b07452fc04fbd36688f6bab1b2cf85fd0f5851) [commits](https://github.com/darksylinc/colibrigui/commit/6e32d068972c3263448feb412b1343ae6b8c6daa) which ported its Unlit customizations from 2.2.x to 2.3.0

# Added HlmsMacroblock::mDepthClamp

It is now possible to toggle Depth Clamp on/off. Check if it's supported via RSC_DEPTH_CLAMP. All desktop GPU should support it unless you're using extremely old OpenGL drivers.
iOS supports it since A11 chip (iPhone 8 or newer)

Users upgrading from older Ogre versions should be careful their libraries and headers don't get out of sync. **A full rebuild is recommended**.

The reason being is that HlmsMacroblock (which is used almost anywhere in Ogre) added a new member variable. And if a DLL or header gets out of sync, it likely won't crash but the artifacts will be very funny (most likely depth buffer will be disabled).

# Added shadow pancaking

With the addition of depth clamp, we are now able to push the near plane of directional shadow maps in PSSM (non-stable variant). This greatly enhances depth buffer precision and reduces self-occlusion and acne bugs.

This improvement may make it possible for users to try using PFG_D16_UNORM instead of PFG_D32_FLOAT for shadow mapping, halving memory consumption.

Shadow pancaking is automatically disabled when depth clamp is not supported.

# PluginOptional

Old timers may remember that Ogre could crash if the latest DirectX runtimes were not installed, despite having an OpenGL backend as a fallback.

This was specially true during the Win 9x and Win XP eras which may not have DirectX 9.0c support. And stopped being an issue in the last decade since... well everyone has it now.

This problem came back with the Vulkan plugin, as laptops having very old drivers (e.g. from 2014) with GPUs that were perfectly capable of running Vulkan would crash due to missing system DLLs.

Furthermore, if the GPU cannot do Vulkan, Ogre would also crash.

We added the keyword `PluginOptional` to the Plugins.cfg file. With this, Ogre will try to load OpenGL, D3D11, Metal and/or Vulkan; and if these plugins fail to load, they will be ignored.

Make sure to update your Plugins.cfg to use this feature to provide a good experience to all of your users, even if they've got old HW or SW.

# Other relevant information when porting

 - `HlmsListener::hlmsTypeChanged` added an argument. Beware of it if you are overloading this function
 - `HlmsListener::propertiesMergedPreGenerationStep` changed its arguments. Beware of it if you are overloading this function
 - Terra's compositor does not need to expose terrain shadows' texture anymore. This is done automatically.
 - Root() constructor now takes one last string parameter to set the app's name. Use this string to tell vendors your app's name so they can create custom driver profiles.
 - SMAA was using `PFG_D24_UNORM_S8_UINT`, now it uses `PFG_D32_FLOAT_S8X24_UINT`
 - SMAA shaders were updated. If you were using SMAA, remember to grab the latest ones
 - `RenderSystem::_setTexture` added an argument which almost always should be set to false (should only be set to true if rendering to a depth buffer without writing to it while also binding that same depth buffer as a texture for sampling).
 - `TextureGpu` now has `getInternalWidth` and `getInternalHeight`. This happens because Vulkan on Android may require us to rotate the window ourselves to avoid performance degradation instead of letting the OS or HW do it (see `TextureGpu::setOrientationMode`). If orientation mode is 90° or 270°, then getInternalWidth returns the height and getInternalHeight returns the width). It is only relevant for Vulkan on Android. This is important if you need to perform copy operations or use AsyncTextureTickets on oriented textures.
 - `CompositorManager2::addWorkspace` removed the last parameters. `ResourceLayoutMap` and `ResourceAccessMap` are no longer needed, they're automatic. `addWorkspace` now accepts a `ResourceStatusMap` in case the workspace needs to assume the texture is in a specific initial layout (*very unlikely*)
 - Terra's compositor setup has changed again, but this time it's been simplified. It is no longer needed to expose the shadow map texture.
 - If you see 'Transitioning texture \<Texture Name\> from Undefined to a read-only layout. Perhaps you didn't want to set TextureFlags::DiscardableContent / aka keep_content in compositor?' messages, then you need to add 'keep_content' to the compositor's declaration. e.g.

Old:
`texture prevFrameDepthBuffer	target_width target_height PFG_R32_FLOAT uav`

New:
`texture prevFrameDepthBuffer	target_width target_height PFG_R32_FLOAT uav keep_content`

 This is normal for textures whose contents are meant to be carried over from the previous frame.

## Do not call notifyDataIsReady more than needed

In Ogre 2.2 you could call notifyDataIsReady as many times as you want. In fact we gave the following example which is now possibly wrong:

```cpp
stagingTexture->startMapRegion();
TextureBox box0 = stagingTexture->mapRegion( width, height, depth, slices, pixelFormat );
... write to box0.data ...
TextureBox box1 = stagingTexture->mapRegion( width, height, depth, slices, pixelFormat );
... write to box1.data ...
stagingTexture->stopMapRegion();
stagingTexture->upload( box0, texture0 );
stagingTexture->upload( box1, texture1 );
textureManager.removeStagingTexture( stagingTexture );
texture0->notifyDataIsReady();
texture1->notifyDataIsReady();
```

However in Ogre 2.3 every `notifyDataIsReady` must previously have had a call to `scheduleTransitionTo` (Resident) or `scheduleReupload`, assuming you didn't cancel the load e.g. assuming you didn't do this:

```cpp
tex->scheduleTransitionTo( GpuResidency::Resident );
tex->scheduleTransitionTo( GpuResidency::OnStorage );
// Must NOT call tex->notifyDataIsReady();
```

Additionally for `ManualTexture` textures, Ogre automatically calls `notifyDataIsReady` as soon as the texture becomes resident. Thus `notifyDataIsReady` shouldn *not* be called by the user if `TextureFlags::ManualTexture` flag is set.

The solution is simple: Remove the call to `notifyDataIsReady`, since it wasn't previous needed, and now it must not be there.

If the assert is triggering inside Ogre, then it means you previously called `notifyDataIsReady` on that texture and now Ogre is doing it again. Find where you're calling `notifyDataIsReady` and remove that line.

[Terra](https://github.com/OGRECave/ogre-next/commit/829d228857d05923ca420486ba7a1d875972f7ea#diff-95607f7ef0960e151e7726bd1b5a1fc3ead98b4e1c36887bbd1ae6c1da6741c6L136), SSAO, Postprocessing samples and [v1 Overlays](https://github.com/OGRECave/ogre-next/commit/829d228857d05923ca420486ba7a1d875972f7ea#diff-dcb86bb90358b03d4647a3d7f5d56811c187d76ddb292abc9c17b0e7b954c1ecL501) were updated to reflect this change.

 ## Global changes for Vulkan compatibility:

  - `GraphicsSystem::initialize` has changed slightly. Look for `SDL2x11` and `VSync Method`
  - The last compositor pass to render to the screen must set `RenderPassDescriptor::mReadyWindowForPresent` (Vulkan requirement). This is handled automatically in `CompositorManager2::prepareRenderWindowsForPresent` whenever the compositor chain changes. However if the workspace that presents to screen is disabled (i.e. you call `CompositorWorkspace::_update` manually) then you'll have to set this out yourself. The same happens if the last pass is a custom pass calls `initialize( rtv )` but never calls `setRenderPassDescToCurrent`.
   - EmptyProject's CMake scripts have been updated to account Vulkan plugin
