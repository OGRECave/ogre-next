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

#ifndef _OgreRenderPassDescriptor_H_
#define _OgreRenderPassDescriptor_H_

#include "OgrePrerequisites.h"
#include "OgreColourValue.h"

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
                    clear( 0, 0, 8192, 8192 );
                    foreach( shadowmap in shadowmaps )
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );

                However this is heavily innefficient for tilers. In tilers you probably
                want to do the following:
                    foreach( shadowmap in shadowmaps )
                        clear( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );

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
            MultisampleResolve,
            /// Resolve MSAA rendering into resolve texture.
            /// Contents of MSAA texture are kept.
            StoreAndMultisampleResolve
        };
    }

    struct _OgreExport RenderPassTargetBase
    {
        TextureGpu  *texture;
        TextureGpu  *resolveTexture;

        uint8       mipLevel;
        uint8       resolveMipLevel;

        uint16      slice;
        uint16      resolveSlice;

        LoadAction::LoadAction      loadAction;
        StoreAction::StoreAction    storeAction;

        RenderPassTargetBase();
    };

    struct _OgreExport RenderPassColourTarget : RenderPassTargetBase
    {
        ColourValue clearColour;
        /// When true, slice will be ignored, and
        /// all slices will be attached instead.
        bool        allLayers;
        RenderPassColourTarget();
    };
    struct _OgreExport RenderPassDepthTarget : RenderPassTargetBase
    {
        Real    clearDepth;
        /// Assume attachment is read only (it's a hint, not an enforcement)
        bool    readOnly;
        RenderPassDepthTarget();
    };
    struct _OgreExport RenderPassStencilTarget : RenderPassTargetBase
    {
        uint32  clearStencil;
        /// Assume attachment is read only (it's a hint, not an enforcement)
        bool    readOnly;
        RenderPassStencilTarget();
    };

    class _OgreExport RenderPassDescriptor : public RenderSysAlloc
    {
    public:
        RenderPassColourTarget  mColour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassDepthTarget   mDepth;
        RenderPassStencilTarget mStencil;

    protected:
        uint8                   mNumColourEntries;
        bool                    mRequiresTextureFlipping;

        void checkRequiresTextureFlipping(void);

    public:
        RenderPassDescriptor();
        virtual ~RenderPassDescriptor();

        /** Call this when you're done modified mColour. Note there must be no gaps,
            e.g. mColour[1] is empty but mColour[0] & mColour[2] are not.
        @remarks
            Values that are modified by calling setClearColour et al don't need to call
            *entriesModified.
            Prefer changing those values using those calls since it's faster.
        */
        virtual void colourEntriesModified(void);
        virtual void depthModified(void);
        virtual void stencilModified(void);

        /// Sets the clear colour to specific entry.
        virtual void setClearColour( uint8 idx, const ColourValue &clearColour );
        virtual void setClearDepth( Real clearDepth );
        virtual void setClearStencil( uint32 clearStencil );

        /// Sets the clear colour to all entries. In some APIs may be faster
        /// than calling setClearColour( idx, clearColour ) for each entry
        /// individually.
        virtual void setClearColour( const ColourValue &clearColour );

        uint8 getNumColourEntries(void) const       { return mNumColourEntries; }
        bool requiresTextureFlipping(void) const    { return mRequiresTextureFlipping; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
