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

#ifndef __Ogre_Paging_Prereq_H__
#define __Ogre_Paging_Prereq_H__

#include "OgrePrerequisites.h"

namespace Ogre
{
    // forward decls
    class Grid2DPageStrategy;
    class Grid3DPageStrategy;
    class Page;
    class PageConnection;
    class PageContent;
    class PageContentFactory;
    class PageContentCollection;
    class PageContentCollectionFactory;
    class PagedWorld;
    class PagedWorldSection;
    class PageManager;
    class PageStrategy;
    class PageStrategyData;
    class PageProvider;
    class SimplePageContentCollection;
    class SimplePageContentCollectionFactory;


    typedef GeneralAllocatedObject PageAlloc;

    /// Identifier for a page
    typedef uint32 PageID;

}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
#   if defined( OGRE_STATIC_LIB )
#       define _OgrePagingExport
#   else
#       if defined( OGRE_PAGING_EXPORTS )
#           define _OgrePagingExport __declspec( dllexport )
#       else
#           if defined( __MINGW32__ )
#               define _OgrePagingExport
#           else
#               define _OgrePagingExport __declspec( dllimport )
#           endif
#       endif
#   endif
#elif defined ( OGRE_GCC_VISIBILITY )
#   define _OgrePagingExport __attribute__ ((visibility("default")))
#else
#   define _OgrePagingExport
#endif 



#endif 
