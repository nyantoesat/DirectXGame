#include "GameScene.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>

using namespace KamataEngine;

GameScene::~GameScene() {
	for (auto& block : blocks_) {
		delete block.sprite;
	}
	delete swingingBlock_.sprite;
	delete background_;
	delete backgroundTileSprites_[0];
	delete backgroundTileSprites_[1];
	delete anchorSprite_;
	for (auto* seg : ropeSegments_) {
		delete seg;
	}
	for (auto* seg : scoreLabelSprites_) {
		delete seg;
	}
	for (auto* seg : highScoreLabelSprites_) {
		delete seg;
	}
	for (auto* seg : scoreDigitSprites_) {
		delete seg;
	}
	for (auto* seg : highScoreDigitSprites_) {
		delete seg;
	}
	for (auto* seg : lifeIconSegments_) {
		delete seg;
	}
	for (auto& particle : particles_) {
		delete particle.sprite;
	}
}

void GameScene::Initialize() {
	// Seeded once here (not per-particle) so debris spread looks different
	// run to run without reseeding constantly.
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	highScore_ = 0;
	std::ifstream highScoreFile(kHighScoreFilePath);
	if (highScoreFile) {
		highScoreFile >> highScore_;
		if (!highScoreFile) {
			highScore_ = 0; // file existed but didn't contain a parseable number
		}
	}

	textureHandle_ = TextureManager::Load("white1x1.png");

	// The 3 block colors are real sprite images now, not tints of
	// white1x1.png - one 3-frame animation each (see kColorCycleCount/
	// kBlockAnimFrameCount and ApplyBlockTexture()). Filenames/order here
	// must match colorIndex 0/1/2 used everywhere else (NextPlacedColorIndex(),
	// SpawnSwingingBlock(), the bounce color-match check in
	// ResolveLanding()). Each color's 3 frames live in their own
	// subfolder under Resources.
	static const char* kBlockTextureFiles[kColorCycleCount][kBlockAnimFrameCount] = {
	    {"./Resources/redBlock/redBlock1.png",       "./Resources/redBlock/redBlock2.png",       "./Resources/redBlock/redBlock3.png"      },
	    {"./Resources/greenBlock/greenBlock1.png",   "./Resources/greenBlock/greenBlock2.png",   "./Resources/greenBlock/greenBlock3.png"  },
	    {"./Resources/purpleBlock/purpleBlock1.png", "./Resources/purpleBlock/purpleBlock2.png", "./Resources/purpleBlock/purpleBlock3.png"},
	};
	for (int c = 0; c < kColorCycleCount; ++c) {
		for (int f = 0; f < kBlockAnimFrameCount; ++f) {
			blockTextureHandles_[c][f] = TextureManager::Load(kBlockTextureFiles[c][f]);
		}
	}

	palette_ = {
	    {0.95f, 0.55f, 0.25f, 1.0f},
        {0.35f, 0.75f, 0.95f, 1.0f},
        {0.85f, 0.35f, 0.45f, 1.0f},
        {0.55f, 0.85f, 0.45f, 1.0f},
        {0.75f, 0.55f, 0.95f, 1.0f},
	};

	background_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.08f, 0.07f, 0.12f, 1.0f});
	background_->SetSize({kScreenWidth, kScreenHeight});
	LoadBackgroundTextures();

	// Pivot marker + the string itself (a handful of small beads spaced
	// along the anchor-to-block line; repositioned every frame in
	// SyncSpritePositions rather than recreated, so this pool is built once).
	anchorSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.55f, 0.55f, 0.6f, 1.0f});
	anchorSprite_->SetSize({kAnchorWidth, kAnchorHeight});

	ropeSegments_.reserve(kRopeSegmentCount);
	for (int i = 0; i < kRopeSegmentCount; ++i) {
		Sprite* segment = Sprite::Create(textureHandle_, {0.0f, 0.0f}, {0.65f, 0.6f, 0.55f, 1.0f});
		segment->SetSize({kRopeSegmentSize, kRopeSegmentSize});
		ropeSegments_.push_back(segment);
	}

	// Score/high-score display: real character images instead of drawn
	// segments (see LoadGlyphs()). Labels are static text, built once;
	// digit rows start at "0" here and get rebuilt only when score_/
	// highScore_ actually change (ResolveLanding()/SaveHighScoreIfNeeded()).
	LoadGlyphs();
	scoreLabelSprites_ = BuildLabelSprites("SCORE", kScoreRowX, kScoreRowY);
	highScoreLabelSprites_ = BuildLabelSprites("HIGH", kHighScoreRowX, kHighScoreRowY);
	RebuildDigitSprites(0, scoreDigitSprites_, kScoreRowX + static_cast<float>(kScoreLabelLength) * kGlyphSpacing, kScoreRowY);
	RebuildDigitSprites(highScore_, highScoreDigitSprites_, kHighScoreRowX + static_cast<float>(kHighScoreLabelLength) * kGlyphSpacing, kHighScoreRowY);

	// Life icons: kStartingLives sprites, top-left. Tinted per-frame by
	// DrawLives() based on the current lives_ - built once here, never
	// recreated (unlike the block textures above, this is just a color
	// change, not a texture swap).
	lifeIconTextureHandle_ = TextureManager::Load(kLifeIconPath);
	lifeIconSegments_.reserve(kStartingLives);
	for (int i = 0; i < kStartingLives; ++i) {
		float x = kLifeIconX + static_cast<float>(i) * kLifeIconSpacing;
		Sprite* icon = Sprite::Create(lifeIconTextureHandle_, {x, kLifeIconY}, {1.0f, 1.0f, 1.0f, 1.0f});
		icon->SetSize({kLifeIconSize, kLifeIconSize});
		lifeIconSegments_.push_back(icon);
	}

	Reset();
}

void GameScene::LoadGlyphs() {
	// Every character actually used on screen: digits for both score
	// rows, plus exactly the letters needed to spell "SCORE" and "HIGH"
	// (H appears in both spots of "HIGH" - one texture handle, reused
	// twice when the label is built).
	static const char kGlyphChars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'S', 'C', 'O', 'R', 'E', 'H', 'I', 'G'};
	static const char* kGlyphFiles[] = {
	    "./Resources/font/0.png", "./Resources/font/1.png", "./Resources/font/2.png", "./Resources/font/3.png", "./Resources/font/4.png", "./Resources/font/5.png",
	    "./Resources/font/6.png", "./Resources/font/7.png", "./Resources/font/8.png", "./Resources/font/9.png", "./Resources/font/S.png", "./Resources/font/C.png",
	    "./Resources/font/O.png", "./Resources/font/R.png", "./Resources/font/E.png", "./Resources/font/H.png", "./Resources/font/I.png", "./Resources/font/G.png",
	};
	for (size_t i = 0; i < sizeof(kGlyphChars) / sizeof(kGlyphChars[0]); ++i) {
		glyphTextureHandles_[kGlyphChars[i]] = TextureManager::Load(kGlyphFiles[i]);
	}
}

std::vector<Sprite*> GameScene::BuildLabelSprites(const std::string& text, float originX, float originY) {
	std::vector<Sprite*> sprites;
	sprites.reserve(text.size());
	for (size_t i = 0; i < text.size(); ++i) {
		auto it = glyphTextureHandles_.find(text[i]);
		if (it == glyphTextureHandles_.end()) {
			continue; // no glyph loaded for this character - skip rather than crash
		}
		float x = originX + static_cast<float>(i) * kGlyphSpacing;
		Sprite* sprite = Sprite::Create(it->second, {x, originY}, {1.0f, 1.0f, 1.0f, 1.0f});
		sprite->SetSize({kGlyphSize, kGlyphSize});
		sprites.push_back(sprite);
	}
	return sprites;
}

void GameScene::RebuildDigitSprites(int value, std::vector<Sprite*>& sprites, float originX, float originY) {
	for (auto* sprite : sprites) {
		delete sprite;
	}
	sprites.clear();
	sprites.resize(kScoreDigitCount, nullptr); // fixed slot count - nullptr means "blank" (a leading zero)

	if (value < 0) {
		value = 0;
	}
	int maxValue = 1;
	for (int i = 0; i < kScoreDigitCount; ++i) {
		maxValue *= 10;
	}
	if (value > maxValue - 1) {
		value = maxValue - 1; // clamp display only; the underlying value is never touched
	}

	int digits[kScoreDigitCount];
	int remaining = value;
	for (int i = kScoreDigitCount - 1; i >= 0; --i) {
		digits[i] = remaining % 10;
		remaining /= 10;
	}

	bool leadingZero = true;
	for (int i = 0; i < kScoreDigitCount; ++i) {
		bool isLastDigit = (i == kScoreDigitCount - 1);
		if (digits[i] == 0 && leadingZero && !isLastDigit) {
			continue; // leave this slot nullptr - blank leading zeroes, e.g. show "7" instead of "0007"
		}
		leadingZero = false;

		char c = static_cast<char>('0' + digits[i]);
		auto it = glyphTextureHandles_.find(c);
		if (it == glyphTextureHandles_.end()) {
			continue;
		}
		float x = originX + static_cast<float>(i) * kGlyphSpacing;
		Sprite* sprite = Sprite::Create(it->second, {x, originY}, {1.0f, 1.0f, 1.0f, 1.0f});
		sprite->SetSize({kGlyphSize, kGlyphSize});
		sprites[i] = sprite;
	}
}

void GameScene::DrawGlyphRow(const std::vector<Sprite*>& sprites) const {
	for (auto* sprite : sprites) {
		if (sprite) {
			sprite->Draw();
		}
	}
}

void GameScene::Reset() {
	for (auto& block : blocks_) {
		delete block.sprite;
	}
	blocks_.clear();

	for (auto& particle : particles_) {
		delete particle.sprite;
	}
	particles_.clear();

	cameraY_ = 0.0f;
	score_ = 0;
	RebuildDigitSprites(score_, scoreDigitSprites_, kScoreRowX + static_cast<float>(kScoreLabelLength) * kGlyphSpacing, kScoreRowY);
	lives_ = kStartingLives;
	streak_ = 0;
	nextPlacedColorIndex_ = 0;
	anchorMoveTimer_ = 0.0f;
	windTimer_ = 0.0f;
	nextWindEventTime_ = kWindEventMinInterval;
	windGustTimer_ = kWindGustDuration; // "already finished" - no gust right at run start
	windOffsetX_ = 0.0f;
	isRestZoneActive_ = false;
	wobbleOffset_ = 0.0f;
	wobbleVelocity_ = 0.0f;
	collapseTimer_ = 0.0f;
	shakeElapsed_ = kShakeDuration; // fully decayed - no leftover shake from a previous run
	shakeMagnitude_ = 0.0f;
	shakeOffsetX_ = 0.0f;
	shakeOffsetY_ = 0.0f;
	state_ = State::kSwinging;

	if (background_) {
		background_->SetColor({0.08f, 0.07f, 0.12f, 1.0f});
	}

	// Base block is placed for free, centered, so the first swing always
	// has something legitimate to land on.
	Block base;
	base.x = kScreenWidth * 0.5f - kBlockWidth * 0.5f;
	base.y = kGroundY - kBlockHeight;
	base.width = kBlockWidth;
	base.height = kBlockHeight;
	base.colorIndex = NextPlacedColorIndex();
	base.color = palette_[base.colorIndex];
	base.weight = kBlockWeightUnits; // resting flat on the ground - fully supported, max weight
	base.colorCycleTimer = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kColorCycleInterval;
	base.animTimer = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kBlockAnimFrameInterval;
	ApplyBlockTexture(base);
	blocks_.push_back(base);

	SpawnSwingingBlock();
}

void GameScene::SpawnSwingingBlock() {
	state_ = State::kSwinging; // hand control back to the string, not gravity

	// Fixed pivot: horizontally dead center always (does NOT track the
	// stack, even if it's leaning), vertically pinned to a fixed screen
	// position via the current camera offset (screenY = anchorY_ -
	// cameraY_ = kAnchorScreenY, always).
	anchorX_ = kAnchorScreenX;
	anchorY_ = kAnchorScreenY + cameraY_;
	swingTime_ = 0.0f;

	// Difficulty ramps with height: the pendulum swings faster, swings
	// wider (bigger max angle), AND the string itself gets longer - a
	// longer rope makes the swing arc wider and harder to read at a
	// glance, and means a longer fall once released.
	swingSpeed_ = std::min(4.0f, 1.6f + 0.08f * static_cast<float>(blocks_.size()));
	swingMaxAngle_ = std::min(1.05f, 0.55f + 0.02f * static_cast<float>(blocks_.size()));
	ropeLength_ = std::min(kRopeLengthMax, kRopeLengthBase + kRopeLengthGrowthPerBlock * static_cast<float>(blocks_.size()));

	// Rest zone: every kRestZoneInterval-th successful placement, the
	// NEXT block gets an easier, narrower swing instead of the ramped-up
	// values above - see isRestZoneActive_'s use in Update() (wind/anchor
	// drift both check this flag too).
	isRestZoneActive_ = score_ > 0 && score_ % kRestZoneInterval == 0;
	if (isRestZoneActive_) {
		swingSpeed_ = kRestZoneSwingSpeed;
		swingMaxAngle_ = kRestZoneSwingAngle;
	}

	// A fresh block never carries over rigid-body state from a previous
	// bounce (see TriggerBounce()) - it's back on the string, not flying
	// off screen.
	swingingBlock_.velocityX = 0.0f;
	swingingBlock_.velocityY = 0.0f;
	swingingBlock_.rotation = 0.0f;
	swingingBlock_.angularVelocity = 0.0f;

	swingingBlock_.width = kBlockWidth;
	swingingBlock_.height = kBlockHeight;
	// angle = 0 at t = 0 -> block hangs straight down from the anchor.
	swingingBlock_.x = anchorX_ - kBlockWidth * 0.5f;
	swingingBlock_.y = anchorY_ + ropeLength_;

	// Same 3-color set the placed blocks cycle through (see
	// kColorCycleCount/UpdatePlacedBlockColors()), so the block on the
	// string always previews a color it could keep once placed - but
	// picked randomly here rather than via NextPlacedColorIndex(), since
	// this block hasn't been placed yet.
	swingingBlock_.colorIndex = std::rand() % kColorCycleCount;
	swingingBlock_.color = palette_[swingingBlock_.colorIndex];
	swingingBlock_.animTimer = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kBlockAnimFrameInterval;
	ApplyBlockTexture(swingingBlock_);
}

void GameScene::BeginFall() {
	// Horizontal position is locked in at the moment of release - the
	// block doesn't drift sideways as it falls, only straight down under
	// gravity - so whatever it's going to hit is already decided right
	// here. This is a real AABB collision test against the block it's
	// being dropped onto (the current top of the stack): the block can
	// ONLY come to rest there if their x-ranges actually intersect at
	// all. A near-miss with real but tiny overlap still counts as a
	// collision (it physically touches, precariously); only a true
	// non-overlap - the ranges don't intersect at all - means there's
	// nothing to land on.
	float newLeft = swingingBlock_.x;
	float newRight = newLeft + kBlockWidth;

	float prevLeft = blocks_.back().x;
	float prevRight = prevLeft + kBlockWidth;

	float overlapLeft = std::max(newLeft, prevLeft);
	float overlapRight = std::min(newRight, prevRight);
	fallOverlapWidth_ = std::max(0.0f, overlapRight - overlapLeft); // <=0 means the AABBs don't intersect at all

	bool collides = fallOverlapWidth_ > 0.0f;
	fallWasMiss_ = !collides; // literally nothing to land on

	if (kDebugDisableGameOver) {
		// Testing mode: force every drop to land on the stack, even a
		// true zero-overlap miss - see the comment on
		// kDebugDisableGameOver in GameScene.h.
		fallWasMiss_ = false;
	}

	fallVelocityY_ = 0.0f;

	if (fallWasMiss_) {
		// No collision at all - let it fall straight past the tower and
		// off the bottom of the (current camera-scrolled) screen.
		fallTargetY_ = kScreenHeight + cameraY_ + 200.0f;
	} else {
		// It collides with the block below - it rests exactly on top of
		// it. Whether that contact area is actually enough to hold its
		// weight (kUnstableOverlapRatio) is resolved once it lands,
		// separately from the raw fact that a collision happened.
		fallTargetY_ = TopBlockY() - kBlockHeight;
	}

	state_ = State::kFalling;
}

void GameScene::ResolveLanding() {
	swingingBlock_.y = fallTargetY_;

	if (fallWasMiss_) {
		state_ = State::kGameOver;
		if (background_) {
			background_->SetColor({0.25f, 0.05f, 0.08f, 1.0f});
		}
		return;
	}

	float prevLeft = blocks_.back().x;
	float prevCenter = prevLeft + kBlockWidth * 0.5f;
	float contactY = blocks_.back().y; // top edge of the block being landed on - the actual contact seam
	float newCenter = swingingBlock_.x + kBlockWidth * 0.5f;
	float signedOffset = newCenter - prevCenter;

	// Color check comes before anything else: no matter how well it
	// overlaps, a block whose colorIndex doesn't EXACTLY match the
	// current top of the stack's never actually lands - it bounces away
	// instead (see TriggerBounce()) and costs one life. Only reached when
	// there's a real collision to bounce off of (fallWasMiss_ already
	// returned above for a genuine miss).
	if (swingingBlock_.colorIndex != blocks_.back().colorIndex) {
		float bounceDirection = signedOffset >= 0.0f ? 1.0f : -1.0f;
		TriggerBounce(bounceDirection);
		return;
	}

	// Split this block's total weight (kBlockWeightUnits) between its
	// overlapping part (resting on the block below) and its hanging,
	// non-overlapping part, in proportion to the actual overlap - see the
	// kBlockWeightUnits comment in GameScene.h. fallOverlapWidth_ > 0 is
	// guaranteed here (fallWasMiss_ already returned above otherwise).
	float overlapRatio = std::clamp(fallOverlapWidth_ / kBlockWidth, 0.0f, 1.0f);
	float overlapUnits = overlapRatio * kBlockWeightUnits;
	float nonOverlapUnits = kBlockWeightUnits - overlapUnits; // this block's own "hanging" parts

	// The tactile part: kick the tower sideways in the direction of the
	// miss, proportional to how far off it was AND to how many of this
	// block's parts are hanging unsupported - a bad drop that also barely
	// overlaps shakes the tower much harder than the same pixel offset
	// would on a block that's still mostly seated.
	wobbleVelocity_ += signedOffset * kWobbleImpulsePerOffset * (1.0f + nonOverlapUnits * kWobblePartsMultiplier);

	// Every real landing gets a screen shake + debris burst, scaled by
	// how unstable this particular contact was (see TriggerImpact()).
	TriggerImpact(overlapRatio, newCenter, contactY);

	Block placed;
	placed.x = swingingBlock_.x;
	placed.y = fallTargetY_;
	placed.width = kBlockWidth;
	placed.height = kBlockHeight;
	placed.colorIndex = NextPlacedColorIndex(); // in-order cycle, same as UpdatePlacedBlockColors()
	placed.color = palette_[placed.colorIndex];
	placed.weight = overlapUnits; // this block's own "supported" weight - only meaningful once it survives the check just below
	placed.colorCycleTimer = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kColorCycleInterval;
	placed.animTimer = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kBlockAnimFrameInterval;
	ApplyBlockTexture(placed);
	blocks_.push_back(placed);

	// Scoring: 1 base point, plus a perfect-placement bonus for a
	// near-dead-center landing (kPerfectOverlapThreshold), plus a streak
	// bonus that grows (and caps) with consecutive successful placements
	// (kStreakBonusInterval/kMaxStreakBonus) - see TriggerBounce() for
	// where the streak resets.
	streak_ += 1;
	int perfectBonus = overlapRatio >= kPerfectOverlapThreshold ? kPerfectPlacementBonus : 0;
	int streakBonus = std::min((streak_ / kStreakBonusInterval) * kStreakBonusPerInterval, kMaxStreakBonus);
	score_ += kBaseScorePerBlock + perfectBonus + streakBonus;
	RebuildDigitSprites(score_, scoreDigitSprites_, kScoreRowX + static_cast<float>(kScoreLabelLength) * kGlyphSpacing, kScoreRowY);
	SaveHighScoreIfNeeded();

	// Every kLifeRecoveryStreak in a row, hand back a life - never above
	// kStartingLives, so this can offset an earlier bounce but can't bank
	// extra lives beyond the starting count.
	if (streak_ % kLifeRecoveryStreak == 0) {
		lives_ = std::min(lives_ + 1, kStartingLives);
	}

	// It collided and is now resting there, but a real collision doesn't
	// automatically mean a STABLE one. Whichever side - overlapping or
	// hanging - holds more of the block's weight units is this block's
	// own heavy side; if the hanging side reaches half or more of the
	// units (overlapRatio <= kUnstableOverlapRatio), it's too heavy to
	// support itself and the block falls right here, before it can even
	// factor into the tower's overall balance below.
	bool unstableContact = overlapUnits <= kUnstableOverlapRatio * kBlockWeightUnits;
	if (unstableContact && !kDebugDisableGameOver) {
		StartCollapse(signedOffset);
		return;
	}

	if (!kDebugDisableGameOver) {
		// The real balance check: does the tower's actual center of mass
		// still land somewhere over the base's own footprint? This is
		// checked fresh against current geometry every placement, not
		// accumulated over the run - a tower that's leaned the same way
		// for stories is fine as long as its weight is still coming down
		// on the base; one bad top-heavy drop can end it outright even
		// on an otherwise clean run.
		float baseLeft = blocks_.front().x;
		float baseRight = baseLeft + kBlockWidth;
		float baseCenterX = baseLeft + kBlockWidth * 0.5f;
		float comX = ComputeCenterOfMassX();

		bool comOutsideBase = comX < baseLeft - kComSupportMargin || comX > baseRight + kComSupportMargin;
		if (comOutsideBase) {
			StartCollapse(comX - baseCenterX);
			return;
		}
	}

	if (kEnableClearGoal && score_ >= kClearScore) {
		// Hit the goal without collapsing - end the run as a win rather
		// than spawning another block. main.cpp polls IsCleared() and
		// swaps to ClearScene once this flips.
		state_ = State::kCleared;
		if (background_) {
			background_->SetColor({0.08f, 0.35f, 0.18f, 1.0f});
		}
		return;
	}

	// Camera only starts scrolling once the stack would otherwise grow
	// past kTopBlockScreenTarget; before that it stays put (cameraY_ = 0)
	// so the base of the tower sits near the actual ground. The pivot
	// itself is always screen-fixed (see SpawnSwingingBlock), so this
	// target just needs to leave kAnchorClearanceBelow of room under it.
	float topY = TopBlockY();
	cameraY_ = std::min(0.0f, topY - kTopBlockScreenTarget);

	SpawnSwingingBlock();
}

void GameScene::StartCollapse(float toppleDirectionOffset) {
	if (blocks_.empty()) {
		state_ = State::kGameOver;
		return;
	}

	// The side it topples toward is simply whichever side is physically
	// heavier/further out right now - toppleDirectionOffset is either the
	// current drop's offset (a joint gave way locally) or the whole
	// tower's center-of-mass offset from the base (it tipped globally) -
	// either way, its sign is the real answer, not a guess.
	float direction = toppleDirectionOffset >= 0.0f ? 1.0f : -1.0f;
	float omega0 = direction * kCollapseSeedAngularVelocity;

	const Block& base = blocks_.front();
	float pivotX = base.x + kBlockWidth * 0.5f;
	float pivotY = base.y + kBlockHeight; // ground contact point, at the base's bottom edge

	for (auto& block : blocks_) {
		float centerX = block.x + kBlockWidth * 0.5f;
		float centerY = block.y + kBlockHeight * 0.5f;
		float relX = centerX - pivotX;
		float relY = centerY - pivotY;

		// Tangential velocity of a point rotating at omega0 around the
		// pivot (v = omega x r): farther from the pivot means faster,
		// with no extra scripting needed for that - see the comment on
		// kCollapseSeedAngularVelocity in GameScene.h.
		// NOTE: if blocks appear to fly the wrong way for your engine's
		// rotation convention (screen-space y-down vs. standard math
		// y-up can flip this), negate omega0 here.
		block.velocityX = -omega0 * relY;
		block.velocityY = omega0 * relX;
		block.angularVelocity = omega0;
		block.rotation = 0.0f;
	}

	collapseTimer_ = 0.0f;
	state_ = State::kCollapsing;
}

void GameScene::TriggerBounce(float directionAwayFromTower) {
	streak_ = 0; // a bounce always breaks the no-bounce streak (see kStreakBonusInterval/ResolveLanding())

	// Reuses the rigid-body fields Block already has for collapse physics
	// (velocityX/velocityY/rotation/angularVelocity) - swingingBlock_ is a
	// Block too, and those fields are otherwise unused while it's on the
	// string or falling normally.
	swingingBlock_.velocityX = directionAwayFromTower * kBounceHorizontalSpeed;
	swingingBlock_.velocityY = -kBounceUpSpeed; // negative = up
	swingingBlock_.rotation = 0.0f;
	swingingBlock_.angularVelocity = directionAwayFromTower * kBounceAngularVelocity;

	state_ = State::kBouncing;
}

void GameScene::SaveHighScoreIfNeeded() {
	if (score_ <= highScore_) {
		return;
	}
	highScore_ = score_;
	RebuildDigitSprites(highScore_, highScoreDigitSprites_, kHighScoreRowX + static_cast<float>(kHighScoreLabelLength) * kGlyphSpacing, kHighScoreRowY);

	std::ofstream highScoreFile(kHighScoreFilePath, std::ios::trunc);
	if (highScoreFile) {
		highScoreFile << highScore_;
	}
	// If the file couldn't be opened (read-only folder, missing
	// permissions, etc.), highScore_ still updates for THIS run/session -
	// it just won't persist to the next launch. Not worth failing the
	// placement over.
}

void GameScene::UpdateWind(float deltaTime) {
	// Advance (or finish) any gust already in progress.
	windGustTimer_ += deltaTime;
	if (windGustTimer_ < kWindGustDuration) {
		// Half-sine envelope: 0 at the start, peak at the middle, back to
		// 0 at the end - a smooth push-and-release rather than an abrupt
		// on/off force.
		float t = windGustTimer_ / kWindGustDuration;
		float envelope = std::sin(kPi * t);
		windOffsetX_ = windGustDirection_ * kWindGustMaxOffset * envelope;
	} else {
		windOffsetX_ = 0.0f;
	}

	if (isRestZoneActive_) {
		return; // no NEW gusts during a rest zone; let an active one above finish naturally
	}

	windTimer_ += deltaTime;
	if (windTimer_ < nextWindEventTime_) {
		return;
	}
	windTimer_ = 0.0f;
	windGustTimer_ = 0.0f;
	windGustDirection_ = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
	float span = kWindEventMaxInterval - kWindEventMinInterval;
	nextWindEventTime_ = kWindEventMinInterval + span * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
}

void GameScene::TriggerImpact(float overlapRatio, float impactCenterX, float impactY) {
	// 0 at a perfectly centered landing, up to (just under) 1 right at the
	// collapse threshold (kUnstableOverlapRatio) - the same instability
	// measure the tower-wide wobble kick uses (see ResolveLanding()), so
	// shake and debris both scale off the same "how shaky was this
	// landing" number.
	float instability = std::clamp((1.0f - overlapRatio) / (1.0f - kUnstableOverlapRatio), 0.0f, 1.0f);

	// --- screen shake ---
	shakeElapsed_ = 0.0f;
	shakeMagnitude_ = kShakeBaseMagnitude + kShakeMaxExtraMagnitude * instability;

	// --- debris burst ---
	// A neutral dust/concrete tint, distinct from the blocks' own palette
	// so debris always reads as "impact effect" rather than another block.
	Vector4 dustColor = {0.55f, 0.5f, 0.45f, 1.0f};
	int particleCount = kParticleBaseCount + static_cast<int>(kParticleMaxExtraCount * instability);

	for (int i = 0; i < particleCount; ++i) {
		float randomUnit = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // [0, 1]

		Particle particle;
		particle.x = impactCenterX + (randomUnit - 0.5f) * kBlockWidth;
		particle.y = impactY;
		particle.velocityX = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * kParticleHorizontalSpread;
		particle.velocityY = kParticleBaseSpeed + kParticleMaxExtraSpeed * instability * randomUnit;
		particle.life = kParticleLifetimeSeconds;
		particle.sprite = Sprite::Create(textureHandle_, {particle.x, particle.y - cameraY_}, dustColor);
		particle.sprite->SetSize({kParticleSize, kParticleSize});
		particles_.push_back(particle);
	}
}

void GameScene::UpdateCameraShake(float deltaTime) {
	shakeElapsed_ += deltaTime;
	if (shakeElapsed_ >= kShakeDuration) {
		shakeOffsetX_ = 0.0f;
		shakeOffsetY_ = 0.0f;
		return;
	}

	// Linear decay envelope, modulated by two different-frequency sine
	// waves so the jitter doesn't just move back and forth on one line.
	float decay = 1.0f - shakeElapsed_ / kShakeDuration;
	float envelope = shakeMagnitude_ * decay;
	shakeOffsetX_ = envelope * std::sin(shakeElapsed_ * kShakeFrequencyX);
	shakeOffsetY_ = envelope * std::cos(shakeElapsed_ * kShakeFrequencyY);
}

void GameScene::UpdateParticles(float deltaTime) {
	for (auto& particle : particles_) {
		particle.velocityY += kGravity * deltaTime; // same real gravity as everything else that falls here
		particle.x += particle.velocityX * deltaTime;
		particle.y += particle.velocityY * deltaTime;
		particle.life -= deltaTime;

		if (particle.sprite) {
			particle.sprite->SetPosition({particle.x + shakeOffsetX_, particle.y - cameraY_ + shakeOffsetY_});
		}
	}

	// Erase-remove idiom: delete each expired sprite before dropping its
	// Particle from the vector, so nothing leaks.
	particles_.erase(
	    std::remove_if(
	        particles_.begin(), particles_.end(),
	        [](Particle& particle) {
		        bool dead = particle.life <= 0.0f;
		        if (dead) {
			        delete particle.sprite;
			        particle.sprite = nullptr;
		        }
		        return dead;
	        }),
	    particles_.end());
}

int GameScene::NextPlacedColorIndex() {
	int index = nextPlacedColorIndex_;
	nextPlacedColorIndex_ = (nextPlacedColorIndex_ + 1) % kColorCycleCount;
	return index;
}

void GameScene::ApplyBlockTexture(Block& block) {
	uint32_t handle = blockTextureHandles_[block.colorIndex][block.animFrame];

	delete block.sprite; // safe no-op if this is the block's first sprite (nullptr)

	// White tint (no multiply-darkening) so the source image's own colors
	// show through untouched - block.color is kept around for the bounce
	// color-match check's bookkeeping, but no longer used as a render tint.
	block.sprite = Sprite::Create(handle, {block.x, block.y - cameraY_}, {1.0f, 1.0f, 1.0f, 1.0f});
	block.sprite->SetSize({block.width, block.height});
	block.sprite->SetRotation(block.rotation);
}

void GameScene::UpdateBlockAnimations(float deltaTime) {
	auto advance = [this, deltaTime](Block& block) {
		block.animTimer += deltaTime;
		if (block.animTimer < kBlockAnimFrameInterval) {
			return;
		}
		block.animTimer -= kBlockAnimFrameInterval; // keep any overshoot rather than resetting to 0, so the cadence doesn't drift
		block.animFrame = (block.animFrame + 1) % kBlockAnimFrameCount;
		ApplyBlockTexture(block);
	};

	for (auto& block : blocks_) {
		advance(block);
	}
	advance(swingingBlock_);
}

void GameScene::UpdatePlacedBlockColors(float deltaTime) {
	if (blocks_.empty()) {
		return;
	}

	// Only the current top of the stack cycles color - once a new block
	// lands on top of it, this one stops changing and keeps whatever
	// color it last landed on; the new top block (already given its own
	// random colorCycleTimer phase at placement - see ResolveLanding()/
	// Reset() - though the color it steps to is the in-order sequence
	// from NextPlacedColorIndex(), not random) takes over from here.
	Block& top = blocks_.back();
	top.colorCycleTimer += deltaTime;
	if (top.colorCycleTimer < kColorCycleInterval) {
		return;
	}
	top.colorCycleTimer -= kColorCycleInterval; // keep any overshoot rather than resetting to 0, so the cadence doesn't drift

	int colorIndex = NextPlacedColorIndex();
	top.colorIndex = colorIndex;
	top.color = palette_[colorIndex];
	ApplyBlockTexture(top);
}

void GameScene::LoadBackgroundTextures() {
	// Real artwork now (see the Resources/ folder these came with) -
	// each is exactly kScreenWidth x kScreenHeight, one per band, in
	// climb order: background1 (lowest) -> background4 (repeats forever
	// past that - see kBackgroundImageCount/SyncBackgroundTiles()).
	static const char* kBackgroundFiles[kBackgroundImageCount] = {
	    "./Resources/background/background1.png",
	    "./Resources/background/background2.png",
	    "./Resources/background/background3.png",
	    "./Resources/background/background4.png",
	};
	for (int i = 0; i < kBackgroundImageCount; ++i) {
		backgroundTextureHandles_[i] = TextureManager::Load(kBackgroundFiles[i]);
	}
}

void GameScene::SyncBackgroundTiles() {
	// lowIndex = index of the band whose bottom edge is nearest the
	// bottom of the screen right now; lowIndex+1 is the band directly
	// above it. Since every band is exactly kBackgroundBandHeight (one
	// screen height) tall, these 2 are always enough to fully cover the
	// visible screen with no gaps, at any cameraY_.
	float screenBottomWorldY = cameraY_ + kScreenHeight;
	int lowIndex = static_cast<int>(std::floor((kGroundY - screenBottomWorldY) / kBackgroundBandHeight));
	if (lowIndex < 0) {
		lowIndex = 0; // never show a band below the ground - band 0 covers the ground itself
	}

	int tileIndices[kBackgroundTileSlotCount] = {lowIndex, lowIndex + 1};
	for (int slot = 0; slot < kBackgroundTileSlotCount; ++slot) {
		int tileIndex = tileIndices[slot];
		int textureIndex = std::min(tileIndex, kBackgroundImageCount - 1); // background4 repeats for every band past it
		float tileWorldTop = kGroundY - static_cast<float>(tileIndex + 1) * kBackgroundBandHeight;
		float screenY = tileWorldTop - cameraY_;

		if (!backgroundTileSprites_[slot] || backgroundTileTextureIndex_[slot] != textureIndex) {
			delete backgroundTileSprites_[slot];
			backgroundTileSprites_[slot] = Sprite::Create(backgroundTextureHandles_[textureIndex], {0.0f, screenY}, {1.0f, 1.0f, 1.0f, 1.0f});
			backgroundTileSprites_[slot]->SetSize({kScreenWidth, kBackgroundBandHeight});
			backgroundTileTextureIndex_[slot] = textureIndex;
		} else {
			backgroundTileSprites_[slot]->SetPosition({0.0f, screenY});
		}
	}
}

void GameScene::SyncCollapseVisual() {
	// Unlike SyncSpritePositions(), block.x/y themselves have actually
	// moved during collapse (see the kCollapsing branch of Update()) -
	// this just reflects that real, independent motion onto each
	// block's sprite; there's no shared pivot/rotation math here anymore.
	for (auto& block : blocks_) {
		if (!block.sprite) {
			continue;
		}
		block.sprite->SetPosition({block.x, block.y - cameraY_});
		block.sprite->SetRotation(block.rotation);
	}
}

void GameScene::SyncSpritePositions() {
	// The base (index 0) never moves; each block above it swings a little
	// more than the one below (heightFactor), scaled by the current
	// spring wobble (see the damped oscillator integrated in Update() and
	// kicked in ResolveLanding()) - so a clean run stays nearly still and
	// a string of bad drops keeps it visibly, physically unsettled. On
	// top of that shared, tower-wide sway, each block ALSO gets its own
	// extra sway proportional to its own hanging parts (kBlockWeightUnits
	// - block.weight, see ResolveLanding()) - a block that barely
	// overlapped visibly shakes more than a solidly-seated one right next
	// to it, independent of how tall the tower is at that point. On top
	// of ALL of that, shakeOffsetX_/shakeOffsetY_ (see
	// UpdateCameraShake()) add a shared, brief screen-shake jitter to
	// every world-space sprite here - background_ deliberately excluded
	// since it's a single flat-colored full-screen rect, so shaking it
	// would just risk exposing its edges for no visible benefit.
	for (size_t i = 0; i < blocks_.size(); ++i) {
		Block& block = blocks_[i];
		if (!block.sprite) {
			continue;
		}

		float heightFactor = std::min(static_cast<float>(i), kSwayMaxHeightBlocks);
		float ownHangingUnits = kBlockWeightUnits - block.weight; // 0 for the base / a dead-center landing
		float ownPartsFactor = 1.0f + ownHangingUnits * kSwayPartsMultiplier;
		float sway = wobbleOffset_ * heightFactor * kSwayHeightScale * ownPartsFactor;
		sway = std::max(-kSwayMaxOffset, std::min(kSwayMaxOffset, sway));

		block.sprite->SetPosition({block.x + sway + shakeOffsetX_, block.y - cameraY_ + shakeOffsetY_});
	}

	if (swingingBlock_.sprite && (state_ == State::kSwinging || state_ == State::kFalling)) {
		swingingBlock_.sprite->SetPosition({swingingBlock_.x + shakeOffsetX_, swingingBlock_.y - cameraY_ + shakeOffsetY_});
	}

	if (state_ != State::kSwinging) {
		return; // no string to draw once it's been released and is falling
	}

	// The block's rope attaches at its top-center edge.
	float attachX = swingingBlock_.x + kBlockWidth * 0.5f;
	float attachY = swingingBlock_.y;

	if (anchorSprite_) {
		anchorSprite_->SetPosition({anchorX_ - kAnchorWidth * 0.5f + shakeOffsetX_, anchorY_ - cameraY_ - kAnchorHeight * 0.5f + shakeOffsetY_});
	}

	for (size_t i = 0; i < ropeSegments_.size(); ++i) {
		float t = ropeSegments_.size() > 1 ? static_cast<float>(i) / static_cast<float>(ropeSegments_.size() - 1) : 0.0f;
		float segX = anchorX_ + (attachX - anchorX_) * t;
		float segY = anchorY_ + (attachY - anchorY_) * t;
		if (ropeSegments_[i]) {
			ropeSegments_[i]->SetPosition({segX - kRopeSegmentSize * 0.5f + shakeOffsetX_, segY - cameraY_ - kRopeSegmentSize * 0.5f + shakeOffsetY_});
		}
	}
}

float GameScene::TopBlockY() const {
	if (blocks_.empty()) {
		return kGroundY;
	}
	return blocks_.back().y;
}

float GameScene::ComputeCenterOfMassX() const {
	if (blocks_.empty()) {
		return kScreenWidth * 0.5f;
	}
	// Seesaw-style weighted average: each block's center X contributes in
	// proportion to its own weight (see kBlockWeightUnits), not equally,
	// so a heavy block sitting off-center drags the tower's real balance
	// point toward itself far more than a light one at the same offset
	// would.
	float weightedSum = 0.0f;
	float totalWeight = 0.0f;
	for (const auto& block : blocks_) {
		float centerX = block.x + kBlockWidth * 0.5f;
		weightedSum += centerX * block.weight;
		totalWeight += block.weight;
	}
	if (totalWeight <= 0.0f) {
		return kScreenWidth * 0.5f; // shouldn't happen (every cycle entry is > 0), but stay safe
	}
	return weightedSum / totalWeight;
}

void GameScene::Update() {
	Input* input = Input::GetInstance();
	constexpr float kDeltaTime = 1.0f / 60.0f;

	if (state_ == State::kCleared) {
		return; // main.cpp is responsible for swapping to ClearScene
	}

	// Damped spring, always integrating: ResolveLanding() below is the
	// only thing that ever kicks wobbleVelocity_; every other frame just
	// lets it spring back toward zero and lose energy to damping.
	float springAccel = -kWobbleStiffness * wobbleOffset_ - kWobbleDamping * wobbleVelocity_;
	wobbleVelocity_ += springAccel * kDeltaTime;
	wobbleOffset_ += wobbleVelocity_ * kDeltaTime;

	// Screen shake and debris both run unconditionally too, same as the
	// wobble spring above - a shake from the last landing keeps decaying,
	// and debris keeps falling, even into kGameOver/kCollapsing.
	UpdateCameraShake(kDeltaTime);
	UpdateParticles(kDeltaTime);
	UpdatePlacedBlockColors(kDeltaTime);
	UpdateBlockAnimations(kDeltaTime);
	UpdateWind(kDeltaTime);
	anchorMoveTimer_ += kDeltaTime;
	SyncBackgroundTiles();

	if (state_ == State::kGameOver) {
		// NOTE: DIK_SPACE / DIK_RETURN come from <dinput.h>. If your
		// KamataEngine.h doesn't already pull that in transitively,
		// add `#include <dinput.h>` at the top of this file.
		if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
			Reset();
		}
		return;
	}

	if (state_ == State::kCollapsing) {
		collapseTimer_ += kDeltaTime;

		// Real, per-block physics: every block accelerates downward at
		// the same kGravity regardless of its size or height in the
		// tower (actual gravity doesn't care about weight), keeps
		// drifting sideways in a straight line at whatever velocity it
		// left the collapse with (see StartCollapse()), and keeps
		// spinning at its own angularVelocity - nothing re-chains it to
		// its former neighbors, so the tower visibly breaks apart as it
		// falls rather than moving as one shape.
		bool allBlocksGone = true;
		for (auto& block : blocks_) {
			block.velocityY += kGravity * kDeltaTime;
			block.x += block.velocityX * kDeltaTime;
			block.y += block.velocityY * kDeltaTime;
			block.rotation += block.angularVelocity * kDeltaTime;

			if (block.y - cameraY_ < kScreenHeight + kBlockHeight) {
				allBlocksGone = false;
			}
		}

		SyncCollapseVisual();

		if (allBlocksGone || collapseTimer_ >= kCollapseMaxDuration) {
			// Either every block has tumbled off the bottom of the
			// screen, or (safety net) the collapse has been running long
			// enough that something clearly isn't going anywhere -
			// either way, show the game-over screen now.
			state_ = State::kGameOver;
			if (background_) {
				background_->SetColor({0.25f, 0.05f, 0.08f, 1.0f});
			}
		}
		return;
	}

	if (state_ == State::kBouncing) {
		swingingBlock_.velocityY += kGravity * kDeltaTime; // same real gravity as everything else in this file
		swingingBlock_.x += swingingBlock_.velocityX * kDeltaTime;
		swingingBlock_.y += swingingBlock_.velocityY * kDeltaTime;
		swingingBlock_.rotation += swingingBlock_.angularVelocity * kDeltaTime;

		if (swingingBlock_.sprite) {
			swingingBlock_.sprite->SetPosition({swingingBlock_.x + shakeOffsetX_, swingingBlock_.y - cameraY_ + shakeOffsetY_});
			swingingBlock_.sprite->SetRotation(swingingBlock_.rotation);
		}

		bool offscreen = swingingBlock_.y - cameraY_ > kScreenHeight + kBounceOffscreenMargin || swingingBlock_.x < -kBounceOffscreenMargin || swingingBlock_.x > kScreenWidth + kBounceOffscreenMargin;
		if (offscreen) {
			// The bounced block is gone for good either way - it never
			// rejoins the tower. Losing a life just decides whether play
			// continues (see SpawnSwingingBlock()) or the run ends here.
			lives_ -= 1;
			if (lives_ <= 0) {
				// Out of lives - end the run the same way reaching
				// kClearScore does, not the kGameOver failure screen:
				// main.cpp hands off to ClearScene with whatever score
				// was reached.
				state_ = State::kCleared;
				if (background_) {
					background_->SetColor({0.08f, 0.35f, 0.18f, 1.0f});
				}
			} else {
				SpawnSwingingBlock();
			}
		}
		return;
	}

	if (state_ == State::kFalling) {
		fallVelocityY_ += kGravity * kDeltaTime;
		swingingBlock_.y += fallVelocityY_ * kDeltaTime;

		if (swingingBlock_.y >= fallTargetY_) {
			ResolveLanding(); // may respawn a new kSwinging block, or end the run
		}

		SyncSpritePositions();
		return;
	}

	swingTime_ += kDeltaTime;

	// Moving anchor (see kAnchorMoveAmplitude/kAnchorMoveFrequency):
	// suppressed during a rest zone so the pivot sits still for that one
	// block, same as wind (see UpdateWind()).
	anchorX_ = kAnchorScreenX;
	if (!isRestZoneActive_) {
		anchorX_ += kAnchorMoveAmplitude * std::sin(anchorMoveTimer_ * kAnchorMoveFrequency);
	}

	// Pendulum swing: angle measured from straight-down, oscillating
	// left/right. The block hangs ropeLength_ below the anchor at all
	// times, so it also rises and falls slightly at the extremes of the
	// swing - that arc is what should read as "on a string" rather than
	// "sliding along a rail". ropeLength_ itself grows with height (set
	// in SpawnSwingingBlock), which is the difficulty knob: a longer
	// rope swings a wider, harder-to-read arc and drops further.
	float angle = swingMaxAngle_ * std::sin(swingTime_ * swingSpeed_);
	float blockCenterX = anchorX_ + std::sin(angle) * ropeLength_;
	float attachY = anchorY_ + std::cos(angle) * ropeLength_;

	swingingBlock_.x = blockCenterX - kBlockWidth * 0.5f;
	swingingBlock_.y = attachY; // block's top edge is the rope's end

	// Wind gust (see UpdateWind()): an additional sideways push on top of
	// the pendulum's own position, only while still on the string -
	// released blocks (kFalling) keep whatever x they had at release, so
	// the collision math computed in BeginFall() never goes stale.
	if (!isRestZoneActive_) {
		swingingBlock_.x += windOffsetX_;
	}

	if (input->TriggerKey(DIK_SPACE)) {
		BeginFall(); // cuts the string; gravity takes over next frame
	}

	SyncSpritePositions();
}

void GameScene::Draw() {
	// NOTE: this Sprite::PreDraw/PostDraw pairing and GetCommandList()
	// name match the raw engine classes this course is built on. If your
	// packaged KamataEngine facade wraps sprite batching differently
	// (e.g. Sprite::PreDraw() takes no arguments, or common draw setup is
	// already handled for you before GameScene::Draw() is called), this
	// is the line to adjust - everything above it is engine-agnostic.
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (background_) {
		background_->Draw();
	}

	// Scrolling background tiles cover the whole screen during normal
	// play, but are deliberately skipped for kGameOver/kCollapsing/
	// kCleared so background_'s own red/green tint (set in
	// StartCollapse()/ResolveLanding()) is what actually shows through
	// as the win/lose feedback, rather than being hidden underneath them.
	if (state_ == State::kSwinging || state_ == State::kFalling || state_ == State::kBouncing) {
		for (auto* tile : backgroundTileSprites_) {
			if (tile) {
				tile->Draw();
			}
		}
	}

	for (auto& block : blocks_) {
		if (block.sprite) {
			block.sprite->Draw();
		}
	}

	if (state_ == State::kSwinging) {
		if (anchorSprite_) {
			anchorSprite_->Draw();
		}
		for (auto* segment : ropeSegments_) {
			if (segment) {
				segment->Draw();
			}
		}
	}
	if ((state_ == State::kSwinging || state_ == State::kFalling || state_ == State::kBouncing) && swingingBlock_.sprite) {
		swingingBlock_.sprite->Draw();
	}

	// Impact debris - drawn on top of the blocks/rope but under the score
	// UI, same layering as the swinging block above.
	for (auto& particle : particles_) {
		if (particle.sprite) {
			particle.sprite->Draw();
		}
	}

	DrawScore(); // UI - always visible, drawn last so nothing covers it
	DrawLives();

	Sprite::PostDraw();
}

void GameScene::DrawScore() {
	DrawGlyphRow(scoreLabelSprites_);
	DrawGlyphRow(scoreDigitSprites_);
	DrawGlyphRow(highScoreLabelSprites_);
	DrawGlyphRow(highScoreDigitSprites_);
}

void GameScene::DrawLives() {
	// Full color for slots still within lives_, dimmed/greyed for spent
	// ones - the icon count itself never changes (always kStartingLives
	// sprites), just the tint, so a lost-then-recovered life (see
	// kLifeRecoveryStreak) simply lights back up in place.
	Vector4 kLifeFullTint = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector4 kLifeSpentTint = {0.35f, 0.35f, 0.35f, 0.4f};

	for (size_t i = 0; i < lifeIconSegments_.size(); ++i) {
		Sprite* icon = lifeIconSegments_[i];
		if (!icon) {
			continue;
		}
		bool full = static_cast<int>(i) < lives_;
		icon->SetColor(full ? kLifeFullTint : kLifeSpentTint);
		icon->Draw();
	}
}