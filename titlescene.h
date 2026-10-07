#pragma once

// See the NOMINMAX comment in GameScene.h - same landmine applies here if
// this scene ever calls std::min/std::max, so it's defined defensively
// before the first KamataEngine.h include in this translation unit.
#define NOMINMAX

#include "KamataEngine.h"
#include <vector>

/// <summary>
/// Title screen. Shows a decorative little block stack behind the game's
/// name, plus a 3-option menu (Play / Info / Exit) navigated with
/// up/down and confirmed with SPACE/ENTER:
///   - Play: finishes this scene (see IsFinished()) so main.cpp tears it
///     down and spins up GameScene.
///   - Info: switches to a simple how-to-play sub-screen within this
///     same scene (Mode::kExplanation); SPACE/ENTER there returns to the
///     menu.
///   - Exit: sets WantsExit(), which main.cpp checks to break its main
///     loop and close the window - see the NOTE on WantsExit() below.
///
/// Each menu option is a single button IMAGE eventually (see the NOTE in
/// TitleScene.cpp's LoadButtonTextures()) - for now, since no artwork
/// exists yet, each button is a plain colored rectangle (white1x1.png
/// tinted per-option) so the menu can be tested before the real images
/// are ready. Swapping in real images later is a one-line change per
/// button (see the NOTE). The selected button is highlighted by
/// brightening its tint and scaling it up slightly (see
/// kButtonSelectedScale) - that part doesn't change either way.
///
/// Mirrors GameScene's structure on purpose (own texture load, own
/// Initialize/Update/Draw, deletes its own sprites in the destructor) so
/// it's easy to eyeball against GameScene.cpp if something looks off.
/// </summary>
class TitleScene {
public:
	~TitleScene();

	void Initialize();
	void Update();
	void Draw();

	// True once "Play" has been selected and confirmed; main.cpp checks
	// this once per frame and swaps to GameScene when it flips.
	bool IsFinished() const { return finished_; }

	// True once "Exit" has been selected and confirmed. NOTE: this scene
	// only ever sets the flag - it has no way to close the window or
	// break main.cpp's loop itself. main.cpp must check this once per
	// frame (the same way it already checks IsFinished()) and, when
	// true, break out of its main while(true) loop so the existing
	// cleanup/KamataEngine::Finalize() code at the bottom of WinMain
	// runs and the app closes.
	bool WantsExit() const { return wantsExit_; }

private:
	enum class Mode {
		kMenu,        // the 3-option Play/Info/Exit list
		kExplanation, // simple placeholder how-to-play screen, entered from Info
	};

	enum MenuOption {
		kMenuOptionPlay = 0,
		kMenuOptionInfo = 1,
		kMenuOptionExit = 2,
		kMenuOptionCount = 3,
	};

	static constexpr float kScreenWidth = 1280.0f;
	static constexpr float kScreenHeight = 720.0f;

	// Small leaning stack drawn behind the title, purely cosmetic -
	// echoes the real game's tower without being tied to it in any way.
	// Only shown in Mode::kMenu, not during the explanation screen.
	static constexpr int kDecoBlockCount = 5;
	static constexpr float kDecoBlockWidth = 160.0f;
	static constexpr float kDecoBlockHeight = 48.0f;
	static constexpr float kDecoBaseY = 620.0f;

	// Placeholder for the title logo/image. See the NOTE in
	// Initialize() for exactly what to change once you have the real
	// artwork - kept as its own texture handle/sprite, separate from the
	// white1x1 elements below, so swapping the file is a one-line change.
	static constexpr float kLogoWidth = 480.0f;
	static constexpr float kLogoHeight = 160.0f;
	static constexpr float kLogoY = 90.0f;

	// Menu buttons - one image per option (file paths are in
	// TitleScene.cpp's kButtonFiles, next to LoadButtonTextures()).
	// kButtonWidth/Height is the NORMAL (unselected) display size; the
	// selected button is drawn at kButtonSelectedScale times that,
	// centered on the same spot rather than growing off to one side (see
	// Draw()) - purely a size+tint change, no separate "selected" image.
	static constexpr float kButtonWidth = 220.0f;
	static constexpr float kButtonHeight = 64.0f;
	static constexpr float kButtonSelectedScale = 1.12f;
	static constexpr float kButtonCenterX = kScreenWidth * 0.5f;
	static constexpr float kButtonStartCenterY = 320.0f;
	static constexpr float kButtonSpacingY = 90.0f;

	// The explanation (Info) screen has no real content yet - just a
	// distinct background tint and a "press space to go back" prompt
	// bar, same blinking-bar technique as the old single title prompt.
	// Replace/extend this once you have instructional images/content.
	static constexpr float kBlinkInterval = 0.5f;
	static constexpr float kPromptWidth = 260.0f;
	static constexpr float kPromptHeight = 20.0f;
	static constexpr float kPromptY = 560.0f;

	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* background_ = nullptr;
	KamataEngine::Sprite* promptSprite_ = nullptr; // "press space to go back", explanation screen only
	std::vector<KamataEngine::Sprite*> decoBlocks_;

	uint32_t logoTextureHandle_ = 0;
	KamataEngine::Sprite* logoSprite_ = nullptr; // placeholder for the title logo/image

	// One texture + one sprite per menu option, indexed by MenuOption
	// (kMenuOptionPlay/Info/Exit). All three currently share
	// textureHandle_ (white1x1.png) as PLACEHOLDERS - see the NOTE in
	// LoadButtonTextures() for exactly what to change once you have real
	// button images. Created once in Initialize(); Draw() just resizes/
	// retints whichever one is currently selected each frame rather than
	// recreating anything.
	uint32_t buttonTextureHandles_[kMenuOptionCount] = {};
	KamataEngine::Sprite* buttonSprites_[kMenuOptionCount] = {nullptr, nullptr, nullptr};

	Mode mode_ = Mode::kMenu;
	int selectedOption_ = kMenuOptionPlay;

	float blinkTimer_ = 0.0f;
	bool blinkVisible_ = true;
	bool finished_ = false;
	bool wantsExit_ = false;

	void LoadButtonTextures();
	void UpdateMenu(KamataEngine::Input* input);
	void UpdateExplanation(KamataEngine::Input* input);
};