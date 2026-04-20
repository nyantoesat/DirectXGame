#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"GC2B_03_ニャン_トー_セッ_AL3");

	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	GameScene* gameScene = new GameScene();

	gameScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		imguiManager->Begin();

		gameScene->Update();

		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();

		gameScene->Draw();

		AxisIndicator::GetInstance()->Draw();

		imguiManager->Draw();


		dxCommon->PostDraw();
	}

	delete gameScene;

	gameScene = nullptr;

	KamataEngine::Finalize();
	return 0;
}
