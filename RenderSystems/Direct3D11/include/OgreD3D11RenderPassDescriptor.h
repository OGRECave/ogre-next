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

#ifndef _OgreD3D11RenderPassDescriptor_H_
#define _OgreD3D11RenderPassDescriptor_H_

#include "OgreD3D11Prerequisites.h"

#include "OgreCommon.h"
#include "OgreD3D11DeviceResource.h"
#include "OgreRenderPassDescriptor.h"
#include "OgreRenderSystem.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    struct D3D11FrameBufferDescValue
    {
        uint16 refCount;
        D3D11FrameBufferDescValue();
    };

    typedef map<FrameBufferDescKey, D3D11FrameBufferDescValue>::type D3D11FrameBufferDescMap;

    /** D3D11 will share groups of ID3D11RenderTargetView all D3D11RenderPassDescriptor that
        share the same RTV setup. This doesn't mean these RenderPassDescriptor are exactly the
        same, as they may have different clear, loadAction or storeAction values.
    */
    class _OgreD3D11Export D3D11RenderPassDescriptor final : public RenderPassDescriptor,
                                                             public RenderSystem::Listener,
                                                             protected D3D11DeviceResource
    {
    protected:
        ComPtr<ID3D11RenderTargetView> mColourRtv[OGRE_MAX_MULTIPLE_RENDER_TARGETS];
        ComPtr<ID3D11DepthStencilView> mDepthStencilRtv;
        bool                           mHasStencilFormat;
        bool                           mHasRenderWindow;

        D3D11FrameBufferDescMap::iterator mSharedFboItor;

        D3D11Device       &mDevice;
        D3D11RenderSystem *mRenderSystem;

        void notifyDeviceLost( D3D11Device *device ) override;
        void notifyDeviceRestored( D3D11Device *device, unsigned pass ) override;

        void checkRenderWindowStatus();
        void calculateSharedKey();

        void updateColourRtv( uint8 lastNumColourEntries );
        void updateDepthRtv();

        /// Returns a mask of RenderPassDescriptor::EntryTypes bits set that indicates
        /// if 'other' wants to perform clears on colour, depth and/or stencil values.
        /// If using MRT, each colour is evaluated independently (only the ones marked
        /// as clear will be cleared).
        uint32 checkForClearActions( D3D11RenderPassDescriptor *other ) const;

        template <typename T>
        static void setSliceToRtvDesc( T &inOutRtvDesc, const RenderPassColourTarget &target );

    public:
        D3D11RenderPassDescriptor( D3D11Device &device, D3D11RenderSystem *renderSystem );
        ~D3D11RenderPassDescriptor() override;

        void entriesModified( uint32 entryTypes ) override;

        uint32 willSwitchTo( D3D11RenderPassDescriptor *newDesc, bool warnIfRtvWasFlushed ) const;

        void performLoadActions( Viewport *viewport, uint32 entriesToFlush, uint32 uavStartingSlot,
                                 const DescriptorSetUav *descSetUav );
        void performStoreActions( uint32 entriesToFlush );

        void clearFrameBuffer();

        void getCustomAttribute( IdString name, void *pData, uint32 extraParam ) override;

        // RenderSystem::Listener overload
        void eventOccurred( const String &eventName, const NameValuePairList *parameters ) override;
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
