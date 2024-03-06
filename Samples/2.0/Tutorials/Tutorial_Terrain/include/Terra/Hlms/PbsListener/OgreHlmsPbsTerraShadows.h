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

#ifndef _OgreHlmsPbsTerraShadows_
#define _OgreHlmsPbsTerraShadows_

#include "OgreGpuProgram.h"
#include "OgreHlmsListener.h"

namespace Ogre
{
    class Terra;

    class HlmsPbsTerraShadows : public HlmsListener
    {
    protected:
        Terra                  *mTerra;
        HlmsSamplerblock const *mTerraSamplerblock;
#if OGRE_DEBUG_MODE
        SceneManager *mSceneManager;
#endif

    public:
        HlmsPbsTerraShadows();
        virtual ~HlmsPbsTerraShadows();

        void setTerra( Terra *terra );

        uint16 getNumExtraPassTextures( const HlmsPropertyVec &properties,
                                        bool                   casterPass ) const override;

        void propertiesMergedPreGenerationStep( Hlms *hlms, const HlmsCache &passCache,
                                                const HlmsPropertyVec &renderableCacheProperties,
                                                const PiecesMap renderableCachePieces[NumShaderTypes],
                                                const HlmsPropertyVec  &properties,
                                                const QueuedRenderable &queuedRenderable,
                                                size_t                  tid ) override;

        void preparePassHash( const CompositorShadowNode *shadowNode, bool casterPass,
                              bool dualParaboloid, SceneManager *sceneManager, Hlms *hlms ) override;

        uint32 getPassBufferSize( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager ) const override;

        float *preparePassBuffer( const CompositorShadowNode *shadowNode, bool casterPass,
                                  bool dualParaboloid, SceneManager *sceneManager,
                                  float *passBufferPtr ) override;

        void hlmsTypeChanged( bool casterPass, CommandBuffer *commandBuffer,
                              const HlmsDatablock *datablock, size_t texUnit ) override;
    };

    struct PbsTerraProperty
    {
        static const IdString TerraEnabled;
    };
}  // namespace Ogre

#endif
