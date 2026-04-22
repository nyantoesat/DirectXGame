#pragma once
#include "KamataEngine.h"
#include "Player.h"

class GameScene
{
public:

	~GameScene();

	void Initialize();

	void Update();

	void Draw();

private:

	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;
	KamataEngine::Camera camera_;
	Player* player_ = nullptr;
};
