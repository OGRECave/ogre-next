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

#include "OgreHlmsDatablock.h"

#include "OgreConstBufferPool.h"
#include "OgreTextureGpuListener.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */

    /** This is not a regular header, therefore it has no include guards.
        This header contains a base class that defines
        common functionality across multiple Hlms datablock implementations that need
        textures. However C++ does not let us statically define the size of
        mTexIndices, mTextures & mSamplerblocks array; we would have to either force the
        same maximum to all implementations, or use the heap.
    @par
        Therefore this header uses macros to customize each base class for each implementations.
        DO NOT INCLUDE THIS HEADER DIRECTLY.

        The way to include this header is by doing:

        @code
        #define _OgreHlmsTextureBaseClassExport _OgreHlmsUnlitExport
        #define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsUnlitBaseTextureDatablock
        #define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_UNLIT_TEXTURE_TYPES
        #define OGRE_HLMS_CREATOR_CLASS HlmsUnlit
            #include "OgreHlmsTextureBaseClass.h"
        #undef _OgreHlmsTextureBaseClassExport
        #undef OGRE_HLMS_TEXTURE_BASE_CLASS
        #undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
        #undef OGRE_HLMS_CREATOR_CLASS
        @endcode

        Where all the necessary macros are defined prior to including this header,
        and once we're done, we undef these macros.

        When OGRE_HLMS_TEXTURE_BASE_CLASS is not defined, the *.inl version of this
        file will include a few headers in order to get proper syntax highlighting.
    */
    class _OgreHlmsTextureBaseClassExport OGRE_HLMS_TEXTURE_BASE_CLASS : public HlmsDatablock,
                                                                         public ConstBufferPoolUser,
                                                                         public TextureGpuListener
    {
    protected:
        /// The last bit in mTexIndices (ManualTexIndexBit) is reserved. When set, it
        /// indicates that mTexIndices was set manually, rather than being automatically
        /// set to texture[i]->getInternalSliceStart()
        uint16 mTexIndices[OGRE_HLMS_TEXTURE_BASE_MAX_TEX];

        DescriptorSetTexture const *mTexturesDescSet;
        DescriptorSetSampler const *mSamplersDescSet;

        TextureGpu *mTextures[OGRE_HLMS_TEXTURE_BASE_MAX_TEX];
        HlmsSamplerblock const *mSamplerblocks[OGRE_HLMS_TEXTURE_BASE_MAX_TEX];

        uint8 mTexLocationInDescSet[OGRE_HLMS_TEXTURE_BASE_MAX_TEX];

        void scheduleConstBufferUpdate( bool updateTextures = false, bool updateSamplers = false );
        void updateDescriptorSets( bool textureSetDirty, bool samplerSetDirty );

        /// Expects caller to call flushRenderables if we return true.
        virtual void bakeTextures( bool hasSeparateSamplers );
        /// Expects caller to call flushRenderables if we return true.
        void bakeSamplers();

        void cloneImpl( HlmsDatablock *datablock ) const override;

    public:
        OGRE_HLMS_TEXTURE_BASE_CLASS( IdString name, Hlms *creator, const HlmsMacroblock *macroblock,
                                      const HlmsBlendblock *blendblock, const HlmsParamVec &params );
        ~OGRE_HLMS_TEXTURE_BASE_CLASS() override;

        void setAlphaTestThreshold( float threshold ) override;

        void preload() override;

        void saveTextures( const String &folderPath, set<String>::type &savedTextures, bool saveOitd,
                           bool saveOriginal, HlmsTextureExportListener *listener ) override;

        /** Sets a new texture for rendering. Calling this function may trigger an
            HlmsDatablock::flushRenderables if the texture or the samplerblock changes.
            Might not be called if old and new texture belong to the same TexturePool.
        @param texType
            Texture unit. Must be in range [0; OGRE_HLMS_TEXTURE_BASE_MAX_TEX)
        @param texture
            Texture to change to. If it is null and previously wasn't (or viceversa), will
            trigger HlmsDatablock::flushRenderables.
        @param refParams
            Optional. We'll create (or retrieve an existing) samplerblock based on the input parameters.
            When null, we leave the previously set samplerblock (if a texture is being set, and if no
            samplerblock was set, we'll create a default one)
        @param sliceIdx
            Optional. When not set to 0xFFFF, it means you want to explicitly set the texture array
            index, instead of relying on texture->getInternalSliceStart().
            Only useful if texture is TextureTypes::Type2DArray. For advanced users.
        */
        void setTexture( uint8 texType, TextureGpu *texture, const HlmsSamplerblock *refParams = 0,
                         uint16 sliceIdx = std::numeric_limits<uint16>::max() );
        TextureGpu *getTexture( uint8 texType ) const;

        /// Same as setTexture, but samplerblockPtr is a raw samplerblock retrieved from HlmsManager,
        /// and is assumed to have its reference count already be incremented for us
        /// (note HlmsManager::getSamplerblock() already increments the ref. count).
        /// Mostly for internal use, but can speed up loading if you already manage samplerblocks
        /// manually and have the raw ptr.
        void _setTexture( uint8 texType, TextureGpu *texture,
                          const HlmsSamplerblock *samplerblockPtr = 0,
                          uint16 sliceIdx = std::numeric_limits<uint16>::max() );

        /** Sets a new sampler block to be associated with the texture
            (i.e. filtering mode, addressing modes, etc). If the samplerblock changes,
            this function will always trigger a HlmsDatablock::flushRenderables
        @param texType
            Texture unit. Must be in range [0; OGRE_HLMS_TEXTURE_BASE_MAX_TEX)
        @param params
            The sampler block to use as reference.
        */
        void setSamplerblock( uint8 texType, const HlmsSamplerblock &params );
        const HlmsSamplerblock *getSamplerblock( uint8 texType ) const;

        /// Same as setSamplerblock, but samplerblockPtr is a raw samplerblock retrieved from
        /// HlmsManager, and is assumed to have its reference count already be incremented for us.
        /// See _setTexture.
        void _setSamplerblock( uint8 texType, const HlmsSamplerblock *samplerblockPtr );

        /// This function has O( log N ) complexity, but O(1) if the texture was not set.
        uint8 getIndexToDescriptorTexture( uint8 texType );
        /// Do not call this function if RSC_SEPARATE_SAMPLERS_FROM_TEXTURES is not set.
        /// If not set, then just the result value from getIndexToDescriptorTexture
        /// instead. Same complexity as getIndexToDescriptorTexture
        uint8 getIndexToDescriptorSampler( uint8 texType );

        void notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                   void *extraData ) override;
        bool shouldStayLoaded( TextureGpu *texture ) override;

        void loadAllTextures();
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"
