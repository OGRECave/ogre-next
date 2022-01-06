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

#ifndef _OgreGL3PlusTextureGpu_H_
#define _OgreGL3PlusTextureGpu_H_

#include "OgreGL3PlusPrerequisites.h"

#include "OgreTextureGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreGL3PlusExport GL3PlusTextureGpu : public TextureGpu
    {
    protected:
        /// This will not be owned by us if hasAutomaticBatching is true.
        /// It will also not be owned by us if we're not in GpuResidency::Resident
        /// This will always point to:
        ///     * A GL texture owned by us.
        ///     * A 4x4 dummy texture (now owned by us).
        ///     * A 64x64 mipmapped texture of us (but now owned by us).
        ///     * A GL texture not owned by us, but contains the final information.
        GLuint mDisplayTextureName;
        GLenum mGlTextureTarget;

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
        /// This value may be a renderbuffer instead of a texture if isRenderbuffer() returns true.
        GLuint mFinalTextureName;
        /// Only used when hasMsaaExplicitResolves() == false.
        /// This value is always an FBO.
        GLuint mMsaaFramebufferName;

        void createInternalResourcesImpl() override;
        void destroyInternalResourcesImpl() override;

        bool isRenderbuffer() const;

        void bindTextureToFrameBuffer( GLenum target, uint8 mipLevel, uint32 depthOrSlice );
        void bindTextureToFrameBuffer( GLenum target, GLuint textureName, uint8 mipLevel,
                                       uint32 depthOrSlice, bool bindMsaaColourRenderbuffer );

        void copyViaFramebuffer( TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel,
                                 const TextureBox &srcBox, uint8 srcMipLevel,
                                 bool keepResolvedTexSynced );

    public:
        GL3PlusTextureGpu( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                           VaoManager *vaoManager, IdString name, uint32 textureFlags,
                           TextureTypes::TextureTypes initialType, TextureGpuManager *textureManager );
        ~GL3PlusTextureGpu() override;

        void setTextureType( TextureTypes::TextureTypes textureType ) override;

        void copyTo(
            TextureGpu *dst, const TextureBox &dstBox, uint8 dstMipLevel, const TextureBox &srcBox,
            uint8 srcMipLevel, bool keepResolvedTexSynced = true,
            CopyEncTransitionMode::CopyEncTransitionMode srcTransitionMode = CopyEncTransitionMode::Auto,
            CopyEncTransitionMode::CopyEncTransitionMode dstTransitionMode =
                CopyEncTransitionMode::Auto ) override;

        void _autogenerateMipmaps( CopyEncTransitionMode::CopyEncTransitionMode transitionMode =
                                       CopyEncTransitionMode::Auto ) override;

        void getSubsampleLocations( vector<Vector2>::type locations ) override;

        void notifyDataIsReady() override;
        bool _isDataReadyImpl() const override;

        void _setToDisplayDummyTexture() override;
        void _notifyTextureSlotChanged( const TexturePool *newPool, uint16 slice ) override;

        /// Returns the GLuid of the texture that is being displayed. While the texture is
        /// being loaded (i.e. data is not ready), we will display a dummy white texture.
        /// Once notifyDataIsReady, getDisplayTextureName should be the same as
        /// getFinalTextureName. In other words, getDisplayTextureName may change its
        /// returned value based on the texture's status
        GLuint getDisplayTextureName() const { return mDisplayTextureName; }

        /// Always returns the internal handle that belongs to this texture.
        /// Note that for TextureFlags::AutomaticBatching textures, this will be the
        /// handle of a 2D Array texture pool.
        ///
        /// If the texture has MSAA enabled, this returns the handle to the resolve
        /// texture, not the MSAA one.
        ///
        /// If TextureFlags::MsaaExplicitResolve is set, it returns the handle
        /// to the MSAA texture, since there is no resolve texture.
        GLuint getFinalTextureName() const { return mFinalTextureName; }

        /// If MSAA > 1u and TextureFlags::MsaaExplicitResolve is not set, this
        /// returns the handle to the temporary MSAA renderbuffer used for rendering,
        /// which will later be resolved into the resolve texture.
        ///
        /// Otherwise it returns null.
        GLuint getMsaaFramebufferName() const { return mMsaaFramebufferName; }

        /// Returns GL_TEXTURE_2D / GL_TEXTURE_2D_ARRAY / etc
        GLenum getGlTextureTarget() const { return mGlTextureTarget; }

        void getCustomAttribute( IdString name, void *pData ) override;
    };

    class _OgreGL3PlusExport GL3PlusTextureGpuRenderTarget : public GL3PlusTextureGpu
    {
    protected:
        uint16         mDepthBufferPoolId;
        bool           mPreferDepthTexture;
        PixelFormatGpu mDesiredDepthBufferFormat;
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        OrientationMode mOrientationMode;
#endif
        void createInternalResourcesImpl() override;
        void destroyInternalResourcesImpl() override;

    public:
        GL3PlusTextureGpuRenderTarget( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                       VaoManager *vaoManager, IdString name, uint32 textureFlags,
                                       TextureTypes::TextureTypes initialType,
                                       TextureGpuManager         *textureManager );
        ~GL3PlusTextureGpuRenderTarget() override;

        void           _setDepthBufferDefaults( uint16 depthBufferPoolId, bool preferDepthTexture,
                                                PixelFormatGpu desiredDepthBufferFormat ) override;
        uint16         getDepthBufferPoolId() const override;
        bool           getPreferDepthTexture() const override;
        PixelFormatGpu getDesiredDepthBufferFormat() const override;

        void setOrientationMode( OrientationMode orientationMode ) override;
#if OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        OrientationMode getOrientationMode() const override;
#endif
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
