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
#ifndef _OgreParallaxCorrectedCubemapBase_H_
#define _OgreParallaxCorrectedCubemapBase_H_

#include "OgreHlmsPbsPrerequisites.h"

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Cubemaps/OgreCubemapProbe.h"
#include "OgreId.h"
#include "OgreIdString.h"
#include "OgreRenderSystem.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorWorkspaceDef;
    typedef vector<CubemapProbe *>::type CubemapProbeVec;

    /**
    @see HlmsPbsDatablock::setCubemapProbe
    */
    class _OgreHlmsPbsExport ParallaxCorrectedCubemapBase : public IdObject,
                                                            public RenderSystem::Listener,
                                                            public CompositorWorkspaceListener
    {
    protected:
        TextureGpu             *mBindTexture;
        HlmsSamplerblock const *mSamplerblockTrilinear;

        CubemapProbeVec mProbes;
        bool            mAutomaticMode;
        bool            mUseDpm2DArray;

        bool mIsRendering;

    public:
        bool mPaused;

    public:
        uint32 mMask;  ///< @see CubemapProbe::mMask
    protected:
        Root                         *mRoot;
        SceneManager                 *mSceneManager;
        CompositorWorkspaceDef const *mDefaultWorkspaceDef;

        Pass         *mPccCompressorPass;
        CubemapProbe *mProbeRenderInProgress;

    public:
        ParallaxCorrectedCubemapBase( IdType id, Root *root, SceneManager *sceneManager,
                                      const CompositorWorkspaceDef *probeWorkspaceDef,
                                      bool                          automaticMode );
        ~ParallaxCorrectedCubemapBase() override;

        virtual void _releaseManualHardwareResources();
        virtual void _restoreManualHardwareResources();

        uint32       getIblTargetTextureFlags( PixelFormatGpu pixelFormat ) const;
        static uint8 getIblNumMipmaps( uint32 width, uint32 height );

        /// Adds a cubemap probe.
        CubemapProbe *createProbe();
        virtual void  destroyProbe( CubemapProbe *probe );
        virtual void  destroyAllProbes();

        /// Destroys the Proxy Items. Useful if you need to call sceneManager->clearScene();
        /// The you MUST call this function before. i.e.
        ///     pcc->prepareForClearScene();
        ///     sceneManager->clearScene();
        ///     pcc->restoreFromClearScene();
        /// Updating ParallaxCorrectedCubemap without calling prepareForClearScene/restoreFromClearScene
        /// will result in a crash.
        virtual void prepareForClearScene();
        virtual void restoreFromClearScene();

        const CubemapProbeVec &getProbes() const { return mProbes; }

        bool getAutomaticMode() const { return mAutomaticMode; }
        bool getUseDpm2DArray() const { return mUseDpm2DArray; }

        TextureGpu             *getBindTexture() const { return mBindTexture; }
        const HlmsSamplerblock *getBindTrilinearSamplerblock() { return mSamplerblockTrilinear; }

        /// By default the probes will be constructed when the user enters its vecinity.
        /// This can cause noticeable stalls. Use this function to regenerate them all
        /// at once (i.e. at loading time)
        virtual void updateAllDirtyProbes() = 0;

        virtual void _notifyPreparePassHash( const Matrix4 &viewMatrix );

        virtual size_t getConstBufferSize();

        virtual void fillConstBufferData( const Matrix4        &viewMatrix,
                                          float *RESTRICT_ALIAS passBufferPtr ) const;
        static void  fillConstBufferData( const CubemapProbe &probe, const Matrix4 &viewMatrix,
                                          const Matrix3        &invViewMat3,
                                          float *RESTRICT_ALIAS passBufferPtr );

        /// See mTmpRtt. Finds an RTT that is compatible to copy to baseParams.
        /// Creates one if none found.
        virtual TextureGpu *findTmpRtt( const TextureGpu *baseParams );
        virtual void        releaseTmpRtt( const TextureGpu *tmpRtt );

        virtual TextureGpu *findIbl( const TextureGpu *baseParams );
        virtual void        releaseIbl( const TextureGpu *tmpRtt );

        virtual void _copyRenderTargetToCubemap( uint32 cubemapArrayIdx );

        /** Acquires a texture with a given slot.
        @param outTexSlot [out]
            Texture slot. Value is left untouched if return value is nullptr
        @return
            Texture. Can be nullptr if ran out of slots.
        */
        virtual TextureGpu *_acquireTextureSlot( uint16 &outTexSlot );
        virtual void        _releaseTextureSlot( TextureGpu *texture, uint32 texSlot );

        virtual void _addManuallyActiveProbe( CubemapProbe *probe );
        virtual void _removeManuallyActiveProbe( CubemapProbe *probe );

        void _setIsRendering( bool bIsRendering ) { mIsRendering = bIsRendering; }
        /// Inform whether we're currently updating a probe. Some Hlms / PCC combinations
        /// should not perform PCC while rendering, either because the RenderTarget is the same
        /// as the cubemap texture, or because other glitches may occur
        bool isRendering() const { return mIsRendering; }

        void _setProbeRenderInProgress( CubemapProbe *probe ) { mProbeRenderInProgress = probe; }

        SceneManager *getSceneManager() const;

        const CompositorWorkspaceDef *getDefaultWorkspaceDef() const;

        void passPreExecute( CompositorPass *pass ) override;

        // RenderSystem::Listener overloads
        void eventOccurred( const String &eventName, const NameValuePairList *parameters ) override;
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
