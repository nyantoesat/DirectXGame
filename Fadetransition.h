#pragma once

// See the matching comment in GameScene.h: KamataEngine pulls in
// <Windows.h> transitively, which (without this) defines min/max as
// macros and silently mangles every std::min/std::max call below into
// invalid syntax. Must be defined before the first include of
// KamataEngine.h anywhere in a translation unit - since this header is
// included first by FadeTransition.cpp, it lives here.
#define NOMINMAX

#include "KamataEngine.h"

/// <summary>
/// A full-screen black overlay that fades in/out over kFadeDuration
/// seconds, used to smooth out a scene switch. Built for the Title ->
/// Game handoff, but StartFadeOut()/StartFadeIn() are generic enough to
/// reuse for Game -> Clear or Clear -> Title later if wanted.
///
/// Unlike TitleScene/GameScene/ClearScene (which get newed/deleted every
/// time the active scene changes), this is meant to be a single instance
/// that lives for the whole app and has Update()/Draw() called on it
/// every frame regardless of which scene is active - Draw() after the
/// active scene's own Draw(), so the overlay ends up on top.
/// </summary>
class FadeTransition {
public:
	~FadeTransition();

	void Initialize();
	void Update();
	void Draw();

	// Starts transparent -> opaque black. No-op if a fade is already in
	// progress (idempotent, so callers don't need to track that
	// themselves).
	void StartFadeOut();

	// Starts opaque black -> transparent. Only takes effect right after a
	// fade-out has finished (see IsFadeOutComplete()); no-op otherwise.
	void StartFadeIn();

	// True once the overlay has reached full opacity and is holding
	// there, waiting for the caller to do the actual scene swap and then
	// call StartFadeIn(). Check this once per frame right after Update().
	bool IsFadeOutComplete() const { return state_ == State::kHoldOpaque; }

private:
	enum class State {
		kIdle,       // fully transparent, doing nothing
		kFadingOut,  // transparent -> opaque
		kHoldOpaque, // sitting fully opaque - caller should swap scenes now
		kFadingIn,   // opaque -> transparent
	};

	static constexpr float kScreenWidth = 1280.0f;
	static constexpr float kScreenHeight = 720.0f;
	static constexpr float kFadeDuration = 0.35f; // seconds, one direction

	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* overlay_ = nullptr;
	State state_ = State::kIdle;
	float timer_ = 0.0f;
};