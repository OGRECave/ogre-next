Tunning memory consumption and resources {#TuningMemoryResources}
========================================

@tableofcontents

Ogre has the following samples to show how to cleanup memory:

1. Samples/2.0/Tutorials/Tutorial_Memory
2. Samples/2.0/Tests/MemoryCleanup
3. Samples/2.0/Tests/TextureResidency

Memory is not released immediately. Of special relevance functions for **cleanup** are:

 1. `SceneManager::shrinkToFitMemoryPools`
 2. `VaoManager::cleanupEmptyPools`

Notice that these can take a large time as they defragment memory thus they
should not be called aggressively

For **tracking** resource consumption there is:

 1. `VaoManager::getMemoryStats` (see Samples/2.0/Tutorials/Tutorial_Memory)
 2. `TextureGpuManager::dumpStats`
 3. `TextureGpuManager::dumpMemoryUsage`
 

## Grouping textures by type {#GroupingTexturesByType}

You can iterate through all textures (`TextureGpuManager::getEntries`) and call:

 1. `TextureGpu::getSettingsDesc` to get a string describing the texture
 2. `TextureGpu::getSizeBytes`
 3. `TextureGpu::getSourceType` to understand where it comes from (regular textures vs compositor vs shadow mapping, etc)

## Dynamic vs Default buffers {#DynamicVsDefaultBuffers}

`BT_DEFAULT_DYNAMIC` is optimized for **data that is modified every frame from scratch.** It's rare to have to have a lot of this type of data except for particle FXs.

By default we triple the buffer size because Ogre manually synchronizes data to prevent arbitrary stalls. Ogre 1.x let the driver handle synchronization.

If your code was structured to map the buffer, even though you should be using `BT_DEFAULT` then we recommend you simply use a malloc'ed memory ptr:

```cpp
// Old code
void *data = buffer->map( ... );
// write to data
buffer->unmap();

// New code
void *data = OGRE_MALLOC_SIMD( ..., MEMCATEGORY_GEOMETRY );
// write to data
buffer->upload( data, 0, numElements );
OGRE_FREE_SIMD( data, MEMCATEGORY_GEOMETRY ); // Or leave the pointer around for reuse later
```

## Tweaking default memory consumption by VaoManager {#TweakingVaoManager}

When initializing Ogre through the first `Root::createRenderWindow`, you can provide settings via `NameValuePairList params`.

It accepts the following

```cpp
// Use double buffer instead of triple buffer to decrease latency,
// lowers BT_DEFAULT_DYNAMIC consumption
params.insert( std::make_pair( "VaoManager::mDynamicBufferMultiplier", "2" ) );

// Used by GL3+ & Metal
params["VaoManager::CPU_INACCESSIBLE"] = Ogre::StringConverter::toString( 8u * 1024u * 1024u );
params["VaoManager::CPU_ACCESSIBLE_DEFAULT"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::CPU_ACCESSIBLE_PERSISTENT"] = Ogre::StringConverter::toString( 8u * 1024u * 1024u ;
params["VaoManager::CPU_ACCESSIBLE_PERSISTENT_COHERENT"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );

// Used by Vulkan
params["VaoManager::CPU_INACCESSIBLE"] = Ogre::StringConverter::toString( 64u * 1024u * 1024u );
params["VaoManager::CPU_WRITE_PERSISTENT"] = Ogre::StringConverter::toString( 16u * 1024u * 1024u );
params["VaoManager::CPU_WRITE_PERSISTENT_COHERENT"] = Ogre::StringConverter::toString( 16u * 1024u * 1024u ;
params["VaoManager::CPU_READ_WRITE"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::TEXTURES_OPTIMAL"] = Ogre::StringConverter::toString( 64u * 1024u * 1024u );

// Used by D3D11
params["VaoManager::VERTEX_IMMUTABLE"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::VERTEX_DEFAULT"] = Ogre::StringConverter::toString( 8u * 1024u * 1024u );
params["VaoManager::VERTEX_DYNAMIC"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::INDEX_IMMUTABLE"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::INDEX_DEFAULT"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::INDEX_DYNAMIC"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::SHADER_IMMUTABLE"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::SHADER_DEFAULT"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
params["VaoManager::SHADER_DYNAMIC"] = Ogre::StringConverter::toString( 4u * 1024u * 1024u );
```

Note the default settings are different for Mobile (Android + iOS) builds

### Vulkan and `TEXTURES_OPTIMAL`

The pool `VaoManager::TEXTURES_OPTIMAL` is used on many GPUs but not all of them.

As it depends on `VkPhysicalDeviceLimits::bufferImageGranularity` property. When this value is 1, then `VaoManager::TEXTURES_OPTIMAL` pool is not used because all texture memory will go to the `VaoManager::CPU_INACCESSIBLE` pool. [Mostly AMD, and some Intel and Qualcomm GPUs are able to use this](http://vulkan.gpuinfo.org/displaydevicelimit.php?name=bufferImageGranularity&platform=all).

When the property is not 1, then all textures are placed into `VaoManager::TEXTURES_OPTIMAL` while buffers go into `VaoManager::CPU_INACCESSIBLE`.