/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef _OgreResourceTransition_H_
#define _OgreResourceTransition_H_

#include "OgrePrerequisites.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    namespace ResourceLayout
    {
    enum Layout
    {
        Undefined,
        Texture,
        RenderTarget,
        RenderTargetReadOnly,
        Clear,
        Uav,
        CopySrc,
        CopyDst,
        MipmapGen,
        PresentReady,

        NumResourceLayouts
    };
    }

    namespace WriteBarrier
    {
    enum WriteBarrier
    {
        /// Notifies we've been writing from CPU to resource.
        CpuWrite        = 0x00000001,
        /// Waits until writes from shaders to resource are finished.
        Uav             = 0x00000002,
        /// Waits until rendering to colour buffers are finished.
        RenderTarget    = 0x00000004,
        /// Waits until rendering to depth/stencil buffers are finished.
        DepthStencil    = 0x00000008,

        /// Full write barrier
        All             = 0xffffffff
    };
    }

    namespace ReadBarrier
    {
    enum ReadBarrier
    {
        /// Finishes writing to & flushes all caches to VRAM so CPU can read it
        CpuRead         = 0x00000001,
        /// After the barrier, data can be used as an indirect buffer
        Indirect        = 0x00000002,
        /// After the barrier, data can be used as a vertex buffer
        VertexBuffer    = 0x00000004,
        /// After the barrier, data can be used as an index buffer
        IndexBuffer     = 0x00000008,
        /// After the barrier, data can be used as a const buffer
        ConstBuffer     = 0x00000010,
        /// After the barrier, data can be used as a texture or as a tex. buffer
        Texture         = 0x00000020,
        /// After the barrier, data can be used as an UAV
        Uav             = 0x00000040,
        /// After the barrier, data can be used as a RenderTarget
        RenderTarget    = 0x00000080,
        /// After the barrier, data can be used as a Depth/Stencil buffer
        DepthStencil    = 0x00000100,

        All             = 0xffffffff
    };
    }

    namespace ResourceAccess
    {
    /// Enum identifying the texture access privilege
    enum ResourceAccess
    {
        Undefined = 0x00,
        Read = 0x01,
        Write = 0x02,
        ReadWrite = Read | Write
    };

    const char* toString( ResourceAccess value );
    }

    struct ResourceTransition
    {
        GpuTrackedResource *resource;

        ResourceLayout::Layout      oldLayout;
        ResourceLayout::Layout      newLayout;

        ResourceAccess::ResourceAccess oldAccess;
        ResourceAccess::ResourceAccess newAccess;

        uint8 oldStageMask;
        uint8 newStageMask;

        // Deprecated
        uint32 writeBarrierBits;    /// @see WriteBarrier::WriteBarrier
        uint32 readBarrierBits;     /// @see ReadBarrier::ReadBarrier

        void    *mRsData;       /// Render-System specific data
    };

    typedef FastArray<ResourceTransition> ResourceTransitionArray;
    struct ResourceTransitionCollection
    {
        ResourceTransitionArray resourceTransitions;
        ResourceTransitionCollection() {}
    };

    struct GpuTrackedResource
    {
        virtual ~GpuTrackedResource();
        virtual bool isTextureGpu( void ) const { return false; }
    };

    struct ResourceStatus
    {
        ResourceLayout::Layout layout;
        ResourceAccess::ResourceAccess access;
        // Accumulates a bitmaks of shader stages currently using this resource
        uint8 stageMask;
        ResourceStatus() :
            layout( ResourceLayout::Undefined ),
            access( ResourceAccess::Undefined ),
            stageMask( 0u )
        {
        }
    };

    typedef StdMap<GpuTrackedResource*, ResourceStatus> ResourceStatusMap;

    class _OgreExport BarrierSolver
    {
        /// Contains previous state
        ResourceStatusMap mResourceStatus;
        FastArray<TextureGpu*> mCopyStateTextures;

    public:
        const ResourceStatusMap &getResourceStatus( void );

        void reset( ResourceTransitionCollection &resourceTransitions );

        /** By specifying how a texture will be used next, this function figures out
            the necessary barriers that may be required and outputs to resourceTransitions
            while storing the current state in mResourceStatus so the next calls to
            resolveTransition know what was the last usage
        @param resourceTransitions [in/out]
            Barriers created so far that will be executed as a single sync point.
            This variable may or may not be modified depending of whether a barrier is needed
        @param texture
            The texture you want to use
        @param newLayout
            How the texture will be used next
        @param access
            The kind of access: Read, Write, R/W.
        @param stageMask
            Bitmask of the shader stages that will be using this texture after this transition.
            Only useful when transitioning to ResourceLayout::Texture and ResourceLayout::Uav.
            Must be 0 otherwise.
        */
        void resolveTransition( ResourceTransitionCollection &resourceTransitions, TextureGpu *texture,
                                ResourceLayout::Layout newLayout, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

        /// Same as the other overload, but meant for buffers
        void resolveTransition( ResourceTransitionCollection &resourceTransitions,
                                GpuTrackedResource *bufferRes, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

        /** Tell the solver the texture has been transitioned to a different layout, externally
        @param newLayout
        @param access
        @param stageMask
        */
        void assumeTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                               ResourceAccess::ResourceAccess access, uint8 stageMask );

        /// Tell the solver all these resources have been transitioned to a different layout, externally
        void assumeTransitions( ResourceStatusMap &resourceStatus );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
