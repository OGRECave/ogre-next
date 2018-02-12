# What's new in Ogre 2.2 {#Ogre22Changes}

The main change has been a full refactor of textures.

The main reason of 2.2 is a complete overhaul of the Texture system, with a focus on
lowering GPU RAM consumption, background texture streaming, asynchronic GPU to CPU texture
transfers, and reducing out of GPU memory scenarios (which are relatively easy to run into
if your project is medium-to-large sized).

## Load Store semantics

Ogre 2.2 is much more mobile friendly. Metal introduced the concepts of “load” and “store”
actions, and we follow that paradigm because it’s easy to implement and understand.

First we need to explain how mobile TBDR GPUs work (**T**iled **B**ased **D**eferred
**R**enderers). In a regular immediate GPU (any Desktop GPU), the GPU just processes and
draws triangles to the screen in the order they’re submitted, writing results to RAM, and
reading from RAM if need to. Run the vertex shader, then the pixel shader, go on to the
next triangle.
The process is slightly more complex because there’s a lot of parallelization going on
(i.e. multiple triangles worked on a the same time), but the overall scheme of things is
that desktop GPUs process things in order.

TBDRs work differently: They process all the vertices first (i.e. run all of the vertex
shaders), and then later it goes through each tile (usually of 8×8 pixels), finds which
triangles touch that tile; sorts them front to back (unless alpha testing or alpha
blending is used). and then runs the pixel shaders on all the triangles and pixels.
Then proceeds to the next tile. This has the following advantages:

1. Most pixels filled by opaque triangles will only be shaded by a pixel shader only once.
1. Early-Z is implicit. This also means a depth-prepass is unnecessary and only a waste of
time.
1. The whole tile stays in an on-chip cache (which has much lower latency, much greater
bandwidth, and much lower power consumption). Once the tile is completely done, the cache
gets flushed into RAM. In contrast, a desktop GPU could be constantly reading back and
forth from main RAM (they have caches, but data doesn’t necessarily stay always in the
cache).

The main disadvantage is that it does not scale well to a large number of vertices (since
the GPU must store **all** of the processed vertices somewhere). There’s a performance cliff
past certain vertex count: Exceed certain threshold and you’ll see your framerate decrease
very fast the more vertices you add once you’re beyond that limit.

This is not usually a problem in mobile because well… nobody expects a phone to process
4 million vertices or more per frame. Also you can make it up by using improved pixel
shaders (because of the Early Z you get for free).

In TBDRs, each tile is a self-contained unit of work that never flushes the cache from
start to end until all the works has been done (unless the GPU has ran out of space
because we’ve submitted too much work, but let’s not dwell into that).

If you want a more in-depth explanation, read
[A look at the PowerVR graphics architecture: Tile-based rendering](https://www.imgtec.com/blog/a-look-at-the-powervr-graphics-architecture-tile-based-rendering/) and
[Understanding PowerVR Series5XT: PowerVR, TBDR and architecture efficiency](https://www.imgtec.com/blog/understanding-powervr-series5xt-powervr-tbdr-and-architecture-efficiency-part-4/).

### Now that we’ve explained how TBDRs work, we can explain load and store actions

In an immediate renderer, clearing the colour and depth buffers means we instruct the GPU
to basically memset the whole texture to a specific value. And then we render to it.

In TBDRs, this is inefficient; as the memset will store a value to RAM, that later needs
to be read from RAM. TBDRs can:

1. Clear the cache instead, rather than loading it from RAM. (i.e. set the cache to a
specific value each time the GPU begins a new tile)
1. If you don’t need the results of a particular buffer once you’re done rendering, you
can discard them instead of flushing it to RAM. This saves bandwidth and power.
For example you may not need to save the depth buffer. Or you may only need the resolved
result of MSAA render, and discard the contents of the MSAA surface.

The Metal RenderSystem in Ogre 2.1 tried to merge clears alongside draws as much as
possible, but it didn’t always get it right; and it glitched when doing complex MSAA
rendering.

Now in Ogre 2.2 you can explicitly specify what you want to do.
For load actions you can do:

1. DontCare: The cache is not initialized. This is the fastest option, and only works
glitch-free if you can guarantee you will render to all the pixels in the screen with
opaque geometry.
1. Clear: The cache is cleared to a particular value. This is fast.
1. Load: Load whatever was on RAM. This is the slowest, but also the default as it is the
safest option.

For store actions you also get:

1. DontCare: Discard the contents after we’re done with the current pass. Useful if you
only want colour and don’t care what happens with the depth & stencil buffers. Discarding
contents either improves framerate or battery duration or makes rendering friendlier to
SLI/Crossfire.
1. Store: Save results to RAM. This is the default.
1. MultisampleResolve: Resolve MSAA rendering into resolve texture.
Contents of MSAA texture are discarded.
1. StoreAndMultisampleResolve: Resolve MSAA rendering into resolve texture.
Contents of MSAA texture are kept.

This gives you a lot of power and control over how mobile GPUs control their caches in
order to achieve maximum performance. But be careful: If you set a load or store action to
“DontCare” and then you do need the values, then you’ll end up reading garbage every frame
(uninitialized values), which can result in glitches.

These semantics can also help on Desktop. Whenever we can, we emulate this behavior
(to make porting to mobile easier), but we also tell the API about this information
whenever the DX11 and GL APIs allow us. This can mostly help with SLI and Crossfire.

## More control over MSAA

Explicit MSAA finally arrived in Ogre 2.2; and thanks to load and store actions; you have
a lot of control over what happens with MSAA and when; which can result in high quality
rendering by making MSAA a first class citizen.

There have been other numerous MSAA changes. In Ogre 2.1 MRT + MSAA did not work correctly
except for D3D11 (which makes SSR sample to only work with MSAA in D3D11).
Now it works everywhere.

Another change for example that in Ogre 2.1 all render textures in the compositor default
to using MSAA if the main target is using MSAA. In Ogre 2.2; we default to not using MSAA
unless told otherwise. We found out most textures do not need MSAA because they’re
postprocessing FXs or similar; thus the MSAA is only a waste of memory and performance by
doing useless resolves. Therefore only a few render textures actually need MSAA.
This saves a lot of GPU RAM and some performance.


# Porting to Ogre 2.2 from 2.1

The impact from this change can vary a lot based on how you were using Ogre:

* Users that mostly interacted with textures via material scripts will barely need porting
efforts.
* Users that heavily manipulated textures from code (creation, upload, download,
destroying, using render textures) will have more work to do.

The following classes belonged to the "old" texturing system were removed or are not
functional and scheduled for removal:

* Texture
* v1::HardwarePixelBuffer
* RenderTarget
* RenderTexture
* RenderWindow
* TextureManager
* MultiRenderTarget

They were instead replaced by the following classes:

* TextureGpu
* TextureGpuManager
* StagingTexture
* AsyncTextureTicket
* Window
* RenderPassDescriptor

There are a few things that need to be clarified first hand:

1. TextureGpu replaces most of these older classes.
1. You cannot map a TextureGpu directly. You can only update its data via a StagingTexture
and read its contents via an AsyncTextureTicket. This better matches what GPUs actually
do behind the scenes.

The following table summarizes old and new classes:

| Task                                              	|                               2.1 	|                  2.2 	|
|---------------------------------------------------	|----------------------------------:	|---------------------:	|
| Use a texture                                     	|                           Texture 	|           TextureGpu 	|
| Render to texture                                 	|      RenderTarget / RenderTexture 	|           TextureGpu 	|
| Access a cubemap face or mipmap                   	|               HardwarePixelBuffer 	|           TextureGpu 	|
| Upload data (CPU → GPU)                           	|           Map HardwarePixelBuffer 	|       StagingTexture 	|
| Download data (GPU → CPU)                         	|           Map HardwarePixelBuffer 	|   AsyncTextureTicket 	|
| Setup MRT (Multiple Render Target)                	| RenderTexture + MultiRenderTarget 	| RenderPassDescriptor 	|
| Creating / destroying textures. Loading from file 	|                    TextureManager 	|    TextureGpuManager 	|
| Managing a window (events, resizing, etc)         	|                      RenderWindow 	|               Window 	|
| Rendering to a window                             	|                      RenderWindow 	|           TextureGpu 	|
| Dealing with depth buffers                        	|                       DepthBuffer 	|           TextureGpu 	|

You may have noticed that 'TextureGpu' is now repeated *a lot*. That is because in 2.1 the functionality was mainly fragmented between 3 classes:

1. Texture (owns multiple HardwarePixelBuffer, usually one per cubemap face and one per mipmap,
but not consistently, e.g. all slices in a 3D volume texture belong to just one
HardwarePixelBuffer)
1. HardwarePixelBuffer (owns a RenderTarget, which may be null if the texture cannot be
used as RenderTarget)
1. RenderTarget (may have a DepthBuffer associated, but doesn't own it)
1. DepthBuffer

This was madness because these distinctions were applied inconsistently and often made no
sense. e.g. a RenderTexture is often drawn to, and then used as a texture source.
Or we want to render to several mips (which are represented as separate RenderTargets)
This means we had to constantly walk upwards and downwards the hierarchy to get the
information we needed.

Now in Ogre 2.2, all of that is condensed into one class:

1. TextureGpu

This doesn't mean that TextureGpu is an overgrown God Class.
The fragmentation into 3 was a bad idea to begin with.

## Useful code snippets

### Create a TextureGpu based on a file

The following snippet loads a texture from file:
```
Ogre::Root *root = ...;
Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
Ogre::TextureGpu *texture = textureMgr->createOrRetrieveTexture(
                                "SaintPetersBasilica.dds",
                                Ogre::GpuPageOutStrategy::Discard,
                                Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
                                Ogre::TextureTypes::TypeCube,
                                Ogre::ResourceGroupManager::
                                AUTODETECT_RESOURCE_GROUP_NAME,
                                Ogre::TextureFilter::TypeGenerateDefaultMipmaps );
```

Note the texture is not loaded yet, so it consumes very little RAM, and functions like
getWidth() and getHeight() won't return valid values.
To load it, call:

```
texture->scheduleTransitionTo( Ogre::GpuResidency::Resident );
```

This will begin loading **in the background**, in a secondary thread managed by
TextureGpuManager.

For example, Trying to call texture->getWidth() immediately after scheduleTransitionTo
will incur in a race condition: if the secondary thread didn't load the texture yet,
you will get garbage values (probably 0 if the texture is new), or you'll get the real
value if the thread managed to load the texture in that time.

To safely call texture->getWidth() you can call:
```
texture->waitForMetadata();
```
This will block the calling thread until the metadata has been loaded. Metadata means:

1. Resolution (width, height, depth)
1. Pixel format
1. Number of mipmaps
1. Actual TextureType (e.g. 2D, 3D, 2DArray, Cubemap)

But the actual texture contents aren't done loaded yet. For that we have:

```
texture->waitForData();
```
This will block the calling thread until the texture is fully loaded.

Last, but not least, you can also call:

```
textureMgr->waitForStreamingCompletion();
```
This will block the calling thread until **all** pending textures are loaded.

### Create a TextureGpu based that you manually fill

The following snippet creates a texture that will be filled by hand (e.g. manually
loading from file yourself, procedural generation, etc):

```
Ogre::Root *root = ...;
Ogre::TextureGpuManager *textureMgr = root->getRenderSystem()->getTextureGpuManager();
Ogre::TextureGpu *texture = textureMgr->createOrRetrieveTexture(
                                "MyPcgTexture",
                                Ogre::GpuPageOutStrategy::Discard,
                                Ogre::TextureFlags::ManualTexture,
                                Ogre::TextureTypes::Type2D,
                                Ogre::BLANKSTRING,
                                0u );
texture->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
texture->setTextureType( Ogre::TextureTypes::Type2D );
texture->setNumMipmaps( 1u );
```

### Uploading data to a TextureGpu
```
const uint32 rowAlignment = 4u;
const size_t dataSize = Ogre::PixelFormatGpuUtils::getSizeBytes( texture->getWidth(),
                                                                 texture->getHeight(),
                                                                 texture->getDepth(),
                                                                 texture->getNumSlices(),
                                                                 texture->getPixelFormat(),
                                                                 rowAlignment );
const size_t bytesPerRow = texture->_getSysRamCopyBytesPerRow( 0 );
uint8 *imageData = reinterpret_cast<uint8*>( OGRE_MALLOC_SIMD( dataSize,
                                                               MEMCATEGORY_RESOURCE ) );

// ... fill imageData ...

//Tell the texture we're going resident. The imageData pointer is only needed
//if the texture pageout strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
//which is in this example is not, so a nullptr would also work just fine.
texture->_transitionTo( GpuResidency::Resident, imageData );
texture->_setNextResidencyStatus( GpuResidency::Resident );


//We have to upload the data via a StagingTexture, which acts as an intermediate stash
//memory that is both visible to CPU and GPU.
StagingTexture *stagingTexture = textureManager->getStagingTexture( texture->getWidth(),
                                                                    texture->getHeight(),
                                                                    texture->getDepth(),
                                                                    texture->getNumSlices(),
                                                                    texture->getPixelFormat() );

//Call this function to indicate you're going to start calling mapRegion. startMapRegion
//must be called from main thread.
stagingTexture->startMapRegion();
//Map region of the staging texture. This function can be called from any thread after
//startMapRegion has already been called.
TextureBox texBox = stagingTexture->mapRegion( texture->getWidth(), texture->getHeight(),
                                               texture->getDepth(), texture->getNumSlices(),
                                               texture->getPixelFormat() );
texBox.copyFrom( imageData, texture->getWidth(), texture->getHeight(), bytesPerRow );
//stopMapRegion indicates you're done calling mapRegion. Call this from the main thread.
//It is your responsability to ensure you're done using all pointers returned from
//previous mapRegion calls, and that you won't call it again.
//You cannot upload until you've called this function.
//Do NOT call startMapRegion again until you're done with upload() calls.
stagingTexture->stopMapRegion();
//Upload an area of the staging texture into the texture. Must be done from main thread.
//The last bool parameter, 'skipSysRamCopy', is only relevant for AlwaysKeepSystemRamCopy
//textures, and we set it to true because we know it's already up to date. Otherwise
//it needs to be false.
stagingTexture->upload( texBox, texture, 0, 0, true );
//Tell the TextureGpuManager we're done with this StagingTexture. Otherwise it will leak.
textureManager->removeStagingTexture( stagingTexture );
stagingTexture = 0;

//Do not free the pointer if texture's paging strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
OGRE_FREE_SIMD( imageData, MEMCATEGORY_RESOURCE );
imageData = 0;

//This call is very important. It notifies the texture is fully ready for being displayed.
//Since we've scheduled the texture to become resident and pp until now, the texture knew
//it was being loaded and that only the metadata was certain. This call here signifies
//loading is done; and any registered listeners will be notified.
texture->notifyDataIsReady();
```

You don't necessarily need to have one StagingTexture per TextureGpu for uploads.
For example you could have four 1024x1024 TextureGpus and request one StagingTexture of
2048x2048 or one of 1024x1024x4; map it four times and perform four uploads. In pseudo
code:

```
StagingTexture *stagingTexture = textureManager->getStagingTexture( 2048u, 2048u,
                                                                    1u, 1u,
                                                                    pixelFormat );
stagingTexture->startMapRegion();
//Bulk upload the 4 regions of the stash
TextureBox texBox0 = stagingTexture->mapRegion( 1024u, 1024u, 1u, 1u, pixelFormat );
texBox.copyFrom( imageData, 1024u, 1024u, bytesPerRow );
TextureBox texBox1 = stagingTexture->mapRegion( 1024u, 1024u, 1u, 1u, pixelFormat );
texBox.copyFrom( imageData, 1024u, 1024u, bytesPerRow );
TextureBox texBox2 = stagingTexture->mapRegion( 1024u, 1024u, 1u, 1u, pixelFormat );
texBox.copyFrom( imageData, 1024u, 1024u, bytesPerRow );
TextureBox texBox3 = stagingTexture->mapRegion( 1024u, 1024u, 1u, 1u, pixelFormat );
texBox.copyFrom( imageData, 1024u, 1024u, bytesPerRow );
stagingTexture->stopMapRegion();
//Perform Stash -> GPU transfers
stagingTexture->upload( texBox0, texture0, 0, 0, true );
texture0->notifyDataIsReady();
stagingTexture->upload( texBox1, texture1, 0, 0, true );
texture1->notifyDataIsReady();
stagingTexture->upload( texBox2, texture2, 0, 0, true );
texture2->notifyDataIsReady();
stagingTexture->upload( texBox3, texture3, 0, 0, true );
texture3->notifyDataIsReady();
```

**Please watch out for three things:**

1. Having many big StagingTextures can cause out of memory conditions. Incredibly big
StagingTextures can also reveal unusual driver/kernel bugs. Don't make them
too big. Don't keep too many of them that are small. Most people just need one
StagingTexture per texture because uploads won't be frequent.
1. StagingTextures can run out of space. mapRegion can return a nullptr in
`TextureBox::data` on failure. This space gets restored once stopMapRegion is called.
1. Use `StagingTexture::supportsFormat` to check if the parameters are compatible with
the upload you're trying to do. However, mapRegion may still fail if it has run
out of space. If supportsFormat returns false, it means mapRegion will always fail. If
supportsFormat returns true, it means mapRegion may or may not succeed.
1. Due to API restrictions, StagingTextures larger than 2048x2048 can only call mapRegion
for one slice at a time. For textures smaller than that, you can map several slices
contiguously.

### Upload streaming

Once you've called `StagingTexture::upload`; calling `StagingTexture::startMapRegion` again
will stall until the copy is done. You can call `StagingTexture::uploadWillStall` to
know if the StagingTexture is ready or not.

For streaming every frame (e.g. video playback, procedural animation from CPU), you
should use two or three StagingTextures (double vs triple buffer), and use one each frame
in cycle (do not release these StagingTextures every frame, hold on to them instead).

### Downloading data from TextureGpu into CPU

For that we'll use AsyncTextureTickets. They're like StagingTextures, but in the opposite
direction.

```
AsyncTextureTicket *asyncTicket =
        textureManager->createAsyncTextureTicket( width, height, depthOrSlices,
                                                  texture->getTextureType(),
                                                  texture->getPixelFormat() );
asyncTicket->download( texture, mip, true );

TextureBox dstBox = this->getData( mip - minMip );

if( asyncTicket->canMapMoreThanOneSlice() )
{
    const TextureBox srcBox = asyncTicket->map( 0 );
    dstBox.copyFrom( srcBox );
    asyncTicket->unmap();
}
else
{
    for( size_t i=0; i<asyncTicket->getNumSlices(); ++i )
    {
        const TextureBox srcBox = asyncTicket->map( i );
        dstBox.copyFrom( srcBox );
        dstBox.data = dstBox.at( 0, 0, 1u );
        --dstBox.numSlices;
        asyncTicket->unmap();
    }
}

textureManager->destroyAsyncTextureTicket( asyncTicket );
asyncTicket = 0;
```

Downloading a single mip for a 2D texture from GPU is straightforward and this code should
be enough. But a generic version that works on all types of textures has many small
details e.g. what if the TextureGpu uses msaa? It needs to be resolved first.

It is for that reason that we advise to use `Image2::convertFromTexture`, which handles
all the small details.

### Downloading streaming

Download streaming is very common in the case of video recording and web streaming.

After calling `AsyncTextureTicket::download`, calling `AsyncTextureTicket::map` to access
the contents can stall until the GPU is done with the transfer.
You can check if we're done by calling `AsyncTextureTicket::queryIsTransferDone`.

You should have two or three AsyncTextureTickets (double vs triple buffer) call
download() at frame N, and call map() at frame N+3 and recycle them (rather than
destroying them).

Additionally, since you'll be mapping 3 frames afterwards, you should call
download( texture, mip, accurateTracking=false );

Using accurateTracking = false reduces tracking overhead, at the expense of more
innacurate queryIsTransferDone calls (which are unnecessary if you are waiting for 3
frames to download).

*Warning:* calling queryIsTransferDone too often when accurateTracking = false; will cause
Ogre to switch to accurateTracking, and cause unnecessary overhead and can hurt your
performance a lot. This is because Ogre will assume your code could be stuck in an
infinite loop, e.g.
```
while( !ticket->queryIsTransferDone() )
	sleep( 1 );
```
and this loop would never exit unless Ogre switches that ticket to accurate tracking.
An warning in the Ogre.log will be logged if this happens.

Accurate tracking isn't slow, but switching from innaccurate to accurate is potentially
very slow, depending on many circumstances; as worst case scenario it can produce a full
stall (CPU waits until GPU has fully finished doing all pending jobs).

# Difference between depth, numSlices and depthOrSlices

You may find that throughout Ogre we refer to `depth`, `numSlices`, and `depthOrSlices`.
They're similar, but conceptually different.

**Depth** is associated with 3D volume textures. Depth is affected by mipmaps.
Meaning that a 512x512x256 texture at mip 0 becomes 256x256x128 at mip 1.
Current GPUs impose a maximum resolution of 2048x2048x2048 for 3D volume textures.
Note however, depending on format, you may run out of GPU memory before reaching such
high resolution.

**NumSlices** is associated with everything else: 1D Array & 2D Array textures, cubemap &
cubemap array textures. A cubemap has a hardcoded `numSlices` of 6. A cubemap array
must be multiple of 6.
Slices are not affected by mipmaps. Meaning that a 512x512x256 texture at mip 0 becomes
256x256x256 at mip 1.

**depthOrSlices** means that we're storing both depth and numSlices in the same variable.
When this happens, usually extra information is needed (in other words, is this a
3D texture or not?). For example TextureGpu declares `uint32 mDepthOrSlices` because
it also declares & knows the texture type `TextureTypes::TextureTypes mTextureType`
If mipmapping isn't involved, then the texture type is not required, thus for the maths
needed, depthOrSlices is the same as depth or numSlices.

AMD GCN Note: At the time of writing (2018/02/01), all known GCN cards add padding to
round texture resolutions to the next power of 2 if they have mipmaps.
This means a cubemap of 1024x1024x6 actually takes as much as 1024x1024x8.
This is very particularly important for 2D Array textures. Always try to use powers of 2
for your slices, otherwise you'll be wasting memory.

# Memory layout of textures and images

All data manipulated by Ogre (e.g. TextureBox, Image2, StagingTextures and
AsyncTextureTickets) have the following memory layouts and rules:

Internal layout of data in memory:
```
    Mip0 -> Slice 0, Slice 1, ..., Slice N
    Mip1 -> Slice 0, Slice 1, ..., Slice N
    ...
    MipN -> Slice 0, Slice 1, ..., Slice N
```

The layout for 3D volume and array textures is the same. The only thing that changes
is that for 3D volumes, the depth also decreases with each mip, while for array textures
it is kept constant.

For 1D array textures, the number of slices is stored in mDepthOrSlices, not in Height.

**For code reference, look at _getSysRamCopyAsBox implementation, and TextureBox::at.**

**Each row of pixels is aligned to 4 bytes (except for compressed formats that require
more strict alignments, such as alignment to the block).**

You can calculate bytesPerRow by doing:
```
const uint32 rowAlignment = 4u;
size_t bytesPerRow = Ogre::PixelFormatGpuUtils::getSizeBytes( width, 1u, 1u, 1u, pixelFormat, rowAlignment );
size_t bytesPerImage = Ogre::PixelFormatGpuUtils::getSizeBytes( width, height, 1u, 1u, pixelFormat, rowAlignment );
size_t bytesPerPixel = PixelFormatGpuUtils::getBytesPerPixel( pixelFormat );
```

# Troubleshooting errors

Ogre 2.2 code is new. While a lot of bugs have been ironed out already, the streaming code
may still contain a few hidden bugs.

Particularly, most of these bugs only surface if textures are loaded in a particular
order (because their resolution or pixel formats affect our algorithms in a way they
misbehave).

This is problematic with threading because results become non-deterministic: the first run
texture A was uploaded then B was uploaded, but on the second run texture A and B were
both uploaded as part of the same batch.
Likewise, different computers take different time to upload (because their CPUs and drives
are slower/faster) unearthing bugs that didn't reproduce in your machine.

To troubleshoot these annoying bugs, you can go to OgreMain/src/OgreTextureGpuManager.cpp
and uncomment the following macro:
```
#define OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD 1
```

This will force Ogre to stream using the main thread, thus behaving deterministically in
all machines.

Uncommenting the following macro may help finding out what's going on:
```
#define OGRE_DEBUG_MEMORY_CONSUMPTION 1
```

If the problem does not reproduce at all when OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
is defined, then play with the following parameters until you nail the problem:

* mEntriesToProcessPerIteration
* mMaxPreloadBytes
* mStagingTextureMaxBudgetBytes
* mBudget

And of course, report your problem and findings in https://forums.ogre3d.org

# RenderPassDescriptors

So far we've covered how to use regular textures. But we left out another big one:
Render Textures. Well, that's easy: just create a texture with TextureFlags::RenderTexture
flag:
```
mTexture = textureManager->createTexture( texName, GpuPageOutStrategy::Discard,
                                          TextureFlags::RenderToTexture,
                                          TextureTypes::Type2D );
```

But... how do we render to it?

Compositors hide most of the complexity from you, and you will likely not be dealing
with RenderPassDescriptors directly.

However if you're still reading here, chances are you're developing something advanced
that requires low level rendering, like developing your own Hlms or rendering a third
party GUI library.

RenderPassDescriptors describe self contained units of work to submit to GPUs, basically
to keep mobile TBDRs happy as described in a previous section.

Setting up a basic RenderPassDescriptors is straightforward:

```
renderPassDesc = renderSystem->createRenderPassDescriptor();

renderPassDesc->mColour[0].loadAction = LoadAction::Clear;
renderPassDesc->mColour[0].storeAction = StoreAction::StoreAndMultisampleResolve;
renderPassDesc->mColour[0].clearColour = ColourValue::White;
renderPassDesc->mColour[0].allLayers = true;
//Note that resolveTexture should be nullptr if texture isn't msaa.
//Also if texture->hasMsaaExplicitResolves() == false, then resolveTexture = texture
//is allowed, as the texture will have an internal texture to be used for resolving
renderPassDesc->mColour[0].texture = texture;
renderPassDesc->mColour[0].resolveTexture = resolveTexture;

renderPassDesc->mDepth.loadAction = LoadAction::Clear;
renderPassDesc->mDepth.storeAction = StoreAction::Store;
renderPassDesc->mDepth.clearDepth = 1.0f;
renderPassDesc->mDepth.readOnly = false;
renderPassDesc->mDepth.texture = depthBufferTexture;

//Same with renderPassDesc->mStencil as with mDepth

renderPassDesc->entriesModified( RenderPassDescriptor::All );

renderSystem->beginRenderPassDescriptor( renderPassDesc, texture, viewportSize,
										 scissors, overlaysEnabled, true );
renderSystem->executeRenderPassDescriptorDelayedActions();

//Render();

renderSystem->endRenderPassDescriptor();

//Do this when you're done with renderPassDesc to avoid leaks. Also,
//you are supposed to cache renderPassDesc rather than creating it and/or setting it up
//every frame.
renderSystem->destroyRenderPassDescriptor( renderPassDesc );
```

A few notes:

1. executeRenderPassDescriptorDelayedActions is of particular interest to Metal
RenderSystem. In this API, blitting operations (copying buffers, copying textures,
uploading to / downloading from textures, generating mipmaps) will break rendering.
Your "Render()" functions should never contain these operations between
executeRenderPassDescriptorDelayedActions and endRenderPassDescriptor. Those operations
must be done either before or afterwards.
1. Calling beginRenderPassDescriptor implies calling endRenderPassDescriptor. It is
encouraged to skip endRenderPassDescriptor and just call the next
beginRenderPassDescriptor unless this is the last one for this frame,
so we can perform certain optimizations in certain APIs.
Ideally you should only call endRenderPassDescriptor at the end of thoe whole frame.

Note RenderPassDescriptors have more parameters. For example, you can set which mipmap
you want to render to, or which slice in an array. You can also render to a 1024x1024 MSAA
texture and resolve the result into the mip 1 of a 2048x2048 texture (mip 1 is 1024x1024).

For further code reference on setting up RenderPassDescriptors you should look
into `CompositorPass::setupRenderPassDesc`.

# DescriptorSetTexture & co.

*Note: This section is only relevant to those writing their own Hlms implementations.*

Ogre 2.2 uses a different binding model to make compatibility in the future with
Vulkan and D3D12 easier.

Rather than binding one texture at a time into a huge table, these APIs work with
the concept of "descriptor sets". We could say in very layman's terms, that these are
just an array of textures, and every frame we bind the list instead.

Descriptor sets are managed in a similar way to how HlmsMacroblocks and HlmsBlendblocks
are created and destroyed:

```
DescriptorSetTexture baseSet;

baseSet.mTextures.push_back( textureA );
baseSet.mTextures.push_back( textureB );
baseSet.mTextures.push_back( textureC );

baseSet.mShaderTypeTexCount[VertexShader] = 1u; //textureA is bound to vertex shader
baseSet.mShaderTypeTexCount[PixelShader] = 2u; //textureB & C are bound to pixel shader

DescriptorSetTexture *mTexturesDescSet = hlmsManager->getDescriptorSetTexture( baseSet );

*commandBuffer->addCommand<CbTextures>() =
        CbTextures( texUnit, hazardousTexIdx, mTexturesDescSet );

hlmsManager->destroyDescriptorSetTexture( mTexturesDescSet );
```

hazardousTexIdx is a special index for textures that are potential hazards, such as
when a texture in particular in the descriptor set could be currently be also bound
as RenderTarget (which is illegal / undefined behavior).
hazardousTexIdx is in range [0; mTextureDescSet.mTextures.size()). If the value is outside
that range, we assume there are no potentially hazardous texture inside the descriptor
set.
This value is only used by D3D11 & Metal.

Creating a DescriptorSetTexture is this easy, and the same applies for
DescriptorSetSampler and DescriptorSetUav.
The difference between DescriptorSetTexture and DescriptorSetTexture2 is that the latter
allows doing more advanced stuff (such as reinterpretting the texture using a different
pixel format)

But if you take a look at `OGRE_HLMS_TEXTURE_BASE_CLASS::bakeTextures` in 
Components/Hlms/Common/include/OgreHlmsTextureBaseClass.inl you'll notice
this routine that generates the descriptor texture & sampler sets is overly complex.
Why is that?

The complexity of bakeTextures comes from two parts:

First, we try to take advantage of D3D11 and Metal. In OpenGL, texture and samplers
are tied together. This means that texture at slot 5 must use the sampler at slot 5.
The main drawback from this approach is that OpenGL is often limited to 16 textures
per shading stage (around 30 textures per shader stage for more modern cards and drivers).
While D3D11 and Metal split these two, meaning that texture at slot 5 can use sampler at
slot 1. This allows D3D11 and Metal to bind up to 128 textures and 16 samplers at the
same time. That's a lot more than what OpenGL can do.

Because supporting more than 16 textures has been a popular complaint about Ogre 2.1,
and dropping OpenGL is not an option, bakeTextures handles both paths. The code
would be simplified if we just assumed one of these paths.

The second reason is descriptor reuse: Material A may use Texture X, Y & Z.
While Material B may use Texture Z, Y & X (same textures, different order).
This different order would cause two sets to be generated. However we sort and ensure
textures are deterministically ordered; therefore the texture sets can be shared between
both materials as only one will be generated.

Additionally, please note that descriptor sets need to be invalidated when a texture
changes residency, which is why we listen for such changes via notifyTextureChanged.
