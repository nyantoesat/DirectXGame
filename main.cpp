#include "ClearScene.h"
#include "FadeTransition.h"
#include "GameScene.h"
#include "KamataEngine.h"
#include "TitleScene.h"
#include <Windows.h>

// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;

namespace {
enum class SceneType {
	kTitle,
	kGame,
	kClear,
};
} // namespace

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"GC2B_03_ニャン_トー_セッ_AL3");

	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// Only one of these three is ever non-null at a time - whichever
	// matches currentScene - swapped in the switches below. Kept as three
	// separate pointers rather than behind a shared base class since
	// Title/Game/Clear don't share an interface today (different
	// Initialize() signatures, etc.); that's the natural next refactor if
	// a 4th scene shows up.
	TitleScene* titleScene = nullptr;
	GameScene* gameScene = nullptr;
	ClearScene* clearScene = nullptr;

	// Unlike the three above, this lives for the whole app - it's an
	// overlay drawn on top of whatever scene is active, not a scene
	// itself. See FadeTransition.h.
	FadeTransition* fadeTransition = new FadeTransition();
	fadeTransition->Initialize();

	// True from the moment the player presses start on the title screen
	// until the fade-out finishes and GameScene has actually been
	// created. While true, TitleScene is frozen (no more Update() calls)
	// so its blinking prompt doesn't keep animating behind the fade.
	bool titleToGameFading = false;

	SceneType currentScene = SceneType::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		imguiManager->Begin();

		switch (currentScene) {
		case SceneType::kTitle:
			if (!titleToGameFading) {
				titleScene->Update();
				if (titleScene->IsFinished()) {
					// Don't swap scenes yet - let the overlay reach full
					// black first, below, so the player never sees the
					// literal moment TitleScene is torn down and
					// GameScene is built.
					titleToGameFading = true;
					fadeTransition->StartFadeOut();
				}
			}
			break;

		case SceneType::kGame:
			gameScene->Update();
			// NOTE: GameScene's own game-over state is still handled
			// entirely inside GameScene (SPACE/ENTER calls its Reset()
			// and the run starts over in place, no fade) - this only
			// intercepts the *win* condition (kCleared, see
			// kClearScore in GameScene.h) to hand off to ClearScene.
			if (gameScene->IsCleared()) {
				int finalScore = gameScene->GetScore();

				delete gameScene;
				gameScene = nullptr;

				clearScene = new ClearScene();
				clearScene->Initialize(finalScore);
				currentScene = SceneType::kClear;
			}
			break;

		case SceneType::kClear:
			clearScene->Update();
			if (clearScene->IsFinished()) {
				delete clearScene;
				clearScene = nullptr;

				titleScene = new TitleScene();
				titleScene->Initialize();
				currentScene = SceneType::kTitle;
			}
			break;
		}

		// Exit chosen from the title menu (see TitleScene::WantsExit()) -
		// break out of the main loop so the cleanup/KamataEngine::Finalize()
		// code below the loop runs and the window closes normally, same
		// as if the OS close button had been clicked (see the
		// KamataEngine::Update() check at the top of the loop).
		if (currentScene == SceneType::kTitle && titleScene && titleScene->WantsExit()) {
			break;
		}

		fadeTransition->Update();

		// The overlay has just reached full black - safe to swap scenes
		// behind it. This runs the frame after titleToGameFading first
		// flips true (once the fade-out timer finishes), not the same
		// frame, since IsFadeOutComplete() reflects the state *after*
		// the Update() call just above.
		if (titleToGameFading && fadeTransition->IsFadeOutComplete()) {
			delete titleScene;
			titleScene = nullptr;

			gameScene = new GameScene();
			gameScene->Initialize();
			currentScene = SceneType::kGame;

			titleToGameFading = false;
			fadeTransition->StartFadeIn();
		}

		// 描画開始
		dxCommon->PreDraw();

		switch (currentScene) {
		case SceneType::kTitle:
			titleScene->Draw();
			break;
		case SceneType::kGame:
			gameScene->Draw();
			break;
		case SceneType::kClear:
			clearScene->Draw();
			break;
		}
		fadeTransition->Draw(); // overlay layer, drawn on top of whichever scene just drew
		AxisIndicator::GetInstance()->Draw();

		dxCommon->PostDraw();
	}

	delete titleScene;
	delete gameScene;
	delete clearScene;
	delete fadeTransition;

	KamataEngine::Finalize();
	return 0;
}