#include "TitleScene.h"

using namespace KamataEngine;

TitleScene::~TitleScene() { delete fade_; }

void TitleScene::Initialize() {
	finished_ = false;

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// シーン開始時にフェードイン
	fade_->StartFadeIn(1.0f);
}

void TitleScene::Update() {
	// フェードの更新
	fade_->Update();

	// スペースキーで終了
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	// タイトル描画（現状は空）

	// フェードの描画（必ず末尾に追加）
	fade_->Draw();
}