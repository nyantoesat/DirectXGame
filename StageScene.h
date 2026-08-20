#pragma once
#include "IScene.h"
#include <cmath>
#include <cstdlib>

// ゲームシーン(2D版StageSceneの3D移植版)
class StageScene : public IScene {
private:
	// ==== フィールド設定 ====
	static constexpr float kFieldMinX = -8.0f;
	static constexpr float kFieldMaxX = 8.0f;
	static constexpr float kFieldMinZ = 0.0f;
	static constexpr float kFieldMaxZ = 16.0f;
	static constexpr float kEnemyZ = 15.0f;
	static constexpr float kPlayerRadius = 0.7f;
	static constexpr float kEnemyRadius = 0.8f;
	static constexpr float kBulletRadius = 0.3f;

	// ==== player ====
	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::WorldTransform worldTransformPlayer_;
	float playerSpeed_;
	int playerMaxHp_, playerHp_;
	int playerDamageTimer_;
	bool isPlayerAlive_;

	// ==== enemy ====
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::WorldTransform worldTransformEnemy_;
	float enemySpeed_;
	bool isEnemyAlive_;
	int enemyMaxHp_, enemyHp_;
	int damageTimer_;
	static const int kDamageFlashDuration = 10;
	int enemyDeathTimer_;
	static const int kEnemyDeathDelay = 120;
	int explosionAnimationTimer_;

	// ==== 自機弾 ====
	struct Bullet {
		KamataEngine::WorldTransform worldTransform;
		float speed;
		bool isActive;
	};
	static const int kMaxBullets = 50;
	KamataEngine::Model* modelBullet_ = nullptr;
	Bullet bullets_[kMaxBullets];

	// ==== 敵弾 ====
	enum BulletType { EnemyBulletFast, EnemyBulletSlow };
	struct EnemyBullet {
		KamataEngine::WorldTransform worldTransform;
		float speed;
		float dirX, dirZ;
		bool isActive;
		BulletType type;
	};
	static const int kMaxEnemyBullets = 50;
	KamataEngine::Model* modelEnemyBullet_ = nullptr;
	EnemyBullet enemyBullets_[kMaxEnemyBullets];
	int enemyShootTimer_;

public:
	void Initialize() override;
	void Update() override;
	void Draw() override;
	~StageScene();
};

inline void StageScene::Initialize() {
	using namespace KamataEngine;

	// ==== モデル読み込み ====
	// ※フォルダ名はお使いのプロジェクトの Resources 構成に合わせて変更してください
	modelPlayer_ = Model::CreateFromOBJ("./Resources/cube/cube.obj", true);
	modelEnemy_ = Model::CreateFromOBJ("./Resources/sphere/sphere.obj", true);
	modelBullet_ = Model::CreateFromOBJ("./Resources/cube/cube.obj", true);
	modelEnemyBullet_ = Model::CreateFromOBJ("./Resources/sphere/sphere.obj", true);

	// ==== カメラ ====
	camera_->translation_ = {0.0f, 9.0f, -12.0f};
	camera_->rotation_ = {0.5f, 0.0f, 0.0f};

	// ==== player ====
	worldTransformPlayer_.Initialize();
	worldTransformPlayer_.translation_ = {0.0f, 0.5f, 2.0f};
	playerSpeed_ = 0.2f;
	playerMaxHp_ = 5;
	playerHp_ = playerMaxHp_;
	playerDamageTimer_ = 0;
	isPlayerAlive_ = true;

	// ==== enemy ====
	worldTransformEnemy_.Initialize();
	worldTransformEnemy_.translation_ = {0.0f, 0.5f, kEnemyZ};
	enemySpeed_ = 0.1f;
	isEnemyAlive_ = true;
	enemyMaxHp_ = 30;
	enemyHp_ = enemyMaxHp_;
	damageTimer_ = 0;
	enemyDeathTimer_ = 0;
	explosionAnimationTimer_ = 0;

	// ==== 自機弾 ====
	for (int i = 0; i < kMaxBullets; i++) {
		bullets_[i].worldTransform.Initialize();
		bullets_[i].worldTransform.scale_ = {0.2f, 0.2f, 0.2f};
		bullets_[i].speed = 0.5f;
		bullets_[i].isActive = false;
	}

	// ==== 敵弾 ====
	for (int j = 0; j < kMaxEnemyBullets; j++) {
		enemyBullets_[j].worldTransform.Initialize();
		enemyBullets_[j].worldTransform.scale_ = {0.25f, 0.25f, 0.25f};
		enemyBullets_[j].isActive = false;
	}
	enemyShootTimer_ = 0;
}

inline void StageScene::Update() {
	using namespace KamataEngine;

	Input* input = Input::GetInstance();

	// 決着がついていたら演出だけ進めてシーン遷移を待つ
	if (!isEnemyAlive_ || !isPlayerAlive_) {
		if (enemyDeathTimer_ > 0) {
			enemyDeathTimer_--;
		} else {
			if (!isEnemyAlive_) {
				sceneNo = CLEAR;
			} else if (!isPlayerAlive_) {
				sceneNo = GAMEOVER;
			}
			return; // シーン変更後はこれ以上更新しない
		}
	}

	// ==== プレイヤー移動 (A/D:左右X, W/S:前後Z) ====
	float dirX = 0.0f;
	float dirZ = 0.0f;
	if (input->PushKey(DIK_W))
		dirZ += 1.0f;
	if (input->PushKey(DIK_S))
		dirZ -= 1.0f;
	if (input->PushKey(DIK_A))
		dirX -= 1.0f;
	if (input->PushKey(DIK_D))
		dirX += 1.0f;

	float length = sqrtf(dirX * dirX + dirZ * dirZ);
	if (length != 0.0f) {
		dirX /= length;
		dirZ /= length;
	}

	worldTransformPlayer_.translation_.x += dirX * playerSpeed_;
	worldTransformPlayer_.translation_.z += dirZ * playerSpeed_;

	// プレイヤーは手前側の領域に留める
	if (worldTransformPlayer_.translation_.x > kFieldMaxX)
		worldTransformPlayer_.translation_.x = kFieldMaxX;
	if (worldTransformPlayer_.translation_.x < kFieldMinX)
		worldTransformPlayer_.translation_.x = kFieldMinX;
	if (worldTransformPlayer_.translation_.z > kFieldMaxZ * 0.4f)
		worldTransformPlayer_.translation_.z = kFieldMaxZ * 0.4f;
	if (worldTransformPlayer_.translation_.z < kFieldMinZ)
		worldTransformPlayer_.translation_.z = kFieldMinZ;

	// ==== 弾発射 ====
	if (input->TriggerKey(DIK_SPACE)) {
		for (int i = 0; i < kMaxBullets; i++) {
			if (!bullets_[i].isActive) {
				bullets_[i].worldTransform.translation_ = worldTransformPlayer_.translation_;
				bullets_[i].worldTransform.translation_.y = 1.0f;
				bullets_[i].isActive = true;
				break;
			}
		}
	}

	// ==== 自機弾の更新・当たり判定 ====
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets_[i].isActive)
			continue;

		bullets_[i].worldTransform.translation_.z += bullets_[i].speed;

		if (bullets_[i].worldTransform.translation_.z > kFieldMaxZ + 5.0f) {
			bullets_[i].isActive = false;
			continue;
		}

		if (isEnemyAlive_) {
			float dx = bullets_[i].worldTransform.translation_.x - worldTransformEnemy_.translation_.x;
			float dz = bullets_[i].worldTransform.translation_.z - worldTransformEnemy_.translation_.z;
			float distanceSq = dx * dx + dz * dz;
			float radiusSum = kEnemyRadius + kBulletRadius;

			if (distanceSq <= radiusSum * radiusSum) {
				enemyHp_--;
				bullets_[i].isActive = false;
				damageTimer_ = kDamageFlashDuration;
				if (enemyHp_ <= 0) {
					isEnemyAlive_ = false;
					enemyDeathTimer_ = kEnemyDeathDelay;
					explosionAnimationTimer_ = 0;
				}
			}
		}

		bullets_[i].worldTransform.TransferMatrix();
	}

	// ==== 敵の左右移動 ====
	worldTransformEnemy_.translation_.x += enemySpeed_;
	if (worldTransformEnemy_.translation_.x >= kFieldMaxX)
		enemySpeed_ = -fabsf(enemySpeed_);
	if (worldTransformEnemy_.translation_.x <= kFieldMinX)
		enemySpeed_ = fabsf(enemySpeed_);

	// ==== 敵弾発射 ====
	enemyShootTimer_++;
	if (isEnemyAlive_ && enemyShootTimer_ > 30) {
		enemyShootTimer_ = 0;

		for (int j = 0; j < kMaxEnemyBullets; j++) {
			if (!enemyBullets_[j].isActive) {
				float dx = worldTransformPlayer_.translation_.x - worldTransformEnemy_.translation_.x;
				float dz = worldTransformPlayer_.translation_.z - worldTransformEnemy_.translation_.z;
				float mag = sqrtf(dx * dx + dz * dz);
				if (mag != 0.0f) {
					dx /= mag;
					dz /= mag;
				}

				enemyBullets_[j].worldTransform.translation_ = worldTransformEnemy_.translation_;
				enemyBullets_[j].worldTransform.translation_.y = 1.0f;
				enemyBullets_[j].dirX = dx;
				enemyBullets_[j].dirZ = dz;
				enemyBullets_[j].isActive = true;
				enemyBullets_[j].type = (rand() % 2 == 0) ? EnemyBulletFast : EnemyBulletSlow;
				enemyBullets_[j].speed = (enemyBullets_[j].type == EnemyBulletFast) ? 0.25f : 0.12f;
				break;
			}
		}
	}

	// ==== 敵弾の更新・当たり判定 ====
	for (int j = 0; j < kMaxEnemyBullets; j++) {
		if (!enemyBullets_[j].isActive)
			continue;

		enemyBullets_[j].worldTransform.translation_.x += enemyBullets_[j].dirX * enemyBullets_[j].speed;
		enemyBullets_[j].worldTransform.translation_.z += enemyBullets_[j].dirZ * enemyBullets_[j].speed;

		if (enemyBullets_[j].worldTransform.translation_.z < kFieldMinZ - 5.0f || enemyBullets_[j].worldTransform.translation_.x < kFieldMinX - 5.0f ||
		    enemyBullets_[j].worldTransform.translation_.x > kFieldMaxX + 5.0f) {
			enemyBullets_[j].isActive = false;
			continue;
		}

		if (isPlayerAlive_) {
			float dx = enemyBullets_[j].worldTransform.translation_.x - worldTransformPlayer_.translation_.x;
			float dz = enemyBullets_[j].worldTransform.translation_.z - worldTransformPlayer_.translation_.z;
			float distanceSq = dx * dx + dz * dz;
			float radiusSum = kPlayerRadius + kBulletRadius;

			if (distanceSq <= radiusSum * radiusSum) {
				playerDamageTimer_ = kDamageFlashDuration;
				playerHp_--;
				enemyBullets_[j].isActive = false;
				if (playerHp_ <= 0) {
					isPlayerAlive_ = false;
					enemyDeathTimer_ = kEnemyDeathDelay;
				}
			}
		}

		enemyBullets_[j].worldTransform.TransferMatrix();
	}

	if (damageTimer_ > 0)
		damageTimer_--;
	if (playerDamageTimer_ > 0)
		playerDamageTimer_--;

	explosionAnimationTimer_++;
	if (explosionAnimationTimer_ >= 60)
		explosionAnimationTimer_ = 0;

	worldTransformPlayer_.TransferMatrix(); // ※コンパイルエラーになる場合はUpdateMatrix()に読み替えてください
	worldTransformEnemy_.TransferMatrix();

	// ==== HUD ====
	// ImGuiのリンクエラーを解消するまで一旦コメントアウト。
	// 代わりにデバッガの出力ウィンドウでHPを確認したい場合は下記のように書けます:
	// char buf[128];
	// sprintf_s(buf, "Player HP: %d / %d, Enemy HP: %d / %d\n", playerHp_, playerMaxHp_, enemyHp_, enemyMaxHp_);
	// OutputDebugStringA(buf);
}

inline void StageScene::Draw() {
	if (isPlayerAlive_) {
		modelPlayer_->Draw(worldTransformPlayer_, *camera_);
	}

	if (isEnemyAlive_) {
		modelEnemy_->Draw(worldTransformEnemy_, *camera_);
	}

	for (int i = 0; i < kMaxBullets; i++) {
		if (bullets_[i].isActive) {
			modelBullet_->Draw(bullets_[i].worldTransform, *camera_);
		}
	}

	for (int j = 0; j < kMaxEnemyBullets; j++) {
		if (enemyBullets_[j].isActive) {
			modelEnemyBullet_->Draw(enemyBullets_[j].worldTransform, *camera_);
		}
	}
}

inline StageScene::~StageScene() {
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelBullet_;
	delete modelEnemyBullet_;
}