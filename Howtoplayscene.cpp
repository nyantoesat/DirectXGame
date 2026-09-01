#include "HowToPlayScene.h"

using namespace KamataEngine;

void HowToPlayScene::Initialize() {
	// ==== 背景画像 ====
	uint32_t backgroundTextureHandle = TextureManager::Load("./Resources/menu/menu.png");
	spriteBackground_ = Sprite::Create(backgroundTextureHandle, Vector2(0.0f, 0.0f));
	spriteBackground_->SetSize(Vector2(1280.0f, 720.0f));

	finished_ = false;

	// ==== フェード ====
	fade_ = new Fade();
	fade_->Initialize();
	fade_->StartFadeIn(kFadeDuration);
	exitRequested_ = false;
}

void HowToPlayScene::Update() {
	fade_->Update();

	Input* input = Input::GetInstance();
	if (!exitRequested_ && input->TriggerKey(DIK_G)) {
		exitRequested_ = true;
		fade_->StartFadeOut(kFadeDuration);
	}

	// フェードアウトが完了してからシーンを終了する
	if (exitRequested_ && fade_->IsFinished()) {
		finished_ = true; // シーン変更 -> main.cppのChangeScene()がGameSceneへ切り替える
	}
}

void HowToPlayScene::Draw() {
	// 背景画像(このシーンには他に3Dモデルが無いため、これだけで問題ない)
	Sprite::PreDraw();
	spriteBackground_->Draw();
	Sprite::PostDraw();

	// フェードの描画(必ず末尾)
	fade_->Draw();
}

HowToPlayScene::~HowToPlayScene() {
	delete spriteBackground_;
	delete fade_;
}