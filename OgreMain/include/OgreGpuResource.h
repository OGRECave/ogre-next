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

            /// VRAM and other GPU resources have been allocated for this resource.
            /// Data may not be yet fully uploaded to GPU though (but should be imminent).
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
            ///
            /// Warning: This flag has a few drawbacks for Textures.
            /// The metadata won't be ready earlier (i.e. it won't be available until the data
            /// is available).
            /// The metadata cache can't be used.
            /// Some calls such as TextureGpu::copyTo require syncing back from GPU to CPU.
            /// This is made async, but if you queue more than one copyTo call, it can stall.
            AlwaysKeepSystemRamCopy
        };
    }

    class _OgreExport GpuResource : public ResourceAlloc
    {
    protected:
        GpuResidency::GpuResidency              mResidencyStatus;
        /// Usually mResidencyStatus == mNextResidencyStatus; but if they're not equal,
        /// that means a change has been scheduled (async). Note: it only stores
        /// the last residency we'll be transitioning to. For example, it's possible
        /// Ogre has scheduled OnStorage -> Resident -> OnSystemRam; and none has been
        /// executed yet, in which case mNextResidencyStatus will be OnSystemRam
        ///
        /// @see    GpuResource::mPendingResidencyChanges
        GpuResidency::GpuResidency              mNextResidencyStatus;
        /// Developer notes: Strategy cannot be changed immediately,
        /// it has to be queued (due to multithreading safety).
        GpuPageOutStrategy::GpuPageOutStrategy  mPageOutStrategy;
        uint32  mPendingResidencyChanges;

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

        void _setNextResidencyStatus( GpuResidency::GpuResidency nextResidency );

        GpuResidency::GpuResidency getResidencyStatus(void) const;
        /** When getResidencyStatus() != getNextResidencyStatus(), residency changes happen
            in the main thread, while some preparation may be happening in the background.

            For example when a texture is not resident but getNextResidencyStatus says it will,
            a background thread is loading the texture file from disk, but the actual transition
            won't happen until the main thread changes it. You can call texture->waitForData()
            which will stall, as the main thread will be communicating back and forth with the
            background to see if it's ready; and when it is, the main thread will perform
            the transition inside waitForData

            Likewise, if that texture is resident but will soon not be, it is still legal
            to access its contents as long as you access them from the main thread before
            that main thread changes the residency. This gives you a strong serialization
            guarantee, but be careful with async tickets such as AsyncTextureTickets:

            If you call
                AsyncTextureTicket *asyncTicket = textureManager->createAsyncTextureTicket( ... );
                assert( texture->getResidencyStatus() == GpuResidency::Resident );
                ... do something else that calls Ogre functionality ...
                assert( texture->getResidencyStatus() == GpuResidency::Resident );
                asyncTicket->download( texture, mip, true );
            Then the second assert may trigger because that "do something else" ended up
            calling a function inside Ogre that finalized the transition.
            Once you've called download and the resource was still Resident, you are
            safe that your data integrity will be kept.
        @remarks
            Beware of the ABA problem. If the following transitions are scheduled:
                OnStorage -> Resident -> OnStorage
            Then both getResidencyStatus & getNextResidencyStatus will return OnStorage.
            Use GpuResource::getPendingResidencyChanges to fix the ABA problem.
        */
        GpuResidency::GpuResidency getNextResidencyStatus(void) const;
        GpuPageOutStrategy::GpuPageOutStrategy getGpuPageOutStrategy(void) const;

        void _addPendingResidencyChanges( uint32 value );

        /** Returns the number of pending residency changes.
            Residency changes may not be immediate and thus be delayed (e.g. see
            TextureGpu::scheduleTransitionTo).

            When this value is 0 it implies that mResidencyStatus == mNextResidencyStatus
        */
        uint32 getPendingResidencyChanges(void) const;

        IdString getName(void) const;
        /// Retrieves a user-friendly name. May involve a look up.
        /// NOT THREAD SAFE. ONLY CALL FROM MAIN THREAD.
        virtual String getNameStr(void) const;
    };
}

#include "OgreHeaderSuffix.h"

#endif
