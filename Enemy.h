#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
using namespace KamataEngine;

class Enemy {
public:
	float walkTimer_ = 0.0f;
	static inline const float kWalkAnimationPeriod = 1.0f; // seconds per cycle
	static inline const float kMaxRockAngle = 15.0f;       // degrees of rock
	Enemy();
	void Initialize(KamataEngine::Model* model, uint32_t textureHandleEnemy, KamataEngine::Camera* camera, Vector3& position);
	void update();
	void draw();
	void setMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:
	// Collision corners
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	struct CollisionMapInfo {
		bool isHitDown = false;
		bool isHitUp = false;
		bool isHitLeft = false;
		bool isHitRight = false;
		Vector3 velocityAfterCollision = {};
	};

	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void CollisionMap(CollisionMapInfo& info);
	void CollisionMapTop(CollisionMapInfo& info);
	void CollisionMapBottom(CollisionMapInfo& info);
	void CollisionMapRight(CollisionMapInfo& info);
	void CollisionMapLeft(CollisionMapInfo& info);
	void CollisionDetected(const CollisionMapInfo& info);
	void ApplyCollisionResult(const CollisionMapInfo& info);

	MapChipField* mapChipField_ = nullptr;
	WorldTransform worldTransform_;
	Model* enemyModel_ = nullptr;
	uint32_t textureHandleEnemy_ = 0;
	Camera* camera_ = nullptr;

	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	bool onGround_ = false;

	static inline const float kGravityAcceleration = 0.05f;
	static inline const float kLimitFallSpeed = 0.5f;
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;
	static inline const float kWalkSpeed = 0.02f;
	static inline const float kWalkMotionAngleStart = 1.0f;
	static inline const float kWalkMotionAngleEnd = 2.0f;
	static inline const float kWalkMotionTime = 0.5f;
};