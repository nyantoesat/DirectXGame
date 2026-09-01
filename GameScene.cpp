#include "GameScene.h"
#include "MathUtility.h"
#include <cmath>

using namespace KamataEngine;

void GameScene::Initialize() {
	// ==== カメラ ====
	cameraController_.Initialize();

	// ==== モデル読み込み ====
	modelPlayer_ = Model::CreateFromOBJ("player", true);
	modelEnemy_ = Model::CreateFromOBJ("enemy", true);
	modelBullet_ = Model::CreateFromOBJ("bullet", true);
	modelEnemyBullet_ = Model::CreateFromOBJ("bullet", true);
	modelHpBar_ = Model::CreateFromOBJ("Hp", true);
	modelSkydome_ = Model::CreateFromOBJ("SkyDome", true);
	modelParticle_ = Model::CreateFromOBJ("deathParticle", true);
	hitTextureHandle_ = TextureManager::Load("./Resources/hitFlash/hitFlash.png");
	shieldTextureHandle_ = TextureManager::Load("./Resources/shieldFlash/shieldFlash.png");
	modelPauseFont_ = Model::CreateFromOBJ("pauseFont", true);
	modelStageFonts_[0] = Model::CreateFromOBJ("stage1Font", true);
	modelStageFonts_[1] = Model::CreateFromOBJ("stage2Font", true);
	modelStageFonts_[2] = Model::CreateFromOBJ("stage3Font", true);
	modelStageStartFont_ = Model::CreateFromOBJ("stageStartFont", true);
	modelEscFont_ = Model::CreateFromOBJ("escFont", true);

	// ==== プレイヤー(画面中央からスタート) ====
	player_ = new Player();
	player_->Initialize(modelPlayer_, modelBullet_, modelHpBar_, &cameraController_.GetCamera(), 0.0f, 0.0f, shieldTextureHandle_);

	// ==== 天球 ====
	skydome_ = new Skydome();
	skydome_->Initialize(modelSkydome_);

	// ==== スコアバー ====
	worldTransformScoreBar_.Initialize();

	// ==== ステージ表示(SpawnWave内で使うため先に初期化しておく) ====
	worldTransformStageIndicator_.Initialize();
	worldTransformStageIndicator_.scale_ = {1.2f, 1.2f, 1.2f};
	stageIndicatorTimer_ = 0;
	stageIndicatorBounceTimer_ = 0.0f;

	worldTransformStageStartFont_.Initialize();
	worldTransformStageStartFont_.scale_ = {1.0f, 1.0f, 1.0f};

	// ==== ウェーブ・スコア・トラック初期化 ====
	waveNumber_ = 1;
	score_ = 0;
	trackDistance_ = 0.0f;

	// ==== 最初の隊列を生成 ====
	SpawnWave();

	finished_ = false;
	result_ = Result::kNone;
	endDelayTimer_ = 0;
	waitingNextWave_ = false;

	// ==== フェード ====
	fade_ = new Fade();
	fade_->Initialize();
	fade_->StartFadeIn(kFadeDuration);
	exitRequested_ = false;

	// ==== パーティクルエフェクト ====
	for (auto& effect : particleEffects_) {
		effect.isActive = false;
	}
	playerWasAlive_ = true;

	// ==== ポーズメニュー ====
	isPaused_ = false;
	pauseBounceTimer_ = 0.0f;
	quitHoldTimer_ = 0;
	worldTransformPauseIndicator_.Initialize();
	worldTransformPauseIndicator_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransformQuitBar_.Initialize();

	// ==== 「ESCでポーズ」の案内 ====
	worldTransformEscFont_.Initialize();
	worldTransformEscFont_.scale_ = {0.7f, 0.7f, 0.7f};
}

void GameScene::SpawnWave() {
	// 既存の敵を破棄
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	// ウェーブが進むほど隊列の敵の数が増える(1体 → 3体 → 5体…)
	int enemyCount = kBaseEnemyCount + (waveNumber_ - 1) * 2;

	for (int i = 0; i < enemyCount; i++) {
		Enemy* enemy = new Enemy();
		// 横一列に均等に並べて配置する
		float x = enemyCount > 1 ? (-kFormationHalfWidth + (2.0f * kFormationHalfWidth / (enemyCount - 1)) * i) : 0.0f;
		// spawnPosition.z にはトラックからの相対的な先行距離を渡す(カメラと一緒に前進するため)
		enemy->Initialize(modelEnemy_, modelEnemyBullet_, modelHpBar_, &cameraController_.GetCamera(), {x, 0.5f, kEnemyAheadDistance}, waveNumber_, hitTextureHandle_);
		enemies_.push_back(enemy);
	}

	// ==== ステージ表示バナーを表示する ====
	stageIndicatorTimer_ = kStageIndicatorDuration;
	stageIndicatorBounceTimer_ = 0.0f;
	Vector3 playerPos = player_->GetWorldPosition();
	worldTransformStageIndicator_.translation_ = {playerPos.x, playerPos.y + kStageIndicatorYOffset, playerPos.z};
}

bool GameScene::AreAllEnemiesDead() const {
	for (Enemy* enemy : enemies_) {
		if (enemy->IsAlive()) {
			return false;
		}
	}
	return true;
}

GameScene::~GameScene() {
	delete player_;
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	delete skydome_;
	delete modelPlayer_;
	delete modelEnemy_;
	delete modelBullet_;
	delete modelEnemyBullet_;
	delete modelHpBar_;
	delete modelSkydome_;
	delete modelParticle_;
	delete modelPauseFont_;
	delete modelStageFonts_[0];
	delete modelStageFonts_[1];
	delete modelStageFonts_[2];
	delete modelStageStartFont_;
	delete modelEscFont_;
	delete fade_;
}

void GameScene::Update() {
	if (finished_) {
		return;
	}

	fade_->Update();

	// ==== 「ESCでポーズ」の案内をカメラのローカル空間(右・上・前)で配置する ====
	// カメラはX軸周りにしか回転しない(rotation_.x = ピッチ)ため、
	// 前方向・上方向ベクトルはこのピッチ角から直接計算できる
	{
		float pitch = cameraController_.GetCamera().rotation_.x;
		float cosP = cosf(pitch);
		float sinP = sinf(pitch);
		Vector3 camPos = cameraController_.GetCamera().translation_;

		// 前方向ベクトル: (0, -sinP, cosP)、上方向ベクトル: (0, cosP, sinP)、右方向ベクトル: (1, 0, 0)
		worldTransformEscFont_.translation_.x = camPos.x + kEscFontRight;
		worldTransformEscFont_.translation_.y = camPos.y + kEscFontUp * cosP - kEscFontForward * sinP;
		worldTransformEscFont_.translation_.z = camPos.z + kEscFontUp * sinP + kEscFontForward * cosP;
		worldTransformEscFont_.matWorld_ = MakeAffineMatrix(worldTransformEscFont_.scale_, worldTransformEscFont_.rotation_, worldTransformEscFont_.translation_);
		worldTransformEscFont_.TransferMatrix();
	}

	// ==== ポーズの切り替え(決着がついた後は受け付けない) ====
	if (result_ == Result::kNone) {
		Input* input = Input::GetInstance();
		if (input->TriggerKey(DIK_ESCAPE)) {
			isPaused_ = !isPaused_;
			quitHoldTimer_ = 0;
			if (isPaused_) {
				// 一時停止した瞬間のプレイヤー位置を基準に目印を表示する
				Vector3 playerPos = player_->GetWorldPosition();
				pauseIndicatorBaseY_ = playerPos.y + kPauseIndicatorYOffset;
				worldTransformPauseIndicator_.translation_ = {playerPos.x, pauseIndicatorBaseY_, playerPos.z};
				worldTransformQuitBar_.translation_ = {playerPos.x, playerPos.y + kQuitBarYOffset, playerPos.z};
				pauseBounceTimer_ = 0.0f;
			}
		}
	}

	if (isPaused_) {
		// ポーズ中はゲーム世界を完全に停止し、ポーズメニューの処理だけ行う
		UpdatePause();
	} else {
		cameraController_.Update(trackDistance_, player_->GetLocalX(), player_->GetLocalDepth(), player_->IsDashing());
		skydome_->Update();
		UpdateParticleEffects();

		// ==== ステージ表示バナーの更新 ====
		if (stageIndicatorTimer_ > 0) {
			stageIndicatorTimer_--;
			stageIndicatorBounceTimer_ += kStageIndicatorBounceSpeed;
			Vector3 playerPos = player_->GetWorldPosition();
			worldTransformStageIndicator_.translation_.x = playerPos.x;
			worldTransformStageIndicator_.translation_.z = playerPos.z;
			worldTransformStageIndicator_.translation_.y = playerPos.y + kStageIndicatorYOffset + sinf(stageIndicatorBounceTimer_) * kStageIndicatorBounceAmplitude;
			worldTransformStageIndicator_.matWorld_ = MakeAffineMatrix(worldTransformStageIndicator_.scale_, worldTransformStageIndicator_.rotation_, worldTransformStageIndicator_.translation_);
			worldTransformStageIndicator_.TransferMatrix();

			// 「STAGE START」はゲーム開始時(ウェーブ1)のみ、番号バナーの少し上に一緒に表示する
			if (waveNumber_ == 1) {
				float restingY = playerPos.y + kStageStartYOffset + sinf(stageIndicatorBounceTimer_) * kStageIndicatorBounceAmplitude;

				// 表示開始からの経過フレーム数に応じて、上から落ちてくる演出を行う
				int elapsed = kStageIndicatorDuration - stageIndicatorTimer_;
				float dropY = restingY;
				if (elapsed < kStageStartDropDuration) {
					float t = static_cast<float>(elapsed) / static_cast<float>(kStageStartDropDuration);
					float easedT = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // イーズアウト(だんだん減速して着地)
					dropY = restingY + kStageStartDropHeight * (1.0f - easedT);
				}

				worldTransformStageStartFont_.translation_.x = playerPos.x;
				worldTransformStageStartFont_.translation_.z = playerPos.z;
				worldTransformStageStartFont_.translation_.y = dropY;
				worldTransformStageStartFont_.matWorld_ = MakeAffineMatrix(worldTransformStageStartFont_.scale_, worldTransformStageStartFont_.rotation_, worldTransformStageStartFont_.translation_);
				worldTransformStageStartFont_.TransferMatrix();
			}
		}

		// ==== 決着後の演出待ち ====
		if (!player_->IsAlive() || waitingNextWave_) {
			if (endDelayTimer_ > 0) {
				endDelayTimer_--;
			} else if (!player_->IsAlive()) {
				if (result_ == Result::kNone) {
					result_ = Result::kGameOver;
				}
			} else if (waitingNextWave_) {
				// 次のウェーブへ
				waveNumber_++;
				if (waveNumber_ > kMaxWaves) {
					if (result_ == Result::kNone) {
						result_ = Result::kClear;
					}
				} else {
					SpawnWave();
					waitingNextWave_ = false;
				}
			}
			UpdateScoreBar();
		} else if (stageIndicatorTimer_ > 0) {
			// ==== ステージ開始演出中は世界を止めて「これから始まる」間を作る ====
			// (プレビューとして、次のウェーブの敵編隊がこの間に見えている)
			UpdateScoreBar();
		} else {
			// ==== トラックの前進(戦闘中のみ) ====
			trackDistance_ += kScrollSpeed * (player_->IsDashing() ? kDashScrollMultiplier : 1.0f);

			player_->Update(trackDistance_);

			Vector3 playerPos = player_->GetWorldPosition();
			for (Enemy* enemy : enemies_) {
				enemy->Update(playerPos, trackDistance_);
			}

			CheckAllCollisions();

			// ==== 撃破時の分解パーティクル(自機) ====
			if (playerWasAlive_ && !player_->IsAlive()) {
				SpawnParticleEffect(player_->GetWorldPosition());
				playerWasAlive_ = false;
			}

			// ==== 決着判定 ====
			if (!player_->IsAlive()) {
				endDelayTimer_ = kEndDelay;
			} else if (AreAllEnemiesDead()) {
				waitingNextWave_ = true;
				endDelayTimer_ = kEndDelay;
			}

			UpdateScoreBar();
		}
	}

	// ==== 結果が確定したらフェードアウトし、完了後にシーンを終了する ====
	if (result_ != Result::kNone) {
		if (!exitRequested_) {
			exitRequested_ = true;
			fade_->StartFadeOut(kFadeDuration);
		} else if (fade_->IsFinished()) {
			finished_ = true;
		}
	}
}

void GameScene::UpdatePause() {
	Input* input = Input::GetInstance();

	// ポーズ目印のバウンドアニメーション(回転はしない)
	pauseBounceTimer_ += kPauseBounceSpeed;
	worldTransformPauseIndicator_.translation_.y = pauseIndicatorBaseY_ + sinf(pauseBounceTimer_) * kPauseBounceAmplitude;
	worldTransformPauseIndicator_.matWorld_ = MakeAffineMatrix(worldTransformPauseIndicator_.scale_, worldTransformPauseIndicator_.rotation_, worldTransformPauseIndicator_.translation_);
	worldTransformPauseIndicator_.TransferMatrix();

	// ==== Rキー長押しでタイトルへ戻る(誤操作防止のため長押し確認) ====
	if (input->PushKey(DIK_R)) {
		quitHoldTimer_++;
		if (quitHoldTimer_ >= kQuitHoldDuration) {
			result_ = Result::kQuitToTitle;
		}
	} else {
		quitHoldTimer_ = 0;
	}

	// 長押しの進行状況をバーで表示
	float holdRatio = static_cast<float>(quitHoldTimer_) / static_cast<float>(kQuitHoldDuration);
	if (holdRatio > 1.0f)
		holdRatio = 1.0f;
	worldTransformQuitBar_.scale_ = {kQuitBarWidth * holdRatio, kQuitBarHeight, kQuitBarDepth};
	worldTransformQuitBar_.matWorld_ = MakeAffineMatrix(worldTransformQuitBar_.scale_, worldTransformQuitBar_.rotation_, worldTransformQuitBar_.translation_);
	worldTransformQuitBar_.TransferMatrix();
}

void GameScene::UpdateScoreBar() {
	int totalEnemiesAllWaves = 0;
	for (int w = 1; w <= kMaxWaves; w++) {
		totalEnemiesAllWaves += kBaseEnemyCount + (w - 1) * 2;
	}
	int maxScore = kScorePerEnemy * totalEnemiesAllWaves;
	float scoreRatio = maxScore > 0 ? static_cast<float>(score_) / static_cast<float>(maxScore) : 0.0f;
	if (scoreRatio > 1.0f)
		scoreRatio = 1.0f;

	Vector3 playerPos = player_->GetWorldPosition();
	worldTransformScoreBar_.scale_ = {kScoreBarWidth * scoreRatio, kScoreBarHeight, kScoreBarDepth};
	worldTransformScoreBar_.translation_ = {playerPos.x, playerPos.y + kScoreBarYOffset, playerPos.z};
	worldTransformScoreBar_.matWorld_ = MakeAffineMatrix(worldTransformScoreBar_.scale_, worldTransformScoreBar_.rotation_, worldTransformScoreBar_.translation_);
	worldTransformScoreBar_.TransferMatrix();
}

void GameScene::SpawnParticleEffect(const Vector3& position) {
	for (auto& effect : particleEffects_) {
		if (!effect.isActive) {
			effect.particles.Initialize(modelParticle_, &cameraController_.GetCamera(), position);
			effect.isActive = true;
			break;
		}
	}
}

void GameScene::UpdateParticleEffects() {
	for (auto& effect : particleEffects_) {
		if (!effect.isActive)
			continue;

		effect.particles.Update();
		if (effect.particles.IsFinished()) {
			effect.isActive = false;
		}
	}
}

void GameScene::DrawParticleEffects() {
	for (auto& effect : particleEffects_) {
		if (effect.isActive) {
			effect.particles.Draw();
		}
	}
}

void GameScene::Draw() {
	Model::PreDraw();
	skydome_->Draw(cameraController_.GetCamera());
	modelEscFont_->Draw(worldTransformEscFont_, cameraController_.GetCamera());
	player_->Draw();
	for (Enemy* enemy : enemies_) {
		enemy->Draw();
	}
	if (score_ > 0) {
		modelHpBar_->Draw(worldTransformScoreBar_, cameraController_.GetCamera());
	}
	DrawParticleEffects();
	if (stageIndicatorTimer_ > 0) {
		modelStageFonts_[waveNumber_ - 1]->Draw(worldTransformStageIndicator_, cameraController_.GetCamera());
		if (waveNumber_ == 1) {
			modelStageStartFont_->Draw(worldTransformStageStartFont_, cameraController_.GetCamera());
		}
	}
	if (isPaused_) {
		modelPauseFont_->Draw(worldTransformPauseIndicator_, cameraController_.GetCamera());
		if (quitHoldTimer_ > 0) {
			modelHpBar_->Draw(worldTransformQuitBar_, cameraController_.GetCamera());
		}
	}
	Model::PostDraw();

	// フェードの描画(必ず末尾)
	fade_->Draw();
}

void GameScene::CheckAllCollisions() {
	const float kBulletHalfSize = 0.3f;

	// ==== 自機弾 vs 敵(全体) ====
	Player::Bullet* playerBullets = player_->GetBullets();
	for (int i = 0; i < Player::kMaxBullets; i++) {
		if (!playerBullets[i].isActive)
			continue;

		Vector3 bulletPos = playerBullets[i].worldTransform.translation_;
		AABB bulletAABB;
		bulletAABB.min = {bulletPos.x - kBulletHalfSize, bulletPos.y - kBulletHalfSize, bulletPos.z - kBulletHalfSize};
		bulletAABB.max = {bulletPos.x + kBulletHalfSize, bulletPos.y + kBulletHalfSize, bulletPos.z + kBulletHalfSize};

		for (Enemy* enemy : enemies_) {
			if (!enemy->IsAlive())
				continue;

			if (IsCollision(bulletAABB, enemy->GetAABB())) {
				bool wasAlive = enemy->IsAlive();
				enemy->TakeDamage();
				if (playerBullets[i].isCharged) {
					enemy->TakeDamage();
				}
				SpawnParticleEffect(bulletPos);
				playerBullets[i].isActive = false;

				if (wasAlive && !enemy->IsAlive()) {
					SpawnParticleEffect(enemy->GetWorldPosition());
					score_ += kScorePerEnemy;
				}
				break;
			}
		}
	}

	// ==== 敵弾(全体) vs 自機 ====
	if (player_->IsAlive()) {
		AABB playerAABB = player_->GetAABB();

		for (Enemy* enemy : enemies_) {
			Enemy::Bullet* enemyBullets = enemy->GetBullets();
			for (int j = 0; j < Enemy::kMaxBullets; j++) {
				if (!enemyBullets[j].isActive)
					continue;

				Vector3 bulletPos = enemyBullets[j].worldTransform.translation_;
				AABB bulletAABB;
				bulletAABB.min = {bulletPos.x - kBulletHalfSize, bulletPos.y - kBulletHalfSize, bulletPos.z - kBulletHalfSize};
				bulletAABB.max = {bulletPos.x + kBulletHalfSize, bulletPos.y + kBulletHalfSize, bulletPos.z + kBulletHalfSize};

				if (IsCollision(bulletAABB, playerAABB)) {
					player_->TakeDamage();
					SpawnParticleEffect(bulletPos);
					enemyBullets[j].isActive = false;
				}
			}
		}
	}
}