#include "main.h"
#include "Core/App.h"
#include <stdio.h>
#include <DirectXTex.h>
#include "Scene/DefaultScene.h"

int main() {
	std::shared_ptr<ISceneBase> scene = std::make_shared<DefaultScene>();
	start_app(TEXT("Hello DirectX12!"), scene);
	return 0;
}