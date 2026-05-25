#pragma once
#include "KamataEngine.h"

class Player;

struct Rect {
	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;
};

class CameraController {
public:
	void Initialize();

	void Update();

	void SetTarget(Player* target);

	void Reset();

	void SetMovableArea(Rect area) { movableArea_ = area; }

	KamataEngine::Camera& GetCamera() { return camera_; }

private:
	KamataEngine::Camera camera_;

	Player* target_ = nullptr;

	KamataEngine::Vector3 targetOffset_ = {0, 0, -10.0f};

	KamataEngine::Vector3 goalCoordinates_ = {};

	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};

	static inline const float kInterpolationRate = 0.1f;

	static inline const float kVelocityBias = 15.0f;

	static inline const Rect kMargin = {-3.0f, 3.0f, -2.0f, 2.0f};
};