#define NOMINMAX
#include "Enemy.h"
#include "MapChipField.h"
#include "MathUtility.h"
#include "Player.h"
#include <array>
#include <cassert>
#include <numbers>

Enemy::Enemy() {}

void Enemy::Initialize(KamataEngine::Model* model, uint32_t textureHandleEnemy, KamataEngine::Camera* camera, Vector3& position) {
	walkTimer_ = 0.0f;

	assert(camera);
	camera_ = camera;
	assert(model);
	enemyModel_ = model;
	textureHandleEnemy_ = textureHandleEnemy;
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_ = {0.0f, -1.5f, 0.0f};
	velocity_ = {-kWalkSpeed, 0, 0};
}

void Enemy::update() {
	walkTimer_ += 1.0f / 60.0f;
	worldTransform_.translation_.x += velocity_.x;
	// rotation animation
	float param = std::sin(walkTimer_ * 3.0f);
	float degree = (param + 1.0f) / 2.0f;
	float startAngle = -kMaxRockAngle;
	float endAngle = +kMaxRockAngle;
	float lerpedDegree = startAngle + (endAngle - startAngle) * degree;
	worldTransform_.rotation_.x = lerpedDegree * (std::numbers::pi_v<float> / 180.0f);

	// Reset onGround each frame
	onGround_ = false;

	// Gravity
	velocity_.y -= kGravityAcceleration;
	velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);

	// Setup collision
	CollisionMapInfo info;
	info.velocityAfterCollision = velocity_;

	// Run collision checks
	CollisionMap(info);

	// Apply position
	CollisionDetected(info);

	// Apply velocity result
	ApplyCollisionResult(info);

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Enemy::draw() { enemyModel_->Draw(worldTransform_, *camera_, textureHandleEnemy_); }

void Enemy::CollisionMap(CollisionMapInfo& info) {
	CollisionMapTop(info);
	CollisionMapBottom(info);
	CollisionMapRight(info);
	CollisionMapLeft(info);
}

void Enemy::CollisionMapBottom(CollisionMapInfo& info) {
	if (info.velocityAfterCollision.y >= 0.0f)
		return;

	Vector3 moved = {worldTransform_.translation_.x, worldTransform_.translation_.y + info.velocityAfterCollision.y, worldTransform_.translation_.z};

	Vector3 leftBottom = CornerPosition(moved, kLeftBottom);
	Vector3 rightBottom = CornerPosition(moved, kRightBottom);

	bool hit = false;
	MapChipField::IndexSet hitIndex{};

	MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(leftBottom);
	if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitIndex = index;
	}

	index = mapChipField_->GetMapChipIndexSetByPosition(rightBottom);
	if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitIndex = index;
	}

	if (hit) {
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float correctY = rect.top + kHeight / 2.0f;
		info.velocityAfterCollision.y = correctY - worldTransform_.translation_.y;
		info.isHitDown = true;
	}
}

void Enemy::CollisionMapTop(CollisionMapInfo& info) {
	if (info.velocityAfterCollision.y <= 0)
		return;

	Vector3 moved = {worldTransform_.translation_.x, worldTransform_.translation_.y + info.velocityAfterCollision.y, worldTransform_.translation_.z};

	std::array<Vector3, kNumCorner> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++)
		positionNew[i] = CornerPosition(moved, static_cast<Corner>(i));

	bool hit = false;
	MapChipField::IndexSet indexSet;

	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock)
		hit = true;

	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock)
		hit = true;

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftTop]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		worldTransform_.translation_.y = rect.bottom - kHeight / 2.0f;
		info.velocityAfterCollision.y = 0.0f;
		info.isHitUp = true;
	}
}

void Enemy::CollisionMapRight(CollisionMapInfo& info) {
	if (info.velocityAfterCollision.x <= 0)
		return;

	Vector3 moved = {worldTransform_.translation_.x + info.velocityAfterCollision.x, worldTransform_.translation_.y, worldTransform_.translation_.z};

	std::array<Vector3, kNumCorner> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++)
		positionNew[i] = CornerPosition(moved, static_cast<Corner>(i));

	bool hit = false;
	MapChipField::IndexSet indexSet;

	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock)
		hit = true;

	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock)
		hit = true;

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightTop]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		worldTransform_.translation_.x = rect.left - kWidth / 2.0f;
		info.velocityAfterCollision.x = 0.0f;
		info.isHitRight = true;
	}
}

void Enemy::CollisionMapLeft(CollisionMapInfo& info) {
	if (info.velocityAfterCollision.x >= 0)
		return;

	Vector3 moved = {worldTransform_.translation_.x + info.velocityAfterCollision.x, worldTransform_.translation_.y, worldTransform_.translation_.z};

	std::array<Vector3, kNumCorner> positionNew;
	for (uint32_t i = 0; i < positionNew.size(); i++)
		positionNew[i] = CornerPosition(moved, static_cast<Corner>(i));

	bool hit = false;
	MapChipField::IndexSet indexSetTop, indexSetBottom;

	indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftTop]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex) == MapChipType::kBlock)
		hit = true;

	indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftBottom]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)
		hit = true;

	if (hit) {
		MapChipField::IndexSet indexSet = indexSetTop;
		if (mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex) == MapChipType::kBlock)
			indexSet = indexSetBottom;

		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		worldTransform_.translation_.x = rect.right + kWidth / 2.0f - 0.001f;
		info.velocityAfterCollision.x = 0.0f;
		info.isHitLeft = true;
	}
}

Vector3 Enemy::CornerPosition(const Vector3& center, Corner corner) {
	const float kAdjustX = 0.03f;
	const float kAdjustY = 0.03f;

	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f - kAdjustX, -kHeight / 2.0f + kAdjustY, 0},
	    {-kWidth / 2.0f + kAdjustX, -kHeight / 2.0f + kAdjustY, 0},
	    {+kWidth / 2.0f - kAdjustX, +kHeight / 2.0f - kAdjustY, 0},
	    {-kWidth / 2.0f + kAdjustX, +kHeight / 2.0f - kAdjustY, 0},
	};

	Vector3 offset = offsetTable[static_cast<uint32_t>(corner)];
	return {center.x + offset.x, center.y + offset.y, center.z + offset.z};
}

void Enemy::CollisionDetected(const CollisionMapInfo& info) {
	if (!info.isHitDown && !info.isHitUp)
		worldTransform_.translation_.y += info.velocityAfterCollision.y;
	if (!info.isHitLeft && !info.isHitRight)
		worldTransform_.translation_.x += info.velocityAfterCollision.x;
	worldTransform_.translation_.z += info.velocityAfterCollision.z;
}

void Enemy::ApplyCollisionResult(const CollisionMapInfo& info) {
	if (info.isHitUp)
		velocity_.y = 0.0f;
	if (info.isHitDown) {
		onGround_ = true;
		velocity_.y = 0.0f;
	}
	if (info.isHitLeft) {
		velocity_.x = +kWalkSpeed;
		worldTransform_.rotation_.y = +1.5f;
	}
	if (info.isHitRight) {
		velocity_.x = -kWalkSpeed;
		worldTransform_.rotation_.y = -1.5f;
	}
}

Vector3 Enemy::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Enemy::OnCollision(const Player* player) { (void)player; }