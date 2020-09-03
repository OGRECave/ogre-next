/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "Terra/Hlms/PbsListener/OgreHlmsPbsTerraShadows.h"
#include "Terra/Terra.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"

#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"

namespace Ogre
{
    const IdString PbsTerraProperty::TerraEnabled   = IdString( "terra_enabled" );

    HlmsPbsTerraShadows::HlmsPbsTerraShadows() :
          mTerra( 0 )
        , mTerraSamplerblock( 0 )
#if OGRE_DEBUG_MODE
        , mSceneManager( 0 )
#endif
    {
    }
    //-----------------------------------------------------------------------------------
    HlmsPbsTerraShadows::~HlmsPbsTerraShadows()
    {
        if( mTerraSamplerblock )
        {
            HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
            hlmsManager->destroySamplerblock( mTerraSamplerblock );
            mTerraSamplerblock = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbsTerraShadows::setTerra( Terra *terra )
    {
        mTerra = terra;
        if( !mTerraSamplerblock )
        {
            HlmsManager *hlmsManager = Root::getSingleton().getHlmsManager();
            mTerraSamplerblock = hlmsManager->getSamplerblock( HlmsSamplerblock() );
        }
    }
    //-----------------------------------------------------------------------------------
    uint16 HlmsPbsTerraShadows::getNumExtraPassTextures( bool casterPass ) const
    {
        return ( !casterPass && mTerra ) ? 1u : 0u;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbsTerraShadows::propertiesMergedPreGenerationStep(
        Hlms *hlms, const HlmsCache &passCache, const HlmsPropertyVec &renderableCacheProperties,
        const PiecesMap renderableCachePieces[NumShaderTypes], const HlmsPropertyVec &properties,
        const QueuedRenderable &queuedRenderable )
    {
        if( hlms->_getProperty( HlmsBaseProp::ShadowCaster ) == 0 && mTerra )
        {
            int32 texUnit = hlms->_getProperty( PbsProperty::Set0TextureSlotEnd );
            hlms->_setTextureReg( VertexShader, "terrainShadows", texUnit - 1 );
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbsTerraShadows::preparePassHash( const CompositorShadowNode *shadowNode,
                                               bool casterPass, bool dualParaboloid,
                                               SceneManager *sceneManager, Hlms *hlms )
    {
        if( !casterPass )
        {
#if OGRE_DEBUG_MODE
            mSceneManager = sceneManager;
#endif

            if( mTerra && hlms->_getProperty( HlmsBaseProp::LightsDirNonCaster ) > 0 )
            {
                //First directional light always cast shadows thanks to our terrain shadows.
                int32 shadowCasterDirectional = hlms->_getProperty( HlmsBaseProp::LightsDirectional );
                shadowCasterDirectional = std::max( shadowCasterDirectional, 1 );
                hlms->_setProperty( HlmsBaseProp::LightsDirectional, shadowCasterDirectional );
            }

            hlms->_setProperty( PbsTerraProperty::TerraEnabled, mTerra != 0 );
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsPbsTerraShadows::getPassBufferSize( const CompositorShadowNode *shadowNode,
                                                   bool casterPass, bool dualParaboloid,
                                                   SceneManager *sceneManager ) const
    {
        return (!casterPass && mTerra) ? 32u : 0u;
    }
    //-----------------------------------------------------------------------------------
    float* HlmsPbsTerraShadows::preparePassBuffer( const CompositorShadowNode *shadowNode,
                                                   bool casterPass, bool dualParaboloid,
                                                   SceneManager *sceneManager,
                                                   float *passBufferPtr )
    {
        if( !casterPass && mTerra )
        {
            const float invHeight = 1.0f / mTerra->getHeight();
            const Vector3 &terrainOrigin = mTerra->getTerrainOrigin();
            const Vector2 &terrainXZInvDim = mTerra->getXZInvDimensions();
            *passBufferPtr++ = -terrainOrigin.x * terrainXZInvDim.x;
            *passBufferPtr++ = -terrainOrigin.y * invHeight;
            *passBufferPtr++ = -terrainOrigin.z * terrainXZInvDim.y;
            *passBufferPtr++ = 1.0f;

            *passBufferPtr++ = terrainXZInvDim.x;
            *passBufferPtr++ = invHeight;
            *passBufferPtr++ = terrainXZInvDim.y;
            *passBufferPtr++ = 1.0f;
        }

        return passBufferPtr;
    }
    //-----------------------------------------------------------------------------------
    void HlmsPbsTerraShadows::hlmsTypeChanged( bool casterPass, CommandBuffer *commandBuffer,
                                               const HlmsDatablock *datablock, size_t texUnit )
    {
        if( !casterPass && mTerra )
        {
            Ogre::TextureGpu *terraShadowTex = mTerra->_getShadowMapTex();

            //Bind the shadows' texture. Tex. slot must match with
            //the one in HlmsPbsTerraShadows::propertiesMergedPreGenerationStep
            *commandBuffer->addCommand<CbTexture>() = CbTexture( texUnit++, terraShadowTex,
                                                                 mTerraSamplerblock );
        }
    }
}
