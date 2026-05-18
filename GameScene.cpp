#include "GameScene.h"
#include "MathUtility.h"
#include <cmath>

using namespace KamataEngine;


void GameScene::Initialize() {

	debugCamera_ = new DebugCamera(1280,720);

	textureHandle_ = TextureManager::Load("./Resources/cube/cube.jpg");
	model_ = Model::Create();
	modelBlock_ = Model::Create();
	camera_.Initialize();
	skydome_ = new Skydome();
	skydome_->Initialize();

	
	for (std::vector<WorldTransform*>& row : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : row) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();


	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;

	
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			if ((i + j) % 2 == 0) {
				worldTransformBlocks_[i][j] = new WorldTransform();
				worldTransformBlocks_[i][j]->Initialize();
				worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
				worldTransformBlocks_[i][j]->translation_.y = kBlockHeight * i;
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
			camera_.UpdateMatrix();
		}
	    skydome_->Update();
	
}
	void GameScene::Draw() {
	Model::PreDraw();
	    skydome_->Draw(camera_);

	    for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		    for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			    if (!worldTransformBlock)
				    continue;
			    modelBlock_->Draw(*worldTransformBlock, camera_, textureHandle_);
		    }
	    }

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			
			if (!worldTransformBlock)
				continue;

			modelBlock_->Draw(*worldTransformBlock, camera_, textureHandle_);
		}
	}

	Model::PostDraw();
}