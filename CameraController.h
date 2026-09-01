#pragma once
#include "KamataEngine.h"

// カメラワークを管理するクラス(縦スクロールシューティング方式)
// 急な見下ろし角度でトラックに沿って自動的に前進し、
// プレイヤーの操作にわずかに追従して視点をずらす
class CameraController {
public:
	void Initialize();
	// trackDistance: 現在の走行距離(ワールドZ座標のベースになる)
	// playerLocalX/playerLocalDepth: プレイヤーの画面内位置(カメラが実際に追従する)
	// isDashing: ダッシュ中かどうか(疾走感の演出に使用)
	void Update(float trackDistance, float playerLocalX, float playerLocalDepth, bool isDashing);

	KamataEngine::Camera& GetCamera() { return camera_; }

private:
	KamataEngine::Camera camera_;

	static constexpr float kHeight = 14.0f;           // かなり高い位置から見下ろす
	static constexpr float kPitch = 1.0f;             // 急な見下ろし角度(ラジアン)
	static constexpr float kBehindOffset = -6.0f;     // 自機より少し手前に位置する
	static constexpr float kParallaxX = 0.6f;         // プレイヤーの左右移動にどれだけ追従するか
	static constexpr float kParallaxDepth = 0.5f;     // プレイヤーの前後移動にどれだけ追従するか
	static constexpr float kDashPitchBoost = 0.05f;   // ダッシュ中はわずかに前のめりにして疾走感を演出
	static constexpr float kPositionLerpRate = 0.25f; // 追従の速さ(はっきり反応するように)

	float goalX_ = 0.0f;
	float goalZ_ = kBehindOffset;
};