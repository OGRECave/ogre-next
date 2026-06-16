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

#ifndef _OgreCompositorPassIblSpecularDef_H_
#define _OgreCompositorPassIblSpecularDef_H_

#include "../OgreCompositorPassDef.h"
#include "OgreCommon.h"
#include "OgreVector4.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    class _OgreExport CompositorPassIblSpecularDef : public CompositorPassDef
    {
        IdString mInputTextureName;
        IdString mOutputTextureName;

    public:
        float mSamplesPerIteration;
        /// Value to use when we can't spread IBL generation over multiple frames
        /// (mNumInitialPasses != std::numeric_limits<uint32>::max()) due to
        /// GPU not having RSC_TYPED_UAV_LOADS
        float mSamplesSingleIterationFallback;
        /// Value Saturation Hue
        Vector4 mIblCorrectionVSH;

        /// Force this pass to behave like GENERATE_MIPMAPS, regardless of compute shader support
        bool mForceMipmapFallback;

        /// IBL via Compute requires mipmaps to be up to date. This being true forces the input cubemap
        /// to autogenerate the mipmaps every frame. Otherwise we won't do it and assume it's already
        /// been done. Ignored if mForceMipmapFallback = true.
        bool mAutogenInputMipmaps;

        CompositorNodeDef *mParentNodeDef;

    public:
        CompositorPassIblSpecularDef( CompositorNodeDef   *parentNodeDef,
                                      CompositorTargetDef *parentTargetDef ) :
            CompositorPassDef( PASS_IBL_SPECULAR, parentTargetDef ),
            mSamplesPerIteration( 128.0 ),
            mSamplesSingleIterationFallback( 128.0 ),
            mIblCorrectionVSH( 0, 1.0f, 0, 0 ),
            mForceMipmapFallback( false ),
            mAutogenInputMipmaps( true ),
            mParentNodeDef( parentNodeDef )
        {
        }

        ~CompositorPassIblSpecularDef() override;

        /**
        @param textureName
            Name of the texture (can come from input channel, local textures, or global ones)
        */
        void setCubemapInput( const String &textureName );
        void setCubemapOutput( const String &textureName );

        IdString getInputTextureName() const { return mInputTextureName; }
        IdString getOutputTextureName() const { return mOutputTextureName; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
