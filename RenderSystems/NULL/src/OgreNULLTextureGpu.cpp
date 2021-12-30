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

#include "OgreNULLTextureGpu.h"

#include "OgreException.h"
#include "OgreVector2.h"

namespace Ogre
{
    NULLTextureGpu::NULLTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                    VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                    TextureTypes::TextureTypes initialType,
                                    TextureGpuManager *textureManager ) :
        TextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager )
    {
    }
    //-----------------------------------------------------------------------------------
    NULLTextureGpu::~NULLTextureGpu() {}
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::createInternalResourcesImpl() {}
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::destroyInternalResourcesImpl() {}
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::getSubsampleLocations( vector<Vector2>::type locations )
    {
        locations.reserve( mSampleDescription.getColourSamples() );
        for( size_t i = 0; i < mSampleDescription.getColourSamples(); ++i )
            locations.push_back( Vector2( 0, 0 ) );
    }
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::notifyDataIsReady()
    {
        OGRE_ASSERT_LOW( mDataPreparationsPending > 0u &&
                         "Calling notifyDataIsReady too often! Remove this call"
                         "See https://github.com/OGRECave/ogre-next/issues/101" );
        --mDataPreparationsPending;
    }
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::_autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode
                                               /*transitionMode*/ )
    {
    }
    //-----------------------------------------------------------------------------------
    void NULLTextureGpu::_setToDisplayDummyTexture() {}
    //-----------------------------------------------------------------------------------
    bool NULLTextureGpu::_isDataReadyImpl() const { return true && mDataPreparationsPending == 0u; }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    NULLTextureGpuRenderTarget::NULLTextureGpuRenderTarget(
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, VaoManager *vaoManager, IdString name,
        uint32 textureFlags, TextureTypes::TextureTypes initialType,
        TextureGpuManager *textureManager ) :
        NULLTextureGpu( pageOutStrategy, vaoManager, name, textureFlags, initialType, textureManager ),
        mDepthBufferPoolId( 1u ),
        mPreferDepthTexture( false ),
        mDesiredDepthBufferFormat( PFG_UNKNOWN )
    {
    }
    //-----------------------------------------------------------------------------------
    void NULLTextureGpuRenderTarget::_setDepthBufferDefaults( uint16 depthBufferPoolId,
                                                              bool preferDepthTexture,
                                                              PixelFormatGpu desiredDepthBufferFormat )
    {
        assert( isRenderToTexture() );
        mDepthBufferPoolId = depthBufferPoolId;
        mPreferDepthTexture = preferDepthTexture;
        mDesiredDepthBufferFormat = desiredDepthBufferFormat;
    }
    //-----------------------------------------------------------------------------------
    uint16 NULLTextureGpuRenderTarget::getDepthBufferPoolId() const { return mDepthBufferPoolId; }
    //-----------------------------------------------------------------------------------
    bool NULLTextureGpuRenderTarget::getPreferDepthTexture() const { return mPreferDepthTexture; }
    //-----------------------------------------------------------------------------------
    PixelFormatGpu NULLTextureGpuRenderTarget::getDesiredDepthBufferFormat() const
    {
        return mDesiredDepthBufferFormat;
    }
}  // namespace Ogre
