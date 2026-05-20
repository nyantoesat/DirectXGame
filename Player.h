#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"

class Player {
public:
	
	enum class LRDirection {
		kRight,
		kLeft,
	};

	void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

private:
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0u;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;

	KamataEngine::Vector3 velocity_ = {};

	
	LRDirection lrDirection_ = LRDirection::kRight;

	
	float turnFirstRotationY_ = 0.0f;
	
	float turnTimer_ = 0.0f;

	
	bool onGround_ = true;

	
	static inline const float kAcceleration = 0.01f;
	static inline const float kLimitRunSpeed = 0.2f;
	static inline const float kAttenuation = 0.1f;

	
	static inline const float kTimeTurn = 0.3f;

	
	static inline const float kGravityAcceleration = 0.05f;
	static inline const float kLimitFallSpeed = 0.5f;
	static inline const float kJumpAcceleration = 0.5f;
};