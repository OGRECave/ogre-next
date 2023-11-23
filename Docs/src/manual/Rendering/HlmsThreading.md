# Multithreaded Shader Compilation {#HlmsThreading}

Starting OgreNext 4.0, the following features were added:
 - Multithreaded Hlms shader generation.
 - Multithreaded shader compilation.
 - Multithreaded PSO generation.

Actual support depends on RenderSystems & CMake build settings. The user can call `Ogre::RenderSystems::supportsMultithreadedShaderCompliation` to query whether it is currently supported.

# CMake Options {#HlmsThreading_CMakeOptions}

The CMake option `OGRE_SHADER_COMPILATION_THREADING_MODE` was added. This option is independent of `OGRE_CONFIG_THREAD_PROVIDER`.

| Option | Short Description                                                                 | Long Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|--------|-----------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0      | Disabled.                                                                         | No shader compilation will be supported.<br/> The macro OGRE\_SHADER\_THREADING\_BACKWARDS\_COMPATIBLE\_API will be defined thus the class Hlms will provide setProperty, getProperty, unsetProperty and co. API calls that do not ask for a tid argument.<br/> If the user calls the functions that ask for a tid argument, **the value tid holds must be 0**.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 1      | Use compatible API.<br/>Enable only if supported by compiler and OS.<br/>Default setting. | The macro OGRE\_SHADER\_THREADING\_BACKWARDS\_COMPATIBLE\_API will be defined thus the class Hlms will provide setProperty, getProperty, unsetProperty and co. API calls that do not ask for a tid argument.<br/><br/>OgreNext will use compiler-assisted TLS (Thread Local Storage) to automatically determine the tid (thread ID) value. In practice OgreNext will **only** use TLS (and hence enabling multithreading) in fully static builds (i.e. CMake option `OGRE_STATIC = TRUE`), and disable it in dynamic library builds.<br/><br/> If `OGRE_STATIC = TRUE` then:<br/>  - If the user calls the functions that ask for a tid argument, the tid value will be used.<br/>  - If the user calls the functions that don't ask for a tid argument, OgreNext will use TLS to determine the correct tid value.<br/><br/>If `OGRE_STATIC = FALSE` the behavior is the same as Disabled. |
| 2      | Force-enable.                                                                     | The macro OGRE\_SHADER\_THREADING\_BACKWARDS\_COMPATIBLE\_API will **not** be defined.<br/> The user must always call setProperty, getProperty and co. that ask for a tid argument.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |

The default option is 1 because it allows to provide Multithreaded support when possible, while providing backwards-compatible API functions that make porting from OgreNext 3.0 easier. Make sure to read [Porting tips (from <= 3.0)](@ref Ogre40Changes_PortingTips) section.

**New users are encouraged to use option 2 instead** to make sure they can maximize performance and avoid confusion over which setProperty, getProperty \& co. functions they should use.

@note This option will not exist forever. See [Deprecation Plan](@ref DeprecationPlan).

# The tid (Thread ID) argument {#HlmsThreading_tidArgument}

Many functions provide or ask for a tid argument. When you're asked for a tid argument, you must pass it along. e.g.:

```cpp
namespace Ogre
{
    class MyHlmsListener : public HlmsListener
    {
        void propertiesMergedPreGenerationStep(
            Hlms *hlms, const HlmsCache &passCache, const HlmsPropertyVec &renderableCacheProperties,
            const PiecesMap renderableCachePieces[NumShaderTypes], const HlmsPropertyVec &properties,
            const QueuedRenderable &queuedRenderable, size_t tid ) override
        {
            // We pass along the tid value.
            int32_t propertyValue = hlms->getProperty( "MyPropertyName", tid );
        }
    };
}
```

@note Whenever a function provides a tid you must assume it is being executed in a worker thread and thus take all the necessary precautions if your code accesses a shared resource (such as using a mutex, or using the tid parameter to index per-thread storage).

Other functions such as Ogre::HlmsListener::preparePassHash don't ask for a tid. This always means the function is being called from the main rendering thread unless stated otherwise by the documentation. When calling setProperty/getProperty \& co. you must use `Ogre::Hlms::kNoTid`. e.g.:

```cpp
namespace Ogre
{
    class MyHlmsListener : public HlmsListener
    {
        void preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                                      bool dualParaboloid, SceneManager *sceneManager, Hlms *hlms ) override
        {
            // We are not being provided a tid value. Therefore we must pass kNoTid instead.
            int32_t propertyValue = hlms->getProperty( "MyPropertyName", Hlms::kNoTid );
        }
    };
}
```

If you're overriding the Ogre::Hlms class instead of Ogre::HlmsListener, the same rules apply. However you might override a function that is expected to be called from a worker thread but does not pass a tid value because the original Hlms implementations never needed it.

The rule is that anything called as part of Ogre::Hlms::createShaderCacheEntry needs a tid, while everything else is called from the main thread.

## API when OGRE_SHADER_COMPILATION_THREADING_MODE = 1

This option is used to facilitate porting from OgreNext 3.0.

All listener functions still provide a tid value. However OgreNext provides setProperty/getProperty functions that don't ask for one. Therefore it is possible to do this:

```cpp
namespace Ogre
{
    class MyHlmsListener : public HlmsListener
    {
        void propertiesMergedPreGenerationStep(
            Hlms *hlms, const HlmsCache &passCache, const HlmsPropertyVec &renderableCacheProperties,
            const PiecesMap renderableCachePieces[NumShaderTypes], const HlmsPropertyVec &properties,
            const QueuedRenderable &queuedRenderable, size_t tid ) override
        {
            // We are not passing along the tid value and it will compile if OGRE_SHADER_COMPILATION_THREADING_MODE < 2.
            // If OGRE_SHADER_COMPILATION_THREADING_MODE == 2, this line will not compile.
            int32_t propertyValue = hlms->getProperty( "MyPropertyName" );

            // This is always possible, which facilitates porting from OgreNext 3.0.
            int32_t propertyValue2 = hlms->getProperty( "MyPropertyName", tid );
        }
    };
}
```

# How does threaded Hlms work?

Hlms shaders are not randomly generated at any time from anywhere. It's not chaotic.

Shader generation requests can originate from 3 locations:

1. We're rendering. i.e. we're inside a `render_scene` compositor pass.
2. We're warming up the cache. i.e. we're inside a `warm_up` compositor pass.
3. We're loading the Ogre::HlmsDiskCache. i.e. we're inside Ogre::HlmsDiskCache::applyTo.

The last two cases are specifically designed to compile shaders and/or generate PSOs. That's its main function.

Those routines will collect as many shader generation requests as possible and then batch-compile them in worker threads.

The first case however, `render_scene`, is a little different. Normally a render_scene in OgreNext 3.0 looks like this (e.g. see the source code for Ogre::RenderQueue::renderGL3) in pseudo-code and extremely simplified:


```cpp
startHlmsJobs();

for( item : itemsToRender )
{
    Pso *pso = item.getPso();
    if( !pso )
        pso = createNewPsoFor( item ); // Note that two items may have the same PSO

    if( pso != lastPso )
        commandBuffer.addCommand( BindPso( pso ) );
    commandBuffer.addCommand( DrawCall( item ) );
    lastPso = pso;
}

// We can't execute all commands until we're done compiling all new PSOs.
waitForHlmsJobs();

// All commands we added so far haven't actually been executed yet. Do that now.
commandBuffer.executeAllCommands();
```

When OgreNext is compiling in parallel, the function `createNewPsoFor()` will actually return immediately with a valid Pso pointer, but it's not ready to be used yet.

That's why we must call `waitForHlmsJobs()` before calling `executeAllCommands()`.

Therefore OgreNext will process and iterate through many Items while worker threads compile shaders that were seen for the first time.

With a bit of luck, if the worker threads finish compiling before we reach `waitForHlmsJobs()` then there will be no stalls at all. If not, then the wait has likely reduced by a little. Furthermore, if multiple new PSOs are encountered, they will be delivered to different worker threads thus compiling in parallel.

## What is the range of tid argument?

The range is `[0; num_threads)`.