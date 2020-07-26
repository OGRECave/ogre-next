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
#include "OgrePixelCountLodStrategy.h"

#include "OgreViewport.h"
#include "OgreCamera.h"

#include "OgreLodStrategyPrivate.inl"

#include <limits>

namespace Ogre {
    //-----------------------------------------------------------------------
    PixelCountLodStrategyBase::PixelCountLodStrategyBase(const String& name)
        : LodStrategy(name)
    { }
    //---------------------------------------------------------------------
    Real PixelCountLodStrategyBase::getBaseValue() const
    {
        // Use the maximum possible value as base
        return -std::numeric_limits<Real>::max();
    }
    //---------------------------------------------------------------------
    Real PixelCountLodStrategyBase::transformBias(Real factor) const
    {
        // No transformation required for pixel count strategy
        return factor;
    }

    /************************************************************************/
    /*  AbsolutPixelCountLodStrategy                                        */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> AbsolutePixelCountLodStrategy* Singleton<AbsolutePixelCountLodStrategy>::msSingleton = 0;
    AbsolutePixelCountLodStrategy* AbsolutePixelCountLodStrategy::getSingletonPtr(void)
    {
        return msSingleton;
    }
    AbsolutePixelCountLodStrategy& AbsolutePixelCountLodStrategy::getSingleton(void)
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    AbsolutePixelCountLodStrategy::AbsolutePixelCountLodStrategy()
        : PixelCountLodStrategyBase("pixel_count")
    { }
    //-----------------------------------------------------------------------
    Real AbsolutePixelCountLodStrategy::getValueImpl( const MovableObject *movableObject,
                                                      const Ogre::Camera *camera ) const
    {
        // Get viewport
        const Viewport *viewport = camera->getLastViewport();

        // Get viewport area
        const Real viewportHeight = static_cast<Real>( viewport->getActualHeight() );
        const Real viewportArea = static_cast<Real>( viewport->getActualWidth() ) * viewportHeight;

        // Get area of unprojected circle with object bounding radius
        Real boundingArea = Math::PI * Math::Sqr(movableObject->getWorldRadius());

        // Base computation on projection type
        switch (camera->getProjectionType())
        {
        case PT_PERSPECTIVE:
            {
                // Get camera distance
                Real distanceSquared = movableObject->getParentNode()->getSquaredViewDepth(camera);

                // Check for 0 distance
                if (distanceSquared <= std::numeric_limits<Real>::epsilon())
                    return getBaseValue();

                // Get projection matrix (this is done to avoid computation of tan(FOV / 2))
                const Matrix4& projMat = camera->getProjectionMatrix();

                // Estimate pixel count
                return -( boundingArea * projMat[1][1] * projMat[1][1] * viewportHeight *
                          viewportHeight ) /
                       ( 4.0f * distanceSquared );
            }
        case PT_ORTHOGRAPHIC:
            {
                // Compute orthographic area
                Real orthoArea = camera->getOrthoWindowHeight() * camera->getOrthoWindowWidth();

                // Check for 0 orthographic area
                if (orthoArea <= std::numeric_limits<Real>::epsilon())
                    return getBaseValue();

                // Estimate pixel count
                return -(boundingArea * viewportArea) / orthoArea;
            }
        default:
            {
                // This case is not covered for obvious reasons
                throw;
            }
        }
    }
    //-----------------------------------------------------------------------
    void AbsolutePixelCountLodStrategy::lodUpdateImpl( const size_t numNodes, ObjectData objData,
                                                       const Camera *camera, Real bias ) const
    {
        const Viewport *viewport = camera->getLastViewport();
        const Real viewportHeight = static_cast<Real>( viewport->getActualHeight() );
        ArrayVector3 cameraPos;
        cameraPos.setAll( camera->_getCachedDerivedPosition() );

        const Matrix4 &projMat = camera->getProjectionMatrix();
        OGRE_ALIGNED_DECL( Real, lodValues[ARRAY_PACKED_REALS], OGRE_SIMD_ALIGNMENT );

        if( camera->getProjectionType() == PT_PERSPECTIVE )
        {
            // See ScreenRatioPixelCountLodStrategy::lodUpdateImpl for formula derivation
            // constTerm is negative so we can store Lod values in ascending
            // order and use lower_bound (which wouldn't be the same as using upper_bound)
            const ArrayReal constTerm( Mathlib::SetAll( -Math::PI * projMat[1][1] * projMat[1][1] *
                                                        viewportHeight * viewportHeight *
                                                        camera->getLodBias() * bias / 4.0f ) );

            for( size_t i=0; i<numNodes; i += ARRAY_PACKED_REALS )
            {
                ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
                                                                            (objData.mWorldRadius);
                ArrayReal sqDistance = objData.mWorldAabb->mCenter.squaredDistance( cameraPos );

                //Avoid division by zero
                sqDistance = Mathlib::Max( sqDistance, Mathlib::fEpsilon );

                // Get area of unprojected circle with object bounding radius
                ArrayReal sqRadius = (*worldRadius * *worldRadius);
                ArrayReal arrayLodValue = (sqRadius * constTerm) / sqDistance;

                CastArrayToReal( lodValues, arrayLodValue );

                lodSet( objData, lodValues );

                objData.advanceLodPack();
            }
        }
        else
        {
            const Real viewportArea = static_cast<Real>( viewport->getActualWidth() ) * viewportHeight;
            Real orthoArea = camera->getOrthoWindowHeight() * camera->getOrthoWindowWidth();

            //Avoid division by zero
            orthoArea = Ogre::max( orthoArea, Real(1e-6) );

            //vpAreaDotProjMat00dot11 is negative so we can store Lod values in ascending
            //order and use lower_bound (which wouldn't be the same as using upper_bound)
            ArrayReal PiDotVpAreaDivOrhtoArea( Mathlib::SetAll( -Math::PI * viewportArea / orthoArea ) );

            ArrayReal lodBias( Mathlib::SetAll( camera->getLodBias() * bias ) );

            for( size_t i=0; i<numNodes; i += ARRAY_PACKED_REALS )
            {
                ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
                                                                            (objData.mWorldRadius);
                ArrayReal arrayLodValue = (*worldRadius * *worldRadius) *
                                            PiDotVpAreaDivOrhtoArea * lodBias;
                CastArrayToReal( lodValues, arrayLodValue );

                lodSet( objData, lodValues );

                objData.advanceLodPack();
            }
        }
    }
    //-----------------------------------------------------------------------

    /************************************************************************/
    /* ScreenRatioPixelCountLodStrategy                                     */
    /************************************************************************/

    //-----------------------------------------------------------------------
    template<> ScreenRatioPixelCountLodStrategy* Singleton<ScreenRatioPixelCountLodStrategy>::msSingleton = 0;
    ScreenRatioPixelCountLodStrategy* ScreenRatioPixelCountLodStrategy::getSingletonPtr(void)
    {
        return msSingleton;
    }
    ScreenRatioPixelCountLodStrategy& ScreenRatioPixelCountLodStrategy::getSingleton(void)
    {
        assert( msSingleton );  return ( *msSingleton );
    }
    //-----------------------------------------------------------------------
    ScreenRatioPixelCountLodStrategy::ScreenRatioPixelCountLodStrategy()
        : PixelCountLodStrategyBase("screen_ratio_pixel_count")
    { }
    //-----------------------------------------------------------------------
    Real ScreenRatioPixelCountLodStrategy::getValueImpl( const MovableObject *movableObject,
                                                         const Ogre::Camera *camera ) const
    {
        // Get absolute pixel count
        Real absoluteValue =
            AbsolutePixelCountLodStrategy::getSingletonPtr()->getValueImpl( movableObject, camera );

        // Get viewport area
        const Viewport *viewport = camera->getLastViewport();
        Real viewportArea =
            static_cast<Real>( viewport->getActualWidth() * viewport->getActualHeight() );

        // Return ratio of screen size to absolutely covered pixel count
        return absoluteValue / viewportArea;
    }
    //-----------------------------------------------------------------------
    void ScreenRatioPixelCountLodStrategy::lodUpdateImpl( const size_t numNodes, ObjectData objData,
                                                          const Camera *camera, Real bias ) const
    {
        ArrayVector3 cameraPos;
        cameraPos.setAll( camera->_getCachedDerivedPosition() );

        const Matrix4 &projMat = camera->getProjectionMatrix();
        OGRE_ALIGNED_DECL( Real, lodValues[ARRAY_PACKED_REALS], OGRE_SIMD_ALIGNMENT );

        if( camera->getProjectionType() == PT_PERSPECTIVE )
        {
            // See:
            //  https://forums.ogre3d.org/viewtopic.php?p=548059#p548059
            //  https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
            //
            // Note: _sp means screen_space; _ws means world_space
            //
            //             cot( fovy / 2 ) * radius_ws   viewport_height
            // radius_sp = --------------------------- * ---------------
            //                      distance                    2
            //
            //                 cot( fovy / 2 ) * radius_ws   viewport_height
            // area_sp = PI *( --------------------------- * --------------- )²
            //                         distance                      2
            //
            // Since cot(fovy / 2) = 1 / tan(fovy / 2) = projMatrix[1][1], thus:
            //
            //            PI * projMatrix[1][1]² * radius_ws² * viewport_height²
            // area_sp = --------------------------------------------------------
            //                                4 * distance²
            //
            // Divide all by the total pixel count (viewport_height * viewport_width)
            // to get screen coverage:
            //
            //               PI * projMatrix[1][1]² * radius_ws² * viewport_height
            // screen_cvg = -------------------------------------------------------
            //                        4 * distance² * viewport_width
            //
            // Since projMatrix[0][0] = projMatrix[1][1] * viewport_width / viewport_height, thus:
            //
            //               PI * projMatrix[0][0] * projMatrix[1][1] * radius_ws²
            // screen_cvg = -------------------------------------------------------
            //                                 4 * distance²
            //
            // From this formula, only distance and radius varies per object.
            // constTerm is negative so we can store Lod values in ascending
            // order and use lower_bound (which wouldn't be the same as using upper_bound)
            const ArrayReal constTerm( Mathlib::SetAll( -Math::PI * projMat[0][0] * projMat[1][1] *
                                                        camera->getLodBias() * bias / 4.0f ) );

            for( size_t i=0; i<numNodes; i += ARRAY_PACKED_REALS )
            {
                ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
                                                                            (objData.mWorldRadius);
                ArrayReal sqDistance = objData.mWorldAabb->mCenter.squaredDistance( cameraPos );

                //Avoid division by zero
                sqDistance = Mathlib::Max( sqDistance, Mathlib::fEpsilon );

                // Get area of unprojected circle with object bounding radius
                ArrayReal sqRadius = (*worldRadius * *worldRadius);
                ArrayReal arrayLodValue = (sqRadius * constTerm) / sqDistance;

                CastArrayToReal( lodValues, arrayLodValue );

                lodSet( objData, lodValues );

                objData.advanceLodPack();
            }
        }
        else
        {
            Real orthoArea = camera->getOrthoWindowHeight() * camera->getOrthoWindowWidth();

            //Avoid division by zero
            orthoArea = Ogre::max( orthoArea, Real(1e-6) );

            //vpAreaDotProjMat00dot11 is negative so we can store Lod values in ascending
            //order and use lower_bound (which wouldn't be the same as using upper_bound)
            ArrayReal PiDotVpAreaDivOrhtoArea( Mathlib::SetAll( -Math::PI / orthoArea ) );

            ArrayReal lodBias( Mathlib::SetAll( camera->getLodBias() * bias ) );

            for( size_t i=0; i<numNodes; i += ARRAY_PACKED_REALS )
            {
                ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
                                                                            (objData.mWorldRadius);
                ArrayReal arrayLodValue = (*worldRadius * *worldRadius) *
                                            PiDotVpAreaDivOrhtoArea * lodBias;
                CastArrayToReal( lodValues, arrayLodValue );

                lodSet( objData, lodValues );

                objData.advanceLodPack();
            }
        }
    }
    //-----------------------------------------------------------------------

} // namespace
