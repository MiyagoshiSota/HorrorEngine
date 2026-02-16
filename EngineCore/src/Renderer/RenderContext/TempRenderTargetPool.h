#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <d3d12.h>

#include "Renderer/Engine.h"
#include "Renderer/Target/RenderTarget.h"

class TempRenderTargetPool
{
public:
    TempRenderTargetPool() = default;
    ~TempRenderTargetPool() {
        m_Pool.clear();
    }

    /// <summary>
    /// プールから指定スペックのRTを取得。なければ新規作成。
    /// </summary>
    std::shared_ptr<RenderTarget> Get(float width, float height, DXGI_FORMAT format)
    {
        // プール内を検索
        std::vector<std::shared_ptr<RenderTarget>>::iterator it;
        it = std::find_if(m_Pool.begin(), m_Pool.end(),
                          [&](const std::shared_ptr<RenderTarget>&
                          rt)
                          {
	                          // 解像度とフォーマットが一致するものを探す
	                          return rt->GetWidth() == width &&
		                          rt->GetHeight() == height &&
		                          rt->GetFormat() == format;
                          });

        // 見つかった場合
        if (it != m_Pool.end())
        {
            auto target = *it;
            // プールから削除して返す
            m_Pool.erase(it);
            return target;
        }

        // 見つからなかった場合は新規作成
        auto newTarget = std::make_shared<RenderTarget>();

        // RenderTargetの作成
        newTarget->Create(
            g_Engine->Device(),
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            format,
            1, // arraySize
            1, // mipLevels
            1, // sampleCount
            0, // sampleQuality
            g_Engine->AllocateRtvHandle(),              // RTV Descriptor
            g_Engine->GetDescriptorHeap()->Allocate(1)  // SRV Descriptor
        );

        return newTarget;
    }

    /// <summary>
    /// 使い終わったRTをプールに返却する
    /// </summary>
    void Return(std::shared_ptr<RenderTarget> target)
    {
        if (target)
        {
            // 再利用のためにリストに追加
            m_Pool.push_back(target);
        }
    }

private:
    // 待機中のレンダーターゲットリスト
    std::vector<std::shared_ptr<RenderTarget>> m_Pool;
};