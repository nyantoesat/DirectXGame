#pragma once
#include "IScene.h"

// タイトルシーン
class TitleScene : public IScene {
private:
	KamataEngine::Model* modelTitle_ = nullptr;
	KamataEngine::WorldTransform worldTransformTitle_;

public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	~TitleScene();
};

inline void TitleScene::Initialize() {
	using namespace KamataEngine;

	// タイトルのシンボル代わりにくるくる回る立方体を表示
	// ※"Resources/cube/cube.obj" が無い場合はお使いのプロジェクトの
	//   モデルフォルダ名に合わせて変更してください
	modelTitle_ = Model::CreateFromOBJ("./Resources/cube/", true);

	worldTransformTitle_.Initialize();
	worldTransformTitle_.translation_ = {0.0f, 1.0f, 10.0f};

	camera_->translation_ = {0.0f, 3.0f, 0.0f};
	camera_->rotation_ = {0.2f, 0.0f, 0.0f};
}

inline void TitleScene::Update() {
	using namespace KamataEngine;

	// 見た目だけの演出でゆっくり回転させる
	worldTransformTitle_.rotation_.y += 0.02f;
	worldTransformTitle_.TransferMatrix(); // ※コンパイルエラーになる場合はUpdateMatrix()に読み替えてください

	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_G)) {
		sceneNo = STAGE; // シーン変更 -> SceneManagerが次フレームでStageScene::Initialize()を呼ぶ
	}

	// HUD(ImGuiのリンクエラーを解消するまで一旦コメントアウト)
	// PRESS G TO START
}

inline void TitleScene::Draw() { modelTitle_->Draw(worldTransformTitle_, *camera_); }

inline TitleScene::~TitleScene() { delete modelTitle_; }