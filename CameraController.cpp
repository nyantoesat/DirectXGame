#include "CameraController.h"

using namespace KamataEngine;

void CameraController::Initialize() {
	camera_.Initialize();
	camera_.translation_ = {0.0f, kHeight, kBehindOffset};
	camera_.rotation_ = {kPitch, 0.0f, 0.0f};
	camera_.UpdateMatrix();

	goalX_ = 0.0f;
	goalZ_ = kBehindOffset;
}

void CameraController::Update(float trackDistance, float playerLocalX, float playerLocalDepth, bool isDashing) {
	// プレイヤーの左右・前後の移動に追従する(これがカメラワークの反応そのもの)
	goalX_ = playerLocalX * kParallaxX;
	goalZ_ = trackDistance + kBehindOffset + playerLocalDepth * kParallaxDepth;

	camera_.translation_.x += (goalX_ - camera_.translation_.x) * kPositionLerpRate;
	camera_.translation_.z += (goalZ_ - camera_.translation_.z) * kPositionLerpRate;

	// ダッシュ中はわずかに前のめりにして疾走感を演出
	camera_.rotation_.x = isDashing ? (kPitch + kDashPitchBoost) : kPitch;

	camera_.UpdateMatrix();
}