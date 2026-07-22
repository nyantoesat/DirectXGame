#include "Player.h"
#include "Enemy.h"
#include "MapChipField.h"
#include "MathUtility.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// イージング関数
static float EaseOut(float start, float end, float t) {
	t = 1.0f - (1.0f - t) * (1.0f - t);
	return start + (end - start) * t;
}

static float EaseIn(float start, float end, float t) {
	t = t * t;
	return start + (end - start) * t;
}

void Player::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {
	assert(model);
	assert(camera);

	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
}

void Player::InputMove() {
	if (onGround_) {

		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration = {};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
				acceleration.x += kAcceleration;
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
				acceleration.x -= kAcceleration;
			}
			velocity_.x += acceleration.x;
			velocity_.y += acceleration.y;
			velocity_.z += acceleration.z;
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}

		if (Input::GetInstance()->PushKey(DIK_UP)) {
			velocity_.y += kJumpAcceleration;
		}
	} else {

		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration = {};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
				acceleration.x += kAcceleration;
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
				acceleration.x -= kAcceleration;
			}
			velocity_.x += acceleration.x;
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		}

		velocity_.y -= kGravityAcceleration;
		velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);
	}
}

void Player::CheckMapCollision(CollisionMapInfo& info) {
	CheckMapCollisionUp(info);
	CheckMapCollisionDown(info);
	CheckMapCollisionRight(info);
	CheckMapCollisionLeft(info);
}

void Player::CheckMapCollisionUp(CollisionMapInfo& info) {
	if (info.move.y <= 0) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ下）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ下）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(0, kHeight / 2.0f, 0));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.y = (std::max)(0.0f, rect.bottom - worldTransform_.translation_.y - (kHeight / 2.0f + kBlank));
		info.ceiling = true;
	}
}

void Player::CheckMapCollisionDown(CollisionMapInfo& info) {
	// 下方向に移動していない場合は判定しない
	if (info.move.y >= 0) {
		return;
	}

	// 移動後の四隅の座標を計算
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 左下点の判定（微小な値だけ下にずらして吸着判定）
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom] + Vector3(0, -kGroundSearchOffset, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ上）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 右下点の判定（微小な値だけ下にずらして吸着判定）
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom] + Vector3(0, -kGroundSearchOffset, 0));
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ上）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// 下方向にヒットしたブロックの上面にスナップする
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move - Vector3(0, kHeight / 2.0f, 0));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.y = (std::min)(0.0f, rect.top - worldTransform_.translation_.y + (kHeight / 2.0f + kBlank));
		// 着地フラグを立てる
		info.landing = true;
	}
}

void Player::CheckMapCollisionRight(CollisionMapInfo& info) {
	// 右方向に移動していない場合は判定しない
	if (info.move.x <= 0) {
		return;
	}

	// 移動後の四隅の座標を計算
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 右上点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ左）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ左）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// 右方向にヒットしたブロックの左面にスナップする
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move + Vector3(kWidth / 2.0f, 0, 0));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = (std::max)(0.0f, rect.left - worldTransform_.translation_.x - (kWidth / 2.0f + kBlank));
		info.hitWall = true;
	}
}

void Player::CheckMapCollisionLeft(CollisionMapInfo& info) {
	// 左方向に移動していない場合は判定しない
	if (info.move.x >= 0) {
		return;
	}

	// 移動後の四隅の座標を計算
	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;

	// 左上点の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ右）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 左下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	// 隣接セル（1つ右）がブロックでなければヒット（セル境界をまたいだ場合のみ）
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// 左方向にヒットしたブロックの右面にスナップする
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + info.move - Vector3(kWidth / 2.0f, 0, 0));
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = (std::min)(0.0f, rect.right - worldTransform_.translation_.x + (kWidth / 2.0f + kBlank));
		info.hitWall = true;
	}
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void Player::ReflectCollisionResult(const CollisionMapInfo& info) { worldTransform_.translation_ += info.move; }

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	if (info.ceiling) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0;
	}
}

void Player::HandleLandingCollision(const CollisionMapInfo& info) {
	if (info.landing) {
		// 着地状態に切り替える（落下を止める）
		onGround_ = true;
		// 着地時にX速度を減衰
		velocity_.x *= (1.0f - kAttenuationLanding);
		// Y速度をゼロにする
		velocity_.y = 0.0f;
	}
}

void Player::HandleWallCollision(const CollisionMapInfo& info) {
	// 壁接触による減速
	if (info.hitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::UpdateOnGroundState(const CollisionMapInfo& info) {
	if (onGround_) {
		// ジャンプ開始：Y速度が上向きになったら空中状態に切り替え
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 地上にいながら落下判定：下にブロックがなければ空中状態に切り替え
			if (!info.landing) {
				// 下方向の床判定をあらためて行う
				CollisionMapInfo groundCheck;
				groundCheck.move = Vector3(0, -kGroundSearchOffset, 0);
				CheckMapCollisionDown(groundCheck);
				if (!groundCheck.landing) {
					onGround_ = false;
				}
			}
		}
	}
}

void Player::Update() {

	// ビヘイビア遷移（Update先頭に差し込む）
	if (behaviorRequest_ != Behavior::kUnknown) {
		// 振るまいを変更する
		behavior_ = behaviorRequest_;
		// 各振るまいごとの初期化を実行
		switch (behavior_) {
		case Behavior::kRoot:
		default:
			BehaviorRootInitialize();
			break;
		case Behavior::kAttack:
			BehaviorAttackInitialize();
			break;
		}
		// 振るまいリクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;
	}

	// 現在のビヘイビアに応じた毎フレームの処理
	switch (behavior_) {
	case Behavior::kRoot:
	default:
		BehaviorRootUpdate();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpdate();
		break;
	}

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		if (turnTimer_ < 0.0f) {
			turnTimer_ = 0.0f;
		}

		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,
		    std::numbers::pi_v<float> * 3.0f / 2.0f,
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		float t = 1.0f - (turnTimer_ / kTimeTurn);
		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, t);
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::BehaviorRootInitialize() {
	// 通常行動初期化（必要に応じてリセット処理を追加）
}

void Player::BehaviorAttackInitialize() {
	// カウンター初期化
	attackParameter_ = 0;
	// 攻撃フェーズを溜めから開始
	attackPhase_ = AttackPhase::溜め;
	// velocity_をゼロクリア
	velocity_ = {};
}

void Player::BehaviorRootUpdate() {
	InputMove();

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;

	CheckMapCollision(collisionMapInfo);
	ReflectCollisionResult(collisionMapInfo);
	HandleCeilingCollision(collisionMapInfo);
	HandleLandingCollision(collisionMapInfo);
	HandleWallCollision(collisionMapInfo);
	UpdateOnGroundState(collisionMapInfo);

	// 攻撃キーを押したら攻撃ビヘイビアをリクエスト
	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		// 攻撃ビヘイビアをリクエスト
		behaviorRequest_ = Behavior::kAttack;
	}
}

void Player::BehaviorAttackUpdate() {
	// 攻撃動作用の速度
	Vector3 velocity{};

	// 攻撃フェーズごとの更新処理
	switch (attackPhase_) {
	// 溜め動作
	case AttackPhase::溜め:
	default: {
		float t = static_cast<float>(attackParameter_) / kChargeTime;
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
		// 前進動作へ移行
		if (attackParameter_ >= kChargeTime) {
			attackPhase_ = AttackPhase::突進;
			attackParameter_ = 0; // カウンターをリセット
		}
		break;
	}
	// 突進動作
	case AttackPhase::突進: {
		float t = static_cast<float>(attackParameter_) / kDashTime;
		worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EaseIn(1.6f, 0.7f, t);
		// 向きに応じて突進速度を設定
		if (lrDirection_ == LRDirection::kRight) {
			velocity = {+kAttackVelocity, 0, 0};
		} else {
			velocity = {-kAttackVelocity, 0, 0};
		}
		// 余韻動作へ移行
		if (attackParameter_ >= kDashTime) {
			attackPhase_ = AttackPhase::余韻;
			attackParameter_ = 0;
		}
		break;
	}
	// 余韻動作
	case AttackPhase::余韻: {
		float t = static_cast<float>(attackParameter_) / kAfterTime;
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
		// 余韻が終わったら通常状態に戻す
		if (attackParameter_ >= kAfterTime) {
			behaviorRequest_ = Behavior::kRoot;
		}
		break;
	}
	}

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity;

	CheckMapCollision(collisionMapInfo);
	ReflectCollisionResult(collisionMapInfo);
	HandleCeilingCollision(collisionMapInfo);
	HandleLandingCollision(collisionMapInfo);
	HandleWallCollision(collisionMapInfo);
	UpdateOnGroundState(collisionMapInfo);

	// 予備動作カウンターを進める
	attackParameter_++;
}

void Player::Draw() {
	// 死亡中は描画しない
	if (isDead_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_, textureHandle_);
}

Vector3 Player::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::OnCollision(const Enemy* enemy) {
	(void)enemy;
	// デスフラグを立てる
	isDead_ = true;
}