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

#include "OgreHardwareVertexBuffer.h"

#include "OgreColourValue.h"
#include "OgreDefaultHardwareBufferManager.h"
#include "OgreException.h"
#include "OgreHardwareBufferManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"

namespace Ogre
{
    namespace v1
    {
        //-----------------------------------------------------------------------------
        HardwareVertexBuffer::HardwareVertexBuffer( HardwareBufferManagerBase *mgr, size_t vertexSize,
                                                    size_t numVertices, HardwareBuffer::Usage usage,
                                                    bool useSystemMemory, bool useShadowBuffer ) :
            HardwareBuffer( usage, useSystemMemory, useShadowBuffer ),
            mMgr( mgr ),
            mNumVertices( numVertices ),
            mVertexSize( vertexSize ),
            mIsInstanceData( false ),
            mInstanceDataStepRate( 1 )
        {
            // Calculate the size of the vertices
            mSizeInBytes = mVertexSize * numVertices;

            // Create a shadow buffer if required
            if( mUseShadowBuffer )
            {
                mShadowBuffer = OGRE_NEW DefaultHardwareVertexBuffer( mMgr, mVertexSize, mNumVertices,
                                                                      HardwareBuffer::HBU_DYNAMIC );
            }
        }
        //-----------------------------------------------------------------------------
        HardwareVertexBuffer::~HardwareVertexBuffer()
        {
            if( mMgr )
            {
                mMgr->_notifyVertexBufferDestroyed( this );
            }
            OGRE_DELETE mShadowBuffer;
        }
        //-----------------------------------------------------------------------------
        bool HardwareVertexBuffer::checkIfVertexInstanceDataIsSupported()
        {
            // Use the current render system
            RenderSystem *rs = Root::getSingleton().getRenderSystem();

            // Check if the supported
            return rs->getCapabilities()->hasCapability( RSC_VERTEX_BUFFER_INSTANCE_DATA );
        }
        //-----------------------------------------------------------------------------
        void HardwareVertexBuffer::setIsInstanceData( const bool val )
        {
            if( val && !checkIfVertexInstanceDataIsSupported() )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "vertex instance data is not supported by the render system.",
                             "HardwareVertexBuffer::checkIfInstanceDataSupported" );
            }
            else
            {
                mIsInstanceData = val;
            }
        }
        //-----------------------------------------------------------------------------
        size_t HardwareVertexBuffer::getInstanceDataStepRate() const { return mInstanceDataStepRate; }
        //-----------------------------------------------------------------------------
        void HardwareVertexBuffer::setInstanceDataStepRate( const size_t val )
        {
            if( val > 0 )
            {
                mInstanceDataStepRate = val;
            }
            else
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Instance data step rate must be bigger then 0.",
                             "HardwareVertexBuffer::setInstanceDataStepRate" );
            }
        }
        //-----------------------------------------------------------------------------
        // VertexElement
        //-----------------------------------------------------------------------------
        VertexElement::VertexElement( unsigned short source, size_t offset, VertexElementType theType,
                                      VertexElementSemantic semantic, unsigned short index ) :
            mSource( source ),
            mOffset( offset ),
            mType( theType ),
            mSemantic( semantic ),
            mIndex( index )
        {
        }
        //-----------------------------------------------------------------------------
        uint32 VertexElement::getSize() const { return getTypeSize( mType ); }
        //-----------------------------------------------------------------------------
        uint32 VertexElement::getTypeSize( VertexElementType etype )
        {
            switch( etype )
            {
            case VET_COLOUR:
            case VET_COLOUR_ABGR:
            case VET_COLOUR_ARGB:
                return sizeof( RGBA );
            case VET_FLOAT1:
                return sizeof( float );
            case VET_FLOAT2:
                return sizeof( float ) * 2;
            case VET_FLOAT3:
                return sizeof( float ) * 3;
            case VET_FLOAT4:
                return sizeof( float ) * 4;
            case VET_DOUBLE1:
                return sizeof( double );
            case VET_DOUBLE2:
                return sizeof( double ) * 2;
            case VET_DOUBLE3:
                return sizeof( double ) * 3;
            case VET_DOUBLE4:
                return sizeof( double ) * 4;
            case VET_SHORT2:
            case VET_SHORT2_SNORM:
                return sizeof( short ) * 2;
            case VET_SHORT4:
            case VET_SHORT4_SNORM:
                return sizeof( short ) * 4;
            case VET_USHORT2:
            case VET_USHORT2_NORM:
                return sizeof( unsigned short ) * 2;
            case VET_USHORT4:
            case VET_USHORT4_NORM:
                return sizeof( unsigned short ) * 4;
            case VET_INT1:
                return sizeof( int );
            case VET_INT2:
                return sizeof( int ) * 2;
            case VET_INT3:
                return sizeof( int ) * 3;
            case VET_INT4:
                return sizeof( int ) * 4;
            case VET_UINT1:
                return sizeof( unsigned int );
            case VET_UINT2:
                return sizeof( unsigned int ) * 2;
            case VET_UINT3:
                return sizeof( unsigned int ) * 3;
            case VET_UINT4:
                return sizeof( unsigned int ) * 4;
            case VET_UBYTE4:
            case VET_UBYTE4_NORM:
                return sizeof( unsigned char ) * 4;
            case VET_BYTE4:
            case VET_BYTE4_SNORM:
                return sizeof( char ) * 4;
            case VET_HALF2:
                return sizeof( float );
            case VET_HALF4:
                return sizeof( float ) * 2;

            case VET_USHORT1_DEPRECATED:
                return sizeof( unsigned short );
            case VET_USHORT3_DEPRECATED:
                return sizeof( unsigned short ) * 3;
            }
            return 0;
        }
        //-----------------------------------------------------------------------------
        uint8 VertexElement::getTypeCount( VertexElementType etype )
        {
            switch( etype )
            {
            case VET_COLOUR:
            case VET_COLOUR_ABGR:
            case VET_COLOUR_ARGB:
            case VET_FLOAT1:
            case VET_UINT1:
            case VET_INT1:
            case VET_DOUBLE1:
            case VET_USHORT1_DEPRECATED:
                return 1;
            case VET_FLOAT2:
            case VET_SHORT2:
            case VET_USHORT2:
            case VET_UINT2:
            case VET_INT2:
            case VET_DOUBLE2:
            case VET_HALF2:
            case VET_SHORT2_SNORM:
            case VET_USHORT2_NORM:
                return 2;
            case VET_FLOAT3:
            case VET_UINT3:
            case VET_INT3:
            case VET_DOUBLE3:
            case VET_USHORT3_DEPRECATED:
                return 3;
            case VET_FLOAT4:
            case VET_SHORT4:
            case VET_USHORT4:
            case VET_UINT4:
            case VET_INT4:
            case VET_DOUBLE4:
            case VET_UBYTE4:
            case VET_HALF4:
            case VET_BYTE4:
            case VET_BYTE4_SNORM:
            case VET_UBYTE4_NORM:
            case VET_SHORT4_SNORM:
            case VET_USHORT4_NORM:
                return 4;
            }
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid type", "VertexElement::getTypeCount" );
        }
        //-----------------------------------------------------------------------------
        bool VertexElement::isTypeNormalized( VertexElementType etype )
        {
            VertexElementType baseType = getBaseType( etype );
            switch( baseType )
            {
            case VET_SHORT2_SNORM:
            case VET_USHORT2_NORM:
            case VET_BYTE4_SNORM:
            case VET_UBYTE4_NORM:
                return true;
            default:
                return false;
            }

            return false;
        }
        //-----------------------------------------------------------------------------
        VertexElementType VertexElement::multiplyTypeCount( VertexElementType baseType,
                                                            unsigned short count )
        {
            switch( baseType )
            {
            case VET_FLOAT1:
                switch( count )
                {
                case 1:
                    return VET_FLOAT1;
                case 2:
                    return VET_FLOAT2;
                case 3:
                    return VET_FLOAT3;
                case 4:
                    return VET_FLOAT4;
                default:
                    break;
                }
                break;
            case VET_SHORT2:
                switch( count )
                {
                case 1:
                case 2:
                    return VET_SHORT2;
                case 3:
                case 4:
                    return VET_SHORT4;
                default:
                    break;
                }
                break;
            case VET_HALF2:
                switch( count )
                {
                case 1:
                case 2:
                    return VET_HALF2;
                case 3:
                case 4:
                    return VET_HALF4;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid base type",
                         "VertexElement::multiplyTypeCount" );
        }
        //--------------------------------------------------------------------------
        VertexElementType VertexElement::getBestColourVertexElementType()
        {
            // Use the current render system to determine if possible
            if( Root::getSingletonPtr() && Root::getSingletonPtr()->getRenderSystem() )
            {
                return Root::getSingleton().getRenderSystem()->getColourVertexElementType();
            }
            else
            {
                // We can't know the specific type right now, so pick a type
                // based on platform
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
                return VET_COLOUR_ARGB;  // prefer D3D format on windows
#else
                return VET_COLOUR_ABGR;  // prefer GL format on everything else
#endif
            }
        }
        //--------------------------------------------------------------------------
        void VertexElement::convertColourValue( VertexElementType srcType, VertexElementType dstType,
                                                uint32 *ptr )
        {
            if( srcType == dstType )
                return;

            // Conversion between ARGB and ABGR is always a case of flipping R/B
            *ptr = ( ( *ptr & 0x00FF0000 ) >> 16 ) | ( ( *ptr & 0x000000FF ) << 16 ) |
                   ( *ptr & 0xFF00FF00 );
        }
        //--------------------------------------------------------------------------
        uint32 VertexElement::convertColourValue( const ColourValue &src, VertexElementType dst )
        {
            switch( dst )
            {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
            default:
#endif
            case VET_COLOUR_ARGB:
                return src.getAsARGB();
#if OGRE_PLATFORM != OGRE_PLATFORM_WIN32 && OGRE_PLATFORM != OGRE_PLATFORM_WINRT
            default:
#endif
            case VET_COLOUR_ABGR:
                return src.getAsABGR();
            };
        }
        //-----------------------------------------------------------------------------
        VertexElementType VertexElement::getBaseType( VertexElementType multiType )
        {
            switch( multiType )
            {
            case VET_FLOAT1:
            case VET_FLOAT2:
            case VET_FLOAT3:
            case VET_FLOAT4:
                return VET_FLOAT1;
            case VET_DOUBLE1:
            case VET_DOUBLE2:
            case VET_DOUBLE3:
            case VET_DOUBLE4:
                return VET_DOUBLE1;
            case VET_INT1:
            case VET_INT2:
            case VET_INT3:
            case VET_INT4:
                return VET_INT1;
            case VET_UINT1:
            case VET_UINT2:
            case VET_UINT3:
            case VET_UINT4:
                return VET_UINT1;
            case VET_COLOUR:
                return VET_COLOUR;
            case VET_COLOUR_ABGR:
                return VET_COLOUR_ABGR;
            case VET_COLOUR_ARGB:
                return VET_COLOUR_ARGB;
            case VET_SHORT2:
            case VET_SHORT4:
                return VET_SHORT2;
            case VET_USHORT1_DEPRECATED:
            case VET_USHORT3_DEPRECATED:
                return VET_USHORT1_DEPRECATED;
            case VET_USHORT2:
            case VET_USHORT4:
                return VET_USHORT2;
            case VET_HALF2:
            case VET_HALF4:
                return VET_HALF2;
            case VET_UBYTE4:
                return VET_UBYTE4;
            case VET_BYTE4:
                return VET_BYTE4;
            case VET_BYTE4_SNORM:
                return VET_BYTE4_SNORM;
            case VET_UBYTE4_NORM:
                return VET_UBYTE4_NORM;
            case VET_SHORT2_SNORM:
            case VET_SHORT4_SNORM:
                return VET_SHORT2_SNORM;
            case VET_USHORT2_NORM:
            case VET_USHORT4_NORM:
                return VET_USHORT2_NORM;
            };
            // To keep compiler happy
            return VET_FLOAT1;
        }
        //-----------------------------------------------------------------------------
        VertexDeclaration::VertexDeclaration( HardwareBufferManagerBase *creator ) :
            mCreator( creator ),
            mBaseInputLayoutId( std::numeric_limits<uint16>::max() ),
            mInputLayoutDirty( false )
        {
            vertexLayoutDirty();
        }
        //-----------------------------------------------------------------------------
        VertexDeclaration::~VertexDeclaration() {}
        //-----------------------------------------------------------------------------
        void VertexDeclaration::vertexLayoutDirty()
        {
            if( !mInputLayoutDirty )
                mInputLayoutDirty = true;
        }
        //-----------------------------------------------------------------------------
        uint16 VertexDeclaration::_getInputLayoutId( HlmsManager *hlmsManager, OperationType opType )
        {
            if( mInputLayoutDirty )
            {
                VertexElement2VecVec vertexElements = convertToV2();
                mBaseInputLayoutId =
                    hlmsManager->_getInputLayoutId( vertexElements, opType ) & ( ( 1u << 10u ) - 1u );
                mInputLayoutDirty = false;
            }

            return uint16( mBaseInputLayoutId | ( uint16( opType ) << 10u ) );
        }
        //-----------------------------------------------------------------------------
        const VertexDeclaration::VertexElementList &VertexDeclaration::getElements() const
        {
            return mElementList;
        }
        //-----------------------------------------------------------------------------
        const VertexElement &VertexDeclaration::addElement( unsigned short source, size_t offset,
                                                            VertexElementType theType,
                                                            VertexElementSemantic semantic,
                                                            unsigned short index )
        {
            vertexLayoutDirty();
            // Refine colour type to a specific type
            if( theType == VET_COLOUR )
            {
                theType = VertexElement::getBestColourVertexElementType();
            }
            mElementList.push_back( VertexElement( source, offset, theType, semantic, index ) );
            return mElementList.back();
        }
        //-----------------------------------------------------------------------------
        const VertexElement &VertexDeclaration::insertElement( unsigned short atPosition,
                                                               unsigned short source, size_t offset,
                                                               VertexElementType theType,
                                                               VertexElementSemantic semantic,
                                                               unsigned short index )
        {
            vertexLayoutDirty();
            if( atPosition >= mElementList.size() )
            {
                return addElement( source, offset, theType, semantic, index );
            }

            VertexElementList::iterator i = mElementList.begin();
            for( unsigned short n = 0; n < atPosition; ++n )
                ++i;

            i = mElementList.insert( i, VertexElement( source, offset, theType, semantic, index ) );
            return *i;
        }
        //-----------------------------------------------------------------------------
        const VertexElement *VertexDeclaration::getElement( unsigned short index ) const
        {
            assert( index < mElementList.size() && "Index out of bounds" );

            VertexElementList::const_iterator i = mElementList.begin();
            for( unsigned short n = 0; n < index; ++n )
                ++i;

            return &( *i );
        }
        //-----------------------------------------------------------------------------
        void VertexDeclaration::removeElement( unsigned short elem_index )
        {
            vertexLayoutDirty();

            assert( elem_index < mElementList.size() && "Index out of bounds" );
            VertexElementList::iterator i = mElementList.begin();
            for( unsigned short n = 0; n < elem_index; ++n )
                ++i;
            mElementList.erase( i );
        }
        //-----------------------------------------------------------------------------
        void VertexDeclaration::removeElement( VertexElementSemantic semantic, unsigned short index )
        {
            vertexLayoutDirty();

            VertexElementList::iterator ei, eiend;
            eiend = mElementList.end();
            for( ei = mElementList.begin(); ei != eiend; ++ei )
            {
                if( ei->getSemantic() == semantic && ei->getIndex() == index )
                {
                    mElementList.erase( ei );
                    break;
                }
            }
        }
        //-----------------------------------------------------------------------------
        void VertexDeclaration::removeAllElements()
        {
            vertexLayoutDirty();
            mElementList.clear();
        }
        //-----------------------------------------------------------------------------
        void VertexDeclaration::modifyElement( unsigned short elem_index, unsigned short source,
                                               size_t offset, VertexElementType theType,
                                               VertexElementSemantic semantic, unsigned short index )
        {
            vertexLayoutDirty();
            assert( elem_index < mElementList.size() && "Index out of bounds" );
            VertexElementList::iterator i = mElementList.begin();
            std::advance( i, elem_index );
            ( *i ) = VertexElement( source, offset, theType, semantic, index );
        }
        //-----------------------------------------------------------------------------
        const VertexElement *VertexDeclaration::findElementBySemantic( VertexElementSemantic sem,
                                                                       unsigned short index ) const
        {
            VertexElementList::const_iterator ei, eiend;
            eiend = mElementList.end();
            for( ei = mElementList.begin(); ei != eiend; ++ei )
            {
                if( ei->getSemantic() == sem && ei->getIndex() == index )
                {
                    return &( *ei );
                }
            }

            return NULL;
        }
        //-----------------------------------------------------------------------------
        VertexDeclaration::VertexElementList VertexDeclaration::findElementsBySource(
            unsigned short source ) const
        {
            VertexElementList retList;
            VertexElementList::const_iterator ei, eiend;
            eiend = mElementList.end();
            for( ei = mElementList.begin(); ei != eiend; ++ei )
            {
                if( ei->getSource() == source )
                {
                    retList.push_back( *ei );
                }
            }
            return retList;
        }

        //-----------------------------------------------------------------------------
        size_t VertexDeclaration::getVertexSize( unsigned short source ) const
        {
            VertexElementList::const_iterator i, iend;
            iend = mElementList.end();
            size_t sz = 0;

            for( i = mElementList.begin(); i != iend; ++i )
            {
                if( i->getSource() == source )
                {
                    sz += i->getSize();
                }
            }
            return sz;
        }
        //-----------------------------------------------------------------------------
        VertexDeclaration *VertexDeclaration::clone( HardwareBufferManagerBase *mgr ) const
        {
            HardwareBufferManagerBase *pManager = mgr ? mgr : HardwareBufferManager::getSingletonPtr();
            VertexDeclaration *ret = pManager->createVertexDeclaration();

            VertexElementList::const_iterator i, iend;
            iend = mElementList.end();
            for( i = mElementList.begin(); i != iend; ++i )
            {
                ret->addElement( i->getSource(), i->getOffset(), i->getType(), i->getSemantic(),
                                 i->getIndex() );
            }
            return ret;
        }
        //-----------------------------------------------------------------------------------
        bool VertexDeclaration::isSortedForV2() const
        {
            bool retVal = true;

            if( !mElementList.empty() )
            {
                VertexElementList::const_iterator currElement = mElementList.begin();
                VertexElementList::const_iterator nextElement = mElementList.begin();
                VertexElementList::const_iterator endt = mElementList.end();

                ++nextElement;

                while( nextElement != endt &&
                       VertexDeclaration::vertexElementLessForV2( *currElement, *nextElement ) )
                {
                    ++currElement;
                    ++nextElement;
                }

                retVal = nextElement == endt;
            }

            return retVal;
        }
        //-----------------------------------------------------------------------------------
        VertexElement2VecVec VertexDeclaration::convertToV2()
        {
            VertexElement2VecVec retVal;
            retVal.resize( getMaxSource() + 1 );

            if( !isSortedForV2() )
            {
                // Make sure the declaration is sorted. Don't sort if already sorted, otherwise
                // the declaration will be tagged as dirty when creating the PSO.
                sortForV2();
            }

            VertexElementList::const_iterator itor = mElementList.begin();
            VertexElementList::const_iterator endt = mElementList.end();

            while( itor != endt )
            {
                retVal[itor->getSource()].push_back(
                    VertexElement2( itor->getType(), itor->getSemantic() ) );
                ++itor;
            }

            return retVal;
        }
        //-----------------------------------------------------------------------------------
        void VertexDeclaration::convertFromV2( const VertexElement2Vec &v2Decl )
        {
            // Recreate the vertex declaration
            removeAllElements();

            size_t acumOffset = 0;
            unsigned short repeatCounts[VES_COUNT];
            memset( repeatCounts, 0, sizeof( repeatCounts ) );

            VertexElement2Vec::const_iterator itor = v2Decl.begin();
            VertexElement2Vec::const_iterator endt = v2Decl.end();

            while( itor != endt )
            {
                addElement( 0, acumOffset, itor->mType, itor->mSemantic,
                            repeatCounts[itor->mSemantic - 1]++ );
                acumOffset += v1::VertexElement::getTypeSize( itor->mType );
                ++itor;
            }
        }
        //-----------------------------------------------------------------------------------
        void VertexDeclaration::convertFromV2( const VertexElement2VecVec &v2Decl )
        {
            // Recreate the vertex declaration
            removeAllElements();

            size_t acumOffset = 0;
            unsigned short repeatCounts[VES_COUNT];
            memset( repeatCounts, 0, sizeof( repeatCounts ) );

            for( size_t i = 0; i < v2Decl.size(); ++i )
            {
                VertexElement2Vec::const_iterator itor = v2Decl[i].begin();
                VertexElement2Vec::const_iterator endt = v2Decl[i].end();

                while( itor != endt )
                {
                    addElement( (uint16)i, acumOffset, itor->mType, itor->mSemantic,
                                repeatCounts[itor->mSemantic - 1]++ );
                    acumOffset += v1::VertexElement::getTypeSize( itor->mType );
                    ++itor;
                }
            }
        }
        //-----------------------------------------------------------------------------
        // Sort routine for VertexElement
        bool VertexDeclaration::vertexElementLess( const VertexElement &e1, const VertexElement &e2 )
        {
            // Sort by source first
            if( e1.getSource() < e2.getSource() )
            {
                return true;
            }
            else if( e1.getSource() == e2.getSource() )
            {
                // Use ordering of semantics to sort
                if( e1.getSemantic() < e2.getSemantic() )
                {
                    return true;
                }
                else if( e1.getSemantic() == e2.getSemantic() )
                {
                    // Use index to sort
                    if( e1.getIndex() < e2.getIndex() )
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        void VertexDeclaration::sort()
        {
            vertexLayoutDirty();
            mElementList.sort( VertexDeclaration::vertexElementLess );
        }
        //-----------------------------------------------------------------------------
        // Sort routine for VertexElement
        bool VertexDeclaration::vertexElementLessForV2( const VertexElement &e1,
                                                        const VertexElement &e2 )
        {
            // Sort by source first, then by offset
            if( e1.getSource() != e2.getSource() )
                return e1.getSource() < e2.getSource();
            else
                return e1.getOffset() < e2.getOffset();
        }
        void VertexDeclaration::sortForV2()
        {
            vertexLayoutDirty();
            mElementList.sort( VertexDeclaration::vertexElementLessForV2 );
        }
        //-----------------------------------------------------------------------------
        void VertexDeclaration::closeGapsInSource()
        {
            if( mElementList.empty() )
                return;

            // Sort first
            sort();

            VertexElementList::iterator i, iend;
            iend = mElementList.end();
            unsigned short targetIdx = 0;
            unsigned short lastIdx = getElement( 0 )->getSource();
            unsigned short c = 0;
            for( i = mElementList.begin(); i != iend; ++i, ++c )
            {
                VertexElement &elem = *i;
                if( lastIdx != elem.getSource() )
                {
                    targetIdx++;
                    lastIdx = elem.getSource();
                }
                if( targetIdx != elem.getSource() )
                {
                    modifyElement( c, targetIdx, elem.getOffset(), elem.getType(), elem.getSemantic(),
                                   elem.getIndex() );
                }
            }
        }
        //-----------------------------------------------------------------------
        VertexDeclaration *VertexDeclaration::getAutoOrganisedDeclaration(
            bool skeletalAnimation, bool vertexAnimation, bool vertexAnimationNormals ) const
        {
            VertexDeclaration *newDecl = this->clone();
            // Set all sources to the same buffer (for now)
            const VertexDeclaration::VertexElementList &elems = newDecl->getElements();
            VertexDeclaration::VertexElementList::const_iterator i;
            unsigned short c = 0;
            for( i = elems.begin(); i != elems.end(); ++i, ++c )
            {
                const VertexElement &elem = *i;
                // Set source & offset to 0 for now, before sort
                newDecl->modifyElement( c, 0, 0, elem.getType(), elem.getSemantic(), elem.getIndex() );
            }
            newDecl->sort();
            // Now sort out proper buffer assignments and offsets
            size_t offset = 0;
            c = 0;
            unsigned short buffer = 0;
            VertexElementSemantic prevSemantic = VES_POSITION;
            for( i = elems.begin(); i != elems.end(); ++i, ++c )
            {
                const VertexElement &elem = *i;

                bool splitWithPrev = false;
                bool splitWithNext = false;
                switch( elem.getSemantic() )
                {
                case VES_POSITION:
                    // Split positions if vertex animated with only positions
                    // group with normals otherwise
                    splitWithPrev = false;
                    splitWithNext = vertexAnimation && !vertexAnimationNormals;
                    break;
                case VES_NORMAL:
                    // Normals can't share with blend weights/indices
                    splitWithPrev =
                        ( prevSemantic == VES_BLEND_WEIGHTS || prevSemantic == VES_BLEND_INDICES );
                    // All animated meshes have to split after normal
                    splitWithNext =
                        ( skeletalAnimation || ( vertexAnimation && vertexAnimationNormals ) );
                    break;
                case VES_BLEND_WEIGHTS:
                    // Blend weights/indices can be sharing with their own buffer only
                    splitWithPrev = true;
                    break;
                case VES_BLEND_INDICES:
                    // Blend weights/indices can be sharing with their own buffer only
                    splitWithNext = true;
                    break;
                default:
                case VES_DIFFUSE:
                case VES_SPECULAR:
                case VES_TEXTURE_COORDINATES:
                case VES_BINORMAL:
                case VES_TANGENT:
                    // Make sure position is separate if animated & there were no normals
                    splitWithPrev =
                        prevSemantic == VES_POSITION && ( skeletalAnimation || vertexAnimation );
                    break;
                }

                if( splitWithPrev && offset )
                {
                    ++buffer;
                    offset = 0;
                }

                prevSemantic = elem.getSemantic();
                newDecl->modifyElement( c, buffer, offset, elem.getType(), elem.getSemantic(),
                                        elem.getIndex() );

                if( splitWithNext )
                {
                    ++buffer;
                    offset = 0;
                }
                else
                {
                    offset += elem.getSize();
                }
            }

            return newDecl;
        }
        //-----------------------------------------------------------------------------
        unsigned short VertexDeclaration::getMaxSource() const
        {
            VertexElementList::const_iterator i, iend;
            iend = mElementList.end();
            unsigned short ret = 0;
            for( i = mElementList.begin(); i != iend; ++i )
            {
                if( i->getSource() > ret )
                {
                    ret = i->getSource();
                }
            }
            return ret;
        }
        //-----------------------------------------------------------------------------
        unsigned short VertexDeclaration::getNextFreeTextureCoordinate() const
        {
            unsigned short texCoord = 0;
            for( VertexElementList::const_iterator i = mElementList.begin(); i != mElementList.end();
                 ++i )
            {
                const VertexElement &el = *i;
                if( el.getSemantic() == VES_TEXTURE_COORDINATES )
                {
                    ++texCoord;
                }
            }
            return texCoord;
        }
        //-----------------------------------------------------------------------------
        VertexBufferBinding::VertexBufferBinding() : mHighIndex( 0 ) {}
        //-----------------------------------------------------------------------------
        VertexBufferBinding::~VertexBufferBinding() { unsetAllBindings(); }
        //-----------------------------------------------------------------------------
        void VertexBufferBinding::setBinding( unsigned short index,
                                              const HardwareVertexBufferSharedPtr &buffer )
        {
            // NB will replace any existing buffer ptr at this index, and will thus cause
            // reference count to decrement on that buffer (possibly destroying it)
            mBindingMap[index] = buffer;
            mHighIndex = std::max( mHighIndex, (unsigned short)( index + 1 ) );
        }
        //-----------------------------------------------------------------------------
        void VertexBufferBinding::unsetBinding( unsigned short index )
        {
            VertexBufferBindingMap::iterator i = mBindingMap.find( index );
            if( i == mBindingMap.end() )
            {
                OGRE_EXCEPT(
                    Exception::ERR_ITEM_NOT_FOUND,
                    "Cannot find buffer binding for index " + StringConverter::toString( index ),
                    "VertexBufferBinding::unsetBinding" );
            }
            mBindingMap.erase( i );
        }
        //-----------------------------------------------------------------------------
        void VertexBufferBinding::unsetAllBindings()
        {
            mBindingMap.clear();
            mHighIndex = 0;
        }
        //-----------------------------------------------------------------------------
        const VertexBufferBinding::VertexBufferBindingMap &VertexBufferBinding::getBindings() const
        {
            return mBindingMap;
        }
        //-----------------------------------------------------------------------------
        const HardwareVertexBufferSharedPtr &VertexBufferBinding::getBuffer( unsigned short index ) const
        {
            VertexBufferBindingMap::const_iterator i = mBindingMap.find( index );
            if( i == mBindingMap.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "No buffer is bound to that index.",
                             "VertexBufferBinding::getBuffer" );
            }
            return i->second;
        }
        //-----------------------------------------------------------------------------
        bool VertexBufferBinding::isBufferBound( unsigned short index ) const
        {
            return mBindingMap.find( index ) != mBindingMap.end();
        }
        //-----------------------------------------------------------------------------
        unsigned short VertexBufferBinding::getLastBoundIndex() const
        {
            return mBindingMap.empty() ? 0 : mBindingMap.rbegin()->first + 1;
        }
        //-----------------------------------------------------------------------------
        bool VertexBufferBinding::hasGaps() const
        {
            if( mBindingMap.empty() )
                return false;
            if( mBindingMap.rbegin()->first + 1 == (int)mBindingMap.size() )
                return false;
            return true;
        }
        //-----------------------------------------------------------------------------
        void VertexBufferBinding::closeGaps( BindingIndexMap &bindingIndexMap )
        {
            bindingIndexMap.clear();

            VertexBufferBindingMap newBindingMap;

            VertexBufferBindingMap::const_iterator it;
            ushort targetIndex = 0;
            for( it = mBindingMap.begin(); it != mBindingMap.end(); ++it, ++targetIndex )
            {
                bindingIndexMap[it->first] = targetIndex;
                newBindingMap[targetIndex] = it->second;
            }

            mBindingMap.swap( newBindingMap );
            mHighIndex = targetIndex;
        }
        //-----------------------------------------------------------------------------
        bool VertexBufferBinding::getHasInstanceData() const
        {
            VertexBufferBinding::VertexBufferBindingMap::const_iterator i, iend;
            iend = mBindingMap.end();
            for( i = mBindingMap.begin(); i != iend; ++i )
            {
                if( i->second->getIsInstanceData() )
                {
                    return true;
                }
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        HardwareVertexBufferSharedPtr::HardwareVertexBufferSharedPtr( HardwareVertexBuffer *buf ) :
            SharedPtr<HardwareVertexBuffer>( buf )
        {
        }
    }  // namespace v1
}  // namespace Ogre
