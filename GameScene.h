#pragma once
#include "CameraController.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include <vector>

class GameScene {
public:
	~GameScene();

	void Initialize();

	void Update();

	void Draw();

	void GenerateBlocks();

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

private:
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	Player* player_ = nullptr;
	Enemy* enemy_ = nullptr;
	uint32_t textureHandle_ = 0;
	uint32_t blockTextureHandle_ = 0;
	uint32_t enemyTextureHandle_ = 0;
	KamataEngine::Camera camera_;
	Skydome* skydome_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;
};