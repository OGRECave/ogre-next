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
#ifndef __Pixel_Count_Lod_Strategy_H__
#define __Pixel_Count_Lod_Strategy_H__

#include "OgrePrerequisites.h"

#include "OgreLodStrategy.h"
#include "OgreSingleton.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup LOD
     *  @{
     */

    class AbsolutePixelCountLodStrategy;
    /// Backward compatible name for Distance_Box strategy.
    typedef AbsolutePixelCountLodStrategy PixelCountLodStrategy;

    /** Abstract base class for level of detail strategy based on pixel count approximations from
     * bounding sphere projection. */
    class _OgreExport PixelCountLodStrategyBase : public LodStrategy
    {
    public:
        /** Default constructor. */
        PixelCountLodStrategyBase( const String &name );

        /// @copydoc LodStrategy::getBaseValue
        Real getBaseValue() const override;

        /// @copydoc LodStrategy::transformBias
        Real transformBias( Real factor ) const override;

        /** Transform user supplied value to internal value.
        @remarks
            Do not throw exceptions for invalid values here, as the LOD strategy
            may be changed such that the values become valid.
        */
        Real transformUserValue( Real userValue ) const override { return -userValue; }
    };
    /** @} */
    /** @} */

    /**
    @brief The AbsolutePixelCountLodStrategy class
        LOD strategy that works like ScreenRatioPixelCountLodStrategy,
        but in range [-width * height;0] instead of [-1.0;0.0]

        Because this strategy heavily depends on target resolution, it is strongly recommended
        that you use ScreenRatioPixelCountLodStrategy instead.
    */
    class _OgreExport AbsolutePixelCountLodStrategy : public PixelCountLodStrategyBase,
                                                      public Singleton<AbsolutePixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        AbsolutePixelCountLodStrategy();

        /// @copydoc LodStrategy::getValueImpl
        Real getValueImpl( const MovableObject *movableObject, const Camera *camera ) const override;

        void lodUpdateImpl( const size_t numNodes, ObjectData t, const Camera *camera,
                            Real bias ) const override;

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static AbsolutePixelCountLodStrategy &getSingleton();
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static AbsolutePixelCountLodStrategy *getSingletonPtr();
    };
    /** @} */
    /** @} */

    /**
    @brief The ScreenRatioPixelCountLodStrategy class
        Implement a strategy which calculates LOD ratios based on coverage in screen ratios in
        the range [-1.0; 0.0] where -1.0 means the object is covering the whole screen and 0
        covering almost nothing in the screen (very tiny).

        Note however that:
            1. Values beyond -1.0 are possible (it just means the object is bigger than the screen)
            2. It's an approximation
    */
    class _OgreExport ScreenRatioPixelCountLodStrategy
        : public PixelCountLodStrategyBase,
          public Singleton<ScreenRatioPixelCountLodStrategy>
    {
    public:
        /** Default constructor. */
        ScreenRatioPixelCountLodStrategy();

        /// @copydoc LodStrategy::getValueImpl
        Real getValueImpl( const MovableObject *movableObject, const Camera *camera ) const override;

        void lodUpdateImpl( const size_t numNodes, ObjectData t, const Camera *camera,
                            Real bias ) const override;

        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static ScreenRatioPixelCountLodStrategy &getSingleton();
        /** Override standard Singleton retrieval.
        @remarks
        Why do we do this? Well, it's because the Singleton
        implementation is in a .h file, which means it gets compiled
        into anybody who includes it. This is needed for the
        Singleton template to work, but we actually only want it
        compiled into the implementation of the class based on the
        Singleton, not all of them. If we don't change this, we get
        link errors when trying to use the Singleton-based class from
        an outside dll.
        @par
        This method just delegates to the template version anyway,
        but the implementation stays in this single compilation unit,
        preventing link errors.
        */
        static ScreenRatioPixelCountLodStrategy *getSingletonPtr();
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
