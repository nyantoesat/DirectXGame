#pragma once
#include "AABB.h"
#include "KamataEngine.h"

// 自機(プレイヤー)クラス
// 縦スクロールシューティング方式:ワールドが自動的にZ方向へ進み続け、
// 画面内(ローカルX/Z)を自由に操作して弾を避けたり撃ったりする
class Player {
public:
	struct Bullet {
		KamataEngine::WorldTransform worldTransform;
		bool isActive = false;
		bool isCharged = false; // 溜め撃ちの弾かどうか(ダメージが大きい)
	};

	static const int kMaxBullets = 50;

	// shieldTextureHandle: シールド発動中に青く表示するためのテクスチャハンドル
	void Initialize(
	    KamataEngine::Model* model, KamataEngine::Model* bulletModel, KamataEngine::Model* hpBarModel, KamataEngine::Camera* camera, float startLocalX, float startLocalDepth,
	    uint32_t shieldTextureHandle = 0);
	// trackDistance: ワールドが進んだ距離(自機のワールドZ座標のベースになる)
	void Update(float trackDistance);
	void Draw();

	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }
	// 画面内での位置(カメラのパララックス演出に使用)
	float GetLocalX() const { return localX_; }
	float GetLocalDepth() const { return localDepth_; }

	bool IsAlive() const { return isAlive_; }
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	void TakeDamage();

	Bullet* GetBullets() { return bullets_; }
	static float GetRadius() { return kRadius; }

	// AABBを取得
	AABB GetAABB() const;

	// ダッシュ中かどうか(カメラワークの演出に使用)
	bool IsDashing() const { return dashTimer_ > 0; }

	// シールド発動中かどうか、クールダウンの割合(HUD表示に使用)
	bool IsShielded() const { return shieldTimer_ > 0; }
	float GetShieldCooldownRatio() const { return static_cast<float>(shieldCooldownTimer_) / static_cast<float>(kShieldCooldown); }

private:
	void UpdateMove();
	void UpdateDash();
	void UpdateShield();
	void UpdateShoot();
	void UpdateBullets();
	void UpdateHpBar();

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* bulletModel_ = nullptr;
	KamataEngine::Model* hpBarModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	uint32_t shieldTextureHandle_ = 0; // シールド発動中に表示するテクスチャ

	// HPバー(cubeモデルを再利用し、HPの割合に応じて幅を縮小する)
	KamataEngine::WorldTransform worldTransformHpBar_;
	static constexpr float kHpBarWidth = 1.4f;
	static constexpr float kHpBarHeight = 0.15f;
	static constexpr float kHpBarDepth = 0.15f;
	static constexpr float kHpBarYOffset = 1.0f;

	Bullet bullets_[kMaxBullets];
	float bulletSpeed_ = 0.6f;
	static constexpr float kChargedBulletSpeed = 0.9f;
	static constexpr float kChargedBulletScale = 0.4f;
	static constexpr float kNormalBulletScale = 0.2f;
	static constexpr float kBulletMaxDistance = 30.0f; // 自機からこれだけ離れたら消す

	// ==== 画面内でのローカル座標(操縦位置、制限なし) ====
	// localX: 画面左右、localDepth: 画面奥/手前(ワールドZ方向の自由オフセット)
	float localX_ = 0.0f;
	float localDepth_ = 0.0f;
	static constexpr float kFixedHeight = 0.5f;
	static constexpr float kForwardOffset = 3.0f; // カメラから見て自機が前にいる距離(画面下側に表示されるよう短めに)

	float speed_ = 0.15f;
	int maxHp_ = 5;
	int hp_ = 5;
	int damageTimer_ = 0;
	bool isAlive_ = true;

	// ==== ダッシュ(回避) ====
	int dashTimer_ = 0;
	int dashCooldownTimer_ = 0;
	static const int kDashDuration = 10;
	static const int kDashCooldown = 45;
	static constexpr float kDashSpeedMultiplier = 3.0f;
	bool IsInvulnerable() const { return dashTimer_ > 0 || shieldTimer_ > 0; }

	// ==== シールド(一定時間無敵になる) ====
	int shieldTimer_ = 0;
	int shieldCooldownTimer_ = 0;
	static const int kShieldDuration = 120; // 発動時間(約2秒)
	static const int kShieldCooldown = 300; // クールダウン(約5秒)

	// ==== 溜め撃ち ====
	int chargeTimer_ = 0;
	bool wasShootKeyDown_ = false;
	static const int kChargeThreshold = 25;

	static constexpr float kRadius = 0.7f;
	static const int kDamageFlashDuration = 10;
};