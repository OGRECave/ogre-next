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

#include "OgreStagingTexture.h"
#include "Vao/OgreVaoManager.h"

#include "OgreLogManager.h"

namespace Ogre
{
    StagingTexture::StagingTexture( VaoManager *vaoManager ) :
        mVaoManager( vaoManager ),
        mLastFrameUsed( vaoManager->getFrameCount() - vaoManager->getDynamicBufferMultiplier() )
  #if OGRE_DEBUG_MODE
        ,mMapRegionStarted( false )
        ,mUserQueriedIfUploadWillStall( false )
  #endif
    {
    }
    //-----------------------------------------------------------------------------------
    StagingTexture::~StagingTexture()
    {
    }
    //-----------------------------------------------------------------------------------
    bool StagingTexture::supportsFormat( PixelFormatGpu pixelFormat ) const
    {
        return true;
    }
    //-----------------------------------------------------------------------------------
    bool StagingTexture::uploadWillStall(void)
    {
#if OGRE_DEBUG_MODE
        mUserQueriedIfUploadWillStall = true;
#endif
        return mVaoManager->isFrameFinished( mLastFrameUsed );
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::startMapRegion(void)
    {
#if OGRE_DEBUG_MODE
        assert( !mMapRegionStarted && "startMapRegion already called!" );
        mMapRegionStarted = true;

        if( mVaoManager->getFrameCount() - mLastFrameUsed < mVaoManager->getDynamicBufferMultiplier() &&
            !mUserQueriedIfUploadWillStall )
        {
            LogManager::getSingleton().logMessage(
                "PERFORMANCE WARNING: Calling StagingTexture::startMapRegion too soon, "
                "it will very likely stall depending on HW and OS setup. You need to wait"
                " VaoManager::getDynamicBufferMultiplier frames after having called "
                "StagingTexture::upload before calling startMapRegion again.",
                LML_CRITICAL );
        }
#endif

        mVaoManager->waitForSpecificFrameToFinish( mLastFrameUsed );
    }
    //-----------------------------------------------------------------------------------
    TextureBox StagingTexture::mapRegion( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                          PixelFormatGpu pixelFormat )
    {
        assert( supportsFormat( pixelFormat ) );
#if OGRE_DEBUG_MODE
        assert( mMapRegionStarted && "You must call startMapRegion first!" );
#endif

        return mapRegionImpl( width, height, depth, slices, pixelFormat );
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::stopMapRegion(void)
    {
#if OGRE_DEBUG_MODE
        assert( mMapRegionStarted && "You didn't call startMapRegion first!" );
        mMapRegionStarted = false;
#endif
    }
    //-----------------------------------------------------------------------------------
    void StagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                                 uint8 mipLevel, const TextureBox *dstBox )
    {
#if OGRE_DEBUG_MODE
        assert( mMapRegionStarted && "You must call stopMapRegion before you can upload!" );
        mMapRegionStarted = false;
        mUserQueriedIfUploadWillStall = false;
#endif
        assert( !dstBox || srcBox.equalSize( *dstBox ) && "Src & Dst must be equal" );
        assert( !dstBox ||
                TextureBox( dstTexture->getWidth(), dstTexture->getHeight(),
                            dstTexture->getDepth(), dstTexture->getNumSlices() ).
                fullyContains( *dstBox ) );
        assert( TextureBox( dstTexture->getWidth(), dstTexture->getHeight(),
                            dstTexture->getDepth(), dstTexture->getNumSlices() ).
                fullyContains( srcBox ) );
        assert( srcBox.x == 0 && srcBox.y == 0 && srcBox.z == 0 && srcBox.sliceStart == 0 );
        assert( belongsToUs( srcBox ) &&
                "This srcBox does not belong to us! Was it created with mapRegion? "
                "Did you modify it? Did it get corrupted?" );

        mLastFrameUsed = mVaoManager->getFrameCount();
    }
}
