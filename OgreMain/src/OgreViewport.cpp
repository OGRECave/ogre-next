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

#include "OgreViewport.h"

#include "OgreCamera.h"
#include "OgreLogManager.h"
#include "OgreMaterialManager.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"

#include <iomanip>

namespace Ogre
{
    //---------------------------------------------------------------------
    Viewport::Viewport( Real left, Real top, Real width, Real height ) :
        mGlobalIndex( std::numeric_limits<size_t>::max() ),
        mRelLeft( left ),
        mRelTop( top ),
        mRelWidth( width ),
        mRelHeight( height ),
        mActLeft( 0 ),
        mActTop( 0 ),
        mActWidth( 0 ),
        mActHeight( 0 ),
        mCurrentTarget( 0 ),
        mCurrentMip( 0 ),
        mScissorRelLeft( left ),
        mScissorRelTop( top ),
        mScissorRelWidth( width ),
        mScissorRelHeight( height ),
        mScissorActLeft( 0 ),
        mScissorActTop( 0 ),
        mScissorActWidth( 0 ),
        mScissorActHeight( 0 ),
        mCoversEntireTarget( true ),
        mScissorsMatchViewport( true )
        // Actual dimensions will update later
        ,
        mUpdated( false ),
        mShowOverlays( true ),
        mVisibilityMask( 0 ),
        mMaterialSchemeName( MaterialManager::DEFAULT_SCHEME_NAME ),
        mColourBuffer( CBT_BACK )
    {
        // Set the default material scheme
        // RenderSystem* rs = Root::getSingleton().getRenderSystem();
        // mMaterialSchemeName = rs->_getDefaultViewportMaterialScheme();

        // Calculate actual dimensions
        _updateDimensions();
    }
    //---------------------------------------------------------------------
    Viewport::Viewport() : Viewport( 0, 0, 0, 0 ) {}
    //---------------------------------------------------------------------
    Viewport::~Viewport() {}
    //---------------------------------------------------------------------
    bool Viewport::_isUpdated() const { return mUpdated; }
    //---------------------------------------------------------------------
    void Viewport::_clearUpdatedFlag() { mUpdated = false; }
    //---------------------------------------------------------------------
    void Viewport::_updateDimensions()
    {
        if( !mCurrentTarget )
        {
            mActLeft = 0;
            mActTop = 0;
            mActWidth = 0;
            mActHeight = 0;
            mScissorActLeft = 0;
            mScissorActTop = 0;
            mScissorActWidth = 0;
            mScissorActHeight = 0;

            mScissorsMatchViewport = true;
            mCoversEntireTarget = true;
            return;
        }
        Real height = (Real)( mCurrentTarget->getHeight() >> mCurrentMip );
        Real width = (Real)( mCurrentTarget->getWidth() >> mCurrentMip );

        assert( mScissorRelLeft >= mRelLeft && mScissorRelTop >= mRelTop &&
                mScissorRelWidth <= mRelWidth && mScissorRelHeight <= mRelHeight &&
                "Scissor rectangle must be inside Viewport's!" );

        mActLeft = (int)( mRelLeft * width );
        mActTop = (int)( mRelTop * height );
        mActWidth = (int)( mRelWidth * width );
        mActHeight = (int)( mRelHeight * height );

        mScissorActLeft = (int)( mScissorRelLeft * width );
        mScissorActTop = (int)( mScissorRelTop * height );
        mScissorActWidth = (int)( mScissorRelWidth * width );
        mScissorActHeight = (int)( mScissorRelHeight * height );

        mScissorsMatchViewport = mActLeft == mScissorActLeft && mActTop == mScissorActTop &&
                                 mActWidth == mScissorActWidth && mActHeight == mScissorActHeight;
        mCoversEntireTarget = mActLeft == 0 && mActTop == 0 && mActWidth == width &&
                              mActHeight == height && mScissorsMatchViewport;

        mUpdated = true;
    }
    //---------------------------------------------------------------------
    Real Viewport::getLeft() const { return mRelLeft; }
    //---------------------------------------------------------------------
    Real Viewport::getTop() const { return mRelTop; }
    //---------------------------------------------------------------------
    Real Viewport::getWidth() const { return mRelWidth; }
    //---------------------------------------------------------------------
    Real Viewport::getHeight() const { return mRelHeight; }
    //---------------------------------------------------------------------
    int Viewport::getActualLeft() const { return mActLeft; }
    //---------------------------------------------------------------------
    int Viewport::getActualTop() const { return mActTop; }
    //---------------------------------------------------------------------
    int Viewport::getActualWidth() const { return mActWidth; }
    //---------------------------------------------------------------------
    int Viewport::getActualHeight() const { return mActHeight; }
    //---------------------------------------------------------------------
    bool Viewport::coversEntireTarget() const { return mCoversEntireTarget; }
    //---------------------------------------------------------------------
    bool Viewport::scissorsMatchViewport() const { return mScissorsMatchViewport; }
    //---------------------------------------------------------------------
    void Viewport::setDimensions( TextureGpu *newTarget, const Vector4 &relativeVp,
                                  const Vector4 &scissors, uint8 mipLevel )
    {
        mCurrentTarget = newTarget;
        mCurrentMip = mipLevel;

        mRelLeft = relativeVp.x;
        mRelTop = relativeVp.y;
        mRelWidth = relativeVp.z;
        mRelHeight = relativeVp.w;

        mScissorRelLeft = scissors.x;
        mScissorRelTop = scissors.y;
        mScissorRelWidth = scissors.z;
        mScissorRelHeight = scissors.w;

        _updateDimensions();
    }
    //---------------------------------------------------------------------
    void Viewport::setScissors( Real left, Real top, Real width, Real height )
    {
        mScissorRelLeft = left;
        mScissorRelTop = top;
        mScissorRelWidth = width;
        mScissorRelHeight = height;
        _updateDimensions();
    }
    //---------------------------------------------------------------------
    void Viewport::_updateCullPhase01( Camera *renderCamera, Camera *cullCamera, const Camera *lodCamera,
                                       uint8 firstRq, uint8 lastRq, bool reuseCullData )
    {
        // Automatic AR cameras are useful for cameras that draw into multiple viewports
        const Real aspectRatio = (Real)mActWidth / (Real)std::max( 1, mActHeight );
        if( cullCamera->getAutoAspectRatio() && cullCamera->getAspectRatio() != aspectRatio )
            cullCamera->setAspectRatio( aspectRatio );

#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        {
            const OrientationMode orientationMode = mCurrentTarget->getOrientationMode();
            cullCamera->setOrientationMode( orientationMode );
            renderCamera->setOrientationMode( orientationMode );
        }
#endif
        // Tell Camera to render into me
        cullCamera->_notifyViewport( this );

        cullCamera->_cullScenePhase01( renderCamera, lodCamera, this, firstRq, lastRq, reuseCullData );
    }
    //---------------------------------------------------------------------
    void Viewport::_updateRenderPhase02( Camera *camera, const Camera *lodCamera, uint8 firstRq,
                                         uint8 lastRq )
    {
        camera->_renderScenePhase02( lodCamera, firstRq, lastRq, mShowOverlays );
    }
    //---------------------------------------------------------------------
    void Viewport::getActualDimensions( int &left, int &top, int &width, int &height ) const
    {
        left = mActLeft;
        top = mActTop;
        width = mActWidth;
        height = mActHeight;
    }
    //---------------------------------------------------------------------
    void Viewport::setOverlaysEnabled( bool enabled ) { mShowOverlays = enabled; }
    //---------------------------------------------------------------------
    bool Viewport::getOverlaysEnabled() const { return mShowOverlays; }
    //-----------------------------------------------------------------------
    void Viewport::_setVisibilityMask( uint32 mask, uint32 lightMask )
    {
        mVisibilityMask = mask;
        mLightVisibilityMask = lightMask;
    }
    //-----------------------------------------------------------------------
    void Viewport::pointOrientedToScreen( const Vector2 &v, int orientationMode, Vector2 &outv )
    {
        pointOrientedToScreen( v.x, v.y, orientationMode, outv.x, outv.y );
    }
    //-----------------------------------------------------------------------
    void Viewport::pointOrientedToScreen( Real orientedX, Real orientedY, int orientationMode,
                                          Real &screenX, Real &screenY )
    {
        Real orX = orientedX;
        Real orY = orientedY;
        switch( orientationMode )
        {
        case 1:
            screenX = orY;
            screenY = Real( 1.0 ) - orX;
            break;
        case 2:
            screenX = Real( 1.0 ) - orX;
            screenY = Real( 1.0 ) - orY;
            break;
        case 3:
            screenX = Real( 1.0 ) - orY;
            screenY = orX;
            break;
        default:
            screenX = orX;
            screenY = orY;
            break;
        }
    }
    //-----------------------------------------------------------------------
    void Viewport::setDrawBuffer( ColourBufferType colourBuffer ) { mColourBuffer = colourBuffer; }
    //-----------------------------------------------------------------------
    ColourBufferType Viewport::getDrawBuffer() const { return mColourBuffer; }
    //-----------------------------------------------------------------------
}  // namespace Ogre
