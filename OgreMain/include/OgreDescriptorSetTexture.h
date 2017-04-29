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

#ifndef _OgreDescriptorSetTexture_H_
#define _OgreDescriptorSetTexture_H_

#include "OgrePrerequisites.h"
#include "OgreCommon.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    /** Descriptor sets describe what textures should be bound together in one place.
        They must be pushed to mTexture in the order of ShaderType.
        For example if you want to use 2 textures bound to the pixel shader stage
        and 1 in the Geometry Shader stage:
            DescriptorSetTexture descSet;
            descSet.mTextures.push_back( pixelShaderTex0 );
            descSet.mTextures.push_back( pixelShaderTex1 );
            descSet.mTextures.push_back( geometryShaderTex1 );
            descSet.mShaderTypeTexCount[PixelShader] = 2u;
            descSet.mShaderTypeTexCount[GeometryShader] = 1u;

            const DescriptorSetTexture *finalSet = hlmsManager->getDescriptorSet( descSet );
            // finalSet can be used with RenderSystem::_setTextures

            // Remove finalSet once you're done using it.
            hlmsManager->destroyDescriptorSet( finalSet );
            finalSet = 0;
    @remarks
        Do not create and destroy a set every frame. You should create it once, and reuse
        it until it is no longer necessary, then you should destroy it.
    */
    struct _OgreExport DescriptorSetTexture
    {
        uint16      mRefCount;
        uint16      mShaderTypeTexCount[NumShaderTypes];
        void        *mRsData;           /// Render-System specific data

        FastArray<const TextureGpu*> mTextures;

        DescriptorSetTexture() :
            mRefCount( 0 ),
            mRsData( 0 )
        {
            memset( mShaderTypeTexCount, 0, sizeof(mShaderTypeTexCount) );
        }

        bool operator != ( const DescriptorSetTexture &other ) const
        {
            const size_t thisNumTextures = mTextures.size();
            if( thisNumTextures != other.mTextures.size() )
                return true;

            for( size_t i=0; i<thisNumTextures; ++i )
            {
                if( this->mTextures[i] != other.mTextures[i] )
                    return true;
            }

            for( size_t i=0; i<NumShaderTypes; ++i )
            {
                if( this->mShaderTypeTexCount[i] != other.mShaderTypeTexCount[i] )
                    return true;
            }

            return false;
        }

        bool operator < ( const DescriptorSetTexture &other ) const
        {
            const size_t thisNumTextures = mTextures.size();
            if( thisNumTextures != other.mTextures.size() )
                return thisNumTextures < other.mTextures.size();

            for( size_t i=0; i<thisNumTextures; ++i )
            {
                if( this->mTextures[i] != other.mTextures[i] )
                    return this->mTextures[i] < other.mTextures[i];
            }

            for( size_t i=0; i<NumShaderTypes; ++i )
            {
                if( this->mShaderTypeTexCount[i] != other.mShaderTypeTexCount[i] )
                    return this->mShaderTypeTexCount[i] < other.mShaderTypeTexCount[i];
            }

            return false;
        }

        void checkValidity() const
        {
            size_t totalTexturesUsed = 0u;

            for( size_t i=0; i<NumShaderTypes; ++i )
                totalTexturesUsed += mShaderTypeTexCount[i];

            assert( totalTexturesUsed > 0 &&
                    "This DescriptorSetTexture doesn't use any texture! Perhaps incorrectly setup?" );
            assert( totalTexturesUsed == mTextures.size() &&
                    "This DescriptorSetTexture doesn't use as many textures as it "
                    "claims to have, or uses more than it has provided" );
        }
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
