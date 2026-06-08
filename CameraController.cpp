#include "CameraController.h"
#include "MathUtility.h"
#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

void CameraController::Initialize() { camera_.Initialize(); }

void CameraController::SetTarget(Player* target) { target_ = target; }

void CameraController::Reset() {
	assert(target_);

	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	goalCoordinates_ = targetWorldTransform.translation_ + targetOffset_;
	camera_.translation_ = goalCoordinates_;

	camera_.UpdateMatrix();
}

void CameraController::Update() {
	assert(target_);

	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	if (mode_ == Mode::kForcedScroll) {
		camera_.translation_.x += kScrollSpeed;
		camera_.translation_.y = std::lerp(
			camera_.translation_.y, 
			targetWorldTransform.translation_.y + targetOffset_.y, 
			kInterpolationRate
		);

	} else {

		goalCoordinates_ = targetWorldTransform.translation_ + targetOffset_ + target_->GetVelocity() * kVelocityBias;

		camera_.translation_.x = std::lerp(camera_.translation_.x, goalCoordinates_.x, kInterpolationRate);
		camera_.translation_.y = std::lerp(camera_.translation_.y, goalCoordinates_.y, kInterpolationRate);
		camera_.translation_.z = std::lerp(camera_.translation_.z, goalCoordinates_.z, kInterpolationRate);

		camera_.translation_.x = (std::max)(camera_.translation_.x, targetWorldTransform.translation_.x + kMargin.left);
		camera_.translation_.x = (std::min)(camera_.translation_.x, targetWorldTransform.translation_.x + kMargin.right);
		camera_.translation_.y = (std::max)(camera_.translation_.y, targetWorldTransform.translation_.y + kMargin.bottom);
		camera_.translation_.y = (std::min)(camera_.translation_.y, targetWorldTransform.translation_.y + kMargin.top);
     }
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

	camera_.UpdateMatrix();
}