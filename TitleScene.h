#pragma once
#include "Fade.h"
#include "KamataEngine.h"

class TitleScene {
public:
	~TitleScene();
	void Initialize();
	void Update();
	void Draw();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }

private:
	// 終了フラグ
	bool finished_ = false;

	// タイトルフォントモデル
	KamataEngine::Model* modelTitleFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformTitleFont_;
	uint32_t textureTitleFont_ = 0;

	// フローティングアニメーション用タイマー
	float floatTimer_ = 0.0f;
	static inline const float kBaseY_ = 3.0f;
	static inline const float kFloatAmplitude_ = 0.3f;
	static inline const float kFloatSpeed_ = 2.0f;

	// プレイヤーモデル
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransformPlayer_;
	uint32_t texturePlayer_ = 0;

	// カメラ
	KamataEngine::Camera camera_;

	// フェード
	Fade* fade_ = nullptr;
};