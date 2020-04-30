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

#include "OgreShadowCameraSetupConcentric.h"

#include "OgreCamera.h"
#include "OgreConvexBody.h"
#include "OgreLight.h"
#include "OgreLogManager.h"
#include "OgrePlane.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreStableHeaders.h"

namespace Ogre
{
    ConcentricShadowCamera::ConcentricShadowCamera( void ) {}
    //-----------------------------------------------------------------------
    ConcentricShadowCamera::~ConcentricShadowCamera( void ) {}
    //-----------------------------------------------------------------------
    void ConcentricShadowCamera::getShadowCamera( const SceneManager *sm, const Camera *cam,
                                                  const Light *light, Camera *texCam, size_t iteration,
                                                  const Vector2 &viewportRealSize ) const
    {
        // check availability - viewport not needed
        OgreAssert( sm != NULL, "SceneManager is NULL" );
        OgreAssert( cam != NULL, "Camera (viewer) is NULL" );
        OgreAssert( light != NULL, "Light is NULL" );
        OgreAssert( texCam != NULL, "Camera (texture) is NULL" );

        if( light->getType() != Light::LT_DIRECTIONAL )
        {
            DefaultShadowCameraSetup::getShadowCamera( sm, cam, light, texCam, iteration,
                                                       viewportRealSize );
            return;
        }

        texCam->setNearClipDistance( light->_deriveShadowNearClipDistance( cam ) );
        texCam->setFarClipDistance( light->_deriveShadowFarClipDistance( cam ) );

        const AxisAlignedBox &casterBox = sm->getCurrentCastersBox();

        // Will be overriden, but not always (in case we early out to use uniform shadows)
        mMaxDistance = casterBox.getMinimum().distance( casterBox.getMaximum() );

        // in case the casterBox is empty (e.g. there are no casters) simply
        // return the standard shadow mapping matrix
        if( casterBox.isNull() )
        {
            texCam->setProjectionType( PT_ORTHOGRAPHIC );
            // Anything will do, there are no casters. But we must ensure depth of the receiver
            // doesn't become negative else a shadow square will appear (i.e. "the sun is below the
            // floor")
            const Real farDistance =
                Ogre::min( cam->getFarClipDistance(), light->getShadowFarDistance() );
            texCam->setPosition( cam->getDerivedPosition() -
                                 light->getDerivedDirection() * farDistance );
            texCam->setOrthoWindow( 1, 1 );
            texCam->setNearClipDistance( 1.0f );
            texCam->setFarClipDistance( 1.1f );

            texCam->getWorldAabbUpdated();

            mMinDistance = 1.0f;
            mMaxDistance = 1.1f;
            return;
        }

        const Node *lightNode = light->getParentNode();
        const Real farDistance = Ogre::min( cam->getFarClipDistance(), light->getShadowFarDistance() );
        const Quaternion scalarLightSpaceToWorld( lightNode->_getDerivedOrientation() );
        const Quaternion scalarWorldToLightSpace( scalarLightSpaceToWorld.Inverse() );
        ArrayQuaternion worldToLightSpace;
        worldToLightSpace.setAll( scalarWorldToLightSpace );

        ArrayVector3 vMinBounds( Mathlib::MAX_POS, Mathlib::MAX_POS, Mathlib::MAX_POS );
        ArrayVector3 vMaxBounds( Mathlib::MAX_NEG, Mathlib::MAX_NEG, Mathlib::MAX_NEG );

#define NUM_ARRAY_VECTORS ( 8 + ARRAY_PACKED_REALS - 1 ) / ARRAY_PACKED_REALS

        // Take the 8 camera frustum's corners, transform to
        // light space, and compute its AABB in light space
        ArrayVector3 corners[NUM_ARRAY_VECTORS];
        cam->getCustomWorldSpaceCorners( corners, farDistance );

        for( size_t i = 0; i < NUM_ARRAY_VECTORS; ++i )
        {
            ArrayVector3 lightSpacePoint = worldToLightSpace * corners[i];
            vMinBounds.makeFloor( lightSpacePoint );
            vMaxBounds.makeCeil( lightSpacePoint );
        }

        Vector3 vMinCamFrustumLS = vMinBounds.collapseMin();
        Vector3 vMaxCamFrustumLS = vMaxBounds.collapseMax();

        Vector3 casterAabbCornersLS[8];
        for( size_t i = 0; i < 8; ++i )
        {
            casterAabbCornersLS[i] = scalarWorldToLightSpace *
                                     casterBox.getCorner( static_cast<AxisAlignedBox::CornerEnum>( i ) );
        }

        ConvexBody convexBody;
        convexBody.define( casterAabbCornersLS );

        Plane p;
        p.redefine( Vector3::NEGATIVE_UNIT_Z, vMinCamFrustumLS );
        convexBody.clip( p );

        Vector3 vMin( std::numeric_limits<Real>::max(), std::numeric_limits<Real>::max(),
                      std::numeric_limits<Real>::max() );
        Vector3 vMax( -std::numeric_limits<Real>::max(), -std::numeric_limits<Real>::max(),
                      -std::numeric_limits<Real>::max() );

        for( size_t i = 0; i < convexBody.getPolygonCount(); ++i )
        {
            const Polygon &polygon = convexBody.getPolygon( i );

            for( size_t j = 0; j < polygon.getVertexCount(); ++j )
            {
                const Vector3 &point = polygon.getVertex( j );
                vMin.makeFloor( point );
                vMax.makeCeil( point );
            }
        }

        if( vMin > vMax )
        {
            // There are no casters that will affect the viewing frustum
            //(or something went wrong with the clipping).
            // Rollback to something valid
            vMin = vMinCamFrustumLS;
            vMax = vMaxCamFrustumLS;

            // Add some padding to prevent negative depth (i.e. "the sun is below the floor")
            vMax.z += 5.0f;  // Backwards is towards +Z!
        }

        const float padding = 3.0f;
        Aabb aabb( scalarWorldToLightSpace * cam->getDerivedPosition(),
                   Ogre::Vector3( farDistance + padding ) );

        const float zPadding = 2.0f;

        texCam->setProjectionType( PT_ORTHOGRAPHIC );
        Vector3 shadowCameraPos = aabb.mCenter;
        shadowCameraPos.z = vMax.z + zPadding;  // Backwards is towards +Z!

        // Round local x/y position based on a world-space texel; this helps to reduce
        // jittering caused by the projection moving with the camera
        const Real worldTexelSizeX = ( texCam->getOrthoWindowWidth() ) / viewportRealSize.x;
        const Real worldTexelSizeY = ( texCam->getOrthoWindowHeight() ) / viewportRealSize.y;

        // snap to nearest texel
        shadowCameraPos.x -= fmod( shadowCameraPos.x, worldTexelSizeX );
        shadowCameraPos.y -= fmod( shadowCameraPos.y, worldTexelSizeY );

        // Go back from light space to world space
        shadowCameraPos = scalarLightSpaceToWorld * shadowCameraPos;

        texCam->setPosition( shadowCameraPos );
        texCam->setOrthoWindow( aabb.mHalfSize.x * 2.0f, aabb.mHalfSize.y * 2.0f );

        mMinDistance = 1.0f;
        mMaxDistance =
            vMax.z - vMin.z + zPadding;  // We just went backwards, we need to enlarge our depth
        texCam->setNearClipDistance( mMinDistance );
        texCam->setFarClipDistance( mMaxDistance );

        // Update the AABB. Note: Non-shadow caster cameras are forbidden to change mid-render
        texCam->getWorldAabbUpdated();
    }
}  // namespace Ogre
