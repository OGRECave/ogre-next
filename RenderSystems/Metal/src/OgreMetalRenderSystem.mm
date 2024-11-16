/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
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

#include "CommandBuffer/OgreCbDrawCall.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreDepthBuffer.h"
#include "OgreFrustum.h"
#include "OgreMetalDescriptorSetTexture.h"
#include "OgreMetalDevice.h"
#include "OgreMetalGpuProgramManager.h"
#include "OgreMetalHardwareBufferManager.h"
#include "OgreMetalHardwareIndexBuffer.h"
#include "OgreMetalHardwareVertexBuffer.h"
#include "OgreMetalHlmsPso.h"
#include "OgreMetalMappings.h"
#include "OgreMetalProgram.h"
#include "OgreMetalProgramFactory.h"
#include "OgreMetalRenderPassDescriptor.h"
#include "OgreMetalTextureGpu.h"
#include "OgreMetalTextureGpuManager.h"
#include "OgreMetalWindow.h"
#include "OgreViewport.h"
#include "Vao/OgreIndirectBufferPacked.h"
#include "Vao/OgreMetalBufferInterface.h"
#include "Vao/OgreMetalConstBufferPacked.h"
#include "Vao/OgreMetalTexBufferPacked.h"
#include "Vao/OgreMetalUavBufferPacked.h"
#include "Vao/OgreMetalVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#import <Foundation/NSEnumerator.h>
#import <Metal/Metal.h>

#include <sstream>

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
        mStencilEnabled( false ),
        mStencilRefValue( 0u ),
        mCurrentIndexBuffer( 0 ),
        mCurrentVertexBuffer( 0 ),
        mCurrentPrimType( MTLPrimitiveTypePoint ),
        mAutoParamsBufferIdx( 0 ),
        mCurrentAutoParamsBufferPtr( 0 ),
        mCurrentAutoParamsBufferSpaceLeft( 0 ),
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
        memset( mHistoricalAutoParamsSize, 0, sizeof( mHistoricalAutoParamsSize ) );

        // set config options defaults
        initConfigOptions();
    }
    //-------------------------------------------------------------------------
    MetalRenderSystem::~MetalRenderSystem() { shutdown(); }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::shutdown()
    {
        if( mActiveDevice )
            mActiveDevice->endAllEncoders();

        for( size_t i = 0; i < mAutoParamsBuffer.size(); ++i )
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
    }
    //-------------------------------------------------------------------------
    const String &MetalRenderSystem::getName() const
    {
        static String strName( "Metal Rendering Subsystem" );
        return strName;
    }
    //-------------------------------------------------------------------------
    const String &MetalRenderSystem::getFriendlyName() const
    {
        static String strName( "Metal_RS" );
        return strName;
    }
    //-------------------------------------------------------------------------
    MetalDeviceList *MetalRenderSystem::getDeviceList( bool refreshList )
    {
        if( refreshList || mDeviceList.count() == 0 )
            mDeviceList.refresh();

        return &mDeviceList;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::initConfigOptions()
    {
        ConfigOption optDevice;
        ConfigOption optFSAA;
        ConfigOption optSRGB;

        optDevice.name = "Rendering Device";
        optDevice.currentValue = "(default)";
        optDevice.possibleValues.push_back( "(default)" );
        MetalDeviceList *deviceList = getDeviceList();
        for( unsigned j = 0; j < deviceList->count(); j++ )
            optDevice.possibleValues.push_back( deviceList->item( j )->getDescription() );
        optDevice.immutable = false;

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        optFSAA.possibleValues.push_back( "None" );
        optFSAA.currentValue = "None";

        // SRGB on auto window
        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.possibleValues.push_back( "Yes" );
        optSRGB.possibleValues.push_back( "No" );
        optSRGB.currentValue = "Yes";
        optSRGB.immutable = false;

        mOptions[optDevice.name] = optDevice;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optSRGB.name] = optSRGB;

        refreshFSAAOptions();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setConfigOption( const String &name, const String &value )
    {
        // Find option
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            StringStream str;
            str << "Option named '" << name << "' does not exist.";
            // OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, str.str(), "MetalRenderSystem::setConfigOption"
            // );
        }

        // Refresh dependent options
        if( name == "Rendering Device" )
            refreshFSAAOptions();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::refreshFSAAOptions()
    {
        ConfigOptionMap::iterator it = mOptions.find( "FSAA" );
        ConfigOption *optFSAA = &it->second;
        optFSAA->possibleValues.clear();

        it = mOptions.find( "Rendering Device" );
        if( @available( iOS 9.0, * ) )
        {
            const MetalDeviceItem *deviceItem = getDeviceList()->item( it->second.currentValue );
            id<MTLDevice> device =
                deviceItem ? deviceItem->getMTLDevice() : MTLCreateSystemDefaultDevice();
            for( unsigned samples = 1; samples <= 32; ++samples )
                if( [device supportsTextureSampleCount:samples] )
                    optFSAA->possibleValues.push_back( StringConverter::toString( samples ) + "x" );
        }

        if( optFSAA->possibleValues.empty() )
        {
            optFSAA->possibleValues.push_back( "1x" );
            optFSAA->possibleValues.push_back( "4x" );
        }

        // Reset FSAA to none if previous doesn't avail in new possible values
        if( std::find( optFSAA->possibleValues.begin(), optFSAA->possibleValues.end(),
                       optFSAA->currentValue ) == optFSAA->possibleValues.end() )
        {
            optFSAA->currentValue = optFSAA->possibleValues[0];
        }
    }
    //-------------------------------------------------------------------------
    SampleDescription MetalRenderSystem::validateSampleDescription( const SampleDescription &sampleDesc,
                                                                    PixelFormatGpu format,
                                                                    uint32 textureFlags )
    {
        uint8 samples = sampleDesc.getMaxSamples();
        if( @available( iOS 9.0, * ) )
        {
            if( mActiveDevice )
            {
                while( samples > 1 && ![mActiveDevice->mDevice supportsTextureSampleCount:samples] )
                    --samples;
            }
        }
        return SampleDescription( samples, sampleDesc.getMsaaPattern() );
    }
    //-------------------------------------------------------------------------
    bool MetalRenderSystem::supportsMultithreadedShaderCompilation() const
    {
#ifndef OGRE_SHADER_THREADING_BACKWARDS_COMPATIBLE_API
        return true;
#else
#    ifdef OGRE_SHADER_THREADING_USE_TLS
        return true;
#    else
        return false;
#    endif
#endif
    }
    //-------------------------------------------------------------------------
    HardwareOcclusionQuery *MetalRenderSystem::createHardwareOcclusionQuery()
    {
        return 0;  // TODO
    }
    //-------------------------------------------------------------------------
    RenderSystemCapabilities *MetalRenderSystem::createRenderSystemCapabilities() const
    {
        RenderSystemCapabilities *rsc = new RenderSystemCapabilities();
        rsc->setRenderSystemName( getName() );

        rsc->setDeviceName( mActiveDevice->mDevice.name.UTF8String );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_CPU == OGRE_CPU_ARM
        rsc->setVendor( GPU_APPLE );
#endif

        rsc->setCapability( RSC_HWSTENCIL );
        rsc->setStencilBufferBitDepth( 8 );
        rsc->setNumTextureUnits( 16 );
        rsc->setNumVertexTextureUnits( 16 );
        rsc->setCapability( RSC_ANISOTROPY );
        rsc->setCapability( RSC_AUTOMIPMAP );
        rsc->setCapability( RSC_BLENDING );
        rsc->setCapability( RSC_DOT3 );
        rsc->setCapability( RSC_CUBEMAPPING );
        rsc->setCapability( RSC_TEXTURE_COMPRESSION );
        rsc->setCapability( RSC_SHADER_FLOAT16 );
#if TARGET_OS_TV
        rsc->setCapability( RSC_TEXTURE_COMPRESSION_ASTC );
#endif
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        // If the device is running macOS older than 11,
        // then they are all x86 systems which all support BCn
        bool supportsBCTextureCompression = true;
        if( @available( macOS 11, * ) )
        {
            supportsBCTextureCompression = mActiveDevice->mDevice.supportsBCTextureCompression;
        }

        if( supportsBCTextureCompression )
        {
            rsc->setCapability( RSC_TEXTURE_COMPRESSION_DXT );
            rsc->setCapability( RSC_TEXTURE_COMPRESSION_BC4_BC5 );
            // rsc->setCapability(RSC_TEXTURE_COMPRESSION_BC6H_BC7);
        }
#else
        // Actually the limit is not the count but rather how many bytes are in the
        // GPU's internal TBDR cache (16 bytes for Family 1, 32 bytes for the rest)
        // Consult Metal's manual for more info.
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1] )
        {
            rsc->setCapability( RSC_TEXTURE_COMPRESSION_ASTC );
        }

        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily4_v1] )
            rsc->setCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );
#endif
        rsc->setCapability( RSC_VBO );
        rsc->setCapability( RSC_32BIT_INDEX );
        rsc->setCapability( RSC_TWO_SIDED_STENCIL );
        rsc->setCapability( RSC_STENCIL_WRAP );
        rsc->setCapability( RSC_USER_CLIP_PLANES );
        rsc->setCapability( RSC_VERTEX_FORMAT_UBYTE4 );
        rsc->setCapability( RSC_INFINITE_FAR_PLANE );
        rsc->setCapability( RSC_NON_POWER_OF_2_TEXTURES );
        rsc->setNonPOW2TexturesLimited( false );
        rsc->setCapability( RSC_HWRENDER_TO_TEXTURE );
        rsc->setCapability( RSC_TEXTURE_FLOAT );
        rsc->setCapability( RSC_POINT_SPRITES );
        rsc->setCapability( RSC_POINT_EXTENDED_PARAMETERS );
        rsc->setCapability( RSC_TEXTURE_1D );
        rsc->setCapability( RSC_TEXTURE_3D );
        rsc->setCapability( RSC_TEXTURE_SIGNED_INT );
        rsc->setCapability( RSC_VERTEX_PROGRAM );
        rsc->setCapability( RSC_FRAGMENT_PROGRAM );
        rsc->setCapability( RSC_VERTEX_BUFFER_INSTANCE_DATA );
        rsc->setCapability( RSC_MIPMAP_LOD_BIAS );
        rsc->setCapability( RSC_ALPHA_TO_COVERAGE );
        rsc->setMaxPointSize( 256 );

        rsc->setCapability( RSC_COMPUTE_PROGRAM );
        rsc->setCapability( RSC_HW_GAMMA );
        rsc->setCapability( RSC_TEXTURE_GATHER );
        rsc->setCapability( RSC_TEXTURE_2D_ARRAY );
        rsc->setCapability( RSC_SEPARATE_SAMPLERS_FROM_TEXTURES );
        rsc->setCapability( RSC_CONST_BUFFER_SLOTS_IN_SHADER );
        rsc->setCapability( RSC_EXPLICIT_FSAA_RESOLVE );

        // These don't make sense on Metal, so just use flexible defaults.
        rsc->setVertexProgramConstantFloatCount( 16384 );
        rsc->setVertexProgramConstantBoolCount( 16384 );
        rsc->setVertexProgramConstantIntCount( 16384 );
        rsc->setFragmentProgramConstantFloatCount( 16384 );
        rsc->setFragmentProgramConstantBoolCount( 16384 );
        rsc->setFragmentProgramConstantIntCount( 16384 );
        rsc->setComputeProgramConstantFloatCount( 16384 );
        rsc->setComputeProgramConstantBoolCount( 16384 );
        rsc->setComputeProgramConstantIntCount( 16384 );

#if defined( __IPHONE_13_0 ) || defined( __MAC_10_15 )
        if( @available( iOS 13.0, macOS 10.15, * ) )
        {
            if( mActiveDevice->mDevice.hasUnifiedMemory )
                rsc->setCapability( RSC_UMA );
        }
#endif

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        uint8 mrtCount = 8u;
#else
        // Actually the limit is not the count but rather how many bytes are in the
        // GPU's internal TBDR cache (16 bytes for Family 1, 32 bytes for the rest)
        // Consult Metal's manual for more info.
        uint8 mrtCount = 4u;
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v1] ||
            [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily2_v2] ||
            [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1] )
        {
            mrtCount = 8u;
        }
#endif
        rsc->setNumMultiRenderTargets( std::min<ushort>( mrtCount, OGRE_MAX_MULTIPLE_RENDER_TARGETS ) );
        rsc->setCapability( RSC_MRT_DIFFERENT_BIT_DEPTHS );

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
            rsc->setCapability( RSC_STORE_AND_MULTISAMPLE_RESOLVE );
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily4_v1] )
            rsc->setCapability( RSC_DEPTH_CLAMP );
#else
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_OSX_GPUFamily1_v2] )
            rsc->setCapability( RSC_STORE_AND_MULTISAMPLE_RESOLVE );
        rsc->setCapability( RSC_DEPTH_CLAMP );
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || \
    ( OGRE_PLATFORM == OGRE_PLATFORM_APPLE && OGRE_CPU == OGRE_CPU_ARM && \
      OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64 )
        rsc->setCapability( RSC_IS_TILER );
        rsc->setCapability( RSC_TILER_CAN_CLEAR_STENCIL_REGION );
#endif

        rsc->setCapability( RSC_UAV );
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
        rsc->setCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily5_v1] )
            rsc->setCapability( RSC_VP_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER );

        if( [mActiveDevice->mDevice supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily4_v1] )
            rsc->setCapability( RSC_TYPED_UAV_LOADS );
#else
        rsc->setCapability( RSC_VP_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER );
        rsc->setCapability( RSC_TYPED_UAV_LOADS );
#endif

        {
            uint32 numTexturesInTextureDescriptor[NumShaderTypes + 1];
            for( size_t i = 0u; i < NumShaderTypes + 1; ++i )
            {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
                // There is a discrepancy. Apple docs say:
                //  MTLGPUFamilyApple4 (A11 + iOS 13) supports 96 texture units
                //  MTLFeatureSet_iOS_GPUFamily4_v1 (A11 + iOS 12) supports 31 texture units
                //
                // Basically same HW but different OS. This is actually possible.
                //
                // TODO: Query MTLGPUFamilyApple4, MTLGPUFamilyApple5 and onwards on iOS 13+
                // numTexturesInTextureDescriptor[i] = 31u;

                // TODO: Actual limit became OGRE_METAL_UAV_SLOT_START since it otherwise
                // overlaps
                numTexturesInTextureDescriptor[i] = OGRE_METAL_UAV_SLOT_START;
#else
                // numTexturesInTextureDescriptor[i] = 128u;
                numTexturesInTextureDescriptor[i] = OGRE_METAL_UAV_SLOT_START;
#endif
            }
            numTexturesInTextureDescriptor[HullShader] = 0u;
            numTexturesInTextureDescriptor[DomainShader] = 0u;
            // Compute Shader has a different start
            numTexturesInTextureDescriptor[NumShaderTypes] = OGRE_METAL_CS_UAV_SLOT_START;
            rsc->setNumTexturesInTextureDescriptor( numTexturesInTextureDescriptor );
        }
        // rsc->setCapability(RSC_ATOMIC_COUNTERS);

        rsc->addShaderProfile( "metal" );

        DriverVersion driverVersion;

        struct FeatureSets
        {
            MTLFeatureSet featureSet;
            const char *name;
            int major;
            int minor;
        };

        FeatureSets featureSets[] = {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            { MTLFeatureSet_iOS_GPUFamily1_v1, "iOS_GPUFamily1_v1", 1, 1 },
            { MTLFeatureSet_iOS_GPUFamily2_v1, "iOS_GPUFamily2_v1", 2, 1 },

            { MTLFeatureSet_iOS_GPUFamily1_v2, "iOS_GPUFamily1_v2", 1, 2 },
            { MTLFeatureSet_iOS_GPUFamily2_v2, "iOS_GPUFamily2_v2", 2, 2 },
            { MTLFeatureSet_iOS_GPUFamily3_v1, "iOS_GPUFamily3_v2", 3, 2 },

            { MTLFeatureSet_iOS_GPUFamily4_v1, "iOS_GPUFamily4_v1", 4, 1 },

#    ifdef __IPHONE_12_0
            { MTLFeatureSet_iOS_GPUFamily4_v2, "iOS_GPUFamily4_v2", 4, 2 },
            { MTLFeatureSet_iOS_GPUFamily5_v1, "iOS_GPUFamily5_v1", 5, 1 },
#    endif
#else
            { MTLFeatureSet_OSX_GPUFamily1_v1, "OSX_GPUFamily1_v1", 1, 1 },
#endif
        };

        for( size_t i = 0; i < sizeof( featureSets ) / sizeof( featureSets[0] ); ++i )
        {
            if( [mActiveDevice->mDevice supportsFeatureSet:featureSets[i].featureSet] )
            {
                LogManager::getSingleton().logMessage( "Supports: " + String( featureSets[i].name ) );
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
    void MetalRenderSystem::reinitialise()
    {
        this->shutdown();
        this->_initialise( true );
    }
    //-------------------------------------------------------------------------
    Window *MetalRenderSystem::_initialise( bool autoCreateWindow, const String &windowTitle )
    {
        ConfigOptionMap::iterator opt = mOptions.find( "Rendering Device" );
        if( opt == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can`t find requested Metal device name!",
                         "MetalRenderSystem::initialise" );
        mDeviceName = opt->second.currentValue;

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
            const MetalDeviceItem *deviceItem = getDeviceList( true )->item( mDeviceName );
            mDevice.init( deviceItem );
            setActiveDevice( &mDevice );
            String selectedDeviceName =
                deviceItem
                    ? deviceItem->getDescription()
                    : MetalDeviceItem( mDevice.mDevice, 0 ).getDescription() + " (system default)";
            LogManager::getSingleton().stream() << "Metal: Requested \"" << mDeviceName
                                                << "\", selected \"" << selectedDeviceName << "\"";

            uint8 dynamicBufferMultiplier = 3u;

            if( miscParams )
            {
                NameValuePairList::const_iterator itOption = miscParams->find( "reverse_depth" );
                if( itOption != miscParams->end() )
                    mReverseDepth = StringConverter::parseBool( itOption->second, true );

                itOption = miscParams->find( "VaoManager::mDynamicBufferMultiplier" );
                if( itOption != miscParams->end() )
                {
                    const uint32 newBufMult =
                        StringConverter::parseUnsignedInt( itOption->second, dynamicBufferMultiplier );
                    dynamicBufferMultiplier = static_cast<uint8>( newBufMult );
                    OGRE_ASSERT_LOW( dynamicBufferMultiplier > 0u );
                }
            }

            mMainGpuSyncSemaphore = dispatch_semaphore_create( dynamicBufferMultiplier );
            mMainSemaphoreAlreadyWaited = false;
            mBeginFrameOnceStarted = false;
            mRealCapabilities = createRenderSystemCapabilities();
            mDriverVersion = mRealCapabilities->getDriverVersion();

            if( !mUseCustomCapabilities )
                mCurrentCapabilities = mRealCapabilities;

            fireEvent( "RenderSystemCapabilitiesCreated" );

            initialiseFromRenderSystemCapabilities( mCurrentCapabilities, 0 );

            mVaoManager = OGRE_NEW MetalVaoManager( &mDevice, miscParams );
            // If the assert fails, probably the default value of dynamicBufferMultiplier
            // does not match the VaoManager's
            OGRE_ASSERT_LOW( mVaoManager->getDynamicBufferMultiplier() == dynamicBufferMultiplier );
            mHardwareBufferManager = new v1::MetalHardwareBufferManager( &mDevice, mVaoManager );
            mTextureGpuManager = OGRE_NEW MetalTextureGpuManager( mVaoManager, this, &mDevice );

            mInitialized = true;
        }

        Window *win = OGRE_NEW MetalWindow( name, width, height, fullScreen, miscParams, &mDevice );
        mWindows.insert( win );

        win->_initialize( mTextureGpuManager, miscParams );
        return win;
    }
    //-------------------------------------------------------------------------
    String MetalRenderSystem::getErrorDescription( long errorNumber ) const { return BLANKSTRING; }
    //-------------------------------------------------------------------------
    bool MetalRenderSystem::hasStoreAndMultisampleResolve() const
    {
        return mCurrentCapabilities->hasCapability( RSC_STORE_AND_MULTISAMPLE_RESOLVE );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_useLights( const LightList &lights, unsigned short limit ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setWorldMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setViewMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setProjectionMatrix( const Matrix4 &m ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                               const ColourValue &specular, const ColourValue &emissive,
                                               Real shininess, TrackVertexColourType tracking )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPointSpritesEnabled( bool enabled ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPointParameters( Real size, bool attenuationEnabled, Real constant,
                                                 Real linear, Real quadratic, Real minSize,
                                                 Real maxSize )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::flushUAVs()
    {
        if( mUavRenderingDirty && mActiveRenderEncoder )
        {
            if( mUavRenderingDescSet )
            {
                MetalDescriptorSetTexture *metalSet =
                    reinterpret_cast<MetalDescriptorSetTexture *>( mUavRenderingDescSet->mRsData );

                // Bind textures
                {
                    FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
                    FastArray<MetalTexRegion>::const_iterator endt = metalSet->textures.end();

                    while( itor != endt )
                    {
                        NSRange range = itor->range;
                        range.location += /*mUavStartingSlot +*/ OGRE_METAL_UAV_SLOT_START;
                        [mActiveRenderEncoder setVertexTextures:itor->textures withRange:range];
                        [mActiveRenderEncoder setFragmentTextures:itor->textures withRange:range];
                        ++itor;
                    }
                }

                // Bind buffers
                {
                    FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
                    FastArray<MetalBufferRegion>::const_iterator endt = metalSet->buffers.end();

                    while( itor != endt )
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
    void MetalRenderSystem::_setTexture( size_t unit, TextureGpu *texPtr, bool bDepthReadOnly )
    {
        if( texPtr )
        {
            const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu *>( texPtr );
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
        FastArray<const TextureGpu *>::const_iterator itor = set->mTextures.begin();

        // for( size_t i=0u; i<NumShaderTypes; ++i )
        for( size_t i = 0u; i < PixelShader + 1u; ++i )
        {
            const size_t numTexturesUsed = set->mShaderTypeTexCount[i];
            for( size_t j = 0u; j < numTexturesUsed; ++j )
            {
                const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu *>( *itor );
                __unsafe_unretained id<MTLTexture> metalTexture = 0;

                if( metalTex )
                {
                    metalTexture = metalTex->getDisplayTextureName();

                    if( ( texUnit - slotStart ) == hazardousTexIdx &&
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
            reinterpret_cast<MetalDescriptorSetTexture *>( set->mRsData );

        // Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator endt = metalSet->textures.end();

            while( itor != endt )
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

        // Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator endt = metalSet->buffers.end();

            while( itor != endt )
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
        __unsafe_unretained id<MTLSamplerState> samplers[16];

        FastArray<const HlmsSamplerblock *>::const_iterator itor = set->mSamplers.begin();

        NSRange texUnitRange;
        texUnitRange.location = slotStart;
        // for( size_t i=0u; i<NumShaderTypes; ++i )
        for( size_t i = 0u; i < PixelShader + 1u; ++i )
        {
            const NSUInteger numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j = 0; j < numSamplersUsed; ++j )
            {
                if( *itor )
                    samplers[j] = (__bridge id<MTLSamplerState>)( *itor )->mRsData;
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
        FastArray<const TextureGpu *>::const_iterator itor = set->mTextures.begin();

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        size_t texIdx = 0;

        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const size_t numTexturesUsed = set->mShaderTypeTexCount[i];
            for( size_t j = 0u; j < numTexturesUsed; ++j )
            {
                const MetalTextureGpu *metalTex = static_cast<const MetalTextureGpu *>( *itor );
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
            reinterpret_cast<MetalDescriptorSetTexture *>( set->mRsData );

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        // Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator endt = metalSet->textures.end();

            while( itor != endt )
            {
                NSRange range = itor->range;
                range.location += slotStart;
                [computeEncoder setTextures:itor->textures withRange:range];
                ++itor;
            }
        }

        // Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator endt = metalSet->buffers.end();

            while( itor != endt )
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
        __unsafe_unretained id<MTLSamplerState> samplers[16];

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        FastArray<const HlmsSamplerblock *>::const_iterator itor = set->mSamplers.begin();

        NSRange texUnitRange;
        texUnitRange.location = slotStart;
        for( size_t i = 0u; i < NumShaderTypes; ++i )
        {
            const NSUInteger numSamplersUsed = set->mShaderTypeSamplerCount[i];

            if( !numSamplersUsed )
                continue;

            for( size_t j = 0; j < numSamplersUsed; ++j )
            {
                if( *itor )
                    samplers[j] = (__bridge id<MTLSamplerState>)( *itor )->mRsData;
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
            reinterpret_cast<MetalDescriptorSetTexture *>( set->mRsData );

        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        // Bind textures
        {
            FastArray<MetalTexRegion>::const_iterator itor = metalSet->textures.begin();
            FastArray<MetalTexRegion>::const_iterator endt = metalSet->textures.end();

            while( itor != endt )
            {
                NSRange range = itor->range;
                range.location += slotStart + OGRE_METAL_CS_UAV_SLOT_START;
                [computeEncoder setTextures:itor->textures withRange:range];
                ++itor;
            }
        }

        // Bind buffers
        {
            FastArray<MetalBufferRegion>::const_iterator itor = metalSet->buffers.begin();
            FastArray<MetalBufferRegion>::const_iterator endt = metalSet->buffers.end();

            while( itor != endt )
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
        // mDevice
    }
    //-------------------------------------------------------------------------
    RenderPassDescriptor *MetalRenderSystem::createRenderPassDescriptor()
    {
        RenderPassDescriptor *retVal = OGRE_NEW MetalRenderPassDescriptor( mActiveDevice, this );
        mRenderPassDescs.insert( retVal );
        return retVal;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                                       uint8 mipLevel, const Vector4 *viewportSizes,
                                                       const Vector4 *scissors, uint32 numViewports,
                                                       bool overlaysEnabled, bool warnIfRtvWasFlushed )
    {
        if( desc->mInformationOnly && desc->hasSameAttachments( mCurrentRenderPassDescriptor ) )
            return;

        const int oldWidth = mCurrentRenderViewport[0].getActualWidth();
        const int oldHeight = mCurrentRenderViewport[0].getActualHeight();
        const int oldX = mCurrentRenderViewport[0].getActualLeft();
        const int oldY = mCurrentRenderViewport[0].getActualTop();

        MetalRenderPassDescriptor *currPassDesc =
            static_cast<MetalRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

        RenderSystem::beginRenderPassDescriptor( desc, anyTarget, mipLevel, viewportSizes, scissors,
                                                 numViewports, overlaysEnabled, warnIfRtvWasFlushed );

        // Calculate the new "lower-left" corner of the viewport to compare with the old one
        const int w = mCurrentRenderViewport[0].getActualWidth();
        const int h = mCurrentRenderViewport[0].getActualHeight();
        const int x = mCurrentRenderViewport[0].getActualLeft();
        const int y = mCurrentRenderViewport[0].getActualTop();

        const bool vpChanged =
            oldX != x || oldY != y || oldWidth != w || oldHeight != h || numViewports > 1u;

        MetalRenderPassDescriptor *newPassDesc = static_cast<MetalRenderPassDescriptor *>( desc );

        // Determine whether:
        //  1. We need to store current active RenderPassDescriptor
        //  2. We need to perform clears when loading the new RenderPassDescriptor
        uint32 entriesToFlush = 0;
        if( currPassDesc )
        {
            entriesToFlush = currPassDesc->willSwitchTo( newPassDesc, warnIfRtvWasFlushed );

            if( entriesToFlush != 0 )
                currPassDesc->performStoreActions( entriesToFlush, false );

            // If rendering was interrupted but we're still rendering to the same
            // RTT, willSwitchTo will have returned 0 and thus we won't perform
            // the necessary load actions.
            // If RTTs were different, we need to have performStoreActions
            // called by now (i.e. to emulate StoreAndResolve)
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

        mEntriesToFlush = entriesToFlush;
        mVpChanged = vpChanged;
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
                static_cast<MetalRenderPassDescriptor *>( mCurrentRenderPassDescriptor );

            MTLRenderPassDescriptor *passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            newPassDesc->performLoadActions( passDesc, mInterruptedRenderCommandEncoder );

            mActiveDevice->mRenderEncoder =
                [mActiveDevice->mCurrentCommandBuffer renderCommandEncoderWithDescriptor:passDesc];
            mActiveRenderEncoder = mActiveDevice->mRenderEncoder;

            static_cast<MetalVaoManager *>( mVaoManager )->bindDrawId();
            [mActiveRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];

            if( mStencilEnabled )
                [mActiveRenderEncoder setStencilReferenceValue:mStencilRefValue];

            mInterruptedRenderCommandEncoder = false;
        }

        flushUAVs();

        const uint32 numViewports = mMaxBoundViewports;

        // If we flushed, viewport and scissor settings got reset.
        if( !mCurrentRenderViewport[0].coversEntireTarget() || ( mVpChanged && !mEntriesToFlush ) ||
            numViewports > 1u )
        {
#if defined( __IPHONE_12_0 ) || \
    ( defined( MAC_OS_X_VERSION_10_13 ) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS )
            bool hasMultiViewports = false;
            if( @available( iOS 12.0, macOS 10.13, tvOS 12.0, * ) )
                hasMultiViewports = true;
            if( numViewports <= 1u || !hasMultiViewports )
#endif
            {
                MTLViewport mtlVp;
                mtlVp.originX = mCurrentRenderViewport[0].getActualLeft();
                mtlVp.originY = mCurrentRenderViewport[0].getActualTop();
                mtlVp.width = mCurrentRenderViewport[0].getActualWidth();
                mtlVp.height = mCurrentRenderViewport[0].getActualHeight();
                mtlVp.znear = 0;
                mtlVp.zfar = 1;
                [mActiveRenderEncoder setViewport:mtlVp];
            }
#if defined( __IPHONE_12_0 ) || \
    ( defined( MAC_OS_X_VERSION_10_13 ) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS )
            else
            {
                MTLViewport mtlVp[16];
                for( size_t i = 0; i < numViewports; ++i )
                {
                    mtlVp[i].originX = mCurrentRenderViewport[i].getActualLeft();
                    mtlVp[i].originY = mCurrentRenderViewport[i].getActualTop();
                    mtlVp[i].width = mCurrentRenderViewport[i].getActualWidth();
                    mtlVp[i].height = mCurrentRenderViewport[i].getActualHeight();
                    mtlVp[i].znear = 0;
                    mtlVp[i].zfar = 1;
                }
                [mActiveRenderEncoder setViewports:mtlVp count:numViewports];
            }
#endif
        }

        if( ( !mCurrentRenderViewport[0].coversEntireTarget() ||
              !mCurrentRenderViewport[0].scissorsMatchViewport() ) ||
            !mEntriesToFlush || numViewports > 1u )
        {
#if defined( __IPHONE_12_0 ) || \
    ( defined( MAC_OS_X_VERSION_10_13 ) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS )
            bool hasMultiViewports = false;
            if( @available( iOS 12.0, macOS 10.13, tvOS 12.0, * ) )
                hasMultiViewports = true;
            if( numViewports <= 1u || !hasMultiViewports )
#endif
            {
                MTLScissorRect scissorRect;
                scissorRect.x = (NSUInteger)mCurrentRenderViewport[0].getScissorActualLeft();
                scissorRect.y = (NSUInteger)mCurrentRenderViewport[0].getScissorActualTop();
                scissorRect.width = (NSUInteger)mCurrentRenderViewport[0].getScissorActualWidth();
                scissorRect.height = (NSUInteger)mCurrentRenderViewport[0].getScissorActualHeight();
                [mActiveRenderEncoder setScissorRect:scissorRect];
            }
#if defined( __IPHONE_12_0 ) || \
    ( defined( MAC_OS_X_VERSION_10_13 ) && OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS )
            else
            {
                MTLScissorRect scissorRect[16];
                for( size_t i = 0; i < numViewports; ++i )
                {
                    scissorRect[i].x = (NSUInteger)mCurrentRenderViewport[i].getScissorActualLeft();
                    scissorRect[i].y = (NSUInteger)mCurrentRenderViewport[i].getScissorActualTop();
                    scissorRect[i].width = (NSUInteger)mCurrentRenderViewport[i].getScissorActualWidth();
                    scissorRect[i].height =
                        (NSUInteger)mCurrentRenderViewport[i].getScissorActualHeight();
                }
                [mActiveRenderEncoder setScissorRects:scissorRect count:numViewports];
            }
#endif
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
    void MetalRenderSystem::executeRenderPassDescriptorDelayedActions()
    {
        executeRenderPassDescriptorDelayedActions( true );
    }
    //-------------------------------------------------------------------------
    inline void MetalRenderSystem::endRenderPassDescriptor( bool isInterruptingRender )
    {
        if( mCurrentRenderPassDescriptor )
        {
            MetalRenderPassDescriptor *passDesc =
                static_cast<MetalRenderPassDescriptor *>( mCurrentRenderPassDescriptor );
            passDesc->performStoreActions( RenderPassDescriptor::All, isInterruptingRender );

            mEntriesToFlush = 0;
            mVpChanged = true;

            mInterruptedRenderCommandEncoder = isInterruptingRender;

            if( !isInterruptingRender )
                RenderSystem::endRenderPassDescriptor();
            else
                mEntriesToFlush = RenderPassDescriptor::All;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::endRenderPassDescriptor() { endRenderPassDescriptor( false ); }
    //-----------------------------------------------------------------------------------
    TextureGpu *MetalRenderSystem::createDepthBufferFor( TextureGpu *colourTexture,
                                                         bool preferDepthTexture,
                                                         PixelFormatGpu depthBufferFormat,
                                                         uint16 poolId )
    {
        if( depthBufferFormat == PFG_UNKNOWN )
            depthBufferFormat = DepthBuffer::DefaultDepthBufferFormat;

        return RenderSystem::createDepthBufferFor( colourTexture, preferDepthTexture, depthBufferFormat,
                                                   poolId );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                         const Frustum *frustum )
    {
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setTextureMatrix( size_t unit, const Matrix4 &xform ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setIndirectBuffer( IndirectBufferPacked *indirectBuffer )
    {
        if( mVaoManager->supportsIndirectBuffers() )
        {
            if( indirectBuffer )
            {
                MetalBufferInterface *bufferInterface =
                    static_cast<MetalBufferInterface *>( indirectBuffer->getBufferInterface() );
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
    void MetalRenderSystem::_waitForTailFrameToFinish()
    {
        if( !mMainSemaphoreAlreadyWaited )
        {
            dispatch_semaphore_wait( mMainGpuSyncSemaphore, DISPATCH_TIME_FOREVER );
            // Semaphore was just grabbed, so ensure we don't grab it twice.
            mMainSemaphoreAlreadyWaited = true;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_beginFrameOnce()
    {
        assert( !mBeginFrameOnceStarted && "Calling MetalRenderSystem::_beginFrameOnce more than once "
                                           "without matching call to _endFrameOnce!!!" );

        // Allow the renderer to preflight 3 frames on the CPU (using a semapore as a guard) and
        // commit them to the GPU. This semaphore will get signaled once the GPU completes a
        // frame's work via addCompletedHandler callback below, signifying the CPU can go ahead
        // and prepare another frame.
        _waitForTailFrameToFinish();

        mBeginFrameOnceStarted = true;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_endFrameOnce()
    {
        @autoreleasepool
        {
            // TODO: We shouldn't tidy up JUST the active device. But all of them.
            RenderSystem::_endFrameOnce();

            cleanAutoParamsBuffers();

            __block dispatch_semaphore_t blockSemaphore = mMainGpuSyncSemaphore;
            [mActiveDevice->mCurrentCommandBuffer addCompletedHandler:^( id<MTLCommandBuffer> buffer ) {
              // GPU has completed rendering the frame and is done using the contents of any buffers
              // previously encoded on the CPU for that frame. Signal the semaphore and allow the CPU
              // to proceed and construct the next frame.
              dispatch_semaphore_signal( blockSemaphore );
            }];

            mActiveDevice->commitAndNextCommandBuffer();

            mActiveDevice->mFrameAborted = false;
            mMainSemaphoreAlreadyWaited = false;
            mBeginFrameOnceStarted = false;
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::cleanAutoParamsBuffers()
    {
        const size_t numUsedBuffers = mAutoParamsBufferIdx;
        size_t usedBytes = 0;
        for( size_t i = 0; i < numUsedBuffers; ++i )
        {
            mAutoParamsBuffer[i]->unmap( ( i == 0u && numUsedBuffers == 1u ) ? UO_KEEP_PERSISTENT
                                                                             : UO_UNMAP_ALL );
            usedBytes += mAutoParamsBuffer[i]->getTotalSizeBytes();
        }

        // Get the max amount of bytes consumed in the last N frames.
        const int numHistoricSamples =
            sizeof( mHistoricalAutoParamsSize ) / sizeof( mHistoricalAutoParamsSize[0] );
        mHistoricalAutoParamsSize[numHistoricSamples - 1] = usedBytes;
        for( int i = 0; i < numHistoricSamples - 1; ++i )
        {
            usedBytes = std::max( usedBytes, mHistoricalAutoParamsSize[i + 1] );
            mHistoricalAutoParamsSize[i] = mHistoricalAutoParamsSize[i + 1];
        }

        if(  // Merge all buffers into one for the next frame.
            numUsedBuffers > 1u ||
            // Historic data shows we've been using less memory. Shrink this record.
            ( !mAutoParamsBuffer.empty() && mAutoParamsBuffer[0]->getTotalSizeBytes() > usedBytes ) )
        {
            if( mAutoParamsBuffer[0]->getMappingState() != MS_UNMAPPED )
                mAutoParamsBuffer[0]->unmap( UO_UNMAP_ALL );

            for( size_t i = 0; i < mAutoParamsBuffer.size(); ++i )
                mVaoManager->destroyConstBuffer( mAutoParamsBuffer[i] );
            mAutoParamsBuffer.clear();

            if( usedBytes > 0 )
            {
                ConstBufferPacked *constBuffer =
                    mVaoManager->createConstBuffer( usedBytes, BT_DYNAMIC_PERSISTENT, 0, false );
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
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( newPso );
#endif

        MetalProgram *computeShader =
            static_cast<MetalProgram *>( newPso->computeShader->_getBindingDelegate() );

        // Btw. HlmsCompute guarantees mNumThreadGroups won't have zeroes.
        assert( newPso->mNumThreadGroups[0] != 0 && newPso->mNumThreadGroups[1] != 0 &&
                newPso->mNumThreadGroups[2] != 0 &&
                "Invalid parameters. Will also cause div. by zero!" );

        NSError *error = 0;
        MTLComputePipelineDescriptor *psd = [[MTLComputePipelineDescriptor alloc] init];
        psd.computeFunction = computeShader->getMetalFunction();
        psd.threadGroupSizeIsMultipleOfThreadExecutionWidth =
            ( newPso->mThreadsPerGroup[0] % newPso->mNumThreadGroups[0] == 0 ) &&
            ( newPso->mThreadsPerGroup[1] % newPso->mNumThreadGroups[1] == 0 ) &&
            ( newPso->mThreadsPerGroup[2] % newPso->mNumThreadGroups[2] == 0 );

        id<MTLComputePipelineState> pso =
            [mActiveDevice->mDevice newComputePipelineStateWithDescriptor:psd
                                                                  options:MTLPipelineOptionNone
                                                               reflection:nil
                                                                    error:&error];
        if( !pso || error )
        {
            String errorDesc;
            if( error )
                errorDesc = error.localizedDescription.UTF8String;

            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failed to create pipeline state for compute, error " + errorDesc,
                         "MetalProgram::_hlmsComputePipelineStateObjectCreated" );
        }

        newPso->rsData = const_cast<void *>( CFBridgingRetain( pso ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsComputePipelineStateObjectDestroyed( HlmsComputePso *pso )
    {
        if( pso->rsData )  // holds id<MTLComputePipelineState>
            CFRelease( pso->rsData );
        pso->rsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_beginFrame() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_endFrame() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setActiveDevice( MetalDevice *device )
    {
        if( mActiveDevice != device )
        {
            mActiveDevice = device;
            mActiveRenderEncoder = device ? device->mRenderEncoder : 0;
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
    void MetalRenderSystem::_notifyActiveComputeEnded() { mComputePso = 0; }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyNewCommandBuffer()
    {
        MetalVaoManager *vaoManager = static_cast<MetalVaoManager *>( mVaoManager );
        vaoManager->_notifyNewCommandBuffer();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_notifyDeviceStalled()
    {
        v1::MetalHardwareBufferManager *hwBufferMgr =
            static_cast<v1::MetalHardwareBufferManager *>( mHardwareBufferManager );
        MetalVaoManager *vaoManager = static_cast<MetalVaoManager *>( mVaoManager );

        hwBufferMgr->_notifyDeviceStalled();
        vaoManager->_notifyDeviceStalled();
    }
    //-------------------------------------------------------------------------
    id<MTLDepthStencilState> MetalRenderSystem::getDepthStencilState( HlmsPso *pso )
    {
        CachedDepthStencilState depthState;
        if( pso->macroblock->mDepthCheck )
        {
            depthState.depthFunc = pso->macroblock->mDepthFunc;
            if( mReverseDepth )
                depthState.depthFunc = reverseCompareFunction( depthState.depthFunc );
            depthState.depthWrite = pso->macroblock->mDepthWrite;
        }
        else
        {
            depthState.depthFunc = CMPF_ALWAYS_PASS;
            depthState.depthWrite = false;
        }

        depthState.stencilParams = pso->pass.stencilParams;

        ScopedLock lock( mMutexDepthStencilStates );

        CachedDepthStencilStateVec::iterator itor =
            std::lower_bound( mDepthStencilStates.begin(), mDepthStencilStates.end(), depthState );

        if( itor == mDepthStencilStates.end() || depthState != *itor )
        {
            // Not cached. Add the entry
            MTLDepthStencilDescriptor *depthStateDesc = [[MTLDepthStencilDescriptor alloc] init];
            depthStateDesc.depthCompareFunction = MetalMappings::get( depthState.depthFunc );
            depthStateDesc.depthWriteEnabled = depthState.depthWrite;

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
            depthState.depthFunc = pso->macroblock->mDepthFunc;
            if( mReverseDepth )
                depthState.depthFunc = reverseCompareFunction( depthState.depthFunc );
            depthState.depthWrite = pso->macroblock->mDepthWrite;
        }
        else
        {
            depthState.depthFunc = CMPF_ALWAYS_PASS;
            depthState.depthWrite = false;
        }

        depthState.stencilParams = pso->pass.stencilParams;

        ScopedLock lock( mMutexDepthStencilStates );
        CachedDepthStencilStateVec::iterator itor =
            std::lower_bound( mDepthStencilStates.begin(), mDepthStencilStates.end(), depthState );

        if( itor == mDepthStencilStates.end() || !( depthState != *itor ) )
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
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        debugLogPso( newPso );
#endif

        MTLRenderPipelineDescriptor *psd = [[MTLRenderPipelineDescriptor alloc] init];
        [psd
            setSampleCount:newPso->pass.sampleDescription.getColourSamples()];  // aka .rasterSampleCount

        MetalProgram *vertexShader = 0;
        MetalProgram *pixelShader = 0;

        if( newPso->vertexShader )
        {
            vertexShader = static_cast<MetalProgram *>( newPso->vertexShader->_getBindingDelegate() );
            [psd setVertexFunction:vertexShader->getMetalFunction()];
        }

        if( newPso->pixelShader &&
            newPso->blendblock->mBlendChannelMask != HlmsBlendblock::BlendChannelForceDisabled )
        {
            pixelShader = static_cast<MetalProgram *>( newPso->pixelShader->_getBindingDelegate() );
            [psd setFragmentFunction:pixelShader->getMetalFunction()];
        }
        // Ducttape shaders
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
            VertexElement2VecVec::const_iterator endt = newPso->vertexElements.end();

            while( itor != endt )
            {
                size_t accumOffset = 0;
                const size_t bufferIdx = size_t( itor - newPso->vertexElements.begin() );
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
            vertexDescriptor.attributes[15].format = MTLVertexFormatUInt;
            vertexDescriptor.attributes[15].bufferIndex = 15;
            vertexDescriptor.attributes[15].offset = 0;

            vertexDescriptor.layouts[15].stride = 4;
            vertexDescriptor.layouts[15].stepFunction = MTLVertexStepFunctionPerInstance;
#endif
            psd.vertexDescriptor = vertexDescriptor;
        }

        psd.alphaToCoverageEnabled =
            newPso->blendblock->mAlphaToCoverage == HlmsBlendblock::A2cEnabled ||
            ( newPso->blendblock->mAlphaToCoverage == HlmsBlendblock::A2cEnabledMsaaOnly &&
              newPso->pass.sampleDescription.isMultisample() );

        uint8 mrtCount = 0;
        for( size_t i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
        {
            if( newPso->pass.colourFormat[i] != PFG_NULL )
                mrtCount = uint8( i + 1u );
        }

        for( size_t i = 0; i < mrtCount; ++i )
        {
            HlmsBlendblock const *blendblock = newPso->blendblock;
            psd.colorAttachments[i].pixelFormat =
                MetalMappings::get( newPso->pass.colourFormat[i], mActiveDevice );

            if( psd.colorAttachments[i].pixelFormat == MTLPixelFormatInvalid ||
                ( blendblock->mBlendOperation == SBO_ADD && blendblock->mSourceBlendFactor == SBF_ONE &&
                  blendblock->mDestBlendFactor == SBF_ZERO &&
                  ( !blendblock->mSeparateBlend ||
                    ( blendblock->mBlendOperation == blendblock->mBlendOperationAlpha &&
                      blendblock->mSourceBlendFactor == blendblock->mSourceBlendFactorAlpha &&
                      blendblock->mDestBlendFactor == blendblock->mDestBlendFactorAlpha ) ) ) )
            {
                // Note: Can NOT use blendblock->mIsTransparent. What Ogre understand
                // as transparent differs from what Metal understands.
                psd.colorAttachments[i].blendingEnabled = NO;
            }
            else
            {
                psd.colorAttachments[i].blendingEnabled = YES;
            }
            psd.colorAttachments[i].rgbBlendOperation =
                MetalMappings::get( blendblock->mBlendOperation );
            psd.colorAttachments[i].alphaBlendOperation =
                MetalMappings::get( blendblock->mBlendOperationAlpha );
            psd.colorAttachments[i].sourceRGBBlendFactor =
                MetalMappings::get( blendblock->mSourceBlendFactor );
            psd.colorAttachments[i].destinationRGBBlendFactor =
                MetalMappings::get( blendblock->mDestBlendFactor );
            psd.colorAttachments[i].sourceAlphaBlendFactor =
                MetalMappings::get( blendblock->mSeparateBlend ? blendblock->mSourceBlendFactorAlpha
                                                               : blendblock->mSourceBlendFactor );
            psd.colorAttachments[i].destinationAlphaBlendFactor =
                MetalMappings::get( blendblock->mSeparateBlend ? blendblock->mDestBlendFactorAlpha
                                                               : blendblock->mDestBlendFactor );

            psd.colorAttachments[i].writeMask = MetalMappings::get( blendblock->mBlendChannelMask );
        }

        if( newPso->pass.depthFormat != PFG_NULL )
        {
            MTLPixelFormat depthFormat = MTLPixelFormatInvalid;
            MTLPixelFormat stencilFormat = MTLPixelFormatInvalid;
            MetalMappings::getDepthStencilFormat( newPso->pass.depthFormat, mActiveDevice, depthFormat,
                                                  stencilFormat );
            psd.depthAttachmentPixelFormat = depthFormat;
            psd.stencilAttachmentPixelFormat = stencilFormat;
        }

        NSError *error = NULL;
        id<MTLRenderPipelineState> pso =
            [mActiveDevice->mDevice newRenderPipelineStateWithDescriptor:psd error:&error];

        if( !pso || error )
        {
            String errorDesc;
            if( error )
                errorDesc = error.localizedDescription.UTF8String;

            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "Failed to created pipeline state for rendering, error " + errorDesc,
                         "MetalRenderSystem::_hlmsPipelineStateObjectCreated" );
        }

        id<MTLDepthStencilState> depthStencilState = getDepthStencilState( newPso );

        MetalHlmsPso *metalPso = new MetalHlmsPso();
        metalPso->pso = pso;
        metalPso->depthStencilState = depthStencilState;
        metalPso->vertexShader = vertexShader;
        metalPso->pixelShader = pixelShader;

        switch( newPso->macroblock->mCullMode )
        {
        case CULL_NONE:
            metalPso->cullMode = MTLCullModeNone;
            break;
        case CULL_CLOCKWISE:
            metalPso->cullMode = MTLCullModeBack;
            break;
        case CULL_ANTICLOCKWISE:
            metalPso->cullMode = MTLCullModeFront;
            break;
        }

        newPso->rsData = metalPso;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsPipelineStateObjectDestroyed( HlmsPso *pso )
    {
        assert( pso->rsData );

        removeDepthStencilState( pso );

        MetalHlmsPso *metalPso = reinterpret_cast<MetalHlmsPso *>( pso->rsData );
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
        samplerDescriptor.maxAnisotropy = (NSUInteger)newBlock->mMaxAnisotropy;
        samplerDescriptor.sAddressMode = MetalMappings::get( newBlock->mU );
        samplerDescriptor.tAddressMode = MetalMappings::get( newBlock->mV );
        samplerDescriptor.rAddressMode = MetalMappings::get( newBlock->mW );
        samplerDescriptor.normalizedCoordinates = YES;
        samplerDescriptor.lodMinClamp = newBlock->mMinLod;
        samplerDescriptor.lodMaxClamp = newBlock->mMaxLod;

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

        id<MTLSamplerState> sampler =
            [mActiveDevice->mDevice newSamplerStateWithDescriptor:samplerDescriptor];

        newBlock->mRsData = const_cast<void *>( CFBridgingRetain( sampler ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_hlmsSamplerblockDestroyed( HlmsSamplerblock *block )
    {
        if( block->mRsData )  // holds id<MTLSamplerState>
            CFRelease( block->mRsData );
        block->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    template <typename TDescriptorSetTexture, typename TTexSlot, typename TBufferPacked, bool isUav>
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
        typename FastArray<TTexSlot>::const_iterator endt = texContainer.end();

        while( itor != endt )
        {
            if( shaderTypeTexCount )
            {
                // We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader && numProcessedSlots >= shaderTypeTexCount[shaderType] )
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
                const TextureTypes::TextureTypes texType = texSlot.texture->getTextureType();
                if( texSlot.needsDifferentView() ||
                    ( isUav &&
                      ( texType == TextureTypes::TypeCube || texType == TextureTypes::TypeCubeArray ) ) )
                {
                    ++metalSet->numTextureViews;
                }
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

        // Create two contiguous arrays of texture and buffers, but we'll split
        // it into regions as a buffer could be in the middle of two textures.
        __unsafe_unretained id<MTLTexture> *textures = 0;
        __unsafe_unretained id<MTLBuffer> *buffers = 0;
        NSUInteger *offsets = 0;

        if( metalSet->numTextureViews > 0 )
        {
            // Create a third array to hold the strong reference
            // to the reinterpreted versions of textures.
            // Must be init to 0 before ARC sees it.
            void *textureViews = OGRE_MALLOC_SIMD(
                sizeof( id<MTLTexture> * ) * metalSet->numTextureViews, MEMCATEGORY_RENDERSYS );
            memset( textureViews, 0, sizeof( id<MTLTexture> * ) * metalSet->numTextureViews );
            metalSet->textureViews = (__strong id<MTLTexture> *)textureViews;
        }
        if( numTextures > 0 )
        {
            textures = (__unsafe_unretained id<MTLTexture> *)OGRE_MALLOC_SIMD(
                sizeof( id<MTLTexture> * ) * numTextures, MEMCATEGORY_RENDERSYS );
        }
        if( numBuffers > 0 )
        {
            buffers = (__unsafe_unretained id<MTLBuffer> *)OGRE_MALLOC_SIMD(
                sizeof( id<MTLBuffer> * ) * numBuffers, MEMCATEGORY_RENDERSYS );
            offsets = (NSUInteger *)OGRE_MALLOC_SIMD( sizeof( NSUInteger ) * numBuffers,
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
        endt = texContainer.end();

        while( itor != endt )
        {
            if( shaderTypeTexCount )
            {
                // We need to break the ranges if we crossed stages
                while( shaderType <= PixelShader && numProcessedSlots >= shaderTypeTexCount[shaderType] )
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
                    texRegion.range.location = NSUInteger( itor - texContainer.begin() );
                    texRegion.range.length = 0;
                    needsNewTexRange = false;
                }

                const typename TDescriptorSetTexture::TextureSlot &texSlot = itor->getTexture();

                assert( dynamic_cast<MetalTextureGpu *>( texSlot.texture ) );
                MetalTextureGpu *metalTex = static_cast<MetalTextureGpu *>( texSlot.texture );
                __unsafe_unretained id<MTLTexture> textureHandle = metalTex->getDisplayTextureName();

                const TextureTypes::TextureTypes texType = texSlot.texture->getTextureType();
                if( texSlot.needsDifferentView() ||
                    ( isUav &&
                      ( texType == TextureTypes::TypeCube || texType == TextureTypes::TypeCubeArray ) ) )
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
                    bufferRegion.range.location = NSUInteger( itor - texContainer.begin() );
                    bufferRegion.range.length = 0;
                    needsNewBufferRange = false;
                }

                const typename TDescriptorSetTexture::BufferSlot &bufferSlot = itor->getBuffer();

                assert( dynamic_cast<TBufferPacked *>( bufferSlot.buffer ) );
                TBufferPacked *metalBuf = static_cast<TBufferPacked *>( bufferSlot.buffer );

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
        for( size_t i = 0; i < numTextureViews; ++i )
            metalSet->textureViews[i] = 0;  // Let ARC free these pointers

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
        _descriptorSetTextureCreated<DescriptorSetTexture2, DescriptorSetTexture2::Slot,
                                     MetalTexBufferPacked, false>( newSet, newSet->mTextures,
                                                                   newSet->mShaderTypeTexCount );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetTexture2Destroyed( DescriptorSetTexture2 *set )
    {
        assert( set->mRsData );

        MetalDescriptorSetTexture *metalSet =
            reinterpret_cast<MetalDescriptorSetTexture *>( set->mRsData );

        destroyMetalDescriptorSetTexture( metalSet );
        delete metalSet;

        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetUavCreated( DescriptorSetUav *newSet )
    {
        _descriptorSetTextureCreated<DescriptorSetUav, DescriptorSetUav::Slot, MetalUavBufferPacked,
                                     true>( newSet, newSet->mUavs, 0 );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_descriptorSetUavDestroyed( DescriptorSetUav *set )
    {
        assert( set->mRsData );

        MetalDescriptorSetTexture *metalSet =
            reinterpret_cast<MetalDescriptorSetTexture *>( set->mRsData );

        destroyMetalDescriptorSetTexture( metalSet );
        delete metalSet;

        set->mRsData = 0;
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setHlmsSamplerblock( uint8 texUnit, const HlmsSamplerblock *samplerblock )
    {
        assert( ( !samplerblock || samplerblock->mRsData ) &&
                "The block must have been created via HlmsManager::getSamplerblock!" );

        if( !samplerblock )
        {
            [mActiveRenderEncoder setFragmentSamplerState:0 atIndex:texUnit];
        }
        else
        {
            __unsafe_unretained id<MTLSamplerState> sampler =
                (__bridge id<MTLSamplerState>)samplerblock->mRsData;
            [mActiveRenderEncoder setVertexSamplerState:sampler atIndex:texUnit];
            [mActiveRenderEncoder setFragmentSamplerState:sampler atIndex:texUnit];
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setPipelineStateObject( const HlmsPso *pso )
    {
        MetalHlmsPso *metalPso = reinterpret_cast<MetalHlmsPso *>( pso->rsData );

        if( pso && !mActiveRenderEncoder )
        {
            assert( mInterruptedRenderCommandEncoder &&
                    "mActiveRenderEncoder can only be null at this stage if rendering was interrupted."
                    " Did you call executeRenderPassDescriptorDelayedActions?" );
            executeRenderPassDescriptorDelayedActions( false );
        }

        if( !mPso || mPso->depthStencilState != metalPso->depthStencilState )
            [mActiveRenderEncoder setDepthStencilState:metalPso->depthStencilState];

        const float biasSign = mReverseDepth ? 1.0f : -1.0f;
        [mActiveRenderEncoder setDepthBias:pso->macroblock->mDepthBiasConstant * biasSign
                                slopeScale:pso->macroblock->mDepthBiasSlopeScale * biasSign
                                     clamp:0.0f];
        [mActiveRenderEncoder setCullMode:metalPso->cullMode];
        if( @available( iOS 11.0, * ) )
        {
            if( mCurrentCapabilities->hasCapability( RSC_DEPTH_CLAMP ) )
            {
                [mActiveRenderEncoder setDepthClipMode:pso->macroblock->mDepthClamp
                                                           ? MTLDepthClipModeClamp
                                                           : MTLDepthClipModeClip];
            }
        }

        if( mPso != metalPso )
        {
            [mActiveRenderEncoder setRenderPipelineState:metalPso->pso];
            mPso = metalPso;
        }

        MTLTriangleFillMode fillMode = ( pso->macroblock->mPolygonMode == PM_SOLID )
                                           ? MTLTriangleFillModeFill
                                           : MTLTriangleFillModeLines;
        [mActiveRenderEncoder setTriangleFillMode:fillMode];
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setComputePso( const HlmsComputePso *pso )
    {
        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        if( mComputePso != pso )
        {
            if( pso )
            {
                __unsafe_unretained id<MTLComputePipelineState> metalPso =
                    (__bridge id<MTLComputePipelineState>)pso->rsData;
                [computeEncoder setComputePipelineState:metalPso];
            }
            mComputePso = pso;
        }
    }
    //-------------------------------------------------------------------------
    VertexElementType MetalRenderSystem::getColourVertexElementType() const
    {
        return VET_COLOUR_ABGR;  // MTLVertexFormatUChar4Normalized
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_dispatch( const HlmsComputePso &pso )
    {
        __unsafe_unretained id<MTLComputeCommandEncoder> computeEncoder =
            mActiveDevice->getComputeEncoder();

        MTLSize numThreadGroups =
            MTLSizeMake( pso.mNumThreadGroups[0], pso.mNumThreadGroups[1], pso.mNumThreadGroups[2] );
        MTLSize threadsPerGroup =
            MTLSizeMake( pso.mThreadsPerGroup[0], pso.mThreadsPerGroup[1], pso.mThreadsPerGroup[2] );

        [computeEncoder dispatchThreadgroups:numThreadGroups threadsPerThreadgroup:threadsPerGroup];
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_setVertexArrayObject( const VertexArrayObject *vao )
    {
        __unsafe_unretained id<MTLBuffer> metalVertexBuffers[15];
        NSUInteger offsets[15];
        memset( offsets, 0, sizeof( offsets ) );

        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

        size_t numVertexBuffers = 0;
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator endt = vertexBuffers.end();

        while( itor != endt )
        {
            MetalBufferInterface *bufferInterface =
                static_cast<MetalBufferInterface *>( ( *itor )->getBufferInterface() );
            metalVertexBuffers[numVertexBuffers++] = bufferInterface->getVboName();
            ++itor;
        }

#if OGRE_DEBUG_MODE
        assert( numVertexBuffers < 15u );
#endif

        [mActiveRenderEncoder setVertexBuffers:metalVertexBuffers
                                       offsets:offsets
                                     withRange:NSMakeRange( 0, numVertexBuffers )];
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const CbDrawCallIndexed *cmd )
    {
        const VertexArrayObject *vao = cmd->vao;

        const MTLIndexType indexType = static_cast<MTLIndexType>( vao->mIndexBuffer->getIndexType() );
        const MTLPrimitiveType primType =
            std::min( MTLPrimitiveTypeTriangleStrip,
                      static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

        MetalBufferInterface *indexBufferInterface =
            static_cast<MetalBufferInterface *>( vao->mIndexBuffer->getBufferInterface() );
        __unsafe_unretained id<MTLBuffer> indexBuffer = indexBufferInterface->getVboName();

        size_t indirectBufferOffset = (size_t)cmd->indirectBufferOffset;
        for( uint32 i = cmd->numDraws; i; i-- )
        {
            [mActiveRenderEncoder drawIndexedPrimitives:primType
                                              indexType:indexType
                                            indexBuffer:indexBuffer
                                      indexBufferOffset:0
                                         indirectBuffer:mIndirectBuffer
                                   indirectBufferOffset:indirectBufferOffset];

            indirectBufferOffset +=
                sizeof( CbDrawIndexed );  // MTLDrawIndexedPrimitivesIndirectArguments
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const CbDrawCallStrip *cmd )
    {
        const VertexArrayObject *vao = cmd->vao;

        const MTLPrimitiveType primType =
            std::min( MTLPrimitiveTypeTriangleStrip,
                      static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

        size_t indirectBufferOffset = (size_t)cmd->indirectBufferOffset;
        for( uint32 i = cmd->numDraws; i; i-- )
        {
            [mActiveRenderEncoder drawPrimitives:primType
                                  indirectBuffer:mIndirectBuffer
                            indirectBufferOffset:indirectBufferOffset];

            indirectBufferOffset += sizeof( CbDrawStrip );  // MTLDrawPrimitivesIndirectArguments
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_renderEmulated( const CbDrawCallIndexed *cmd )
    {
        const VertexArrayObject *vao = cmd->vao;
        CbDrawIndexed *drawCmd = reinterpret_cast<CbDrawIndexed *>( mSwIndirectBufferPtr +
                                                                    (size_t)cmd->indirectBufferOffset );

        const MTLIndexType indexType = static_cast<MTLIndexType>( vao->mIndexBuffer->getIndexType() );
        const MTLPrimitiveType primType =
            std::min( MTLPrimitiveTypeTriangleStrip,
                      static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        // Calculate bytesPerVertexBuffer & numVertexBuffers which is the same for all draws in this cmd
        uint32 bytesPerVertexBuffer[15];
        size_t numVertexBuffers = 0;
        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
        VertexBufferPackedVec::const_iterator itor = vertexBuffers.begin();
        VertexBufferPackedVec::const_iterator endt = vertexBuffers.end();

        while( itor != endt )
        {
            bytesPerVertexBuffer[numVertexBuffers] = ( *itor )->getBytesPerElement();
            ++numVertexBuffers;
            ++itor;
        }
#endif

        // Get index buffer stuff which is the same for all draws in this cmd
        const size_t bytesPerIndexElement = vao->mIndexBuffer->getBytesPerElement();

        MetalBufferInterface *indexBufferInterface =
            static_cast<MetalBufferInterface *>( vao->mIndexBuffer->getBufferInterface() );
        __unsafe_unretained id<MTLBuffer> indexBuffer = indexBufferInterface->getVboName();

        for( uint32 i = cmd->numDraws; i--; )
        {
#if OGRE_DEBUG_MODE
            assert( ( ( drawCmd->firstVertexIndex * bytesPerIndexElement ) & 0x03 ) == 0 &&
                    "Index Buffer must be aligned to 4 bytes. If you're messing with "
                    "VertexArrayObject::setPrimitiveRange, you've entered an invalid "
                    "primStart; not supported by the Metal API." );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            for( size_t j = 0; j < numVertexBuffers; ++j )
            {
                // Manually set vertex buffer offsets since in iOS baseVertex is not supported
                // until iOS GPU Family 3 v1 & OS X (we just use indirect rendering there).
                [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseVertex * bytesPerVertexBuffer[j]
                                                    atIndex:j];
            }

            // Setup baseInstance.
#    if TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * 4u atIndex:15];
#    else
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * 256u atIndex:15];
#    endif

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
        CbDrawStrip *drawCmd =
            reinterpret_cast<CbDrawStrip *>( mSwIndirectBufferPtr + (size_t)cmd->indirectBufferOffset );

        const MTLPrimitiveType primType =
            std::min( MTLPrimitiveTypeTriangleStrip,
                      static_cast<MTLPrimitiveType>( vao->getOperationType() - 1u ) );

        //        const size_t numVertexBuffers = vertexBuffers.size();
        //        for( size_t j=0; j<numVertexBuffers; ++j )
        //        {
        //            //baseVertex is not needed as vertexStart does the same job.
        //            [mActiveRenderEncoder setVertexBufferOffset:0 atIndex:j];
        //        }

        for( uint32 i = cmd->numDraws; i--; )
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
            // Setup baseInstance.
#    if TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * 4u atIndex:15];
#    else
            [mActiveRenderEncoder setVertexBufferOffset:drawCmd->baseInstance * 256u atIndex:15];
#    endif
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
        memset( offsets, 0, sizeof( offsets ) );

        size_t maxUsedSlot = 0;
        const v1::VertexBufferBinding::VertexBufferBindingMap &binds =
            cmd->vertexData->vertexBufferBinding->getBindings();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator itor = binds.begin();
        v1::VertexBufferBinding::VertexBufferBindingMap::const_iterator endt = binds.end();

        while( itor != endt )
        {
            v1::MetalHardwareVertexBuffer *metalBuffer =
                static_cast<v1::MetalHardwareVertexBuffer *>( itor->second.get() );

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

        [mActiveRenderEncoder setVertexBuffers:metalVertexBuffers
                                       offsets:offsets
                                     withRange:NSMakeRange( 0, maxUsedSlot )];

        mCurrentIndexBuffer = cmd->indexData;
        mCurrentVertexBuffer = cmd->vertexData;
        mCurrentPrimType = std::min( MTLPrimitiveTypeTriangleStrip,
                                     static_cast<MTLPrimitiveType>( cmd->operationType - 1u ) );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const v1::CbDrawCallIndexed *cmd )
    {
        const MTLIndexType indexType =
            static_cast<MTLIndexType>( mCurrentIndexBuffer->indexBuffer->getType() );

        // Get index buffer stuff which is the same for all draws in this cmd
        const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

        size_t offsetStart;
        v1::MetalHardwareIndexBuffer *metalBuffer =
            static_cast<v1::MetalHardwareIndexBuffer *>( mCurrentIndexBuffer->indexBuffer.get() );
        __unsafe_unretained id<MTLBuffer> indexBuffer = metalBuffer->getBufferName( offsetStart );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    if OGRE_DEBUG_MODE
        assert( ( ( cmd->firstVertexIndex * bytesPerIndexElement ) & 0x03 ) == 0 &&
                "Index Buffer must be aligned to 4 bytes. If you're messing with "
                "IndexBuffer::indexStart, you've entered an invalid "
                "indexStart; not supported by the Metal API." );
#    endif

        // Setup baseInstance.
#    if TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * 4u atIndex:15];
#    else
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * 256u atIndex:15];
#    endif

        [mActiveRenderEncoder
            drawIndexedPrimitives:mCurrentPrimType
                       indexCount:cmd->primCount
                        indexType:indexType
                      indexBuffer:indexBuffer
                indexBufferOffset:cmd->firstVertexIndex * bytesPerIndexElement + offsetStart
                    instanceCount:cmd->instanceCount];
#else
        [mActiveRenderEncoder
            drawIndexedPrimitives:mCurrentPrimType
                       indexCount:cmd->primCount
                        indexType:indexType
                      indexBuffer:indexBuffer
                indexBufferOffset:cmd->firstVertexIndex * bytesPerIndexElement + offsetStart
                    instanceCount:cmd->instanceCount
                       baseVertex:(NSInteger)mCurrentVertexBuffer->vertexStart
                     baseInstance:cmd->baseInstance];
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::_render( const v1::CbDrawCallStrip *cmd )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        // Setup baseInstance.
#    if TARGET_OS_SIMULATOR == 0 || OGRE_CPU == OGRE_CPU_ARM
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * 4u atIndex:15];
#    else
        [mActiveRenderEncoder setVertexBufferOffset:cmd->baseInstance * 256u atIndex:15];
#    endif
        [mActiveRenderEncoder
            drawPrimitives:mCurrentPrimType
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
        RenderSystem::_render( op );

        const size_t numberOfInstances = op.numberOfInstances;

        // Render to screen!
        if( op.useIndexes )
        {
            do
            {
                // Update derived depth bias.
                if( mDerivedDepthBias && mCurrentPassIterationNum > 0 )
                {
                    const float biasSign = mReverseDepth ? 1.0f : -1.0f;
                    [mActiveRenderEncoder
                        setDepthBias:( mDerivedDepthBiasBase +
                                       mDerivedDepthBiasMultiplier * mCurrentPassIterationNum ) *
                                     biasSign
                          slopeScale:mDerivedDepthBiasSlopeScale * biasSign
                               clamp:0.0f];
                }

                const MTLIndexType indexType =
                    static_cast<MTLIndexType>( mCurrentIndexBuffer->indexBuffer->getType() );

                // Get index buffer stuff which is the same for all draws in this cmd
                const size_t bytesPerIndexElement = mCurrentIndexBuffer->indexBuffer->getIndexSize();

                size_t offsetStart;
                v1::MetalHardwareIndexBuffer *metalBuffer = static_cast<v1::MetalHardwareIndexBuffer *>(
                    mCurrentIndexBuffer->indexBuffer.get() );
                __unsafe_unretained id<MTLBuffer> indexBuffer =
                    metalBuffer->getBufferName( offsetStart );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
#    if OGRE_DEBUG_MODE
                assert( ( ( mCurrentIndexBuffer->indexStart * bytesPerIndexElement ) & 0x03 ) == 0 &&
                        "Index Buffer must be aligned to 4 bytes. If you're messing with "
                        "IndexBuffer::indexStart, you've entered an invalid "
                        "indexStart; not supported by the Metal API." );
#    endif

                [mActiveRenderEncoder
                    drawIndexedPrimitives:mCurrentPrimType
                               indexCount:mCurrentIndexBuffer->indexCount
                                indexType:indexType
                              indexBuffer:indexBuffer
                        indexBufferOffset:mCurrentIndexBuffer->indexStart * bytesPerIndexElement +
                                          offsetStart
                            instanceCount:numberOfInstances];
#else
                [mActiveRenderEncoder
                    drawIndexedPrimitives:mCurrentPrimType
                               indexCount:mCurrentIndexBuffer->indexCount
                                indexType:indexType
                              indexBuffer:indexBuffer
                        indexBufferOffset:mCurrentIndexBuffer->indexStart * bytesPerIndexElement +
                                          offsetStart
                            instanceCount:numberOfInstances
                               baseVertex:(NSInteger)mCurrentVertexBuffer->vertexStart
                             baseInstance:0];
#endif
            } while( updatePassIterationRenderState() );
        }
        else
        {
            do
            {
                // Update derived depth bias.
                if( mDerivedDepthBias && mCurrentPassIterationNum > 0 )
                {
                    const float biasSign = mReverseDepth ? 1.0f : -1.0f;
                    [mActiveRenderEncoder
                        setDepthBias:( mDerivedDepthBiasBase +
                                       mDerivedDepthBiasMultiplier * mCurrentPassIterationNum ) *
                                     biasSign
                          slopeScale:mDerivedDepthBiasSlopeScale * biasSign
                               clamp:0.0f];
                }

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
                const uint32 vertexStart = 0;
#else
                const uint32 vertexStart = static_cast<uint32>( mCurrentVertexBuffer->vertexStart );
#endif

                if( numberOfInstances > 1 )
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
            } while( updatePassIterationRenderState() );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::bindGpuProgramParameters( GpuProgramType gptype,
                                                      GpuProgramParametersSharedPtr params,
                                                      uint16 variabilityMask )
    {
        MetalProgram *shader = 0;
        switch( gptype )
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
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "Geometry Shaders are not supported in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_HULL_PROGRAM:
            mActiveTessellationHullGpuProgramParameters = params;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "Tessellation is different in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_DOMAIN_PROGRAM:
            mActiveTessellationDomainGpuProgramParameters = params;
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED, "Tessellation is different in Metal.",
                         "MetalRenderSystem::bindGpuProgramParameters" );
            break;
        case GPT_COMPUTE_PROGRAM:
            mActiveComputeGpuProgramParameters = params;
            shader = static_cast<MetalProgram *>( mComputePso->computeShader->_getBindingDelegate() );
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
                    ConstBufferPacked *constBuffer =
                        mVaoManager->createConstBuffer( std::max<size_t>( 512u * 1024u, bytesToWrite ),
                                                        BT_DYNAMIC_PERSISTENT, 0, false );
                    mAutoParamsBuffer.push_back( constBuffer );
                }

                ConstBufferPacked *constBuffer = mAutoParamsBuffer[mAutoParamsBufferIdx];
                mCurrentAutoParamsBufferPtr =
                    reinterpret_cast<uint8 *>( constBuffer->map( 0, constBuffer->getNumElements() ) );
                mCurrentAutoParamsBufferSpaceLeft = constBuffer->getTotalSizeBytes();

                ++mAutoParamsBufferIdx;
            }

            shader->updateBuffers( params, mCurrentAutoParamsBufferPtr );

            assert(
                dynamic_cast<MetalConstBufferPacked *>( mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] ) );

            MetalConstBufferPacked *constBuffer =
                static_cast<MetalConstBufferPacked *>( mAutoParamsBuffer[mAutoParamsBufferIdx - 1u] );
            const uint32 bindOffset =
                uint32( constBuffer->getTotalSizeBytes() - mCurrentAutoParamsBufferSpaceLeft );
            switch( gptype )
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
            mCurrentAutoParamsBufferPtr = reinterpret_cast<uint8 *>( alignToNextMultiple<uintptr_t>(
                reinterpret_cast<uintptr_t>( mCurrentAutoParamsBufferPtr ),
                mVaoManager->getConstBufferAlignment() ) );
            bytesToWrite += size_t( mCurrentAutoParamsBufferPtr - oldBufferPos );

            // We know that bytesToWrite <= mCurrentAutoParamsBufferSpaceLeft, but that was
            // before padding. After padding this may no longer hold true.
            mCurrentAutoParamsBufferSpaceLeft -=
                std::min( mCurrentAutoParamsBufferSpaceLeft, bytesToWrite );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::bindGpuProgramPassIterationParameters( GpuProgramType gptype ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::clearFrameBuffer( RenderPassDescriptor *renderPassDesc,
                                              TextureGpu *anyTarget, uint8 mipLevel )
    {
        Vector4 fullVp( 0, 0, 1, 1 );
        beginRenderPassDescriptor( renderPassDesc, anyTarget, mipLevel, &fullVp, &fullVp, 1u, false,
                                   false );
        executeRenderPassDescriptorDelayedActions();
    }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getHorizontalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getVerticalTexelOffset() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getMinimumDepthInputValue() { return 0.0f; }
    //-------------------------------------------------------------------------
    Real MetalRenderSystem::getMaximumDepthInputValue() { return 1.0f; }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::preExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::postExtraThreadsStarted() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::registerThread() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::unregisterThread() {}
    //-------------------------------------------------------------------------
    const PixelFormatToShaderType *MetalRenderSystem::getPixelFormatToShaderType() const
    {
        return &mPixelFormatToShaderType;
    }

    //-------------------------------------------------------------------------
    void MetalRenderSystem::initGPUProfiling()
    {
#if OGRE_PROFILING == OGRE_PROFILING_REMOTERY
//        _rmt_BindMetal( mActiveDevice->mCurrentCommandBuffer );
#endif
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::deinitGPUProfiling()
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
    void MetalRenderSystem::beginProfileEvent( const String &eventName ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::endProfileEvent() {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::markProfileEvent( const String &event ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setClipPlanesImpl( const PlaneList &clipPlanes ) {}
    //-------------------------------------------------------------------------
    void MetalRenderSystem::initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                                    Window *primary )
    {
        selectDepthBufferFormat( DepthBuffer::DFM_D32 | DepthBuffer::DFM_D24 | DepthBuffer::DFM_D16 |
                                 DepthBuffer::DFM_S8 );
        mShaderManager = OGRE_NEW MetalGpuProgramManager( &mDevice );
        mMetalProgramFactory = new MetalProgramFactory( &mDevice );
        HighLevelGpuProgramManager::getSingleton().addFactory( mMetalProgramFactory );
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::updateCompositorManager( CompositorManager2 *compositorManager )
    {
        // Metal requires that a frame's worth of rendering be invoked inside an autorelease pool.
        // This is true for both iOS and macOS.
        @autoreleasepool
        {
            compositorManager->_updateImplementation();
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::compositorWorkspaceBegin( CompositorWorkspace *workspace,
                                                      const bool forceBeginFrame )
    {
        @autoreleasepool
        {
            RenderSystem::compositorWorkspaceBegin( workspace, forceBeginFrame );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::compositorWorkspaceUpdate( CompositorWorkspace *workspace )
    {
        @autoreleasepool
        {
            RenderSystem::compositorWorkspaceUpdate( workspace );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::compositorWorkspaceEnd( CompositorWorkspace *workspace,
                                                    const bool forceEndFrame )
    {
        @autoreleasepool
        {
            RenderSystem::compositorWorkspaceEnd( workspace, forceEndFrame );
        }
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::flushCommands()
    {
        endRenderPassDescriptor( false );
        mActiveDevice->commitAndNextCommandBuffer();
    }
    //-------------------------------------------------------------------------
    void MetalRenderSystem::setStencilBufferParams( uint32 refValue, const StencilParams &stencilParams )
    {
        RenderSystem::setStencilBufferParams( refValue, stencilParams );

        // There are two main cases:
        // 1. The active render encoder is valid and will be subsequently used for drawing.
        //      We need to set the stencil reference value on this encoder. We do this below.
        // 2. The active render is invalid or is about to go away.
        //      In this case, we need to set the stencil reference value on the new encoder when it is
        //      created (see createRenderEncoder). (In this case, the setStencilReferenceValue below in
        //      this wasted, but it is inexpensive).

        // Save this info so we can transfer it into a new encoder if necessary
        mStencilEnabled = stencilParams.enabled;
        if( mStencilEnabled )
        {
            mStencilRefValue = refValue;

            if( mActiveRenderEncoder )
                [mActiveRenderEncoder setStencilReferenceValue:refValue];
        }
    }
}
