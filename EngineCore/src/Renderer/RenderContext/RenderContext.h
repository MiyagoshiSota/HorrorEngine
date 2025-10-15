#pragma once

#include <map>
#include <string>
#include "Renderer/Target/RenderTarget.h" 
#include "Scene/Camera/SceneCamera.h"
#include "memory"
#include "Scene/GameObject/GameObject.h"

struct ID3D12GraphicsCommandList;

class RenderContext
{
public:
    // フレーム開始時に、パイプラインマネージャーが生成する
    RenderContext(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<SceneCamera> camera, std::vector<std::shared_ptr <GameObject>> gameObjects, std::shared_ptr<ITargetBase> sourceRT, std::shared_ptr<ITargetBase> destRT, float width, float height)
        : CommandList(cmdList)
        , Camera(camera)
        , GameObjects(gameObjects)
		, SourceRT(sourceRT)
		, DestRT(destRT)
        , ScreenWidth(width)
        , ScreenHeight(height)
    {
    }

    // --- 各パスが必要とする情報 ---
    ID3D12GraphicsCommandList* CommandList;
    std::shared_ptr<SceneCamera> Camera;
    std::vector<std::shared_ptr <GameObject>> GameObjects;
    std::shared_ptr<ITargetBase> SourceRT;
    std::shared_ptr<ITargetBase> DestRT;
    float ScreenWidth;
    float ScreenHeight;

    // 名前を指定してレンダーターゲットを取得する
    std::shared_ptr<ITargetBase> GetRenderTarget(const std::string& name)
    {
        if (m_TargetPool.count(name)) {
            return m_TargetPool[name];
        }
        return nullptr;
    }

    // パイプラインマネージャーがレンダーターゲットを登録するのに使う
    void AddRenderTarget(const std::string& name, std::shared_ptr<ITargetBase> target)
    {
        m_TargetPool[name] = target;
    }

	void SetSourceRT(std::shared_ptr<ITargetBase> rt) { SourceRT = rt; }
	void SetDestRT(std::shared_ptr<ITargetBase> rt) { DestRT = rt; }

    std::shared_ptr<ITargetBase> GetSourceRT() { return SourceRT; }
    std::shared_ptr<ITargetBase> GetDestRT() { return DestRT; }


private:
    std::map<std::string, std::shared_ptr<ITargetBase>> m_TargetPool;
};