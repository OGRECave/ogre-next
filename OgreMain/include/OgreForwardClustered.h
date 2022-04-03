/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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
#ifndef _OgreForwardClustered_H_
#define _OgreForwardClustered_H_

#include "OgrePrerequisites.h"

#include "OgreForwardPlusBase.h"
#include "OgreRawPtr.h"
#include "Threading/OgreUniformScalableTask.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    /** Implementation of Clustered Forward Shading */
    class _OgreExport ForwardClustered : public ForwardPlusBase, public UniformScalableTask
    {
        struct ArrayPlane
        {
            ArrayVector3 normal;
            ArrayReal    negD;
        };
        struct FrustumRegion
        {
            ArrayPlane   plane[6];
            ArrayAabb    aabb;
            ArrayVector3 corners[8];
        };

        uint32 mWidth;
        uint32 mHeight;
        uint32 mNumSlices;
        size_t mReservedSlotsPerCell;
        size_t mObjsPerCell;
        size_t mLightsPerCell;
        size_t mDecalsPerCell;
        size_t mCubemapProbesPerCell;

        RawSimdUniquePtr<FrustumRegion, MEMCATEGORY_SCENE_CONTROL> mFrustumRegions;

        uint16 *RESTRICT_ALIAS mGridBuffer;
        Camera                *mCurrentCamera;

        float mMinDistance;
        float mMaxDistance;
        float mExponentK;
        float mInvExponentK;

        ObjectMemoryManager   *mObjectMemoryManager;
        NodeMemoryManager     *mNodeMemoryManager;
        vector<Camera *>::type mThreadCameras;

        bool                     mDebugWireAabbFrozen;
        vector<WireAabb *>::type mDebugWireAabb;

        inline size_t getDecalsOffsetStart() const;
        inline size_t getCubemapProbesOffsetStart() const;

        /// Performs the reverse of getSliceAtDepth. @see getSliceAtDepth.
        inline float getDepthAtSlice( uint32 slice ) const;

        /** Returns the slice index at the given depth
        @param depth
            Depth, in view space.
        @return
            Slice index, in range [0; mNumSlices)
        */
        inline uint32 getSliceAtDepth( Real depth ) const;

        void collectObjsForSlice( const size_t numPackedFrustumsPerSlice, const size_t frustumStartIdx,
                                  uint16 offsetStart, size_t minRq, size_t maxRq, size_t currObjsPerCell,
                                  size_t cellOffsetStart, ObjTypes objType, uint16 numFloat4PerObj );
        void collectLightForSlice( size_t slice, size_t threadId );

        void collectObjs( const Camera *camera, size_t &outNumDecals, size_t &outNumCubemapProbes );

    public:
        ForwardClustered( uint32 width, uint32 height, uint32 numSlices, uint32 lightsPerCell,
                          uint32 decalsPerCell, uint32 cubemapProbesPerCell, float minDistance,
                          float maxDistance, SceneManager *sceneManager );
        ~ForwardClustered() override;

        ForwardPlusMethods getForwardPlusMethod() const override { return MethodForwardClustered; }

        void setDebugFrustum( bool bEnableDebugFrustumWireAabb );
        bool getDebugFrustum() const;

        void setFreezeDebugFrustum( bool freezeDebugFrustum );
        bool getFreezeDebugFrustum() const;

        void execute( size_t threadId, size_t numThreads ) override;

        void collectLights( Camera *camera ) override;

        uint32 getWidth() const { return mWidth; }
        uint32 getHeight() const { return mHeight; }
        uint32 getNumSlices() const { return mNumSlices; }
        uint32 getLightsPerCell() const { return static_cast<uint32>( mLightsPerCell ); }
        uint32 getDecalsPerCell() const { return static_cast<uint32>( mDecalsPerCell ); }
        float  getMinDistance() const { return mMinDistance; }
        float  getMaxDistance() const { return mMaxDistance; }

        /// Returns the amount of bytes that fillConstBufferData is going to fill.
        size_t getConstBufferSize() const override;

        /** Fills 'passBufferPtr' with the necessary data for ForwardClustered rendering.
            @see getConstBufferSize
        @remarks
            Assumes 'passBufferPtr' is aligned to a vec4/float4 boundary.
        */
        void fillConstBufferData( Viewport *viewport, bool bRequiresTextureFlipping,
                                  uint32 renderTargetHeight, IdString shaderSyntax, bool instancedStereo,
                                  float *RESTRICT_ALIAS passBufferPtr ) const override;

        void setHlmsPassProperties( Hlms *hlms ) override;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
