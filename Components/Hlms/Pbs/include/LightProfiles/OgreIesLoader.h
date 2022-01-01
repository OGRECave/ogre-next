/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#ifndef _OgreIesLoader_H_
#define _OgreIesLoader_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace LampConeType
    {
        enum LampConeType
        {
            Type90,
            Type180
        };

    }
    namespace LampHorizType
    {
        enum LampHorizType
        {
            Type90,
            Type180,
            Type360
        };
    }

    /**
    @class IesLoader
    */
    class _OgreHlmsPbsExport IesLoader : public OgreAllocatedObj
    {
        float mCandelaMult;
        /// Vertical angle aka Cone Angle
        uint32 mNumVertAngles;
        uint32 mNumHorizAngles;

        float mBallastFactor;
        float mBallastLampPhotometricFactor;

        LampConeType::LampConeType   mLampConeType;
        LampHorizType::LampHorizType mLampHorizType;

        FastArray<float> mAngleData;
        FastArray<float> mCandelaValues;

        String mFilename;

        static void skipWhitespace( const char *text, size_t &offset );

        void verifyDataIsSorted() const;
        void loadFromString( const char *iesTextData );

    public:
        IesLoader( const String &filename, const char *iesTextData );

        const String &getName() const { return mFilename; }

        uint32 getSuggestedTexWidth() const;

        void convertToImage1D( Image2 &inOutImage, uint32 row );
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
