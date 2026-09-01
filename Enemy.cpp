#include "Enemy.h"
#include "MathUtility.h"
#include <cmath>
#include <cstdlib>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Model* bulletModel, Model* hpBarModel, Camera* camera, const Vector3& spawnPosition, int waveNumber, uint32_t hitTextureHandle) {
	model_ = model;
	bulletModel_ = bulletModel;
	hpBarModel_ = hpBarModel;
	camera_ = camera;
	hitTextureHandle_ = hitTextureHandle;

	basePositionX_ = spawnPosition.x;
	basePositionY_ = spawnPosition.y;
	currentOffsetX_ = 0.0f;
	currentOffsetY_ = 0.0f;
	targetOffsetX_ = 0.0f;
	targetOffsetY_ = 0.0f;
	randomMoveTimer_ = 0;
	aheadDistance_ = spawnPosition.z; // 呼び出し側が「現在のトラック位置からの距離」を渡す想定

	worldTransform_.Initialize();
	worldTransform_.translation_ = spawnPosition;
	worldTransform_.rotation_.y = 3.14159265f; // プレイヤー側(手前・-Z方向)を向く

	worldTransformHpBar_.Initialize();

	for (int i = 0; i < kMaxBullets; i++) {
		bullets_[i].worldTransform.Initialize();
		bullets_[i].worldTransform.scale_ = {0.22f, 0.22f, 0.22f};
		bullets_[i].isActive = false;
	}

	// ウェーブが進むほど体力と動きの速さが上がる
	int wave = waveNumber < 1 ? 1 : waveNumber;
	maxHp_ = 5 + (wave - 1) * 3; // 難易度調整のためHPを約半分に
	hp_ = maxHp_;
	// ウェーブが進むほど動きが少し速くなる
	moveLerpRateMultiplier_ = 1.0f + (wave - 1) * 0.3f;

	damageTimer_ = 0;
	isAlive_ = true;
	shootTimer_ = 0;
}

void Enemy::Update(const Vector3& targetPosition, float trackDistance) {
	if (isAlive_) {
		UpdateRandomMove();
		UpdateShoot(targetPosition);
	}

	// カメラ(トラック)と一緒に前進し、常に一定距離だけ先に留まる
	worldTransform_.translation_.z = trackDistance + aheadDistance_;

	UpdateBullets();

	if (damageTimer_ > 0)
		damageTimer_--;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	UpdateHpBar();
}

void Enemy::UpdateRandomMove() {
	// 一定時間ごとに新しいランダムな目標地点を選ぶ
	randomMoveTimer_--;
	if (randomMoveTimer_ <= 0) {
		// -1.0~1.0のランダム値で目標オフセットを決める
		targetOffsetX_ = ((rand() % 2000) / 1000.0f - 1.0f) * kMoveRangeX;
		targetOffsetY_ = ((rand() % 2000) / 1000.0f - 1.0f) * kMoveRangeY;
		randomMoveTimer_ = kMoveChangeIntervalMin + rand() % (kMoveChangeIntervalMax - kMoveChangeIntervalMin);
	}

	// 目標地点へ滑らかに近づく
	float lerpRate = kMoveLerpRate * moveLerpRateMultiplier_;
	currentOffsetX_ += (targetOffsetX_ - currentOffsetX_) * lerpRate;
	currentOffsetY_ += (targetOffsetY_ - currentOffsetY_) * lerpRate;

	worldTransform_.translation_.x = basePositionX_ + currentOffsetX_;
	worldTransform_.translation_.y = basePositionY_ + currentOffsetY_;
}

void Enemy::UpdateShoot(const Vector3& targetPosition) {
	shootTimer_++;
	if (shootTimer_ <= kShootInterval)
		return;

	shootTimer_ = 0;
	FireAimedShot(targetPosition);
}

void Enemy::FireAimedShot(const Vector3& targetPosition) {
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets_[i].isActive) {
			float dx = targetPosition.x - worldTransform_.translation_.x;
			float dz = targetPosition.z - worldTransform_.translation_.z;
			float mag = sqrtf(dx * dx + dz * dz);
			if (mag != 0.0f) {
				dx /= mag;
				dz /= mag;
			}

			bullets_[i].worldTransform.translation_ = worldTransform_.translation_;
			bullets_[i].dirX = dx;
			bullets_[i].dirZ = dz;
			bullets_[i].speed = 0.18f;
			bullets_[i].isActive = true;
			break;
		}
	}
}

void Enemy::UpdateBullets() {
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets_[i].isActive)
			continue;

		bullets_[i].worldTransform.translation_.x += bullets_[i].dirX * bullets_[i].speed;
		bullets_[i].worldTransform.translation_.z += bullets_[i].dirZ * bullets_[i].speed;

		float dx = bullets_[i].worldTransform.translation_.x - basePositionX_;
		float dz = bullets_[i].worldTransform.translation_.z - worldTransform_.translation_.z;
		if (dx * dx + dz * dz > 30.0f * 30.0f) {
			bullets_[i].isActive = false;
			continue;
		}

		bullets_[i].worldTransform.matWorld_ = MakeAffineMatrix(bullets_[i].worldTransform.scale_, bullets_[i].worldTransform.rotation_, bullets_[i].worldTransform.translation_);
		bullets_[i].worldTransform.TransferMatrix();
	}
}

void Enemy::UpdateHpBar() {
	float hpRatio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
	if (hpRatio < 0.0f)
		hpRatio = 0.0f;
	worldTransformHpBar_.scale_ = {kHpBarWidth * hpRatio, kHpBarHeight, kHpBarDepth};
	worldTransformHpBar_.translation_ = {worldTransform_.translation_.x, worldTransform_.translation_.y + kHpBarYOffset, worldTransform_.translation_.z};
	worldTransformHpBar_.matWorld_ = MakeAffineMatrix(worldTransformHpBar_.scale_, worldTransformHpBar_.rotation_, worldTransformHpBar_.translation_);
	worldTransformHpBar_.TransferMatrix();
}

void Enemy::Draw() {
	if (isAlive_) {
		if (damageTimer_ > 0) {
			// 被弾後1秒間は赤いテクスチャで表示する
			model_->Draw(worldTransform_, *camera_, hitTextureHandle_);
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

void Enemy::TakeDamage() {
	if (damageTimer_ > 0)
		return;

	hp_--;
	damageTimer_ = kDamageFlashDuration;
	if (hp_ <= 0) {
		isAlive_ = false;
	}
}

AABB Enemy::GetAABB() const {
	Vector3 pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kRadius, pos.y - kRadius, pos.z - kRadius};
	aabb.max = {pos.x + kRadius, pos.y + kRadius, pos.z + kRadius};
	return aabb;
}