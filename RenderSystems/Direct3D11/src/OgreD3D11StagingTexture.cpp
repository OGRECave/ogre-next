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

#include "OgreD3D11StagingTexture.h"
#include "OgreD3D11TextureGpu.h"
#include "OgreD3D11Mappings.h"

#include "Vao/OgreD3D11VaoManager.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreLogManager.h"

namespace Ogre
{
    D3D11StagingTexture::D3D11StagingTexture( VaoManager *vaoManager, PixelFormatGpu formatFamily,
                                              uint32 width, uint32 height, uint32 depthOrSlices,
                                              D3D11Device &device ) :
        StagingTexture( vaoManager, formatFamily ),
        mStagingTexture( 0 ),
        mWidth( width ),
        mHeight( height ),
        mDepthOrSlices( depthOrSlices ),
        mIsArray2DTexture( false ),
        mDevice( device )
    {
        HRESULT hr;
        if( mDepthOrSlices > 1u &&
            mWidth <= D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION &&
            mHeight <= D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION )
        {
            D3D11_TEXTURE3D_DESC desc;
            memset( &desc, 0, sizeof( desc ) );

            desc.Width      = static_cast<UINT>( mWidth );
            desc.Height     = static_cast<UINT>( mHeight );
            desc.Depth      = static_cast<UINT>( mDepthOrSlices );
            desc.MipLevels  = 1u;
            desc.Format     = D3D11Mappings::getFamily( mFormatFamily );
            desc.Usage              = D3D11_USAGE_STAGING;
            desc.BindFlags          = 0;
            desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags          = 0;

            ID3D11Texture3D *texture = 0;
            hr = mDevice->CreateTexture3D( &desc, 0, &texture );
            mStagingTexture = texture;
            mIsArray2DTexture = false;

            mSubresourceData.resize( 1u );
            mLastSubresourceData.resize( 1u );
            memset( &mSubresourceData[0], 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
            memset( &mLastSubresourceData[0], 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc;
            memset( &desc, 0, sizeof( desc ) );

            desc.Width      = static_cast<UINT>( mWidth );
            desc.Height     = static_cast<UINT>( mHeight );
            desc.MipLevels  = 1u;
            desc.ArraySize  = static_cast<UINT>( mDepthOrSlices );
            desc.Format     = D3D11Mappings::getFamily( mFormatFamily );
            desc.SampleDesc.Count   = 1u;
            desc.Usage              = D3D11_USAGE_STAGING;
            desc.BindFlags          = 0;
            desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags          = 0;

            ID3D11Texture2D *texture = 0;
            hr = mDevice->CreateTexture2D( &desc, 0, &texture );
            mStagingTexture = texture;
            mIsArray2DTexture = true;

            mSubresourceData.resize( mDepthOrSlices );
            mLastSubresourceData.resize( mDepthOrSlices );
            for( size_t i=0; i<mDepthOrSlices; ++i )
            {
                memset( &mSubresourceData[i], 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
                memset( &mLastSubresourceData[i], 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
            }
        }

        if( FAILED(hr) || mDevice.isError() )
        {
            SAFE_RELEASE( mStagingTexture );
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Error creating StagingTexture\nError Description:" + errorDescription,
                            "D3D11StagingTexture::D3D11StagingTexture" );
        }
    }
    //-----------------------------------------------------------------------------------
    D3D11StagingTexture::~D3D11StagingTexture()
    {
        if( mMapRegionStarted )
        {
            LogManager::getSingleton().logMessage(
                        "WARNING: D3D11StagingTexture being deleted did not "
                        "call stopMapRegion. Calling it for you.", LML_CRITICAL );
            stopMapRegion();
        }

        SAFE_RELEASE( mStagingTexture );
    }
    //-----------------------------------------------------------------------------------
    bool D3D11StagingTexture::supportsFormat( uint32 width, uint32 height, uint32 depth, uint32 slices,
                                              PixelFormatGpu pixelFormat ) const
    {
        if( mFormatFamily != PixelFormatGpuUtils::getFamily( pixelFormat ) )
            return false;

        if( width > mWidth )
            return false;
        if( height > mHeight )
            return false;

        const uint32 depthOrSlices = std::max( depth, slices );
        if( depthOrSlices > mDepthOrSlices )
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    bool D3D11StagingTexture::isSmallerThan( const StagingTexture *other ) const
    {
        const D3D11StagingTexture *otherD3d = static_cast<const D3D11StagingTexture*>( other );
        size_t thisSize = PixelFormatGpuUtils::getSizeBytes( this->mWidth, this->mHeight,
                                                             this->mDepthOrSlices, 1u,
                                                             this->mFormatFamily, 4u );
        size_t otherSize= PixelFormatGpuUtils::getSizeBytes( otherD3d->mWidth, otherD3d->mHeight,
                                                             otherD3d->mDepthOrSlices, 1u,
                                                             otherD3d->mFormatFamily, 4u );
        return thisSize < otherSize;
    }
    //-----------------------------------------------------------------------------------
    size_t D3D11StagingTexture::_getSizeBytes(void)
    {
        return PixelFormatGpuUtils::getSizeBytes( this->mWidth, this->mHeight,
                                                  this->mDepthOrSlices, 1u,
                                                  this->mFormatFamily, 4u );
    }
    //-----------------------------------------------------------------------------------
    bool D3D11StagingTexture::belongsToUs( const TextureBox &box )
    {
        if( box.numSlices > 1u && mIsArray2DTexture )
            return false;

        const size_t sliceIdx = mIsArray2DTexture ? box.sliceStart : 0;

        return  box.x + box.width <= mWidth &&
                box.y + box.height <= mHeight &&
                box.getZOrSlice() + box.getDepthOrSlices() <= mDepthOrSlices &&
                box.data >= mLastSubresourceData[sliceIdx].pData &&
                box.data < (static_cast<uint8*>( mLastSubresourceData[sliceIdx].pData ) +
                            mLastSubresourceData[sliceIdx].DepthPitch * mDepthOrSlices);
    }
    //-----------------------------------------------------------------------------------
    void D3D11StagingTexture::shrinkRecords( size_t slice, StagingBoxVec::iterator record,
                                             const TextureBox &consumedBox )
    {
        if( record->width == consumedBox.width && record->height == consumedBox.height )
        {
            //Whole record was consumed. Easy case.
            efficientVectorRemove( mFreeBoxes[slice], record );
            return;
        }

        if( consumedBox.width <= record->width >> 1u &&
            consumedBox.height <= record->height >> 1u )
        {
            //If what was consumed is too small, partition the records in 4 fragments,
            //which should maximize our free space (assuming most textures are powers of 2).
            record->width >>= 1u;
            record->height>>= 1u;

            StagingBox newRecord[3];
            newRecord[0] = *record;
            newRecord[1] = *record;
            newRecord[2] = *record;

            newRecord[0].x += newRecord[0].width;
            newRecord[1].y += newRecord[1].height;
            newRecord[2].x += newRecord[2].width;
            newRecord[2].y += newRecord[2].height;

            const size_t idx = record - mFreeBoxes[slice].begin();
            mFreeBoxes[slice].push_back( newRecord[0] );
            mFreeBoxes[slice].push_back( newRecord[1] );
            mFreeBoxes[slice].push_back( newRecord[2] );
            record = mFreeBoxes[slice].begin() + idx;
        }

        //We need to split the record into 3 parts (2 parts are free, 1 is consumed).
        //To do that, we'll overwrite the existing record, and create one more.
        // If this is the record (origin is at upper left corner in this graph):
        //  -----------------
        //  |               |
        //  |               |
        //  |               |
        //  |               |
        //  |-              |
        //  |R|             |
        //  -----------------
        //Then we slice it like this:
        //  -----------------
        //  | |             |
        //  | |             |
        //  |0|      1      |
        //  | |             |
        //  |-|             |
        //  |R|             |
        //  -----------------

        // New slice #0
        StagingBox originalRecord = *record;
        record->width   = consumedBox.width;
        record->y       = consumedBox.height;
        record->height  = originalRecord.height - consumedBox.height;
        if( record->height == 0 )
            efficientVectorRemove( mFreeBoxes[slice], record );

        // New slice #1
        StagingBox newRecord = originalRecord;
        newRecord.x     = consumedBox.width;
        newRecord.width = originalRecord.width - consumedBox.width;
        if( newRecord.width > 0 )
            mFreeBoxes[slice].push_back( newRecord );
    }
    //-----------------------------------------------------------------------------------
    void D3D11StagingTexture::shrinkMultisliceRecords( size_t slice, StagingBoxVec::iterator record,
                                                       const TextureBox &consumedBox )
    {
        if( record->x == consumedBox.x && record->y == consumedBox.y )
        {
            //Trivial case. Consumed box is at the origin of the record.
            shrinkRecords( slice, record, consumedBox );
            return;
        }

        //The consumed box isn't at the origin of the record. It's somewhere in the middle.
        //Therefore we need to divide the record in 4, and leave a hole in the middle.
        //If this is the record (origin is at upper left corner in this graph):
        //  -----------------
        //  |               |
        //  |     -         |
        //  |    |R|        |
        //  |     -         |
        //  |               |
        //  |               |
        //  -----------------
        //Then we slice it like this:
        //  -----------------
        //  |       0       |
        //  |---------------|
        //  | 1  |R|   2    |
        //  |---------------|
        //  |       3       |
        //  |               |
        //  -----------------

        // New slice #0
        StagingBox originalRecord = *record;
        record->height = consumedBox.y;
        if( record->height == 0 )
            efficientVectorRemove( mFreeBoxes[slice], record );

        StagingBox newRecord;

        // New slice #1
        newRecord = originalRecord;
        newRecord.width     = consumedBox.x;
        newRecord.y         = consumedBox.y;
        newRecord.height    = consumedBox.height;
        if( newRecord.width > 0 )
            mFreeBoxes[slice].push_back( newRecord );

        // New slice #2
        newRecord = originalRecord;
        newRecord.x         = consumedBox.x + consumedBox.width;
        newRecord.width     = originalRecord.width - newRecord.x;
        newRecord.y         = consumedBox.y;
        newRecord.height    = consumedBox.height;
        if( newRecord.width > 0 )
            mFreeBoxes[slice].push_back( newRecord );

        // New slice #3
        newRecord = originalRecord;
        newRecord.y         = consumedBox.y + consumedBox.height;
        newRecord.height    = originalRecord.height - newRecord.y;
        if( newRecord.height > 0 )
            mFreeBoxes[slice].push_back( newRecord );
    }
    //-----------------------------------------------------------------------------------
    TextureBox D3D11StagingTexture::mapMultipleSlices( uint32 width, uint32 height, uint32 depth,
                                                       uint32 slices, PixelFormatGpu pixelFormat )
    {
        //By definition, we can't map multiple slices if this is an array texture
        assert( !mIsArray2DTexture );

        const uint32 depthOrSlices = std::max( depth, slices );

        vector<StagingBoxVec::iterator>::type selectedRecords;
        selectedRecords.reserve( depthOrSlices ); //One record per slice

        //Walk backwards, to start with less fragmented slices first.
        const size_t numSlices = mFreeBoxes.size();
        for( size_t slice=numSlices; slice--; )
        {
            StagingBoxVec &freeBoxesInSlice = mFreeBoxes[slice];

            StagingBoxVec::iterator itor = freeBoxesInSlice.begin();
            StagingBoxVec::iterator end  = freeBoxesInSlice.end();

            while( itor != end )
            {
                //There are too many possible candidates. Just grab any free region, and see if
                //this region is also available in the consecutive slices (backwards or forwards).
                //This algorithm may not find any candidate even if it is still possible.
                //An exhaustive search would be too expensive.
                if( width <= itor->width &&
                    height <= itor->height )
                {
                    size_t numBackwardSlices = 0;
                    size_t numForwardSlices = 0;

                    size_t lastNumSlices = 1u;

                    //Now check how many slices backwards & forwards can also
                    //contain this 3D region at the same location.
                    //Walk backwards
                    for( size_t extraSlice=slice;
                         extraSlice-- && lastNumSlices != numBackwardSlices &&
                         (numBackwardSlices + numForwardSlices) < depthOrSlices - 1u; )
                    {
                        lastNumSlices = numBackwardSlices;

                        StagingBoxVec &freeBoxesInExtraSlice = mFreeBoxes[extraSlice];
                        StagingBoxVec::iterator it = freeBoxesInExtraSlice.begin();
                        StagingBoxVec::iterator en = freeBoxesInExtraSlice.end();

                        while( it != en )
                        {
                            if( it->contains( itor->x, itor->y, width, height ) )
                            {
                                selectedRecords.push_back( it );
                                ++numBackwardSlices;
                                break;
                            }
                            ++it;
                        }
                    }

                    selectedRecords.push_back( itor );
                    lastNumSlices = 1u;

                    //Walk forward
                    for( size_t extraSlice=slice + 1u;
                         extraSlice<numSlices && lastNumSlices != numForwardSlices &&
                         (numBackwardSlices + numForwardSlices) < depthOrSlices - 1u;
                         ++extraSlice )
                    {
                        lastNumSlices = numForwardSlices;

                        StagingBoxVec &freeBoxesInExtraSlice = mFreeBoxes[extraSlice];
                        StagingBoxVec::iterator it = freeBoxesInExtraSlice.begin();
                        StagingBoxVec::iterator en = freeBoxesInExtraSlice.end();

                        while( it != en )
                        {
                            if( it->contains( itor->x, itor->y, width, height ) )
                            {
                                selectedRecords.push_back( it );
                                ++numForwardSlices;
                                break;
                            }
                            ++it;
                        }
                    }

                    if( (numBackwardSlices + numForwardSlices) < depthOrSlices - 1u )
                    {
                        //Found it!
                        TextureBox retVal( width, height, depth, slices,
                                           PixelFormatGpuUtils::getBytesPerPixel( pixelFormat ),
                                           mSubresourceData[0].RowPitch,
                                           mSubresourceData[0].DepthPitch );
                        retVal.data = reinterpret_cast<uint8*>( mSubresourceData[0].pData ) +
                                      mSubresourceData[0].DepthPitch * (slice - numBackwardSlices);

                        for( size_t i=0; i<depthOrSlices; ++i )
                        {
                            const size_t currentSliceIdx = i + slice - numBackwardSlices;
                            shrinkMultisliceRecords( currentSliceIdx, selectedRecords[i], retVal );
                        }

                        return retVal; //We're done!
                    }

                    //If we couldn't find it, then we need to be prepared for the next iteration.
                    selectedRecords.clear();
                }

                ++itor;
            }
        }

        return TextureBox();
    }
    //-----------------------------------------------------------------------------------
    TextureBox D3D11StagingTexture::mapRegionImpl( uint32 width, uint32 height, uint32 depth,
                                                   uint32 slices, PixelFormatGpu pixelFormat )
    {
        if( depth > 1u || slices > 1u )
            return mapMultipleSlices( width, height, depth, slices, pixelFormat );

        size_t bestMatchSlice = 0;
        StagingBoxVec::iterator bestMatch = mFreeBoxes[0].end();

        const size_t numSlices = mFreeBoxes.size();
        for( size_t slice=0; slice<numSlices; ++slice )
        {
            StagingBoxVec &freeBoxesInSlice = mFreeBoxes[slice];

            StagingBoxVec::iterator itor = freeBoxesInSlice.begin();
            StagingBoxVec::iterator end  = freeBoxesInSlice.end();

            while( itor != end )
            {
                //Grab the smallest fragment that can hold the request,
                //to avoid hurtful fragmentation.
                if( width <= itor->width &&
                    height <= itor->height )
                {
                    if( bestMatch == mFreeBoxes[bestMatchSlice].end() ||
                        itor->width < bestMatch->width ||
                        itor->height < bestMatch->height )
                    {
                        bestMatchSlice = slice;
                        bestMatch = itor;
                    }
                }

                ++itor;
            }
        }

        const size_t sliceIdx = mIsArray2DTexture ? bestMatchSlice : 0;

        TextureBox retVal( width, height, depth, slices,
                           PixelFormatGpuUtils::getBytesPerPixel( pixelFormat ),
                           mSubresourceData[sliceIdx].RowPitch,
                           mSubresourceData[sliceIdx].DepthPitch );
        if( bestMatch != mFreeBoxes[0].end() )
        {
            retVal.x = bestMatch->x;
            retVal.y = bestMatch->y;
            retVal.data = reinterpret_cast<uint8*>( mSubresourceData[sliceIdx].pData ) +
                          mSubresourceData[sliceIdx].DepthPitch * bestMatchSlice;

            //Now shrink our records.
            shrinkRecords( bestMatchSlice, bestMatch, retVal );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void D3D11StagingTexture::startMapRegion(void)
    {
        StagingTexture::startMapRegion();

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        const uint32 numSlices = mIsArray2DTexture ? mDepthOrSlices : 1u;
        for( uint32 i=0; i<numSlices; ++i )
        {
            const UINT subresourceIdx = D3D11CalcSubresource( 0, i, 1u );
            HRESULT hr = context->Map( mStagingTexture, subresourceIdx,
                                       D3D11_MAP_WRITE, 0, &mSubresourceData[i] );
            mLastSubresourceData[i] = mSubresourceData[i];

            if( FAILED(hr) || mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Error mapping StagingTexture\nError Description:" + errorDescription,
                                "D3D11StagingTexture::startMapRegion" );
            }
        }

        mFreeBoxes.reserve( mDepthOrSlices );
        for( size_t i=0; i<mDepthOrSlices; ++i )
        {
            mFreeBoxes.push_back( StagingBoxVec() );
            StagingBoxVec &freeBoxesInSlice = mFreeBoxes.back();
            freeBoxesInSlice.push_back( StagingBox( 0, 0, mWidth, mHeight ) );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11StagingTexture::stopMapRegion(void)
    {
        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        const uint32 numSlices = mIsArray2DTexture ? mDepthOrSlices : 1u;
        for( uint32 i=0; i<numSlices; ++i )
        {
            const UINT subresourceIdx = D3D11CalcSubresource( 0, i, 1u );
            context->Unmap( mStagingTexture, subresourceIdx );
            memset( &mSubresourceData[i], 0, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
        }
        mFreeBoxes.clear();

        StagingTexture::stopMapRegion();
    }
    //-----------------------------------------------------------------------------------
    void D3D11StagingTexture::upload( const TextureBox &srcBox, TextureGpu *dstTexture,
                                      uint8 mipLevel, const TextureBox *dstBox, bool skipSysRamCopy )
    {
        StagingTexture::upload( srcBox, dstTexture, mipLevel, dstBox, skipSysRamCopy );

        D3D11_BOX srcBoxD3d;

        srcBoxD3d.left  = srcBox.x;
        srcBoxD3d.top   = srcBox.y;
        srcBoxD3d.front = srcBox.getZOrSlice();
        srcBoxD3d.right = srcBox.x + srcBox.width;
        srcBoxD3d.bottom= srcBox.y + srcBox.height;
        //We copy one slice at a time, so add only srcBox.depth;
        //which is either 1u, or more than 1u for 3D textures.
        srcBoxD3d.back  = srcBox.getZOrSlice() + srcBox.depth;
        UINT srcSlicePos = srcBox.sliceStart;

        const UINT xPos = static_cast<UINT>( dstBox ? dstBox->x : 0 );
        const UINT yPos = static_cast<UINT>( dstBox ? dstBox->y : 0 );
        const UINT zPos = static_cast<UINT>( dstBox ? dstBox->getZOrSlice() : 0 );
        UINT dstSlicePos = static_cast<UINT>( dstBox ? dstBox->sliceStart : 0 );

        assert( dynamic_cast<D3D11TextureGpu*>( dstTexture ) );
        D3D11TextureGpu *dstTextureD3d = static_cast<D3D11TextureGpu*>( dstTexture );

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();

        for( UINT slice=0; slice<(UINT)srcBox.numSlices; ++slice )
        {
            const UINT srcSubResourceIdx = mIsArray2DTexture ?
                        D3D11CalcSubresource( 0, srcSlicePos, 1u ) : 0;
            const UINT dstSubResourceIdx = D3D11CalcSubresource( mipLevel, dstSlicePos,
                                                                 dstTexture->getNumMipmaps() );

            context->CopySubresourceRegion( dstTextureD3d->getFinalTextureName(), dstSubResourceIdx,
                                            xPos, yPos, zPos, mStagingTexture,
                                            srcSubResourceIdx, &srcBoxD3d );
            ++srcSlicePos;
            ++dstSlicePos;
            if( !mIsArray2DTexture )
            {
                ++srcBoxD3d.front;
                ++srcBoxD3d.back;
            }
        }

        if( mDevice.isError() )
        {
            String errorDescription = mDevice.getErrorDescription();
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Error after calling CopySubresourceRegion\n"
                         "Error Description:" + errorDescription,
                         "D3D11StagingTexture::upload" );
        }
    }
}
