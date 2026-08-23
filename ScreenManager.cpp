#include "SceneManager.h"

using namespace KamataEngine;

// コンストラクタ
SceneManager::SceneManager() {
	// カメラの初期化(3Dシーンには必須)
	camera_.Initialize();

	// 各シーンの生成
	// 今回は例としてわかりやすくするために、SceneManagerクラスの
	// コンストラクタ内で全てのシーンを生成しているが、メモリ効率を
	// 考えるとシーン生成のタイミングについてはもう少しスマートに行うべきである
	sceneArr_[TITLE] = std::make_unique<TitleScene>();
	sceneArr_[STAGE] = std::make_unique<StageScene>();
	sceneArr_[CLEAR] = std::make_unique<ClearScene>();
	sceneArr_[GAMEOVER] = std::make_unique<GameOverScene>();

	// 全シーンにカメラを共有する
	for (auto& scene : sceneArr_) {
		scene->SetCamera(&camera_);
	}

	// 初期シーンの設定
	currentSceneNo_ = TITLE;
	prevSceneNo_ = TITLE;

	sceneArr_[currentSceneNo_]->Initialize();
}

// デストラクタ
SceneManager::~SceneManager() {}

void SceneManager::Update() {
	// シーンのチェック
	prevSceneNo_ = currentSceneNo_;
	currentSceneNo_ = sceneArr_[currentSceneNo_]->GetSceneNo();

	// シーン変更チェック
	if (prevSceneNo_ != currentSceneNo_) {
		sceneArr_[currentSceneNo_]->Initialize();
	}

	// カメラの行列更新
	// ※お使いのKamataEngineのバージョンによっては
	//   UpdateMatrix() ではなく TransferMatrix() という名前の場合があります。
	//   コンパイルエラーになった場合はそちらに読み替えてください。
	camera_.UpdateMatrix();

	// 更新処理
	sceneArr_[currentSceneNo_]->Update(); // シーンごとの更新処理
}

void SceneManager::Draw() {
	// モデル描画の準備(cmdListのセットアップ)。これが無いとModel::Draw()内で
	// cmdListがnullptrのままクラッシュします。
	Model::PreDraw();

	// 描画処理
	sceneArr_[currentSceneNo_]->Draw();

	Model::PostDraw();
}