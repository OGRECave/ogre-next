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

#include "OgreStableHeaders.h"

#include "OgrePrerequisites.h"

#include "OgreWorkarounds.h"

namespace Ogre
{
#ifdef OGRE_VK_WORKAROUND_ADRENO_UBO64K
    bool Workarounds::mAdrenoUbo64kLimitTriggered = false;
    size_t Workarounds::mAdrenoUbo64kLimit = 0u;
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_D32_FLOAT
    bool Workarounds::mAdrenoD32FloatBug = false;
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
    bool Workarounds::mAdreno5xx6xxMinCaps = false;
#endif
#ifdef OGRE_VK_WORKAROUND_BAD_3D_BLIT
    bool Workarounds::mBad3DBlit = false;
#endif

    void Workarounds::dump( String &outStr )
    {
#ifdef OGRE_VK_WORKAROUND_ADRENO_UBO64K
        outStr +=
            "\n - mAdrenoUbo64kLimit: " + StringConverter::toString( Workarounds::mAdrenoUbo64kLimit );
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_D32_FLOAT
        outStr +=
            "\n - mAdrenoD32FloatBug: " + StringConverter::toString( Workarounds::mAdrenoD32FloatBug );
#endif
#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
        outStr += "\n - mAdreno5xx6xxMinCaps: " +
                  StringConverter::toString( Workarounds::mAdreno5xx6xxMinCaps );
#endif
#ifdef OGRE_VK_WORKAROUND_BAD_3D_BLIT
        outStr += "\n - mBad3DBlit: " + StringConverter::toString( Workarounds::mBad3DBlit );
#endif
    }
}  // namespace Ogre
