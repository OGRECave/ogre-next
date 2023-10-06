/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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
            ResolveDest,
            Clear,
            Uav,
            CopySrc,
            CopyDst,
            MipmapGen,
            /// Copy encoder is managing this texture. BarrierSolver can't resolve
            /// operations until the copy encoder is closed.
            /// (i.e. call RenderSystem::endCopyEncoder)
            CopyEncoderManaged,
            PresentReady,

            NumResourceLayouts
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

        const char *toString( ResourceAccess value );
    }  // namespace ResourceAccess

    struct ResourceTransition
    {
        GpuTrackedResource *resource;

        ResourceLayout::Layout oldLayout;
        ResourceLayout::Layout newLayout;

        /// If oldAccess == Undefined, it means there are no previous stage dependencies
        /// AND there is no guarantee previous contents will be preserved.
        ResourceAccess::ResourceAccess oldAccess;
        /// newAccess == Undefined is invalid
        ResourceAccess::ResourceAccess newAccess;

        /// If oldStageMask == Undefined, it means there are no previous
        /// stage dependencies (e.g. beginning of the frame)
        uint8 oldStageMask;
        /// If newStageMask == Undefined is invalid
        uint8 newStageMask;
    };

    typedef FastArray<ResourceTransition> ResourceTransitionArray;

    struct _OgreExport GpuTrackedResource
    {
        virtual ~GpuTrackedResource();
        virtual bool isTextureGpu() const { return false; }
    };

    struct ResourceStatus
    {
        ResourceLayout::Layout         layout;
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

    typedef StdMap<GpuTrackedResource *, ResourceStatus> ResourceStatusMap;

    class _OgreExport BarrierSolver
    {
        /// Contains previous state
        ResourceStatusMap mResourceStatus;

        /// Temporary variable that can be reused to avoid needless reallocations
        ResourceTransitionArray mTmpResourceTransitions;

        static void debugCheckDivergingTransition( const ResourceTransitionArray &resourceTransitions,
                                                   const TextureGpu              *texture,
                                                   const ResourceLayout::Layout   newLayout,
                                                   const RenderSystem            *renderSystem,
                                                   const ResourceLayout::Layout   lastKnownLayout );

    public:
        const ResourceStatusMap &getResourceStatus();

        /// Returns a temporary array variable that can be reused to avoid needless reallocations
        /// You're not forced to use it, but it will increase performance.
        ///
        /// Beware not to have it in use in two places at the same time! Use it as soon as possible
        ResourceTransitionArray &getNewResourceTransitionsArrayTmp()
        {
            mTmpResourceTransitions.clear();
            return mTmpResourceTransitions;
        }

        void reset();

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
        void resolveTransition( ResourceTransitionArray &resourceTransitions, TextureGpu *texture,
                                ResourceLayout::Layout newLayout, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

        /// Same as the other overload, but meant for buffers
        void resolveTransition( ResourceTransitionArray &resourceTransitions,
                                GpuTrackedResource *bufferRes, ResourceAccess::ResourceAccess access,
                                uint8 stageMask );

        /** Tell the solver the texture has been transitioned to a different layout, externally
        @param texture      See resolveTransition().
        @param newLayout    See resolveTransition().
        @param access       See resolveTransition().
        @param stageMask    See resolveTransition().
        */
        void assumeTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                               ResourceAccess::ResourceAccess access, uint8 stageMask );

        /// Tell the solver all these resources have been transitioned to a different layout, externally
        void assumeTransitions( ResourceStatusMap &resourceStatus );

        /** Similar to our notifyTextureChanged overload, to delete what is in mResourceStatus.
            However:

            - mCopyStateTextures needs a O(N) search. It is rare for us to put textures there
              so we use texture->addListener() and notifyTextureChanged to get notified
              when a texture living in mCopyStateTextures needs to be searched and removed
            - mResourceStatus is an std::map, and many textures can end up there
              so it could be rather inefficient to call addListener / removeListener frequently
              Just searching in mResourceStatus should be quite fast. Thus TextureGpuManager will call
              this function for all textures
        */
        void textureDeleted( TextureGpu *texture );
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
