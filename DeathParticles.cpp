#include "DeathParticles.h"
#include "MathUtility.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3& position) {
	model_ = model;
	camera_ = camera;
	counter_ = 0.0f;
	isFinished_ = false;

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position;
	}

	// 各パーティクルを360度均等に広がる方向に速度を設定
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		float angle = (2.0f * std::numbers::pi_v<float>) / kNumParticles * i;
		velocities_[i] = {std::cos(angle) * kSpeed, std::sin(angle) * kSpeed, 0.0f};
	}
}

void DeathParticles::Update() {
	// 終了済みなら何もしない（早期リターン）
	if (isFinished_) {
		return;
	}

	// カウンターを1フレーム分の秒数進める
	counter_ += 1.0f / 60.0f;

	// 存続時間の上限に達したら
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		// 終了扱いにする
		isFinished_ = true;
	}

	// スケールで縮小フェード（1.0 → 0.0）
	float scale = std::clamp(1.0f - counter_ / kDuration, 0.0f, 1.0f);

	// ワールド変換の更新（8個分の速度ベクトルを計算して座標を進める）
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 移動
		worldTransforms_[i].translation_ += velocities_[i];

		// スケールを縮小
		worldTransforms_[i].scale_ = {scale, scale, scale};

		// アフィン行列の計算・VRAM転送
		worldTransforms_[i].matWorld_ = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].TransferMatrix();
	}
}

void DeathParticles::Draw() {
	// 終了済みなら何もしない（早期リターン）
	if (isFinished_) {
		return;
	}

	// モデルの描画
	for (WorldTransform& worldTransform : worldTransforms_) {
		model_->Draw(worldTransform, *camera_);
	}
}