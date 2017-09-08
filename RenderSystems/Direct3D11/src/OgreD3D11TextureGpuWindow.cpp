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

#include "OgreD3D11TextureGpuWindow.h"
#include "OgreD3D11Mappings.h"
#include "OgreD3D11TextureGpuManager.h"

#include "OgreTextureGpuListener.h"
#include "OgreTextureBox.h"
#include "OgreVector2.h"
#include "OgreWindow.h"

#include "Vao/OgreVaoManager.h"

#include "OgreException.h"

namespace Ogre
{
    extern const IdString CustomAttributeIdString_GLCONTEXT;

    D3D11TextureGpuWindow::D3D11TextureGpuWindow(
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            VaoManager *vaoManager, IdString name, uint32 textureFlags,
            TextureTypes::TextureTypes initialType,
            TextureGpuManager *textureManager,
            ID3D11Texture2D *backbuffer, Window *window ) :
        D3D11TextureGpuRenderTarget( pageOutStrategy, vaoManager, name,
                                     textureFlags, initialType, textureManager ),
        mWindow( window )
    {
        mTextureType = TextureTypes::Type2D;
        mFinalTextureName = backbuffer;
        mDisplayTextureName = backbuffer;
        mDefaultDisplaySrv = _createSrv( mPixelFormat, false );
    }
    //-----------------------------------------------------------------------------------
    D3D11TextureGpuWindow::~D3D11TextureGpuWindow()
    {
        destroyInternalResourcesImpl();
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::createInternalResourcesImpl(void)
    {
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::destroyInternalResourcesImpl(void)
    {
        _setBackbuffer( 0 );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::notifyDataIsReady(void)
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForDisplay );
    }
    //-----------------------------------------------------------------------------------
    bool D3D11TextureGpuWindow::isDataReady(void) const
    {
        return mResidencyStatus == GpuResidency::Resident;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::swapBuffers(void)
    {
        mWindow->swapBuffers();
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::getCustomAttribute( IdString name, void *pData )
    {
        /*if( name == CustomAttributeIdString_GLCONTEXT )
            *static_cast<D3D11Context**>(pData) = mContext;
        else */if( name == "Window" )
            *static_cast<Window**>(pData) = mWindow;
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::_setToDisplayDummyTexture(void)
    {
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "",
                     "D3D11TextureGpuWindow::_notifyTextureSlotChanged" );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::_setBackbuffer( ID3D11Texture2D *backbuffer )
    {
        if( mDefaultDisplaySrv )
            mDefaultDisplaySrv->Release();

        mFinalTextureName = backbuffer;
        mDisplayTextureName = backbuffer;
        if( backbuffer )
            mDefaultDisplaySrv = _createSrv( mPixelFormat, false );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::setTextureType( TextureTypes::TextureTypes textureType )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "You cannot call setTextureType if isRenderWindowSpecific is true",
                     "D3D11TextureGpuWindow::setTextureType" );
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::copyTo( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                          const TextureBox &srcBox, uint8 srcMipLevel )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                     "TODO",
                     "D3D11TextureGpuWindow::copyTo" );
        TextureGpu::copyTo( dst, dstBox, dstMipLevel, srcBox, srcMipLevel );

//        assert( dynamic_cast<D3D11TextureGpu*>( dst ) );

//        D3D11TextureGpuWindow *dstGl = static_cast<D3D11TextureGpuWindow*>( dst );
//        D3D11TextureGpuWindowManager *textureManagerGl =
//                static_cast<D3D11TextureGpuWindowManager*>( mTextureManager );
//        const D3D11Support &support = textureManagerGl->getGlSupport();
    }
    //-----------------------------------------------------------------------------------
    void D3D11TextureGpuWindow::getSubsampleLocations( vector<Vector2>::type locations )
    {
        OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "",
                     "D3D11TextureGpuWindow::getSubsampleLocations" );
#if TODO_OGRE_2_2
        locations.reserve( mMsaa );
        if( mMsaa <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mMsaaPattern != MsaaPatterns::Undefined );

            float vals[2];
            for( int i=0; i<mMsaa; ++i )
            {
                glGetMultisamplefv( GL_SAMPLE_POSITION, i, vals );
                locations.push_back( Vector2( vals[0], vals[1] ) * 2.0f - 1.0f );
            }
        }
#endif
    }
}
