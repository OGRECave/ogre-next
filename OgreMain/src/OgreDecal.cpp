/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2018 Torus Knot Software Ltd

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

#include "OgreDecal.h"
#include "OgreForwardPlusBase.h"

#include "OgreSceneManager.h"

#include "OgreRoot.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"

namespace Ogre
{
    Decal::Decal( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager ) :
        MovableObject( id, objectMemoryManager, manager, ForwardPlusBase::MinDecalRq ),
        mDiffuseTexture( 0 ),
        mNormalTexture( 0 ),
        mEmissiveTexture( 0 ),
        mDiffuseIdx( 0 ),
        mNormalMapIdx( 0 ),
        mEmissiveIdx( 0 ),
        mIgnoreDiffuseAlpha( 0 ),
        mMetalness( 1.0f ),
        mRoughness( 1.0f )
    {
        //NOTE: For performance reasons, ForwardClustered::collectLightForSlice ignores
        //mLocalAabb & mWorldAabb and assumes its local AABB is this aabb we set as
        //default. To change its shape, use node scaling
        Aabb aabb( Vector3::ZERO, Vector3::UNIT_SCALE * 0.5f );
        mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
        mObjectData.mWorldAabb->setFromAabb( aabb, mObjectData.mIndex );
        const float radius = aabb.getRadius();
        mObjectData.mLocalRadius[mObjectData.mIndex] = radius;
        mObjectData.mWorldRadius[mObjectData.mIndex] = radius;

        //Disable shadow casting by default. Otherwise it's a waste or resources
        setCastShadows( false );
    }
    //-----------------------------------------------------------------------------------
    Decal::~Decal()
    {
        if( mDiffuseTexture && mDiffuseTexture->hasAutomaticBatching() )
        {
            mDiffuseTexture->removeListener( this );
            mDiffuseTexture = 0;
        }

        if( mNormalTexture && mNormalTexture->hasAutomaticBatching() )
        {
            mNormalTexture->removeListener( this );
            mNormalTexture= 0;
        }

        if( mEmissiveTexture && mEmissiveTexture->hasAutomaticBatching() )
        {
            mEmissiveTexture->removeListener( this );
            mEmissiveTexture= 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Decal::setDiffuseTexture( TextureGpu *diffuseTex )
    {
        if( mDiffuseTexture && mDiffuseTexture->hasAutomaticBatching() )
            mDiffuseTexture->removeListener( this );
        if( diffuseTex )
        {
            OGRE_ASSERT_LOW( diffuseTex->hasAutomaticBatching() &&
                             "If the texture does is not AutomaticBatching, the use Raw calls!" );
            OGRE_ASSERT_LOW( diffuseTex->getTextureType() == TextureTypes::Type2D );
            diffuseTex->addListener( this );
            diffuseTex->scheduleTransitionTo( GpuResidency::Resident );
            mDiffuseTexture = diffuseTex;
            mDiffuseIdx = static_cast<uint16>( diffuseTex->getInternalSliceStart() );
        }
        else
        {
            mDiffuseTexture = 0;
            mDiffuseIdx = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Decal::getDiffuseTexture(void) const
    {
        return mDiffuseTexture;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setNormalTexture( TextureGpu *normalTex )
    {
        if( mNormalTexture && mNormalTexture->hasAutomaticBatching() )
            mNormalTexture->removeListener( this );
        if( normalTex )
        {
            OGRE_ASSERT_LOW( normalTex->hasAutomaticBatching() &&
                             "If the texture does is not AutomaticBatching, the use Raw calls!" );
            OGRE_ASSERT_LOW( normalTex->getTextureType() == TextureTypes::Type2D );
            normalTex->addListener( this );
            normalTex->scheduleTransitionTo( GpuResidency::Resident );
            mNormalTexture = normalTex;
            mNormalMapIdx = static_cast<uint16>( normalTex->getInternalSliceStart() );
        }
        else
        {
            mNormalTexture = 0;
            mNormalMapIdx = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Decal::getNormalTexture(void) const
    {
        return mNormalTexture;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setEmissiveTexture( TextureGpu *emissiveTex )
    {
        if( mEmissiveTexture && mEmissiveTexture->hasAutomaticBatching() )
            mEmissiveTexture->removeListener( this );
        if( emissiveTex )
        {
            OGRE_ASSERT_LOW( emissiveTex->hasAutomaticBatching() &&
                             "If the texture does is not AutomaticBatching, the use Raw calls!" );
            OGRE_ASSERT_LOW( emissiveTex->getTextureType() == TextureTypes::Type2D );
            emissiveTex->addListener( this );
            emissiveTex->scheduleTransitionTo( GpuResidency::Resident );
            mEmissiveTexture = emissiveTex;
            mEmissiveIdx = static_cast<uint16>( emissiveTex->getInternalSliceStart() );
        }
        else
        {
            mEmissiveTexture = 0;
            mEmissiveIdx = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Decal::setDiffuseTextureRaw( TextureGpu *diffuseTex, uint32 sliceIdx )
    {
        if( mDiffuseTexture && mDiffuseTexture->hasAutomaticBatching() )
            mDiffuseTexture->removeListener( this );
        OGRE_ASSERT_LOW( (!diffuseTex || !diffuseTex->hasAutomaticBatching()) &&
                         "Only use Raw call if texture is not AutomaticBatching!" );
        OGRE_ASSERT_LOW( diffuseTex->getTextureType() == TextureTypes::Type2DArray );
        mDiffuseTexture = diffuseTex;
        mDiffuseIdx = sliceIdx;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setNormalTextureRaw( TextureGpu *normalTex, uint32 sliceIdx )
    {
        if( mNormalTexture && mNormalTexture->hasAutomaticBatching() )
            mNormalTexture->removeListener( this );
        OGRE_ASSERT_LOW( (!normalTex || !normalTex->hasAutomaticBatching()) &&
                         "Only use Raw call if texture is not AutomaticBatching!" );
        OGRE_ASSERT_LOW( normalTex->getTextureType() == TextureTypes::Type2DArray );
        mNormalTexture = normalTex;
        mNormalMapIdx = sliceIdx;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setEmissiveTextureRaw( TextureGpu *emissiveTex, uint32 sliceIdx )
    {
        if( mEmissiveTexture && mEmissiveTexture->hasAutomaticBatching() )
            mEmissiveTexture->removeListener( this );
        OGRE_ASSERT_LOW( (!emissiveTex || !emissiveTex->hasAutomaticBatching()) &&
                         "Only use Raw call if texture is not AutomaticBatching!" );
        OGRE_ASSERT_LOW( emissiveTex->getTextureType() == TextureTypes::Type2DArray );
        mEmissiveTexture = emissiveTex;
        mEmissiveIdx = sliceIdx;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* Decal::getEmissiveTexture(void) const
    {
        return mEmissiveTexture;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setIgnoreAlphaDiffuse( bool bIgnore )
    {
        mIgnoreDiffuseAlpha = bIgnore ? 1u : 0u;
    }
    //-----------------------------------------------------------------------------------
    bool Decal::getIgnoreAlphaDiffuse(void) const
    {
        return mIgnoreDiffuseAlpha != 0u;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setRoughness( float roughness )
    {
        mRoughness = std::max( roughness, 0.02f );
    }
    //-----------------------------------------------------------------------------------
    void Decal::setMetalness( float value )
    {
        mMetalness = value;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setRectSize( Vector2 planeDimensions, Real depth )
    {
        if( mParentNode )
            mParentNode->setScale( planeDimensions.x, depth, planeDimensions.y );
    }
    //-----------------------------------------------------------------------------------
    const String& Decal::getMovableType(void) const
    {
        return DecalFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------------------
    void Decal::setRenderQueueGroup(uint8 queueID)
    {
        assert( queueID >= ForwardPlusBase::MinDecalRq && queueID <= ForwardPlusBase::MaxDecalRq &&
                "RenderQueue IDs > 128 are reserved for other Forward+ objects" );
        MovableObject::setRenderQueueGroup( queueID );
    }
    //-----------------------------------------------------------------------------------
    void Decal::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                      void *extraData )
    {
        if( reason == TextureGpuListener::PoolTextureSlotChanged )
        {
            if( texture == mDiffuseTexture )
                mDiffuseIdx = static_cast<uint16>( mDiffuseTexture->getInternalSliceStart() );
            if( texture == mNormalTexture )
                mNormalMapIdx = static_cast<uint16>( mNormalTexture->getInternalSliceStart() );
            if( texture == mEmissiveTexture )
                mEmissiveIdx = static_cast<uint16>( mEmissiveTexture->getInternalSliceStart() );
        }
        else if( reason == TextureGpuListener::Deleted )
        {
            if( texture == mDiffuseTexture )
            {
                mDiffuseIdx = 0;
                mDiffuseTexture->removeListener( this );
                mDiffuseTexture = 0;
            }
            if( texture == mNormalTexture )
            {
                mNormalMapIdx = 0;
                mNormalTexture->removeListener( this );
                mNormalTexture = 0;
            }
            if( texture == mEmissiveTexture )
            {
                mEmissiveIdx = 0;
                mEmissiveTexture->removeListener( this );
                mEmissiveTexture = 0;
            }
        }
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String DecalFactory::FACTORY_TYPE_NAME = "Decal";
    //-----------------------------------------------------------------------
    const String& DecalFactory::getType(void) const
    {
     return FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    MovableObject* DecalFactory::createInstanceImpl( IdType id,
                                                  ObjectMemoryManager *objectMemoryManager,
                                                  SceneManager *manager,
                                                  const NameValuePairList* params )
    {

     Decal* decal = OGRE_NEW Decal( id, objectMemoryManager, manager );
     return decal;
    }
    //-----------------------------------------------------------------------
    void DecalFactory::destroyInstance( MovableObject* obj)
    {
     OGRE_DELETE obj;
    }
}
