#pragma once
#include "KamataEngine.h"

// 天球(スカイドーム)クラス
class Skydome {
public:
	void Initialize(KamataEngine::Model* model);
	void Update();
	void Draw(const KamataEngine::Camera& camera);

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	// シーン全体を包み込むよう大きめのスケールにする
	static constexpr float kScale = 500.0f;
};