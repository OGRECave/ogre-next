/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreGL3PlusTextureGpu_H_
#define _OgreGL3PlusTextureGpu_H_

#include "OgreGL3PlusPrerequisites.h"
#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreGL3PlusExport GL3PlusTextureGpu : public TextureGpu
    {
        /// This will not be owned by us if hasAutomaticBatching is true.
        /// It will also not be owned by us if we're not in GpuResidency::Resident
        /// This will always point to:
        ///     * A GL texture owned by us.
        ///     * A 4x4 dummy texture (now owned by us).
        ///     * A 64x64 mipmapped texture of us (but now owned by us).
        ///     * A GL texture not owned by us, but contains the final information.
        GLuint  mDisplayTextureName;
        GLenum  mGlTextureTarget;

        /// When we're transitioning to GpuResidency::Resident but we're not there yet,
        /// we will be either displaying a 4x4 dummy texture or a 64x64 one. However
        /// we reserve a spot to a final place will already be there for us once the
        /// texture data is fully uploaded. This variable contains that texture.
        /// Async upload operations should stack to this variable.
        /// May contain:
        ///     1. 0, if not resident or resident but not yet reached main thread.
        ///     2. The texture
        ///     3. An msaa texture (hasMsaaExplicitResolves == true)
        ///     4. The msaa resolved texture (hasMsaaExplicitResolves==false)
        GLuint  mFinalTextureName;
        /// Only used when hasMsaaExplicitResolves() == false.
        GLuint  mMsaaFramebufferName;

        virtual void createInternalResourcesImpl(void);
        virtual void destroyInternalResourcesImpl(void);

    public:
        GL3PlusTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                           VaoManager *vaoManager, IdString name, uint32 textureFlags,
                           TextureGpuManager *textureManager );
        virtual ~GL3PlusTextureGpu();

        virtual void copyTo( TextureGpu *dst, const TextureBox &srcBox, uint8 srcMipLevel,
                             const TextureBox &dstBox, uint8 dstMipLevel );

        virtual void getSubsampleLocations( vector<Vector2>::type locations );

        virtual void _setToDisplayDummyTexture(void);
        virtual void _notifyTextureSlotReserved(void);

        GLuint getDisplayTextureName(void) const    { return mDisplayTextureName; }
        GLuint getFinalTextureName(void) const      { return mFinalTextureName; }

        /// Returns GL_TEXTURE_2D / GL_TEXTURE_2D_ARRAY / etc
        GLenum getGlTextureTarget(void) const       { return mGlTextureTarget; }
    };
}

#include "OgreHeaderSuffix.h"

#endif
