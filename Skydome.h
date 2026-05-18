#pragma once
#include "KamataEngine.h"

class Skydome {
public:
	void Initialize();
	void Update();
	void Draw(KamataEngine::Camera& camera);
	//~Skydome();

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* modelSkydome_ = nullptr;
	uint32_t skytextureHandle_ = 0;
	
	
};