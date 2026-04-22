#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"GC2B_03_ニャン_トー_セッ_AL2");
	GameScene* gameScene = new GameScene();
	gameScene->Initialize();
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	while (true) {
		if (KamataEngine::Update()) {
			break; 
		}
		gameScene->Update();
		// 描画開始
		dxCommon->PreDraw();

		gameScene->Draw();

		dxCommon->PostDraw();
	}

	delete gameScene;

	gameScene = nullptr;

	KamataEngine::Finalize();
	return 0;
}
