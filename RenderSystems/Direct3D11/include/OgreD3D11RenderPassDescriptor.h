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

#ifndef _OgreD3D11RenderPassDescriptor_H_
#define _OgreD3D11RenderPassDescriptor_H_

#include "OgreD3D11Prerequisites.h"
#include "OgreRenderPassDescriptor.h"
#include "OgreRenderSystem.h"
#include "OgreCommon.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    struct FrameBufferDescKey
    {
        uint8                   numColourEntries;
        bool                    allLayers[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase    colour[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        RenderPassTargetBase    depth;
        RenderPassTargetBase    stencil;

        FrameBufferDescKey();
        FrameBufferDescKey( const RenderPassDescriptor &desc );

        bool operator < ( const FrameBufferDescKey &other ) const;
    };
    struct FrameBufferDescValue
    {
        uint16 refCount;
        FrameBufferDescValue();
    };

    typedef map<FrameBufferDescKey, FrameBufferDescValue>::type FrameBufferDescMap;

    /** D3D11 will share groups of ID3D11RenderTargetView all D3D11RenderPassDescriptor that
        share the same RTV setup. This doesn't mean these RenderPassDescriptor are exactly the
        same, as they may have different clear, loadAction or storeAction values.
    */
    class _OgreD3D11Export D3D11RenderPassDescriptor : public RenderPassDescriptor,
                                                       public RenderSystem::Listener
    {
    protected:
        ID3D11RenderTargetView  *mColourRtv[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        ID3D11DepthStencilView  *mDepthStencilRtv;
        bool                    mHasStencilFormat;
        bool                    mHasRenderWindow;

        FrameBufferDescMap::iterator mSharedFboItor;

        D3D11Device         &mDevice;
        D3D11RenderSystem   *mRenderSystem;

        void checkRenderWindowStatus(void);
        void calculateSharedKey(void);

        virtual void updateColourRtv( uint8 lastNumColourEntries );
        virtual void updateDepthRtv(void);

        /// Returns a mask of RenderPassDescriptor::EntryTypes bits set that indicates
        /// if 'other' wants to perform clears on colour, depth and/or stencil values.
        /// If using MRT, each colour is evaluated independently (only the ones marked
        /// as clear will be cleared).
        uint32 checkForClearActions( D3D11RenderPassDescriptor *other ) const;

        template <typename T> static
        void setSliceToRtvDesc( T &inOutRtvDesc, const RenderPassColourTarget &target );

    public:
        D3D11RenderPassDescriptor( D3D11Device &device, D3D11RenderSystem *renderSystem );
        virtual ~D3D11RenderPassDescriptor();

        virtual void entriesModified( uint32 entryTypes );

        uint32 willSwitchTo( D3D11RenderPassDescriptor *newDesc, bool warnIfRtvWasFlushed ) const;

        void performLoadActions( Viewport *viewport, uint32 entriesToFlush,
                                 uint32 uavStartingSlot, const DescriptorSetUav *descSetUav );
        void performStoreActions( uint32 entriesToFlush );

        void clearFrameBuffer(void);

        virtual void getCustomAttribute( IdString name, void *pData, uint32 extraParam );

        // RenderSystem::Listener overload
        virtual void eventOccurred( const String &eventName,
                                    const NameValuePairList *parameters );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
