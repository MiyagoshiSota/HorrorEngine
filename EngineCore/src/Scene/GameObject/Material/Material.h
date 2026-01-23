#pragma once
#include <DirectXMath.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "Renderer/Texture/Texture2D.h"
#include "Renderer/Texture/TextureResourceManager.h"
#include "Modules/Other/EngineString.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHeap.h"

class Material
{
public:
    Material()
    {
        // 定数バッファの作成 (Colorなどのパラメータ用)
        constantBuffer = std::make_shared<ConstantBuffer>(sizeof(DirectX::XMFLOAT4));
        m_BaseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

        // --- デフォルトのテクスチャスロット定義 ---
        // シェーダー側のレジスタ番号(t0, t1...)と名前を紐付けます
        DefineTextureSlot("_MainTex", 0);      // Albedo
        DefineTextureSlot("_NormalMap", 1);    // Normal
        DefineTextureSlot("_MetallicRoughnessMap", 2);  // Metallic
        DefineTextureSlot("_EmissiveMap", 3);  // Emissive

        // 初期化時は「変更あり」としてマーク
        m_isDirty = true;
    }

    ~Material() {}

    // --- セットアップ系 ---

    // テクスチャをパスからロードしてセットする汎用関数
    // name: "_MainTex" や "_NormalMap" など
    // path: ファイルパス
    bool SetTexture(const std::string& name, const std::wstring& path)
    {
        std::shared_ptr<Texture2D> tex;

        if (path.empty())
        {
            // パスが空なら白テクスチャ
            tex = TextureResourceManager::Instance().WhiteTexture();
        }
        else
        {
            // マネージャー経由でロード（キャッシュ有効）
            tex = TextureResourceManager::Instance().GetTexture(path);

            // ロード失敗時も安全のため白テクスチャを入れる
            if (!tex) tex = TextureResourceManager::Instance().WhiteTexture();
        }

        // プロパティに保存
        m_textureProperties[name] = tex;
        m_isDirty = true; // ディスクリプタの再構築が必要

        return true;
    }

    // カラー設定
    void SetColor(DirectX::XMFLOAT4 color) { m_BaseColor = color; }
    DirectX::XMFLOAT4 GetColor() { return m_BaseColor; }


    // --- DirectX 12 内部処理 ---

    // 描画前に呼び出し、ディスクリプタヒープを更新する
    void UpdateDescriptors()
    {
        if (!m_isDirty) return;

        // 必要なスロット数を確保 (まだ確保していない、またはサイズが足りない場合)
        if (!m_SrvHandle || m_SrvHandle->index < m_maxSlotIndex + 1)
        {
            // 最大スロット数分、連続した領域を確保する
            // (例: PBRなら6枚分を一気に確保)
            m_SrvHandle = g_Engine->GetDescriptorHeap()->Allocate(m_maxSlotIndex + 1);
            
            if (!m_SrvHandle) {
                printf("マテリアルのSRVハンドル確保に失敗\n");
                return;
            }
        }

        auto device = g_Engine->Device();
        auto handleIncrementSize = g_Engine->GetDescriptorHeap()->GetIncrementSize();
        D3D12_CPU_DESCRIPTOR_HANDLE startCpuHandle = m_SrvHandle->cpuHandle;

        // 2. 定義されたスロット順にSRVを作成していく
        for (auto const& [name, slotIndex] : m_slotMap)
        {
            std::shared_ptr<Texture2D> tex;

            // セットされているテクスチャを取得
            auto it = m_textureProperties.find(name);
            if (it != m_textureProperties.end() && it->second)
            {
                tex = it->second;
            }
            else
            {
                // セットされていなければデフォルト（白）を使う
                // ※NormalMapの場合は本来青(0.5, 0.5, 1.0)が良いが、一旦白で代用
                tex = TextureResourceManager::Instance().WhiteTexture();
            }

            // 書き込み先のハンドル位置を計算 (開始位置 + スロット番号 * サイズ)
            D3D12_CPU_DESCRIPTOR_HANDLE destHandle = startCpuHandle;
            destHandle.ptr += slotIndex * handleIncrementSize;

            // SRV作成
            // Texture2Dクラスが持っている設定(ViewDesc)を使う
            auto desc = tex->GetViewDesc();
            device->CreateShaderResourceView(tex->GetResource().Get(), &desc, destHandle);
        }

        m_isDirty = false;
    }

    // GPUハンドルを取得 (描画コマンド用)
    std::shared_ptr<DescriptorHandle> GetSrvHandle()
    {
        UpdateDescriptors(); // 必要なら更新してから返す
        return m_SrvHandle;
    }

    std::shared_ptr<ConstantBuffer> GetConstantBuffer() { return constantBuffer; }

private:
    // スロット定義ヘルパー
    void DefineTextureSlot(const std::string& name, int slot)
    {
        m_slotMap[name] = slot;
        if (slot > m_maxSlotIndex) m_maxSlotIndex = slot;
    }

private:
    // --- プロパティデータ ---
    DirectX::XMFLOAT4 m_BaseColor;
    std::shared_ptr<ConstantBuffer> constantBuffer;

    // テクスチャ保持用マップ (名前 -> テクスチャ実体)
    std::map<std::string, std::shared_ptr<Texture2D>> m_textureProperties;

    // --- 管理用データ ---
    // 名前とGPUレジスタ番号の対応表 (例: "_MainTex" -> 0)
    std::map<std::string, int> m_slotMap;
    int m_maxSlotIndex = 0;

    // DX12 ハンドル
    std::shared_ptr<DescriptorHandle> m_SrvHandle;
    bool m_isDirty = true; // 再構築フラグ
};