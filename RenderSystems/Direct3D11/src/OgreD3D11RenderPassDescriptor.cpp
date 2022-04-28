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

#include "OgreD3D11RenderPassDescriptor.h"

#include "OgreD3D11Mappings.h"
#include "OgreD3D11RenderSystem.h"
#include "OgreD3D11TextureGpu.h"
#include "OgreD3D11Window.h"
#include "OgreHlmsDatablock.h"
#include "OgrePixelFormatGpuUtils.h"

namespace Ogre
{
    D3D11RenderPassDescriptor::D3D11RenderPassDescriptor( D3D11Device &device,
                                                          D3D11RenderSystem *renderSystem ) :
        mHasStencilFormat( false ),
        mHasRenderWindow( false ),
        mSharedFboItor( renderSystem->_getFrameBufferDescMap().end() ),
        mDevice( device ),
        mRenderSystem( renderSystem )
    {
    }
    //-----------------------------------------------------------------------------------
    D3D11RenderPassDescriptor::~D3D11RenderPassDescriptor()
    {
        D3D11FrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
            mSharedFboItor = frameBufferDescMap.end();
        }

        if( mHasRenderWindow )
            mRenderSystem->removeListener( this );
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::notifyDeviceLost( D3D11Device *device )
    {
        for( size_t i = 0; i < mNumColourEntries; ++i )
            mColourRtv[i].Reset();
        mDepthStencilRtv.Reset();
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::notifyDeviceRestored( D3D11Device *device, unsigned pass )
    {
        if( pass == 1 )
            entriesModified( RenderPassDescriptor::All );
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::checkRenderWindowStatus()
    {
        if( ( mNumColourEntries > 0 && mColour[0].texture->isRenderWindowSpecific() ) ||
            ( mDepth.texture && mDepth.texture->isRenderWindowSpecific() ) ||
            ( mStencil.texture && mStencil.texture->isRenderWindowSpecific() ) )
        {
            if( mNumColourEntries > 1u )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "D3D11RenderPassDescriptor::colourEntriesModified" );
            }

            if( ( mNumColourEntries > 0 && !mColour[0].texture->isRenderWindowSpecific() ) ||
                ( mDepth.texture && !mDepth.texture->isRenderWindowSpecific() ) ||
                ( mStencil.texture && !mStencil.texture->isRenderWindowSpecific() ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot mix RenderWindow colour texture with depth or stencil buffer "
                             "that aren't for RenderWindows, or vice-versa",
                             "D3D11RenderPassDescriptor::checkRenderWindowStatus" );
            }

            if( !mHasRenderWindow )
            {
                mHasRenderWindow = true;
                // Listen for swapchain resizes
                mRenderSystem->addListener( this );
            }
        }
        else
        {
            if( mHasRenderWindow )
            {
                mRenderSystem->removeListener( this );
                mHasRenderWindow = false;
            }
        }

        calculateSharedKey();
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::calculateSharedKey()
    {
        FrameBufferDescKey key( *this );
        D3D11FrameBufferDescMap &frameBufferDescMap = mRenderSystem->_getFrameBufferDescMap();
        D3D11FrameBufferDescMap::iterator newItor = frameBufferDescMap.find( key );

        if( newItor == frameBufferDescMap.end() )
        {
            D3D11FrameBufferDescValue value;
            value.refCount = 0;
            frameBufferDescMap[key] = value;
            newItor = frameBufferDescMap.find( key );
        }

        ++newItor->second.refCount;

        if( mSharedFboItor != frameBufferDescMap.end() )
        {
            --mSharedFboItor->second.refCount;
            if( !mSharedFboItor->second.refCount )
                frameBufferDescMap.erase( mSharedFboItor );
        }

        mSharedFboItor = newItor;
    }
    //-----------------------------------------------------------------------------------
    template <typename T>
    void D3D11RenderPassDescriptor::setSliceToRtvDesc( T &inOutRtvDesc,
                                                       const RenderPassColourTarget &target )
    {
        if( target.allLayers )
        {
            inOutRtvDesc.FirstArraySlice = 0;
            inOutRtvDesc.ArraySize = target.texture->getNumSlices();
        }
        else
        {
            inOutRtvDesc.FirstArraySlice = target.slice;
            inOutRtvDesc.ArraySize = 1u;
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::updateColourRtv( uint8 lastNumColourEntries )
    {
        if( mNumColourEntries < lastNumColourEntries )
        {
            for( size_t i = mNumColourEntries; i < lastNumColourEntries; ++i )
            {
                // Detach removed colour entries
                mColourRtv[i].Reset();
            }
        }

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            mColourRtv[i].Reset();

            if( mColour[i].texture->getResidencyStatus() != GpuResidency::Resident )
            {
                OGRE_EXCEPT(
                    Exception::ERR_INVALIDPARAMS,
                    "RenderTexture '" + mColour[i].texture->getNameStr() + "' must be resident!",
                    "D3D11RenderPassDescriptor::updateColourRtv" );
            }
            if( mHasRenderWindow != mColour[i].texture->isRenderWindowSpecific() )
            {
                // This is a GL restriction actually, which we mimic for consistency
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "Cannot use RenderWindow as MRT with other colour textures",
                             "D3D11RenderPassDescriptor::updateColourRtv" );
            }

            if( mColour[i].texture->getPixelFormat() == PFG_NULL )
                continue;

            D3D11_RENDER_TARGET_VIEW_DESC viewDesc;
            memset( &viewDesc, 0, sizeof( viewDesc ) );
            viewDesc.Format = D3D11Mappings::get( mColour[i].texture->getPixelFormat() );

            if( mColour[i].texture->isMultisample() )
            {
                if( !mColour[i].texture->hasMsaaExplicitResolves() ||
                    mColour[i].texture->getTextureType() != TextureTypes::Type2DArray )
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
                else if( mColour[i].texture->getTextureType() == TextureTypes::Type2DArray )
                {
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    setSliceToRtvDesc( viewDesc.Texture2DMSArray, mColour[i] );
                }
            }
            else
            {
                switch( mColour[i].texture->getTextureType() )
                {
                case TextureTypes::Type1D:
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    viewDesc.Texture1D.MipSlice = mColour[i].mipLevel;
                    break;
                case TextureTypes::Type1DArray:
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    viewDesc.Texture1DArray.MipSlice = mColour[i].mipLevel;
                    setSliceToRtvDesc( viewDesc.Texture1DArray, mColour[i] );
                    break;
                case TextureTypes::Unknown:
                case TextureTypes::Type2D:
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    viewDesc.Texture2D.MipSlice = mColour[i].mipLevel;
                    break;
                case TextureTypes::Type2DArray:
                case TextureTypes::TypeCube:
                case TextureTypes::TypeCubeArray:
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    viewDesc.Texture2DArray.MipSlice = mColour[i].mipLevel;
                    setSliceToRtvDesc( viewDesc.Texture2DArray, mColour[i] );
                    break;
                case TextureTypes::Type3D:
                    viewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                    viewDesc.Texture3D.MipSlice = mColour[i].mipLevel;
                    if( mColour[i].allLayers )
                    {
                        viewDesc.Texture3D.FirstWSlice = 0;
                        viewDesc.Texture3D.WSize = mColour[i].texture->getDepth() >> mColour[i].mipLevel;
                    }
                    else
                    {
                        viewDesc.Texture3D.FirstWSlice = mColour[i].slice;
                        viewDesc.Texture3D.WSize = 1u;
                    }
                    break;
                }
            }

            assert( dynamic_cast<D3D11TextureGpu *>( mColour[i].texture ) );
            D3D11TextureGpu *textureD3d = static_cast<D3D11TextureGpu *>( mColour[i].texture );

            ID3D11Resource *resourceTex = 0;
            if( mColour[i].texture->isMultisample() && !mColour[i].texture->hasMsaaExplicitResolves() )
            {
                resourceTex = textureD3d->getMsaaFramebufferName();
            }
            else
            {
                resourceTex = textureD3d->getFinalTextureName();
            }

            HRESULT hr = mDevice->CreateRenderTargetView( resourceTex, &viewDesc,
                                                          mColourRtv[i].ReleaseAndGetAddressOf() );

            if( FAILED( hr ) || mDevice.isError() )
            {
                String errorDescription = mDevice.getErrorDescription( hr );
                OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                                "Error creating Render Target View for texture '" +
                                    mColour[i].texture->getNameStr() +
                                    "'\nError Description:" + errorDescription,
                                "D3D11RenderPassDescriptor::updateColourRtv" );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::updateDepthRtv()
    {
        mDepthStencilRtv.Reset();
        mHasStencilFormat = false;

        if( !mDepth.texture )
        {
            if( mStencil.texture )
            {
                OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                             "Stencil without depth (RenderTexture '" + mStencil.texture->getNameStr() +
                                 "'). This is not supported by D3D11",
                             "D3D11RenderPassDescriptor::updateDepthRtv" );
            }

            return;
        }

        if( mDepth.texture->getResidencyStatus() != GpuResidency::Resident )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "RenderTexture '" + mDepth.texture->getNameStr() + "' must be resident!",
                         "D3D11RenderPassDescriptor::updateDepthRtv" );
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        memset( &depthStencilDesc, 0, sizeof( depthStencilDesc ) );

        depthStencilDesc.Format = D3D11Mappings::get( mDepth.texture->getPixelFormat() );
        if( mDepth.texture->isMultisample() )
            depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        else
            depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        if( mDepth.readOnly )
            depthStencilDesc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;
        if( mStencil.readOnly )
            depthStencilDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;

        assert( dynamic_cast<D3D11TextureGpu *>( mDepth.texture ) );
        D3D11TextureGpu *texture = static_cast<D3D11TextureGpu *>( mDepth.texture );

        HRESULT hr = mDevice->CreateDepthStencilView( texture->getFinalTextureName(), &depthStencilDesc,
                                                      mDepthStencilRtv.ReleaseAndGetAddressOf() );
        if( FAILED( hr ) )
        {
            mDepthStencilRtv.Reset();
            String errorDescription = mDevice.getErrorDescription( hr );
            OGRE_EXCEPT_EX( Exception::ERR_RENDERINGAPI_ERROR, hr,
                            "Unable to create depth stencil view for texture '" +
                                mDepth.texture->getNameStr() + "\nError Description:" + errorDescription,
                            "D3D11RenderSystem::updateDepthRtv" );
        }

        mHasStencilFormat = hasStencilFormat();
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::entriesModified( uint32 entryTypes )
    {
        uint8 lastNumColourEntries = mNumColourEntries;
        RenderPassDescriptor::entriesModified( entryTypes );

        checkRenderWindowStatus();

        if( entryTypes & RenderPassDescriptor::Colour )
            updateColourRtv( lastNumColourEntries );

        if( entryTypes & ( RenderPassDescriptor::Depth | RenderPassDescriptor::Stencil ) )
            updateDepthRtv();
    }
    //-----------------------------------------------------------------------------------
    uint32 D3D11RenderPassDescriptor::checkForClearActions( D3D11RenderPassDescriptor *other ) const
    {
        uint32 entriesToFlush = 0;

        assert( this->mSharedFboItor == other->mSharedFboItor );
        assert( this->mNumColourEntries == other->mNumColourEntries );

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        for( size_t i = 0; i < mNumColourEntries; ++i )
        {
            // this->mColour[i].allLayers doesn't need to be analyzed
            // because it requires a different FBO.
            if( other->mColour[i].loadAction == LoadAction::Clear ||
                ( isTiler && mColour[i].loadAction == LoadAction::ClearOnTilers ) )
            {
                entriesToFlush |= RenderPassDescriptor::Colour0 << i;
            }
        }

        if( other->mDepth.loadAction == LoadAction::Clear ||
            ( isTiler && mDepth.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Depth;
        }

        if( other->mStencil.loadAction == LoadAction::Clear ||
            ( isTiler && mStencil.loadAction == LoadAction::ClearOnTilers ) )
        {
            entriesToFlush |= RenderPassDescriptor::Stencil;
        }

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    uint32 D3D11RenderPassDescriptor::willSwitchTo( D3D11RenderPassDescriptor *newDesc,
                                                    bool warnIfRtvWasFlushed ) const
    {
        uint32 entriesToFlush = 0;

        if( !newDesc || this->mSharedFboItor != newDesc->mSharedFboItor || this->mInformationOnly ||
            newDesc->mInformationOnly )
        {
            entriesToFlush = RenderPassDescriptor::All;
        }
        else
            entriesToFlush |= checkForClearActions( newDesc );

        if( warnIfRtvWasFlushed )
            newDesc->checkWarnIfRtvWasFlushed( entriesToFlush );

        return entriesToFlush;
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::performLoadActions( Viewport *viewport, uint32 entriesToFlush,
                                                        uint32 uavStartingSlot,
                                                        const DescriptorSetUav *descSetUav )
    {
        if( mInformationOnly )
            return;

        // D3D11.1 doesn't allow ClearViews on stencil, and DiscardView on depth & stencil separately.
        // That means if the depth buffer has stencil we have to behave like non-tiler.

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ID3D11DeviceContext1 *context1 = mDevice.GetImmediateContext1();

        const RenderSystemCapabilities *capabilities = mRenderSystem->getCapabilities();
        const bool isTiler = capabilities->hasCapability( RSC_IS_TILER );

        if( entriesToFlush & RenderPassDescriptor::Colour )
        {
            for( size_t i = 0; i < mNumColourEntries; ++i )
            {
                if( entriesToFlush & ( RenderPassDescriptor::Colour0 << i ) && mColourRtv[i] )
                {
                    if( mColour[i].loadAction == LoadAction::Clear ||
                        ( isTiler && mColour[i].loadAction == LoadAction::ClearOnTilers ) )
                    {
                        FLOAT clearColour[4];
                        clearColour[0] = mColour[i].clearColour.r;
                        clearColour[1] = mColour[i].clearColour.g;
                        clearColour[2] = mColour[i].clearColour.b;
                        clearColour[3] = mColour[i].clearColour.a;
                        context->ClearRenderTargetView( mColourRtv[i].Get(), clearColour );
                    }
                    else if( mColour[i].loadAction == LoadAction::DontCare )
                    {
                        if( context1 )
                            context1->DiscardView( mColourRtv[i].Get() );
                    }
                }
            }
        }

        if( mDepthStencilRtv &&
            ( entriesToFlush & ( RenderPassDescriptor::Depth | RenderPassDescriptor::Stencil ) ) )
        {
            // Cannot discard depth & stencil if we're being asked to keep stencil around
            if( mDepth.loadAction == LoadAction::DontCare &&
                ( mStencil.loadAction == LoadAction::DontCare ||
                  mStencil.loadAction == LoadAction::Clear || !mHasStencilFormat ) )
            {
                if( context1 )
                    context1->DiscardView( mDepthStencilRtv.Get() );
            }

            if( mDepth.loadAction == LoadAction::Clear || mStencil.loadAction == LoadAction::Clear )
            {
                UINT flags = 0;
                if( mDepth.loadAction == LoadAction::Clear )
                    flags |= D3D11_CLEAR_DEPTH;
                if( mStencil.loadAction == LoadAction::Clear )
                    flags |= D3D11_CLEAR_STENCIL;

                float clearDepthValue;
                if( !mRenderSystem->isReverseDepth() )
                    clearDepthValue = mDepth.clearDepth;
                else
                    clearDepthValue = 1.0f - mDepth.clearDepth;
                context->ClearDepthStencilView( mDepthStencilRtv.Get(), flags, clearDepthValue,
                                                mStencil.clearStencil );
            }
        }

        if( !descSetUav )
            context->OMSetRenderTargets( mNumColourEntries, mColourRtv[0].GetAddressOf(),
                                         mDepthStencilRtv.Get() );
        else
        {
            const UINT numUavs = static_cast<UINT>( descSetUav->mUavs.size() );
            ComPtr<ID3D11UnorderedAccessView> *uavList =
                reinterpret_cast<ComPtr<ID3D11UnorderedAccessView> *>( descSetUav->mRsData );
            context->OMSetRenderTargetsAndUnorderedAccessViews(
                mNumColourEntries, mColourRtv[0].GetAddressOf(), mDepthStencilRtv.Get(), uavStartingSlot,
                numUavs, uavList[0].GetAddressOf(), 0 );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::performStoreActions( uint32 entriesToFlush )
    {
        if( mInformationOnly )
            return;

        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        ID3D11DeviceContext1 *context1 = mDevice.GetImmediateContext1();

        if( entriesToFlush & RenderPassDescriptor::Colour )
        {
            for( size_t i = 0; i < mNumColourEntries; ++i )
            {
                if( entriesToFlush & ( RenderPassDescriptor::Colour0 << i ) && mColourRtv[i] )
                {
                    if( ( mColour[i].storeAction == StoreAction::MultisampleResolve ||
                          mColour[i].storeAction == StoreAction::StoreAndMultisampleResolve ) &&
                        mColour[i].resolveTexture )
                    {
                        assert( mColour[i].resolveTexture->getResidencyStatus() ==
                                GpuResidency::Resident );
                        assert( dynamic_cast<D3D11TextureGpu *>( mColour[i].resolveTexture ) );
                        D3D11TextureGpu *resolveTexture =
                            static_cast<D3D11TextureGpu *>( mColour[i].resolveTexture );
                        D3D11TextureGpu *texture = static_cast<D3D11TextureGpu *>( mColour[i].texture );

                        UINT dstSubResource =
                            D3D11CalcSubresource( mColour[i].resolveMipLevel, mColour[i].resolveSlice,
                                                  resolveTexture->getDepthOrSlices() );
                        UINT srcSubResource = D3D11CalcSubresource(
                            mColour[i].mipLevel, mColour[i].slice, texture->getDepthOrSlices() );
                        DXGI_FORMAT format = D3D11Mappings::get( resolveTexture->getPixelFormat() );

                        ID3D11Resource *srcRes = texture->hasMsaaExplicitResolves()
                                                     ? texture->getFinalTextureName()
                                                     : texture->getMsaaFramebufferName();

                        context->ResolveSubresource( resolveTexture->getFinalTextureName(),
                                                     dstSubResource, srcRes, srcSubResource, format );
                    }

                    if( mColour[i].storeAction == StoreAction::DontCare ||
                        mColour[i].storeAction == StoreAction::MultisampleResolve )
                    {
                        if( context1 )
                            context1->DiscardView( mColourRtv[i].Get() );
                    }
                }
            }
        }

        if( ( entriesToFlush & ( RenderPassDescriptor::Depth | RenderPassDescriptor::Stencil ) ) &&
            mDepth.storeAction == StoreAction::DontCare &&
            ( mStencil.storeAction == StoreAction::DontCare || !mHasStencilFormat ) && mDepthStencilRtv )
        {
            if( context1 )
                context1->DiscardView( mDepthStencilRtv.Get() );
        }

        // Prevent the runtime from thinking we might be sampling from the render target
        context->OMSetRenderTargets( 0, 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::clearFrameBuffer()
    {
        ID3D11DeviceContextN *context = mDevice.GetImmediateContext();
        const size_t numColourEntries = mNumColourEntries;
        for( size_t i = 0; i < numColourEntries; ++i )
        {
            if( mColour[i].loadAction == LoadAction::Clear )
            {
                FLOAT clearValue[4];
                clearValue[0] = mColour[i].clearColour.r;
                clearValue[1] = mColour[i].clearColour.g;
                clearValue[2] = mColour[i].clearColour.b;
                clearValue[3] = mColour[i].clearColour.a;
                context->ClearRenderTargetView( mColourRtv[i].Get(), clearValue );
            }
        }

        if( mDepthStencilRtv &&
            ( mDepth.loadAction == LoadAction::Clear || mStencil.loadAction == LoadAction::Clear ) )
        {
            UINT flags = 0;
            if( mDepth.loadAction == LoadAction::Clear )
                flags |= D3D11_CLEAR_DEPTH;
            if( mStencil.loadAction == LoadAction::Clear )
                flags |= D3D11_CLEAR_STENCIL;

            float clearDepthValue;
            if( !mRenderSystem->isReverseDepth() )
                clearDepthValue = mDepth.clearDepth;
            else
                clearDepthValue = 1.0f - mDepth.clearDepth;
            context->ClearDepthStencilView( mDepthStencilRtv.Get(), flags, clearDepthValue,
                                            mStencil.clearStencil );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::getCustomAttribute( IdString name, void *pData, uint32 extraParam )
    {
        if( name == "ID3D11RenderTargetView" )
        {
            if( extraParam >= OGRE_MAX_MULTIPLE_RENDER_TARGETS )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "extraParams out of bounds for ID3D11RenderTargetView",
                             "D3D11RenderPassDescriptor::getCustomAttribute" );
            }

            ID3D11RenderTargetView **outRtv = (ID3D11RenderTargetView **)pData;
            *outRtv = mColourRtv[extraParam].Get();
        }
        else if( name == "ID3D11DepthStencilView" )
        {
            ID3D11DepthStencilView **outDsv = (ID3D11DepthStencilView **)pData;
            *outDsv = mDepthStencilRtv.Get();
        }
        else
        {
            RenderPassDescriptor::getCustomAttribute( name, pData, extraParam );
        }
    }
    //-----------------------------------------------------------------------------------
    void D3D11RenderPassDescriptor::eventOccurred( const String &eventName,
                                                   const NameValuePairList *parameters )
    {
        if( !parameters )
            return;

        if( eventName != "WindowBeforeResize" && eventName != "WindowResized" )
            return;

        NameValuePairList::const_iterator itor = parameters->find( "Window" );

        if( itor == parameters->end() )
        {
            OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "Window pointer not passed on SwapChain resize",
                         "D3D11RenderPassDescriptor::eventOccurred" );
        }

        D3D11Window *window = (D3D11Window *)StringConverter::parseSizeT( itor->second );
        if( mNumColourEntries > 0 && mColour[0].texture == window->getTexture() )
        {
            if( eventName == "WindowBeforeResize" )
            {
                mColourRtv[0].Reset();
                mDepthStencilRtv.Reset();
            }
            else
            {
                entriesModified( RenderPassDescriptor::All );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    D3D11FrameBufferDescValue::D3D11FrameBufferDescValue() : refCount( 0 ) {}
}  // namespace Ogre
