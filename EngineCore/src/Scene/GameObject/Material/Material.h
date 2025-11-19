#pragma once
#include <DirectXMath.h>

#include "Renderer/Texture/Texture2D.h"
#include "Modules/ComPtr.h"
#include "Modules/Other/engineString.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/DescriptorHeap/SrvDescriptorHeap.h"

class Material
{
public:
    ~Material(){}

	bool create_material(std::wstring deff_path)
	{
		// デフォルト値の設定
		m_color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

		// SRVヒープからSRVハンドルを確保
		m_SrvHandle = g_Engine->GetSrvHeap()->Allocate();
		if (!m_SrvHandle)
		{
			printf("マテリアルのSRVハンドルの確保に失敗\n");
			return false;
		}

		std::shared_ptr<Texture2D> mainTex;
		auto texManager = g_Engine->GetTextureManager();

		if (deff_path.empty())
		{
			// テクスチャが指定されていない場合は白テクスチャを設定
			mainTex = texManager->GetWhite();
		}
		else
		{
			// MainTexture分のSRV生成
			// TODO:わざわざ拡張子をtgaに変えているけど問題ありそう.
			auto texPath = engine_string::replace_extension(deff_path, "tga");
			mainTex = texManager->Get(texPath);
		}

		auto desc = mainTex->ViewDesc();

		// SRVの作成
		g_Engine->Device()->CreateShaderResourceView(mainTex->Resource().Get(), &desc, m_SrvHandle->cpuHandle); 

		// テクスチャの読み込み
		set_diffuse_texture(mainTex);

		return true;
	}

	std::shared_ptr<Texture2D> get_diffuse_texture() { return m_DiffuseTexture; }
	std::shared_ptr<DescriptorHandle> get_srv_handle() { return m_SrvHandle; }

private:
	void set_diffuse_texture(std::shared_ptr<Texture2D> texture) { m_DiffuseTexture = texture; }

    std::shared_ptr<Texture2D> m_DiffuseTexture;
    DirectX::XMFLOAT4 m_color;
	std::shared_ptr<DescriptorHandle> m_SrvHandle;
};

