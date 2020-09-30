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
#ifndef _OgrePlatformInformation_H_
#define _OgrePlatformInformation_H_

#include "OgrePrerequisites.h"

namespace Ogre {
//
// TODO: Puts following macros into OgrePlatform.h?
//

/* Find how to declare aligned variable.
*/
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#   define OGRE_ALIGNED_DECL(type, var, alignment)  __declspec(align(alignment)) type var

#elif (OGRE_COMPILER == OGRE_COMPILER_GNUC) || (OGRE_COMPILER == OGRE_COMPILER_CLANG)
#   define OGRE_ALIGNED_DECL(type, var, alignment)  type var __attribute__((__aligned__(alignment)))

#else
#   define OGRE_ALIGNED_DECL(type, var, alignment)  type var
#endif

/** Find perfect alignment (should supports SIMD alignment if SIMD available)
*/
#if OGRE_CPU == OGRE_CPU_X86
#   define OGRE_SIMD_ALIGNMENT  16

#else
#   define OGRE_SIMD_ALIGNMENT  16
#endif

/* Declare variable aligned to SIMD alignment.
*/
#define OGRE_SIMD_ALIGNED_DECL(type, var)   OGRE_ALIGNED_DECL(type, var, OGRE_SIMD_ALIGNMENT)

/* Define whether or not Ogre compiled with SSE support.
*/
#if OGRE_USE_SIMD == 1
    #if   OGRE_DOUBLE_PRECISION == 0 && OGRE_CPU == OGRE_CPU_X86 && OGRE_PLATFORM != OGRE_PLATFORM_NACL
    #   define __OGRE_HAVE_SSE  1
    #endif

    /* Define whether or not Ogre compiled with NEON support.
     */
    #if OGRE_DOUBLE_PRECISION == 0 && OGRE_CPU == OGRE_CPU_ARM && \
        ( defined(__ARM_NEON__) || defined(_WIN32_WINNT_WIN8) && _WIN32_WINNT >= _WIN32_WINNT_WIN8 )
    #   define __OGRE_HAVE_NEON  1
    #endif
#endif

#ifndef __OGRE_HAVE_SSE
#   define __OGRE_HAVE_SSE  0
#endif

#if OGRE_USE_SIMD == 0 || !defined(__OGRE_HAVE_NEON)
#   define __OGRE_HAVE_NEON  0
#endif

#if !defined(__OGRE_HAVE_DIRECTXMATH)
#   define __OGRE_HAVE_DIRECTXMATH  0
#endif
    
	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup General
	*  @{
	*/


    /** Class which provides the run-time platform information Ogre runs on.
        @remarks
            Ogre is designed to be platform-independent, but some platform
            and run-time environment specific optimised functions are built-in
            to maximise performance, and those special optimised routines are
            need to determine run-time environment for select variant executing
            path.
        @par
            This class manages that provides a couple of functions to determine
            platform information of the run-time environment.
        @note
            This class is supposed to use by advanced user only.
    */
    class _OgreExport PlatformInformation
    {
    public:

        /// Enum describing the different CPU features we want to check for, platform-dependent
        enum CpuFeatures
        {
#if OGRE_CPU == OGRE_CPU_X86
            CPU_FEATURE_SSE         = 1 << 0,
            CPU_FEATURE_SSE2        = 1 << 1,
            CPU_FEATURE_SSE3        = 1 << 2,
            CPU_FEATURE_MMX         = 1 << 3,
            CPU_FEATURE_MMXEXT      = 1 << 4,
            CPU_FEATURE_3DNOW       = 1 << 5,
            CPU_FEATURE_3DNOWEXT    = 1 << 6,
            CPU_FEATURE_CMOV        = 1 << 7,
            CPU_FEATURE_TSC         = 1 << 8,
            CPU_FEATURE_FPU         = 1 << 9,
            CPU_FEATURE_PRO         = 1 << 10,
            CPU_FEATURE_HTT         = 1 << 11,
#elif OGRE_CPU == OGRE_CPU_ARM
            CPU_FEATURE_VFP         = 1 << 12,
            CPU_FEATURE_NEON        = 1 << 13,
#elif OGRE_CPU == OGRE_CPU_MIPS
            CPU_FEATURE_MSA         = 1 << 14,
#endif

            CPU_FEATURE_NONE        = 0
        };

        /** Gets a string of the CPU identifier.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static const String& getCpuIdentifier(void);

        /** Gets a or-masked of enum CpuFeatures that are supported by the CPU.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static uint getCpuFeatures(void);

        /** Gets whether a specific feature is supported by the CPU.
        @note
            Actual detecting are performs in the first time call to this function,
            and then all future calls with return internal cached value.
        */
        static bool hasCpuFeature(CpuFeatures feature);

        /** Returns the number of logical cores, including Hyper Threaded / SMT cores
        @note
            Returns 0 if couldn't detect.
        */
        static uint32 getNumLogicalCores(void);


        /** Write the CPU information to the passed in Log */
        static void log(Log* pLog);

    };
    /** @} */
    /** @} */

}

#endif  // _OgrePlatformInformation_H_
