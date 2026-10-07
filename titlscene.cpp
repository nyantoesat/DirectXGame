#include "TitleScene.h"

using namespace KamataEngine;

TitleScene::~TitleScene() {
	delete background_;
	delete promptSprite_;
	delete logoSprite_;
	for (auto* block : decoBlocks_) {
		delete block;
	}
	for (auto* sprite : buttonSprites_) {
		delete sprite;
	}
}

void TitleScene::LoadButtonTextures() {
	// NOTE: no button artwork exists yet, so every option currently uses
	// textureHandle_ (white1x1.png, already loaded in Initialize()) as a
	// plain colored-rectangle placeholder - see the per-button tint
	// colors in Draw(). Once you have real button images:
	//   1. Put each file alongside your other texture assets (same place
	//      as "white1x1.png").
	//   2. Replace the textureHandle_ below with
	//      TextureManager::Load("yourFile.png") for that option.
	//   3. Delete that option's SetColor() call in Draw() (or pass
	//      {1,1,1,1}) - a real image should show its own colors, not a
	//      tint - though you can keep dimming the UNSELECTED ones
	//      slightly if you still want that highlight effect.
	for (int i = 0; i < kMenuOptionCount; ++i) {
		buttonTextureHandles_[i] = textureHandle_;
	}
}

void TitleScene::Initialize() {
	textureHandle_ = TextureManager::Load("white1x1.png");

	background_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.08f, 0.07f, 0.12f, 1.0f});
	background_->SetSize({kScreenWidth, kScreenHeight});

	// Same palette as GameScene's, just to keep the title screen visually
	// consistent with the game itself.
	static const Vector4 kPalette[] = {
	    {0.95f, 0.55f, 0.25f, 1.0f},
        {0.35f, 0.75f, 0.95f, 1.0f},
        {0.85f, 0.35f, 0.45f, 1.0f},
        {0.55f, 0.85f, 0.45f, 1.0f},
        {0.75f, 0.55f, 0.95f, 1.0f},
	};

	decoBlocks_.clear();
	decoBlocks_.reserve(kDecoBlockCount);
	float offset = 0.0f;
	for (int i = 0; i < kDecoBlockCount; ++i) {
		float x = kScreenWidth * 0.5f - kDecoBlockWidth * 0.5f + offset;
		float y = kDecoBaseY - kDecoBlockHeight * static_cast<float>(i + 1);
		Sprite* block = Sprite::Create(textureHandle_, {x, y}, kPalette[i % 5]);
		block->SetSize({kDecoBlockWidth, kDecoBlockHeight});
		decoBlocks_.push_back(block);
		offset += (i % 2 == 0 ? 12.0f : -18.0f); // gentle zigzag lean, purely decorative
	}

	if (!promptSprite_) {
		promptSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.95f, 0.9f, 0.6f, 1.0f});
		promptSprite_->SetSize({kPromptWidth, kPromptHeight});
	}
	promptSprite_->SetPosition({kScreenWidth * 0.5f - kPromptWidth * 0.5f, kPromptY});

	// NOTE: this is a placeholder rectangle standing in for the title
	// logo/image. Once you have the artwork:
	//   1. Put the image file alongside your other texture assets (same
	//      place as "white1x1.png").
	//   2. Change the Load() call below to that filename.
	//   3. Update kLogoWidth/kLogoHeight (in TitleScene.h) to the image's
	//      real size, or whatever size you want it drawn at.
	//   4. Delete the SetColor() call in Draw() below (or just don't add
	//      one) - a real image should show its own colors, not a tint.
	logoTextureHandle_ = TextureManager::Load("white1x1.png");
	if (!logoSprite_) {
		logoSprite_ = Sprite::Create(logoTextureHandle_, {0.0f, 0.0f}, {0.9f, 0.85f, 0.6f, 1.0f});
		logoSprite_->SetSize({kLogoWidth, kLogoHeight});
	}
	logoSprite_->SetPosition({kScreenWidth * 0.5f - kLogoWidth * 0.5f, kLogoY});

	// Menu buttons: one image sprite per option, built once here. Draw()
	// resizes/retints whichever is selected each frame - see the class
	// comment in TitleScene.h.
	LoadButtonTextures();
	for (int i = 0; i < kMenuOptionCount; ++i) {
		delete buttonSprites_[i];
		buttonSprites_[i] = Sprite::Create(buttonTextureHandles_[i], {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
		buttonSprites_[i]->SetSize({kButtonWidth, kButtonHeight});
	}

	mode_ = Mode::kMenu;
	selectedOption_ = kMenuOptionPlay;
	blinkTimer_ = 0.0f;
	blinkVisible_ = true;
	finished_ = false;
	wantsExit_ = false;
}

void TitleScene::Update() {
	Input* input = Input::GetInstance();
	constexpr float kDeltaTime = 1.0f / 60.0f;

	blinkTimer_ += kDeltaTime;
	if (blinkTimer_ >= kBlinkInterval) {
		blinkTimer_ -= kBlinkInterval;
		blinkVisible_ = !blinkVisible_;
	}

	if (mode_ == Mode::kMenu) {
		UpdateMenu(input);
	} else {
		UpdateExplanation(input);
	}
}

void TitleScene::UpdateMenu(Input* input) {
	if (input->TriggerKey(DIK_UP)) {
		selectedOption_ = (selectedOption_ + kMenuOptionCount - 1) % kMenuOptionCount;
	}
	if (input->TriggerKey(DIK_DOWN)) {
		selectedOption_ = (selectedOption_ + 1) % kMenuOptionCount;
	}

	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
		switch (selectedOption_) {
		case kMenuOptionPlay:
			finished_ = true; // main.cpp's IsFinished() check picks this up
			break;
		case kMenuOptionInfo:
			mode_ = Mode::kExplanation;
			blinkTimer_ = 0.0f;
			blinkVisible_ = true;
			break;
		case kMenuOptionExit:
			wantsExit_ = true; // main.cpp's WantsExit() check must break its loop - see the NOTE in TitleScene.h
			break;
		}
	}
}

void TitleScene::UpdateExplanation(Input* input) {
	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_ESCAPE)) {
		mode_ = Mode::kMenu;
		blinkTimer_ = 0.0f;
		blinkVisible_ = true;
	}
}

void TitleScene::Draw() {
	// Same PreDraw/PostDraw pairing as GameScene::Draw() - see the NOTE
	// there if your packaged KamataEngine facade wraps this differently.
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (background_) {
		background_->Draw();
	}

	if (mode_ == Mode::kMenu) {
		if (logoSprite_) {
			logoSprite_->Draw();
		}
		for (auto* block : decoBlocks_) {
			if (block) {
				block->Draw();
			}
		}

		// Each option gets its own placeholder color (see
		// LoadButtonTextures()'s NOTE) so the three buttons read as
		// distinct even before real images exist - dimmed when
		// unselected, full brightness (and slightly bigger) when
		// selected.
		static const Vector4 kButtonNormalColors[kMenuOptionCount] = {
		    {0.25f, 0.5f,  0.3f,  1.0f}, // Play - dim green
		    {0.25f, 0.45f, 0.6f,  1.0f}, // Info - dim blue
		    {0.55f, 0.25f, 0.25f, 1.0f}, // Exit - dim red
		};
		static const Vector4 kButtonSelectedColors[kMenuOptionCount] = {
		    {0.45f, 0.9f, 0.55f, 1.0f}, // Play - bright green
		    {0.45f, 0.8f, 1.0f,  1.0f}, // Info - bright blue
		    {1.0f,  0.4f, 0.4f,  1.0f}, // Exit - bright red
		};

		for (int i = 0; i < kMenuOptionCount; ++i) {
			Sprite* button = buttonSprites_[i];
			if (!button) {
				continue;
			}

			bool selected = (i == selectedOption_);
			float width = kButtonWidth * (selected ? kButtonSelectedScale : 1.0f);
			float height = kButtonHeight * (selected ? kButtonSelectedScale : 1.0f);
			float centerY = kButtonStartCenterY + kButtonSpacingY * static_cast<float>(i);

			button->SetSize({width, height});
			button->SetPosition({kButtonCenterX - width * 0.5f, centerY - height * 0.5f});
			button->SetColor(selected ? kButtonSelectedColors[i] : kButtonNormalColors[i]);
			button->Draw();
		}
	} else {
		// Explanation screen: no real content yet (see the kBlinkInterval
		// comment in TitleScene.h) - just the "press space to go back"
		// prompt, same blinking-bar technique the old title screen used.
		if (blinkVisible_ && promptSprite_) {
			promptSprite_->Draw();
		}
	}

	Sprite::PostDraw();
}