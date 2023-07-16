/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#include "OgrePrerequisites.h"

#include "OgreIdString.h"
#include "OgreRenderQueue.h"
#include "ParticleSystem/OgreParticle2.h"

#include <map>

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class ParticleSystemDef;

    class ParticleEmitterDefDataFactory;

    class _OgreExport ParticleSystemManager2
    {
        SceneManager *mSceneManager;

        FastArray<ParticleSystemDef *> mActiveParticleSystemDefs;

        std::map<IdString, ParticleSystemDef *> mParticleSystemDefMap;

        IndexBufferPacked *mSharedIndexBuffer16;
        IndexBufferPacked *mSharedIndexBuffer32;
        uint32             mHighestPossibleQuota16;
        uint32             mHighestPossibleQuota32;

        float mTimeSinceLast;  // For threaded update.

        void calculateHighestPossibleQuota( VaoManager *vaoManager );
        void createSharedIndexBuffers( VaoManager *vaoManager );

        void tickParticles( size_t threadIdx, Real timeSinceLast, ParticleCpuData cpuData,
                            ParticleGpuData *gpuData, const size_t numParticles,
                            ParticleSystemDef *systemDef );

        void updateSerialPre( Real timeSinceLast );
        void updateSerialPos();

        void updateParallel( size_t threadIdx, size_t numThreads );

    public:
        ParticleSystemManager2( SceneManager *sceneManager );
        ~ParticleSystemManager2();

        static void addEmitterFactory( ParticleEmitterDefDataFactory *factory );
        static void removeEmitterFactory( ParticleEmitterDefDataFactory *factory );

        static ParticleEmitterDefDataFactory *getFactory( IdString name );

        /** ParticleSystemManager2 must know the highest possible quota any of its particle
            systems may achieve.

            After the index buffer has been initialized, this value cannot be changed.

            After initialization you can create new ParticleSystemDef that set quotas
            lower than the already highest possible one. However you cannot set a quota
            larger than that.

            Make sure to call this function with the highest values you anticipate your particle
            systems will need.

            Note this value directly correlates to VRAM used.
            The amount of bytes consumed are : highestQuota32 * 6 * 4 bytes + highestQuota16 * 6 * 2
        @param highestQuota16
            The highest possible quota for ParticleSystemDefs that need less than 65536 particles.
            If unsure, you can set it to 65536.

            This value can be 0, however we may raise it if we find a ParticleSystemDef that needs it.
        @param highestQuota32
            The highest possible quota for ParticleSystemDefs that need more than 65535 particles.
            This value can be 0, however we may raise it if we find a ParticleSystemDef that needs it.
        */
        void setHighestPossibleQuota( uint16 highestQuota16, uint32 highestQuota32 );

        uint32 getHighestPossibleQuota16() const { return mHighestPossibleQuota16; }
        uint32 getHighestPossibleQuota32() const { return mHighestPossibleQuota32; }

        ParticleSystemDef *createParticleSystemDef( const String &name );

        /** Retrieves an existing ParticleSystemDef with the given name
            Throws if not found.
        @param name
            Name of the ParticleSystemDef.
        @return
            The ParticleSystemDef.
        */
        ParticleSystemDef *getParticleSystemDef( const String &name );

        void destroyAllParticleSystems();

        /** Instructs us to add all the ParticleSystemDef to the RenderQueue that match the given
            renderQueueId and pass the visibilityMask.
        @param threadIdx
        @param numThreads
        @param renderQueue
        @param renderQueueId
        @param visibilityMask
        */
        void _addToRenderQueue( size_t threadIdx, size_t numThreads, RenderQueue *renderQueue,
                                uint8 renderQueueId, uint32 visibilityMask ) const;

        IndexBufferPacked *_getSharedIndexBuffer( size_t maxQuota, VaoManager *vaoManager );

        void update( Real timeSinceLast );

        void _addParticleSystemDefAsActive( ParticleSystemDef *def );
        void _removeParticleSystemDefFromActive( ParticleSystemDef *def );
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"
