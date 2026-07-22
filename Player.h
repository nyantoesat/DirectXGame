#pragma once
#include "AABB.h"
#include "KamataEngine.h"

class MapChipField;
class Enemy;

class Player {
public:
	enum class LRDirection {
		kRight,
		kLeft,
	};

	// 振るまい
	enum class Behavior {
		kRoot,    // 通常状態
		kAttack,  // 攻撃中
		kUnknown, // リクエストなし
	};

	// 攻撃フェーズ（型）
	enum class AttackPhase {
		溜め,
		突進,
		余韻,
	};

	void Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// ワールド座標を取得
	KamataEngine::Vector3 GetWorldPosition();
	// AABBを取得
	AABB GetAABB();
	// 衝突応答
	void OnCollision(const Enemy* enemy);
	// デスフラグのgetter
	bool IsDead() const { return isDead_; }

private:
	struct CollisionMapInfo {
		bool ceiling = false;
		bool landing = false;
		bool hitWall = false;
		KamataEngine::Vector3 move = {};
	};

	enum Corner {
		kRightBottom,
		kLeftBottom,
		kRightTop,
		kLeftTop,
		kNumCorner,
	};

	void InputMove();

	void CheckMapCollision(CollisionMapInfo& info);
	void CheckMapCollisionUp(CollisionMapInfo& info);
	void CheckMapCollisionDown(CollisionMapInfo& info);
	void CheckMapCollisionRight(CollisionMapInfo& info);
	void CheckMapCollisionLeft(CollisionMapInfo& info);

	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	void ReflectCollisionResult(const CollisionMapInfo& info);
	void HandleCeilingCollision(const CollisionMapInfo& info);
	void HandleLandingCollision(const CollisionMapInfo& info);
	void HandleWallCollision(const CollisionMapInfo& info);
	void UpdateOnGroundState(const CollisionMapInfo& info);

	// ビヘイビアごとの初期化
	void BehaviorRootInitialize();
	void BehaviorAttackInitialize();

	// ビヘイビアごとの更新
	void BehaviorRootUpdate();
	void BehaviorAttackUpdate();

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelAttack_ = nullptr;
	KamataEngine::WorldTransform worldTransformAttack_;
	uint32_t textureHandle_ = 0u;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Vector3 velocity_ = {};

	LRDirection lrDirection_ = LRDirection::kRight;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	bool onGround_ = true;

	// デスフラグ
	bool isDead_ = false;

	// 振るまい
	Behavior behavior_ = Behavior::kRoot;
	// 次の振るまいリクエスト
	Behavior behaviorRequest_ = Behavior::kUnknown;

	// 攻撃ギミックの経過時間カウンター
	uint32_t attackParameter_ = 0;
	// 現在の攻撃フェーズ（変数）
	AttackPhase attackPhase_ = AttackPhase::溜め;

	MapChipField* mapChipField_ = nullptr;

	static inline const float kAcceleration = 0.01f;
	static inline const float kLimitRunSpeed = 0.2f;
	static inline const float kAttenuation = 0.1f;

	static inline const float kTimeTurn = 0.3f;

	static inline const float kGravityAcceleration = 0.05f;
	static inline const float kLimitFallSpeed = 0.5f;
	static inline const float kJumpAcceleration = 0.5f;

	static inline const float kAttenuationLanding = 0.1f;
	static inline const float kAttenuationWall = 0.1f;
	static inline const float kGroundSearchOffset = 0.06f;

	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;
	static inline const float kBlank = 0.04f;

	// 攻撃時間
	static inline const uint32_t kAttackTime = 60;
	// 溜め動作時間
	static inline const uint32_t kChargeTime = 20;
	// 突進動作時間
	static inline const uint32_t kDashTime = 10;
	// 余韻動作時間
	static inline const uint32_t kAfterTime = 20;
	// 攻撃速度
	static inline const float kAttackVelocity = 0.3f;
};