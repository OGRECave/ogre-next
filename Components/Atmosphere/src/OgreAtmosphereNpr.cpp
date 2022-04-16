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

#include "OgreCamera.h"
#include "OgreMaterialManager.h"
#include "OgrePass.h"
#include "OgreRectangle2D2.h"
#include "OgreSceneManager.h"
#include "OgreTechnique.h"

namespace Ogre
{
    AtmosphereNpr::AtmosphereNpr() :
        mSunDir( Ogre::Vector3( 0, 1, 1 ).normalisedCopy() ),
        mNormalizedTimeOfDay( std::asin( mSunDir.y ) ),
        mLinkedLight( 0 ),
        mPass( 0 )
    {
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
                         "To use the sky, bundle the resources included in Samples/Media/Common",
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
        const float finalMultiplier = 0.5f + Math::smoothstep( 0.02f, 0.4f, sunHeightWeight );
        const Vector4 packedParams1( mieAbsorption, finalMultiplier );

        GpuProgramParametersSharedPtr psParams = mPass->getFragmentProgramParameters();

        psParams->setNamedConstant( "packedParams0", packedParams0 );
        psParams->setNamedConstant( "skyColour", mPreset.skyColour );
        psParams->setNamedConstant( "sunDir", mSunDir );
        psParams->setNamedConstant( "skyLightAbsorption",
                                    getSkyRayleighAbsorption( mPreset.skyColour, lightDensity ) );
        psParams->setNamedConstant( "sunAbsorption",
                                    getSkyRayleighAbsorption( 1.0f - mPreset.skyColour, lightDensity ) );
        psParams->setNamedConstant( "packedParams1", packedParams1 );
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
        if( mLinkedLight )
        {
            mLinkedLight->setType( Light::LT_DIRECTIONAL );
            mLinkedLight->setDirection( -mSunDir );
        }
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
        setSunDir( sunDir, sunAltitude.valueRadians() / Math::PI );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setSunDir( const Ogre::Vector3 &sunDir, const float normalizedTimeOfDay )
    {
        mSunDir = sunDir;
        mSunDir.normalise();

        mNormalizedTimeOfDay = normalizedTimeOfDay;
        mNormalizedTimeOfDay = std::min( normalizedTimeOfDay, 1.0f - 1e-6f );

        setPackedParams();

        if( mLinkedLight )
            mLinkedLight->setDirection( -mSunDir );
    }
    //-------------------------------------------------------------------------
    void AtmosphereNpr::setPreset( const Preset &preset )
    {
        mPreset = preset;
        setPackedParams();
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
    }
}  // namespace Ogre
