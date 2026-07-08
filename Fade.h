#pragma once
#include "KamataEngine.h"

/// <summary>
/// フェード
/// </summary>
class Fade {
public:
	// フェードの状態
	enum class Status {
		None,    // フェードなし
		FadeIn,  // フェードイン中
		FadeOut, // フェードアウト中
	};

	~Fade();
	void Initialize();
	void Update();
	void Draw();

	// フェードイン開始
	void StartFadeIn(float duration = 1.0f);
	// フェードアウト開始
	void StartFadeOut(float duration = 1.0f);
	// フェードが終了しているか
	bool IsFinished() const { return status_ == Status::None; }

private:
	// 黒スプライト
	KamataEngine::Sprite* sprite_ = nullptr;

	// 現在のフェードの状態
	Status status_ = Status::None;

	// 経過時間カウント
	float counter_ = 0.0f;
	// フェード継続時間
	float duration_ = 1.0f;
};