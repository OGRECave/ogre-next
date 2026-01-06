/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include "OgreMetalTextureGpuWindow.h"

#include "OgreException.h"
#include "OgreMetalMappings.h"
#include "OgreMetalTextureGpuManager.h"
#include "OgreMetalWindow.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreVector2.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    MetalTextureGpuWindow::MetalTextureGpuWindow( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  VaoManager *vaoManager, IdString name,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  TextureGpuManager *textureManager,
                                                  MetalWindow *window ) :
        MetalTextureGpuRenderTarget( pageOutStrategy, vaoManager, name, textureFlags, initialType,
                                     textureManager ),
        mWindow( window )
    {
        mTextureType = TextureTypes::Type2D;
        mFinalTextureName = 0;
        mDisplayTextureName = 0;
    }
    //-----------------------------------------------------------------------------------
    MetalTextureGpuWindow::~MetalTextureGpuWindow() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::createInternalResourcesImpl() {}
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::destroyInternalResourcesImpl() { _setBackbuffer( 0 ); }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::notifyDataIsReady()
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;
        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool MetalTextureGpuWindow::_isDataReadyImpl() const
    {
        return mResidencyStatus == GpuResidency::Resident;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::swapBuffers()
    {
        mWindow->swapBuffers();

        if( !mWindow->isManualSwapRelease() )
        {
            // Release strong references
            mFinalTextureName = 0;
            mDisplayTextureName = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == "Window" )
            *static_cast<Window **>( pData ) = mWindow;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::_setToDisplayDummyTexture() {}
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "",
                     "MetalTextureGpuWindow::_notifyTextureSlotChanged" );
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::nextDrawable() { mWindow->nextDrawable(); }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::_setBackbuffer( id<MTLTexture> backbuffer )
    {
        mFinalTextureName = backbuffer;
        mDisplayTextureName = backbuffer;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::_setMsaaBackbuffer( id<MTLTexture> msaaTex )
    {
        mMsaaFramebufferName = msaaTex;
    }
    //-----------------------------------------------------------------------------------
    void MetalTextureGpuWindow::setTextureType( TextureTypes::TextureTypes textureType )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "You cannot call setTextureType if isRenderWindowSpecific is true",
                     "MetalTextureGpuWindow::setTextureType" );
    }
}  // namespace Ogre
