#pragma once
#include "Fade.h"
#include "KamataEngine.h"

// 遊び方説明シーン(タイトルとゲームの間に挟む)
class HowToPlayScene {
public:
	~HowToPlayScene();

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	// ==== 背景画像(このシーンには他に3Dモデルが無いためSpriteで問題ない) ====
	KamataEngine::Sprite* spriteBackground_ = nullptr;

	bool finished_ = false;

	// ==== フェード ====
	Fade* fade_ = nullptr;
	bool exitRequested_ = false;
	static constexpr float kFadeDuration = 0.5f;
};