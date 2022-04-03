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
#include "OgreStableHeaders.h"

#include "OgreOptimisedUtil.h"

#include "OgrePlatformInformation.h"

//#define __DO_PROFILE__
#ifdef __DO_PROFILE__
#include "OgreRoot.h"
#endif

namespace Ogre {

    //---------------------------------------------------------------------
    // External functions
    extern OptimisedUtil* _getOptimisedUtilGeneral();
#if __OGRE_HAVE_SSE
    extern OptimisedUtil* _getOptimisedUtilSSE();
#endif
#if __OGRE_HAVE_DIRECTXMATH
    extern OptimisedUtil* _getOptimisedUtilDirectXMath();
#endif

#ifdef __DO_PROFILE__
    //---------------------------------------------------------------------
#if OGRE_COMPILER == OGRE_COMPILER_MSVC
    typedef unsigned __int64 uint64;
#pragma warning(push)
#pragma warning(disable: 4035)  // no return value
    static FORCEINLINE uint64 getCpuTimestamp()
    {
        __asm rdtsc
        // Return values in edx:eax, No return statement requirement here for VC.
    }
#pragma warning(pop)

#elif (OGRE_COMPILER == OGRE_COMPILER_GNUC || OGRE_COMPILER == OGRE_COMPILER_CLANG)
    typedef unsigned long long uint64;
    static FORCEINLINE uint64 getCpuTimestamp()
    {
        uint64 result;
        __asm__ __volatile__ ( "rdtsc" : "=A" (result) );
        return result;
    }

#endif  // OGRE_COMPILER

    //---------------------------------------------------------------------
    class OptimisedUtilProfiler : public OptimisedUtil
    {
    protected:

        enum
        {
            IMPL_DEFAULT,
#if __OGRE_HAVE_SSE
            IMPL_SSE,
#endif
            IMPL_COUNT
        };

        struct ProfileItem
        {
            uint mAvgTicks;
            uint mCount;
            uint64 mTotalTicks;
            uint64 mStartTick;

            ProfileItem()
                : mAvgTicks()
                , mCount()
                , mTotalTicks()
                , mStartTick()
            {
            }

            void begin()
            {
                mStartTick = getCpuTimestamp();
            }

            void end()
            {
                uint64 ticks = getCpuTimestamp() - mStartTick;
                mTotalTicks += ticks;
                ++mCount;
                mAvgTicks = mTotalTicks / mCount;
            }
        };

        typedef ProfileItem ProfileItems[IMPL_COUNT];

        typedef vector<OptimisedUtil*>::type OptimisedUtilList;
        OptimisedUtilList mOptimisedUtils;

    public:
        OptimisedUtilProfiler()
        {
            mOptimisedUtils.push_back(_getOptimisedUtilGeneral());
#if __OGRE_HAVE_SSE
            if (PlatformInformation::getCpuFeatures() & PlatformInformation::CPU_FEATURE_SSE)
            {
                mOptimisedUtils.push_back(_getOptimisedUtilSSE());
            }
#endif
        }

        virtual void softwareVertexSkinning(
            const float *srcPosPtr, float *destPosPtr,
            const float *srcNormPtr, float *destNormPtr,
            const float *blendWeightPtr, const unsigned char* blendIndexPtr,
            const Matrix4* const* blendMatrices,
            size_t srcPosStride, size_t destPosStride,
            size_t srcNormStride, size_t destNormStride,
            size_t blendWeightStride, size_t blendIndexStride,
            size_t numWeightsPerVertex,
            size_t numVertices)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->softwareVertexSkinning(
                srcPosPtr, destPosPtr,
                srcNormPtr, destNormPtr,
                blendWeightPtr, blendIndexPtr,
                blendMatrices,
                srcPosStride, destPosStride,
                srcNormStride, destNormStride,
                blendWeightStride, blendIndexStride,
                numWeightsPerVertex,
                numVertices);
            profile.end();

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

        virtual void softwareVertexMorph(
            Real t,
            const float *srcPos1, const float *srcPos2,
            float *dstPos,
            size_t pos1VSize, size_t pos2VSize, size_t dstVSize, 
            size_t numVertices,
            bool morphNormals)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->softwareVertexMorph(
                t,
                srcPos1, srcPos2,
                dstPos,
                pos1VSize, pos2VSize, dstVSize,
                numVertices,
                morphNormals);
            profile.end();

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

        virtual void concatenateAffineMatrices(
            const Matrix4& baseMatrix,
            const Matrix4* srcMatrices,
            Matrix4* dstMatrices,
            size_t numMatrices)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->concatenateAffineMatrices(
                baseMatrix,
                srcMatrices,
                dstMatrices,
                numMatrices);
            profile.end();

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

        /// @copydoc OptimisedUtil::calculateFaceNormals
        virtual void calculateFaceNormals(
            const float *positions,
            const EdgeData::Triangle *triangles,
            Vector4 *faceNormals,
            size_t numTriangles)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->calculateFaceNormals(
                positions,
                triangles,
                faceNormals,
                numTriangles);
            profile.end();

            //
            //   Dagon SkeletonAnimation sample test results (CPU timestamp per-function call):
            //
            //                  Pentium 4 3.0G HT       Athlon XP 2500+
            //
            //      General     657080                  486494
            //      SSE         223559                  399495
            //

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

        /// @copydoc OptimisedUtil::calculateLightFacing
        virtual void calculateLightFacing(
            const Vector4& lightPos,
            const Vector4* faceNormals,
            char* lightFacings,
            size_t numFaces)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->calculateLightFacing(
                lightPos,
                faceNormals,
                lightFacings,
                numFaces);
            profile.end();

            //
            //   Dagon SkeletonAnimation sample test results (CPU timestamp per-function call):
            //
            //                  Pentium 4 3.0G HT       Athlon XP 2500+
            //
            //      General     171875                  86998
            //      SSE          47934                  63995
            //

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

        virtual void extrudeVertices(
            const Vector4& lightPos,
            Real extrudeDist,
            const float* srcPositions,
            float* destPositions,
            size_t numVertices)
        {
            static ProfileItems results;
            static size_t index;
            index = Root::getSingleton().getNextFrameNumber() % mOptimisedUtils.size();
            OptimisedUtil* impl = mOptimisedUtils[index];
            ProfileItem& profile = results[index];

            profile.begin();
            impl->extrudeVertices(
                lightPos,
                extrudeDist,
                srcPositions,
                destPositions,
                numVertices);
            profile.end();

            //
            //   Dagon SkeletonAnimation sample test results (CPU timestamp per-function call):
            //
            //                                  Pentium 4 3.0G HT   Athlon XP 2500+
            //
            //      Directional Light, General   38106               92306
            //      Directional Light, SSE       27292               67055
            //
            //      Point Light, General        224209              155483
            //      Point Light, SSE             56817              106663
            //

            // You can put break point here while running test application, to
            // watch profile results.
            ++index;    // So we can put break point here even if in release build
        }

    };
#endif // __DO_PROFILE__

    //---------------------------------------------------------------------
    OptimisedUtil* OptimisedUtil::msImplementation = OptimisedUtil::_detectImplementation();

    //---------------------------------------------------------------------
    OptimisedUtil* OptimisedUtil::_detectImplementation()
    {
        //
        // Some speed test results (averaged number of CPU timestamp (RDTSC) per-function call):
        //
        //   Dagon SkeletonAnimation sample - softwareVertexSkinning:
        //
        //                                      Pentium 4 3.0G HT       Athlon XP 2500+     Athlon 64 X2 Dual Core 3800+
        //
        //      Shared Buffers, General C       763677                  462903              473038
        //      Shared Buffers, Unrolled SSE    210030 *best*           369762              228328 *best*
        //      Shared Buffers, General SSE     286202                  352412 *best*       302796
        //
        //      Separated Buffers, General C    762640                  464840              478740
        //      Separated Buffers, Unrolled SSE 219222 *best*           287992 *best*       238770 *best*
        //      Separated Buffers, General SSE  290129                  341614              307262
        //
        //      PosOnly, General C              388663                  257350              262831
        //      PosOnly, Unrolled SSE           139814 *best*           200323 *best*       168995 *best*
        //      PosOnly, General SSE            172693                  213704              175447
        //
        //   Another my own test scene - softwareVertexSkinning:
        //
        //                                      Pentium P4 3.0G HT      Athlon XP 2500+
        //
        //      Shared Buffers, General C       74527                   -
        //      Shared Buffers, Unrolled SSE    22743 *best*            -
        //      Shared Buffers, General SSE     28527                   -
        //
        //
        // Note that speed test appears unaligned load/store instruction version
        // loss performance 5%-10% than aligned load/store version, even if both
        // of them access to aligned data. Thus, we should use aligned load/store
        // as soon as possible.
        //
        //
        // We are pick up the implementation based on test results above.
        //
#ifdef __DO_PROFILE__
        {
            static OptimisedUtilProfiler msOptimisedUtilProfiler;
            return &msOptimisedUtilProfiler;
        }

#else   // !__DO_PROFILE__

#if __OGRE_HAVE_SSE
        if (PlatformInformation::getCpuFeatures() & PlatformInformation::CPU_FEATURE_SSE)
        {
            return _getOptimisedUtilSSE();
        }
        else
#endif  // __OGRE_HAVE_SSE
        {
#if __OGRE_HAVE_DIRECTXMATH
            return _getOptimisedUtilDirectXMath();
#else // __OGRE_HAVE_DIRECTXMATH
             return _getOptimisedUtilGeneral();
#endif
        }

#endif  // __DO_PROFILE__
    }

}
