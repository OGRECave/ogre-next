/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "Vao/OgreUavBufferPacked.h"

#include "Vao/OgreReadOnlyBufferPacked.h"
#include "Vao/OgreTexBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    UavBufferPacked::UavBufferPacked( size_t internalBufferStartBytes, size_t numElements,
                                      uint32 bytesPerElement, uint32 bindFlags, void *initialData,
                                      bool keepAsShadow, VaoManager *vaoManager,
                                      BufferInterface *bufferInterface ) :
        BufferPacked( internalBufferStartBytes, numElements, bytesPerElement, 0, BT_DEFAULT, initialData,
                      keepAsShadow, vaoManager, bufferInterface ),
        mBindFlags( bindFlags )
    {
    }
    //-----------------------------------------------------------------------------------
    UavBufferPacked::~UavBufferPacked() { destroyAllBufferViews(); }
    //-----------------------------------------------------------------------------------
    TexBufferPacked *UavBufferPacked::getAsTexBufferView( PixelFormatGpu pixelFormat )
    {
        OGRE_ASSERT_LOW( mBindFlags & BB_FLAG_TEX && "Buffer must've been created with BB_FLAG_TEX" );

        TexBufferPacked *retVal = 0;
        vector<TexBufferPacked *>::type::const_iterator itor = mTexBufferViews.begin();
        vector<TexBufferPacked *>::type::const_iterator endt = mTexBufferViews.end();
        while( itor != endt && !retVal )
        {
            if( ( *itor )->getPixelFormat() == pixelFormat &&
                ( *itor )->getBufferPackedType() == BP_TYPE_TEX )
            {
                retVal = *itor;
            }
            ++itor;
        }

        if( !retVal )
        {
            retVal = getAsTexBufferImpl( pixelFormat );
            mTexBufferViews.push_back( retVal );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void UavBufferPacked::destroyTexBufferView( PixelFormatGpu pixelFormat )
    {
        OGRE_ASSERT_LOW( pixelFormat != PFG_NULL && "PixelFormat cannot be PFG_NULL" );

        vector<TexBufferPacked *>::type::iterator itor = mTexBufferViews.begin();
        vector<TexBufferPacked *>::type::iterator endt = mTexBufferViews.end();

        while( itor != endt && ( ( *itor )->getPixelFormat() != pixelFormat ||
                                 ( *itor )->getBufferPackedType() != BP_TYPE_TEX ) )
        {
            ++itor;
        }

        if( itor != endt )
        {
            ( *itor )->_setBufferInterface( (BufferInterface *)0 );
            delete *itor;
            efficientVectorRemove( mTexBufferViews, itor );
        }

    }
    //-----------------------------------------------------------------------------------
    ReadOnlyBufferPacked *UavBufferPacked::getAsReadOnlyBufferView( void )
    {
        OGRE_ASSERT_LOW( mBindFlags & BB_FLAG_READONLY &&
                         "Buffer must've been created with BB_FLAG_READONLY" );

        ReadOnlyBufferPacked *retVal = 0;

        if( !mTexBufferViews.empty() &&
            mTexBufferViews.front()->getBufferPackedType() == BP_TYPE_READONLY )
        {
            // ReadOnlyBufferPacked is always kept at the front
            OGRE_ASSERT_HIGH( dynamic_cast<ReadOnlyBufferPacked *>( mTexBufferViews.front() ) );
            retVal = static_cast<ReadOnlyBufferPacked *>( mTexBufferViews.front() );
        }
        else
        {
            retVal = getAsReadOnlyBufferImpl();
            // Keep always at front
            mTexBufferViews.insert( mTexBufferViews.begin(), retVal );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void UavBufferPacked::destroyReadOnlyBufferView( void )
    {
        if( !mTexBufferViews.empty() &&
            mTexBufferViews.front()->getBufferPackedType() == BP_TYPE_READONLY )
        {
            // ReadOnlyBufferPacked is always kept at the front
            vector<TexBufferPacked *>::type::iterator toDelete = mTexBufferViews.begin();
            ( *toDelete )->_setBufferInterface( (BufferInterface *)0 );
            delete *toDelete;
            efficientVectorRemove( mTexBufferViews, toDelete );
        }
    }
    //-----------------------------------------------------------------------------------
    void UavBufferPacked::destroyAllBufferViews( void )
    {
        vector<TexBufferPacked *>::type::const_iterator itor = mTexBufferViews.begin();
        vector<TexBufferPacked *>::type::const_iterator endt = mTexBufferViews.end();

        while( itor != endt )
        {
            ( *itor )->_setBufferInterface( (BufferInterface *)0 );
            delete *itor;
            ++itor;
        }

        mTexBufferViews.clear();
    }
}  // namespace Ogre
