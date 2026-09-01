#include "Player.h"
#include "MathUtility.h"
#include <cmath>

using namespace KamataEngine;

void Player::Initialize(Model* model, Model* bulletModel, Model* hpBarModel, Camera* camera, float startLocalX, float startLocalDepth, uint32_t shieldTextureHandle) {
	model_ = model;
	bulletModel_ = bulletModel;
	hpBarModel_ = hpBarModel;
	camera_ = camera;
	shieldTextureHandle_ = shieldTextureHandle;

	localX_ = startLocalX;
	localDepth_ = startLocalDepth;

	worldTransform_.Initialize();
	worldTransform_.translation_ = {localX_, kFixedHeight, kForwardOffset};

	worldTransformHpBar_.Initialize();

	for (int i = 0; i < kMaxBullets; i++) {
		bullets_[i].worldTransform.Initialize();
		bullets_[i].worldTransform.scale_ = {kNormalBulletScale, kNormalBulletScale, kNormalBulletScale};
		bullets_[i].isActive = false;
		bullets_[i].isCharged = false;
	}

	maxHp_ = 5;
	hp_ = maxHp_;
	damageTimer_ = 0;
	isAlive_ = true;

	dashTimer_ = 0;
	dashCooldownTimer_ = 0;
	chargeTimer_ = 0;
	wasShootKeyDown_ = false;

	shieldTimer_ = 0;
	shieldCooldownTimer_ = 0;
}

void Player::Update(float trackDistance) {
	if (isAlive_) {
		UpdateDash();
		UpdateShield();
		UpdateMove();
		UpdateShoot();
	}

	// カメラの前方一定距離を基準に、ローカルオフセット分だけ前後にずれる
	worldTransform_.translation_.x = localX_;
	worldTransform_.translation_.y = kFixedHeight;
	worldTransform_.translation_.z = trackDistance + kForwardOffset + localDepth_;

	UpdateBullets();

	if (damageTimer_ > 0)
		damageTimer_--;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	UpdateHpBar();
}

void Player::UpdateDash() {
	Input* input = Input::GetInstance();

	if (dashCooldownTimer_ > 0)
		dashCooldownTimer_--;

	if (dashTimer_ > 0) {
		dashTimer_--;
	} else if (input->TriggerKey(DIK_LSHIFT) && dashCooldownTimer_ <= 0) {
		dashTimer_ = kDashDuration;
		dashCooldownTimer_ = kDashCooldown;
	}
}

void Player::UpdateShield() {
	Input* input = Input::GetInstance();

	if (shieldTimer_ > 0) {
		shieldTimer_--;
		if (shieldTimer_ == 0) {
			shieldCooldownTimer_ = kShieldCooldown; // 発動が終わったらクールダウン開始
		}
	} else if (shieldCooldownTimer_ > 0) {
		shieldCooldownTimer_--;
	} else if (input->TriggerKey(DIK_F)) {
		shieldTimer_ = kShieldDuration;
	}
}

void Player::UpdateMove() {
	Input* input = Input::GetInstance();

	// ==== 画面内での移動 (W/S:奥/手前、A/D:左右) ====
	float dirX = 0.0f;
	float dirDepth = 0.0f;
	if (input->PushKey(DIK_W))
		dirDepth += 1.0f;
	if (input->PushKey(DIK_S))
		dirDepth -= 1.0f;
	if (input->PushKey(DIK_A))
		dirX -= 1.0f;
	if (input->PushKey(DIK_D))
		dirX += 1.0f;

	float length = sqrtf(dirX * dirX + dirDepth * dirDepth);
	if (length != 0.0f) {
		dirX /= length;
		dirDepth /= length;
	}

	float moveSpeed = speed_ * (IsDashing() ? kDashSpeedMultiplier : 1.0f);

	localX_ += dirX * moveSpeed;
	localDepth_ += dirDepth * moveSpeed;
}

void Player::UpdateShoot() {
	Input* input = Input::GetInstance();

	bool shootKeyDown = input->PushKey(DIK_SPACE);

	if (shootKeyDown) {
		chargeTimer_++;
	}

	// スペースキーを離した瞬間に発射(押していた時間で溜め撃ち判定)
	if (wasShootKeyDown_ && !shootKeyDown) {
		bool charged = chargeTimer_ >= kChargeThreshold;

		for (int i = 0; i < kMaxBullets; i++) {
			if (!bullets_[i].isActive) {
				bullets_[i].worldTransform.translation_ = worldTransform_.translation_;
				bullets_[i].isActive = true;
				bullets_[i].isCharged = charged;
				bullets_[i].worldTransform.scale_ =
				    charged ? Vector3{kChargedBulletScale, kChargedBulletScale, kChargedBulletScale} : Vector3{kNormalBulletScale, kNormalBulletScale, kNormalBulletScale};
				break;
			}
		}

		chargeTimer_ = 0;
	}

	wasShootKeyDown_ = shootKeyDown;
}

void Player::UpdateBullets() {
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets_[i].isActive)
			continue;

		float speed = bullets_[i].isCharged ? kChargedBulletSpeed : bulletSpeed_;
		bullets_[i].worldTransform.translation_.z += speed;

		if (bullets_[i].worldTransform.translation_.z > worldTransform_.translation_.z + kBulletMaxDistance) {
			bullets_[i].isActive = false;
			continue;
		}

		bullets_[i].worldTransform.matWorld_ = MakeAffineMatrix(bullets_[i].worldTransform.scale_, bullets_[i].worldTransform.rotation_, bullets_[i].worldTransform.translation_);
		bullets_[i].worldTransform.TransferMatrix();
	}
}

void Player::UpdateHpBar() {
	float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	if (hpRatio < 0.0f)
		hpRatio = 0.0f;
	worldTransformHpBar_.scale_ = {kHpBarWidth * hpRatio, kHpBarHeight, kHpBarDepth};
	worldTransformHpBar_.translation_ = {worldTransform_.translation_.x, worldTransform_.translation_.y + kHpBarYOffset, worldTransform_.translation_.z};
	worldTransformHpBar_.matWorld_ = MakeAffineMatrix(worldTransformHpBar_.scale_, worldTransformHpBar_.rotation_, worldTransformHpBar_.translation_);
	worldTransformHpBar_.TransferMatrix();
}

void Player::Draw() {
	if (isAlive_) {
		if (shieldTimer_ > 0) {
			// シールド発動中は青いテクスチャで表示する
			model_->Draw(worldTransform_, *camera_, shieldTextureHandle_);
		} else {
			model_->Draw(worldTransform_, *camera_);
		}
		hpBarModel_->Draw(worldTransformHpBar_, *camera_);
	}

	for (int i = 0; i < kMaxBullets; i++) {
		if (bullets_[i].isActive) {
			bulletModel_->Draw(bullets_[i].worldTransform, *camera_);
		}
	}
}

void Player::TakeDamage() {
	if (IsInvulnerable())
		return;
	if (damageTimer_ > 0)
		return;

	hp_--;
	damageTimer_ = kDamageFlashDuration;
	if (hp_ <= 0) {
		isAlive_ = false;
	}
}

AABB Player::GetAABB() const {
	Vector3 pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kRadius, pos.y - kRadius, pos.z - kRadius};
	aabb.max = {pos.x + kRadius, pos.y + kRadius, pos.z + kRadius};
	return aabb;
}