/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2016 Torus Knot Software Ltd

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

#include "OgreMetalRenderSystem.h"
#include "OgreMetalWindow.h"
#include "OgreMetalTextureManager.h"
#include "Vao/OgreMetalVaoManager.h"
#include "Vao/OgreMetalBufferInterface.h"
#include "OgreMetalHlmsPso.h"
#include "OgreMetalRenderTargetCommon.h"
#include "OgreMetalDepthBuffer.h"
#include "OgreMetalDevice.h"
#include "OgreMetalGpuProgramManager.h"
#include "OgreMetalProgram.h"
#include "OgreMetalProgramFactory.h"
#include "OgreMetalTexture.h"
#include "OgreMetalMultiRenderTarget.h"
#include "OgreMetalTextureGpu.h"
#include "OgreMetalRenderPassDescriptor.h"
#include "OgreMetalDescriptorSetTexture.h"
#include "Vao/OgreMetalTexBufferPacked.h"

#include "OgreMetalHardwareBufferManager.h"
#include "OgreMetalHardwareIndexBuffer.h"
#include "OgreMetalHardwareVertexBuffer.h"

#include "OgreMetalTextureGpuManager.h"

#include "Vao/OgreMetalConstBufferPacked.h"
#include "Vao/OgreMetalUavBufferPacked.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreVertexArrayObject.h"
#include "CommandBuffer/OgreCbDrawCall.h"

#include "OgreFrustum.h"
#include "OgreViewport.h"
#include "Compositor/OgreCompositorManager2.h"

#include "OgreMetalMappings.h"

#import <Metal/Metal.h>
#import <Foundation/NSEnumerator.h>


namespace Ogre
{
    //-------------------------------------------------------------------------
    MetalRenderSystem::MetalRenderSystem() :
        RenderSystem(),
        mInitialized( false ),
        mHardwareBufferManager( 0 ),
        mShaderManager( 0 ),
        mMetalProgramFactory( 0 ),
        mIndirectBuffer( 0 ),
        mSwIndirectBufferPtr( 0 ),
        mPso( 0 ),
        mComputePso( 0 ),
        mCurrentIndexBuffer( 0 ),
        mCurrentVertexBuffer( 0 ),
        mCurrentPrimType( MTLPrimitiveTypePoint ),
        mAutoParamsBufferIdx( 0 ),
        mCurrentAutoParamsBufferPtr( 0 ),
        mCurrentAutoParamsBufferSpaceLeft( 0 ),
        mNumMRTs( 0 ),
        mCurrentDepthBuffer( 0 ),
        mActiveDevice( 0 ),
        mActiveRenderEncoder( 0 ),
        mDevice( this ),
        mMainGpuSyncSemaphore( 0 ),
        mMainSemaphoreAlreadyWaited( false ),
        mBeginFrameOnceStarted( false ),
        mEntriesToFlush( 0 ),
        mVpChanged( false ),
        mInterruptedRenderCommandEncoder( false )
    {
        memset( mHistoricalAutoParamsSize, 0, sizeof(mHistoricalAutoParamsSize) );
        for( size_t i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            mCurrentColourRTs[i] = 0;
    }
    //-------------------------------------------------------------------------
    MetalRenderSystem::~MetalRenderSystem()
    {
        shutdown();

        // Destroy render windows
        RenderTargetMap::iterator i;
        for (i = mRenderTargets.begin(); i != mRenderTargets.end(); ++i)
            OGRE_DELETE i->second;
        mRenderTargets.clear();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::shutdown(void)
    {
        for( size_t i=0; i<mAutoParamsBuffer.size(); ++i )
        {
            if( mAutoParamsBuffer[i]->getMappingState() != MS_UNMAPPED )
                mAutoParamsBuffer[i]->unmap( UO_UNMAP_ALL );
            mVaoManager->destroyConstBuffer( mAutoParamsBuffer[i] );
        }
        mAutoParamsBuffer.clear();
        mAutoParamsBufferIdx = 0;
        mCurrentAutoParamsBufferPtr = 0;
        mCurrentAutoParamsBufferSpaceLeft = 0;

        RenderSystem::shutdown();

        OGRE_DELETE mHardwareBufferManager;
        mHardwareBufferManager = 0;

        if( mMetalProgramFactory )
        {
            // Remove from manager safely
            if( HighLevelGpuProgramManager::getSingletonPtr() )
                HighLevelGpuProgramManager::getSingleton().removeFactory( mMetalProgramFactory );
            OGRE_DELETE mMetalProgramFactory;
            mMetalProgramFactory = 0;
        }

        OGRE_DELETE mShaderManager;
        mShaderManager = 0;

        OGRE_DELETE mTextureManager;
        mTextureManager = 0;
    }
    //-------------------------------------------------------------------------
    const String& MetalRenderSystem::getName(void) const
    {
        static String strName("Metal Rendering Subsystem");
        return strName;
    }
    //-------------------------------------------------------------------------
    const String& MetalRenderSystem::getFriendlyName(void) const
    {
        static String strName("Metal_RS");
        return strName;
    }
    //-------------------------------------------------------------------------
    HardwareOcclusionQuery* MetalRenderSystem::createHardwareOcclusionQuery(void)
    {
        return 0; //TODO
    }
    //-------------------------------------------------------------------------
    RenderSystemCapabilities* MetalRenderSystem::createRenderSystemCapabilities(void) const
    {
        RenderSystemCapabilities* rsc = new RenderSystemCapabilities();
        rsc->setRenderSystemName(getName());

        rsc->setCapability(RSC_HWSTENCIL);
        rsc->setStencilBufferBitDepth(8);
        rsc->setNumTextureUnits(16);
        rsc->setNumVertexTextureUnits(16);
        rsc->setCapability(RSC_ANISOTROPY);
        rsc->setCapability(RSC_AUTOMIPMAP);
        rsc->setCapability(RSC_BLENDING);
        rsc->setCapability(RSC_DOT3);
        rsc->setCapability(RSC_CUBEMAPPING);
        rsc->setCapability(RSC_TEXTURE_COMPRESSION);
#if TARGET_OS_TV
        rsc->setCapability(RSC_TEXTURE_COMPRESSION_ASTC);
#endif
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        rsc->setCapability(RSC_TEXTURE_COMPRESSION_DXT);
        rsc->setCapability(RSC_TEXTURE_COMPRESSION_BC4_BC5);
        //rsc->setCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7);
#else
        //Actually the limit is not the count but rather how many bytes are in the
        //GPU's internal TBDR cache (16 bytes for Family 1, 32 bytes for the rest)
        //Consult Metal's manual for more info.
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1] )
        {
            rsc->setCapability(RSC_TEXTURE_COMPRESSION_ASTC);
        }

        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily4_v1] )
            rsc->setCapability(RSC_TEXTURE_CUBE_MAP_ARRAY);
#endif
        rsc->setCapability(RSC_VBO);
        rsc->setCapability(RSC_32BIT_INDEX);
        rsc->setCapability(RSC_TWO_SIDED_STENCIL);
        rsc->setCapability(RSC_STENCIL_WRAP);
        rsc->setCapability(RSC_USER_CLIP_PLANES);
        rsc->setCapability(RSC_VERTEX_FORMAT_UBYTE4);
        rsc->setCapability(RSC_INFINITE_FAR_PLANE);
        rsc->setCapability(RSC_NON_POWER_OF_2_TEXTURES);
        rsc->setNonPOW2TexturesLimited(false);
        rsc->setCapability(RSC_HWRENDER_TO_TEXTURE);
        rsc->setCapability(RSC_TEXTURE_FLOAT);
        rsc->setCapability(RSC_POINT_SPRITES);
        rsc->setCapability(RSC_POINT_EXTENDED_PARAMETERS);
        rsc->setCapability(RSC_TEXTURE_1D);
        rsc->setCapability(RSC_TEXTURE_3D);
        rsc->setCapability(RSC_TEXTURE_SIGNED_INT);
        rsc->setCapability(RSC_VERTEX_PROGRAM);
        rsc->setCapability(RSC_FRAGMENT_PROGRAM);
        rsc->setCapability(RSC_VERTEX_BUFFER_INSTANCE_DATA);
        rsc->setCapability(RSC_MIPMAP_LOD_BIAS);
        rsc->setCapability(RSC_ALPHA_TO_COVERAGE);
        rsc->setMaxPointSize(256);

        rsc->setCapability(RSC_COMPUTE_PROGRAM);
        rsc->setCapability(RSC_HW_GAMMA);
        rsc->setCapability(RSC_TEXTURE_GATHER);
        rsc->setCapability(RSC_TEXTURE_2D_ARRAY);
        rsc->setCapability(RSC_SEPARATE_SAMPLERS_FROM_TEXTURES);
        rsc->setCapability(RSC_CONST_BUFFER_SLOTS_IN_SHADER);
        rsc->setCapability(RSC_EXPLICIT_FSAA_RESOLVE);

        //These don't make sense on Metal, so just use flexible defaults.
        rsc->setVertexProgramConstantFloatCount( 16384 );
        rsc->setVertexProgramConstantBoolCount( 16384 );
        rsc->setVertexProgramConstantIntCount( 16384 );
        rsc->setFragmentProgramConstantFloatCount( 16384 );
        rsc->setFragmentProgramConstantBoolCount( 16384 );
        rsc->setFragmentProgramConstantIntCount( 16384 );
        rsc->setComputeProgramConstantFloatCount( 16384 );
        rsc->setComputeProgramConstantBoolCount( 16384 );
        rsc->setComputeProgramConstantIntCount( 16384 );

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        uint8 mrtCount = 8u;
#else
        //Actually the limit is not the count but rather how many bytes are in the
        //GPU's internal TBDR cache (16 bytes for Family 1, 32 bytes for the rest)
        //Consult Metal's manual for more info.
        uint8 mrtCount = 4u;
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1] ||
            [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2] ||
            [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1] )
        {
            mrtCount = 8u;
        }
#endif
        rsc->setNumMultiRenderTargets( std::min<int>(mrtCount, OGRE_MAX_MULTIPLE_RENDER_TARGETS) );
        rsc->setCapability(RSC_MRT_DIFFERENT_BIT_DEPTHS);

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        uint16 max2DResolution = 16384;
#else
        uint16 max2DResolution = 4096;
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v2] ||
            [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2] )
        {
            max2DResolution = 8192;
        }
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1] )
        {
            max2DResolution = 16384;
        }
#endif
        rsc->setMaximumResolutions( max2DResolution, 2048, max2DResolution );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v2] )
            rsc->setCapability(RSC_STORE_AND_MULTISAMPLE_RESOLVE);
#else
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_OSX_GPUFamily1_v2] )
            rsc->setCapability(RSC_STORE_AND_MULTISAMPLE_RESOLVE);
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        rsc->setCapability( RSC_IS_TILER );
        rsc->setCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION );
#endif

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        rsc->setCapability(RSC_UAV);
        rsc->setCapability(RSC_TEXTURE_CUBE_MAP_ARRAY);
#endif
        rsc->setCapability( RSC_TYPED_UAV_LOADS );
        //rsc->setCapability(RSC_ATOMIC_COUNTERS);

        rsc->addShaderProfile( "metal" );

        DriverVersion driverVersion;

        struct FeatureSets
        {
            MTLFeatureSet featureSet;
            const char* name;
            int major;
            int minor;
        };

        FeatureSets featureSets[] =
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            { MTLFeatureSet_iOS_GPUFamily1_v1, "iOS_GPUFamily1_v1", 1, 1 },
            { MTLFeatureSet_iOS_GPUFamily2_v1, "iOS_GPUFamily2_v1", 2, 1 },

            { MTLFeatureSet_iOS_GPUFamily1_v2, "iOS_GPUFamily1_v2", 1, 2 },
            { MTLFeatureSet_iOS_GPUFamily2_v2, "iOS_GPUFamily2_v2", 2, 2 },
            { MTLFeatureSet_iOS_GPUFamily3_v1, "iOS_GPUFamily3_v2", 3, 2 },

            { MTLFeatureSet_iOS_GPUFamily4_v1, "iOS_GPUFamily4_v1", 4, 1 },

    #ifdef __IPHONE_12_0
            { MTLFeatureSet_iOS_GPUFamily4_v2, "iOS_GPUFamily4_v2", 4, 2 },
            { MTLFeatureSet_iOS_GPUFamily5_v1, "iOS_GPUFamily5_v1", 5, 1 },
    #endif
#else
            { MTLFeatureSet_OSX_GPUFamily1_v1, "OSX_GPUFamily1_v1", 1, 1 },
#endif
        };

        for( size_t i=0; i<sizeof(featureSets) / sizeof(featureSets[0]); ++i )
        {
            if( [mActiveDevice->mDevice supportsFeatureSet:featureSets[i].featureSet] )
            {
                LogManager::getSingleton().logMessage( "Supports: " + String(featureSets[i].name) );
                driverVersion.major = featureSets[i].major;
                driverVersion.minor = featureSets[i].minor;
            }
        }

        rsc->setDriverVersion( driverVersion );

        uint32 threadgroupLimits[3] = { 1024u, 1024u, 64u };
        uint32 maxThreadsPerThreadgroup = 1024u;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if( driverVersion.major < 4 )
        {
            threadgroupLimits[0] = 512u;
            threadgroupLimits[1] = 512u;
            maxThreadsPerThreadgroup = 512u;
        }
#endif
        rsc->setMaxThreadsPerThreadgroupAxis( threadgroupLimits );
        rsc->setMaxThreadsPerThreadgroup( maxThreadsPerThreadgroup );

        return rsc;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::reinitialise(void)
    {
        this->shutdown();
        this->_initialise(true);
    }
    //-------------------------------------------------------------------------
    Window* MetalRenderSystem::_initialise( bool autoCreateWindow, const String& windowTitle )
    {
        Window *autoWindow = 0;
        if( autoCreateWindow )
            autoWindow = _createRenderWindow( windowTitle, 1, 1, false );
        RenderSystem::_initialise( autoCreateWindow, windowTitle );

        return autoWindow;
    }
    //-------------------------------------------------------------------------
    Window *MetalRenderSystem::_createRenderWindow( const String &name, uint32 width, uint32 height,
                                                    bool fullScreen,
                                                    const NameValuePairList *miscParams )
    {
        if( !mInitialized )
        {
            mDevice.init();
            setActiveDevice(&mDevice);

            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );
            }

            const long c_inFlightCommandBuffers = 3;
            mMainGpuSyncSemaphore = dispatch_semaphore_create(c_inFlightCommandBuffers);
            mMainSemaphoreAlreadyWaited = false;
            mBeginFrameOnceStarted = false;
            mRealCapabilities = createRenderSystemCapabilities();
            mDriverVersion = mRealCapabilities->getDriverVersion();

            if (!mUseCustomCapabilities)
                mCurrentCapabilities = mRealCapabilities;

            fireEvent("RenderSystemCapabilitiesCreated");

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, 0 );

            mTextureManager = new MetalTextureManager( &mDevice );
            mVaoManager = OGRE_NEW MetalVaoManager( c_inFlightCommandBuffers, &mDevice, miscParams );
            mHardwareBufferManager = new v1::MetalHardwareBufferManager( &mDevice, mVaoManager );
            mTextureGpuManager = OGRE_NEW MetalTextureGpuManager( mVaoManager, this, &mDevice );

            mInitialized = true;
        }

        Window *win = OGRE_NEW MetalWindow( name, width, height, fullScreen, miscParams, &mDevice );
        win->_initialize( mTextureGpuManager );
        return win;
    }
    //-------------------------------------------------------------------------
    MultiRenderTarget* MetalRenderSystem::createMultiRenderTarget( const String & name )
    {
        MetalMultiRenderTarget *retVal = OGRE_NEW MetalMultiRenderTarget( name );
        attachRenderTarget( *retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    String MetalRenderSystem::getErrorDescription(long errorNumber) const
    {
        return BLANKSTRING;
    }
    //-------------------------------------------------------------------------
    bool MetalRenderSystem::hasStoreAndMultisampleResolve(void) const
    {
        return mCurrentCapabilities->hasCapability( RSC_STORE_AND_MULTISAMPLE_RESOLVE );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_useLights(const LightList& lights, unsigned short limit)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setWorldMatrix(const Matrix4 &m)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setViewMatrix(const Matrix4 &m)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setProjectionMatrix(const Matrix4 &m)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setSurfaceParams( const ColourValue &ambient,
                            const ColourValue &diffuse, const ColourValue &specular,
                            const ColourValue &emissive, Real shininess,
                            TrackVertexColourType tracking )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPointSpritesEnabled(bool enabled)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPointParameters(Real size, bool attenuationEnabled,
        Real constant, Real linear, Real quadratic, Real minSize, Real maxSize)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::flushUAVs(void)
    {
        if( mUavRenderingDirty && mActiveRenderEncoder )
        {
            if( mUavRenderingDescSet )
            {
                MetalDescriptorSetTexture *metalSet =
                        reinterpret_cast<MetalDescriptorSetTexture*>( mUavRenderingDescSet->mRsData );

                //Bind textures
                {
                    FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
                    FastArray<MetalTexRegion>::const_iterator end  = metalSet->textures.end();

                    while( itor != end )
                    {
                        NSRange range = itor->range;
                        range.location += /*mUavStartingSlot +*/ OGRE_METAL_UAV_SLOT_START;
                        [mActiveRenderEncoder setVertexTextures:itor->textures withRange:range];
                        [mActiveRenderEncoder setFragmentTextures:itor->textures withRange:range];
                        ++itor;
                    }
                }

                //Bind buffers
                {
                    FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
                    FastArray<MetalBufferRegion>::const_iterator end  = metalSet->buffers.end();

                    while( itor != end )
                    {
                        NSRange range = itor->range;
                        range.location += /*mUavStartingSlot +*/ OGRE_METAL_UAV_SLOT_START;

                        [mActiveRenderEncoder setVertexBuffers:itor->buffers
                                                       offsets:itor->offsets
                                                     withRange:range];
                        [mActiveRenderEncoder setFragmentBuffers:itor->buffers
                                                         offsets:itor->offsets
                                                       withRange:range];
                        ++itor;
                    }
                }
            }
            mUavRenderingDirty = false;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr )
    {
        if( texPtr )
        {
            const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu*>( texPtr );
            __unsafe_unretained id<MTLTexture> metalTexture = metalTex->getDisplayTextureName();

            [mActiveRenderEncoder setVertexTexture:metalTexture atIndex:unit];
            [mActiveRenderEncoder setFragmentTexture:metalTexture atIndex:unit];
        }
        else
        {
            [mActiveRenderEncoder setVertexTexture:0 atIndex:unit];
            [mActiveRenderEncoder setFragmentTexture:0 atIndex:unit];
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture *set,
                                          uint32 hazardousTexIdx )
    {
        uint32 texUnit = slotStart;
        FastArray<const TextureGpu*>::const_iterator itor = set->mTextures.begin();

        //for( size_t i=0u; i<NumShaderTypes; ++i )
        for( size_t i=0u; i<PixelShader + 1u; ++i )
        {
            const size_t numTexturesUsed = set->mShaderTypeTexCount[i];
            for( size_t j=0u; j<numTexturesUsed; ++j )
            {
                const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu*>( *itor );
                __unsafe_unretained id<MTLTexture> metalTexture = 0;

                if( metalTex )
                {
                    metalTexture = metalTex->getDisplayTextureName();

                    if( (texUnit - slotStart) == hazardousTexIdx &&
                        mCurrentRenderPassDescriptor->hasAttachment( set->mTextures[hazardousTexIdx] ) )
                    {
                        metalTexture = nil;
                    }
                }

                switch( i )
                {
                case VertexShader:
                    [mActiveRenderEncoder setVertexTexture:metalTexture atIndex:texUnit];
                    break;
                case PixelShader:
                    [mActiveRenderEncoder setFragmentTexture:metalTexture atIndex:texUnit];
                    break;
                case GeometryShader:
                case HullShader:
                case DomainShader:
                    break;
                }

                ++texUnit;
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextures( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        MetalDescriptorSetTexture *metalSet =
                reinterpret_cast<MetalDescriptorSetTexture*>( set->mRsData );

        //Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator end  = metalSet->textures.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart;

                switch( itor->shaderType )
                {
                case VertexShader:
                    [mActiveRenderEncoder setVertexTextures:itor->textures withRange:range];
                    break;
                case PixelShader:
                    [mActiveRenderEncoder setFragmentTextures:itor->textures withRange:range];
                    break;
                case GeometryShader:
                case HullShader:
                case DomainShader:
                case NumShaderTypes:
                    break;
                }

                ++itor;
            }
        }

        //Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator end  = metalSet->buffers.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart + OGRE_METAL_TEX_SLOT_START;

                switch( itor->shaderType )
                {
                case VertexShader:
                    [mActiveRenderEncoder setVertexBuffers:itor->buffers
                                                   offsets:itor->offsets
                                                 withRange:range];
                    break;
                case PixelShader:
                    [mActiveRenderEncoder setFragmentBuffers:itor->buffers
                                                     offsets:itor->offsets
                                                   withRange:range];
                    break;
                case GeometryShader:
                case HullShader:
                case DomainShader:
                case NumShaderTypes:
                    break;
                }

                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setSamplers( uint32 slotStart, const DescriptorSetSampler *set )
    {
        __unsafe_unretained id <MTLSamplerState> samplers[16];

        FastArray<const HlmsSamplerblock*>::const_iterator itor = set->mSamplers.begin();

        NSRange texUnitRange;
        texUnitRange.location = slotStart;
        //for( size_t i=0u; i<NumShaderTypes; ++i )
        for( size_t i=0u; i<PixelShader + 1u; ++i )
        {
            const NSUInteger numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j=0; j<numSamplersUsed; ++j )
            {
                if( *itor )
                    samplers[j] = (__bridge id<MTLSamplerState>)(*itor)->mRsData;
                else
                    samplers[j] = 0;
                ++itor;
            }

            texUnitRange.length = numSamplersUsed;

            switch( i )
            {
            case VertexShader:
                [mActiveRenderEncoder setVertexSamplerStates:samplers withRange:texUnitRange];
                break;
            case PixelShader:
                [mActiveRenderEncoder setFragmentSamplerStates:samplers withRange:texUnitRange];
                break;
            case GeometryShader:
            case HullShader:
            case DomainShader:
                break;
            }

            texUnitRange.location += numSamplersUsed;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture *set )
    {
        FastArray<const TextureGpu*>::const_iterator itor = set->mTextures.begin();

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        size_t texIdx = 0;

        for( size_t i=0u; i<NumShaderTypes; ++i )
        {
            const size_t numTexturesUsed = set->mShaderTypeTexCount[i];
            for( size_t j=0u; j<numTexturesUsed; ++j )
            {
                const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu*>( *itor );
                __unsafe_unretained id<MTLTexture> metalTexture = 0;

                if( metalTex )
                    metalTexture = metalTex->getDisplayTextureName();

                [computeEncoder setTexture:metalTexture atIndex:slotStart + texIdx];
                texIdx += numTexturesUsed;

                ++texIdx;
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTexturesCS( uint32 slotStart, const DescriptorSetTexture2 *set )
    {
        MetalDescriptorSetTexture *metalSet =
                reinterpret_cast<MetalDescriptorSetTexture*>( set->mRsData );

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        //Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator end  = metalSet->textures.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart;
                [computeEncoder setTextures:itor->textures withRange:range];
                ++itor;
            }
        }

        //Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator end  = metalSet->buffers.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart + OGRE_METAL_CS_TEX_SLOT_START;
                [computeEncoder setBuffers:itor->buffers offsets:itor->offsets withRange:range];
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setSamplersCS( uint32 slotStart, const DescriptorSetSampler *set )
    {
        __unsafe_unretained id <MTLSamplerState> samplers[16];

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        FastArray<const HlmsSamplerblock*>::const_iterator itor = set->mSamplers.begin();

        NSRange texUnitRange;
        texUnitRange.location = slotStart;
        for( size_t i=0u; i<NumShaderTypes; ++i )
        {
            const NSUInteger numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j=0; j<numSamplersUsed; ++j )
            {
                if( *itor )
                    samplers[j] = (__bridge id<MTLSamplerState>)(*itor)->mRsData;
                else
                    samplers[j] = 0;
                ++itor;
            }

            texUnitRange.length = numSamplersUsed;

            [computeEncoder setSamplerStates:samplers withRange:texUnitRange];
            texUnitRange.location += numSamplersUsed;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setUavCS( uint32 slotStart, const DescriptorSetUav *set )
    {
        MetalDescriptorSetTexture *metalSet =
                reinterpret_cast<MetalDescriptorSetTexture*>( set->mRsData );

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        //Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator end  = metalSet->textures.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart + OGRE_METAL_CS_UAV_SLOT_START;
                [computeEncoder setTextures:itor->textures withRange:range];
                ++itor;
            }
        }

        //Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator end  = metalSet->buffers.end();

            while( itor != end )
            {
                NSRange range = itor->range;
                range.location += slotStart + OGRE_METAL_CS_UAV_SLOT_START;
                [computeEncoder setBuffers:itor->buffers offsets:itor->offsets withRange:range];
                ++itor;
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setCurrentDeviceFromTexture( TextureGpu *texture )
    {
        //mDevice
    }
    //-------------------------------------------------------------------------
    RenderPassDescriptor* MetalRenderSystem::createRenderPassDescriptor(void)
    {
        RenderPassDescriptor *retVal = OGRE_NEW MetalRenderPassDescriptor( mActiveDevice, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::beginRenderPassDescriptor( RenderPassDescriptor *desc,
                                                       TextureGpu *anyTarget, uint8 mipLevel,
                                                       const Vector4 &viewportSize,
                                                       const Vector4 &scissors,
                                                       bool overlaysEnabled,
                                                       bool warnIfRtvWasFlushed )
    {
        if( desc->mInformationOnly && desc->hasSameAttachments( mCurrentRenderPassDescriptor ) )
            return;

        const int oldWidth = mCurrentRenderViewport.getActualWidth();
        const int oldHeight = mCurrentRenderViewport.getActualHeight();
        const int oldX = mCurrentRenderViewport.getActualLeft();
        const int oldY = mCurrentRenderViewport.getActualTop();

        MetalRenderPassDescriptor *currPassDesc =
                static_cast<MetalRenderPassDescriptor*>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSize, scissors,
                                                 overlaysEnabled, warnIfRtvWasFlushed );


        // Calculate the new "lower-left" corner of the viewport to compare with the old one
        const int w = mCurrentRenderViewport.getActualWidth();
        const int h = mCurrentRenderViewport.getActualHeight();
        const int x = mCurrentRenderViewport.getActualLeft();
        const int y = mCurrentRenderViewport.getActualTop();

        const bool vpChanged = oldX != x || oldY != y || oldWidth != w || oldHeight != h;

        MetalRenderPassDescriptor *newPassDesc =
                static_cast<MetalRenderPassDescriptor*>( desc );

        //Determine whether:
        //  1. We need to store current active RenderPassDescriptor
        //  2. We need to perform clears when loading the new RenderPassDescriptor
        uint32 entriesToFlush = 0;
        if( currPassDesc )
        {
            entriesToFlush = currPassDesc->willSwitchTo( newPassDesc, warnIfRtvWasFlushed );

            if( entriesToFlush != 0 )
                currPassDesc->performStoreActions( entriesToFlush, false );

            //If rendering was interrupted but we're still rendering to the same
            //RTT, willSwitchTo will have returned 0 and thus we won't perform
            //the necessary load actions.
            //If RTTs were different, we need to have performStoreActions
            //called by now (i.e. to emulate StoreAndResolve)
            if( mInterruptedRenderCommandEncoder )
            {
                entriesToFlush = RenderPassDescriptor::All;
                if( warnIfRtvWasFlushed )
                    newPassDesc->checkWarnIfRtvWasFlushed( entriesToFlush );
            }
        }
        else
        {
            entriesToFlush = RenderPassDescriptor::All;
        }

        mActiveViewport = &mCurrentRenderViewport;

        mEntriesToFlush = entriesToFlush;
        mVpChanged      = vpChanged;
        mInterruptedRenderCommandEncoder = false;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::executeRenderPassDescriptorDelayedActions( bool officialCall )
    {
        if( officialCall )
            mInterruptedRenderCommandEncoder = false;

        if( mEntriesToFlush )
        {
            mActiveDevice->endAllEncoders( false );
            mActiveRenderEncoder = 0;

            MetalRenderPassDescriptor *newPassDesc =
                    static_cast<MetalRenderPassDescriptor*>( mCurrentRenderPassDescriptor );

            MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            newPassDesc->performLoadActions( passDesc, mInterruptedRenderCommandEncoder );

            mActiveDevice->mRenderEncoder =
                    [mActiveDevice->mCurrentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
            mActiveRenderEncoder = mActiveDevice->mRenderEncoder;

            static_cast<MetalVaoManager*>( mVaoManager )->bindDrawId();
            [mActiveRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];

            if (mStencilEnabled)
                [mActiveRenderEncoder setStencilReferenceValue:mStencilRefValue];

            mInterruptedRenderCommandEncoder = false;
        }

        flushUAVs();

        //If we flushed, viewport and scissor settings got reset.
        if( !mCurrentRenderViewport.coversEntireTarget() || (mVpChanged && !mEntriesToFlush) )
        {
            MTLViewport mtlVp;
            mtlVp.originX   = mCurrentRenderViewport.getActualLeft();
            mtlVp.originY   = mCurrentRenderViewport.getActualTop();
            mtlVp.width     = mCurrentRenderViewport.getActualWidth();
            mtlVp.height    = mCurrentRenderViewport.getActualHeight();
            mtlVp.znear     = 0;
            mtlVp.zfar      = 1;
            [mActiveRenderEncoder setViewport:mtlVp];
        }

        if( (!mCurrentRenderViewport.coversEntireTarget() ||
             !mCurrentRenderViewport.scissorsMatchViewport()) || !mEntriesToFlush )
        {
            MTLScissorRect scissorRect;
            scissorRect.x       = mCurrentRenderViewport.getScissorActualLeft();
            scissorRect.y       = mCurrentRenderViewport.getScissorActualTop();
            scissorRect.width   = mCurrentRenderViewport.getScissorActualWidth();
            scissorRect.height  = mCurrentRenderViewport.getScissorActualHeight();
            [mActiveRenderEncoder setScissorRect:scissorRect];
        }

        mEntriesToFlush = 0;
        mVpChanged = false;
        mInterruptedRenderCommandEncoder = false;

        if( mActiveDevice->mFrameAborted )
        {
            mActiveDevice->endAllEncoders();
            mActiveRenderEncoder = 0;
            return;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::executeRenderPassDescriptorDelayedActions(void)
    {
        executeRenderPassDescriptorDelayedActions( true );
    }
    //-------------------------------------------------------------------------
    inline void MetalRenderSystem::endRenderPassDescriptor( bool isInterruptingRender )
    {
        if( mCurrentRenderPassDescriptor )
        {
            uint32 x, y, w, h;
            w = mCurrentRenderViewport.getActualWidth();
            h = mCurrentRenderViewport.getActualHeight();
            x = mCurrentRenderViewport.getActualLeft();
            y = mCurrentRenderViewport.getActualTop();

            MetalRenderPassDescriptor *passDesc =
                    static_cast<MetalRenderPassDescriptor*>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions( RenderPassDescriptor::All, isInterruptingRender );

            mEntriesToFlush = 0;
            mVpChanged = false;

            mInterruptedRenderCommandEncoder = isInterruptingRender;

            if( !isInterruptingRender )
                RenderSystem::endRenderPassDescriptor();
            else
                mEntriesToFlush = RenderPassDescriptor::All;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::endRenderPassDescriptor(void)
    {
        endRenderPassDescriptor( false );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* MetalRenderSystem::createDepthBufferFor( TextureGpu *colourTexture,
                                                         bool preferDepthTexture,
                                                         PixelFormatGpu depthBufferFormat )
    {
        if( depthBufferFormat == PFG_UNKNOWN )
            depthBufferFormat = DepthBuffer::DefaultDepthBufferFormat;

        return RenderSystem::createDepthBufferFor( colourTexture, preferDepthTexture,
                                                   depthBufferFormat );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                        const Frustum* frustum )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureBlendMode(size_t unit, const LayerBlendModeEx& bm)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureMatrix(size_t unit, const Matrix4& xform)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
        if( mVaoManager->supportsIndirectBuffers() )
        {
            if( indirectBuffer )
            {
                MetalBufferInterface *bufferInterface = static_cast<MetalBufferInterface*>(
                                                            indirectBuffer->getBufferInterface() );
                mIndirectBuffer = bufferInterface->getVboName();
            }
            else
            {
                mIndirectBuffer = 0;
            }
        }
        else
        {
            if( indirectBuffer )
                mSwIndirectBufferPtr = indirectBuffer->getSwBufferPtr();
            else
                mSwIndirectBufferPtr = 0;
        }
    }
    //-------------------------------------------------------------------------
    DepthBuffer* MetalRenderSystem::_createDepthBufferFor( RenderTarget *renderTarget,
                                                           bool exactMatchFormat )
    {
        return 0;
#if TODO_OGRE_2_2
        MTLTextureDescriptor *desc = [MTLTextureDescriptor new];
        desc.sampleCount = renderTarget->getFSAA();
        desc.textureType = renderTarget->getFSAA() > 1u ? MTLTextureType2DMultisample :
                                                          MTLTextureType2D;
        desc.width              = (NSUInteger)renderTarget->getWidth();
        desc.height             = (NSUInteger)renderTarget->getHeight();
        desc.depth              = (NSUInteger)1u;
        desc.arrayLength        = 1u;
        desc.mipmapLevelCount   = 1u;

        desc.usage = MTLTextureUsageRenderTarget;
        if( renderTarget->prefersDepthTexture() )
            desc.usage |= MTLTextureUsageShaderRead;

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        desc.storageMode = MTLStorageModePrivate;
#endif

        PixelFormat desiredDepthBufferFormat = renderTarget->getDesiredDepthBufferFormat();

        MTLPixelFormat depthFormat = MTLPixelFormatInvalid;
        MTLPixelFormat stencilFormat = MTLPixelFormatInvalid;
        MetalMappings::getDepthStencilFormat( mActiveDevice, desiredDepthBufferFormat,
                                              depthFormat, stencilFormat );

        id<MTLTexture> depthTexture = 0;
        id<MTLTexture> stencilTexture = 0;

        if( depthFormat != MTLPixelFormatInvalid )
        {
            desc.pixelFormat = depthFormat;
            depthTexture = [mActiveDevice->mDevice newTextureWithDescriptor: desc];
        }

        if( stencilFormat != MTLPixelFormatInvalid )
        {
            if( stencilFormat != MTLPixelFormatDepth32Float_Stencil8
   #if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
            && stencilFormat != MTLPixelFormatDepth24Unorm_Stencil8
   #endif
            )
            {
                //Separate Stencil & Depth
                desc.pixelFormat = stencilFormat;
                stencilTexture = [mActiveDevice->mDevice newTextureWithDescriptor: desc];
            }
            else
            {
                //Combined Stencil & Depth
                stencilTexture = depthTexture;
            }
        }

        DepthBuffer *retVal = new MetalDepthBuffer( 0, this, renderTarget->getWidth(),
                                                    renderTarget->getHeight(),
                                                    renderTarget->getFSAA(), 0,
                                                    desiredDepthBufferFormat,
                                                    renderTarget->prefersDepthTexture(), false,
                                                    depthTexture, stencilTexture,
                                                    mActiveDevice );

        return retVal;
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_waitForTailFrameToFinish(void)
    {
        if( !mMainSemaphoreAlreadyWaited )
        {
            dispatch_semaphore_wait( mMainGpuSyncSemaphore, DISPATCH_TIME_FOREVER );
            //Semaphore was just grabbed, so ensure we don't grab it twice.
            mMainSemaphoreAlreadyWaited = true;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_beginFrameOnce(void)
    {
        assert( !mBeginFrameOnceStarted &&
                "Calling MetalRenderSystem::_beginFrameOnce more than once "
                "without matching call to _endFrameOnce!!!" );

        //Allow the renderer to preflight 3 frames on the CPU (using a semapore as a guard) and
        //commit them to the GPU. This semaphore will get signaled once the GPU completes a
        //frame's work via addCompletedHandler callback below, signifying the CPU can go ahead
        //and prepare another frame.
        _waitForTailFrameToFinish();

        mBeginFrameOnceStarted = true;

        mActiveRenderTarget = 0;
        mActiveViewport = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_endFrameOnce(void)
    {
        RenderSystem::_endFrameOnce();

        //TODO: We shouldn't tidy up JUST the active device. But all of them.

        cleanAutoParamsBuffers();

        __block dispatch_semaphore_t blockSemaphore = mMainGpuSyncSemaphore;
        [mActiveDevice->mCurrentCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer)
        {
            // GPU has completed rendering the frame and is done using the contents of any buffers
            // previously encoded on the CPU for that frame. Signal the semaphore and allow the CPU
            // to proceed and construct the next frame.
            dispatch_semaphore_signal( blockSemaphore );
        }];

        mActiveDevice->commitAndNextCommandBuffer();

        mActiveRenderTarget = 0;
        mActiveViewport = 0;
        mActiveDevice->mFrameAborted = false;
        mMainSemaphoreAlreadyWaited = false;
        mBeginFrameOnceStarted = false;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::cleanAutoParamsBuffers(void)
    {
        const size_t numUsedBuffers = mAutoParamsBufferIdx;
        size_t usedBytes = 0;
        for( size_t i=0; i<numUsedBuffers; ++i )
        {
            mAutoParamsBuffer[i]->unmap( (i == 0u && numUsedBuffers == 1u) ? UO_KEEP_PERSISTENT :
                                                                             UO_UNMAP_ALL );
            usedBytes += mAutoParamsBuffer[i]->getTotalSizeBytes();
        }

        //Get the max amount of bytes consumed in the last N frames.
        const int numHistoricSamples = sizeof(mHistoricalAutoParamsSize) /
                                       sizeof(mHistoricalAutoParamsSize[0]);
        mHistoricalAutoParamsSize[numHistoricSamples-1] = usedBytes;
        for( int i=0; i<numHistoricSamples - 1; ++i )
        {
            usedBytes = std::max( usedBytes, mHistoricalAutoParamsSize[i + 1] );
            mHistoricalAutoParamsSize[i] = mHistoricalAutoParamsSize[i + 1];
        }

        if( //Merge all buffers into one for the next frame.
            numUsedBuffers > 1u ||
            //Historic data shows we've been using less memory. Shrink this record.
            (!mAutoParamsBuffer.empty() && mAutoParamsBuffer[0]->getTotalSizeBytes() > usedBytes) )
        {
            if( mAutoParamsBuffer[0]->getMappingState() != MS_UNMAPPED )
                mAutoParamsBuffer[0]->unmap( UO_UNMAP_ALL );

            for( size_t i=0; i<mAutoParamsBuffer.size(); ++i )
                mVaoManager->destroyConstBuffer( mAutoParamsBuffer[i] );
            mAutoParamsBuffer.clear();

            if( usedBytes > 0 )
            {
                ConstBufferPacked *constBuffer = mVaoManager->createConstBuffer( usedBytes,
                                                                                 BT_DYNAMIC_PERSISTENT,
                                                                                 0, false );
                mAutoParamsBuffer.push_back( constBuffer );
            }
        }

        mCurrentAutoParamsBufferPtr = 0;
        mCurrentAutoParamsBufferSpaceLeft = 0;
        mAutoParamsBufferIdx = 0;

    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsComputePipelineStateObjectCreated( HlmsComputePso *newPso )
    {
        MetalProgram *computeShader = static_cast<MetalProgram*>(
                    newPso->computeShader->_getBindingDelegate() );

        //Btw. HlmsCompute guarantees mNumThreadGroups won't have zeroes.
        assert( newPso->mNumThreadGroups[0] != 0 &&
                newPso->mNumThreadGroups[1] != 0 &&
                newPso->mNumThreadGroups[2] != 0 &&
                "Invalid parameters. Will also cause div. by zero!" );

        NSError* error = 0;
        MTLComputePipelineDescriptor *psd = [[MTLComputePipelineDescriptor alloc] init];
        psd.computeFunction = computeShader->getMetalFunction();
        psd.threadGroupSizeIsMultipleOfThreadExecutionWidth =
                (newPso->mThreadsPerGroup[0] % newPso->mNumThreadGroups[0] == 0) &&
                (newPso->mThreadsPerGroup[1] % newPso->mNumThreadGroups[1] == 0) &&
                (newPso->mThreadsPerGroup[2] % newPso->mNumThreadGroups[2] == 0);

        id<MTLComputePipelineState> pso =
                [mActiveDevice->mDevice newComputePipelineStateWithDescriptor:psd
                                                                      options:MTLPipelineOptionNone
                                                                   reflection:nil
                                                                        error:&error];
        if( !pso || error )
        {
            String errorDesc;
            if( error )
                errorDesc = [error localizedDescription].UTF8String;

            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failed to create pipeline state for compute, error " +
                         errorDesc, "MetalProgram::_hlmsComputePipelineStateObjectCreated" );
        }

        newPso->rsData = const_cast<void*>( CFBridgingRetain( pso ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso )
    {
        id<MTLComputePipelineState> metalPso = reinterpret_cast< id<MTLComputePipelineState> >(
                    CFBridgingRelease( pso->rsData ) );
        pso->rsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_beginFrame(void)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_endFrame(void)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setViewport(Viewport *vp)
    {
        mActiveViewport = vp;
#if TODO_OGRE_2_2
        if( mActiveViewport != vp )
        {
            mActiveViewport = vp;

            if( vp )
            {
                const bool activeHasColourWrites = mNumMRTs != 0;

                if( vp->getTarget() != mActiveRenderTarget ||
                    vp->getColourWrite() != activeHasColourWrites )
                {
                    _setRenderTarget( vp->getTarget(), vp->getViewportRenderTargetFlags() );
                }

                if( mActiveRenderEncoder || ( !mActiveRenderEncoder &&
                                              (!vp->coversEntireTarget() ||
                                               !vp->scissorsMatchViewport()) ) )
                {
                    if( !mActiveRenderEncoder )
                        createRenderEncoder();

                    if( !vp->coversEntireTarget() )
                    {
                        MTLViewport mtlVp;
                        mtlVp.originX   = vp->getActualLeft();
                        mtlVp.originY   = vp->getActualTop();
                        mtlVp.width     = vp->getActualWidth();
                        mtlVp.height    = vp->getActualHeight();
                        mtlVp.znear     = 0;
                        mtlVp.zfar      = 1;
                        [mActiveRenderEncoder setViewport:mtlVp];
                    }

                    if( !vp->scissorsMatchViewport() )
                    {
                        MTLScissorRect scissorRect;
                        scissorRect.x       = vp->getScissorActualLeft();
                        scissorRect.y       = vp->getScissorActualTop();
                        scissorRect.width   = vp->getScissorActualWidth();
                        scissorRect.height  = vp->getScissorActualHeight();
                        [mActiveRenderEncoder setScissorRect:scissorRect];
                    }
                }
            }
        }

        if( mActiveRenderEncoder && mUavsDirty )
            flushUAVs();
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setActiveDevice( MetalDevice *device )
    {
        if( mActiveDevice != device )
        {
            mActiveDevice           = device;
            mActiveRenderEncoder    = device ? device->mRenderEncoder : 0;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyActiveEncoderEnded( bool callEndRenderPassDesc )
    {
        // MetalDevice::endRenderEncoder gets called either because
        //  * Another encoder was required. Thus we interrupted and callEndRenderPassDesc = true
        //  * endRenderPassDescriptor called us. Thus callEndRenderPassDesc = false
        //  * executeRenderPassDescriptorDelayedActions called us. Thus callEndRenderPassDesc = false
        // In all cases, when callEndRenderPassDesc = true, it also implies rendering was interrupted.
        if( callEndRenderPassDesc )
            endRenderPassDescriptor( true );

        mUavRenderingDirty = true;
        mActiveRenderEncoder = 0;
        mPso = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyActiveComputeEnded(void)
    {
        mComputePso = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyNewCommandBuffer(void)
    {
        MetalVaoManager *vaoManager = static_cast<MetalVaoManager*>( mVaoManager );
        vaoManager->_notifyNewCommandBuffer();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyDeviceStalled(void)
    {
        v1::MetalHardwareBufferManager *hwBufferMgr = static_cast<v1::MetalHardwareBufferManager*>(
                    mHardwareBufferManager );
        MetalVaoManager *vaoManager = static_cast<MetalVaoManager*>( mVaoManager );

        hwBufferMgr->_notifyDeviceStalled();
        vaoManager->_notifyDeviceStalled();
    }
    //-------------------------------------------------------------------------
    id <MTLDepthStencilState> MetalRenderSystem::getDepthStencilState( HlmsPso *pso )
    {
        CachedDepthStencilState depthState;
        if( pso->macroblock->mDepthCheck )
        {
            depthState.depthFunc    = pso->macroblock->mDepthFunc;
            if( mReverseDepth )
                depthState.depthFunc = reverseCompareFunction( depthState.depthFunc );
            depthState.depthWrite   = pso->macroblock->mDepthWrite;
        }
        else
        {
            depthState.depthFunc    = CMPF_ALWAYS_PASS;
            depthState.depthWrite   = false;
        }

        depthState.stencilParams = pso->pass.stencilParams;

        CachedDepthStencilStateVec::iterator itor = std::lower_bound( mDepthStencilStates.begin(),
                                                                      mDepthStencilStates.end(),
                                                                      depthState );

        if( itor == mDepthStencilStates.end() || depthState != *itor )
        {
            //Not cached. Add the entry
            MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
            depthStateDesc.depthCompareFunction = MetalMappings::get( depthState.depthFunc );
            depthStateDesc.depthWriteEnabled    = depthState.depthWrite;

            if( pso->pass.stencilParams.enabled )
            {
                if( pso->pass.stencilParams.stencilFront != StencilStateOp() )
                {
                    const StencilStateOp &stencilOp = pso->pass.stencilParams.stencilFront;

                    MTLStencilDescriptor *stencilDesc = [MTLStencilDescriptor alloc];
                    stencilDesc.stencilCompareFunction = MetalMappings::get( stencilOp.compareOp );
                    stencilDesc.stencilFailureOperation = MetalMappings::get( stencilOp.stencilFailOp );
                    stencilDesc.depthFailureOperation =
                            MetalMappings::get( stencilOp.stencilDepthFailOp );
                    stencilDesc.depthStencilPassOperation =
                            MetalMappings::get( stencilOp.stencilPassOp );

                    stencilDesc.readMask = pso->pass.stencilParams.readMask;
                    stencilDesc.writeMask = pso->pass.stencilParams.writeMask;

                    depthStateDesc.frontFaceStencil = stencilDesc;
                }

                if( pso->pass.stencilParams.stencilBack != StencilStateOp() )
                {
                    const StencilStateOp &stencilOp = pso->pass.stencilParams.stencilBack;

                    MTLStencilDescriptor *stencilDesc = [MTLStencilDescriptor alloc];
                    stencilDesc.stencilCompareFunction = MetalMappings::get( stencilOp.compareOp );
                    stencilDesc.stencilFailureOperation = MetalMappings::get( stencilOp.stencilFailOp );
                    stencilDesc.depthFailureOperation =
                            MetalMappings::get( stencilOp.stencilDepthFailOp );
                    stencilDesc.depthStencilPassOperation =
                            MetalMappings::get( stencilOp.stencilPassOp );

                    stencilDesc.readMask = pso->pass.stencilParams.readMask;
                    stencilDesc.writeMask = pso->pass.stencilParams.writeMask;

                    depthStateDesc.backFaceStencil = stencilDesc;
                }
            }

            depthState.depthStencilState =
                    [mActiveDevice->mDevice newDepthStencilStateWithDescriptor:depthStateDesc];

            itor = mDepthStencilStates.insert( itor, depthState );
        }

        ++itor->refCount;
        return itor->depthStencilState;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::removeDepthStencilState( HlmsPso *pso )
    {
        CachedDepthStencilState depthState;
        if( pso->macroblock->mDepthCheck )
        {
            depthState.depthFunc    = pso->macroblock->mDepthFunc;
            if( mReverseDepth )
                depthState.depthFunc = reverseCompareFunction( depthState.depthFunc );
            depthState.depthWrite   = pso->macroblock->mDepthWrite;
        }
        else
        {
            depthState.depthFunc    = CMPF_ALWAYS_PASS;
            depthState.depthWrite   = false;
        }

        depthState.stencilParams = pso->pass.stencilParams;

        CachedDepthStencilStateVec::iterator itor = std::lower_bound( mDepthStencilStates.begin(),
                                                                      mDepthStencilStates.end(),
                                                                      depthState );

        if( itor == mDepthStencilStates.end() || !(depthState != *itor) )
        {
            --itor->refCount;
            if( !itor->refCount )
            {
                mDepthStencilStates.erase( itor );
            }
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsPipelineStateObjectCreated( HlmsPso *newPso )
    {
        MTLRenderPipelineDescriptor *psd = [[MTLRenderPipelineDescriptor alloc] init];
        [psd setSampleCount: newPso->pass.multisampleCount];

        MetalProgram *vertexShader = 0;
        MetalProgram *pixelShader = 0;

        if( !newPso->vertexShader.isNull() )
        {
            vertexShader = static_cast<MetalProgram*>( newPso->vertexShader->_getBindingDelegate() );
            [psd setVertexFunction:vertexShader->getMetalFunction()];
        }

        if( !newPso->pixelShader.isNull() )
        {
            pixelShader = static_cast<MetalProgram*>( newPso->pixelShader->_getBindingDelegate() );
            [psd setFragmentFunction:pixelShader->getMetalFunction()];
        }
        //Ducttape shaders
//        id<MTLLibrary> library = [mActiveDevice->mDevice newDefaultLibrary];
//        [psd setVertexFunction:[library newFunctionWithName:@"vertex_vs"]];
//        [psd setFragmentFunction:[library newFunctionWithName:@"pixel_ps"]];

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        // On iOS we can skip the Vertex Desc.
        // On OS X we always need to set it for the baseInstance / draw id
        if( !newPso->vertexElements.empty() )
#endif
        {
            size_t numUVs = 0;
            MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];

            VertexElement2VecVec::const_iterator itor = newPso->vertexElements.begin();
            VertexElement2VecVec::const_iterator end  = newPso->vertexElements.end();

            while( itor != end )
            {
                size_t accumOffset = 0;
                const size_t bufferIdx = itor - newPso->vertexElements.begin();
                VertexElement2Vec::const_iterator it = itor->begin();
                VertexElement2Vec::const_iterator en = itor->end();

                while( it != en )
                {
                    size_t elementIdx = MetalVaoManager::getAttributeIndexFor( it->mSemantic );
                    if( it->mSemantic == VES_TEXTURE_COORDINATES )
                    {
                        elementIdx += numUVs;
                        ++numUVs;
                    }
                    vertexDescriptor.attributes[elementIdx].format = MetalMappings::get( it->mType );
                    vertexDescriptor.attributes[elementIdx].bufferIndex = bufferIdx;
                    vertexDescriptor.attributes[elementIdx].offset = accumOffset;

                    accumOffset += v1::VertexElement::getTypeSize( it->mType );
                    ++it;
                }

                vertexDescriptor.layouts[bufferIdx].stride = accumOffset;
                vertexDescriptor.layouts[bufferIdx].stepFunction = MTLVertexStepFunctionPerVertex;

                ++itor;
            }

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
            vertexDescriptor.attributes[15].format      = MTLVertexFormatUInt;
            vertexDescriptor.attributes[15].bufferIndex = 15;
            vertexDescriptor.attributes[15].offset      = 0;

            vertexDescriptor.layouts[15].stride = 4;
            vertexDescriptor.layouts[15].stepFunction = MTLVertexStepFunctionPerInstance;
#endif
            [psd setVertexDescriptor:vertexDescriptor];
        }

        uint8 mrtCount = 0;
        for( int i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( newPso->pass.colourFormat[i] != PFG_NULL )
                mrtCount = i + 1u;
        }

        for( int i=0; i<mrtCount; ++i )
        {
            HlmsBlendblock const *blendblock = newPso->blendblock;
            psd.colorAttachments[i].pixelFormat = MetalMappings::get( newPso->pass.colourFormat[i] );

            if( psd.colorAttachments[i].pixelFormat == MTLPixelFormatInvalid ||
                (blendblock->mBlendOperation == SBO_ADD &&
                 blendblock->mSourceBlendFactor == SBF_ONE &&
                 blendblock->mDestBlendFactor == SBF_ZERO &&
                (!blendblock->mSeparateBlend ||
                 (blendblock->mBlendOperation == blendblock->mBlendOperationAlpha &&
                 blendblock->mSourceBlendFactor == blendblock->mSourceBlendFactorAlpha &&
                 blendblock->mDestBlendFactor == blendblock->mDestBlendFactorAlpha))) )
            {
                //Note: Can NOT use blendblock->mIsTransparent. What Ogre understand
                //as transparent differs from what Metal understands.
                psd.colorAttachments[i].blendingEnabled = NO;
            }
            else
            {
                psd.colorAttachments[i].blendingEnabled = YES;
            }
            psd.colorAttachments[i].rgbBlendOperation           = MetalMappings::get( blendblock->mBlendOperation );
            psd.colorAttachments[i].alphaBlendOperation         = MetalMappings::get( blendblock->mBlendOperationAlpha );
            psd.colorAttachments[i].sourceRGBBlendFactor        = MetalMappings::get( blendblock->mSourceBlendFactor );
            psd.colorAttachments[i].destinationRGBBlendFactor   = MetalMappings::get( blendblock->mDestBlendFactor );
            psd.colorAttachments[i].sourceAlphaBlendFactor      = MetalMappings::get( blendblock->mSourceBlendFactorAlpha );
            psd.colorAttachments[i].destinationAlphaBlendFactor = MetalMappings::get( blendblock->mDestBlendFactorAlpha );

            psd.colorAttachments[i].writeMask = MetalMappings::get( blendblock->mBlendChannelMask );
        }

        if( newPso->pass.depthFormat != PFG_NULL )
        {
            MTLPixelFormat depthFormat = MTLPixelFormatInvalid;
            MTLPixelFormat stencilFormat = MTLPixelFormatInvalid;
            MetalMappings::getDepthStencilFormat( mActiveDevice, newPso->pass.depthFormat,
                                                  depthFormat, stencilFormat );
            psd.depthAttachmentPixelFormat = depthFormat;
            psd.stencilAttachmentPixelFormat = stencilFormat;
        }

        NSError* error = NULL;
        id <MTLRenderPipelineState> pso =
                [mActiveDevice->mDevice newRenderPipelineStateWithDescriptor:psd error:&error];

        if( !pso || error )
        {
            String errorDesc;
            if( error )
                errorDesc = [error localizedDescription].UTF8String;

            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failed to created pipeline state for rendering, error " + errorDesc,
                         "MetalRenderSystem::_hlmsPipelineStateObjectCreated" );
        }

        id <MTLDepthStencilState> depthStencilState = getDepthStencilState( newPso );

        MetalHlmsPso *metalPso = new MetalHlmsPso();
        metalPso->pso = pso;
        metalPso->depthStencilState = depthStencilState;
        metalPso->vertexShader      = vertexShader;
        metalPso->pixelShader       = pixelShader;

        switch( newPso->macroblock->mCullMode )
        {
        case CULL_NONE:             metalPso->cullMode = MTLCullModeNone; break;
        case CULL_CLOCKWISE:        metalPso->cullMode = MTLCullModeBack; break;
        case CULL_ANTICLOCKWISE:    metalPso->cullMode = MTLCullModeFront; break;
        }

        newPso->rsData = metalPso;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        assert( pso->rsData );

        removeDepthStencilState( pso );

        MetalHlmsPso *metalPso = reinterpret_cast<MetalHlmsPso*>(pso->rsData);
        delete metalPso;
        pso->rsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsSamplerblockCreated( HlmsSamplerblock *newBlock )
    {
        MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];
        samplerDescriptor.minFilter = MetalMappings::get( newBlock->mMinFilter );
        samplerDescriptor.magFilter = MetalMappings::get( newBlock->mMagFilter );
        samplerDescriptor.mipFilter = MetalMappings::getMipFilter( newBlock->mMipFilter );
        samplerDescriptor.maxAnisotropy = newBlock->mMaxAnisotropy;
        samplerDescriptor.sAddressMode  = MetalMappings::get( newBlock->mU );
        samplerDescriptor.tAddressMode  = MetalMappings::get( newBlock->mV );
        samplerDescriptor.rAddressMode  = MetalMappings::get( newBlock->mW );
        samplerDescriptor.normalizedCoordinates = YES;
        samplerDescriptor.lodMinClamp   = newBlock->mMinLod;
        samplerDescriptor.lodMaxClamp   = newBlock->mMaxLod;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        const bool supportsCompareFunction =
                [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1];
#else
        const bool supportsCompareFunction = true;
#endif

        if( supportsCompareFunction )
        {
            if( newBlock->mCompareFunction != NUM_COMPARE_FUNCTIONS )
                samplerDescriptor.compareFunction = MetalMappings::get( newBlock->mCompareFunction );
        }

        id <MTLSamplerState> sampler =
                [mActiveDevice->mDevice newSamplerStateWithDescriptor:samplerDescriptor];

        newBlock->mRsData = const_cast<void*>( CFBridgingRetain( sampler ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        id <MTLSamplerState> sampler = reinterpret_cast< id <MTLSamplerState> >(
                    CFBridgingRelease( block->mRsData ) );
    }
    //-------------------------------------------------------------------------
    template <typename TDescriptorSetTexture,
              typename TTexSlot,
              typename TBufferPacked>
    void MetalRenderSystem::_descriptorSetTextureCreated( TDescriptorSetTexture *newSet,
                                                          const FastArray<TTexSlot> &texContainer,
                                                          uint16 *shaderTypeTexCount )
    {
        MetalDescriptorSetTexture *metalSet = new MetalDescriptorSetTexture();

        metalSet->numTextureViews = 0;

        size_t numBuffers = 0;
        size_t numTextures = 0;

        size_t numBufferRanges = 0;
        size_t numTextureRanges = 0;

        bool needsNewTexRange = true;
        bool needsNewBufferRange = true;

        ShaderType shaderType = VertexShader;
        size_t numProcessedSlots = 0;

        typename FastArray<TTexSlot>::const_iterator itor = texContainer.begin();
        typename FastArray<TTexSlot>::const_iterator end  = texContainer.end();

        while( itor != end )
        {
            if( shaderTypeTexCount )
            {
                //We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader &&
                       numProcessedSlots >= shaderTypeTexCount[shaderType] )
                {
                    numProcessedSlots = 0;
                    shaderType = static_cast<ShaderType>( shaderType + 1u );
                    needsNewTexRange = true;
                    needsNewBufferRange = true;
                }
            }

            if( itor->isTexture() )
            {
                needsNewBufferRange = true;
                if( needsNewTexRange )
                {
                    ++numTextureRanges;
                    needsNewTexRange = false;
                }

                const typename TDescriptorSetTexture::TextureSlot &texSlot = itor->getTexture();
                if( texSlot.needsDifferentView() )
                    ++metalSet->numTextureViews;
                ++numTextures;
            }
            else
            {
                needsNewTexRange = true;
                if( needsNewBufferRange )
                {
                    ++numBufferRanges;
                    needsNewBufferRange = false;
                }

                ++numBuffers;
            }
            ++numProcessedSlots;
            ++itor;
        }

        //Create two contiguous arrays of texture and buffers, but we'll split
        //it into regions as a buffer could be in the middle of two textures.
        __unsafe_unretained id <MTLTexture> *textures = 0;
        __unsafe_unretained id <MTLBuffer> *buffers = 0;
        NSUInteger *offsets = 0;

        if( metalSet->numTextureViews > 0 )
        {
            //Create a third array to hold the strong reference
            //to the reinterpreted versions of textures.
            //Must be init to 0 before ARC sees it.
            void *textureViews = OGRE_MALLOC_SIMD( sizeof(id<MTLTexture>*) * metalSet->numTextureViews,
                                                   MEMCATEGORY_RENDERSYS );
            memset( textureViews, 0, sizeof(id<MTLTexture>*) * metalSet->numTextureViews );
            metalSet->textureViews = (__strong id <MTLTexture>*)textureViews;
        }
        if( numTextures > 0 )
        {
            textures = (__unsafe_unretained id <MTLTexture>*)
                       OGRE_MALLOC_SIMD( sizeof(id<MTLTexture>*) * numTextures, MEMCATEGORY_RENDERSYS );
        }
        if( numBuffers > 0 )
        {
            buffers = (__unsafe_unretained id <MTLBuffer>*)
                       OGRE_MALLOC_SIMD( sizeof(id<MTLBuffer>*) * numBuffers, MEMCATEGORY_RENDERSYS );
            offsets = (NSUInteger*)OGRE_MALLOC_SIMD( sizeof(NSUInteger) * numBuffers,
                                                     MEMCATEGORY_RENDERSYS );
        }

        metalSet->textures.reserve( numTextureRanges );
        metalSet->buffers.reserve( numBufferRanges );

        needsNewTexRange = true;
        needsNewBufferRange = true;

        shaderType = VertexShader;
        numProcessedSlots = 0;

        size_t texViewIndex = 0;

        itor = texContainer.begin();
        end  = texContainer.end();

        while( itor != end )
        {
            if( shaderTypeTexCount )
            {
                //We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader &&
                       numProcessedSlots >= shaderTypeTexCount[shaderType] )
                {
                    numProcessedSlots = 0;
                    shaderType = static_cast<ShaderType>( shaderType + 1u );
                    needsNewTexRange = true;
                    needsNewBufferRange = true;
                }
            }

            if( itor->isTexture() )
            {
                needsNewBufferRange = true;
                if( needsNewTexRange )
                {
                    metalSet->textures.push_back( MetalTexRegion() );
                    MetalTexRegion &texRegion = metalSet->textures.back();
                    texRegion.textures = textures;
                    texRegion.shaderType = shaderType;
                    texRegion.range.location    = itor - texContainer.begin();
                    texRegion.range.length      = 0;
                    needsNewTexRange = false;
                }

                const typename TDescriptorSetTexture::TextureSlot &texSlot = itor->getTexture();

                assert( dynamic_cast<MetalTextureGpu*>( texSlot.texture ) );
                MetalTextureGpu *metalTex = static_cast<MetalTextureGpu*>( texSlot.texture );
                __unsafe_unretained id<MTLTexture> textureHandle = metalTex->getDisplayTextureName();

                if( texSlot.needsDifferentView() )
                {
                    metalSet->textureViews[texViewIndex] = metalTex->getView( texSlot );
                    textureHandle = metalSet->textureViews[texViewIndex];
                    ++texViewIndex;
                }

                MetalTexRegion &texRegion = metalSet->textures.back();
                *textures = textureHandle;
                ++texRegion.range.length;

                ++textures;
            }
            else
            {
                needsNewTexRange = true;
                if( needsNewBufferRange )
                {
                    metalSet->buffers.push_back( MetalBufferRegion() );
                    MetalBufferRegion &bufferRegion = metalSet->buffers.back();
                    bufferRegion.buffers = buffers;
                    bufferRegion.offsets = offsets;
                    bufferRegion.shaderType = shaderType;
                    bufferRegion.range.location = itor - texContainer.begin();
                    bufferRegion.range.length   = 0;
                    needsNewBufferRange = false;
                }

                const typename TDescriptorSetTexture::BufferSlot &bufferSlot = itor->getBuffer();

                assert( dynamic_cast<TBufferPacked*>( bufferSlot.buffer ) );
                TBufferPacked *metalBuf = static_cast<TBufferPacked*>( bufferSlot.buffer );

                MetalBufferRegion &bufferRegion = metalSet->buffers.back();
                metalBuf->bindBufferForDescriptor( buffers, offsets, bufferSlot.offset );
                ++bufferRegion.range.length;

                ++buffers;
                ++offsets;
            }
            ++numProcessedSlots;
            ++itor;
        }

        newSet->mRsData = metalSet;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::destroyMetalDescriptorSetTexture( MetalDescriptorSetTexture *metalSet )
    {
        const size_t numTextureViews = metalSet->numTextureViews;
        for( size_t i=0; i<numTextureViews; ++i )
            metalSet->textureViews[i] = 0; //Let ARC free these pointers

        if( numTextureViews )
        {
            OGRE_FREE_SIMD( metalSet->textureViews, MEMCATEGORY_RENDERSYS );
            metalSet->textureViews = 0;
        }

        if( !metalSet->textures.empty() )
        {
            MetalTexRegion &texRegion = metalSet->textures.front();
            OGRE_FREE_SIMD( texRegion.textures, MEMCATEGORY_RENDERSYS );
        }

        if( !metalSet->buffers.empty() )
        {
            MetalBufferRegion &bufferRegion = metalSet->buffers.front();
            OGRE_FREE_SIMD( bufferRegion.buffers, MEMCATEGORY_RENDERSYS );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetTexture2Created( DescriptorSetTexture2 *newSet )
    {
        _descriptorSetTextureCreated<
                DescriptorSetTexture2,
                DescriptorSetTexture2::Slot,
                MetalTexBufferPacked>( newSet, newSet->mTextures, newSet->mShaderTypeTexCount );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        assert( set->mRsData );

        MetalDescriptorSetTexture *metalSet =
                reinterpret_cast<MetalDescriptorSetTexture*>( set->mRsData );

        destroyMetalDescriptorSetTexture( metalSet );
        delete metalSet;

        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetUavCreated( DescriptorSetUav *newSet )
    {
        _descriptorSetTextureCreated<
                DescriptorSetUav,
                DescriptorSetUav::Slot,
                MetalUavBufferPacked>( newSet, newSet->mUavs, 0 );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetUavDestroyed( DescriptorSetUav *set )
    {
        assert( set->mRsData );

        MetalDescriptorSetTexture *metalSet =
                reinterpret_cast<MetalDescriptorSetTexture*>( set->mRsData );

        destroyMetalDescriptorSetTexture( metalSet );
        delete metalSet;

        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        assert( (!samplerblock || samplerblock->mRsData) &&
                "The block must have been created via HlmsManager::getSamplerblock!" );

        if( !samplerblock )
        {
            [mActiveRenderEncoder setFragmentSamplerState:0 atIndex: texUnit];
        }
        else
        {
            __unsafe_unretained id <MTLSamplerState> sampler =
                    (__bridge id<MTLSamplerState>)samplerblock->mRsData;
            [mActiveRenderEncoder setVertexSamplerState:sampler atIndex: texUnit];
            [mActiveRenderEncoder setFragmentSamplerState:sampler atIndex: texUnit];
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        MetalHlmsPso *metalPso = reinterpret_cast<MetalHlmsPso*>(pso->rsData);

        if( pso && !mActiveRenderEncoder )
        {
            assert( mInterruptedRenderCommandEncoder &&
                    "mActiveRenderEncoder can only be null at this stage if rendering was interrupted."
                    " Did you call executeRenderPassDescriptorDelayedActions?");
            executeRenderPassDescriptorDelayedActions( false );
        }

        if( !mPso || mPso->depthStencilState != metalPso->depthStencilState )
            [mActiveRenderEncoder setDepthStencilState:metalPso->depthStencilState];

        [mActiveRenderEncoder setDepthBias:pso->macroblock->mDepthBiasConstant
                                     slopeScale:pso->macroblock->mDepthBiasSlopeScale
                                     clamp:0.0f];
        [mActiveRenderEncoder setCullMode:metalPso->cullMode];

        if( mPso != metalPso )
        {
            [mActiveRenderEncoder setRenderPipelineState:metalPso->pso];
            mPso = metalPso;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        __unsafe_unretained id<MTLComputePipelineState> metalPso =
                (__bridge id<MTLComputePipelineState>)pso->rsData;

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        if( mComputePso != pso )
        {
            [computeEncoder setComputePipelineState:metalPso];
            mComputePso = pso;
        }
    }
    //-------------------------------------------------------------------------
    VertexElementType MetalRenderSystem::getColourVertexElementType(void) const
    {
        return VET_COLOUR_ARGB;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
                mActiveDevice->getComputeEncoder();

        MTLSize numThreadGroups = MTLSizeMake( pso.mNumThreadGroups[0],
                                               pso.mNumThreadGroups[1],
                                               pso.mNumThreadGroups[2] );
        MTLSize threadsPerGroup = MTLSizeMake( pso.mThreadsPerGroup[0],
                                               pso.mThreadsPerGroup[1],
                                               pso.mThreadsPerGroup[2] );

        [computeEncoder dispatchThreadgroups:numThreadGroups
                       threadsPerThreadgroup:threadsPerGroup];
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setVertexArrayObject( const VertexArrayObject *vao )
    {
        __unsafe_unretained id<MTLBuffer> metalVertexBuffers[15];
        NSUInteger offsets[15];
        memset( offsets, 0, sizeof(offsets) );

        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

        size_t numVertexBuffers = 0;
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator end  = vertexBuffers.end();

        while( itor != end )
        {
            MetalBufferInterface *bufferInterface = static_cast<MetalBufferInterface*>(
                        (*itor)->getBufferInterface() );
            metalVertexBuffers[numVertexBuffers++] = bufferInterface->getVboName();
            ++itor;
        }

#if OGRE_DEBUG_MODE
        assert( numVertexBuffers < 15u );
#endif

        [mActiveRenderEncoder setVertexBuffers:metalVertexBuffers offsets:offsets
                                               withRange:NSMakeRange( 0, numVertexBuffers )];
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const CbDrawCallStrip *cmd )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        const VertexArrayObject *vao = cmd->vao;
        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const MTLIndexType indexType = static_cast<MTLIndexType>( vao->mIndexBuffer->getIndexType() );
        const MTLPrimitiveType primType =  std::min(
                    MTLPrimitiveTypeTriangleStrip,
                    static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

        //Calculate bytesPerVertexBuffer & numVertexBuffers which is the same for all draws in this cmd
        uint32 bytesPerVertexBuffer[15];
        size_t numVertexBuffers = 0;
        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator end  = vertexBuffers.end();

        while( itor != end )
        {
            bytesPerVertexBuffer[numVertexBuffers] = (*itor)->getBytesPerElement();
            ++numVertexBuffers;
            ++itor;
        }

        //Get index buffer stuff which is the same for all draws in this cmd
        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();

        MetalBufferInterface *indexBufferInterface = static_cast<MetalBufferInterface*>(
                    vao->mIndexBuffer->getBufferInterface() );
        __unsafe_unretained id<MTLBuffer> indexBuffer = indexBufferInterface->getVboName();

        for( uint32 i=cmd->numDraws; i--; )
        {
#if OGRE_DEBUG_MODE
            assert( ((drawCmd->firstVertexIndex * bytesPerIndexElement) & 0x03) == 0
                    && "Index Buffer must be aligned to 4 bytes. If you're messing with "
                    "VertexArrayObject::setPrimitiveRange, you've entered an invalid "
                    "primStart; not supported by the Metal API." );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            for( size_t j=0; j<numVertexBuffers; ++j )
            {
                //Manually set vertex buffer offsets since in iOS baseVertex is not supported
                //until iOS GPU Family 3 v1 & OS X (we just use indirect rendering there).
                [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseVertex * bytesPerVertexBuffer[j]
                                                            atIndex:j];
            }

            //Setup baseInstance.
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * sizeof(uint32)
                                                atIndex:15];

            [mActiveRenderEncoder drawIndexedPrimitives:primType
                       indexCount:drawCmd->primCount
                        indexType:indexType
                      indexBuffer:indexBuffer
                indexBufferOffset:drawCmd->firstVertexIndex * bytesPerIndexElement
                    instanceCount:drawCmd->instanceCount];
#else
            [mActiveRenderEncoder drawIndexedPrimitives:primType
                       indexCount:drawCmd->primCount
                        indexType:indexType
                      indexBuffer:indexBuffer
                indexBufferOffset:drawCmd->firstVertexIndex * bytesPerIndexElement
                    instanceCount:drawCmd->instanceCount
                       baseVertex:drawCmd->baseVertex
                     baseInstance:drawCmd->baseInstance];
#endif
            ++drawCmd;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_renderEmulated( const CbDrawCallStrip *cmd )
    {
        const VertexArrayObject *vao = cmd->vao;
        CbDrawStrip *drawCmd = reinterpret_cast<CbDrawStrip*>(
                                    mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const MTLPrimitiveType primType =  std::min(
                    MTLPrimitiveTypeTriangleStrip,
                    static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

//        const size_t numVertexBuffers = vertexBuffers.size();
//        for( size_t j=0; j<numVertexBuffers; ++j )
//        {
//            //baseVertex is not needed as vertexStart does the same job.
//            [mActiveRenderEncoder setVertexBufferOffset:0 atIndex:j];
//        }

        for( uint32 i=cmd->numDraws; i--; )
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            //Setup baseInstance.
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * sizeof(uint32)
                                                atIndex:15];
            [mActiveRenderEncoder drawPrimitives:primType
                      vertexStart:drawCmd->firstVertexIndex
                      vertexCount:drawCmd->primCount
                    instanceCount:drawCmd->instanceCount];
#else
            [mActiveRenderEncoder drawPrimitives:primType
                      vertexStart:drawCmd->firstVertexIndex
                      vertexCount:drawCmd->primCount
                    instanceCount:drawCmd->instanceCount
                     baseInstance:drawCmd->baseInstance];
#endif
            ++drawCmd;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setRenderOperation( const v1::CbRenderOp *cmd )
    {
        __unsafe_unretained id<MTLBuffer> metalVertexBuffers[15];
        NSUInteger offsets[15];
        memset( offsets, 0, sizeof(offsets) );

        size_t maxUsedSlot = 0;
        const v1::VertexBufferBinding::VertexBufferBindingMap& binds =
                cmd->vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator itor = binds.begin();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator end  = binds.end();

        while( itor != end )
        {
            v1::MetalHardwareVertexBuffer *metalBuffer =
                static_cast<v1::MetalHardwareVertexBuffer*>( itor->second.get() );

            const size_t slot = itor->first;
#if OGRE_DEBUG_MODE
            assert( slot < 15u );
#endif
            size_t offsetStart;
            metalVertexBuffers[slot] = metalBuffer->getBufferName( offsetStart );
            offsets[slot] = offsetStart;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            offsets[slot] += cmd->vertexData->vertexStart * metalBuffer->getVertexSize();
#endif
            ++itor;
            maxUsedSlot = std::max( maxUsedSlot, slot + 1u );
        }

        [mActiveRenderEncoder setVertexBuffers:metalVertexBuffers offsets:offsets
                                               withRange:NSMakeRange( 0, maxUsedSlot )];

        mCurrentIndexBuffer = cmd->indexData;
        mCurrentVertexBuffer= cmd->vertexData;
        mCurrentPrimType    = std::min(  MTLPrimitiveTypeTriangleStrip,
                                         static_cast<MTLPrimitiveType>( cmd->operationType - 1u ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        const MTLIndexType indexType = static_cast<MTLIndexType>(
                    mCurrentIndexBuffer->indexBuffer->getType() );

        //Get index buffer stuff which is the same for all draws in this cmd
        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

        size_t offsetStart;
        v1::MetalHardwareIndexBuffer *metalBuffer =
            static_cast<v1::MetalHardwareIndexBuffer*>( mCurrentIndexBuffer->indexBuffer.get() );
        __unsafe_unretained id<MTLBuffer> indexBuffer = metalBuffer->getBufferName( offsetStart );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #if OGRE_DEBUG_MODE
            assert( ((cmd->firstVertexIndex * bytesPerIndexElement) & 0x03) == 0
                    && "Index Buffer must be aligned to 4 bytes. If you're messing with "
                    "IndexBuffer::indexStart, you've entered an invalid "
                    "indexStart; not supported by the Metal API." );
    #endif

        //Setup baseInstance.
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * sizeof(uint32)
                                            atIndex:15];

        [mActiveRenderEncoder drawIndexedPrimitives:mCurrentPrimType
                   indexCount:cmd->primCount
                    indexType:indexType
                  indexBuffer:indexBuffer
            indexBufferOffset:cmd->firstVertexIndex * bytesPerIndexElement + offsetStart
            instanceCount:cmd->instanceCount];
#else
        [mActiveRenderEncoder drawIndexedPrimitives:mCurrentPrimType
                   indexCount:cmd->primCount
                    indexType:indexType
                  indexBuffer:indexBuffer
            indexBufferOffset:cmd->firstVertexIndex * bytesPerIndexElement + offsetStart
                instanceCount:cmd->instanceCount
                   baseVertex:mCurrentVertexBuffer->vertexStart
                 baseInstance:cmd->baseInstance];
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        //Setup baseInstance.
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * sizeof(uint32)
                                            atIndex:15];
        [mActiveRenderEncoder drawPrimitives:mCurrentPrimType
                  vertexStart:0 /*cmd->firstVertexIndex already handled in _setRenderOperation*/
                  vertexCount:cmd->primCount
                instanceCount:cmd->instanceCount];
#else
        [mActiveRenderEncoder drawPrimitives:mCurrentPrimType
                  vertexStart:cmd->firstVertexIndex
                  vertexCount:cmd->primCount
                instanceCount:cmd->instanceCount
                 baseInstance:cmd->baseInstance];
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const v1::RenderOperation &op )
    {
        // Call super class.
        RenderSystem::_render(op);

        const size_t numberOfInstances = op.numberOfInstances;
        const bool hasInstanceData = mCurrentVertexBuffer->vertexBufferBinding->getHasInstanceData();

        // Render to screen!
        if( op.useIndexes )
        {
            do
            {
                // Update derived depth bias.
                if (mDerivedDepthBias && mCurrentPassIterationNum > 0)
                {
                    [mActiveRenderEncoder setDepthBias:mDerivedDepthBiasBase +
                                                       mDerivedDepthBiasMultiplier *
                                                       mCurrentPassIterationNum
                                            slopeScale:mDerivedDepthBiasSlopeScale
                                                 clamp:0.0f];
                }

                const MTLIndexType indexType = static_cast<MTLIndexType>(
                            mCurrentIndexBuffer->indexBuffer->getType() );

                //Get index buffer stuff which is the same for all draws in this cmd
                const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

                size_t offsetStart;
                v1::MetalHardwareIndexBuffer *metalBuffer =
                    static_cast<v1::MetalHardwareIndexBuffer*>( mCurrentIndexBuffer->indexBuffer.get() );
                __unsafe_unretained id<MTLBuffer> indexBuffer = metalBuffer->getBufferName( offsetStart );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #if OGRE_DEBUG_MODE
                    assert( ((mCurrentIndexBuffer->indexStart * bytesPerIndexElement) & 0x03) == 0
                            && "Index Buffer must be aligned to 4 bytes. If you're messing with "
                            "IndexBuffer::indexStart, you've entered an invalid "
                            "indexStart; not supported by the Metal API." );
    #endif

                [mActiveRenderEncoder drawIndexedPrimitives:mCurrentPrimType
                           indexCount:mCurrentIndexBuffer->indexCount
                            indexType:indexType
                          indexBuffer:indexBuffer
                    indexBufferOffset:mCurrentIndexBuffer->indexStart * bytesPerIndexElement + offsetStart
                        instanceCount:numberOfInstances];
#else
                [mActiveRenderEncoder drawIndexedPrimitives:mCurrentPrimType
                           indexCount:mCurrentIndexBuffer->indexCount
                            indexType:indexType
                          indexBuffer:indexBuffer
                    indexBufferOffset:mCurrentIndexBuffer->indexStart * bytesPerIndexElement + offsetStart
                        instanceCount:numberOfInstances
                           baseVertex:mCurrentVertexBuffer->vertexStart
                         baseInstance:0];
#endif
            } while (updatePassIterationRenderState());
        }
        else
        {
            do
            {
                // Update derived depth bias.
                if (mDerivedDepthBias && mCurrentPassIterationNum > 0)
                {
                    [mActiveRenderEncoder setDepthBias:mDerivedDepthBiasBase +
                                                       mDerivedDepthBiasMultiplier *
                                                       mCurrentPassIterationNum
                                            slopeScale:mDerivedDepthBiasSlopeScale
                                                 clamp:0.0f];
                }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
                const uint32 vertexStart = 0;
#else
                const uint32 vertexStart = static_cast<uint32>( mCurrentVertexBuffer->vertexStart );
#endif

                if (hasInstanceData)
                {
                    [mActiveRenderEncoder drawPrimitives:mCurrentPrimType
                                             vertexStart:vertexStart
                                             vertexCount:mCurrentVertexBuffer->vertexCount
                                           instanceCount:numberOfInstances];
                }
                else
                {
                    [mActiveRenderEncoder drawPrimitives:mCurrentPrimType
                                             vertexStart:vertexStart
                                             vertexCount:mCurrentVertexBuffer->vertexCount];
                }
            } while (updatePassIterationRenderState());
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                      GpuProgramParametersSharedPtr params,
                                                      uint16 variabilityMask )
    {
        MetalProgram *shader = 0;
        switch (gptype)
        {
        case GPT_VERTEX_PROGRAM:
            mActiveVertexGpuProgramParameters = params;
            shader = mPso->vertexShader;
            break;
        case GPT_FRAGMENT_PROGRAM:
            mActiveFragmentGpuProgramParameters = params;
            shader = mPso->pixelShader;
            break;
        case GPT_GEOMETRY_PROGRAM:
            mActiveGeometryGpuProgramParameters = params;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "Geometry Shaders are not supported in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_HULL_PROGRAM:
            mActiveTessellationHullGpuProgramParameters = params;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "Tesselation is different in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_DOMAIN_PROGRAM:
            mActiveTessellationDomainGpuProgramParameters = params;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "Tesselation is different in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_COMPUTE_PROGRAM:
            mActiveComputeGpuProgramParameters = params;
            shader = static_cast<MetalProgram*>( mComputePso->computeShader->_getBindingDelegate() );
            break;
        default:
            break;
        }

        size_t bytesToWrite = shader->getBufferRequiredSize();
        if( shader && bytesToWrite > 0 )
        {
            if( mCurrentAutoParamsBufferSpaceLeft < bytesToWrite )
            {
                if( mAutoParamsBufferIdx >= mAutoParamsBuffer.size() )
                {
                    ConstBufferPacked *constBuffer =  mVaoManager->createConstBuffer(
                                std::max<size_t>( 512u * 1024u, bytesToWrite ),
                                BT_DYNAMIC_PERSISTENT, 0, false );
                    mAutoParamsBuffer.push_back( constBuffer );
                }

                ConstBufferPacked *constBuffer = mAutoParamsBuffer[mAutoParamsBufferIdx];
                mCurrentAutoParamsBufferPtr = reinterpret_cast<uint8*>(
                            constBuffer->map( 0, constBuffer->getNumElements() ) );
                mCurrentAutoParamsBufferSpaceLeft = constBuffer->getTotalSizeBytes();

                ++mAutoParamsBufferIdx;
            }

            shader->updateBuffers( params, mCurrentAutoParamsBufferPtr );

            assert( dynamic_cast<MetalConstBufferPacked*>(
                    mAutoParamsBuffer[mAutoParamsBufferIdx-1u] ) );

            MetalConstBufferPacked *constBuffer = static_cast<MetalConstBufferPacked*>(
                        mAutoParamsBuffer[mAutoParamsBufferIdx-1u] );
            const size_t bindOffset = constBuffer->getTotalSizeBytes() -
                                        mCurrentAutoParamsBufferSpaceLeft;
            switch (gptype)
            {
            case GPT_VERTEX_PROGRAM:
                constBuffer->bindBufferVS( OGRE_METAL_PARAMETER_SLOT - OGRE_METAL_CONST_SLOT_START,
                                           bindOffset );
                break;
            case GPT_FRAGMENT_PROGRAM:
                constBuffer->bindBufferPS( OGRE_METAL_PARAMETER_SLOT - OGRE_METAL_CONST_SLOT_START,
                                           bindOffset );
                break;
            case GPT_COMPUTE_PROGRAM:
                constBuffer->bindBufferCS( OGRE_METAL_CS_PARAMETER_SLOT - OGRE_METAL_CS_CONST_SLOT_START,
                                           bindOffset );
                break;
            case GPT_GEOMETRY_PROGRAM:
            case GPT_HULL_PROGRAM:
            case GPT_DOMAIN_PROGRAM:
                break;
            }

            mCurrentAutoParamsBufferPtr += bytesToWrite;

            const uint8 *oldBufferPos = mCurrentAutoParamsBufferPtr;
            mCurrentAutoParamsBufferPtr = reinterpret_cast<uint8*>(
                    alignToNextMultiple( reinterpret_cast<uintptr_t>( mCurrentAutoParamsBufferPtr ),
                                         mVaoManager->getConstBufferAlignment() ) );
            bytesToWrite += mCurrentAutoParamsBufferPtr - oldBufferPos;

            //We know that bytesToWrite <= mCurrentAutoParamsBufferSpaceLeft, but that was
            //before padding. After padding this may no longer hold true.
            mCurrentAutoParamsBufferSpaceLeft -= std::min( mCurrentAutoParamsBufferSpaceLeft,
                                                           bytesToWrite );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::bindGpuProgramPassIterationParameters(GpuProgramType gptype)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                              TextureGpu *anyTarget, uint8 mipLevel )
    {
        Vector4 fullVp( 0, 0, 1, 1 );
        beginRenderPassDescriptor( renderPassDesc, anyTarget, mipLevel, fullVp, fullVp, false, false );
        executeRenderPassDescriptorDelayedActions();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::discardFrameBuffer( unsigned int buffers )
    {
        if( buffers & FBT_COLOUR )
        {
            for( size_t i=0; i<mNumMRTs; ++i )
            {
                if( mCurrentColourRTs[i] )
                    mCurrentColourRTs[i]->mColourAttachmentDesc.loadAction = MTLLoadActionDontCare;
            }
        }

        if( mCurrentDepthBuffer )
        {
            if( buffers & FBT_DEPTH && mCurrentDepthBuffer->mDepthAttachmentDesc )
                mCurrentDepthBuffer->mDepthAttachmentDesc.loadAction = MTLLoadActionDontCare;

            if( buffers & FBT_STENCIL && mCurrentDepthBuffer->mStencilAttachmentDesc )
                mCurrentDepthBuffer->mStencilAttachmentDesc.loadAction = MTLLoadActionDontCare;
        }
    }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getHorizontalTexelOffset(void)
    {
        return 0.0f;
    }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getVerticalTexelOffset(void)
    {
        return 0.0f;
    }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getMinimumDepthInputValue(void)
    {
        return 0.0f;
    }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getMaximumDepthInputValue(void)
    {
        return 1.0f;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setRenderTarget(RenderTarget *target, uint8 viewportRenderTargetFlags)
    {
#if TODO_OGRE_2_2
        {
            const bool activeHasColourWrites = mNumMRTs != 0;
            if( mActiveRenderTarget == target &&
                activeHasColourWrites == (viewportRenderTargetFlags & VP_RTT_COLOUR_WRITE) )
            {
                if( mActiveRenderEncoder && mUavsDirty )
                    flushUAVs();
                return;
            }
        }

        if( mActiveDevice )
            mActiveDevice->endRenderEncoder();

        mActiveRenderTarget = target;

        if( target )
        {
            if( target->getForceDisableColourWrites() )
                viewportRenderTargetFlags &= ~VP_RTT_COLOUR_WRITE;

            mCurrentColourRTs[0] = 0;
            //We need to set mCurrentColourRTs[0] to grab the active device,
            //even if we won't be drawing to colour target.
            target->getCustomAttribute( "mNumMRTs", &mNumMRTs );
            target->getCustomAttribute( "MetalRenderTargetCommon", &mCurrentColourRTs[0] );

            MetalDevice *ownerDevice = 0;

            if( viewportRenderTargetFlags & VP_RTT_COLOUR_WRITE )
            {
                for( size_t i=0; i<mNumMRTs; ++i )
                {
                    MTLRenderPassColorAttachmentDescriptor *desc =
                            mCurrentColourRTs[i]->mColourAttachmentDesc;

                    //TODO. This information is stored in Texture. Metal needs it now.
                    const bool explicitResolve = false;

                    //TODO: Compositor should be able to tell us whether to use
                    //MTLStoreActionDontCare with some future enhancements.
                    if( target->getFSAA() > 1 && !explicitResolve )
                    {
                        desc.storeAction = MTLStoreActionMultisampleResolve;
                    }
                    else
                    {
                        desc.storeAction = MTLStoreActionStore;
                    }

                    ownerDevice = mCurrentColourRTs[i]->getOwnerDevice();
                }
            }
            else
            {
                mNumMRTs = 0;
            }

            MetalDepthBuffer *depthBuffer = static_cast<MetalDepthBuffer*>( target->getDepthBuffer() );

            if( target->getDepthBufferPool() != DepthBuffer::POOL_NO_DEPTH && !depthBuffer )
            {
                // Depth is automatically managed and there is no depth buffer attached to this RT
                setDepthBufferFor( target, true );
            }

            depthBuffer = static_cast<MetalDepthBuffer*>( target->getDepthBuffer() );
            mCurrentDepthBuffer = depthBuffer;
            if( depthBuffer )
            {
                if( depthBuffer->mDepthAttachmentDesc )
                    depthBuffer->mDepthAttachmentDesc.storeAction = MTLStoreActionStore;
                if( depthBuffer->mStencilAttachmentDesc )
                    depthBuffer->mStencilAttachmentDesc.storeAction = MTLStoreActionStore;

                ownerDevice = depthBuffer->getOwnerDevice();
            }

            setActiveDevice( ownerDevice );
        }
        else
        {
            mNumMRTs = 0;
            mCurrentDepthBuffer = 0;
        }
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::preExtraThreadsStarted()
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::postExtraThreadsStarted()
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::registerThread()
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::unregisterThread()
    {
    }
    //-------------------------------------------------------------------------
    const PixelFormatToShaderType* MetalRenderSystem::getPixelFormatToShaderType(void) const
    {
        return &mPixelFormatToShaderType;
    }

    //-------------------------------------------------------------------------
    void MetalRenderSystem::initGPUProfiling(void)
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
//        _rmt_BindMetal( mActiveDevice->mCurrentCommandBuffer );
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::deinitGPUProfiling(void)
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_UnbindMetal();
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::beginGPUSampleProfile( const String &name, uint32 *hashCache )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_BeginMetalSample( name.c_str(), hashCache );
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::endGPUSampleProfile( const String &name )
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
        _rmt_EndMetalSample();
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::beginProfileEvent( const String &eventName )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::endProfileEvent( void )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::markProfileEvent( const String &event )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setClipPlanesImpl(const PlaneList& clipPlanes)
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, Window *primary)
    {
        DepthBuffer::DefaultDepthBufferFormat = PFG_D32_FLOAT_S8X24_UINT;
        mShaderManager = OGRE_NEW MetalGpuProgramManager( &mDevice );
        mMetalProgramFactory = new MetalProgramFactory( &mDevice );
        HighLevelGpuProgramManager::getSingleton().addFactory( mMetalProgramFactory );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::updateCompositorManager( CompositorManager2 *compositorManager,
                                                     SceneManagerEnumerator &sceneManagers,
                                                     HlmsManager *hlmsManager )
    {
        // Metal requires that a frame's worth of rendering be invoked inside an autorelease pool.
        // This is true for both iOS and macOS.
        @autoreleasepool
        {
            compositorManager->_updateImplementation( sceneManagers, hlmsManager );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );
        
        // There are two main cases:
        // 1. The active render encoder is valid and will be subsequently used for drawing.
        //      We need to set the stencil reference value on this encoder. We do this below.
        // 2. The active render is invalid or is about to go away.
        //      In this case, we need to set the stencil reference value on the new encoder when it is created
        //      (see createRenderEncoder). (In this case, the setStencilReferenceValue below in this wasted, but it is inexpensive).

        // Save this info so we can transfer it into a new encoder if necessary
        mStencilEnabled = stencilParams.enabled;
        if (mStencilEnabled)
        {
            mStencilRefValue = refValue;

            if( mActiveRenderEncoder )
                [mActiveRenderEncoder setStencilReferenceValue:refValue];
        }
    }
 }
