#pragma once
#include "KamataEngine.h"

// シーン名を列挙型(Enum)で定義
enum SceneName { TITLE, STAGE, CLEAR, GAMEOVER };

// シーン内での処理を行う基底クラス
class IScene {
protected:
	// シーン番号を管理する変数(2Dの頃と同じ考え方)
	static SceneName sceneNo;

	// SceneManagerから共有してもらうカメラ(3Dでは全シーン共通で1つ持つのが基本)
	KamataEngine::Camera* camera_ = nullptr;

public:
	// 継承先で実装される関数
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	// 仮想デストラクタを用意しないと警告される
	virtual ~IScene();

	// シーン番号のゲッター
	SceneName GetSceneNo();

	// SceneManagerがカメラを渡すためのセッター
	void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }
};