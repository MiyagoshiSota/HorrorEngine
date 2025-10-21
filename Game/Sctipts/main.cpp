#include "main.h"
#include "Core/App.h"
#include <DirectXTex.h>

#include "Scene/Default/Scene/DefaultScene.h"
#include "Modules/PublicConst/const_name_pref.h"

int main() {
	std::shared_ptr<ISceneBase> scene = std::make_shared<DefaultScene>();
	start_app(TEXT(const_name_pref::WindowName), scene);
	shutdown_app();
	return 0;
}