#pragma once
#include "IScene.h"
#include "MathUtility.h"
#include <cmath>
#include <cstdlib>

// ゲームシーン(2D版StageSceneの3D移植版・左右シューティングレイアウト)
class StageScene : public IScene {
private:
	// ==== フィールド設定 ====
	// プレイヤーは画面左側の範囲内を自由に移動できる。敵は画面右側に固定。
	static constexpr float kPlayerMinX = -12.0f;
	static constexpr float kPlayerMaxX = -6.0f;
	static constexpr float kFieldMinY = -6.0f;
	static constexpr float kFieldMaxY = 6.0f;
	static constexpr float kEnemyX = 8.0f; // 敵の固定X座標(右側)
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
	float enemySpeed_; // Y方向(上下)に往復移動
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
		float dirX, dirY;
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
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelBullet_ = Model::CreateFromOBJ("cube", true);
	modelEnemyBullet_ = Model::CreateFromOBJ("cube", true);

	// ==== カメラ ====
	// 左右のレイアウト全体が見えるよう、真正面よりに引いて配置
	camera_->translation_ = {0.0f, 6.0f, -20.0f};
	camera_->rotation_ = {0.3f, 0.0f, 0.0f};

	// ==== player (画面左側) ====
	worldTransformPlayer_.Initialize();
	worldTransformPlayer_.translation_ = {kPlayerMinX, 0.0f, 0.0f};
	playerSpeed_ = 0.2f;
	playerMaxHp_ = 5;
	playerHp_ = playerMaxHp_;
	playerDamageTimer_ = 0;
	isPlayerAlive_ = true;

	// ==== enemy (画面右側、上下に往復) ====
	worldTransformEnemy_.Initialize();
	worldTransformEnemy_.scale_ = {1.5f, 1.5f, 1.5f}; // 一時的にcubeを流用しているので、playerと見分けやすいよう大きめに
	worldTransformEnemy_.translation_ = {kEnemyX, 0.0f, 0.0f};
	enemySpeed_ = 0.08f;
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

	// ==== プレイヤー移動 (W/S:上下Y、A/D:左右X) ====
	float dirX = 0.0f;
	float dirY = 0.0f;
	if (input->PushKey(DIK_W))
		dirY += 1.0f;
	if (input->PushKey(DIK_S))
		dirY -= 1.0f;
	if (input->PushKey(DIK_A))
		dirX -= 1.0f;
	if (input->PushKey(DIK_D))
		dirX += 1.0f;

	float length = sqrtf(dirX * dirX + dirY * dirY);
	if (length != 0.0f) {
		dirX /= length;
		dirY /= length;
	}

	worldTransformPlayer_.translation_.x += dirX * playerSpeed_;
	worldTransformPlayer_.translation_.y += dirY * playerSpeed_;

	// プレイヤーは画面左側の可動範囲に留める
	if (worldTransformPlayer_.translation_.x > kPlayerMaxX)
		worldTransformPlayer_.translation_.x = kPlayerMaxX;
	if (worldTransformPlayer_.translation_.x < kPlayerMinX)
		worldTransformPlayer_.translation_.x = kPlayerMinX;
	if (worldTransformPlayer_.translation_.y > kFieldMaxY)
		worldTransformPlayer_.translation_.y = kFieldMaxY;
	if (worldTransformPlayer_.translation_.y < kFieldMinY)
		worldTransformPlayer_.translation_.y = kFieldMinY;

	// ==== 弾発射 ====
	if (input->TriggerKey(DIK_SPACE)) {
		for (int i = 0; i < kMaxBullets; i++) {
			if (!bullets_[i].isActive) {
				bullets_[i].worldTransform.translation_ = worldTransformPlayer_.translation_;
				bullets_[i].isActive = true;
				break;
			}
		}
	}

	// ==== 自機弾の更新・当たり判定 (右方向へ飛ぶ) ====
	for (int i = 0; i < kMaxBullets; i++) {
		if (!bullets_[i].isActive)
			continue;

		bullets_[i].worldTransform.translation_.x += bullets_[i].speed;

		if (bullets_[i].worldTransform.translation_.x > kEnemyX + 5.0f) {
			bullets_[i].isActive = false;
			continue;
		}

		if (isEnemyAlive_) {
			float dx = bullets_[i].worldTransform.translation_.x - worldTransformEnemy_.translation_.x;
			float dy = bullets_[i].worldTransform.translation_.y - worldTransformEnemy_.translation_.y;
			float distanceSq = dx * dx + dy * dy;
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

		bullets_[i].worldTransform.matWorld_ = MakeAffineMatrix(bullets_[i].worldTransform.scale_, bullets_[i].worldTransform.rotation_, bullets_[i].worldTransform.translation_);
		bullets_[i].worldTransform.TransferMatrix();
	}

	// ==== 敵の上下移動 ====
	worldTransformEnemy_.translation_.y += enemySpeed_;
	if (worldTransformEnemy_.translation_.y >= kFieldMaxY)
		enemySpeed_ = -fabsf(enemySpeed_);
	if (worldTransformEnemy_.translation_.y <= kFieldMinY)
		enemySpeed_ = fabsf(enemySpeed_);

	// ==== 敵弾発射 ====
	enemyShootTimer_++;
	if (isEnemyAlive_ && enemyShootTimer_ > 30) {
		enemyShootTimer_ = 0;

		for (int j = 0; j < kMaxEnemyBullets; j++) {
			if (!enemyBullets_[j].isActive) {
				float dx = worldTransformPlayer_.translation_.x - worldTransformEnemy_.translation_.x;
				float dy = worldTransformPlayer_.translation_.y - worldTransformEnemy_.translation_.y;
				float mag = sqrtf(dx * dx + dy * dy);
				if (mag != 0.0f) {
					dx /= mag;
					dy /= mag;
				}

				enemyBullets_[j].worldTransform.translation_ = worldTransformEnemy_.translation_;
				enemyBullets_[j].dirX = dx;
				enemyBullets_[j].dirY = dy;
				enemyBullets_[j].isActive = true;
				enemyBullets_[j].type = (rand() % 2 == 0) ? EnemyBulletFast : EnemyBulletSlow;
				enemyBullets_[j].speed = (enemyBullets_[j].type == EnemyBulletFast) ? 0.25f : 0.12f;
				break;
			}
		}
	}

	// ==== 敵弾の更新・当たり判定 (プレイヤーへ向かって飛ぶ) ====
	for (int j = 0; j < kMaxEnemyBullets; j++) {
		if (!enemyBullets_[j].isActive)
			continue;

		enemyBullets_[j].worldTransform.translation_.x += enemyBullets_[j].dirX * enemyBullets_[j].speed;
		enemyBullets_[j].worldTransform.translation_.y += enemyBullets_[j].dirY * enemyBullets_[j].speed;

		if (enemyBullets_[j].worldTransform.translation_.x < kPlayerMinX - 5.0f || enemyBullets_[j].worldTransform.translation_.y < kFieldMinY - 5.0f ||
		    enemyBullets_[j].worldTransform.translation_.y > kFieldMaxY + 5.0f) {
			enemyBullets_[j].isActive = false;
			continue;
		}

		if (isPlayerAlive_) {
			float dx = enemyBullets_[j].worldTransform.translation_.x - worldTransformPlayer_.translation_.x;
			float dy = enemyBullets_[j].worldTransform.translation_.y - worldTransformPlayer_.translation_.y;
			float distanceSq = dx * dx + dy * dy;
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

		enemyBullets_[j].worldTransform.matWorld_ = MakeAffineMatrix(enemyBullets_[j].worldTransform.scale_, enemyBullets_[j].worldTransform.rotation_, enemyBullets_[j].worldTransform.translation_);
		enemyBullets_[j].worldTransform.TransferMatrix();
	}

	if (damageTimer_ > 0)
		damageTimer_--;
	if (playerDamageTimer_ > 0)
		playerDamageTimer_--;

	explosionAnimationTimer_++;
	if (explosionAnimationTimer_ >= 60)
		explosionAnimationTimer_ = 0;

	worldTransformPlayer_.matWorld_ = MakeAffineMatrix(worldTransformPlayer_.scale_, worldTransformPlayer_.rotation_, worldTransformPlayer_.translation_);
	worldTransformPlayer_.TransferMatrix();
	worldTransformEnemy_.matWorld_ = MakeAffineMatrix(worldTransformEnemy_.scale_, worldTransformEnemy_.rotation_, worldTransformEnemy_.translation_);
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