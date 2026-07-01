#include "TitleScene.h"

using namespace KamataEngine;

void TitleScene::Initialize() { finished_ = false; }

void TitleScene::Update() {
	// スペースキーで終了
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	// タイトル描画（現状は空）
}