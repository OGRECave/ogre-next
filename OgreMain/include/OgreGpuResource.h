/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#ifndef _OgreGpuResource_H_
#define _OgreGpuResource_H_

#include "OgrePrerequisites.h"
#include "OgreIdString.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace GpuResidency
    {
        enum GpuResidency
        {
            /// Texture is on storage  (i.e. sourced from disk, from listener)
            /// A 4x4 blank texture will be shown if user attempts to use this Texture.
            /// No memory is consumed.
            /// While in this state, many settings may not be trusted (width, height, etc)
            /// as nothing is loaded.
            OnStorage,

            /// Texture is on System RAM.
            /// If the texture is fully not resident, a 4x4 blank texture will be shown if
            /// user attempts to use this Texture.
            /// If the texture is transitioning to Resident, a 64x64 mip version of the
            /// texture will be shown. Starts blank, gets progressively filled. If not
            /// enough resources we may just display the 4x4 blank dummy.
            /// Explicit APIs (D3D12 / Vulkan) may use true OS residency funcs.
            OnSystemRam,

            /// It's loaded on VRAM.
            /// Ready to be used by GPU.
            /// May keep a copy on system RAM (user tweakable)
            /// Texture may transition directly from OnStorage to Resident.
            /// However, Sys. RAM is a very attractive option for 64-bit editors
            /// (keep everything on Sys. RAM; load to GPU based on scene demands)
            Resident
        };
    }

    namespace GpuPageOutStrategy
    {
        /// When a resource is no longer resident, we need to know what to do with
        /// the resource CPU side.
        enum GpuPageOutStrategy
        {
            /// When the resource is no longer resident, we copy its contents from
            /// GPU back to system RAM and then release the GPU memory.
            /// This means the data will be ready for quickly moving it back
            /// to GPU if needed.
            SaveToSystemRam,

            /// When the resource is no longer resident, we just throw the GPU data.
            /// If the data is required again, it will be read from storage; which
            /// can be slow. Use this if you know you won't be using this resource
            /// for a long time after it's discarded (i.e. data that belongs to
            /// a specific level)
            Discard,

            /// Always keep a copy on system RAM, even when resident.
            /// This makes for fast reading from CPU.
            /// Not all resources allow this option (e.g. RTT & UAV textures)
            AlwaysKeepSystemRamCopy
        };
    }

    class _OgreExport GpuResource : public ResourceAlloc
    {
    protected:
        GpuResidency::GpuResidency              mResidencyStatus;
        GpuPageOutStrategy::GpuPageOutStrategy  mPageOutStrategy;

        /// User tells us priorities via a "rank" system.
        /// Must be loaded first. Must be kept always resident. Rank = 0
        /// Can be paged out if necessary. Rank > 0
        /// Prefer the terms "Important" and "Trivial".
        /// Avoid using the terms "high" rank and "low" rank since it's confusing.
        /// (because a "high" rank is important, but it is set by having a low
        /// number in mRank)
        int32   mRank;

        /// Every time a resource is used the frame count it was used in is saved.
        /// We’ll refer to this as resources "getting touched".
        /// The older a texture remains unused, the more likely it will be paged out
        /// (if memory thresholds are exceeded, and multiplied by rank).
        /// The minimum distance to camera may be saved as well. More distant textures
        /// may be paged out and replaced with 64x64 mips.
        uint32  mLastFrameUsed;
        float   mLowestDistanceToCamera;

        VaoManager  *mVaoManager;

        IdString mName;

    public:
        GpuResource( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                     VaoManager *vaoManager, IdString name );
        virtual ~GpuResource();

        GpuResidency::GpuResidency getResidencyStatus(void) const;
        GpuPageOutStrategy::GpuPageOutStrategy getGpuPageOutStrategy(void) const;

        IdString getName(void) const;
        /// Retrieves a user-friendly name. May involve a look up.
        virtual String getNameStr(void) const;
    };
}

#include "OgreHeaderSuffix.h"

#endif
