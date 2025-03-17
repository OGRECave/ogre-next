/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#ifndef _OgreRenderPassDescriptor_H_
#define _OgreRenderPassDescriptor_H_

#include "OgrePrerequisites.h"

#include "OgreColourValue.h"
#include "OgreIdString.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    namespace LoadAction
    {
        enum LoadAction
        {
            /// Do not care about the initial contents. Useful for colour buffers
            /// if you intend to cover the whole screen with opaque draws.
            /// Not recommended for depth & stencil.
            DontCare,
            /// Clear the whole resource or a fragment of it depending on
            /// viewport settings.
            /// Actual behavior depends on the API used.
            Clear,
            /** On tilers, will clear the subregion.
                On non-tilers, it's the same as LoadAction::Load and assumes
                the resource is already cleared.
            @remarks
                This is useful for shadow map atlases. On desktop you probably
                want to render to a 8192x8192 shadow map in the following way:
                @code
                    clear( 0, 0, 8192, 8192 );
                    foreach( shadowmap in shadowmaps )
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );
                @endcode
                However this is heavily innefficient for tilers. In tilers you probably
                want to do the following:
                @code
                    foreach( shadowmap in shadowmaps )
                        clear( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );
                @endcode
                ClearOnTilers allows you to perform the most efficient shadow map rendering
                on both tilers & non-tilers using the same compositor script, provided the
                full clear pass was set to only execute on non-tilers.
            */
            ClearOnTilers,
            /// Load the contents that were stored in the texture.
            Load
        };
    }
    namespace StoreAction
    {
        enum StoreAction
        {
            /// Discard the contents after we're done with the current pass.
            /// Useful if you only want colour and don't care what happens
            /// with the depth & stencil buffers.
            /// Discarding contents either improves framerate or battery duration
            /// (particularly important in mobile), or makes rendering
            /// friendlier for SLI/Crossfire.
            DontCare,
            /// Keep the contents of what we've just rendered
            Store,
            /// Resolve MSAA rendering into resolve texture.
            /// Contents of MSAA texture are discarded.
            /// It is invalid to use this flag without an MSAA texture.
            MultisampleResolve,
            /// Resolve MSAA rendering into resolve texture.
            /// Contents of MSAA texture are kept.
            /// It is valid to use this flag without an MSAA texture.
            StoreAndMultisampleResolve,
            /// If texture is MSAA, has same effects as MultisampleResolve.
            /// If texture is not MSAA, has same effects as Store.
            /// There is one exception: If texture is MSAA w/ explicit resolves,
            /// but no resolve texture was set, it has same effects as Store
            ///
            /// To be used only by the Compositor.
            /// Should not be used in the actual RenderPassDescriptor directly.
            StoreOrResolve
        };
    }

    struct _OgreExport RenderPassTargetBase
    {
        TextureGpu *texture;
        TextureGpu *resolveTexture;

        uint8 mipLevel;
        uint8 resolveMipLevel;

        uint16 slice;
        uint16 resolveSlice;

        LoadAction::LoadAction   loadAction;
        StoreAction::StoreAction storeAction;

        RenderPassTargetBase();

        bool operator!=( const RenderPassTargetBase &other ) const;
        bool operator<( const RenderPassTargetBase &other ) const;
    };

    struct _OgreExport RenderPassColourTarget : RenderPassTargetBase
    {
        ColourValue clearColour;
        /// When true, slice will be ignored, and
        /// all slices will be attached instead.
        bool allLayers;
        RenderPassColourTarget();
    };
    struct _OgreExport RenderPassDepthTarget : RenderPassTargetBase
    {
        Real clearDepth;
        /// Assume attachment is read only (it's a hint, not an enforcement)
        bool readOnly;
        RenderPassDepthTarget();
    };
    struct _OgreExport RenderPassStencilTarget : RenderPassTargetBase
    {
        uint32 clearStencil;
        /// Assume attachment is read only (it's a hint, not an enforcement)
        bool readOnly;
        RenderPassStencilTarget();
    };

    class _OgreExport RenderPassDescriptor : public OgreAllocatedObj
    {
    public:
        enum EntryTypes
        {
            Colour0 = 1u << 0u,
            Colour1 = 1u << 1u,
            Colour2 = 1u << 2u,
            Colour3 = 1u << 3u,
            Colour4 = 1u << 4u,
            Colour5 = 1u << 5u,
            Colour6 = 1u << 6u,
            Colour7 = 1u << 7u,
            Depth = 1u << 30u,
            Stencil = 1u << 31u,
            Colour = Colour0 | Colour1 | Colour2 | Colour3 | Colour4 | Colour5 | Colour6 | Colour7,
            All = Colour | Depth | Stencil
        };

        RenderPassColourTarget  mColour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassDepthTarget   mDepth;
        RenderPassStencilTarget mStencil;

    protected:
        uint8 mNumColourEntries;
        bool  mRequiresTextureFlipping;
        /// When true, if we have a RenderWindow among our colour entries, then this
        /// pass is the last one to render to it and should ready the surface for
        /// presentation/swapping. After changing this flag you MUST call entriesModified( Colour );
        ///
        /// This value will be automatically reset to false if no entry is a RenderWindow
    public:
        bool mReadyWindowForPresent;
        /// When true, beginRenderPassDescriptor & endRenderPassDescriptor won't actually
        /// load/store this pass descriptor; but will still set the mCurrentRenderPassDescriptor
        /// so we have required information by some passes.
        /// Examples of these are stencil passes.
    public:
        bool mInformationOnly;

    public:
        void checkWarnIfRtvWasFlushed( uint32 entriesToFlush );

    protected:
        void         checkRequiresTextureFlipping();
        void         validateMemorylessTexture( const TextureGpu              *texture,
                                                const LoadAction::LoadAction   loadAction,
                                                const StoreAction::StoreAction storeAction,
                                                const bool                     bIsDepthStencil );
        virtual void colourEntriesModified();

    public:
        RenderPassDescriptor();
        virtual ~RenderPassDescriptor();

        /** Call this when you're done modified mColour. Note there must be no gaps,
            e.g. mColour[1] is empty but mColour[0] & mColour[2] are not.
        @remarks
            Values that are modified by calling setClearColour et al don't need to call
            *entriesModified.
            Prefer changing those values using those calls since it's faster.
        @param entryTypes
            Bitmask. See EntryTypes
        */
        virtual void entriesModified( uint32 entryTypes );

        /// Sets the clear colour to specific entry.
        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearDepth( Real clearDepth );
        virtual void setClearStencil( uint32 clearStencil );

        /// Sets the clear colour to all entries. In some APIs may be faster
        /// than calling setClearColour( idx, clearColour ) for each entry
        /// individually.
        virtual void setClearColour( const ColourValue &clearColour );

        virtual bool hasSameAttachments( const RenderPassDescriptor *otherPassDesc ) const;

        bool hasAttachment( const TextureGpu *texture ) const;

        uint8 getNumColourEntries() const { return mNumColourEntries; }
        bool  requiresTextureFlipping() const { return mRequiresTextureFlipping; }
        /// Returns true if either Stencil is set, or if Depth is set with depth-stencil attachment.
        bool hasStencilFormat() const;

        /// Finds the first non-null texture and outputs it
        /// May return nullptr if nothing is bound
        void findAnyTexture( TextureGpu **outAnyTargetTexture, uint8 &outAnyMipLevel );

        /**
        @param name
            When it's set to "ID3D11RenderTargetView", extraParam must be in range
            [0;OGRE_MAX_MULTIPLE_RENDER_TARGETS)
            When it's set to "ID3D11DepthStencilView", extraParam can be any value
        @param pData
            Output
        @param extraParam
            See name
         */
        virtual void getCustomAttribute( IdString name, void *pData, uint32 extraParam ) {}
    };

    struct _OgreExport FrameBufferDescKey
    {
        bool                 readyWindowForPresent;
        uint8                numColourEntries;
        bool                 allLayers[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase colour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase depth;
        RenderPassTargetBase stencil;

        FrameBufferDescKey();
        FrameBufferDescKey( const RenderPassDescriptor &desc );

        bool operator<( const FrameBufferDescKey &other ) const;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
