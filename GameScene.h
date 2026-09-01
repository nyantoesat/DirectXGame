#pragma once
#include "AABB.h"
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "KamataEngine.h"
#include "Player.h"
#include "Skydome.h"
#include <vector>

class GameScene {
public:
	// ゲームの結果(型)
	enum class Result {
		kNone,
		kClear,
		kGameOver,
		kQuitToTitle, // ポーズメニューからタイトルへ戻る
	};

	~GameScene();

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }
	Result GetResult() const { return result_; }
	int GetScore() const { return score_; }

private:
	void CheckAllCollisions();
	void SpawnWave();
	void UpdateScoreBar();
	void SpawnParticleEffect(const KamataEngine::Vector3& position);
	void UpdateParticleEffects();
	void DrawParticleEffects();
	bool AreAllEnemiesDead() const;
	void UpdatePause();

	CameraController cameraController_;

	KamataEngine::Model* modelPlayer_ = nullptr;
	KamataEngine::Model* modelEnemy_ = nullptr;
	KamataEngine::Model* modelBullet_ = nullptr;
	KamataEngine::Model* modelEnemyBullet_ = nullptr;
	KamataEngine::Model* modelHpBar_ = nullptr;
	uint32_t hitTextureHandle_ = 0;    // 被弾時の赤フラッシュ用テクスチャ
	uint32_t shieldTextureHandle_ = 0; // シールド発動中の青フラッシュ用テクスチャ

	Player* player_ = nullptr;
	std::vector<Enemy*> enemies_;

	Skydome* skydome_ = nullptr;
	KamataEngine::Model* modelSkydome_ = nullptr;

	// ==== ウェーブ制 ====
	// ウェーブが進むごとに、隊列として出現する敵の数が増える
	int waveNumber_ = 1;
	static const int kMaxWaves = 3;
	static const int kBaseEnemyCount = 1; // ウェーブ1での敵の数(1体→3体→5体…と増えていく)

	// ==== トラック(スクロール)の進行 ====
	float trackDistance_ = 0.0f;
	static constexpr float kScrollSpeed = 0.08f;
	static constexpr float kDashScrollMultiplier = 1.6f;
	static constexpr float kEnemyAheadDistance = 10.0f; // 現在地からこれだけ先に次の隊列を配置(画面内に収まるよう短めに)
	static constexpr float kFormationHalfWidth = 5.0f;  // 隊列の横幅の半分

	// ==== スコア ====
	int score_ = 0;
	static const int kScorePerEnemy = 50;
	KamataEngine::WorldTransform worldTransformScoreBar_;
	static constexpr float kScoreBarWidth = 2.0f;
	static constexpr float kScoreBarHeight = 0.15f;
	static constexpr float kScoreBarDepth = 0.15f;
	static constexpr float kScoreBarYOffset = 1.4f;

	// 決着後、演出のために少し待ってからシーン終了フラグ・次ウェーブ開始を行う
	int endDelayTimer_ = 0;
	static const int kEndDelay = 60;
	bool waitingNextWave_ = false;

	// 終了フラグ
	bool finished_ = false;
	// 結果(クリア/ゲームオーバー)
	Result result_ = Result::kNone;

	// ==== フェード ====
	Fade* fade_ = nullptr;
	bool exitRequested_ = false;
	static constexpr float kFadeDuration = 0.5f;

	// ==== パーティクルエフェクト ====
	struct ParticleEffect {
		DeathParticles particles;
		bool isActive = false;
	};
	static const int kMaxParticleEffects = 20;
	ParticleEffect particleEffects_[kMaxParticleEffects];
	KamataEngine::Model* modelParticle_ = nullptr;

	// ==== ステージ表示(ウェーブ開始時に一時的に表示する3Dテキスト) ====
	KamataEngine::Model* modelStageFonts_[kMaxWaves] = {nullptr, nullptr, nullptr};
	KamataEngine::WorldTransform worldTransformStageIndicator_;
	int stageIndicatorTimer_ = 0;
	static const int kStageIndicatorDuration = 120; // 表示時間(約2秒)
	float stageIndicatorBounceTimer_ = 0.0f;
	static constexpr float kStageIndicatorYOffset = 3.0f;
	static constexpr float kStageIndicatorBounceAmplitude = 0.3f;
	static constexpr float kStageIndicatorBounceSpeed = 0.08f;

	// ==== 「STAGE START」の表示(ゲーム開始時、ウェーブ1のみ表示) ====
	KamataEngine::Model* modelStageStartFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformStageStartFont_;
	static constexpr float kStageStartYOffset = 4.2f;    // ステージ番号バナーの少し上に表示
	static constexpr float kStageStartDropHeight = 8.0f; // 上から落ちてくる高さ
	static const int kStageStartDropDuration = 30;       // 落下にかかるフレーム数(約0.5秒)

	bool playerWasAlive_ = true;

	// ==== ポーズメニュー ====
	bool isPaused_ = false;
	float pauseBounceTimer_ = 0.0f;
	float pauseIndicatorBaseY_ = 0.0f;
	KamataEngine::Model* modelPauseFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformPauseIndicator_; // 一時停止中に表示するテキスト
	static constexpr float kPauseIndicatorYOffset = 3.5f;
	static constexpr float kPauseBounceAmplitude = 0.3f;
	static constexpr float kPauseBounceSpeed = 0.08f;

	// Rキーを一定時間押し続けるとタイトルへ戻る(誤操作防止の確認動作)
	int quitHoldTimer_ = 0;
	static const int kQuitHoldDuration = 90;             // 1.5秒相当
	KamataEngine::WorldTransform worldTransformQuitBar_; // 押し続け具合を示すバー(HPバーと同じ仕組み)
	static constexpr float kQuitBarWidth = 2.0f;
	static constexpr float kQuitBarHeight = 0.2f;
	static constexpr float kQuitBarDepth = 0.2f;
	static constexpr float kQuitBarYOffset = 2.3f;

	// ==== 「ESCでポーズ」の案内(カメラのローカル空間で左上・近くに配置する) ====
	KamataEngine::Model* modelEscFont_ = nullptr;
	KamataEngine::WorldTransform worldTransformEscFont_;
	static constexpr float kEscFontRight = -3.0f;  // カメラのローカル右方向(負の値で左)
	static constexpr float kEscFontUp = 3.0f;      // カメラのローカル上方向
	static constexpr float kEscFontForward = 8.0f; // カメラの前方向への距離
};