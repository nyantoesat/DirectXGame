#pragma once
#include "AABB.h"
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "MathUtility.h"
#include "Player.h"
#include "Skydome.h"
#include <list>
#include <vector>

class GameScene {
public:
	// ゲームのフェーズ（型）
	enum class Phase {
		kPlay,  // ゲームプレイ
		kDeath, // デス演出
	};

	~GameScene();

	void Initialize();

	void Update();

	void Draw();

	void GenerateBlocks();
	void CheckAllCollisions();
	void ChangePhase();
	void UpdatePlayPhase();
	void UpdateDeathPhase();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

private:
	bool isDebugCameraActive_ = false;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* modelBlock_ = nullptr;
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelPlayerAttack_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	Player* player_ = nullptr;
	std::list<Enemy*> enemies_;
	DeathParticles* deathParticles_ = nullptr;
	KamataEngine::Model* modelDeathParticles_ = nullptr;
	uint32_t textureHandle_ = 0;
	uint32_t blockTextureHandle_ = 0;
	uint32_t enemyTextureHandle_ = 0;
	KamataEngine::Camera camera_;
	Skydome* skydome_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	CameraController* cameraController_ = nullptr;
	// ゲームの現在フェーズ（変数）
	Phase phase_ = Phase::kPlay;
	// 終了フラグ
	bool finished_ = false;
	// フェード
	Fade* fade_ = nullptr;
};