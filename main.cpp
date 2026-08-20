#include "KamataEngine.h"
#include "SceneManager.h"
#include <Windows.h>

// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"GC2B_03_ニャン_トー_セッ_AL3_3D");

	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// シーン管理をSceneManagerに一任する(2D版のGameManagerに相当)
	SceneManager* sceneManager = new SceneManager();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		imguiManager->Begin();

		sceneManager->Update();

		// 描画開始
		dxCommon->PreDraw();

		sceneManager->Draw();
		AxisIndicator::GetInstance()->Draw();

		// ImGuiの描画(imguiManager->Begin()と対で必要)
		imguiManager->Draw();

		dxCommon->PostDraw();
	}

	delete sceneManager;

	sceneManager = nullptr;

	KamataEngine::Finalize();
	return 0;
}