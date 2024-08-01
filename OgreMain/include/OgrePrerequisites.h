/*-------------------------------------------------------------------------
This source file is a part of OGRE-Next
(Object-oriented Graphics Rendering Engine)

For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE
-------------------------------------------------------------------------*/
#ifndef __OgrePrerequisites_H__
#define __OgrePrerequisites_H__

// Platform-specific stuff
#include "OgrePlatform.h"

#include <string>

// configure memory tracking
#if OGRE_DEBUG_MODE
#    if OGRE_MEMORY_TRACKER_DEBUG_MODE
#        define OGRE_MEMORY_TRACKER 1
#    else
#        define OGRE_MEMORY_TRACKER 0
#    endif
#else
#    if OGRE_MEMORY_TRACKER_RELEASE_MODE
#        define OGRE_MEMORY_TRACKER 1
#    else
#        define OGRE_MEMORY_TRACKER 0
#    endif
#endif

#if __clang__ && !defined( Q_CREATOR_RUN )
#    define ogre_nullable _Nullable
#    define ogre_nonnull _Nonnull
#    define OGRE_ASSUME_NONNULL_BEGIN _Pragma( "clang assume_nonnull begin" )
#    define OGRE_ASSUME_NONNULL_END _Pragma( "clang assume_nonnull end" )
#    define ogre_likely( x ) __builtin_expect( ( x ), 1 )
#    define ogre_unlikely( x ) __builtin_expect( ( x ), 0 )
#else
#    define ogre_nullable
#    define ogre_nonnull
#    define OGRE_ASSUME_NONNULL_BEGIN
#    define OGRE_ASSUME_NONNULL_END
#    if __GNUC__
#        define ogre_likely( x ) __builtin_expect( ( x ), 1 )
#        define ogre_unlikely( x ) __builtin_expect( ( x ), 0 )
#    else
#        define ogre_likely( x ) ( x )
#        define ogre_unlikely( x ) ( x )
#    endif
#endif

namespace Ogre
{
// Define ogre version
#define OGRE_VERSION_MAJOR 4
#define OGRE_VERSION_MINOR 0
#define OGRE_VERSION_PATCH 0
#define OGRE_VERSION_SUFFIX "unstable"
#define OGRE_VERSION_NAME "F"

#define OGRE_MAKE_VERSION( maj, min, patch ) ( ( maj << 16 ) | ( min << 8 ) | patch )
#define OGRE_VERSION ( ( OGRE_VERSION_MAJOR << 16 ) | ( OGRE_VERSION_MINOR << 8 ) | OGRE_VERSION_PATCH )
#define OGRE_NEXT_VERSION OGRE_VERSION

#define OGRE_UNUSED_VAR( x ) ( (void)x )

#if __cplusplus >= 201703L
#    define OGRE_FALLTHROUGH [[fallthrough]]
#else
#    if OGRE_COMPILER == OGRE_COMPILER_CLANG
#        define OGRE_FALLTHROUGH [[clang::fallthrough]]
#    elif OGRE_COMPILER == OGRE_COMPILER_GNUC
#        define OGRE_FALLTHROUGH [[gnu::fallthrough]]
#    else
#        define OGRE_FALLTHROUGH ( (void)0 )
#    endif
#endif

// define the real number values to be used
// default to use 'float' unless precompiler option set
#if OGRE_DOUBLE_PRECISION == 1
    /** Software floating point type.
    @note Not valid as a pointer to GPU buffers / parameters
    */
    typedef double Real;
    typedef uint64 RealAsUint;
#else
    /** Software floating point type.
    @note Not valid as a pointer to GPU buffers / parameters
    */
    typedef float        Real;
    typedef uint32       RealAsUint;
#endif

    /** In order to avoid finger-aches :)
     */
    typedef unsigned char  uchar;
    typedef unsigned short ushort;
    typedef unsigned int   uint;
    typedef unsigned long  ulong;

    // Pre-declare classes
    // Allows use of pointers in header files without including individual .h
    // so decreases dependencies between files

    struct Aabb;
    class Angle;
    class AnimableValue;
    class ArrayMatrix4;
    class ArrayMatrixAf4x3;
    class ArrayQuaternion;
    class ArrayVector3;
    class ArrayMemoryManager;
    class Archive;
    class ArchiveFactory;
    class ArchiveManager;
    class AsyncTextureTicket;
    class AsyncTicket;
    class AtmosphereComponent;
    class AutoParamDataSource;
    class AxisAlignedBox;
    class AxisAlignedBoxSceneQuery;
    class Barrier;
    class BillboardSet;
    class Bone;
    class BoneMemoryManager;
    struct BoneTransform;
    class BufferInterface;
    class BufferPacked;
    class Camera;
    struct CbBase;
    struct CbDrawCallIndexed;
    struct CbDrawCallStrip;
    class Codec;
    class ColourValue;
    class CommandBuffer;
    class ComputeTools;
    class ConfigDialog;
    class ConstBufferPacked;
    template <typename T>
    class Controller;
    template <typename T>
    class ControllerFunction;
    class ControllerManager;
    template <typename T>
    class ControllerValue;
    class DataStream;
    class Decal;
    class DefaultWorkQueue;
    class Degree;
    struct DepthBuffer;
    struct DescriptorSetSampler;
    struct DescriptorSetTexture;
    struct DescriptorSetTexture2;
    struct DescriptorSetUav;
    class DynLib;
    class DynLibManager;
    class EmitterDefData;
    class ErrorDialog;
    class ExternalTextureSourceManager;
    class Factory;
    class Forward3D;
    class ForwardClustered;
    class ForwardPlusBase;
    struct FrameEvent;
    class FrameListener;
    class Frustum;
    struct GpuLogicalBufferStruct;
    struct GpuNamedConstants;
    class GpuProgramParameters;
    class GpuSharedParameters;
    class GpuProgram;
    class GpuProgramManager;
    class GpuProgramUsage;
    class GpuResource;
    struct GpuTrackedResource;
    class HardwareOcclusionQuery;
    class HighLevelGpuProgram;
    class HighLevelGpuProgramManager;
    class HighLevelGpuProgramFactory;
    class Hlms;
    struct HlmsBlendblock;
    struct HlmsCache;
    class HlmsCompute;
    class HlmsComputeJob;
    struct HlmsComputePso;
    class HlmsDatablock;
    class HlmsListener;
    class HlmsLowLevel;
    class HlmsLowLevelDatablock;
    struct HlmsMacroblock;
    class HlmsManager;
    struct HlmsPso;
    struct HlmsSamplerblock;
    class HlmsTextureExportListener;
    struct HlmsTexturePack;
    class IndexBufferPacked;
    class IndirectBufferPacked;
    class InternalCubemapProbe;
    class IntersectionSceneQuery;
    class IntersectionSceneQueryListener;
    class Image2;
    class Item;
    struct KfTransform;
    class Light;
    class Log;
    class LogManager;
    class LodStrategy;
    class LodStrategyManager;
    class LwString;
    class ManualResourceLoader;
    class Material;
    class MaterialManager;
    class Math;
    class Matrix3;
    class Matrix4;
    class MemoryDataStream;
    class MemoryManager;
    class Mesh;
    class MeshManager;
    class ManualObject;
    class MovableObject;
    class MovablePlane;
#ifdef _OGRE_MULTISOURCE_VBO
    class MultiSourceVertexBufferPool;
#endif
    class Node;
    class NodeMemoryManager;
    struct ObjectData;
    class ObjectMemoryManager;
    class Particle;
    class ParticleAffector;
    class ParticleAffector2;
    class ParticleAffectorFactory;
    class ParticleEmitter;
    class ParticleEmitterFactory;
    class ParticleSystem;
    class ParticleSystem2;
    class ParticleSystemDef;
    class ParticleSystemManager;
    class ParticleSystemManager2;
    class ParticleSystemRenderer;
    class ParticleSystemRendererFactory;
    class ParticleVisualData;
    class Pass;
    class Plane;
    class PlaneBoundedVolume;
    class Plugin;
    class Profile;
    class Profiler;
    class Quaternion;
    class Radian;
    class Ray;
    class RaySceneQuery;
    class RaySceneQueryListener;
    class ReadOnlyBufferPacked;
    class Rectangle2D;
    class Renderable;
    class RenderPriorityGroup;
    class RenderQueue;
    class RenderQueueListener;
    class RenderObjectListener;
    class RenderPassDescriptor;
    class RenderSystem;
    class RenderSystemCapabilities;
    class RenderSystemCapabilitiesManager;
    class RenderSystemCapabilitiesSerializer;
    class Resource;
    class ResourceBackgroundQueue;
    class ResourceGroupManager;
    class ResourceManager;
    class Root;
    class RootLayout;
    class SceneManager;
    class SceneManagerEnumerator;
    class SceneNode;
    class SceneQuery;
    class SceneQueryListener;
    class ScriptCompiler;
    class ScriptCompilerManager;
    class ScriptLoader;
    class Semaphore;
    class Serializer;
    class ShadowCameraSetup;
    class SimpleMatrixAf4x3;
    class SimpleSpline;
    class SkeletonDef;
    class SkeletonInstance;
    class SkeletonManager;
    class Sphere;
    class SphereSceneQuery;
    class StagingBuffer;
    class StagingTexture;
    class StreamSerialiser;
    class StringConverter;
    class StringInterface;
    class SubItem;
    class SubMesh;
    class TagPoint;
    class Technique;
    class TempBlendedBufferInfo;
    class TexBufferPacked;
    class ExternalTextureSource;
    class TextureUnitState;
    struct TextureBox;
    class TextureGpu;
    class TextureGpuListener;
    class TextureGpuManager;
    struct TexturePool;
    struct Transform;
    class Timer;
    class UavBufferPacked;
    class UserObjectBindings;
    class VaoManager;
    class Vector2;
    class Vector3;
    class Vector4;
    class Viewport;
    class VertexAnimationTrack;
    struct VertexArrayObject;
    class VertexBufferPacked;
    class Window;
    class WireAabb;
    class WireBoundingBox;
    class WorkQueue;
    class CompositorManager2;
    class CompositorWorkspace;

    template <typename T>
    class SharedPtr;
    typedef SharedPtr<AnimableValue>          AnimableValuePtr;
    typedef SharedPtr<AsyncTicket>            AsyncTicketPtr;
    typedef SharedPtr<DataStream>             DataStreamPtr;
    typedef SharedPtr<GpuProgram>             GpuProgramPtr;
    typedef SharedPtr<GpuNamedConstants>      GpuNamedConstantsPtr;
    typedef SharedPtr<GpuLogicalBufferStruct> GpuLogicalBufferStructPtr;
    typedef SharedPtr<GpuSharedParameters>    GpuSharedParametersPtr;
    typedef SharedPtr<GpuProgramParameters>   GpuProgramParametersSharedPtr;
    typedef SharedPtr<HighLevelGpuProgram>    HighLevelGpuProgramPtr;
    typedef SharedPtr<Material>               MaterialPtr;
    typedef SharedPtr<MemoryDataStream>       MemoryDataStreamPtr;
    typedef SharedPtr<Mesh>                   MeshPtr;
    typedef SharedPtr<Resource>               ResourcePtr;
    typedef SharedPtr<ShadowCameraSetup>      ShadowCameraSetupPtr;
    typedef SharedPtr<SkeletonDef>            SkeletonDefPtr;

    namespace v1
    {
        class Animation;
        class AnimationState;
        class AnimationStateSet;
        class AnimationTrack;
        class Billboard;
        class BillboardChain;
        class BillboardSet;
        struct CbRenderOp;
        struct CbDrawCallIndexed;
        struct CbDrawCallStrip;
        class EdgeData;
        class EdgeListBuilder;
        class Entity;
        class HardwareIndexBuffer;
        class HardwareVertexBuffer;
        class HardwarePixelBuffer;
        class HardwarePixelBufferSharedPtr;
        class IndexData;
        class KeyFrame;
        class ManualObject;
        class Mesh;
        class MeshManager;
        class NumericAnimationTrack;
        class NumericKeyFrame;
        class OldBone;
        class OldNode;
        class OldNodeAnimationTrack;
        class OldSkeletonInstance;
        class OldSkeletonManager;
        class PatchMesh;
        class Pose;
        class RenderOperation;
        class RibbonTrail;
        class SimpleRenderable;
        class Skeleton;
        class SubEntity;
        class SubMesh;
        class TagPoint;
        class TransformKeyFrame;
        class VertexBufferBinding;
        class VertexData;
        class VertexDeclaration;
        class VertexMorphKeyFrame;

        typedef SharedPtr<Mesh>      MeshPtr;
        typedef SharedPtr<PatchMesh> PatchMeshPtr;
        typedef SharedPtr<Skeleton>  SkeletonPtr;
    }  // namespace v1
}  // namespace Ogre

/* Include all the standard header *after* all the configuration
settings have been made.
*/
#include "OgreStdHeaders.h"

#include "OgreMemoryAllocatorConfig.h"

namespace Ogre
{
#if OGRE_STRING_USE_CUSTOM_MEMORY_ALLOCATOR
#    if OGRE_WCHAR_T_STRINGS
    typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, STLAllocator<wchar_t, AllocPolicy>>
        _StringBase;
#    else
    typedef std::basic_string<char, std::char_traits<char>, STLAllocator<char, AllocPolicy>> _StringBase;
#    endif

#    if OGRE_WCHAR_T_STRINGS
    typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>,
                                    STLAllocator<wchar_t, AllocPolicy>>
        _StringStreamBase;
#    else
    typedef std::basic_stringstream<char, std::char_traits<char>, STLAllocator<char, AllocPolicy>>
        _StringStreamBase;
#    endif

#    define StdStringT( T ) std::basic_string<T, std::char_traits<T>, std::allocator<T>>
#    define CustomMemoryStringT( T ) \
        std::basic_string<T, std::char_traits<T>, STLAllocator<T, AllocPolicy>>

    template <typename T>
    bool operator<( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) < 0;
    }
    template <typename T>
    bool operator<( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) < 0;
    }
    template <typename T>
    bool operator<=( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) <= 0;
    }
    template <typename T>
    bool operator<=( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) <= 0;
    }
    template <typename T>
    bool operator>( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) > 0;
    }
    template <typename T>
    bool operator>( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) > 0;
    }
    template <typename T>
    bool operator>=( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) >= 0;
    }
    template <typename T>
    bool operator>=( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) >= 0;
    }

    template <typename T>
    bool operator==( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) == 0;
    }
    template <typename T>
    bool operator==( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) == 0;
    }

    template <typename T>
    bool operator!=( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) != 0;
    }
    template <typename T>
    bool operator!=( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return l.compare( 0, l.length(), o.c_str(), o.length() ) != 0;
    }

    template <typename T>
    CustomMemoryStringT( T ) operator+=( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return CustomMemoryStringT( T )( l ) += o.c_str();
    }
    template <typename T>
    CustomMemoryStringT( T ) operator+=( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return CustomMemoryStringT( T )( l.c_str() ) += o.c_str();
    }

    template <typename T>
    CustomMemoryStringT( T ) operator+( const CustomMemoryStringT( T ) & l, const StdStringT( T ) & o )
    {
        return CustomMemoryStringT( T )( l ) += o.c_str();
    }

    template <typename T>
    CustomMemoryStringT( T ) operator+( const StdStringT( T ) & l, const CustomMemoryStringT( T ) & o )
    {
        return CustomMemoryStringT( T )( l.c_str() ) += o.c_str();
    }

    template <typename T>
    CustomMemoryStringT( T ) operator+( const T *l, const CustomMemoryStringT( T ) & o )
    {
        return CustomMemoryStringT( T )( l ) += o;
    }

#    undef StdStringT
#    undef CustomMemoryStringT

#else
#    if OGRE_WCHAR_T_STRINGS
    typedef std::wstring _StringBase;
#    else
    typedef std::string _StringBase;
#    endif

#    if OGRE_WCHAR_T_STRINGS
    typedef std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>
        _StringStreamBase;
#    else
    typedef std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>>
        _StringStreamBase;
#    endif

#endif

    typedef _StringBase       String;
    typedef _StringStreamBase StringStream;
    typedef StringStream      stringstream;

}  // namespace Ogre

#if OGRE_STRING_USE_CUSTOM_MEMORY_ALLOCATOR
namespace std
{
    template <>
    struct hash<Ogre::String>
    {
    public:
        size_t operator()( const Ogre::String &str ) const
        {
            size_t _Val = 2166136261U;
            size_t _First = 0;
            size_t _Last = str.size();
            size_t _Stride = 1 + _Last / 10;

            for( ; _First < _Last; _First += _Stride )
                _Val = 16777619U * _Val ^ (size_t)str[_First];
            return ( _Val );
        }
    };
}  // namespace std
#endif

// Forward declaration of a regular STL. This is a workaround to prevent forward declaring
// an std::map & co (which is undefined behavior). Use this for rarely used maps/vector/etc that
// are passed by reference & need to be everywhere in headers (thus affecting compilation times)
namespace Ogre
{
#if !OGRE_CONTAINERS_USE_CUSTOM_MEMORY_ALLOCATOR
    template <typename T>
    class StdVector;

    template <typename K, typename V, typename P = std::less<K>>
    class StdMap;

    template <typename K, typename V, typename P = std::less<K>>
    class StdMultiMap;

    template <typename T>
    class StdList;

    template <typename K, typename H = ::std::hash<K>, typename E = std::equal_to<K>>
    class StdUnorderedSet;
#else
    template <typename, typename>
    class STLAllocator;

    template <typename T, typename A = STLAllocator<T, AllocPolicy>>
    class StdVector;

    template <typename K, typename V, typename P = std::less<K>,
              typename A = STLAllocator<std::pair<const K, V>, AllocPolicy>>
    class StdMap;

    template <typename K, typename V, typename P = std::less<K>,
              typename A = STLAllocator<std::pair<const K, V>, AllocPolicy>>
    class StdMultiMap;

    template <typename T, typename A = STLAllocator<T, AllocPolicy>>
    class StdList;

    template <typename K, typename H = ::std::hash<K>, typename E = std::equal_to<K>,
              typename A = STLAllocator<K, AllocPolicy>>
    class StdUnorderedSet;
#endif
}  // namespace Ogre

#include "OgreAssert.h"
#include "OgreWorkarounds.h"

#endif  // __OgrePrerequisites_H__
