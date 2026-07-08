#define NOMINMAX
#include "Fade.h"
#include <algorithm>

using namespace KamataEngine;

Fade::~Fade() { delete sprite_; }

void Fade::Initialize() {
	// スプライト生成
	sprite_ = Sprite::Create(0, Vector2(0, 0));
	// 画面全体を覆うサイズ
	sprite_->SetSize(Vector2(1280.0f, 720.0f));
	// 黒（RGBA: 0, 0, 0, 1）
	sprite_->SetColor(Vector4(0, 0, 0, 1));

	status_ = Status::None;
	counter_ = 0.0f;
}

void Fade::Update() {
	if (status_ == Status::None) {
		return;
	}

	// 1フレーム分の秒数をカウントアップ
	counter_ += 1.0f / 60.0f;

	// フェード継続時間に達したら打ち止め
	counter_ = std::min(counter_, duration_);

	if (status_ == Status::FadeOut) {
		// フェードアウト：0.0f → 1.0f（だんだん黒くなる）
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));
	} else if (status_ == Status::FadeIn) {
		// フェードイン：1.0f → 0.0f（だんだん見えてくる）
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(1.0f - counter_ / duration_, 0.0f, 1.0f)));
	}

	// フェード完了
	if (counter_ >= duration_) {
		status_ = Status::None;
	}
}

void Fade::Draw() {
	Sprite::PreDraw();
	sprite_->Draw();
	Sprite::PostDraw();
}

void Fade::StartFadeIn(float duration) {
	status_ = Status::FadeIn;
	counter_ = 0.0f;
	duration_ = duration;
	// 最初は完全に黒
	sprite_->SetColor(Vector4(0, 0, 0, 1));
}

void Fade::StartFadeOut(float duration) {
	status_ = Status::FadeOut;
	counter_ = 0.0f;
	duration_ = duration;
	// 最初は完全に透明
	sprite_->SetColor(Vector4(0, 0, 0, 0));
}