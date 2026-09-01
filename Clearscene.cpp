#include "ClearScene.h"
#include "MathUtility.h"
#include <cmath>

using namespace KamataEngine;

void ClearScene::Initialize() {
	camera_.Initialize();
	camera_.translation_ = {0.0f, 3.0f, 0.0f};
	camera_.rotation_ = {0.2f, 0.0f, 0.0f};
	camera_.UpdateMatrix();

	model_ = Model::CreateFromOBJ("player", true);
	modelFont_ = Model::CreateFromOBJ("winFont", true);
	modelRestartFont_ = Model::CreateFromOBJ("restartFont", true);

	// ==== 背景(3Dの板として奥に配置する) ====
	modelBackground_ = Model::CreateFromOBJ("cube", true);
	backgroundTextureHandle_ = TextureManager::Load("./Resources/backGround/backGround.png");
	worldTransformBackground_.Initialize();
	worldTransformBackground_.translation_ = {0.0f, 3.0f, 25.0f};
	worldTransformBackground_.scale_ = {50.0f, 32.0f, 0.1f};
	worldTransformBackground_.rotation_.x = 0.2f; // カメラの傾きに合わせて台形歪みを防ぐ
	worldTransformBackground_.matWorld_ = MakeAffineMatrix(worldTransformBackground_.scale_, worldTransformBackground_.rotation_, worldTransformBackground_.translation_);
	worldTransformBackground_.TransferMatrix();

	worldTransform_.Initialize();
	worldTransform_.translation_ = {0.0f, 1.0f, 8.0f};

	worldTransformFont_.Initialize();
	worldTransformFont_.translation_ = {-3.3f, kFontBaseY, 7.5f};
	worldTransformFont_.rotation_.y = 3.14159265f; // 正面を向くよう180度反転
	worldTransformFont_.scale_ = {1.3f, 1.3f, 1.3f};
	fontBounceTimer_ = 0.0f;

	worldTransformRestartFont_.Initialize();
	worldTransformRestartFont_.translation_ = {-3.3f, -0.5f, 7.5f};
	worldTransformRestartFont_.rotation_.y = 3.14159265f; // 正面を向くよう180度反転
	worldTransformRestartFont_.scale_ = {0.9f, 0.9f, 0.9f};

	finished_ = false;

	// ==== フェード ====
	fade_ = new Fade();
	fade_->Initialize();
	fade_->StartFadeIn(kFadeDuration);
	exitRequested_ = false;
}

void ClearScene::Update() {
	fade_->Update();

	camera_.UpdateMatrix();

	worldTransform_.rotation_.y += 0.03f;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	fontBounceTimer_ += kFontBounceSpeed;
	worldTransformFont_.translation_.y = kFontBaseY + sinf(fontBounceTimer_) * kFontBounceAmplitude;
	worldTransformFont_.matWorld_ = MakeAffineMatrix(worldTransformFont_.scale_, worldTransformFont_.rotation_, worldTransformFont_.translation_);
	worldTransformFont_.TransferMatrix();

	worldTransformRestartFont_.matWorld_ = MakeAffineMatrix(worldTransformRestartFont_.scale_, worldTransformRestartFont_.rotation_, worldTransformRestartFont_.translation_);
	worldTransformRestartFont_.TransferMatrix();

	Input* input = Input::GetInstance();
	if (!exitRequested_ && input->TriggerKey(DIK_R)) {
		exitRequested_ = true;
		fade_->StartFadeOut(kFadeDuration);
	}

	// フェードアウトが完了してからシーンを終了する
	if (exitRequested_ && fade_->IsFinished()) {
		finished_ = true; // シーン変更 -> main.cppのChangeScene()がTitleSceneへ切り替える
	}
}

void ClearScene::Draw() {
	Model::PreDraw();
	modelBackground_->Draw(worldTransformBackground_, camera_, backgroundTextureHandle_);
	model_->Draw(worldTransform_, camera_);
	modelFont_->Draw(worldTransformFont_, camera_);
	modelRestartFont_->Draw(worldTransformRestartFont_, camera_);
	Model::PostDraw();

	// フェードの描画(必ず末尾)
	fade_->Draw();
}

ClearScene::~ClearScene() {
	delete model_;
	delete modelFont_;
	delete modelRestartFont_;
	delete modelBackground_;
	delete fade_;
}