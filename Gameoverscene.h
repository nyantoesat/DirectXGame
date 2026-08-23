#pragma once
#include "IScene.h"
#include "MathUtility.h"

// ゲームオーバー(LOSE)シーン
class GameOverScene : public IScene {
private:
	KamataEngine::Model* modelGameOver_ = nullptr;
	KamataEngine::WorldTransform worldTransformGameOver_;

public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	~GameOverScene();
};

inline void GameOverScene::Initialize() {
	using namespace KamataEngine;

	modelGameOver_ = Model::CreateFromOBJ("cube", true);

	worldTransformGameOver_.Initialize();
	worldTransformGameOver_.translation_ = {0.0f, 1.0f, 8.0f};

	camera_->translation_ = {0.0f, 3.0f, 0.0f};
	camera_->rotation_ = {0.2f, 0.0f, 0.0f};
}

inline void GameOverScene::Update() {
	using namespace KamataEngine;

	worldTransformGameOver_.rotation_.x += 0.03f;
	worldTransformGameOver_.matWorld_ = MakeAffineMatrix(worldTransformGameOver_.scale_, worldTransformGameOver_.rotation_, worldTransformGameOver_.translation_);
	worldTransformGameOver_.TransferMatrix();

	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_R)) {
		sceneNo = TITLE;
	}

	// HUD(ImGuiのリンクエラーを解消するまで一旦コメントアウト)
	// GAME OVER / PRESS R TO TITLE
}

inline void GameOverScene::Draw() { modelGameOver_->Draw(worldTransformGameOver_, *camera_); }

inline GameOverScene::~GameOverScene() { delete modelGameOver_; }