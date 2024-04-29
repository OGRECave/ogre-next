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

#include "OgreAtmosphereNpr.h"

#include "CommandBuffer/OgreCbShaderBuffer.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreCamera.h"
#include "OgreHlms.h"
#include "OgreLogManager.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreRectangle2D2.h"
#include "OgreSceneManager.h"
#include "OgreShaderPrimitives.h"
#include "OgreTechnique.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    struct AtmoSettingsGpu
    {
        float densityCoeff;
        float lightDensity;
        float sunHeight;
        float sunHeightWeight;

        float4 skyLightAbsorption;
        float4 sunAbsorption;
        float4 cameraDisplacement;
        float4 packedParams1;
        float4 packedParams2;
        float4 packedParams3;

        float fogDensity;
        float fogBreakMinBrightness;
        float fogBreakFalloff;
        float padding;
    };

    AtmosphereNpr::AtmosphereNpr( VaoManager *vaoManager ) :
        mSunDir( Ogre::Vector3( 0, 1, 1 ).normalisedCopy() ),
        mNormalizedTimeOfDay( std::asin( mSunDir.y ) ),
        mLinkedLight( 0 ),
        mConvention( Yup ),
        mAtmosphereSeaLevel( 0.0f ),
        mAtmosphereHeight( 110.0f * 1000.0f ),  // in meters (actually in units)
        mPass( 0 ),
        mHlmsBuffer( 0 ),
        mVaoManager( vaoManager )
    {
        mHlmsBuffer = vaoManager->createConstBuffer( sizeof( AtmoSettingsGpu ), BT_DEFAULT, 0, false );
        createMaterial();
    }
    //-------------------------------------------------------------------------
    AtmosphereNpr::~AtmosphereNpr()
    {
        std::map<Ogre::SceneManager *, Rectangle2D *>::const_iterator itor = mSkies.begin();
        std::map<Ogre::SceneManager *, Rectangle2D *>::const_iterator endt = mSkies.end();

        while( itor != endt )
        {
            if( itor->first->getAtmosphere() == this )
                itor->first->_setAtmosphere( nullptr );

            itor->second->detachFromParent();
            OGRE_DELETE itor->second;

            ++itor;
        }

        mSkies.clear();

        if( mMaterial )
        {
            MaterialManager &materialManager = MaterialManager::getSingleton();
            materialManager.remove( mMaterial );
            mMaterial.reset();
            mPass = 0;
        }

        mVaoManager->destroyConstBuffer( mHlmsBuffer );
        mHlmsBuffer = 0;
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::createMaterial()
    {
        OGRE_ASSERT_LOW( !mMaterial );

        MaterialManager &materialManager = MaterialManager::getSingleton();

        mMaterial = materialManager.getByName( "Ogre/Atmo/NprSky",
                                               ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        if( !mMaterial )
        {
            OGRE_EXCEPT( Exception::ERR_FILE_NOT_FOUND,
                         "To use the sky, bundle the resources included in Samples/Media/Common and "
                         "Samples/Media/Hlms/Pbs/Any/Atmosphere",
                         "AtmosphereNpr::createMaterial" );
        }

        mMaterial = mMaterial->clone( mMaterial->getName() + "/" +
                                      StringConverter::toString( Id::generateNewId<AtmosphereNpr>() ) );
        mMaterial->load();

        mPass = mMaterial->getTechnique( 0u )->getPass( 0u );
        setPackedParams();
    }
    //-------------------------------------------------------------------------
    inline Vector3 getSkyRayleighAbsorption( const Vector3 &vDir, const float density )
    {
        Vector3 absorption = -density * vDir;
        absorption.x = std::exp2( absorption.x );
        absorption.y = std::exp2( absorption.y );
        absorption.z = std::exp2( absorption.z );
        return absorption * 2.0f;
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setPackedParams()
    {
        const float sunHeight = std::sin( mNormalizedTimeOfDay * Math::PI );
        const float sunHeightWeight = std::exp2( -1.0f / sunHeight );
        // Gets smaller as sunHeight gets bigger
        const float lightDensity =
            mPreset.densityCoeff / std::pow( std::max( sunHeight, 0.0035f ), 0.75f );

        Vector4 packedParams0;
        packedParams0.x = mPreset.densityCoeff;  // densityCoeff
        packedParams0.y = lightDensity;          // lightDensity
        packedParams0.z = sunHeight;             // sunHeight
        packedParams0.w = sunHeightWeight;       // sunHeightWeight

        const Vector3 mieAbsorption =
            std::pow( std::max( 1.0f - lightDensity, 0.1f ), 4.0f ) *
            Math::lerp( mPreset.skyColour, Vector3::UNIT_SCALE, sunHeightWeight );
        const float finalMultiplier =
            ( 0.5f + Math::smoothstep( 0.02f, 0.4f, sunHeightWeight ) ) * mPreset.skyPower;
        const Vector4 packedParams1( mieAbsorption, finalMultiplier );
        const Vector4 packedParams2( mSunDir, mPreset.horizonLimit );
        const Vector4 packedParams3( mPreset.skyColour, mPreset.densityDiffusion );

        GpuProgramParametersSharedPtr psParams = mPass->getFragmentProgramParameters();

        psParams->setNamedConstant( "packedParams0", packedParams0 );
        psParams->setNamedConstant( "skyLightAbsorption",
                                    getSkyRayleighAbsorption( mPreset.skyColour, lightDensity ) );
        psParams->setNamedConstant(
            "sunAbsorption", Vector4( getSkyRayleighAbsorption( 1.0f - mPreset.skyColour, lightDensity ),
                                      mPreset.sunPower ) );
        psParams->setNamedConstant( "packedParams1", packedParams1 );
        psParams->setNamedConstant( "packedParams2", packedParams2 );
        psParams->setNamedConstant( "packedParams3", packedParams3 );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::syncToLight()
    {
        if( !mLinkedLight )
            return;

        mLinkedLight->setType( Light::LT_DIRECTIONAL );
        mLinkedLight->setDirection( -mSunDir );

        Vector3 sunColour3 = getAtmosphereAt( mSunDir );
        const Real maxPower = std::max( std::max( sunColour3.x, sunColour3.y ), sunColour3.z );
        sunColour3 /= maxPower;
        const ColourValue sunColour( sunColour3.x, sunColour3.y, sunColour3.z );
        mLinkedLight->setDiffuseColour( sunColour );
        mLinkedLight->setSpecularColour( sunColour );
        mLinkedLight->setPowerScale( mPreset.linkedLightPower );

        const Vector3 upperHemi3 = getAtmosphereAt( Vector3::UNIT_Y, true ) *
                                   ( mPreset.linkedSceneAmbientUpperPower / maxPower );
        const Vector3 lowerHemi3 = getAtmosphereAt( Vector3::UNIT_X + Vector3( 0, 0.1f, 0 ), true ) *
                                   ( mPreset.linkedSceneAmbientLowerPower / maxPower );

        const ColourValue upperHemi( upperHemi3.x, upperHemi3.y, upperHemi3.z );
        const ColourValue lowerHemi( lowerHemi3.x, lowerHemi3.y, lowerHemi3.z );

        Vector3 hemiDir( Vector3::ZERO );
        hemiDir[mConvention & 0x3u] = ( mConvention & NegationFlag ) ? -1.0f : 1.0f;
        hemiDir += Quaternion( Degree( 180.0f ), hemiDir ) * mSunDir;
        hemiDir.normalise();

        const float envmapScale = mPreset.envmapScale;
        std::map<Ogre::SceneManager *, Rectangle2D *>::const_iterator itor = mSkies.begin();
        std::map<Ogre::SceneManager *, Rectangle2D *>::const_iterator endt = mSkies.end();

        while( itor != endt )
        {
            if( itor->first->getAtmosphere() == this )
                itor->first->setAmbientLight( upperHemi, lowerHemi, hemiDir, envmapScale );
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setSky( Ogre::SceneManager *sceneManager, bool bEnabled )
    {
        Rectangle2D *sky = 0;

        std::map<Ogre::SceneManager *, Rectangle2D *>::iterator itor = mSkies.find( sceneManager );
        if( itor == mSkies.end() )
        {
            sky = OGRE_NEW Rectangle2D( Id::generateNewId<MovableObject>(),
                                        &sceneManager->_getEntityMemoryManager( SCENE_STATIC ),
                                        sceneManager );
            // We can't use BT_DYNAMIC_* because the scene may be rendered from multiple cameras
            // in the same frame, and dynamic supports only one set of values per frame
            sky->initialize( BT_DEFAULT,
                             Rectangle2D::GeometryFlagQuad | Rectangle2D::GeometryFlagNormals );
            sky->setGeometry( -Ogre::Vector2::UNIT_SCALE, Ogre::Vector2( 2.0f ) );
            sky->setRenderQueueGroup( 212u );  // Render after most stuff
            sceneManager->getRootSceneNode( SCENE_STATIC )->attachObject( sky );

            sky->setMaterial( mMaterial );
            mSkies[sceneManager] = sky;
        }
        else
        {
            sky = itor->second;
        }

        sky->setVisible( bEnabled );
        if( bEnabled )
            sceneManager->_setAtmosphere( this );
        else
            sceneManager->_setAtmosphere( nullptr );

        syncToLight();
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::destroySky( Ogre::SceneManager *sceneManager )
    {
        std::map<Ogre::SceneManager *, Rectangle2D *>::iterator itor = mSkies.find( sceneManager );
        if( itor != mSkies.end() )
        {
            OGRE_DELETE itor->second;
            mSkies.erase( itor );
        }
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setLight( Light *light )
    {
        mLinkedLight = light;
        syncToLight();
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setSunDir( const Radian sunAltitude, const Radian azimuth )
    {
        float sinSunAzimuth = std::sin( azimuth.valueRadians() );
        float cosSunAzimuth = std::cos( azimuth.valueRadians() );
        float sinSunAltitude = std::sin( sunAltitude.valueRadians() );
        float cosSunAltitude = std::cos( sunAltitude.valueRadians() );

        Ogre::Vector3 sunDir( sinSunAzimuth * cosSunAltitude, sinSunAltitude,
                              cosSunAzimuth * cosSunAltitude );
        setSunDir( -sunDir, sunAltitude.valueRadians() / Math::PI );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setSunDir( const Ogre::Vector3 &sunDir, const float normalizedTimeOfDay )
    {
        mSunDir = -sunDir;
        mSunDir.normalise();

        mNormalizedTimeOfDay = std::min( normalizedTimeOfDay, 1.0f - 1e-6f );

        setPackedParams();
        syncToLight();
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setPresets( const PresetArray &presets )
    {
        mPresets = presets;

        PresetArray::const_iterator itor = mPresets.begin();
        PresetArray::const_iterator endt = mPresets.end();

        while( itor != endt )
        {
            if( fabsf( itor->time ) > 1.0f )
            {
                LogManager::getSingleton().logMessage(
                    "Preset outside range [-1; 1] in AtmosphereNpr::setPresets" );
            }
            ++itor;
        }

        std::sort( mPresets.begin(), mPresets.end(), Preset() );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::updatePreset( const Vector3 &sunDir, const float fTime )
    {
        if( mPresets.empty() )
        {
            LogManager::getSingleton().logMessage(
                "AtmosphereNpr::updatePreset but mPresets is empty!" );
            return;
        }

        // Find presets
        PresetArray::const_iterator itor =
            std::lower_bound( mPresets.begin(), mPresets.end(), fTime, Preset() );

        if( itor == mPresets.end() )
            itor = mPresets.end() - 1u;

        float prevTime = itor->time;
        PresetArray::const_iterator prevIt = itor;
        if( prevIt != mPresets.begin() )
        {
            --prevIt;
            prevTime = prevIt->time;
        }
        else
        {
            prevIt = mPresets.end() - 1u;
            prevTime = prevIt->time - 2.0f;
        }

        // Interpolate
        float timeLength = ( itor->time - prevTime );
        if( timeLength == 0.0f )
            timeLength = 1.0f;

        Preset result;
        const float fW = Math::saturate( ( fTime - prevTime ) / timeLength );
        result.lerp( *prevIt, *itor, fW );

        // Manually set the sun dir so that later setPreset syncs to light
        mSunDir = -sunDir;
        mSunDir.normalise();

        mNormalizedTimeOfDay = std::min( fabsf( result.time ), 1.0f - 1e-6f );

        // Set the interpolated result
        setPreset( result );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setPreset( const Preset &preset )
    {
        mPreset = preset;
        setPackedParams();
        syncToLight();
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::_update( SceneManager *sceneManager, Camera *camera )
    {
        const Vector3 *corners = camera->getWorldSpaceCorners();
        const Vector3 &cameraPos = camera->getDerivedPosition();

        const Real invFarPlane = 1.0f / camera->getFarClipDistance();
        Vector3 cameraDirs[4];
        cameraDirs[0] = ( corners[5] - cameraPos ) * invFarPlane;
        cameraDirs[1] = ( corners[6] - cameraPos ) * invFarPlane;
        cameraDirs[2] = ( corners[4] - cameraPos ) * invFarPlane;
        cameraDirs[3] = ( corners[7] - cameraPos ) * invFarPlane;

        std::map<Ogre::SceneManager *, Rectangle2D *>::iterator itor = mSkies.find( sceneManager );
        OGRE_ASSERT_LOW( itor != mSkies.end() );
        Rectangle2D *sky = itor->second;

        sky->setNormals( cameraDirs[0], cameraDirs[1], cameraDirs[2], cameraDirs[3] );
        sky->update();

        Vector3 cameraDisplacement( Vector3::ZERO );
        {
            float camHeight = cameraPos[mConvention & 0x03u];
            camHeight *= ( mConvention & NegationFlag ) ? -1.0f : 1.0f;
            camHeight = ( camHeight - mAtmosphereSeaLevel ) / mAtmosphereHeight;
            cameraDisplacement[mConvention & 0x03u] = camHeight;

            GpuProgramParametersSharedPtr psParams = mPass->getFragmentProgramParameters();
            psParams->setNamedConstant( "cameraDisplacement", cameraDisplacement );
        }

        AtmoSettingsGpu atmoSettingsGpu;

        const float sunHeight = std::sin( mNormalizedTimeOfDay * Math::PI );
        const float sunHeightWeight = std::exp2( -1.0f / sunHeight );
        // Gets smaller as sunHeight gets bigger
        const float lightDensity =
            mPreset.densityCoeff / std::pow( std::max( sunHeight, 0.0035f ), 0.75f );

        atmoSettingsGpu.densityCoeff = mPreset.densityCoeff;
        atmoSettingsGpu.lightDensity = lightDensity;
        atmoSettingsGpu.sunHeight = sunHeight;
        atmoSettingsGpu.sunHeightWeight = sunHeightWeight;

        const Vector3 mieAbsorption =
            std::pow( std::max( 1.0f - lightDensity, 0.1f ), 4.0f ) *
            Math::lerp( mPreset.skyColour, Vector3::UNIT_SCALE, sunHeightWeight );
        const float finalMultiplier =
            ( 0.5f + Math::smoothstep( 0.02f, 0.4f, sunHeightWeight ) ) * mPreset.skyPower;
        const Vector4 packedParams1( mieAbsorption, finalMultiplier );
        const Vector4 packedParams2( mSunDir, mPreset.horizonLimit );
        const Vector4 packedParams3( mPreset.skyColour, mPreset.densityDiffusion );

        atmoSettingsGpu.skyLightAbsorption =
            Vector4( getSkyRayleighAbsorption( mPreset.skyColour, lightDensity ), cameraPos.x );
        atmoSettingsGpu.sunAbsorption =
            Vector4( getSkyRayleighAbsorption( 1.0f - mPreset.skyColour, lightDensity ), cameraPos.y );
        atmoSettingsGpu.cameraDisplacement = Vector4( cameraDisplacement, cameraPos.z );
        atmoSettingsGpu.packedParams1 = packedParams1;
        atmoSettingsGpu.packedParams2 = packedParams2;
        atmoSettingsGpu.packedParams3 = packedParams3;
        atmoSettingsGpu.fogDensity = mPreset.fogDensity;
        atmoSettingsGpu.fogBreakMinBrightness = mPreset.fogBreakMinBrightness * mPreset.fogBreakFalloff;
        atmoSettingsGpu.fogBreakFalloff = -mPreset.fogBreakFalloff;

        mHlmsBuffer->upload( &atmoSettingsGpu, 0u, sizeof( atmoSettingsGpu ) );
    }
    //-------------------------------------------------------------------------
    uint32 AtmosphereNpr::preparePassHash( Hlms *hlms, size_t constBufferSlot )
    {
        hlms->_setProperty( Hlms::kNoTid, HlmsBaseProp::Fog, 1 );
        hlms->_setProperty( Hlms::kNoTid, "atmosky_npr", int32( constBufferSlot ) );
        return 1u;
    }
    //-------------------------------------------------------------------------
    uint32 AtmosphereNpr::getNumConstBuffersSlots() const { return 1u; }
    //-------------------------------------------------------------------------
    uint32 AtmosphereNpr::bindConstBuffers( CommandBuffer *commandBuffer, size_t slotIdx )
    {
        *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
            VertexShader, uint16( slotIdx ), mHlmsBuffer, 0, (uint32)mHlmsBuffer->getTotalSizeBytes() );
        *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
            PixelShader, uint16( slotIdx ), mHlmsBuffer, 0, (uint32)mHlmsBuffer->getTotalSizeBytes() );

        return 1u;
    }
    //-------------------------------------------------------------------------
    inline float getSunDisk( const float LdotV, const float sunY, const float sunPower )
    {
        return std::pow( LdotV, Math::lerp( 4.0f, 8500.0f, sunY ) ) * sunPower;
    }
    //-------------------------------------------------------------------------
    inline float getMie( const float LdotV ) { return LdotV; }
    //-------------------------------------------------------------------------
    inline Vector3 pow3( Vector3 v, float e )
    {
        return Vector3( std::pow( v.x, e ), std::pow( v.y, e ), std::pow( v.z, e ) );
    }
    //-------------------------------------------------------------------------
    inline Vector3 exp2( Vector3 v )
    {
        return Vector3( std::exp2( v.x ), std::exp2( v.y ), std::exp2( v.z ) );
    }
    //-------------------------------------------------------------------------
    Vector3 AtmosphereNpr::getAtmosphereAt( const Vector3 cameraDir, bool bSkipSun )
    {
        Vector3 atmoCameraDir = cameraDir.normalisedCopy();

        const Vector3 p_sunDir = mSunDir;
        const float p_densityDiffusion = mPreset.densityDiffusion;
        const float p_borderLimit = mPreset.horizonLimit;
        const float p_densityCoeff = mPreset.densityCoeff;
        const float p_sunHeight = std::sin( mNormalizedTimeOfDay * Math::PI );
        const float p_sunHeightWeight = std::exp2( -1.0f / p_sunHeight );
        // Gets smaller as sunHeight gets bigger
        const float p_lightDensity =
            mPreset.densityCoeff / std::pow( std::max( p_sunHeight, 0.0035f ), 0.75f );
        const Vector3 p_skyColour = mPreset.skyColour;
        const float p_finalMultiplier =
            ( 0.5f + Math::smoothstep( 0.02f, 0.4f, p_sunHeightWeight ) ) * mPreset.skyPower;
        const Vector3 p_sunAbsorption =
            getSkyRayleighAbsorption( 1.0f - mPreset.skyColour, p_lightDensity );
        const Vector3 p_mieAbsorption =
            std::pow( std::max( 1.0f - p_lightDensity, 0.1f ), 4.0f ) *
            Math::lerp( mPreset.skyColour, Vector3::UNIT_SCALE, p_sunHeightWeight );
        const Vector3 p_skyLightAbsorption =
            getSkyRayleighAbsorption( mPreset.skyColour, p_lightDensity );

        const float LdotV = std::max( atmoCameraDir.dotProduct( p_sunDir ), 0.0f );

        atmoCameraDir.y +=
            p_densityDiffusion * 0.075f * ( 1.0f - atmoCameraDir.y ) * ( 1.0f - atmoCameraDir.y );
        atmoCameraDir.normalise();

        atmoCameraDir.y = std::max( atmoCameraDir.y, p_borderLimit );

        // It's not a mistake. We do this twice. Doing it before p_borderLimit
        // allows the horizon to shift. Doing it after p_borderLimit lets
        // the sky to get darker as we get upwards.
        atmoCameraDir.normalise();

        const float LdotV360 = atmoCameraDir.dotProduct( p_sunDir ) * 0.5f + 0.5f;

        // ptDensity gets smaller as sunHeight gets bigger
        // ptDensity gets smaller as atmoCameraDir.y gets bigger
        const float ptDensity =
            p_densityCoeff /
            std::pow( std::max( atmoCameraDir.y / ( 1.0f - p_sunHeight ), 0.0035f ),
                      Math::lerp( 0.10f, p_densityDiffusion, std::pow( atmoCameraDir.y, 0.3f ) ) );

        const float sunDisk = getSunDisk( LdotV, p_sunHeight, mPreset.sunPower );

        const float antiMie = std::max( p_sunHeightWeight, 0.08f );

        const Vector3 skyAbsorption = getSkyRayleighAbsorption( p_skyColour, ptDensity );
        const Vector3 skyColourGradient = pow3( exp2( -atmoCameraDir.y / p_skyColour ), 1.5f );

        const float mie = getMie( LdotV360 );

        Vector3 atmoColour = Vector3( 0.0f, 0.0f, 0.0f );

        const Vector3 sharedTerms = skyColourGradient * skyAbsorption;

        atmoColour += antiMie * sharedTerms * p_sunAbsorption;
        atmoColour += ( mie * ptDensity * p_lightDensity ) * sharedTerms * p_skyLightAbsorption;
        atmoColour += mie * p_mieAbsorption;
        atmoColour *= p_lightDensity;

        atmoColour *= p_finalMultiplier;
        if( !bSkipSun )
            atmoColour += sunDisk * p_skyLightAbsorption;

        return atmoColour;
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::Preset::lerp( const Preset &a, const Preset &b, const float w )
    {
#define LERP_VALUE( x ) this->x = Math::lerp( a.x, b.x, w )
        if( a.time > b.time )
        {
            this->time = Math::lerp( a.time - 2.0f, b.time, w );
            if( this->time < -1.0f )
                this->time += 2.0f;
        }
        else
        {
            LERP_VALUE( time );
        }
        LERP_VALUE( densityCoeff );
        LERP_VALUE( densityDiffusion );
        LERP_VALUE( horizonLimit );
        LERP_VALUE( sunPower );
        LERP_VALUE( skyPower );
        LERP_VALUE( skyColour );
        LERP_VALUE( fogDensity );
        LERP_VALUE( fogBreakMinBrightness );
        LERP_VALUE( fogBreakFalloff );
        LERP_VALUE( linkedLightPower );
        LERP_VALUE( linkedSceneAmbientUpperPower );
        LERP_VALUE( linkedSceneAmbientLowerPower );
        LERP_VALUE( envmapScale );
#undef LERP_VALUE
    }
}  // namespace Ogre
