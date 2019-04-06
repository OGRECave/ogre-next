/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

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

#include "Vct/OgreVctMaterial.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreConstBufferPacked.h"

#define TODO_convert_datablock
#define TODO_free_buffers

namespace Ogre
{
    static const size_t c_numDatablocksPerConstBuffer = 1024u;

    struct ShaderVctMaterial
    {
        float diffuse[4];
        float emissive[4];
        uint32 diffuseTexIdx;
        uint32 emissiveTexIdx;
        uint32 padding01[2];
        uint32 padding2[4];
    };
    //-------------------------------------------------------------------------
    VctMaterial::VctMaterial( VaoManager *vaoManager ) :
        mVaoManager( vaoManager )
    {
    }
    //-------------------------------------------------------------------------
    VctMaterial::~VctMaterial()
    {
        TODO_free_buffers;
    }
    //-------------------------------------------------------------------------
    VctMaterial::DatablockConversionResult VctMaterial::addDatablockToBucket( HlmsDatablock *datablock,
                                                                              MaterialBucket &bucket )
    {
        const size_t usedSlots = bucket.datablocks.size();

        OGRE_ASSERT_MEDIUM( usedSlots < c_numDatablocksPerConstBuffer );

        ShaderVctMaterial shaderMaterial;
        memset( &shaderMaterial, 0, sizeof(shaderMaterial) );
        TODO_convert_datablock;

        bucket.buffer->upload( &shaderMaterial, usedSlots * sizeof( ShaderVctMaterial ),
                               sizeof( ShaderVctMaterial ) );

        bucket.datablocks.insert( datablock );

        DatablockConversionResult conversionResult;
        conversionResult.slotIdx        = static_cast<uint32>( usedSlots );
        conversionResult.constBuffer    = bucket.buffer;
        mDatablockConversionResults[datablock] = conversionResult;

        return conversionResult;
    }
    //-------------------------------------------------------------------------
    VctMaterial::DatablockConversionResult VctMaterial::addDatablock( HlmsDatablock *datablock )
    {
        DatablockConversionResult retVal;

        DatablockConversionResultMap::const_iterator itResult =
                mDatablockConversionResults.find( datablock );
        if( itResult != mDatablockConversionResults.end() )
            retVal = itResult->second;
        else
        {
            if( mBuckets.empty() || mBuckets.back().datablocks.size() >= c_numDatablocksPerConstBuffer )
            {
                //Create a new bucket
                MaterialBucket bucket;
                bucket.buffer = mVaoManager->createConstBuffer( 1024u * sizeof(ShaderVctMaterial),
                                                                BT_DEFAULT, 0, false );
                mBuckets.push_back( bucket );
            }

            retVal = addDatablockToBucket( datablock, mBuckets.back() );
        }

        return retVal;
    }
}
