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

#include "OgreStableHeaders.h"
#include "OgreGpuResource.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    GpuResource::GpuResource( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                              VaoManager *vaoManager, IdString name ) :
        mResidencyStatus( GpuResidency::OnStorage ),
        mNextResidencyStatus( GpuResidency::OnStorage ),
        mPageOutStrategy( pageOutStrategy ),
        mPendingResidencyChanges( 0 ),
        mRank( 1 ),
        mLastFrameUsed( vaoManager->getFrameCount() ),
        mLowestDistanceToCamera( 0 ),
        mVaoManager( vaoManager ),
        mName( name )
    {
    }
    //-----------------------------------------------------------------------------------
    GpuResource::~GpuResource()
    {
    }
    //-----------------------------------------------------------------------------------
    void GpuResource::_setNextResidencyStatus( GpuResidency::GpuResidency nextResidency )
    {
        mNextResidencyStatus = nextResidency;
    }
    //-----------------------------------------------------------------------------------
    GpuResidency::GpuResidency GpuResource::getResidencyStatus(void) const
    {
        return mResidencyStatus;
    }
    //-----------------------------------------------------------------------------------
    GpuResidency::GpuResidency GpuResource::getNextResidencyStatus(void) const
    {
        return mNextResidencyStatus;
    }
    //-----------------------------------------------------------------------------------
    GpuPageOutStrategy::GpuPageOutStrategy GpuResource::getGpuPageOutStrategy(void) const
    {
        return mPageOutStrategy;
    }
    //-----------------------------------------------------------------------------------
    void GpuResource::_addPendingResidencyChanges( uint32 value )
    {
        mPendingResidencyChanges += value;
    }
    //-----------------------------------------------------------------------------------
    uint32 GpuResource::getPendingResidencyChanges(void) const
    {
        return mPendingResidencyChanges;
    }
    //-----------------------------------------------------------------------------------
    IdString GpuResource::getName(void) const
    {
        return mName;
    }
    //-----------------------------------------------------------------------------------
    String GpuResource::getNameStr(void) const
    {
        //TODO: Get friendly name from manager which will be kept in a std::map
        return mName.getFriendlyText();
    }
}
