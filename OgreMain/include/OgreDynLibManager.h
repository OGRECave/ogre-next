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
#ifndef __DynLibManager_H__
#define __DynLibManager_H__

#include "OgrePrerequisites.h"
#include "OgreSingleton.h"

#include "ogrestd/map.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup General
     *  @{
     */
    /** Manager for Dynamic-loading Libraries.
    @remarks
        This manager keeps a track of all the open dynamic-loading
        libraries, opens them and returns references to already-open
        libraries.
    */
    class _OgreExport DynLibManager : public Singleton<DynLibManager>, public OgreAllocatedObj
    {
    protected:
        typedef map<String, DynLib *>::type DynLibList;
        DynLibList                          mLibList;

    public:
        /** Default constructor.
        @note
            <br>Should never be called as the singleton is automatically
            created during the creation of the Root object.
        @see
            Root::Root
        */
        DynLibManager();

        /** Default destructor.
        @see
            Root::~Root
        */
        virtual ~DynLibManager();

        /** Loads the passed library.
        @param filename
            The name of the library. The extension can be omitted.
        @param bOptional
            When true, we will skip it if it fails to initialize
        */
        DynLib *load( const String &filename, const bool bOptional );

        /** Unloads the passed library.
        @param lib
            The library.
        */
        void unload( DynLib *lib );

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
        static DynLibManager &getSingleton();
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
        static DynLibManager *getSingletonPtr();
    };
    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif  // __DynLibManager_H__
