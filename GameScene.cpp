#include "GameScene.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
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
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	camera_.Initialize();
	skydome_ = new Skydome();
	skydome_->Initialize();
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("blocks.csv");
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
	player_ = new Player();
	player_->Initialize(modelPlayer_, textureHandle_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);
	GenerateBlocks();

	enemyTextureHandle_ = TextureManager::Load("./Resources/enemy/enemy.png");

	// 敵の生成（複数体）
	for (int32_t i = 0; i < 3; ++i) {
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(5 + i * 5, 18);
		newEnemy->Initialize(modelEnemy_, enemyTextureHandle_, &camera_, enemyPosition);
		newEnemy->setMapChipField(mapChipField_);
		enemies_.push_back(newEnemy);
	}

	cameraController_ = new CameraController();
	cameraController_->Initialize();
	cameraController_->SetTarget(player_);
	cameraController_->Reset();

	// デスパーティクルのモデルを読み込み（生成はプレイヤー死亡時）
	modelDeathParticles_ = Model::CreateFromOBJ("deathParticle", true);

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();
	// シーン開始時にフェードイン
	fade_->StartFadeIn(1.0f);
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
	delete modelEnemy_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	delete cameraController_;
	delete deathParticles_;
	delete modelDeathParticles_;
	delete fade_;
}

void GameScene::Update() {

	debugCamera_->Update();

	// フェーズごとの更新処理
	switch (phase_) {
	case Phase::kPlay:
		UpdatePlayPhase();
		break;
	case Phase::kDeath:
		UpdateDeathPhase();
		break;
	}

	// フェーズの切り替え（更新後に判定）
	ChangePhase();

	// フェードの更新
	fade_->Update();

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}
#endif

	if (isDebugCameraActive_) {
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		// kPlayのみカメラコントローラを使う
		if (phase_ == Phase::kPlay) {
			cameraController_->Update();
			camera_.matView = cameraController_->GetCamera().matView;
			camera_.matProjection = cameraController_->GetCamera().matProjection;
		}
		camera_.TransferMatrix();
	}
}

void GameScene::UpdatePlayPhase() {
	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->TransferMatrix();
		}
	}

	// 天球の更新
	skydome_->Update();
	// 自キャラの更新
	player_->Update();
	// 敵の更新（複数）
	for (Enemy* enemy : enemies_) {
		enemy->update();
	}

	// 全ての当たり判定を行う（各オブジェクトの行列計算が終わった後に実行）
	CheckAllCollisions();
}

void GameScene::UpdateDeathPhase() {
	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;
			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->TransferMatrix();
		}
	}

	// 天球の更新
	skydome_->Update();
	// 敵の更新（複数）
	for (Enemy* enemy : enemies_) {
		enemy->update();
	}
	// デスパーティクルの更新
	if (deathParticles_) {
		deathParticles_->Update();
		if (deathParticles_->IsFinished()) {
			// 演出終了時にフェードアウト開始
			fade_->StartFadeOut(1.0f);
			delete deathParticles_;
			deathParticles_ = nullptr;
		}
	}

	// フェードアウト完了後にゲームシーン終了フラグを立てる
	if (!deathParticles_ && phase_ == Phase::kDeath && fade_->IsFinished()) {
		finished_ = true;
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		// 自キャラがデス状態になったら
		if (player_->IsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;
			// 自キャラの座標を取得
			const Vector3 deathParticlesPosition = player_->GetWorldPosition();
			// 自キャラの座標にデスパーティクルを発生、初期化
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(modelDeathParticles_, &camera_, deathParticlesPosition);
		}
		break;
	case Phase::kDeath:
		// 終了フラグはUpdateDeathPhase内で設定される
		break;
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
	for (Enemy* enemy : enemies_) {
		enemy->draw();
	}
	// デスパーティクルの描画
	if (deathParticles_) {
		deathParticles_->Draw();
	}
	Model::PostDraw();

	// フェードの描画（必ず末尾）
	fade_->Draw();
}

void GameScene::CheckAllCollisions() {
#pragma region 自キャラと敵キャラの当たり判定
	// 判定対象1と2の座標
	AABB aabb1, aabb2;

	// 自キャラの座標
	aabb1 = player_->GetAABB();

	// 自キャラと敵全ての当たり判定
	for (Enemy* enemy : enemies_) {
		// 敵の座標
		aabb2 = enemy->GetAABB();

		// AABB同士の交差判定
		if (IsCollision(aabb1, aabb2)) {
			// 自キャラの衝突時コールバックを呼び出す
			player_->OnCollision(enemy);
			// 敵の衝突時コールバックを呼び出す
			enemy->OnCollision(player_);
		}
	}
#pragma endregion
}