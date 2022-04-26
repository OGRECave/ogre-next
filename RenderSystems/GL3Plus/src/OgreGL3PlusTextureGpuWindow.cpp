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

#include "OgreGL3PlusTextureGpuWindow.h"
#include "OgreGL3PlusMappings.h"
#include "OgreGL3PlusSupport.h"
#include "OgreGL3PlusTextureGpuManager.h"

#include "OgreTextureBox.h"
#include "OgreTextureGpuListener.h"
#include "OgreVector2.h"
#include "OgreWindow.h"

#include "Vao/OgreVaoManager.h"

#include "OgreException.h"

namespace Ogre
{
    extern const IdString CustomAttributeIdString_GLCONTEXT;

    GL3PlusTextureGpuWindow::GL3PlusTextureGpuWindow(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType, TextureGpuManager *textureManager,
        GL3PlusContext *context, Window *window ) :
        GL3PlusTextureGpuRenderTarget( pageOutStrategy, vaoManager, name, textureFlags, initialType,
                                       textureManager ),
        mContext( context ),
        mWindow( window )
    {
        mTextureType = TextureTypes::Type2D;
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuWindow::~GL3PlusTextureGpuWindow() { destroyInternalResourcesImpl(); }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::createInternalResourcesImpl() {}
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::destroyInternalResourcesImpl() {}
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::notifyDataIsReady()
    {
        assert( mResidencyStatus == GpuResidency::Resident );
        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;
        notifyAllListenersTextureChanged( TextureGpuListener::ReadyForRendering );
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpuWindow::_isDataReadyImpl() const
    {
        return mResidencyStatus == GpuResidency::Resident;
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::swapBuffers() { mWindow->swapBuffers(); }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == CustomAttributeIdString_GLCONTEXT )
            *static_cast<GL3PlusContext **>( pData ) = mContext;
        else if( name == "Window" )
            *static_cast<Window **>( pData ) = mWindow;
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpuWindow::isOpenGLRenderWindow() const { return true; }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::_setToDisplayDummyTexture() {}
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::_notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "",
                     "GL3PlusTextureGpuWindow::_notifyTextureSlotChanged" );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::setTextureType( TextureTypes::TextureTypes textureType )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "You cannot call setTextureType if isRenderWindowSpecific is true",
                     "GL3PlusTextureGpuWindow::setTextureType" );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuWindow::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mSampleDescription.getColourSamples() );
        if( mSampleDescription.getColourSamples() <= 1u )
        {
            locations.push_back( Vector2( 0.0f, 0.0f ) );
        }
        else
        {
            assert( mSampleDescription.getMsaaPattern() != MsaaPatterns::Undefined );

            float vals[2];
            for( int i = 0; i < mSampleDescription.getColourSamples(); ++i )
            {
                glGetMultisamplefv( GL_SAMPLE_POSITION, static_cast<GLuint>( i ), vals );
                locations.push_back( Vector2( vals[0], vals[1] ) * 2.0f - 1.0f );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuHeadlessWindow::GL3PlusTextureGpuHeadlessWindow(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType, TextureGpuManager *textureManager,
        GL3PlusContext *context, Window *window ) :
        GL3PlusTextureGpuRenderTarget( pageOutStrategy, vaoManager, name, textureFlags, initialType,
                                       textureManager ),
        mContext( context ),
        mWindow( window )
    {
        mTextureType = TextureTypes::Type2D;
    }
    //-----------------------------------------------------------------------------------
    GL3PlusTextureGpuHeadlessWindow::~GL3PlusTextureGpuHeadlessWindow()
    {
        destroyInternalResourcesImpl();
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuHeadlessWindow::swapBuffers() { mWindow->swapBuffers(); }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuHeadlessWindow::getCustomAttribute( IdString name, void *pData )
    {
        if( name == CustomAttributeIdString_GLCONTEXT )
            *static_cast<GL3PlusContext **>( pData ) = mContext;
        else if( name == "Window" )
            *static_cast<Window **>( pData ) = mWindow;
    }
    //-----------------------------------------------------------------------------------
    bool GL3PlusTextureGpuHeadlessWindow::isOpenGLRenderWindow() const
    {
        return false;  // We must lie. Internally it's an FBO. We don't expose the PBuffer
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuHeadlessWindow::_notifyTextureSlotChanged( const TexturePool *newPool,
                                                                     uint16 slice )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "",
                     "GL3PlusTextureGpuHeadlessWindow::_notifyTextureSlotChanged" );
    }
    //-----------------------------------------------------------------------------------
    void GL3PlusTextureGpuHeadlessWindow::setTextureType( TextureTypes::TextureTypes textureType )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "You cannot call setTextureType if isRenderWindowSpecific is true",
                     "GL3PlusTextureGpuHeadlessWindow::setTextureType" );
    }
}  // namespace Ogre
