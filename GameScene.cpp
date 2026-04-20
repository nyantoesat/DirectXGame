#include "GameScene.h"
#include "2d/ImGuiManager.h"

using namespace KamataEngine;
void GameScene::Initialize() { 
	textureHandle_ = TextureManager::Load("uvChecker.png");
	soundDataHandle_ = Audio::GetInstance()->LoadWave("mokugyo.wav");

	//sprite_ = Sprite::Create(textureHandle_, {100, 100});
	model_ = Model::Create();

	worldTransform_.Initialize();
	camera_.Initialize();

	Audio::GetInstance()->PlayWave(soundDataHandle_);
	voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	//PrimitiveDrawer::GetInstance()->SetCamera(&camera_);

	debugCamera_ = new DebugCamera(1280, 720);
	
	AxisIndicator ::GetInstance()->SetVisible(true);
	AxisIndicator ::GetInstance()->SetTargetCamera(&debugCamera_->GetCamera());
	
}
void GameScene::Update() { 
	/* Vector2 position = sprite_->GetPosition();
	position.x += 2.0f;
	position.y += 2.0f;
	sprite_->SetPosition(position);*/

	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		Audio ::GetInstance()->StopWave(voiceHandle_);
	}
	/* ImGui::Begin("Debug1");
	ImGui::InputFloat3("InputFloat3", inputFloat3);
	ImGui::SliderFloat3("SliderFloat3", inputFloat3, 0.0f, 0.0f);
	ImGui::Text("Kamata Tarou %d.%d.%d", 2050, 12, 31);
	ImGui::ShowDemoWindow();
	ImGui::End();*/
	debugCamera_->Update();
}
void GameScene::Draw() { 

	// スプライト描画前の処理
	Sprite::PreDraw();
	//sprite_->Draw();
    Sprite::PostDraw();

	Model::PreDraw();
	model_->Draw(worldTransform_, debugCamera_->GetCamera(), textureHandle_);
	Model::PostDraw();

	//PrimitiveDrawer::GetInstance()->DrawLine3d({0, 0, 0}, {0, 10, 0}, {1.0f, 0.0f, 0.0f, 1.0f});

}

