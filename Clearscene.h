#pragma once

// See the NOMINMAX comment in GameScene.h - same landmine applies here if
// this scene ever calls std::min/std::max, so it's defined defensively
// before the first KamataEngine.h include in this translation unit.
#define NOMINMAX

#include "KamataEngine.h"
#include <vector>

/// <summary>
/// Shown once GameScene reports IsCleared() (see kClearScore in
/// GameScene.h). main.cpp reads the final score off GameScene right
/// before deleting it and passes it in via Initialize(). Waits for
/// SPACE/ENTER before finishing, so main.cpp knows to tear this down and
/// return to TitleScene.
/// </summary>
class ClearScene {
public:
	~ClearScene();

	void Initialize(int finalScore);
	void Update();
	void Draw();

	// True once the player has pressed on; main.cpp checks this once per
	// frame and swaps back to TitleScene when it flips.
	bool IsFinished() const { return finished_; }

private:
	static constexpr float kScreenWidth = 1280.0f;
	static constexpr float kScreenHeight = 720.0f;

	// Score digits, drawn as sprite-based seven-segment digits (small
	// tinted rectangles) - same technique GameScene uses for its own
	// score display, since KamataEngine::DebugText::GetInstance() returns
	// nullptr in this build (never created/initialized anywhere in this
	// project) and calling Printf/DrawAll on it crashes. Built once in
	// Initialize(); DrawScore() just decides which of these light up each
	// frame based on finalScore_, which never changes once this scene is
	// entered. Centered horizontally, roughly where the old deco-block
	// stack used to sit.
	static constexpr int kScoreDigitCount = 4; // "0000".."9999"
	static constexpr float kScoreScreenY = 260.0f;
	static constexpr float kDigitWidth = 40.0f;
	static constexpr float kDigitHeight = 64.0f;
	static constexpr float kDigitSpacing = 56.0f; // distance between digit origins
	static constexpr float kDigitThickness = 10.0f;

	// "press to continue" blinks on/off at this interval (seconds). No
	// text rendering here - see the NOTE in ClearScene.cpp - so this is a
	// plain sprite bar, not a string.
	static constexpr float kBlinkInterval = 0.5f;
	static constexpr float kPromptWidth = 260.0f;
	static constexpr float kPromptHeight = 20.0f;
	static constexpr float kPromptY = 500.0f;

	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* background_ = nullptr;
	KamataEngine::Sprite* promptSprite_ = nullptr; // the blinking "continue" bar

	// Flat array of kScoreDigitCount * 7 segment sprites, indexed as
	// scoreSegments_[digitIndex * 7 + segmentIndex]. Built once in
	// Initialize(); DrawScore() picks which ones to Draw() each frame.
	std::vector<KamataEngine::Sprite*> scoreSegments_;

	float blinkTimer_ = 0.0f;
	bool blinkVisible_ = true;
	int finalScore_ = 0;
	bool finished_ = false;

	void DrawScore();
};