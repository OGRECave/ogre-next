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
#ifndef _OgreVertexBufferDownloadHelper_H_
#define _OgreVertexBufferDownloadHelper_H_

#include "OgrePrerequisites.h"
#include "OgreSharedPtr.h"
#include "Vao/OgreAsyncTicket.h"
#include "Vao/OgreVertexBufferPacked.h"
#include "OgreHardwareVertexBuffer.h"
#include "OgreBitwise.h"
#include "OgreVector3.h"
#include "OgreVector4.h"
#include "OgreHeaderPrefix.h"

namespace  Ogre
{
    struct VertexElementSemanticFull
    {
        VertexElementSemantic   semantic;
        uint8                   repeat;
        VertexElementSemanticFull( VertexElementSemantic _semantic, uint8 _repeat=0 ) :
            semantic( _semantic ), repeat( _repeat ) {}
    };

    typedef vector<VertexBufferPacked*>::type VertexBufferPackedVec;
    typedef FastArray<VertexElementSemanticFull> VertexElementSemanticFullArray;
    struct VertexElement2;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /**
    @class VertexBufferDownloadHelper
        Ogre currently does not support deinterleaved vertex buffers. However it may in the future.
        Retrieving vertex data out of one or multiple buffers can be tricky.

        This class helps with that issue.
    */
    class _OgreExport VertexBufferDownloadHelper
    {
    public:
        struct DownloadData
        {
            /// If this is nullptr, see origElements comment
            AsyncTicketPtr ticket;
            /// If this value is nullptr, then the requested
            /// element semantic was not present in the Vao
            VertexElement2 const *origElements;
            size_t bufferIdx;
            size_t srcOffset;
            size_t srcBytesPerVertex;

            DownloadData();
        };

    protected:
        typedef FastArray<DownloadData> DownloadDataArray;
        DownloadDataArray   mDownloadData;

    public:
        /** Creates AsyncTickets to download GPU -> CPU the requested VertexElementSemantics
            from one or multiple vertex buffers in a Vao.
        @param vao
            Vao to download from
        @param semanticsToDownload
            Semantics from the Vao to download
        @param elementStart
            Offset to start, in the vertex buffer.
        @param elementCount
            Number of vertices to download. When 0, we download all that remains.
        */
        void queueDownload( const VertexArrayObject *vao,
                            const VertexElementSemanticFullArray &semanticsToDownload,
                            size_t elementStart=0, size_t elementCount=0 );

        /** Maps the buffers that have been downloaded with queueDownload.
            and outputs the pointers in outDataPtrs.
        @remarks
            queueDownload must have been called prior to this.

            @see    VertexBufferDownloadHelper::queueDownload
            @see    VertexBufferDownloadHelper::unmap
            @see    VertexBufferDownloadHelper::getDownloadData
        @param outDataPtrs
            The vertex data that was requested. outDataPtrs must have enough size, which is
            the number of semantics that was requested to queueDownload
            (i.e. semanticsToDownload.size())

            None, some, or all elements in outDataPtrs[i] may alias each other, e.g.
            outDataPtrs[0] == outDataPtrs[1] is possible, and that just means that
            both vertex elements are present interleaved the same vertex buffer.

            If outDataPtrs[i] is a nullptr that means the semantic was not found in the Vao.
        */
        void map( uint8 const **outDataPtrs );
        void unmap();

        inline static Vector4 getVector4( uint8 const *srcData, VertexElement2 vertexElement )
        {
            float retVal[4];

            const size_t readSize = v1::VertexElement::getTypeSize( vertexElement.mType );

            const VertexElementType baseType = v1::VertexElement::getBaseType( vertexElement.mType );
            if( baseType == VET_HALF2 )
            {
                //Convert half to float.
                uint16 hfData[4];
                memcpy( hfData, srcData, readSize );

                const size_t typeCount = v1::VertexElement::getTypeCount( vertexElement.mType );

                for( size_t j=0; j<typeCount; ++j )
                    retVal[j] = Bitwise::halfToFloat( hfData[j] );
            }
            else
            {
                //Raw. Transfer as is.
                memcpy( retVal, srcData, readSize );
            }

            return Vector4( static_cast<Real>( retVal[0] ),
                            static_cast<Real>( retVal[1] ),
                            static_cast<Real>( retVal[2] ),
                            static_cast<Real>( retVal[3] ) );
        }

        inline static Vector3 getNormal( uint8 const *srcData, VertexElement2 vertexElement )
        {
            Vector3 retVal;

            const size_t readSize = v1::VertexElement::getTypeSize( vertexElement.mType );

            const VertexElementType baseType = v1::VertexElement::getBaseType( vertexElement.mType );
            if( baseType == VET_HALF2 )
            {
                retVal = getVector4( srcData, vertexElement ).xyz();
            }
            else if( vertexElement.mType == VET_SHORT4_SNORM )
            {
                //Dealing with QTangents.
                Quaternion qTangent;
                const int16 *srcData16 = reinterpret_cast<const int16*>( srcData );
                qTangent.x = Bitwise::snorm16ToFloat( srcData16[0] );
                qTangent.y = Bitwise::snorm16ToFloat( srcData16[1] );
                qTangent.z = Bitwise::snorm16ToFloat( srcData16[2] );
                qTangent.w = Bitwise::snorm16ToFloat( srcData16[3] );

                float reflection = 1.0f;
                if( qTangent.w < 0 )
                    reflection = -1.0f;

                retVal = qTangent.xAxis();
            }
            else
            {
                //Raw. Transfer as is.
                float tmpData[4];
                memcpy( tmpData, srcData, readSize );
                retVal = Vector3( static_cast<Real>( retVal[0] ),
                                  static_cast<Real>( retVal[1] ),
                                  static_cast<Real>( retVal[2] ) );
            }

            return retVal;
        }

        const DownloadDataArray& getDownloadData(void) const        { return mDownloadData; }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
