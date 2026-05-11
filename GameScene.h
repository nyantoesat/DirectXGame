#pragma once
#include "KamataEngine.h"

#include <vector>

class GameScene {
public:
	~GameScene();


	void Initialize();

	void Update();

	void Draw();
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

private:
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr; 
	uint32_t textureHandle_ = 0;
	KamataEngine::Camera camera_;
	
	
};