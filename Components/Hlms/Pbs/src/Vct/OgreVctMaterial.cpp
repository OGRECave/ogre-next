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

#include "OgreHlms.h"
#include "OgreHlmsPbsDatablock.h"

#include "OgreLogManager.h"

#define TODO_convert_unlit_datablock
#define TODO_free_buffers

namespace Ogre
{
    static const size_t c_numDatablocksPerConstBuffer = 1024u;

    struct ShaderVctMaterial
    {
        float bgDiffuse[4];
        float diffuse[4];
        float emissive[4];
        uint32 diffuseTexIdx;
        uint32 emissiveTexIdx;
        uint32 padding01[2];
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

        if( datablock->getCreator()->getType() == HLMS_PBS )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsPbsDatablock*>( datablock ) );
            HlmsPbsDatablock *pbsDatablock = static_cast<HlmsPbsDatablock*>( datablock );

            ColourValue bgDiffuse = pbsDatablock->getBackgroundDiffuse();
            Vector3 diffuse = pbsDatablock->getDiffuse();
            float transparency = pbsDatablock->getTransparency();
            Vector3 emissive = pbsDatablock->getEmissive();

            for( size_t i=0; i<4; ++i )
                shaderMaterial.bgDiffuse[i] = bgDiffuse[i];

            for( size_t i=0; i<3; ++i )
            {
                shaderMaterial.diffuse[i] = diffuse[i];
                shaderMaterial.emissive[i] = emissive[i];
            }
            shaderMaterial.diffuse[3] = transparency;
            shaderMaterial.emissive[3] = 1.0f;
        }
        else
        {
            //Default to white if we can't recognize the material
            for( size_t i=0; i<4; ++i )
                shaderMaterial.diffuse[i] = 1.0f;

            const String *datablockNamePtr = datablock->getNameStr();
            String datablockName = datablockNamePtr ? *datablockNamePtr :
                                                      datablock->getName().getFriendlyText();
            LogManager::getSingleton().logMessage(
                        "WARNING: VctMaterial::addDatablockToBucket could not recognize the "
                        "type of datablock '" + datablockName + "' using white instead." );
        }
        TODO_convert_unlit_datablock;

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
