#pragma once
#include "KamataEngine.h"
#include "MathUtility.h"
#include <array>

class DeathParticles {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	// 終了しているか
	bool IsFinished() const { return isFinished_; }

private:
	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;
	// 移動速度
	static inline const float kSpeed = 0.05f;
	// 消滅までの時間
	static inline const float kDuration = 1.0f;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	// ワールドトランスフォームの配列
	std::array<KamataEngine::WorldTransform, kNumParticles> worldTransforms_;
	// 各パーティクルの速度
	std::array<KamataEngine::Vector3, kNumParticles> velocities_;

	// 終了フラグ
	bool isFinished_ = false;
	// 経過時間カウント
	float counter_ = 0.0f;
};