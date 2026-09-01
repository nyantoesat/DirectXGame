#pragma once
#include "Fade.h"
#include "KamataEngine.h"

class ClearScene {
public:
	~ClearScene();

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	KamataEngine::Camera camera_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	// ==== 背景(3Dの板として奥に配置する。Spriteだと3D描画が乱れるため) ====
	KamataEngine::Model* modelBackground_ = nullptr;
	KamataEngine::WorldTransform worldTransformBackground_;
	uint32_t backgroundTextureHandle_ = 0;

	// ==== クリアロゴ(3Dテキスト) ====
	KamataEngine::Model* modelFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformFont_;
	float fontBounceTimer_ = 0.0f;
	static constexpr float kFontBaseY = 3.5f;
	static constexpr float kFontBounceAmplitude = 0.3f;
	static constexpr float kFontBounceSpeed = 0.06f;

	// ==== 「Rで再挑戦」等の案内テキスト(3Dテキスト) ====
	KamataEngine::Model* modelRestartFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformRestartFont_;

	bool finished_ = false;

	// ==== フェード ====
	Fade* fade_ = nullptr;
	bool exitRequested_ = false;
	static constexpr float kFadeDuration = 0.5f;
};