#pragma once
#include "AABB.h"
#include "KamataEngine.h"

// 敵クラス
// トラック上の固定地点に配置され、左右に小さく揺れながら
// 接近してくるプレイヤーを狙って弾を撃つ
class Enemy {
public:
	struct Bullet {
		KamataEngine::WorldTransform worldTransform;
		float dirX = 0.0f, dirZ = 0.0f;
		float speed = 0.0f;
		bool isActive = false;
	};

	static const int kMaxBullets = 30;

	// spawnPosition: 敵の初期配置(X/Yと、出現時点でのトラック上のZ基準)
	// waveNumber: 1から始まるウェーブ番号。大きいほどHPや速度が上がる
	// hitTextureHandle: 被弾時に赤く点滅させるためのテクスチャハンドル
	void Initialize(
	    KamataEngine::Model* model, KamataEngine::Model* bulletModel, KamataEngine::Model* hpBarModel, KamataEngine::Camera* camera, const KamataEngine::Vector3& spawnPosition, int waveNumber = 1,
	    uint32_t hitTextureHandle = 0);
	// targetPosition: 弾を狙う対象(プレイヤー)の座標
	// trackDistance: 現在のスクロール距離(カメラと一緒に移動するための基準)
	void Update(const KamataEngine::Vector3& targetPosition, float trackDistance);
	void Draw();

	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }

	bool IsAlive() const { return isAlive_; }
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	void TakeDamage();

	Bullet* GetBullets() { return bullets_; }
	static float GetRadius() { return kRadius; }

	// AABBを取得
	AABB GetAABB() const;

private:
	void UpdateRandomMove();
	void UpdateShoot(const KamataEngine::Vector3& targetPosition);
	void UpdateBullets();
	void UpdateHpBar();
	void FireAimedShot(const KamataEngine::Vector3& targetPosition);

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* bulletModel_ = nullptr;
	KamataEngine::Model* hpBarModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	uint32_t hitTextureHandle_ = 0; // 被弾時に赤く表示するためのテクスチャ

	// HPバー(cubeモデルを再利用し、HPの割合に応じて幅を縮小する)
	KamataEngine::WorldTransform worldTransformHpBar_;
	static constexpr float kHpBarWidth = 1.2f;
	static constexpr float kHpBarHeight = 0.15f;
	static constexpr float kHpBarDepth = 0.15f;
	static constexpr float kHpBarYOffset = 1.0f;

	Bullet bullets_[kMaxBullets];
	int shootTimer_ = 0;

	// ==== ランダムな動き(周辺をふらつくようにランダムウォークする) ====
	float basePositionX_ = 0.0f;
	float basePositionY_ = 0.0f;
	float currentOffsetX_ = 0.0f;
	float currentOffsetY_ = 0.0f;
	float targetOffsetX_ = 0.0f;
	float targetOffsetY_ = 0.0f;
	int randomMoveTimer_ = 0;
	static constexpr float kMoveRangeX = 3.0f;     // 左右に動ける範囲
	static constexpr float kMoveRangeY = 1.5f;     // 上下に動ける範囲
	static constexpr float kMoveLerpRate = 0.03f;  // 目標地点への追従の滑らかさ
	static const int kMoveChangeIntervalMin = 40;  // 次の目標地点を決めるまでの最短フレーム数
	static const int kMoveChangeIntervalMax = 100; // 次の目標地点を決めるまでの最長フレーム数
	float moveLerpRateMultiplier_ = 1.0f;          // ウェーブが進むほど動きを速くする倍率

	// カメラ(トラック)から見て常にこの距離だけ先に留まり続ける
	float aheadDistance_ = 0.0f;

	int maxHp_ = 10;
	int hp_ = 10;
	int damageTimer_ = 0;
	bool isAlive_ = true;

	static constexpr float kRadius = 0.7f;
	static const int kDamageFlashDuration = 20; // 約0.33秒(60fps想定)。この間は赤く表示され、再被弾も無効になる
	static const int kShootInterval = 100;      // 発射間隔(フレーム数)。大きいほど発射頻度が下がる
};