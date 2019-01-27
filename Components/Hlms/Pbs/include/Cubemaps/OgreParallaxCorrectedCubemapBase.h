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
#ifndef _OgreParallaxCorrectedCubemapBase_H_
#define _OgreParallaxCorrectedCubemapBase_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "Cubemaps/OgreCubemapProbe.h"
#include "OgreIdString.h"
#include "OgreId.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorWorkspaceDef;
    typedef vector<CubemapProbe*>::type CubemapProbeVec;

    /**
    @see HlmsPbsDatablock::setCubemapProbe
    */
    class _OgreHlmsPbsExport ParallaxCorrectedCubemapBase : public IdObject
    {
    protected:
        TextureGpu                      *mBindTexture;
        HlmsSamplerblock const          *mSamplerblockTrilinear;

        CubemapProbeVec mProbes;
        bool            mAutomaticMode;
        bool            mUseDpm2DArray;

        bool            mIsRendering;
        public: bool                    mPaused;
        public: uint32                  mMask; /// @see CubemapProbe::mMask
    protected:
        Root                            *mRoot;
        SceneManager                    *mSceneManager;
        CompositorWorkspaceDef const    *mDefaultWorkspaceDef;

    public:
        ParallaxCorrectedCubemapBase( IdType id, Root *root, SceneManager *sceneManager,
                                      const CompositorWorkspaceDef *probeWorkspaceDef,
                                      bool automaticMode );
        virtual ~ParallaxCorrectedCubemapBase();

        /// Adds a cubemap probe.
        CubemapProbe* createProbe(void);
        void destroyProbe( CubemapProbe *probe );
        virtual void destroyAllProbes(void);

        /// Destroys the Proxy Items. Useful if you need to call sceneManager->clearScene();
        /// The you MUST call this function before. i.e.
        ///     pcc->prepareForClearScene();
        ///     sceneManager->clearScene();
        ///     pcc->restoreFromClearScene();
        /// Updating ParallaxCorrectedCubemap without calling prepareForClearScene/restoreFromClearScene
        /// will result in a crash.
        virtual void prepareForClearScene(void);
        virtual void restoreFromClearScene(void);

        const CubemapProbeVec& getProbes(void) const        { return mProbes; }

        bool getAutomaticMode(void) const               { return mAutomaticMode; }
        bool getUseDpm2DArray(void) const               { return mUseDpm2DArray; }

        TextureGpu* getBindTexture(void) const          { return mBindTexture; }
        const HlmsSamplerblock* getBindTrilinearSamplerblock(void)
                                                        { return mSamplerblockTrilinear; }

        virtual void _notifyPreparePassHash( const Matrix4 &viewMatrix );
        virtual size_t getConstBufferSize(void);
        virtual void fillConstBufferData( const Matrix4 &viewMatrix,
                                          float * RESTRICT_ALIAS passBufferPtr ) const;
        static void fillConstBufferData( const CubemapProbe &probe,
                                         const Matrix4 &viewMatrix,
                                         const Matrix3 &invViewMat3,
                                         float * RESTRICT_ALIAS passBufferPtr );

        /// See mTmpRtt. Finds an RTT that is compatible to copy to baseParams.
        /// Creates one if none found.
        virtual TextureGpu* findTmpRtt( const TextureGpu *baseParams );
        virtual void releaseTmpRtt( const TextureGpu *tmpRtt );

        virtual void _copyRenderTargetToCubemap( uint32 cubemapArrayIdx );

        /** Acquires a texture with a given slot.
        @param outTexSlot [out]
            Texture slot. Value is left untouched if return value is nullptr
        @return
            Texture. Can be nullptr if ran out of slots.
        */
        virtual TextureGpu* _acquireTextureSlot( uint32 &outTexSlot );
        virtual void _releaseTextureSlot( TextureGpu *texture, uint32 texSlot );

        virtual void _addManuallyActiveProbe( CubemapProbe *probe );
        virtual void _removeManuallyActiveProbe( CubemapProbe *probe );

        void _setIsRendering( bool bIsRendering )       { mIsRendering = bIsRendering; }
        /// Inform whether we're currently updating a probe. Some Hlms / PCC combinations
        /// should not perform PCC while rendering, either because the RenderTarget is the same
        /// as the cubemap texture, or because other glitches may occur
        bool isRendering(void) const                    { return mIsRendering; }

        SceneManager* getSceneManager(void) const;
        const CompositorWorkspaceDef* getDefaultWorkspaceDef(void) const;
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
