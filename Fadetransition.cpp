#include "FadeTransition.h"
#include <algorithm>

using namespace KamataEngine;

FadeTransition::~FadeTransition() { delete overlay_; }

void FadeTransition::Initialize() {
	textureHandle_ = TextureManager::Load("white1x1.png");

	overlay_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f});
	overlay_->SetSize({kScreenWidth, kScreenHeight});

	state_ = State::kIdle;
	timer_ = 0.0f;
}

void FadeTransition::StartFadeOut() {
	if (state_ != State::kIdle) {
		return; // already mid-transition somewhere - don't stomp on it
	}
	state_ = State::kFadingOut;
	timer_ = 0.0f;
}

void FadeTransition::StartFadeIn() {
	if (state_ != State::kHoldOpaque) {
		return; // only meant to follow a completed fade-out
	}
	state_ = State::kFadingIn;
	timer_ = 0.0f;
}

void FadeTransition::Update() {
	constexpr float kDeltaTime = 1.0f / 60.0f;
	float alpha = 0.0f;

	switch (state_) {
	case State::kIdle:
		alpha = 0.0f;
		break;

	case State::kFadingOut:
		timer_ += kDeltaTime;
		alpha = std::min(timer_ / kFadeDuration, 1.0f);
		if (timer_ >= kFadeDuration) {
			state_ = State::kHoldOpaque;
			alpha = 1.0f;
		}
		break;

	case State::kHoldOpaque:
		alpha = 1.0f;
		break;

	case State::kFadingIn:
		timer_ += kDeltaTime;
		alpha = std::max(1.0f - timer_ / kFadeDuration, 0.0f);
		if (timer_ >= kFadeDuration) {
			state_ = State::kIdle;
			alpha = 0.0f;
		}
		break;
	}

	if (overlay_) {
		overlay_->SetColor({0.0f, 0.0f, 0.0f, alpha});
	}
}

void FadeTransition::Draw() {
	if (state_ == State::kIdle || !overlay_) {
		return; // fully transparent - nothing to draw
	}

	// Own PreDraw/PostDraw pair, same as every other scene's Draw() - this
	// runs as an extra sprite-batch "layer" on top of whichever scene
	// already drew this frame, not instead of it.
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
	overlay_->Draw();
	Sprite::PostDraw();
}