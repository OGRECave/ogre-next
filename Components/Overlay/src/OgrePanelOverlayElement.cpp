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

#include "OgrePanelOverlayElement.h"

#include "OgreHardwareBufferManager.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreString.h"
#include "OgreStringConverter.h"
#include "OgreTechnique.h"

#ifdef OGRE_BUILD_COMPONENT_HLMS_UNLIT
#    include "OgreHlmsUnlitDatablock.h"
#else
#    include "OgreHlmsUnlitMobileDatablock.h"
#endif

namespace Ogre
{
    namespace v1
    {
        //---------------------------------------------------------------------
        String PanelOverlayElement::msTypeName = "Panel";
        PanelOverlayElement::CmdTiling PanelOverlayElement::msCmdTiling;
        PanelOverlayElement::CmdTransparent PanelOverlayElement::msCmdTransparent;
        PanelOverlayElement::CmdUVCoords PanelOverlayElement::msCmdUVCoords;
//---------------------------------------------------------------------
// vertex buffer bindings, set at compile time (we could look these up but no point)
#define POSITION_BINDING 0
#define TEXCOORD_BINDING 1

        //---------------------------------------------------------------------
        PanelOverlayElement::PanelOverlayElement( const String &name ) :
            OverlayContainer( name ),
            mTransparent( false )
            // Defer creation of texcoord buffer until we know how big it needs to be
            ,
            mNumTexCoordsInBuffer( 0 ),
            mU1( 0.0 ),
            mV1( 0.0 ),
            mU2( 1.0 ),
            mV2( 1.0 )

        {
            // Init tiling
            for( ushort i = 0; i < OGRE_MAX_TEXTURE_COORD_SETS; ++i )
            {
                mTileX[i] = 1.0f;
                mTileY[i] = 1.0f;
            }

            // No normals or colours
            if( createParamDictionary( "PanelOverlayElement" ) )
            {
                addBaseParameters();
            }
        }
        //---------------------------------------------------------------------
        PanelOverlayElement::~PanelOverlayElement() { OGRE_DELETE mRenderOp.vertexData; }
        //---------------------------------------------------------------------
        void PanelOverlayElement::initialise()
        {
            bool init = !mInitialised;

            OverlayContainer::initialise();
            if( init )
            {
                // Setup render op in advance
                mRenderOp.vertexData = OGRE_NEW VertexData( NULL );
                // Vertex declaration: 1 position, add texcoords later depending on #layers
                // Create as separate buffers so we can lock & discard separately
                VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
                decl->addElement( POSITION_BINDING, 0, VET_FLOAT3, VES_POSITION );

                // Basic vertex data
                mRenderOp.vertexData->vertexStart = 0;
                mRenderOp.vertexData->vertexCount = 4;
                // No indexes & issue as a strip
                mRenderOp.useIndexes = false;
                mRenderOp.operationType = OT_TRIANGLE_STRIP;
                mRenderOp.useGlobalInstancingVertexBufferIsAvailable = false;

                mInitialised = true;

                _restoreManualHardwareResources();
            }
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::_restoreManualHardwareResources()
        {
            if( !mInitialised )
                return;

            VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
            HardwareBufferManagerBase *mgr = mRenderOp.vertexData->_getHardwareBufferManager();

            // Vertex buffer #1
            HardwareVertexBufferSharedPtr vbuf = mgr->createVertexBuffer(
                decl->getVertexSize( POSITION_BINDING ), mRenderOp.vertexData->vertexCount,
                HardwareBuffer::HBU_STATIC_WRITE_ONLY  // mostly static except during resizing
            );
            // Bind buffer
            mRenderOp.vertexData->vertexBufferBinding->setBinding( POSITION_BINDING, vbuf );

            // Buffers are restored, but with trash within
            mGeomPositionsOutOfDate = true;
            mGeomUVsOutOfDate = true;
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::_releaseManualHardwareResources()
        {
            if( !mInitialised )
                return;

            VertexBufferBinding *bind = mRenderOp.vertexData->vertexBufferBinding;
            bind->unsetBinding( POSITION_BINDING );

            // Remove all texcoord element declarations
            if( mNumTexCoordsInBuffer > 0 )
            {
                bind->unsetBinding( TEXCOORD_BINDING );

                VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
                for( size_t i = mNumTexCoordsInBuffer; i > 0; --i )
                {
                    decl->removeElement( VES_TEXTURE_COORDINATES, static_cast<unsigned short>( i - 1 ) );
                }
                mNumTexCoordsInBuffer = 0;
            }
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::setTiling( Real x, Real y, ushort layer )
        {
            assert( layer < OGRE_MAX_TEXTURE_COORD_SETS );
            assert( x != 0 && y != 0 );

            mTileX[layer] = x;
            mTileY[layer] = y;

            mGeomUVsOutOfDate = true;
        }
        //---------------------------------------------------------------------
        Real PanelOverlayElement::getTileX( ushort layer ) const { return mTileX[layer]; }
        //---------------------------------------------------------------------
        Real PanelOverlayElement::getTileY( ushort layer ) const { return mTileY[layer]; }
        //---------------------------------------------------------------------
        void PanelOverlayElement::setTransparent( bool inTransparent ) { mTransparent = inTransparent; }
        //---------------------------------------------------------------------
        bool PanelOverlayElement::isTransparent() const { return mTransparent; }
        //---------------------------------------------------------------------
        void PanelOverlayElement::setUV( Real u1, Real v1, Real u2, Real v2 )
        {
            mU1 = u1;
            mU2 = u2;
            mV1 = v1;
            mV2 = v2;
            mGeomUVsOutOfDate = true;
        }
        void PanelOverlayElement::getUV( Real &u1, Real &v1, Real &u2, Real &v2 ) const
        {
            u1 = mU1;
            u2 = mU2;
            v1 = mV1;
            v2 = mV2;
        }
        //---------------------------------------------------------------------
        const String &PanelOverlayElement::getTypeName() const { return msTypeName; }
        //---------------------------------------------------------------------
        void PanelOverlayElement::getRenderOperation( RenderOperation &op, bool casterPass )
        {
            op = mRenderOp;
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::setMaterialName( const String &matName )
        {
            OverlayContainer::setMaterialName( matName );
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::_updateRenderQueue( RenderQueue *queue, Camera *camera,
                                                      const Camera *lodCamera )
        {
            if( mVisible )
            {
                if( !mTransparent && !mMaterialName.empty() )
                {
                    OverlayElement::_updateRenderQueue( queue, camera, lodCamera );
                }

                // Also add children
                ChildIterator it = getChildIterator();
                while( it.hasMoreElements() )
                {
                    // Give children ZOrder 1 higher than this
                    it.getNext()->_updateRenderQueue( queue, camera, lodCamera );
                }
            }
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::updatePositionGeometry()
        {
            /*
                0-----2
                |    /|
                |  /  |
                |/    |
                1-----3
            */
            Real left, right, top, bottom;

            /* Convert positions into -1, 1 coordinate space (homogenous clip space).
                - Left / right is simple range conversion
                - Top / bottom also need inverting since y is upside down - this means
                that top will end up greater than bottom and when computing texture
                coordinates, we have to flip the v-axis (ie. subtract the value from
                1.0 to get the actual correct value).
            */
            left = _getDerivedLeft() * 2 - 1;
            right = left + ( mWidth * 2 );
            top = -( ( _getDerivedTop() * 2 ) - 1 );
            bottom = top - ( mHeight * 2 );

            HardwareVertexBufferSharedPtr vbuf =
                mRenderOp.vertexData->vertexBufferBinding->getBuffer( POSITION_BINDING );
            HardwareBufferLockGuard vbufLock( vbuf, HardwareBuffer::HBL_DISCARD );
            float *pPos = static_cast<float *>( vbufLock.pData );

            // Use the furthest away depth value, since materials should have depth-check off
            // This initialised the depth buffer for any 3D objects in front
            Real zValue = Root::getSingleton().getRenderSystem()->getMaximumDepthInputValue();
            *pPos++ = left;
            *pPos++ = top;
            *pPos++ = zValue;

            *pPos++ = left;
            *pPos++ = bottom;
            *pPos++ = zValue;

            *pPos++ = right;
            *pPos++ = top;
            *pPos++ = zValue;

            *pPos++ = right;
            *pPos++ = bottom;
            *pPos++ = zValue;
        }
        //---------------------------------------------------------------------
        void PanelOverlayElement::updateTextureGeometry()
        {
            // Generate for as many texture layers as there are in material
            if( !mMaterialName.empty() && mInitialised )
            {
#ifdef OGRE_BUILD_COMPONENT_HLMS_UNLIT
                uint8 numLayers = 1;  // TODO?
#else
                HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
                Hlms *hlms = hlmsManager->getHlms( HLMS_UNLIT );
                HlmsDatablock *datablock = hlms->getDatablock( mMaterialName );
                assert( dynamic_cast<OverlayUnlitDatablock *>( datablock ) );
                OverlayUnlitDatablock *guiDatablock = static_cast<OverlayUnlitDatablock *>( datablock );
                uint8 numLayers = guiDatablock->getNumUvSets();
#endif
                VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
                // Check the number of texcoords we have in our buffer now
                if( mNumTexCoordsInBuffer > numLayers )
                {
                    // remove extras
                    for( size_t i = mNumTexCoordsInBuffer; i > numLayers; --i )
                    {
                        decl->removeElement( VES_TEXTURE_COORDINATES,
                                             static_cast<unsigned short>( i - 1 ) );
                    }
                }
                else if( mNumTexCoordsInBuffer < numLayers )
                {
                    // Add extra texcoord elements
                    size_t offset = VertexElement::getTypeSize( VET_FLOAT2 ) * mNumTexCoordsInBuffer;
                    for( size_t i = mNumTexCoordsInBuffer; i < numLayers; ++i )
                    {
                        decl->addElement( TEXCOORD_BINDING, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES,
                                          static_cast<unsigned short>( i ) );
                        offset += VertexElement::getTypeSize( VET_FLOAT2 );
                    }
                }

                // if number of layers changed at all, we'll need to reallocate buffer
                if( mNumTexCoordsInBuffer != numLayers )
                {
                    // NB reference counting will take care of the old one if it exists
                    HardwareVertexBufferSharedPtr newbuf =
                        mRenderOp.vertexData->_getHardwareBufferManager()->createVertexBuffer(
                            decl->getVertexSize( TEXCOORD_BINDING ), mRenderOp.vertexData->vertexCount,
                            HardwareBuffer::HBU_STATIC_WRITE_ONLY  // mostly static except during
                                                                   // resizing
                        );
                    // Bind buffer, note this will unbind the old one and destroy the buffer it had
                    mRenderOp.vertexData->vertexBufferBinding->setBinding( TEXCOORD_BINDING, newbuf );
                    // Set num tex coords in use now
                    mNumTexCoordsInBuffer = numLayers;
                }

                // Get the tcoord buffer & lock
                if( mNumTexCoordsInBuffer )
                {
                    HardwareVertexBufferSharedPtr vbuf =
                        mRenderOp.vertexData->vertexBufferBinding->getBuffer( TEXCOORD_BINDING );
                    HardwareBufferLockGuard vbufLock( vbuf, HardwareBuffer::HBL_DISCARD );
                    float *pVBStart = static_cast<float *>( vbufLock.pData );

                    size_t uvSize = VertexElement::getTypeSize( VET_FLOAT2 ) / sizeof( float );
                    size_t vertexSize = decl->getVertexSize( TEXCOORD_BINDING ) / sizeof( float );
                    for( ushort i = 0; i < numLayers; ++i )
                    {
                        // Calc upper tex coords
                        Real upperX = mU2 * mTileX[i];
                        Real upperY = mV2 * mTileY[i];

                        /*
                            0-----2
                            |    /|
                            |  /  |
                            |/    |
                            1-----3
                        */
                        // Find start offset for this set
                        float *pTex = pVBStart + ( i * uvSize );

                        pTex[0] = mU1;
                        pTex[1] = mV1;

                        pTex += vertexSize;  // jump by 1 vertex stride
                        pTex[0] = mU1;
                        pTex[1] = upperY;

                        pTex += vertexSize;
                        pTex[0] = upperX;
                        pTex[1] = mV1;

                        pTex += vertexSize;
                        pTex[0] = upperX;
                        pTex[1] = upperY;
                    }
                }
            }
        }
        //-----------------------------------------------------------------------
        void PanelOverlayElement::addBaseParameters()
        {
            OverlayContainer::addBaseParameters();
            ParamDictionary *dict = getParamDictionary();

            dict->addParameter(
                ParameterDef( "uv_coords",
                              "The texture coordinates for the texture. 1 set of uv values.",
                              PT_STRING ),
                &msCmdUVCoords );

            dict->addParameter(
                ParameterDef( "tiling", "The number of times to repeat the background texture.",
                              PT_STRING ),
                &msCmdTiling );

            dict->addParameter(
                ParameterDef( "transparent",
                              "Sets whether the panel is transparent, i.e. invisible itself "
                              "but it's contents are still displayed.",
                              PT_BOOL ),
                &msCmdTransparent );
        }
        //-----------------------------------------------------------------------
        // Command objects
        //-----------------------------------------------------------------------
        String PanelOverlayElement::CmdTiling::doGet( const void *target ) const
        {
            // NB only returns 1st layer tiling
            String ret = "0 " + StringConverter::toString(
                                    static_cast<const PanelOverlayElement *>( target )->getTileX() );
            ret += " " + StringConverter::toString(
                             static_cast<const PanelOverlayElement *>( target )->getTileY() );
            return ret;
        }
        void PanelOverlayElement::CmdTiling::doSet( void *target, const String &val )
        {
            // 3 params: <layer> <x_tile> <y_tile>
            // Param count is validated higher up
            vector<String>::type vec = StringUtil::split( val );
            ushort layer = (ushort)StringConverter::parseUnsignedInt( vec[0] );
            Real x_tile = StringConverter::parseReal( vec[1] );
            Real y_tile = StringConverter::parseReal( vec[2] );

            static_cast<PanelOverlayElement *>( target )->setTiling( x_tile, y_tile, layer );
        }
        //-----------------------------------------------------------------------
        String PanelOverlayElement::CmdTransparent::doGet( const void *target ) const
        {
            return StringConverter::toString(
                static_cast<const PanelOverlayElement *>( target )->isTransparent() );
        }
        void PanelOverlayElement::CmdTransparent::doSet( void *target, const String &val )
        {
            static_cast<PanelOverlayElement *>( target )->setTransparent(
                StringConverter::parseBool( val ) );
        }
        //-----------------------------------------------------------------------
        String PanelOverlayElement::CmdUVCoords::doGet( const void *target ) const
        {
            Real u1, v1, u2, v2;

            static_cast<const PanelOverlayElement *>( target )->getUV( u1, v1, u2, v2 );
            String ret = " " + StringConverter::toString( u1 ) + " " + StringConverter::toString( v1 ) +
                         " " + StringConverter::toString( u2 ) + " " + StringConverter::toString( v2 );

            return ret;
        }
        void PanelOverlayElement::CmdUVCoords::doSet( void *target, const String &val )
        {
            vector<String>::type vec = StringUtil::split( val );

            static_cast<PanelOverlayElement *>( target )->setUV(
                StringConverter::parseReal( vec[0] ), StringConverter::parseReal( vec[1] ),
                StringConverter::parseReal( vec[2] ), StringConverter::parseReal( vec[3] ) );
        }
    }  // namespace v1
}  // namespace Ogre
