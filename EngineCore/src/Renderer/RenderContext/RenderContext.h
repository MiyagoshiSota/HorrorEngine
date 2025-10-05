#pragma once

#include <map>
#include <string>
#include "Renderer/Target/RenderTarget.h" 
#include "Scene/Camera/SceneCamera.h"
#include "memory"
#include "Scene/GameObject/IGameObjectBase.h"

struct ID3D12GraphicsCommandList;

class RenderContext
{
public:
    // フレーム開始時に、パイプラインマネージャーが生成する
    RenderContext(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<SceneCamera> camera, std::vector<std::shared_ptr <IGameObjectBase>> gameObjects, float width, float height)
        : CommandList(cmdList)
        , Camera(camera)
        , GameObjects(gameObjects)
        , ScreenWidth(width)
        , ScreenHeight(height)
    {
    }

    // --- 各パスが必要とする情報 ---
    ID3D12GraphicsCommandList* CommandList;
    std::shared_ptr<SceneCamera> Camera;
    std::vector<std::shared_ptr <IGameObjectBase>> GameObjects;
    float ScreenWidth;
    float ScreenHeight;

    // 名前を指定してレンダーターゲットを取得する
    ITargetBase* GetRenderTarget(const std::string& name)
    {
        if (m_TargetPool.count(name)) {
            return m_TargetPool[name].get();
        }
        return nullptr;
    }

    // パイプラインマネージャーがレンダーターゲットを登録するのに使う
    void AddRenderTarget(const std::string& name, std::shared_ptr<ITargetBase> target)
    {
        m_TargetPool[name] = target;
    }

    

private:
    std::map<std::string, std::shared_ptr<ITargetBase>> m_TargetPool;
};