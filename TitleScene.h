#pragma once
#include "Fade.h"
#include "KamataEngine.h"

class TitleScene {
public:
	~TitleScene();
	void Initialize();
	void Update();
	void Draw();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }

private:
	// 終了フラグ
	bool finished_ = false;

	// フェード
	Fade* fade_ = nullptr;
};