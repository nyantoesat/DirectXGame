#include "TitleScene.h"
#include "MathUtility.h"
#include <cmath>
#include <numbers>

using namespace KamataEngine;

TitleScene::~TitleScene() {
	delete modelTitleFont_;
	delete modelPlayer_;
	delete fade_;
}

void TitleScene::Initialize() {
	finished_ = false;
	floatTimer_ = 0.0f;

	// カメラの初期化
	camera_.Initialize();

	// タイトルフォントモデルの読み込み
	textureTitleFont_ = TextureManager::Load("./Resources/titleFont/white1x1.png");
	modelTitleFont_ = Model::CreateFromOBJ("titleFont", true);

	// タイトルフォントのワールドトランスフォーム初期化
	worldTransformTitleFont_.Initialize();
	worldTransformTitleFont_.translation_ = {0.0f, kBaseY_, 0.0f};

	// プレイヤーモデルの読み込み
	texturePlayer_ = TextureManager::Load("./Resources/player/player.png");
	modelPlayer_ = Model::CreateFromOBJ("player", true);

	// プレイヤーのワールドトランスフォーム初期化
	worldTransformPlayer_.Initialize();
	worldTransformPlayer_.translation_ = {0.0f, -1.0f, 0.0f};
	worldTransformPlayer_.scale_ = {5.0f, 5.0f, 5.0f};
	worldTransformPlayer_.rotation_.y = 135.0f;

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// シーン開始時にフェードイン
	fade_->StartFadeIn(1.0f);
}

void TitleScene::Update() {
	// フェードの更新
	fade_->Update();

	// フローティングアニメーション
	floatTimer_ += 1.0f / 60.0f;
	worldTransformTitleFont_.translation_.y = kBaseY_ + std::sin(floatTimer_ * kFloatSpeed_) * kFloatAmplitude_;

	// タイトルフォントのワールド行列更新
	worldTransformTitleFont_.matWorld_ = MakeAffineMatrix(worldTransformTitleFont_.scale_, worldTransformTitleFont_.rotation_, worldTransformTitleFont_.translation_);
	worldTransformTitleFont_.TransferMatrix();

	// プレイヤーのワールド行列更新
	worldTransformPlayer_.matWorld_ = MakeAffineMatrix(worldTransformPlayer_.scale_, worldTransformPlayer_.rotation_, worldTransformPlayer_.translation_);
	worldTransformPlayer_.TransferMatrix();

	// カメラの更新
	camera_.UpdateMatrix();

	// スペースキーで終了
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	Model::PreDraw();
	// タイトルフォントの描画
	modelTitleFont_->Draw(worldTransformTitleFont_, camera_, textureTitleFont_);
	// プレイヤーモデルの描画
	modelPlayer_->Draw(worldTransformPlayer_, camera_, texturePlayer_);
	Model::PostDraw();

	// フェードの描画（必ず末尾に追加）
	fade_->Draw();
}