#include "RayTracingPipelineState.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <atlbase.h>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

bool RayTracingPipelineState::Create(
    ID3D12Device5* device,
    const wchar_t* shaderLibraryPath,
    UINT maxPayloadSize,
    UINT maxAttributeSize,
    UINT maxRecursionDepth)
{
    if (!device || !shaderLibraryPath)
    {
        printf("[RayTracingPipelineState] エラー: デバイスまたはシェーダーパスがnullです\n");
        return false;
    }

    printf("[RayTracingPipelineState] シェーダーライブラリをロード中: %ls\n", shaderLibraryPath);

    // -------------------------------------------------------------------------
    // DXCによるシェーダーコンパイル
    // -------------------------------------------------------------------------
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ComPtr<IDxcIncludeHandler> includeHandler;

    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) return false;

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) return false;

    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) return false;

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = dxcUtils->LoadFile(shaderLibraryPath, nullptr, &sourceBlob);
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] エラー: シェーダーファイルの読み込み失敗\n");
        return false;
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-T");
    arguments.push_back(L"lib_6_3");
    // lib_6_3 では関数がデフォルトで export されないため、GetShaderIdentifier で名前解決するには外部リンケージが必要
    arguments.push_back(L"-default-linkage");
    arguments.push_back(L"external");
    arguments.push_back(L"-exports");
    arguments.push_back(L"ShadowRayGen;ShadowMiss;ShadowAnyHit;ShadowClosestHit");

#if defined(_DEBUG)
    arguments.push_back(DXC_ARG_DEBUG);
    arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> compileResult;
    hr = dxcCompiler->Compile(
        &sourceBuffer,
        arguments.data(),
        static_cast<UINT32>(arguments.size()),
        includeHandler.Get(),
        IID_PPV_ARGS(&compileResult)
    );

    if (FAILED(hr)) return false;

    HRESULT compileStatus;
    compileResult->GetStatus(&compileStatus);
    if (FAILED(compileStatus))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        compileResult->GetErrorBuffer(&errorBlob);
        if (errorBlob)
        {
            printf("[RayTracingPipelineState] コンパイルエラー:\n%s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    ComPtr<IDxcBlob> shaderBlob;
    compileResult->GetResult(&shaderBlob);

    // -------------------------------------------------------------------------
    // DXIL エクスポート名のダンプ（デバッグ用）
    // -------------------------------------------------------------------------
    {
        ComPtr<IDxcContainerReflection> containerReflection;
        hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection));
        if (SUCCEEDED(hr))
        {
            hr = containerReflection->Load(shaderBlob.Get());
            if (SUCCEEDED(hr))
            {
                UINT32 dxilPartIndex = 0;
                const UINT32 kDxilFourcc = 0x4C495844; // 'DXIL'
                hr = containerReflection->FindFirstPartKind(kDxilFourcc, &dxilPartIndex);
                if (SUCCEEDED(hr))
                {
                    ComPtr<ID3D12LibraryReflection> libraryReflection;
                    hr = containerReflection->GetPartReflection(dxilPartIndex, IID_PPV_ARGS(&libraryReflection));
                    if (SUCCEEDED(hr))
                    {
                        D3D12_LIBRARY_DESC libDesc = {};
                        hr = libraryReflection->GetDesc(&libDesc);
                        if (SUCCEEDED(hr))
                        {
                            printf("[RayTracingPipelineState] DXIL Export (%u functions):\n", libDesc.FunctionCount);
                            for (UINT i = 0; i < libDesc.FunctionCount; ++i)
                            {
                                ID3D12FunctionReflection* functionReflection = libraryReflection->GetFunctionByIndex(static_cast<INT>(i));
                                if (functionReflection)
                                {
                                    D3D12_FUNCTION_DESC funcDesc = {};
                                    hr = functionReflection->GetDesc(&funcDesc);
                                    if (SUCCEEDED(hr) && funcDesc.Name && funcDesc.Name[0] != '\0')
                                        printf("  [%u] %s\n", i, funcDesc.Name);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    //　State Object の構築準備
    // -------------------------------------------------------------------------
    
    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(16); 

    // エクスポート名の定義（CreateStateObject は HLSL の関数名＝アンマングル名を要求する）
    static const wchar_t* kRayGenExport = L"ShadowRayGen";
    static const wchar_t* kMissExport = L"ShadowMiss";
    static const wchar_t* kAnyHitExport = L"ShadowAnyHit";
    static const wchar_t* kClosestHitExport = L"ShadowClosestHit";
    static const wchar_t* kHitGroupExport = L"ShadowHitGroup";

    // --- (A) DXIL Library ---
    D3D12_EXPORT_DESC exports[4] = {};
    exports[0].Name = kRayGenExport;    exports[0].Flags = D3D12_EXPORT_FLAG_NONE;
    exports[1].Name = kMissExport;      exports[1].Flags = D3D12_EXPORT_FLAG_NONE;
    exports[2].Name = kAnyHitExport;    exports[2].Flags = D3D12_EXPORT_FLAG_NONE;
    exports[3].Name = kClosestHitExport; exports[3].Flags = D3D12_EXPORT_FLAG_NONE;

    D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
    dxilLibDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
    dxilLibDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
    dxilLibDesc.NumExports = _countof(exports);
    dxilLibDesc.pExports = exports;

    D3D12_STATE_SUBOBJECT dxilLibSubobject = {};
    dxilLibSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    dxilLibSubobject.pDesc = &dxilLibDesc;
    subobjects.push_back(dxilLibSubobject);

    // --- (B) Hit Group ---
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = kHitGroupExport;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.AnyHitShaderImport = kAnyHitExport;
    hitGroupDesc.ClosestHitShaderImport = kClosestHitExport;

    D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
    hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    hitGroupSubobject.pDesc = &hitGroupDesc;
    subobjects.push_back(hitGroupSubobject);

    // --- (C) Shader Config ---
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
    shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;

    D3D12_STATE_SUBOBJECT shaderConfigSubobject = {};
    shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    shaderConfigSubobject.pDesc = &shaderConfig;
    subobjects.push_back(shaderConfigSubobject);

    // Local Root Signature の関連付け用（CreateStateObject まで有効なスコープに置く）
    static const wchar_t* kLocalRootSigAssociationExports[] = { kRayGenExport, kHitGroupExport };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootAssociation = {};

    // --- (D) Local Root Signature (作成と関連付け) ---
    {
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(0, nullptr);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        ComPtr<ID3DBlob> signatureBlob, errorBlob;
        hr = D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) return false;

        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(m_localRootSignature.GetAddressOf()));
        if (FAILED(hr)) return false;

        D3D12_STATE_SUBOBJECT localRootSigSubobject = {};
        localRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        localRootSigSubobject.pDesc = m_localRootSignature.GetAddressOf();
        subobjects.push_back(localRootSigSubobject);

        localRootAssociation.pSubobjectToAssociate = &subobjects.back();
        localRootAssociation.NumExports = _countof(kLocalRootSigAssociationExports);
        localRootAssociation.pExports = kLocalRootSigAssociationExports;

        D3D12_STATE_SUBOBJECT associationSubobject = {};
        associationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        associationSubobject.pDesc = &localRootAssociation;
        subobjects.push_back(associationSubobject);
    }

    // --- (E) Global Root Signature ---
    // t0: TLAS, u0: Output, b0: SceneConstants
    CD3DX12_DESCRIPTOR_RANGE ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); 
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0); 

    CD3DX12_ROOT_PARAMETER rootParameters[3];
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0]);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1]);
    rootParameters[2].InitAsConstantBufferView(0);

    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> globalSigBlob, globalErrorBlob;
    hr = D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &globalSigBlob, &globalErrorBlob);
    if (FAILED(hr)) return false;

    hr = device->CreateRootSignature(0, globalSigBlob->GetBufferPointer(), globalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_globalRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;

    D3D12_STATE_SUBOBJECT globalRootSigSubobject = {};
    globalRootSigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    globalRootSigSubobject.pDesc = m_globalRootSignature.GetAddressOf();
    subobjects.push_back(globalRootSigSubobject);

    // --- (F) Pipeline Config ---
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = maxRecursionDepth;

    D3D12_STATE_SUBOBJECT pipelineConfigSubobject = {};
    pipelineConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    pipelineConfigSubobject.pDesc = &pipelineConfig;
    subobjects.push_back(pipelineConfigSubobject);

    // -------------------------------------------------------------------------
    // State Object 作成 (Final)
    // -------------------------------------------------------------------------
    printf("[RayTracingPipelineState] State Objectを作成中 (subobjects: %zu)\n", subobjects.size());

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
    stateObjectDesc.pSubobjects = subobjects.data();

    hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_stateObject.GetAddressOf()));

    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] エラー: State Objectの作成失敗 (HRESULT: 0x%08X)\n", hr);
        return false;
    }

    printf("[RayTracingPipelineState] State Object作成成功\n");

    // -------------------------------------------------------------------------
    // Properties取得 (Identifier取得用)
    // -------------------------------------------------------------------------
    hr = m_stateObject->QueryInterface(IID_PPV_ARGS(m_stateObjectProperties.GetAddressOf()));
    if (FAILED(hr)) return false;

    // 確認用: GetShaderIdentifier の戻り値と ID の全バイトをダンプ
    {
        const UINT idSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        void* idPtr = m_stateObjectProperties->GetShaderIdentifier(kRayGenExport);

        printf("[RayTracingPipelineState] GetShaderIdentifier(\"%ls\") => %s\n",
            kRayGenExport, idPtr ? "非null" : "null");

        if (idPtr)
        {
            const UINT8* bytes = static_cast<const UINT8*>(idPtr);
            printf("[RayTracingPipelineState] RayGen ID 全 %u バイト (hex):\n  ", idSize);
            for (UINT i = 0; i < idSize; ++i)
            {
                printf("%02X", bytes[i]);
                if ((i + 1) % 16 == 0 && i + 1 < idSize)
                    printf("\n  ");
                else if (i + 1 < idSize)
                    printf(" ");
            }
            printf("\n");
            UINT32 word0, word1;
            memcpy(&word0, idPtr, 4);
            memcpy(&word1, static_cast<const UINT8*>(idPtr) + 4, 4);
            printf("[RayTracingPipelineState] RayGen ID offset 0 (LE): 0x%08X, offset 4 (LE): 0x%08X\n", word0, word1);
        }
        else
        {
            printf("[RayTracingPipelineState] 警告: RayGen ID が取得できませんでした (Export名不一致?)\n");
        }
    }

    return true;
}

bool RayTracingPipelineState::CreateForRTAO(
    ID3D12Device5* device,
    const wchar_t* shaderLibraryPath,
    UINT maxPayloadSize,
    UINT maxAttributeSize)
{
    if (!device || !shaderLibraryPath)
    {
        printf("[RayTracingPipelineState] RTAO: デバイスまたはシェーダーパスがnullです\n");
        return false;
    }

    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) return false;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) return false;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) return false;

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = dxcUtils->LoadFile(shaderLibraryPath, nullptr, &sourceBlob);
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTAO: シェーダーファイルの読み込み失敗\n");
        return false;
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-T");
    arguments.push_back(L"lib_6_3");
    arguments.push_back(L"-default-linkage");
    arguments.push_back(L"external");
    arguments.push_back(L"-exports");
    arguments.push_back(L"RTAORayGen;RTAOMiss;RTAOClosestHit");
#if defined(_DEBUG)
    arguments.push_back(DXC_ARG_DEBUG);
    arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

    DxcBuffer sourceBuffer = { sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), 0 };
    ComPtr<IDxcResult> compileResult;
    hr = dxcCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()),
        includeHandler.Get(), IID_PPV_ARGS(&compileResult));
    if (FAILED(hr)) return false;
    HRESULT compileStatus;
    compileResult->GetStatus(&compileStatus);
    if (FAILED(compileStatus))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        compileResult->GetErrorBuffer(&errorBlob);
        if (errorBlob)
            printf("[RayTracingPipelineState] RTAO コンパイルエラー:\n%s\n", (char*)errorBlob->GetBufferPointer());
        return false;
    }
    ComPtr<IDxcBlob> shaderBlob;
    compileResult->GetResult(&shaderBlob);

    static const wchar_t* kRayGenExport = L"RTAORayGen";
    static const wchar_t* kMissExport = L"RTAOMiss";
    static const wchar_t* kClosestHitExport = L"RTAOClosestHit";
    static const wchar_t* kHitGroupExport = L"RTAOHitGroup";

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(16);

    D3D12_EXPORT_DESC exports[3] = {};
    exports[0].Name = kRayGenExport;
    exports[1].Name = kMissExport;
    exports[2].Name = kClosestHitExport;
    D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
    dxilLibDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
    dxilLibDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
    dxilLibDesc.NumExports = 3;
    dxilLibDesc.pExports = exports;
    D3D12_STATE_SUBOBJECT dxilLibSub = { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibDesc };
    subobjects.push_back(dxilLibSub);

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = kHitGroupExport;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = kClosestHitExport;
    D3D12_STATE_SUBOBJECT hitGroupSub = { D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc };
    subobjects.push_back(hitGroupSub);

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
    shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;
    D3D12_STATE_SUBOBJECT shaderConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig };
    subobjects.push_back(shaderConfigSub);

    CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(0, nullptr);
    localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    ComPtr<ID3DBlob> localSigBlob, localErrBlob;
    hr = D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &localSigBlob, &localErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, localSigBlob->GetBufferPointer(), localSigBlob->GetBufferSize(), IID_PPV_ARGS(m_localRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT localRootSub = { D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_localRootSignature.GetAddressOf() };
    subobjects.push_back(localRootSub);
    static const wchar_t* kLocalAssocExports[] = { kRayGenExport, kHitGroupExport };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localAssoc = {};
    localAssoc.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
    localAssoc.NumExports = 2;
    localAssoc.pExports = kLocalAssocExports;
    D3D12_STATE_SUBOBJECT localAssocSub = { D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localAssoc };
    subobjects.push_back(localAssocSub);

    CD3DX12_DESCRIPTOR_RANGE ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_ROOT_PARAMETER rootParams[5];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0]);
    rootParams[1].InitAsDescriptorTable(1, &ranges[1]);
    rootParams[2].InitAsDescriptorTable(1, &ranges[2]);
    rootParams[3].InitAsDescriptorTable(1, &ranges[3]);
    rootParams[4].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(5, rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> globalSigBlob, globalErrBlob;
    hr = D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &globalSigBlob, &globalErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, globalSigBlob->GetBufferPointer(), globalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_globalRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT globalRootSub = { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature.GetAddressOf() };
    subobjects.push_back(globalRootSub);

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 1;
    D3D12_STATE_SUBOBJECT pipelineConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig };
    subobjects.push_back(pipelineConfigSub);

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
    stateObjectDesc.pSubobjects = subobjects.data();
    hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTAO: State Objectの作成失敗\n");
        return false;
    }
    hr = m_stateObject->QueryInterface(IID_PPV_ARGS(m_stateObjectProperties.GetAddressOf()));
    if (FAILED(hr)) return false;
    printf("[RayTracingPipelineState] RTAO State Object作成成功\n");
    return true;
}

bool RayTracingPipelineState::CreateForRTGI(
    ID3D12Device5* device,
    const wchar_t* shaderLibraryPath,
    UINT maxPayloadSize,
    UINT maxAttributeSize)
{
    if (!device || !shaderLibraryPath)
    {
        printf("[RayTracingPipelineState] RTGI: デバイスまたはシェーダーパスがnullです\n");
        return false;
    }

    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) return false;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) return false;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) return false;

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = dxcUtils->LoadFile(shaderLibraryPath, nullptr, &sourceBlob);
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTGI: シェーダーファイルの読み込み失敗\n");
        return false;
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-T");
    arguments.push_back(L"lib_6_3");
    arguments.push_back(L"-default-linkage");
    arguments.push_back(L"external");
    arguments.push_back(L"-exports");
    arguments.push_back(L"RTGIRayGen;RTGIMiss;RTGIClosestHit");
#if defined(_DEBUG)
    arguments.push_back(DXC_ARG_DEBUG);
    arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

    DxcBuffer sourceBuffer = { sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), 0 };
    ComPtr<IDxcResult> compileResult;
    hr = dxcCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()),
        includeHandler.Get(), IID_PPV_ARGS(&compileResult));
    if (FAILED(hr)) return false;
    HRESULT compileStatus;
    compileResult->GetStatus(&compileStatus);
    if (FAILED(compileStatus))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        compileResult->GetErrorBuffer(&errorBlob);
        if (errorBlob)
            printf("[RayTracingPipelineState] RTGI コンパイルエラー:\n%s\n", (char*)errorBlob->GetBufferPointer());
        return false;
    }
    ComPtr<IDxcBlob> shaderBlob;
    compileResult->GetResult(&shaderBlob);

    static const wchar_t* kRayGenExport = L"RTGIRayGen";
    static const wchar_t* kMissExport = L"RTGIMiss";
    static const wchar_t* kClosestHitExport = L"RTGIClosestHit";
    static const wchar_t* kHitGroupExport = L"RTGIHitGroup";

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(20);

    D3D12_EXPORT_DESC exports[3] = {};
    exports[0].Name = kRayGenExport;
    exports[1].Name = kMissExport;
    exports[2].Name = kClosestHitExport;
    D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
    dxilLibDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
    dxilLibDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
    dxilLibDesc.NumExports = 3;
    dxilLibDesc.pExports = exports;
    D3D12_STATE_SUBOBJECT dxilLibSub = { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibDesc };
    subobjects.push_back(dxilLibSub);

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = kHitGroupExport;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = kClosestHitExport;
    D3D12_STATE_SUBOBJECT hitGroupSub = { D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc };
    subobjects.push_back(hitGroupSub);

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
    shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;
    D3D12_STATE_SUBOBJECT shaderConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig };
    subobjects.push_back(shaderConfigSub);

    ComPtr<ID3D12RootSignature> rayGenLocalRootSig;
    {
        CD3DX12_ROOT_SIGNATURE_DESC emptyLocalDesc(0, nullptr);
        emptyLocalDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        ComPtr<ID3DBlob> localSigBlob, localErrBlob;
        hr = D3D12SerializeRootSignature(&emptyLocalDesc, D3D_ROOT_SIGNATURE_VERSION_1, &localSigBlob, &localErrBlob);
        if (FAILED(hr)) return false;
        hr = device->CreateRootSignature(0, localSigBlob->GetBufferPointer(), localSigBlob->GetBufferSize(), IID_PPV_ARGS(&rayGenLocalRootSig));
        if (FAILED(hr)) return false;
        D3D12_STATE_SUBOBJECT localRootSub = { D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rayGenLocalRootSig.GetAddressOf() };
        subobjects.push_back(localRootSub);
        static const wchar_t* kRayGenOnlyExport[] = { kRayGenExport };
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localAssoc = {};
        localAssoc.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
        localAssoc.NumExports = 1;
        localAssoc.pExports = kRayGenOnlyExport;
        D3D12_STATE_SUBOBJECT localAssocSub = { D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localAssoc };
        subobjects.push_back(localAssocSub);
    }

    CD3DX12_DESCRIPTOR_RANGE ranges[5];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
    CD3DX12_ROOT_PARAMETER rootParams[7];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0]);
    rootParams[1].InitAsDescriptorTable(1, &ranges[1]);
    rootParams[2].InitAsDescriptorTable(1, &ranges[2]);
    rootParams[3].InitAsDescriptorTable(1, &ranges[3]);
    rootParams[4].InitAsDescriptorTable(1, &ranges[4]);
    rootParams[5].InitAsConstantBufferView(0);
    rootParams[6].InitAsConstantBufferView(1);
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(7, rootParams, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> globalSigBlob, globalErrBlob;
    hr = D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &globalSigBlob, &globalErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, globalSigBlob->GetBufferPointer(), globalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_globalRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT globalRootSub = { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature.GetAddressOf() };
    subobjects.push_back(globalRootSub);

    // RTGI Hit Group用 Local Root Signature（VB / IB / Albedo の 3 SRV）
    CD3DX12_DESCRIPTOR_RANGE localRange;
    localRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 1);
    CD3DX12_ROOT_PARAMETER localRootParam;
    localRootParam.InitAsDescriptorTable(1, &localRange);
    CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(1, &localRootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    ComPtr<ID3DBlob> rtgiLocalSigBlob, rtgiLocalErrBlob;
    hr = D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rtgiLocalSigBlob, &rtgiLocalErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, rtgiLocalSigBlob->GetBufferPointer(), rtgiLocalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_localRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT rtgiLocalRootSub = { D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_localRootSignature.GetAddressOf() };
    subobjects.push_back(rtgiLocalRootSub);
    static const wchar_t* kRTGILocalAssocExport[] = { kHitGroupExport };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rtgiLocalAssoc = {};
    rtgiLocalAssoc.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
    rtgiLocalAssoc.NumExports = 1;
    rtgiLocalAssoc.pExports = kRTGILocalAssocExport;
    D3D12_STATE_SUBOBJECT rtgiLocalAssocSub = { D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rtgiLocalAssoc };
    subobjects.push_back(rtgiLocalAssocSub);

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 1;
    D3D12_STATE_SUBOBJECT pipelineConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig };
    subobjects.push_back(pipelineConfigSub);

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
    stateObjectDesc.pSubobjects = subobjects.data();
    hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTGI: State Objectの作成失敗\n");
        return false;
    }
    hr = m_stateObject->QueryInterface(IID_PPV_ARGS(m_stateObjectProperties.GetAddressOf()));
    if (FAILED(hr)) return false;
    printf("[RayTracingPipelineState] RTGI State Object作成成功\n");
    return true;
}

bool RayTracingPipelineState::CreateForRTReflection(
    ID3D12Device5* device,
    const wchar_t* shaderLibraryPath,
    UINT maxPayloadSize,
    UINT maxAttributeSize)
{
    if (!device || !shaderLibraryPath)
    {
        printf("[RayTracingPipelineState] RTReflection: デバイスまたはシェーダーパスがnullです\n");
        return false;
    }

    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) return false;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) return false;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    if (FAILED(hr)) return false;

    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = dxcUtils->LoadFile(shaderLibraryPath, nullptr, &sourceBlob);
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTReflection: シェーダーファイルの読み込み失敗\n");
        return false;
    }

    std::vector<LPCWSTR> arguments;
    arguments.push_back(L"-T");
    arguments.push_back(L"lib_6_3");
    arguments.push_back(L"-default-linkage");
    arguments.push_back(L"external");
    arguments.push_back(L"-exports");
    arguments.push_back(L"RTReflectionRayGen;RTReflectionMiss;RTReflectionClosestHit");
#if defined(_DEBUG)
    arguments.push_back(DXC_ARG_DEBUG);
    arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
#else
    arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif

    DxcBuffer sourceBuffer = { sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), 0 };
    ComPtr<IDxcResult> compileResult;
    hr = dxcCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<UINT32>(arguments.size()),
        includeHandler.Get(), IID_PPV_ARGS(&compileResult));
    if (FAILED(hr)) return false;
    HRESULT compileStatus;
    compileResult->GetStatus(&compileStatus);
    if (FAILED(compileStatus))
    {
        ComPtr<IDxcBlobEncoding> errorBlob;
        compileResult->GetErrorBuffer(&errorBlob);
        if (errorBlob)
            printf("[RayTracingPipelineState] RTReflection コンパイルエラー:\n%s\n", (char*)errorBlob->GetBufferPointer());
        return false;
    }
    ComPtr<IDxcBlob> shaderBlob;
    compileResult->GetResult(&shaderBlob);

    static const wchar_t* kRayGenExport = L"RTReflectionRayGen";
    static const wchar_t* kMissExport = L"RTReflectionMiss";
    static const wchar_t* kClosestHitExport = L"RTReflectionClosestHit";
    static const wchar_t* kHitGroupExport = L"RTReflectionHitGroup";

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;
    subobjects.reserve(20);

    D3D12_EXPORT_DESC exports[3] = {};
    exports[0].Name = kRayGenExport;
    exports[1].Name = kMissExport;
    exports[2].Name = kClosestHitExport;
    D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
    dxilLibDesc.DXILLibrary.pShaderBytecode = shaderBlob->GetBufferPointer();
    dxilLibDesc.DXILLibrary.BytecodeLength = shaderBlob->GetBufferSize();
    dxilLibDesc.NumExports = 3;
    dxilLibDesc.pExports = exports;
    D3D12_STATE_SUBOBJECT dxilLibSub = { D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibDesc };
    subobjects.push_back(dxilLibSub);

    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = kHitGroupExport;
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = kClosestHitExport;
    D3D12_STATE_SUBOBJECT hitGroupSub = { D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc };
    subobjects.push_back(hitGroupSub);

    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
    shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;
    D3D12_STATE_SUBOBJECT shaderConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig };
    subobjects.push_back(shaderConfigSub);

    ComPtr<ID3D12RootSignature> rayGenLocalRootSig;
    {
        CD3DX12_ROOT_SIGNATURE_DESC emptyLocalDesc(0, nullptr);
        emptyLocalDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        ComPtr<ID3DBlob> localSigBlob, localErrBlob;
        hr = D3D12SerializeRootSignature(&emptyLocalDesc, D3D_ROOT_SIGNATURE_VERSION_1, &localSigBlob, &localErrBlob);
        if (FAILED(hr)) return false;
        hr = device->CreateRootSignature(0, localSigBlob->GetBufferPointer(), localSigBlob->GetBufferSize(), IID_PPV_ARGS(&rayGenLocalRootSig));
        if (FAILED(hr)) return false;
        D3D12_STATE_SUBOBJECT localRootSub = { D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rayGenLocalRootSig.GetAddressOf() };
        subobjects.push_back(localRootSub);
        static const wchar_t* kRayGenOnlyExport[] = { kRayGenExport };
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localAssoc = {};
        localAssoc.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
        localAssoc.NumExports = 1;
        localAssoc.pExports = kRayGenOnlyExport;
        D3D12_STATE_SUBOBJECT localAssocSub = { D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localAssoc };
        subobjects.push_back(localAssocSub);
    }

    CD3DX12_DESCRIPTOR_RANGE ranges[4];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    CD3DX12_ROOT_PARAMETER rootParams[6];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0]);
    rootParams[1].InitAsDescriptorTable(1, &ranges[1]);
    rootParams[2].InitAsDescriptorTable(1, &ranges[2]);
    rootParams[3].InitAsDescriptorTable(1, &ranges[3]);
    rootParams[4].InitAsDescriptorTable(1, &uavRange);
    rootParams[5].InitAsConstantBufferView(0);
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(6, rootParams, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    ComPtr<ID3DBlob> globalSigBlob, globalErrBlob;
    hr = D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &globalSigBlob, &globalErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, globalSigBlob->GetBufferPointer(), globalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_globalRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT globalRootSub = { D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature.GetAddressOf() };
    subobjects.push_back(globalRootSub);

    CD3DX12_DESCRIPTOR_RANGE localRange;
    localRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 1);
    CD3DX12_ROOT_PARAMETER localRootParam;
    localRootParam.InitAsDescriptorTable(1, &localRange);
    CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(1, &localRootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    ComPtr<ID3DBlob> rtReflLocalSigBlob, rtReflLocalErrBlob;
    hr = D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rtReflLocalSigBlob, &rtReflLocalErrBlob);
    if (FAILED(hr)) return false;
    hr = device->CreateRootSignature(0, rtReflLocalSigBlob->GetBufferPointer(), rtReflLocalSigBlob->GetBufferSize(), IID_PPV_ARGS(m_localRootSignature.GetAddressOf()));
    if (FAILED(hr)) return false;
    D3D12_STATE_SUBOBJECT rtReflLocalRootSub = { D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, m_localRootSignature.GetAddressOf() };
    subobjects.push_back(rtReflLocalRootSub);
    static const wchar_t* kRTReflLocalAssocExport[] = { kHitGroupExport };
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rtReflLocalAssoc = {};
    rtReflLocalAssoc.pSubobjectToAssociate = &subobjects[subobjects.size() - 1];
    rtReflLocalAssoc.NumExports = 1;
    rtReflLocalAssoc.pExports = kRTReflLocalAssocExport;
    D3D12_STATE_SUBOBJECT rtReflLocalAssocSub = { D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rtReflLocalAssoc };
    subobjects.push_back(rtReflLocalAssocSub);

    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 1;
    D3D12_STATE_SUBOBJECT pipelineConfigSub = { D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig };
    subobjects.push_back(pipelineConfigSub);

    D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
    stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    stateObjectDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
    stateObjectDesc.pSubobjects = subobjects.data();
    hr = device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    if (FAILED(hr))
    {
        printf("[RayTracingPipelineState] RTReflection: State Objectの作成失敗\n");
        return false;
    }
    hr = m_stateObject->QueryInterface(IID_PPV_ARGS(m_stateObjectProperties.GetAddressOf()));
    if (FAILED(hr)) return false;
    printf("[RayTracingPipelineState] RTReflection State Object作成成功\n");
    return true;
}

void* RayTracingPipelineState::GetShaderIdentifier(const wchar_t* shaderName) const
{
    if (!m_stateObjectProperties || !shaderName)
        return nullptr;
    return m_stateObjectProperties->GetShaderIdentifier(shaderName);
}

// ============================================================================
// ShaderBindingTable
// ============================================================================

void ShaderBindingTable::AddShaderRecord(
    void* destination,
    void* shaderIdentifier,
    UINT shaderIdentifierSize,
    void* localRootArguments,
    UINT localRootArgumentsSize)
{
    memcpy(destination, shaderIdentifier, shaderIdentifierSize);
    if (localRootArguments && localRootArgumentsSize > 0)
        memcpy(static_cast<char*>(destination) + shaderIdentifierSize, localRootArguments, localRootArgumentsSize);
}

bool ShaderBindingTable::Build(
    ID3D12Device5* device,
    RayTracingPipelineState* pipelineState,
    UINT numRayGenShaders,
    UINT numMissShaders,
    UINT numHitGroups)
{
    if (!device || !pipelineState)
        return false;

    const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    // レコードストライド: 32 バイト最小だが、ヒットグループ等で 64 バイトを要求するハードもあるため 64 に統一
    const UINT kRecordStride = 64u;
    m_shaderRecordSize = kRecordStride;
    m_rayGenShaderRecordSize = m_shaderRecordSize;
    m_missShaderRecordSize = m_shaderRecordSize;
    m_hitGroupShaderRecordSize = m_shaderRecordSize;
    m_numRayGenShaders = numRayGenShaders;
    m_numMissShaders = numMissShaders;
    m_numHitGroups = numHitGroups;

    // テーブル先頭は D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT(64) にアラインするため、バッファサイズを 64 の倍数に
    const UINT kTableAlignment = 64u;
    const UINT rayGenTableSize = ((numRayGenShaders * m_rayGenShaderRecordSize) + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    const UINT missTableSize = ((numMissShaders * m_missShaderRecordSize) + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    const UINT hitGroupTableSize = ((numHitGroups * m_hitGroupShaderRecordSize) + kTableAlignment - 1u) & ~(kTableAlignment - 1u);

    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    if (rayGenTableSize > 0)
    {
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rayGenTableSize);
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_rayGenShaderTable.GetAddressOf()));
        if (FAILED(hr)) return false;

        void* rayGenId = pipelineState->GetShaderIdentifier(L"ShadowRayGen");
        if (!rayGenId) return false;
        void* mapped = nullptr;
        if (FAILED(m_rayGenShaderTable->Map(0, nullptr, &mapped))) return false;
        memset(mapped, 0, rayGenTableSize);
        AddShaderRecord(mapped, rayGenId, shaderIdentifierSize);
        m_rayGenShaderTable->Unmap(0, nullptr);
    }

    if (missTableSize > 0)
    {
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(missTableSize);
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_missShaderTable.GetAddressOf()));
        if (FAILED(hr)) return false;

        void* missId = pipelineState->GetShaderIdentifier(L"ShadowMiss");
        if (!missId) return false;
        void* mapped = nullptr;
        if (FAILED(m_missShaderTable->Map(0, nullptr, &mapped))) return false;
        memset(mapped, 0, missTableSize);
        AddShaderRecord(mapped, missId, shaderIdentifierSize);
        m_missShaderTable->Unmap(0, nullptr);
    }

    if (hitGroupTableSize > 0)
    {
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(hitGroupTableSize);
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_hitGroupShaderTable.GetAddressOf()));
        if (FAILED(hr)) return false;

        void* hitGroupId = pipelineState->GetShaderIdentifier(L"ShadowHitGroup");
        if (!hitGroupId) return false;
        void* mapped = nullptr;
        if (FAILED(m_hitGroupShaderTable->Map(0, nullptr, &mapped))) return false;
        memset(mapped, 0, hitGroupTableSize);
        AddShaderRecord(mapped, hitGroupId, shaderIdentifierSize);
        m_hitGroupShaderTable->Unmap(0, nullptr);
    }

    return true;
}

bool ShaderBindingTable::BuildForRTAO(ID3D12Device5* device, RayTracingPipelineState* pipelineState)
{
    if (!device || !pipelineState)
        return false;

    const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    const UINT kRecordStride = 64u;
    const UINT kTableAlignment = 64u;
    m_shaderRecordSize = kRecordStride;
    m_rayGenShaderRecordSize = m_shaderRecordSize;
    m_missShaderRecordSize = m_shaderRecordSize;
    m_hitGroupShaderRecordSize = m_shaderRecordSize;
    m_numRayGenShaders = 1;
    m_numMissShaders = 1;
    m_numHitGroups = 1;

    const UINT rayGenTableSize = (kRecordStride + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    const UINT missTableSize = rayGenTableSize;
    const UINT hitGroupTableSize = rayGenTableSize;
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    void* rayGenId = pipelineState->GetShaderIdentifier(L"RTAORayGen");
    void* missId = pipelineState->GetShaderIdentifier(L"RTAOMiss");
    void* hitGroupId = pipelineState->GetShaderIdentifier(L"RTAOHitGroup");
    if (!rayGenId || !missId || !hitGroupId)
        return false;

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rayGenTableSize);
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_rayGenShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    void* mapped = nullptr;
    if (FAILED(m_rayGenShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, rayGenTableSize);
    AddShaderRecord(mapped, rayGenId, shaderIdentifierSize);
    m_rayGenShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(missTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_missShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_missShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, missTableSize);
    AddShaderRecord(mapped, missId, shaderIdentifierSize);
    m_missShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(hitGroupTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_hitGroupShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_hitGroupShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, hitGroupTableSize);
    AddShaderRecord(mapped, hitGroupId, shaderIdentifierSize);
    m_hitGroupShaderTable->Unmap(0, nullptr);

    return true;
}

bool ShaderBindingTable::BuildForRTGI(ID3D12Device5* device, RayTracingPipelineState* pipelineState)
{
    return BuildForRTGI(device, pipelineState, 1, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 }, 0);
}

bool ShaderBindingTable::BuildForRTGI(
    ID3D12Device5* device,
    RayTracingPipelineState* pipelineState,
    UINT numHitGroupRecords,
    D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptorForVBIB,
    UINT descriptorIncrementSize)
{
    if (!device || !pipelineState)
        return false;

    const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    const UINT kRecordStride = 64u;
    const UINT kTableAlignment = 64u;
    m_shaderRecordSize = kRecordStride;
    m_rayGenShaderRecordSize = m_shaderRecordSize;
    m_missShaderRecordSize = m_shaderRecordSize;
    m_hitGroupShaderRecordSize = kRecordStride;
    m_numRayGenShaders = 1;
    m_numMissShaders = 1;
    m_numHitGroups = (numHitGroupRecords > 0) ? numHitGroupRecords : 1;

    const UINT rayGenTableSize = (kRecordStride + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    const UINT missTableSize = rayGenTableSize;
    const UINT hitGroupTableSize = ((m_numHitGroups * kRecordStride) + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    void* rayGenId = pipelineState->GetShaderIdentifier(L"RTGIRayGen");
    void* missId = pipelineState->GetShaderIdentifier(L"RTGIMiss");
    void* hitGroupId = pipelineState->GetShaderIdentifier(L"RTGIHitGroup");
    if (!rayGenId || !missId || !hitGroupId)
        return false;

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rayGenTableSize);
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_rayGenShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    void* mapped = nullptr;
    if (FAILED(m_rayGenShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, rayGenTableSize);
    AddShaderRecord(mapped, rayGenId, shaderIdentifierSize);
    m_rayGenShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(missTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_missShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_missShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, missTableSize);
    AddShaderRecord(mapped, missId, shaderIdentifierSize);
    m_missShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(hitGroupTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_hitGroupShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_hitGroupShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, hitGroupTableSize);
    for (UINT i = 0; i < m_numHitGroups; ++i)
    {
        char* record = static_cast<char*>(mapped) + i * kRecordStride;
        memcpy(record, hitGroupId, shaderIdentifierSize);
        if (numHitGroupRecords > 0 && descriptorIncrementSize > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle;
            handle.ptr = baseDescriptorForVBIB.ptr + i * 3u * descriptorIncrementSize;
            memcpy(record + shaderIdentifierSize, &handle, sizeof(handle));
        }
    }
    m_hitGroupShaderTable->Unmap(0, nullptr);

    return true;
}

bool ShaderBindingTable::BuildForRTReflection(ID3D12Device5* device, RayTracingPipelineState* pipelineState)
{
    return BuildForRTReflection(device, pipelineState, 1, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 }, 0);
}

bool ShaderBindingTable::BuildForRTReflection(
    ID3D12Device5* device,
    RayTracingPipelineState* pipelineState,
    UINT numHitGroupRecords,
    D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptorForVBIB,
    UINT descriptorIncrementSize)
{
    if (!device || !pipelineState)
        return false;

    const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    const UINT kRecordStride = 64u;
    const UINT kTableAlignment = 64u;
    m_shaderRecordSize = kRecordStride;
    m_rayGenShaderRecordSize = m_shaderRecordSize;
    m_missShaderRecordSize = m_shaderRecordSize;
    m_hitGroupShaderRecordSize = kRecordStride;
    m_numRayGenShaders = 1;
    m_numMissShaders = 1;
    m_numHitGroups = (numHitGroupRecords > 0) ? numHitGroupRecords : 1;

    const UINT rayGenTableSize = (kRecordStride + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    const UINT missTableSize = rayGenTableSize;
    const UINT hitGroupTableSize = ((m_numHitGroups * kRecordStride) + kTableAlignment - 1u) & ~(kTableAlignment - 1u);
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    void* rayGenId = pipelineState->GetShaderIdentifier(L"RTReflectionRayGen");
    void* missId = pipelineState->GetShaderIdentifier(L"RTReflectionMiss");
    void* hitGroupId = pipelineState->GetShaderIdentifier(L"RTReflectionHitGroup");
    if (!rayGenId || !missId || !hitGroupId)
        return false;

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(rayGenTableSize);
    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_rayGenShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    void* mapped = nullptr;
    if (FAILED(m_rayGenShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, rayGenTableSize);
    AddShaderRecord(mapped, rayGenId, shaderIdentifierSize);
    m_rayGenShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(missTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_missShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_missShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, missTableSize);
    AddShaderRecord(mapped, missId, shaderIdentifierSize);
    m_missShaderTable->Unmap(0, nullptr);

    bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(hitGroupTableSize);
    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_hitGroupShaderTable.GetAddressOf()));
    if (FAILED(hr)) return false;
    if (FAILED(m_hitGroupShaderTable->Map(0, nullptr, &mapped))) return false;
    memset(mapped, 0, hitGroupTableSize);
    printf("[RTReflection/SBT] BuildForRTReflection: numHitGroups=%u baseDescriptor.ptr=%llu descriptorIncrementSize=%u\n",
        m_numHitGroups, (unsigned long long)baseDescriptorForVBIB.ptr, descriptorIncrementSize);
    for (UINT i = 0; i < m_numHitGroups; ++i)
    {
        char* record = static_cast<char*>(mapped) + i * kRecordStride;
        memcpy(record, hitGroupId, shaderIdentifierSize);
        if (numHitGroupRecords > 0 && descriptorIncrementSize > 0)
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle;
            handle.ptr = baseDescriptorForVBIB.ptr + i * 3u * descriptorIncrementSize;
            memcpy(record + shaderIdentifierSize, &handle, sizeof(handle));
            if (i < 3u || i == m_numHitGroups - 1u)
                printf("[RTReflection/SBT] hitGroupRecord#%u -> descriptorTable.ptr=%llu\n", i, (unsigned long long)handle.ptr);
        }
    }
    m_hitGroupShaderTable->Unmap(0, nullptr);

    return true;
}

D3D12_DISPATCH_RAYS_DESC ShaderBindingTable::GetDispatchRaysDesc(UINT width, UINT height) const
{
    D3D12_DISPATCH_RAYS_DESC desc = {};
    desc.RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable
        ? m_rayGenShaderTable->GetGPUVirtualAddress()
        : 0;
    desc.RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderRecordSize;
    desc.MissShaderTable.StartAddress = m_missShaderTable
        ? m_missShaderTable->GetGPUVirtualAddress()
        : 0;
    desc.MissShaderTable.SizeInBytes = m_numMissShaders * m_missShaderRecordSize;
    desc.MissShaderTable.StrideInBytes = m_missShaderRecordSize;
    desc.HitGroupTable.StartAddress = m_hitGroupShaderTable
        ? m_hitGroupShaderTable->GetGPUVirtualAddress()
        : 0;
    desc.HitGroupTable.SizeInBytes = m_numHitGroups * m_hitGroupShaderRecordSize;
    desc.HitGroupTable.StrideInBytes = m_hitGroupShaderRecordSize;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = 1;
    return desc;
}