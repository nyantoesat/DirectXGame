#pragma once
#include "KamataEngine.h"



class GameScene
{
	

public:
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	// デストラクタ
private:
	uint32_t textureHandle_ = 0;
    uint32_t soundDataHandle_ = 0;
	uint32_t voiceHandle_ = 0;

	float inputFloat3[3] = {0, 0, 0};

	// スプライト
	//KamataEngine::Sprite* sprite_ = nullptr;

	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;

	//ワールドトランスフォーマー
	KamataEngine::WorldTransform worldTransform_;
	// カメラ
	KamataEngine::Camera camera_;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

};
