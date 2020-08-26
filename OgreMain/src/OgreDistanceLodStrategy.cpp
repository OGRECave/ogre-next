/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "OgreStableHeaders.h"
#include "OgreDistanceLodStrategy.h"
#include "OgreCamera.h"
#include "OgreNode.h"
#include "OgreViewport.h"

#include "OgreCamera.h"

#include <limits>

#include "OgreLodStrategyPrivate.inl"
namespace Ogre {
    DistanceLodStrategyBase::DistanceLodStrategyBase(const String& name)
        : LodStrategy(name)
        , mReferenceViewEnabled(false)
        , mReferenceViewValue(-1)
    { }
    //-----------------------------------------------------------------------
    Real DistanceLodStrategyBase::getValueImpl(const MovableObject *movableObject, const Ogre::Camera *camera) const
    {
        Real squaredDepth = getSquaredDepth(movableObject, camera);

        // Check if reference view needs to be taken into account
        if (mReferenceViewEnabled)
        {
            // Reference view only applicable to perspective projection
            assert(camera->getProjectionType() == PT_PERSPECTIVE && "Camera projection type must be perspective!");

            // Get camera viewport
            Viewport *viewport = camera->getLastViewport();

            // Get viewport area
            Real viewportArea = static_cast<Real>(viewport->getActualWidth() * viewport->getActualHeight());

            // Get projection matrix (this is done to avoid computation of tan(FOV / 2))
            const Matrix4& projectionMatrix = camera->getProjectionMatrix();

            // Compute bias value (note that this is similar to the method used for PixelCountLodStrategy)
            Real biasValue = viewportArea * projectionMatrix[0][0] * projectionMatrix[1][1];

            // Scale squared depth appropriately
            squaredDepth *= (mReferenceViewValue / biasValue);
        }

        // Squared depth should never be below 0, so clamp
        squaredDepth = std::max(squaredDepth, Real(0));

        // Now adjust it by the camera bias and return the computed value
        return squaredDepth * camera->_getLodBiasInverse();
    }
    //-----------------------------------------------------------------------
    void DistanceLodStrategyBase::lodUpdateImpl( const size_t numNodes, ObjectData objData,
                                             const Camera *camera, Real bias ) const
    {
        ArrayVector3 cameraPos;
        cameraPos.setAll( camera->_getCachedDerivedPosition() );

        ArrayReal lodInvBias( Mathlib::SetAll( camera->_getLodBiasInverse() * bias ) );
        OGRE_ALIGNED_DECL( Real, lodValues[ARRAY_PACKED_REALS], OGRE_SIMD_ALIGNMENT );

        for( size_t i=0; i<numNodes; i += ARRAY_PACKED_REALS )
        {
            ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
                                                                        (objData.mWorldRadius);
            ArrayReal arrayLodValue = objData.mWorldAabb->mCenter.distance( cameraPos ) - (*worldRadius);
            arrayLodValue = arrayLodValue * lodInvBias;
            CastArrayToReal( lodValues, arrayLodValue );

            lodSet( objData, lodValues );

            objData.advanceLodPack();
        }
    }
    //-----------------------------------------------------------------------
    Real DistanceLodStrategyBase::getBaseValue() const
    {
        return Real(0);
    }
    //---------------------------------------------------------------------
    Real DistanceLodStrategyBase::transformBias(Real factor) const
    {
        assert(factor > 0.0f && "Bias factor must be > 0!");
        return 1.0f / factor;
    }
    //-----------------------------------------------------------------------
    Real DistanceLodStrategyBase::transformUserValue(Real userValue) const
    {
        return userValue;
    }
    //---------------------------------------------------------------------
    void DistanceLodStrategyBase::setReferenceView(Real viewportWidth, Real viewportHeight, Radian fovY)
    {
        // Determine x FOV based on aspect ratio
        Radian fovX = fovY * (viewportWidth / viewportHeight);

        // Determine viewport area
        Real viewportArea = viewportHeight * viewportWidth;

        // Compute reference view value based on viewport area and FOVs
        mReferenceViewValue = viewportArea * Math::Tan(fovX * 0.5f) * Math::Tan(fovY * 0.5f);

        // Enable use of reference view
        mReferenceViewEnabled = true;
    }
    //---------------------------------------------------------------------
    void DistanceLodStrategyBase::setReferenceViewEnabled(bool value)
    {
        // Ensure reference value has been set before being enabled
        if (value)
            assert(mReferenceViewValue != -1 && "Reference view must be set before being enabled!");

        mReferenceViewEnabled = value;
    }
    //---------------------------------------------------------------------
    bool DistanceLodStrategyBase::isReferenceViewEnabled() const
    {
        return mReferenceViewEnabled;
    }

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> DistanceLodSphereStrategy* Singleton<DistanceLodSphereStrategy>::msSingleton = 0;
    DistanceLodSphereStrategy* DistanceLodSphereStrategy::getSingletonPtr(void)
    {
        return msSingleton;
    }
    DistanceLodSphereStrategy& DistanceLodSphereStrategy::getSingleton(void)
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    DistanceLodSphereStrategy::DistanceLodSphereStrategy()
        : DistanceLodStrategyBase("distance_sphere")
    { }
    //-----------------------------------------------------------------------
    Real DistanceLodSphereStrategy::getSquaredDepth(const MovableObject *movableObject, const Ogre::Camera *camera) const
    {
        // Get squared depth taking into account bounding radius
        // (d - r) ^ 2 = d^2 - 2dr + r^2, but this requires a lot 
        // more computation (including a sqrt) so we approximate 
        // it with d^2 - r^2, which is good enough for determining 
        // LOD.
        return movableObject->getParentNode()->getSquaredViewDepth(camera) - Math::Sqr(movableObject->getWorldRadius());
    }
    //-----------------------------------------------------------------------

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> DistanceLodBoxStrategy* Singleton<DistanceLodBoxStrategy>::msSingleton = 0;
    DistanceLodBoxStrategy* DistanceLodBoxStrategy::getSingletonPtr(void)
    {
        return msSingleton;
    }
    DistanceLodBoxStrategy& DistanceLodBoxStrategy::getSingleton(void)
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    DistanceLodBoxStrategy::DistanceLodBoxStrategy()
        : DistanceLodStrategyBase("distance_box")
    { }
    //-----------------------------------------------------------------------
    Real DistanceLodBoxStrategy::getSquaredDepth(const MovableObject *movableObject, const Ogre::Camera *camera) const
    {
        return Math::Sqr(movableObject->getWorldAabb().distance(camera->getPosition()));
    }
    //-----------------------------------------------------------------------

} // namespace
