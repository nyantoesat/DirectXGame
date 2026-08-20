#pragma once
#include "IScene.h"

// クリア(WIN)シーン
class ClearScene : public IScene {
private:
	KamataEngine::Model* modelClear_ = nullptr;
	KamataEngine::WorldTransform worldTransformClear_;

public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	~ClearScene();
};

inline void ClearScene::Initialize() {
	using namespace KamataEngine;

	modelClear_ = Model::CreateFromOBJ("./Resources/sphere/sphere.obj", true);

	worldTransformClear_.Initialize();
	worldTransformClear_.translation_ = {0.0f, 1.0f, 8.0f};

	camera_->translation_ = {0.0f, 3.0f, 0.0f};
	camera_->rotation_ = {0.2f, 0.0f, 0.0f};
}

inline void ClearScene::Update() {
	using namespace KamataEngine;

	worldTransformClear_.rotation_.y += 0.03f;
	worldTransformClear_.TransferMatrix(); // ※コンパイルエラーになる場合はUpdateMatrix()に読み替えてください

	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_R)) {
		sceneNo = TITLE;
	}

	// HUD(ImGuiのリンクエラーを解消するまで一旦コメントアウト)
	// STAGE CLEAR! / PRESS R TO TITLE
}

inline void ClearScene::Draw() { modelClear_->Draw(worldTransformClear_, *camera_); }

inline ClearScene::~ClearScene() { delete modelClear_; }