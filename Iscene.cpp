#include "IScene.h"

// タイトルシーンで初期化
SceneName IScene::sceneNo = TITLE;

// 仮想デストラクタの定義
IScene::~IScene() {}

// シーン番号のゲッター
SceneName IScene::GetSceneNo() { return sceneNo; }