/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "OgreMetalDevice.h"
#include "OgreMetalRenderSystem.h"
#include "OgreProfiler.h"

#import <Metal/MTLDevice.h>
#import <Metal/MTLCommandQueue.h>
#import <Metal/MTLCommandBuffer.h>

namespace Ogre
{
    MetalDevice::MetalDevice( MetalRenderSystem *renderSystem ) :
        mFrameAborted( false ),
        mDevice( 0 ),
        mMainCommandQueue( 0 ),
        mCurrentCommandBuffer( 0 ),
        mBlitEncoder( 0 ),
        mComputeEncoder( 0 ),
        mRenderEncoder( 0 ),
        mRenderSystem( renderSystem ),
        mStallSemaphore( 0 )
    {
        mStallSemaphore = dispatch_semaphore_create( 0 );
    }
    //-------------------------------------------------------------------------
    MetalDevice::~MetalDevice()
    {
        mDevice = 0;
        mMainCommandQueue = 0;
        mCurrentCommandBuffer = 0;
        mBlitEncoder = 0;
        mComputeEncoder = 0;
        mRenderEncoder = 0;
    }
    //-------------------------------------------------------------------------
    void MetalDevice::init( const MetalDeviceItem *device )
    {
        @autoreleasepool
        {
            mDevice = device ? device->getMTLDevice() : MTLCreateSystemDefaultDevice();
            mMainCommandQueue = [mDevice newCommandQueue];
            mCurrentCommandBuffer = [mMainCommandQueue commandBuffer];
            mBlitEncoder = 0;
            mComputeEncoder = 0;
            mRenderEncoder = 0;
        }
    }
    //-------------------------------------------------------------------------
    void MetalDevice::endBlitEncoder(void)
    {
        if( mBlitEncoder )
        {
            [mBlitEncoder endEncoding];
            mBlitEncoder = 0;
        }
    }
    //-------------------------------------------------------------------------
    void MetalDevice::endRenderEncoder( bool endRenderPassDesc )
    {
        if( mRenderEncoder )
        {
            [mRenderEncoder endEncoding];
            mRenderEncoder = 0;

            if( mRenderSystem->getActiveDevice() == this )
                mRenderSystem->_notifyActiveEncoderEnded( endRenderPassDesc );
        }
    }
    //-------------------------------------------------------------------------
    void MetalDevice::endComputeEncoder(void)
    {
        if( mComputeEncoder )
        {
            [mComputeEncoder endEncoding];
            mComputeEncoder = 0;

            if( mRenderSystem->getActiveDevice() == this )
                mRenderSystem->_notifyActiveComputeEnded();
        }
    }
    //-------------------------------------------------------------------------
    void MetalDevice::endAllEncoders( bool endRenderPassDesc )
    {
        endBlitEncoder();
        endRenderEncoder( endRenderPassDesc );
        endComputeEncoder();
    }
    //-------------------------------------------------------------------------
    void MetalDevice::commitAndNextCommandBuffer(void)
    {
        endAllEncoders();
        //Push the command buffer to the GPU
        [mCurrentCommandBuffer commit];
        @autoreleasepool
        {
            mCurrentCommandBuffer = [mMainCommandQueue commandBuffer];
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
            _rmt_BindMetal( mCurrentCommandBuffer );
#endif
        }

        mRenderSystem->_notifyNewCommandBuffer();
    }
    //-------------------------------------------------------------------------
    id<MTLBlitCommandEncoder> MetalDevice::getBlitEncoder(void)
    {
        if( !mBlitEncoder )
        {
            endRenderEncoder();
            endComputeEncoder();

            @autoreleasepool
            {
                mBlitEncoder = [mCurrentCommandBuffer blitCommandEncoder];
            }
        }

        return mBlitEncoder;
    }
    //-------------------------------------------------------------------------
    id<MTLComputeCommandEncoder> MetalDevice::getComputeEncoder(void)
    {
        if( !mComputeEncoder )
        {
            endRenderEncoder();
            endBlitEncoder();

            mComputeEncoder = [mCurrentCommandBuffer computeCommandEncoder];
        }

        return mComputeEncoder;
    }
    //-------------------------------------------------------------------------
    void MetalDevice::stall(void)
    {
        __block dispatch_semaphore_t blockSemaphore = mStallSemaphore;
        [mCurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
        {
            dispatch_semaphore_signal( blockSemaphore );
        }];
        commitAndNextCommandBuffer();

        const long result = dispatch_semaphore_wait( mStallSemaphore, DISPATCH_TIME_FOREVER );

        if( result != 0 )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failure while waiting for a MetalFence. Could be out of GPU memory. "
                         "Update your video card drivers. If that doesn't help, "
                         "contact the developers. Error code: " + StringConverter::toString( result ),
                         "MetalDevice::stall" );
        }

        mRenderSystem->_notifyDeviceStalled();
    }
    //-------------------------------------------------------------------------
    // MARK: -
    //-------------------------------------------------------------------------
    MetalDeviceItem::MetalDeviceItem()
        : mSameNameAdapterIndex(0)
    {
    }
    //-------------------------------------------------------------------------
    MetalDeviceItem::MetalDeviceItem( id<MTLDevice> device, unsigned sameNameIndex )
        : mDevice( device ), mSameNameAdapterIndex( sameNameIndex )
    {
    }
    //-------------------------------------------------------------------------
    String MetalDeviceItem::getDescription() const
    {
        String desc( mDevice.name.UTF8String ?: "" );
        if( mSameNameAdapterIndex != 0 )
        {
            desc.append( " (" )
                .append( Ogre::StringConverter::toString( mSameNameAdapterIndex + 1 ) )
                .append( ")" );
        }
        return desc;
    }
    //-------------------------------------------------------------------------
    id<MTLDevice> MetalDeviceItem::getMTLDevice() const
    {
        return mDevice;
    }
    //-------------------------------------------------------------------------
    // MARK: -
    //-------------------------------------------------------------------------
    void MetalDeviceList::clear()
    {
        mItems.clear();
    }
    //-------------------------------------------------------------------------
    void MetalDeviceList::refresh()
    {
        clear();
        map<String, unsigned>::type sameNameCounter;

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        LogManager::getSingleton().logMessage( "Metal: Devices Detection Starts" );
        
        NSArray<id<MTLDevice>> *availableDevices = MTLCopyAllDevices();
        for( id<MTLDevice> device in availableDevices )
        {
            String name = device.name.UTF8String;

            // inserted map entry would be zero-initialized by map::operator[]
            unsigned sameNameIndex = sameNameCounter[name]++;

            mItems.push_back( MetalDeviceItem( device, sameNameIndex ) );

            LogManager::getSingleton().logMessage("Metal: \"" + mItems.back().getDescription() + "\"");
        }
        LogManager::getSingleton().logMessage( "Metal: Devices Detection Ends" );
#endif
    }
    //-------------------------------------------------------------------------
    size_t MetalDeviceList::count() const
    {
        return mItems.size();
    }
    //-------------------------------------------------------------------------
    const MetalDeviceItem* MetalDeviceList::item( size_t index ) const
    {
        return &mItems.at( index );
    }
    //-------------------------------------------------------------------------
    const MetalDeviceItem* MetalDeviceList::item( const String &name ) const
    {
        vector<MetalDeviceItem>::type::const_iterator it = mItems.begin(), it_end = mItems.end();
        for( ; it != it_end; ++it )
            if( it->getDescription() == name )
                return &*it;
        
        return NULL;
    }
}
