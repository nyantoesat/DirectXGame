#include "ClearScene.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "HowToPlayScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"
#include <Windows.h>

using namespace KamataEngine;

// シーン(型)
enum class Scene {
	kUnknown = 0,
	kTitle,
	kHowToPlay,
	kGame,
	kClear,
	kGameOver,
};

// グローバル変数
TitleScene* titleScene = nullptr;
HowToPlayScene* howToPlayScene = nullptr;
GameScene* gameScene = nullptr;
ClearScene* clearScene = nullptr;
GameOverScene* gameOverScene = nullptr;

// 現在シーン(型)
Scene scene = Scene::kUnknown;

// シーン切り替え
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			scene = Scene::kHowToPlay;
			delete titleScene;
			titleScene = nullptr;
			howToPlayScene = new HowToPlayScene();
			howToPlayScene->Initialize();
		}
		break;

	case Scene::kHowToPlay:
		if (howToPlayScene->IsFinished()) {
			scene = Scene::kGame;
			delete howToPlayScene;
			howToPlayScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene->IsFinished()) {
			GameScene::Result result = gameScene->GetResult();
			delete gameScene;
			gameScene = nullptr;

			if (result == GameScene::Result::kClear) {
				scene = Scene::kClear;
				clearScene = new ClearScene();
				clearScene->Initialize();
			} else if (result == GameScene::Result::kQuitToTitle) {
				scene = Scene::kTitle;
				titleScene = new TitleScene();
				titleScene->Initialize();
			} else {
				scene = Scene::kGameOver;
				gameOverScene = new GameOverScene();
				gameOverScene->Initialize();
			}
		}
		break;

	case Scene::kClear:
		if (clearScene->IsFinished()) {
			scene = Scene::kTitle;
			delete clearScene;
			clearScene = nullptr;
			titleScene = new TitleScene();
			titleScene->Initialize();
		}
		break;

	case Scene::kGameOver:
		if (gameOverScene->IsFinished()) {
			scene = Scene::kTitle;
			delete gameOverScene;
			gameOverScene = nullptr;
			titleScene = new TitleScene();
			titleScene->Initialize();
		}
		break;
	}
}

// シーンの更新
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kHowToPlay:
		howToPlayScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	case Scene::kClear:
		clearScene->Update();
		break;
	case Scene::kGameOver:
		gameOverScene->Update();
		break;
	}
}

// シーンの描画
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kHowToPlay:
		howToPlayScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	case Scene::kClear:
		clearScene->Draw();
		break;
	case Scene::kGameOver:
		gameOverScene->Draw();
		break;
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"GC2B_03_Skystrike");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// 最初のシーンの初期化
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		// シーン切り替え
		ChangeScene();
		// 現在シーン更新
		UpdateScene();

		// 描画開始
		dxCommon->PreDraw();

		// 現在シーンの描画
		DrawScene();

		dxCommon->PostDraw();
	}

	// シーン解放
	delete titleScene;
	delete howToPlayScene;
	delete gameScene;
	delete clearScene;
	delete gameOverScene;

	KamataEngine::Finalize();
	return 0;
}