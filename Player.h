#pragma once
#include "KamataEngine.h"

class Player {
public:
	void Initialize(KamataEngine::Model* model,uint32_t textureHandle,KamataEngine::Camera* camera);
	void Update();
	void Draw();

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::Model* model_ = nullptr;

	uint32_t textureHandle_ = 0;

};