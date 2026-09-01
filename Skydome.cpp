#include "Skydome.h"
#include "MathUtility.h"

using namespace KamataEngine;

void Skydome::Initialize(Model* model) {
	model_ = model;

	worldTransform_.Initialize();
	worldTransform_.scale_ = {kScale, kScale, kScale};
	worldTransform_.translation_ = {0.0f, 0.0f, 0.0f};
}

void Skydome::Update() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Skydome::Draw(const Camera& camera) { model_->Draw(worldTransform_, camera); }