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

#ifndef __AllocatedObject_H__
#define __AllocatedObject_H__

#include "OgrePrerequisites.h"

#include "OgreHeaderPrefix.h"

// Anything that has done a #define new <blah> will screw operator new definitions up
// so undefine
#ifdef new
#    undef new
#endif
#ifdef delete
#    undef delete
#endif

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Memory
     *  @{
     */
    /** Superclass for all objects that wish to use custom memory allocators
        when their new / delete operators are called.
        Requires a template parameter identifying the memory allocator policy
        to use (e.g. see StdAllocPolicy).
    */
    template <class Alloc>
    class _OgreExport AllocatedObject
    {
    public:
        explicit AllocatedObject() {}

        ~AllocatedObject() {}

        /// operator new, with debug line info
        void *operator new( size_t sz, const char *file, int line, const char *func )
        {
            return Alloc::allocateBytes( sz, file, line, func );
        }

        void *operator new( size_t sz ) { return Alloc::allocateBytes( sz ); }

        /// placement operator new
        void *operator new( size_t sz, void *ptr )
        {
            (void)sz;
            return ptr;
        }

        /// array operator new, with debug line info
        void *operator new[]( size_t sz, const char *file, int line, const char *func )
        {
            return Alloc::allocateBytes( sz, file, line, func );
        }

        void *operator new[]( size_t sz ) { return Alloc::allocateBytes( sz ); }

        void operator delete( void *ptr ) { Alloc::deallocateBytes( ptr ); }

        // Corresponding operator for placement delete (second param same as the first)
        void operator delete( void *ptr, void * ) { Alloc::deallocateBytes( ptr ); }

        // only called if there is an exception in corresponding 'new'
        void operator delete( void *ptr, const char *, int, const char * )
        {
            Alloc::deallocateBytes( ptr );
        }

        void operator delete[]( void *ptr ) { Alloc::deallocateBytes( ptr ); }

        void operator delete[]( void *ptr, const char *, int, const char * )
        {
            Alloc::deallocateBytes( ptr );
        }
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
