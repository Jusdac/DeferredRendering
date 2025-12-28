//***************************************************************************************
// DeferredRenderer.h by ZSD
//***************************************************************************************

#ifndef DeferredRenderer_H
#define DeferredRenderer_H

#pragma once

#include "Common/d3dUtil.h"
#include "FrameResource.h"


class DeferredRenderer
{
public:

    DeferredRenderer(ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        UINT width, UINT height);
    DeferredRenderer(const DeferredRenderer& rhs) = delete;
    DeferredRenderer& operator=(const DeferredRenderer& rhs) = delete;
    ~DeferredRenderer() = default;

    static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static const DXGI_FORMAT PositionMapFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    static const DXGI_FORMAT AlbedoMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static const DXGI_FORMAT MaterialMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    UINT DeferredRendererMapWidth()const;
    UINT DeferredRendererMapHeight()const;

    ID3D12Resource* NormalMap();
    ID3D12Resource* PositionMap();
    ID3D12Resource* AlbedoMap();
    ID3D12Resource* MaterialMap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv()const;

    CD3DX12_GPU_DESCRIPTOR_HANDLE PositionMapSrv()const { return mhPositionMapGpuSrv; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE PositionMapRtv()const { return mhPositionMapCpuRtv; }

    CD3DX12_GPU_DESCRIPTOR_HANDLE AlbedoMapSrv()const { return mhAlbedoMapGpuSrv; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE AlbedoMapRtv()const { return mhAlbedoMapCpuRtv; }

    CD3DX12_GPU_DESCRIPTOR_HANDLE MaterialMapSrv()const { return mhMaterialMapGpuSrv; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE MaterialMapRtv()const { return mhMaterialMapCpuRtv; }

    void BuildDescriptors(
        ID3D12Resource* depthStencilBuffer,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
        UINT cbvSrvUavDescriptorSize,
        UINT rtvDescriptorSize);

    void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);

    void SetPSOs(ID3D12PipelineState* DeferredRendererPso, ID3D12PipelineState* DeferredRendererBlurPso);


    void OnResize(UINT newWidth, UINT newHeight);

    void ComputeDeferredRenderer(
        ID3D12GraphicsCommandList* cmdList,
        FrameResource* currFrame,
        int blurCount);


private:

    void BuildResources();

private:
    ID3D12Device* md3dDevice;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> mDeferredRendererRootSig;

    ID3D12PipelineState* mDeferredRendererPso = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> mPositionMap;  // R32G32B32A32_FLOAT
    Microsoft::WRL::ComPtr<ID3D12Resource> mNormalMap;    // R16G16B16A16_SNORM
    Microsoft::WRL::ComPtr<ID3D12Resource> mAlbedoMap;    // R8G8B8A8_UNORM
    Microsoft::WRL::ComPtr<ID3D12Resource> mMaterialMap;  // R8G8B8A8_UNORM 或 R16G16B16A16_FLOAT

    // G-Buffer SRV 描述符 
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhPositionMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhPositionMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhAlbedoMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAlbedoMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhMaterialMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhMaterialMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhMaterialMapCpuRtv;

    UINT mRenderTargetWidth;
    UINT mRenderTargetHeight;


    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
};

#endif // DeferredRenderer_H#pragma once
