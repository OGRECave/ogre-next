/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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

#ifndef __NULLRenderSystem_H__
#define __NULLRenderSystem_H__

#include "OgreNULLPrerequisites.h"

#include "OgreRenderSystem.h"

namespace Ogre
{
    namespace v1
    {
        class HardwareBufferManager;
    }

    class _OgreNULLExport NULLPixelFormatToShaderType final : public PixelFormatToShaderType
    {
    public:
        const char *getPixelFormatType( PixelFormatGpu pixelFormat ) const override { return 0; }
        const char *getDataType( PixelFormatGpu pixelFormat, uint32 textureType, bool isMsaa,
                                 ResourceAccess::ResourceAccess access ) const override
        {
            return 0;
        }
    };

    /**
       Implementation of NULL as a rendering system.
    */
    class _OgreNULLExport NULLRenderSystem final : public RenderSystem
    {
        bool mInitialized;

        v1::HardwareBufferManager *mHardwareBufferManager;

        ConfigOptionMap mOptions;

        NULLPixelFormatToShaderType mPixelFormatToShaderType;

    public:
        NULLRenderSystem();
        ~NULLRenderSystem() override;

        void shutdown() override;

        const String &getName() const override;
        const String &getFriendlyName() const override;

        ConfigOptionMap &getConfigOptions() override { return mOptions; }
        void             setConfigOption( const String &name, const String &value ) override {}

        HardwareOcclusionQuery *createHardwareOcclusionQuery() override;

        String validateConfigOptions() override { return BLANKSTRING; }

        RenderSystemCapabilities *createRenderSystemCapabilities() const override;

        void reinitialise() override;

        Window *_initialise( bool          autoCreateWindow,
                             const String &windowTitle = "OGRE Render Window" ) override;

        Window *_createRenderWindow( const String &name, uint32 width, uint32 height, bool fullScreen,
                                     const NameValuePairList *miscParams = 0 ) override;

        String getErrorDescription( long errorNumber ) const override;

        void _useLights( const LightList &lights, unsigned short limit ) override;
        void _setWorldMatrix( const Matrix4 &m ) override;
        void _setViewMatrix( const Matrix4 &m ) override;
        void _setProjectionMatrix( const Matrix4 &m ) override;

        void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                const ColourValue &specular, const ColourValue &emissive, Real shininess,
                                TrackVertexColourType tracking = TVC_NONE ) override;
        void _setPointSpritesEnabled( bool enabled ) override;
        void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                  Real quadratic, Real minSize, Real maxSize ) override;

        void _setCurrentDeviceFromTexture( TextureGpu *texture ) override;
        void _setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                           uint32 hazardousTexIdx ) override;
        void _setTextures( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplers( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set ) override;
        void _setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set ) override;
        void _setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set ) override;
        void _setUavCS( uint32 slotStart, const DescriptorSetUav *set ) override;

        void _setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                          const Frustum *frustum = 0 ) override;
        void _setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) override;
        void _setTextureMatrix( size_t unit, const Matrix4 &xform ) override;

        void _setIndirectBuffer( IndirectBufferPacked *indirectBuffer ) override;

        RenderPassDescriptor *createRenderPassDescriptor() override;

        void _beginFrame() override;
        void _endFrame() override;

        void _setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *Samplerblock ) override;
        void _setPipelineStateObject( const HlmsPso *pso ) override;
        void _setComputePso( const HlmsComputePso *pso ) override;

        VertexElementType getColourVertexElementType() const override;
        void              _convertProjectionMatrix( const Matrix4 &matrix, Matrix4 &dest ) override {}

        void _dispatch( const HlmsComputePso &pso ) override;

        void _setVertexArrayObject( const VertexArrayObject *vao ) override;

        void _render( const CbDrawCallIndexed *cmd ) override;
        void _render( const CbDrawCallStrip *cmd ) override;
        void _renderEmulated( const CbDrawCallIndexed *cmd ) override;
        void _renderEmulated( const CbDrawCallStrip *cmd ) override;

        void _setRenderOperation( const v1::CbRenderOp *cmd ) override;
        void _render( const v1::CbDrawCallIndexed *cmd ) override;
        void _render( const v1::CbDrawCallStrip *cmd ) override;

        void bindGpuProgramParameters( GpuProgramType gptype, GpuProgramParametersSharedPtr params,
                                       uint16 variabilityMask ) override;
        void bindGpuProgramPassIterationParameters( GpuProgramType gptype ) override;

        void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                               uint8 mipLevel ) override;

        Real getHorizontalTexelOffset() override;
        Real getVerticalTexelOffset() override;
        Real getMinimumDepthInputValue() override;
        Real getMaximumDepthInputValue() override;

        void preExtraThreadsStarted() override;
        void postExtraThreadsStarted() override;
        void registerThread() override;
        void unregisterThread() override;

        unsigned int getDisplayMonitorCount() const override { return 1; }

        const PixelFormatToShaderType *getPixelFormatToShaderType() const override;

        void flushCommands() override;

        void beginProfileEvent( const String &eventName ) override;
        void endProfileEvent() override;
        void markProfileEvent( const String &event ) override;

        void initGPUProfiling() override;
        void deinitGPUProfiling() override;
        void beginGPUSampleProfile( const String &name, uint32 *hashCache ) override;
        void endGPUSampleProfile( const String &name ) override;

        bool hasAnisotropicMipMapFilter() const override { return true; }

        void setClipPlanesImpl( const PlaneList &clipPlanes ) override;
        void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                     Window                   *primary ) override;
    };
}  // namespace Ogre

#endif
