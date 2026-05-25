#include "GameScene.h"
#include "MapChipField.h"
#include "MathUtility.h"
#include "Player.h"
#include <cmath>

using namespace KamataEngine;

void GameScene::Initialize() {

	debugCamera_ = new DebugCamera(1280, 720);

	textureHandle_ = TextureManager::Load("./Resources/player/player.png");
	blockTextureHandle_ = TextureManager::Load("./Resources/block/block.png");

	model_ = Model::Create();
	modelBlock_ = Model::CreateFromOBJ("block", true);
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	camera_.Initialize();
	skydome_ = new Skydome();
	skydome_->Initialize();
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("blocks.csv");
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
	player_ = new Player();
	player_->Initialize(modelPlayer_, textureHandle_, &camera_, playerPosition);
	GenerateBlocks();

	cameraController_ = new CameraController();
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);
	cameraController_->Reset();
}

void GameScene::GenerateBlocks() {
	for (std::vector<WorldTransform*>& row : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : row) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
	uint32_t numBlockVertical = mapChipField_->GetNumBlockVertical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = worldTransform;
			}
		}
	}
}

GameScene::~GameScene() {
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
	delete model_;
	delete modelBlock_;
	delete debugCamera_;
	delete skydome_;
	delete mapChipField_;
	delete modelPlayer_;
	delete player_;
	delete cameraController_;
}

void GameScene::Update() {

	debugCamera_->Update();

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {

			if (!worldTransformBlock)
				continue;

			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

			worldTransformBlock->TransferMatrix();
		}
	}

	skydome_->Update();
	player_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		cameraController_->Update();
		camera_.matView = cameraController_->GetCamera().matView;
		camera_.matProjection = cameraController_->GetCamera().matProjection;
		camera_.TransferMatrix();
	}
}

void GameScene::Draw() {
	Model::PreDraw();
	skydome_->Draw(camera_);

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			modelBlock_->Draw(*worldTransformBlock, camera_, blockTextureHandle_);
		}
	}
	player_->Draw();
	Model::PostDraw();
}