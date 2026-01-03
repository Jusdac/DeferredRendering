//***************************************************************************************
// DeferredRenderer.cpp by ZSD
//***************************************************************************************

#include "DeferredRenderer.h"
#include <DirectXPackedVector.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

DeferredRenderer::DeferredRenderer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    UINT width, UINT height)

{
    md3dDevice = device;

    OnResize(width, height);
}

UINT DeferredRenderer::DeferredRendererMapWidth()const
{
    return mRenderTargetWidth / 2;
}

UINT DeferredRenderer::DeferredRendererMapHeight()const
{
    return mRenderTargetHeight / 2;
}


ID3D12Resource* DeferredRenderer::PositionMap()
{
    return mPositionMap.Get();
}
ID3D12Resource* DeferredRenderer::AlbedoMap()
{
    return mAlbedoMap.Get();
}
ID3D12Resource* DeferredRenderer::MaterialMap()
{
    return mMaterialMap.Get();
}
ID3D12Resource* DeferredRenderer::NormalMap()
{
    return mNormalMap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DeferredRenderer::NormalMapRtv()const
{
    return mhNormalMapCpuRtv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DeferredRenderer::NormalMapSrv()const
{
    return mhNormalMapGpuSrv;
}

void DeferredRenderer::BuildDescriptors(
    ID3D12Resource* depthStencilBuffer,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
    CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
    UINT cbvSrvUavDescriptorSize,
    UINT rtvDescriptorSize)
{
    // Save references to the descriptors.  The DeferredRenderer reserves heap space
    // for 5 contiguous Srvs.

    mhPositionMapCpuSrv = hCpuSrv;
    mhNormalMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhAlbedoMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhMaterialMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    mhPositionMapGpuSrv = hGpuSrv;
    mhNormalMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhAlbedoMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    mhMaterialMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    mhPositionMapCpuRtv = hCpuRtv;
    mhNormalMapCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
    mhAlbedoMapCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
    mhMaterialMapCpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

    //  Create the descriptors
    RebuildDescriptors(depthStencilBuffer);

}


void DeferredRenderer::RebuildDescriptors(ID3D12Resource* depthStencilBuffer)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = NormalMapFormat;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    md3dDevice->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mhNormalMapCpuSrv);

    srvDesc.Format = PositionMapFormat;
    md3dDevice->CreateShaderResourceView(mPositionMap.Get(), &srvDesc, mhPositionMapCpuSrv);

    srvDesc.Format = AlbedoMapFormat;
    md3dDevice->CreateShaderResourceView(mAlbedoMap.Get(), &srvDesc, mhAlbedoMapCpuSrv);

    srvDesc.Format = MaterialMapFormat;
    md3dDevice->CreateShaderResourceView(mMaterialMap.Get(), &srvDesc, mhMaterialMapCpuSrv);
  
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = NormalMapFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    md3dDevice->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mhNormalMapCpuRtv);

    rtvDesc.Format = PositionMapFormat;
    md3dDevice->CreateRenderTargetView(mPositionMap.Get(), &rtvDesc, mhPositionMapCpuRtv);

    rtvDesc.Format = AlbedoMapFormat;
    md3dDevice->CreateRenderTargetView(mAlbedoMap.Get(), &rtvDesc, mhAlbedoMapCpuRtv);

    rtvDesc.Format = MaterialMapFormat;
    md3dDevice->CreateRenderTargetView(mMaterialMap.Get(), &rtvDesc, mhMaterialMapCpuRtv);

}

void DeferredRenderer::SetPSOs(ID3D12PipelineState* DeferredRendererPso, ID3D12PipelineState* DeferredRendererBlurPso)
{
    mDeferredRendererPso = DeferredRendererPso;
}

void DeferredRenderer::OnResize(UINT newWidth, UINT newHeight)
{
    if (mRenderTargetWidth != newWidth || mRenderTargetHeight != newHeight)
    {
        mRenderTargetWidth = newWidth;
        mRenderTargetHeight = newHeight;

        // We render to ambient map at half the resolution.
        mViewport.TopLeftX = 0.0f;
        mViewport.TopLeftY = 0.0f;
        mViewport.Width = mRenderTargetWidth / 2.0f;
        mViewport.Height = mRenderTargetHeight / 2.0f;
        mViewport.MinDepth = 0.0f;
        mViewport.MaxDepth = 1.0f;

        mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };

        BuildResources();
    }
}

void DeferredRenderer::ComputeDeferredRenderer(
    ID3D12GraphicsCommandList* cmdList,
    FrameResource* currFrame,
    int blurCount)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);

    // We compute the initial DeferredRenderer to AmbientMap0.

    // Change to RENDER_TARGET.
    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
     //   D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

    float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
   // cmdList->ClearRenderTargetView(mhAmbientMap0CpuRtv, clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    //cmdList->OMSetRenderTargets(1, &mhAmbientMap0CpuRtv, true, nullptr);

    // Bind the constant buffer for this pass.
   // auto DeferredRendererCBAddress = currFrame->DeferredRendererCB->Resource()->GetGPUVirtualAddress();
    //cmdList->SetGraphicsRootConstantBufferView(0, DeferredRendererCBAddress);
    cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    // Bind the normal and depth maps.
    cmdList->SetGraphicsRootDescriptorTable(2, mhNormalMapGpuSrv);

    // Bind the random vector map.
   // cmdList->SetGraphicsRootDescriptorTable(3, mhRandomVectorMapGpuSrv);

    cmdList->SetPipelineState(mDeferredRendererPso);

    // Draw fullscreen quad.
    cmdList->IASetVertexBuffers(0, 0, nullptr);
    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(6, 1, 0, 0);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
   //     D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void DeferredRenderer::BuildResources()
{
    // Free the old resources if they exist.
    mPositionMap = nullptr;
    mNormalMap = nullptr; // 你已有
    mAlbedoMap = nullptr;
    mMaterialMap = nullptr;

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mRenderTargetWidth;
    texDesc.Height = mRenderTargetHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DeferredRenderer::NormalMapFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    // --- 创建 Position Map ---
    texDesc.Format = PositionMapFormat; // 或 R16G16B16A16_FLOAT
    float positionClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 世界坐标不会是 (0,0,0)
    CD3DX12_CLEAR_VALUE optClearPosition(texDesc.Format, positionClearColor);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON, // 初始状态为渲染目标
        &optClearPosition,
        IID_PPV_ARGS(&mPositionMap)));

    // --- 创建 Normal Map (沿用你现有的逻辑，但注意格式) ---
    texDesc.Format = NormalMapFormat; // 或 R32G32B32A32_FLOAT
    float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f }; // 默认法线指向 Z 轴正方向
    CD3DX12_CLEAR_VALUE optClearNormal(texDesc.Format, normalClearColor);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON, // 
        &optClearNormal,
        IID_PPV_ARGS(&mNormalMap)));

    // --- 创建 Albedo Map ---
    texDesc.Format = AlbedoMapFormat; // 或 R16G16B16A16_FLOAT
    float albedoClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 默认黑色 (如果需要)
    CD3DX12_CLEAR_VALUE optClearAlbedo(texDesc.Format, albedoClearColor);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON, // 
        &optClearAlbedo,
        IID_PPV_ARGS(&mAlbedoMap)));

    // --- 创建 Material Map ---
    texDesc.Format = MaterialMapFormat; // 或 R16G16B16A16_FLOAT
    float materialClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f }; // 默认值取决于你的编码方式
    CD3DX12_CLEAR_VALUE optClearMaterial(texDesc.Format, materialClearColor);
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON, // 初始状态为渲染目标
        &optClearMaterial,
        IID_PPV_ARGS(&mMaterialMap)));
}
