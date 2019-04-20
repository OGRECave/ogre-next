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

#include "Vct/OgreVctLighting.h"
#include "Vct/OgreVctVoxelizer.h"

#include "OgreTextureGpuManager.h"
#include "OgreStringConverter.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    VctLighting::VctLighting( IdType id, VctVoxelizer *voxelizer ) :
        IdObject( id ),
        mVoxelizer( voxelizer )
    {
        TextureGpuManager *textureManager = voxelizer->getTextureGpuManager();

        RenderSystem *renderSystem = voxelizer->getRenderSystem();
        const bool hasTypedUavs = renderSystem->getCapabilities()->hasCapability( RSC_TYPED_UAV_LOADS );

        uint32 texFlags = TextureFlags::Uav;
        if( !hasTypedUavs )
            texFlags |= TextureFlags::Reinterpretable;

        const TextureGpu *albedoVox = voxelizer->getAlbedoVox();

        const uint32 width  = albedoVox->getWidth();
        const uint32 height = albedoVox->getHeight();
        const uint32 depth  = albedoVox->getDepth();

        const uint32 widthAniso     = std::max( 1u, width >> 1u );
        const uint32 heightAniso    = std::max( 1u, height >> 1u );
        const uint32 depthAniso     = std::max( 1u, depth >> 1u );

        const uint32 numMipsMain  = PixelFormatGpuUtils::getMaxMipmapCount( width, height, depth );
        const uint32 numMipsAniso = PixelFormatGpuUtils::getMaxMipmapCount( widthAniso, heightAniso,
                                                                            depthAniso );

        for( size_t i=0; i<6u; ++i )
        {
            TextureGpu *texture = textureManager->createTexture( "VctLighting" +
                                                                 StringConverter::toString( getId() ),
                                                                 GpuPageOutStrategy::Discard,
                                                                 texFlags, TextureTypes::Type3D );
            if( i == 0u )
            {
                texture->setResolution( width, height, depth );
                texture->setNumMipmaps( numMipsMain );
            }
            else
            {
                texture->setResolution( widthAniso, heightAniso, depthAniso );
                texture->setNumMipmaps( numMipsAniso );
            }
            texture->scheduleTransitionTo( GpuResidency::Resident );
            mLightVoxel[i] = texture;
        }
    }
    //-------------------------------------------------------------------------
    VctLighting::~VctLighting()
    {
    }
    //-------------------------------------------------------------------------
    void VctLighting::update( uint32 lightMask )
    {

    }
}
