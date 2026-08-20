#pragma once

#include "ClearScene.h"
#include "GameOverScene.h"
#include "IScene.h" // シーンの基底クラス
#include "KamataEngine.h"
#include "StageScene.h"
#include "TitleScene.h" // 各シーンのヘッダ
#include <memory>       // std::unique_ptrを使うため

// どのシーンを呼び出すかを管理するクラス
// (2D版のGameManagerと同じ役割。3DなのでCameraを1つ持って各シーンに配る)
class SceneManager {
private:
	// シーンを保持するメンバ変数
	std::unique_ptr<IScene> sceneArr_[4];

	// どのシーンを呼び出すかを管理
	SceneName currentSceneNo_; // 現在のシーン番号
	SceneName prevSceneNo_;    // 前回のシーン番号

	// 3Dなので全シーン共通のカメラを1つ持つ
	KamataEngine::Camera camera_;

public:
	SceneManager();  // コンストラクタ
	~SceneManager(); // デストラクタ

	void Update(); // 毎フレームの更新
	void Draw();   // 毎フレームの描画
};