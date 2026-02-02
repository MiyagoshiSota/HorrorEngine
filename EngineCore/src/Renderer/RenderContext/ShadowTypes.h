#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

/// シャドウ描画方式（ランタイムで切り替え可能）
enum class ShadowMode
{
    None = 0,             /// 影なし（比較用）
    RasterDepth = 1,      /// ラスタライズ深度比較（従来のシャドウマップ）
    RayTracedMask = 2,    /// レイトレ：ライト空間 R32 0/1 マスク
    RayTracedVisibility = 3  /// レイトレ：カメラ解像度 visibility テクスチャ（将来）
};

/// ライティングパスが参照するシャドウ関連データの契約
struct ShadowContext
{
    ShadowMode mode = ShadowMode::RasterDepth;
    DirectX::XMMATRIX mainLightViewProj = DirectX::XMMatrixIdentity();
    D3D12_GPU_DESCRIPTOR_HANDLE shadowSrv = { 0 };   /// t4: 深度 or R32 マスク
    D3D12_GPU_DESCRIPTOR_HANDLE visibilitySrv = { 0 }; /// t5: カメラ解像度 visibility（RayTracedVisibility 時）
};
