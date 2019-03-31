
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

#include "Vao/OgreVertexBufferDownloadHelper.h"
#include "Vao/OgreVertexArrayObject.h"

namespace Ogre
{
    VertexBufferDownloadHelper::DownloadData::DownloadData() :
        origElements( 0 ),
        bufferIdx( 0 ),
        srcOffset( 0 ),
        srcBytesPerVertex( 0 )
    {
    }
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    void VertexBufferDownloadHelper::queueDownload(
            const VertexArrayObject *vao,
            const VertexElementSemanticFullArray &semanticsToDownload,
            size_t elementStart, size_t elementCount )
    {
        const size_t numSemantics = semanticsToDownload.size();
        mDownloadData.resize( numSemantics );

        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

        for( size_t i=0; i<numSemantics; ++i )
        {
            mDownloadData[i].origElements = vao->findBySemantic( semanticsToDownload[i].semantic,
                                                                 mDownloadData[i].bufferIdx,
                                                                 mDownloadData[i].srcOffset,
                                                                 semanticsToDownload[i].repeat );
            if( mDownloadData[i].origElements )
            {
                mDownloadData[i].srcBytesPerVertex =
                        vertexBuffers[mDownloadData[i].bufferIdx]->getBytesPerElement();
            }
        }

        if( elementCount == 0 )
            elementCount = vertexBuffers[0]->getNumElements() - elementStart;

        //Pack all the requests together
        for( size_t i=0; i<numSemantics; ++i )
        {
            bool sameBuffer = false;
            for( size_t j=0; j<i && !sameBuffer; ++j )
                sameBuffer = mDownloadData[j].bufferIdx == mDownloadData[i].bufferIdx;

            if( !sameBuffer && mDownloadData[i].origElements )
            {
                mDownloadData[i].ticket =
                        vertexBuffers[mDownloadData[i].bufferIdx]->readRequest( elementStart,
                                                                                elementCount );
            }
        }
    }
    //-------------------------------------------------------------------------
    void VertexBufferDownloadHelper::map( uint8 const **outDataPtrs )
    {
        const size_t numSemantics = mDownloadData.size();

        //Pack all the maps together
        for( size_t i=0; i<numSemantics; ++i )
        {
            outDataPtrs[i] = 0;

            if( mDownloadData[i].ticket.isNull() )
            {
                for( size_t j=0; j<i; ++j )
                {
                    if( mDownloadData[i].bufferIdx == mDownloadData[j].bufferIdx )
                    {
                        outDataPtrs[i] = outDataPtrs[j];
                        break;
                    }
                }
            }
            else
            {
                outDataPtrs[i] = reinterpret_cast<const uint8*>( mDownloadData[i].ticket->map() );
            }
        }
    }
    //-------------------------------------------------------------------------
    void VertexBufferDownloadHelper::unmap()
    {
        const size_t numSemantics = mDownloadData.size();
        for( size_t i=0; i<numSemantics; ++i )
        {
            if( !mDownloadData[i].ticket.isNull() )
            {
                mDownloadData[i].ticket->unmap();
                mDownloadData[i].ticket.reset();
            }
        }
    }
}
