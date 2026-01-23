#include "Core/App.h"
#include <DirectXTex.h>

#include "Scene/Default/Scene/DefaultScene.h"
#include "Modules/PublicConst/ConstNamePref.h"

int main() {
	std::shared_ptr<ISceneBase> scene = std::make_shared<DefaultScene>();
	StartApp(TEXT(ConstNamePref::WindowName), scene);
	ShutdownApp();
	return 0;
}