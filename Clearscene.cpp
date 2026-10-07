#include "ClearScene.h"

using namespace KamataEngine;

namespace {

// Classic seven-segment layout, segments ordered {a,b,c,d,e,f,g}:
//   _a_
//  f   b
//   -g-
//  e   c
//   -d-
// Duplicated from GameScene.cpp rather than shared, since this project
// keeps each scene's translation unit self-contained (see the similar
// duplication of the blinking-prompt pattern between TitleScene and this
// file) - if you later pull this into a shared header, this table and
// ComputeSegmentRect() below are the only two things to move.
constexpr bool kDigitSegments[10][7] = {
    {true,  true,  true,  true,  true,  true,  false}, // 0
    {false, true,  true,  false, false, false, false}, // 1
    {true,  true,  false, true,  true,  false, true }, // 2
    {true,  true,  true,  true,  false, false, true }, // 3
    {false, true,  true,  false, false, true,  true }, // 4
    {true,  false, true,  true,  false, true,  true }, // 5
    {true,  false, true,  true,  true,  true,  true }, // 6
    {true,  true,  true,  false, false, false, false}, // 7
    {true,  true,  true,  true,  true,  true,  true }, // 8
    {true,  true,  true,  true,  false, true,  true }, // 9
};

struct SegRect {
	float x, y, w, h;
};

// Geometry for one of the 7 segments of a digit whose bounding box is
// (ox, oy, w, h), with stroke thickness t.
SegRect ComputeSegmentRect(int segment, float ox, float oy, float w, float h, float t) {
	float h2 = h * 0.5f;
	switch (segment) {
	case 0:
		return {ox + t, oy, w - 2.0f * t, t}; // a: top
	case 1:
		return {ox + w - t, oy + t, t, h2 - t}; // b: top-right
	case 2:
		return {ox + w - t, oy + h2, t, h2 - t}; // c: bottom-right
	case 3:
		return {ox + t, oy + h - t, w - 2.0f * t, t}; // d: bottom
	case 4:
		return {ox, oy + h2, t, h2 - t}; // e: bottom-left
	case 5:
		return {ox, oy + t, t, h2 - t}; // f: top-left
	default:
		return {ox + t, oy + h2 - t * 0.5f, w - 2.0f * t, t}; // g: middle
	}
}

} // namespace

ClearScene::~ClearScene() {
	delete background_;
	delete promptSprite_;
	for (auto* seg : scoreSegments_) {
		delete seg;
	}
}

void ClearScene::Initialize(int finalScore) {
	finalScore_ = finalScore;
	finished_ = false;
	blinkTimer_ = 0.0f;
	blinkVisible_ = true;

	textureHandle_ = TextureManager::Load("white1x1.png");

	// Green-tinted, distinct from both the title's neutral background and
	// GameScene's game-over red.
	background_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.08f, 0.22f, 0.14f, 1.0f});
	background_->SetSize({kScreenWidth, kScreenHeight});

	// Score digits: built once here, centered horizontally roughly where
	// the old deco-block stack used to sit. DrawScore() decides which of
	// these light up each frame based on finalScore_ - see the
	// kScoreDigitCount comment in ClearScene.h for why sprites instead of
	// a text API.
	for (auto* seg : scoreSegments_) {
		delete seg;
	}
	scoreSegments_.clear();

	Vector4 kScoreColor = {0.95f, 0.9f, 0.75f, 1.0f};
	float totalWidth = static_cast<float>(kScoreDigitCount) * kDigitSpacing;
	float startX = kScreenWidth * 0.5f - totalWidth * 0.5f;
	scoreSegments_.reserve(static_cast<size_t>(kScoreDigitCount) * 7);
	for (int digit = 0; digit < kScoreDigitCount; ++digit) {
		float ox = startX + static_cast<float>(digit) * kDigitSpacing;
		for (int seg = 0; seg < 7; ++seg) {
			SegRect rect = ComputeSegmentRect(seg, ox, kScoreScreenY, kDigitWidth, kDigitHeight, kDigitThickness);
			Sprite* segment = Sprite::Create(textureHandle_, {rect.x, rect.y}, kScoreColor);
			segment->SetSize({rect.w, rect.h});
			scoreSegments_.push_back(segment);
		}
	}

	if (!promptSprite_) {
		promptSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.95f, 0.9f, 0.6f, 1.0f});
		promptSprite_->SetSize({kPromptWidth, kPromptHeight});
	}
	promptSprite_->SetPosition({kScreenWidth * 0.5f - kPromptWidth * 0.5f, kPromptY});
}

void ClearScene::Update() {
	Input* input = Input::GetInstance();
	constexpr float kDeltaTime = 1.0f / 60.0f;

	blinkTimer_ += kDeltaTime;
	if (blinkTimer_ >= kBlinkInterval) {
		blinkTimer_ -= kBlinkInterval;
		blinkVisible_ = !blinkVisible_;
	}

	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
		finished_ = true;
	}
}

void ClearScene::Draw() {
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (background_) {
		background_->Draw();
	}

	DrawScore();

	if (blinkVisible_ && promptSprite_) {
		promptSprite_->Draw();
	}

	Sprite::PostDraw();
}

void ClearScene::DrawScore() {
	int value = finalScore_;
	if (value < 0) {
		value = 0;
	}
	int maxValue = 1;
	for (int i = 0; i < kScoreDigitCount; ++i) {
		maxValue *= 10;
	}
	if (value > maxValue - 1) {
		value = maxValue - 1; // clamp display only; finalScore_ itself is never touched
	}

	int digits[kScoreDigitCount];
	int remaining = value;
	for (int i = kScoreDigitCount - 1; i >= 0; --i) {
		digits[i] = remaining % 10;
		remaining /= 10;
	}

	bool leadingZero = true;
	for (int i = 0; i < kScoreDigitCount; ++i) {
		bool isLastDigit = (i == kScoreDigitCount - 1);
		if (digits[i] == 0 && leadingZero && !isLastDigit) {
			continue; // blank leading zeroes, e.g. show "7" instead of "0007"
		}
		leadingZero = false;

		const bool* segmentsOn = kDigitSegments[digits[i]];
		for (int seg = 0; seg < 7; ++seg) {
			Sprite* sprite = scoreSegments_[static_cast<size_t>(i) * 7 + static_cast<size_t>(seg)];
			if (segmentsOn[seg] && sprite) {
				sprite->Draw();
			}
		}
	}
}