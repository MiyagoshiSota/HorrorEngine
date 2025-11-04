#pragma once
#include <DirectXMath.h>

#include "Renderer/Texture/Texture2D.h"
#include "Modules/ComPtr.h"
#include "Modules/Other/engineString.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHeap.h"

class Material
{
public:
	Material()
	{
		constantBuffer = std::make_shared<ConstantBuffer>(sizeof(DirectX::XMFLOAT4));
	};
	
    ~Material()
    {
    }

	bool create_material(std::wstring deff_path)
	{
		// デフォルト値の設定
		m_DiffuseColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

		// SRVヒープからSRVハンドルを確保
		m_SrvHandle = g_Engine->GetDescriptorHeap()->Allocate();
		if (!m_SrvHandle)
		{
			printf("マテリアルのSRVハンドルの確保に失敗\n");
			return false;
		}

		std::shared_ptr<Texture2D> mainTex;

		if (deff_path.empty())
		{
			// テクスチャが指定されていない場合は白テクスチャを設定
			mainTex = Texture2D::GetWhite();
		}
		else
		{
			// MainTexture分のSRV生成
			// TODO:わざわざ拡張子をtgaに変えているけど問題ありそう.

			// // psdのみtgaに変換して読み込む仕様にする
			// if ()
			// {
			// 	auto texPath = engine_string::replace_extension(deff_path, "tga");
			// }
			
			mainTex = Texture2D::Get(deff_path.c_str());
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
	DirectX::XMFLOAT4 get_color() { return m_DiffuseColor; }

	void set_color(DirectX::XMFLOAT4 color) { m_DiffuseColor = color; }
	std::shared_ptr<ConstantBuffer> get_constant_buffer() { return constantBuffer; }

private:
	void set_diffuse_texture(std::shared_ptr<Texture2D> texture) { m_DiffuseTexture = texture; }

	std::shared_ptr<ConstantBuffer> constantBuffer;
	DirectX::XMFLOAT4 m_DiffuseColor;

    std::shared_ptr<Texture2D> m_DiffuseTexture;
	std::shared_ptr<DescriptorHandle> m_SrvHandle;
};

