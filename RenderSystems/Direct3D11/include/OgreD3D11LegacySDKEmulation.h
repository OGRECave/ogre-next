/**
This file aims to allow compiling when using the DX June 2010 SDK by
forward declaring several structs, and defining several classes where
we need its functionality.

HOWEVER IT IS NOT INTENDED TO BE USED AS A REPLACEMENT.
Code should detect whether the Windows 8 SDK or newer is present by doing
"#if defined( _WIN32_WINNT_WIN8 )" and avoid creating a DX 11.1 device
or use Feature Level 11.1

This header only allows compiling the code so that fallbacks to 11.0 and earlier
work without having #ifdefs all over the place; while the C++ code can determine
at runtime whether to use DX11.1 and DXGI 1.2 or not.

Do NOT attempt to use a D3D11.1 device (or DXGI 1.2+ device) using the
DX June 2010 SDK with this header.
*/

#ifndef _OgreD3D11LegacySDKEmulation_H_
#define _OgreD3D11LegacySDKEmulation_H_

struct ID3D11DeviceContext1;
struct ID3D11Device1;
struct ID3D11BlendState1;
struct ID3D11RasterizerState1;
struct D3D11_BLEND_DESC1;
struct D3D11_RASTERIZER_DESC1;

#define DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ((DXGI_SWAP_EFFECT)(3))
#define DXGI_SWAP_EFFECT_FLIP_DISCARD ((DXGI_SWAP_EFFECT)(4))

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
    float r;
    float g;
    float b;
    float a;
} D3DCOLORVALUE;
#define D3DCOLORVALUE_DEFINED
#endif
typedef D3DCOLORVALUE DXGI_RGBA;
typedef enum DXGI_ALPHA_MODE {
    DXGI_ALPHA_MODE_UNSPECIFIED = 0,
    DXGI_ALPHA_MODE_PREMULTIPLIED = 1,
    DXGI_ALPHA_MODE_STRAIGHT = 2,
    DXGI_ALPHA_MODE_IGNORE = 3,
    DXGI_ALPHA_MODE_FORCE_DWORD = 0xffffffff
} DXGI_ALPHA_MODE;
typedef enum DXGI_SCALING {
    DXGI_SCALING_STRETCH = 0,
    DXGI_SCALING_NONE = 1
} DXGI_SCALING;
typedef struct DXGI_SWAP_CHAIN_DESC1 {
    UINT Width;
    UINT Height;
    DXGI_FORMAT Format;
    int Stereo;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    DXGI_SCALING Scaling;
    DXGI_SWAP_EFFECT SwapEffect;
    DXGI_ALPHA_MODE AlphaMode;
    UINT Flags;
} DXGI_SWAP_CHAIN_DESC1;
typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC {
    DXGI_RATIONAL RefreshRate;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
    int Windowed;
} DXGI_SWAP_CHAIN_FULLSCREEN_DESC;
typedef struct DXGI_PRESENT_PARAMETERS {
    UINT DirtyRectsCount;
    RECT *pDirtyRects;
    RECT *pScrollRect;
    POINT *pScrollOffset;
} DXGI_PRESENT_PARAMETERS;

struct ID3DDeviceContextState;
struct IDXGISwapChain1;

DEFINE_GUID(IID_ID3D11Device1, 0xa04bfb29, 0x08ef, 0x43d6, 0xa4,0x9c, 0xa9,0xbd,0xbd,0xcb,0xe6,0x86);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("a04bfb29-08ef-43d6-a49c-a9bdbdcbe686")
ID3D11Device1 : public ID3D11Device
{
    virtual void STDMETHODCALLTYPE GetImmediateContext1(
        ID3D11DeviceContext1 **ppImmediateContext) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext1(
        UINT ContextFlags,
        ID3D11DeviceContext1 **ppDeferredContext) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateBlendState1(
        const D3D11_BLEND_DESC1 *pBlendStateDesc,
        ID3D11BlendState1 **ppBlendState) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState1(
        const D3D11_RASTERIZER_DESC1 *pRasterizerDesc,
        ID3D11RasterizerState1 **ppRasterizerState) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateDeviceContextState(
        UINT Flags,
        const D3D_FEATURE_LEVEL *pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        REFIID EmulatedInterface,
        D3D_FEATURE_LEVEL *pChosenFeatureLevel,
        ID3DDeviceContextState **ppContextState) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResource1(
        HANDLE hResource,
        REFIID returnedInterface,
        void **ppResource) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenSharedResourceByName(
        LPCWSTR lpName,
        DWORD dwDesiredAccess,
        REFIID returnedInterface,
        void **ppResource) = 0;

};
#endif

DEFINE_GUID(IID_ID3D11DeviceContext1, 0xbb2c6faa, 0xb5fb, 0x4082, 0x8e,0x6b, 0x38,0x8b,0x8c,0xfa,0x90,0xe1);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("bb2c6faa-b5fb-4082-8e6b-388b8cfa90e1")
ID3D11DeviceContext1 : public ID3D11DeviceContext
{
    virtual void STDMETHODCALLTYPE CopySubresourceRegion1(
        ID3D11Resource *pDstResource,
        UINT DstSubresource,
        UINT DstX,
        UINT DstY,
        UINT DstZ,
        ID3D11Resource *pSrcResource,
        UINT SrcSubresource,
        const D3D11_BOX *pSrcBox,
        UINT CopyFlags) = 0;

    virtual void STDMETHODCALLTYPE UpdateSubresource1(
        ID3D11Resource *pDstResource,
        UINT DstSubresource,
        const D3D11_BOX *pDstBox,
        const void *pSrcData,
        UINT SrcRowPitch,
        UINT SrcDepthPitch,
        UINT CopyFlags) = 0;

    virtual void STDMETHODCALLTYPE DiscardResource(
        ID3D11Resource *pResource) = 0;

    virtual void STDMETHODCALLTYPE DiscardView(
        ID3D11View *pResourceView) = 0;

    virtual void STDMETHODCALLTYPE VSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE HSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE DSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE GSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE PSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE CSSetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers,
        const UINT *pFirstConstant,
        const UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE VSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE HSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE DSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE GSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE PSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE CSGetConstantBuffers1(
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers,
        UINT *pFirstConstant,
        UINT *pNumConstants) = 0;

    virtual void STDMETHODCALLTYPE SwapDeviceContextState(
        ID3DDeviceContextState *pState,
        ID3DDeviceContextState **ppPreviousState) = 0;

    virtual void STDMETHODCALLTYPE ClearView(
        ID3D11View *pView,
        const FLOAT Color[4],
        const D3D11_RECT *pRect,
        UINT NumRects) = 0;

    virtual void STDMETHODCALLTYPE DiscardView1(
        ID3D11View *pResourceView,
        const D3D11_RECT *pRects,
        UINT NumRects) = 0;

};
#endif

DEFINE_GUID(IID_IDXGIFactory2, 0x50c83a1c, 0xe072, 0x4c48, 0x87,0xb0, 0x36,0x30,0xfa,0x36,0xa6,0xd0);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("50c83a1c-e072-4c48-87b0-3630fa36a6d0")
IDXGIFactory2 : public IDXGIFactory1
{
    virtual int STDMETHODCALLTYPE IsWindowedStereoEnabled(
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
        IUnknown *pDevice,
        HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC1 *pDesc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
        IDXGIOutput *pRestrictToOutput,
        IDXGISwapChain1 **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(
        IUnknown *pDevice,
        IUnknown *pWindow,
        const DXGI_SWAP_CHAIN_DESC1 *pDesc,
        IDXGIOutput *pRestrictToOutput,
        IDXGISwapChain1 **ppSwapChain) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(
        HANDLE hResource,
        LUID *pLuid) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(
        HWND WindowHandle,
        UINT wMsg,
        DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(
        HANDLE hEvent,
        DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterStereoStatus(
        DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(
        HWND WindowHandle,
        UINT wMsg,
        DWORD *pdwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(
        HANDLE hEvent,
        DWORD *pdwCookie) = 0;

    virtual void STDMETHODCALLTYPE UnregisterOcclusionStatus(
        DWORD dwCookie) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(
        IUnknown *pDevice,
        const DXGI_SWAP_CHAIN_DESC1 *pDesc,
        IDXGIOutput *pRestrictToOutput,
        IDXGISwapChain1 **ppSwapChain) = 0;

};
#endif

DEFINE_GUID(IID_IDXGISwapChain1, 0x790a45f7, 0x0d42, 0x4876, 0x98,0x3a, 0x0a,0x55,0xcf,0xe6,0xf4,0xaa);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("790a45f7-0d42-4876-983a-0a55cfe6f4aa")
IDXGISwapChain1 : public IDXGISwapChain
{
    virtual HRESULT STDMETHODCALLTYPE GetDesc1(
        DXGI_SWAP_CHAIN_DESC1 *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetHwnd(
        HWND *pHwnd) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(
        REFIID refiid,
        void **ppUnk) = 0;

    virtual HRESULT STDMETHODCALLTYPE Present1(
        UINT SyncInterval,
        UINT PresentFlags,
        const DXGI_PRESENT_PARAMETERS *pPresentParameters) = 0;

    virtual int STDMETHODCALLTYPE IsTemporaryMonoSupported(
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(
        IDXGIOutput **ppRestrictToOutput) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(
        const DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(
        DXGI_RGBA *pColor) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRotation(
        DXGI_MODE_ROTATION Rotation) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRotation(
        DXGI_MODE_ROTATION *pRotation) = 0;

};
#endif

#endif
