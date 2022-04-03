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
#include "OgreStableHeaders.h"

#include "OgreHardwareIndexBuffer.h"

#include "OgreDefaultHardwareBufferManager.h"
#include "OgreHardwareBufferManager.h"

namespace Ogre
{
    namespace v1
    {
        //-----------------------------------------------------------------------------
        HardwareIndexBuffer::HardwareIndexBuffer( HardwareBufferManagerBase *mgr, IndexType idxType,
                                                  size_t numIndexes, HardwareBuffer::Usage usage,
                                                  bool useSystemMemory, bool useShadowBuffer ) :
            HardwareBuffer( usage, useSystemMemory, useShadowBuffer ),
            mMgr( mgr ),
            mIndexType( idxType ),
            mNumIndexes( numIndexes )
        {
            // Calculate the size of the indexes
            switch( mIndexType )
            {
            case IT_16BIT:
                mIndexSize = sizeof( unsigned short );
                break;
            case IT_32BIT:
                mIndexSize = sizeof( unsigned int );
                break;
            }
            mSizeInBytes = mIndexSize * mNumIndexes;

            // Create a shadow buffer if required
            if( mUseShadowBuffer )
            {
                mShadowBuffer = OGRE_NEW DefaultHardwareIndexBuffer( mIndexType, mNumIndexes,
                                                                     HardwareBuffer::HBU_DYNAMIC );
            }
        }
        //-----------------------------------------------------------------------------
        HardwareIndexBuffer::~HardwareIndexBuffer()
        {
            if( mMgr )
            {
                mMgr->_notifyIndexBufferDestroyed( this );
            }

            OGRE_DELETE mShadowBuffer;
        }
        //-----------------------------------------------------------------------------
        HardwareIndexBufferSharedPtr::HardwareIndexBufferSharedPtr( HardwareIndexBuffer *buf ) :
            SharedPtr<HardwareIndexBuffer>( buf )
        {
        }
    }  // namespace v1
}  // namespace Ogre
