#pragma once

// KamataEngine pulls in <Windows.h> transitively, which (without this)
// defines min/max as macros. That silently mangles every std::min/std::max
// call in GameScene.cpp into invalid syntax - it's the cause of the
// "illegal token on right side of '::'" / "expected an identifier" errors.
// Must be defined before the first include of KamataEngine.h anywhere in
// a translation unit, so it lives here since GameScene.h is included first.
#define NOMINMAX

#include "KamataEngine.h"
#include <string>
#include <unordered_map>
#include <vector>

/// <summary>
/// Base tower-stacking prototype (Tower Bloxx-style core loop only).
///
/// Loop:
///   - A block swings left/right above the current top of the stack.
///   - Space locks it in place.
///   - How much it overlaps the block below decides whether it's kept.
///   - Every placement (even a "successful" one) can add lean if it's
///     off-center; lean accumulates across the whole tower and is what
///     eventually ends the run, not just a single bad drop.
///
/// No つなぐ-specific mechanics are in here yet on purpose - this is the
/// base loop we agreed to build first and layer the connect mechanics
/// onto afterward (skybridge / utility line / access path etc.).
/// </summary>
class GameScene {
public:
	~GameScene();

	void Initialize();
	void Update();
	void Draw();

	// main.cpp polls these once per frame to know when to tear this scene
	// down and hand off to ClearScene, and what score to carry over.
	bool IsCleared() const { return state_ == State::kCleared; }
	int GetScore() const { return score_; }

private:
	enum class State {
		kSwinging,   // hanging on the string, oscillating
		kFalling,    // released, dropping under gravity toward its landing spot
		kBouncing,   // color didn't match the block below - bounced away instead of landing, see TriggerBounce()
		kCollapsing, // the tower itself is toppling over - see StartCollapse()
		kGameOver,
		kCleared, // reached kClearScore (or ran out of lives - see kStartingLives) without collapsing - main.cpp owns leaving this state
	};

	struct Block {
		float x = 0.0f; // world-space left edge
		float y = 0.0f; // world-space top edge
		float width = 0.0f;
		float height = 0.0f;
		KamataEngine::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		KamataEngine::Sprite* sprite = nullptr;

		// Which of palette_'s first kColorCycleCount entries `color` above
		// currently is (0, 1, or 2) - kept alongside color itself so
		// ResolveLanding() can check for an EXACT color match against the
		// block below with a plain integer comparison, rather than
		// comparing Vector4 floats directly.
		int colorIndex = 0;

		// Relative mass, assigned from how much this block actually
		// overlapped the block below at the moment it landed (see
		// kBlockWeightUnits/ResolveLanding()). Only feeds
		// ComputeCenterOfMassX() below - a seesaw-style weighted balance
		// point instead of a plain average - and never touches collapse
		// physics or how the block looks, both of which stay exactly as
		// before (uniform gravity, same sprite/color regardless of
		// weight).
		float weight = 1.0f;

		// Per-block rigid-body state - all zero except during
		// State::kCollapsing (see StartCollapse()). Once the tower
		// starts to fall, each block becomes its own independent falling
		// body with its own velocity and spin, rather than the whole
		// stack moving as one rigid shape.
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float rotation = 0.0f;
		float angularVelocity = 0.0f;

		// Counts up toward kColorCycleInterval; once the CURRENT TOP block
		// of the stack crosses it (see UpdatePlacedBlockColors()), it
		// picks a new random color from palette_'s first kColorCycleCount
		// entries and this resets (keeping any overshoot, not snapping to
		// 0, so the cadence doesn't drift). Every block gets its own
		// random starting phase at spawn (see ResolveLanding()/Reset())
		// so whichever block becomes the new top doesn't wait a full
		// interval before its first change. Only ever advances for
		// whichever block is currently blocks_.back() - a block stops
		// changing the moment something lands on top of it, keeping
		// whatever color it last had. Purely cosmetic - never touches
		// block.weight or any balance/overlap math.
		float colorCycleTimer = 0.0f;

		// Which frame (0, 1, or 2) of blockTextureHandles_[colorIndex] is
		// currently showing - a purely cosmetic torch-flicker loop, never
		// touching weight/overlap/balance math or the colorIndex match
		// check in ResolveLanding(). Advances every kBlockAnimFrameInterval
		// seconds (see UpdateBlockAnimations()/ApplyBlockTexture()), with
		// its own random starting phase (animTimer) at spawn so every
		// block in the tower doesn't flicker in lockstep.
		int animFrame = 0;
		float animTimer = 0.0f;
	};

	// A single piece of impact debris, spawned by TriggerImpact() whenever
	// a block actually lands on the one below it (see ResolveLanding()).
	// Falls under the same real gravity as everything else in this file,
	// drifts sideways at whatever speed it was given at spawn, and is
	// removed once its life runs out - short-lived and purely cosmetic,
	// never touches game logic.
	struct Particle {
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float life = 0.0f; // seconds remaining; removed once <= 0
		KamataEngine::Sprite* sprite = nullptr;
	};

	// --- tunables -----------------------------------------------------

	// TEMPORARY debug switch: when true, NOTHING ends the run - not a
	// total miss (zero overlap with the block below), not unstable
	// contact, not toppling. Every drop is forced to land on top of the
	// stack regardless of overlap. Off by default now that the real
	// balance gameplay (see kComSupportMargin below) is what's wanted;
	// flip back to true if you need to test other systems in isolation.
	static constexpr bool kDebugDisableGameOver = false;

	// Resolution assumed 1280x720 (this course's usual default). Change
	// these four if your project uses a different window size.
	static constexpr float kScreenWidth = 1280.0f;
	static constexpr float kScreenHeight = 720.0f;
	static constexpr float kGroundY = 620.0f;

	// Real-world scale: every physically meaningful quantity below (block
	// size, gravity, rope length) is defined in real units - meters,
	// m/s^2 - first, then converted to pixels through this ONE constant.
	// Change this single number and the whole tower rescales together and
	// stays physically consistent; the old approach (every pixel value
	// hand-picked independently) could only ever coincidentally agree
	// with real-world proportions, not guarantee it.
	static constexpr float kPixelsPerMeter = 200.0f;

	// Used by UpdateWind()'s gust envelope (a half sine curve). Not
	// otherwise tied to real-world units like the constants above/below.
	static constexpr float kPi = 3.14159265358979323846f;

	// A block is sized like an actual concrete/foam building cube - 64cm
	// on each side, roughly what a small crane (or a person) could
	// plausibly pick up and set down one at a time, which is the physical
	// premise the whole game is built on. Square (width == height).
	static constexpr float kBlockWidthMeters = 0.64f;
	static constexpr float kBlockHeightMeters = 0.64f;
	static constexpr float kBlockWidth = kBlockWidthMeters * kPixelsPerMeter;   // 128px
	static constexpr float kBlockHeight = kBlockHeightMeters * kPixelsPerMeter; // 128px

	// A block's own weight is derived directly from how it landed, split
	// into this many discrete "parts" (matches the design intent: a
	// block's total weight of kBlockWeightUnits is divided between its
	// overlapping part - resting on the block below - and its hanging,
	// non-overlapping part). Whichever part holds more of the units is
	// this block's own heavier side. If the hanging side reaches half or
	// more of the units (overlapRatio <= kUnstableOverlapRatio), that
	// overhang is judged too heavy to support itself and the block falls
	// right there (see ResolveLanding()) - this is the actual instability
	// check, replacing the old flat overlap-ratio cutoff. A block that
	// DOES pass keeps its own overlapUnits (kBlockWeightUnits *
	// overlapRatio, so always > half) as block.weight, which then also
	// feeds ComputeCenterOfMassX()'s tower-wide balance: a block that
	// landed almost dead-center (overlapRatio near 1.0, full units)
	// anchors more firmly into the tower's overall balance point than one
	// that barely survived just over the half-line. Zero overlap is a
	// separate case entirely (no collision at all - see fallWasMiss_ -
	// it falls straight past instead of reaching this check).
	static constexpr float kBlockWeightUnits = 10.0f;
	static constexpr float kUnstableOverlapRatio = 0.5f;

	// The current top block of the stack keeps recoloring itself every
	// kColorCycleInterval seconds, stepping in order (0, 1, 2, 0, 1, 2...)
	// through the first kColorCycleCount entries of palette_ (see
	// NextPlacedColorIndex()/UpdatePlacedBlockColors()) - purely a visual
	// effect layered on top of the block's initial placement color, never
	// touching weight, overlap, or balance math. Once a new block lands
	// on top of it, it stops changing and keeps its last color; the new
	// top takes over. The still-swinging (not yet placed) block is
	// unaffected - it keeps picking a random color of the 3 each spawn
	// (see SpawnSwingingBlock()).
	static constexpr float kColorCycleInterval = 1.5f;
	static constexpr int kColorCycleCount = 3;

	// Each of the kColorCycleCount colors is drawn from a real 3-frame
	// sprite sheet (loaded once into blockTextureHandles_ in
	// Initialize()) rather than a tinted white1x1.png rectangle - see
	// ApplyBlockTexture()/UpdateBlockAnimations(). Every block (base,
	// placed, and the still-swinging one) loops its own 3 frames
	// continuously and independently, regardless of colorCycleTimer/
	// NextPlacedColorIndex() above, which only ever pick WHICH color's
	// frames to show, never the animation itself.
	static constexpr int kBlockAnimFrameCount = 3;
	static constexpr float kBlockAnimFrameInterval = 0.15f; // seconds per frame

	// The player starts with this many lives. Checked only when the
	// dropped block actually has something to collide with (a genuine
	// miss - fallWasMiss_, zero AABB overlap - is a separate, unrelated
	// failure mode and still ends the run immediately, same as before):
	// if the block's own colorIndex doesn't EXACTLY match the current top
	// of the stack's colorIndex at the moment of contact, it doesn't
	// land - it bounces away instead (see kBounce* below and
	// TriggerBounce()) and one life is spent. Reaching 0 lives ends the
	// run the same way reaching kClearScore does (state_ = kCleared) -
	// main.cpp hands off to ClearScene showing the score reached so far,
	// rather than the kGameOver/kCollapsing failure screen.
	static constexpr int kStartingLives = 3;

	// The bounce-away motion on a color mismatch: an instant upward +
	// sideways kick (away from the tower), then real kGravity takes over
	// exactly like every other falling body in this file. Purely a launch
	// velocity - once it's off any edge of the (margin-padded) screen,
	// the bounce is considered resolved (see kBouncing in Update()).
	static constexpr float kBounceUpSpeed = 520.0f;         // px/s, initial upward kick
	static constexpr float kBounceHorizontalSpeed = 260.0f; // px/s, sideways kick, away from the tower
	static constexpr float kBounceAngularVelocity = 4.0f;   // rad/s, purely cosmetic spin while it flies off
	static constexpr float kBounceOffscreenMargin = 200.0f; // px past any screen edge before the bounce counts as "gone"

	// This is the REAL stability rule, replacing a scripted "too many bad
	// drops" counter: the tower's center of mass (see
	// ComputeCenterOfMassX(), a seesaw-style weighted average - see
	// kBlockWeightUnits above) has to stay somewhere over the base's own
	// footprint, exactly like a real stack of blocks - lean ten stories
	// one way and it's fine as long as gravity's pull is still landing on
	// the base; drift the center of mass even slightly past the base's
	// edge and nothing holds it up anymore. kComSupportMargin is a tiny
	// float-precision buffer only (2cm at this scale) - not a gameplay
	// allowance, the physics doesn't get a discount.
	static constexpr float kComSupportMarginMeters = 0.02f;
	static constexpr float kComSupportMargin = kComSupportMarginMeters * kPixelsPerMeter; // 4px

	// Reaching this many *placed* blocks (score_ - the free base block
	// doesn't count, see ResolveLanding) would end the run as a win
	// instead of letting the player keep going forever - but that's
	// gated behind kEnableClearGoal below, which is currently off, so
	// this number does nothing right now.
	static constexpr int kClearScore = 15;

	// Master switch for the win condition above. false = the run only
	// ever ends by missing entirely or toppling (see kComSupportMargin) -
	// no block count ever triggers ClearScene, so a well-balanced tower
	// can keep growing and scoring indefinitely. Flip to true to bring
	// back the kClearScore win screen.
	static constexpr bool kEnableClearGoal = false;

	// --- wind gusts -----------------------------------------------------
	// A periodic random crosswind that nudges the still-swinging block
	// sideways (see UpdateWind()) - only while it's on the string, never
	// after release, so the falling/landing math (fallOverlapWidth_ etc.,
	// computed once at BeginFall()) stays exactly as consistent as
	// before. A gust ramps up and back down smoothly over its duration
	// (a half sine envelope) rather than snapping on/off. Suppressed
	// entirely during a rest zone (see kRestZoneInterval below).
	static constexpr float kWindEventMinInterval = 4.0f; // seconds, shortest gap between gusts
	static constexpr float kWindEventMaxInterval = 9.0f; // seconds, longest gap between gusts
	static constexpr float kWindGustDuration = 1.4f;     // seconds, one gust's full ramp-up-and-down
	static constexpr float kWindGustMaxOffset = 46.0f;   // px, peak sideways push at the middle of a gust

	// --- moving anchor ----------------------------------------------------
	// The crane pivot itself drifts slowly side to side (a sine wave, see
	// Update()'s kSwinging branch) instead of sitting dead still - the
	// player has to track a moving aim point, not just a swinging block.
	// Suppressed during a rest zone, same as wind.
	static constexpr float kAnchorMoveAmplitude = 60.0f; // px either side of kAnchorScreenX
	static constexpr float kAnchorMoveFrequency = 0.35f; // rad/s

	// --- perfect-placement bonus ------------------------------------------
	// A landing this well-centered (overlapRatio, see ResolveLanding())
	// earns bonus score on top of the usual kBaseScorePerBlock per block.
	static constexpr int kBaseScorePerBlock = 100;
	static constexpr float kPerfectOverlapThreshold = 0.95f;
	static constexpr int kPerfectPlacementBonus = 500;

	// --- no-bounce streak ---------------------------------------------------
	// Consecutive successful (non-bounced) placements build a streak (see
	// streak_/ResolveLanding()/TriggerBounce()) that both adds bonus score
	// and, every kLifeRecoveryStreak in a row, hands back a life (capped
	// at kStartingLives - this never lets the player bank MORE lives than
	// they started with).
	static constexpr int kStreakBonusInterval = 5;     // a bonus is earned every this many in a row
	static constexpr int kStreakBonusPerInterval = 50; // points per interval reached
	static constexpr int kMaxStreakBonus = 200;        // caps the bonus so it can't run away forever
	static constexpr int kLifeRecoveryStreak = 8;      // every this many in a row, +1 life (capped)

	// --- rest zones -----------------------------------------------------
	// Every kRestZoneInterval-th successful placement (by score_, the
	// free base block doesn't count), the NEXT block gets an easier swing
	// (see SpawnSwingingBlock()) and wind/anchor drift are suppressed for
	// it - a brief breather before the difficulty ramp resumes.
	static constexpr int kRestZoneInterval = 10;
	static constexpr float kRestZoneSwingSpeed = 1.2f;
	static constexpr float kRestZoneSwingAngle = 0.3f;

	// --- high score persistence ---------------------------------------------
	// Loaded once in Initialize(), and rewritten to disk (see
	// SaveHighScoreIfNeeded()) any time score_ beats it. Plain text, one
	// integer - no format worth a real save system for a single number.
	static constexpr const char* kHighScoreFilePath = "./highscore.txt";

	// The block hangs on a string from a fixed pivot above the stack and
	// swings like a pendulum, rather than sliding sideways at a constant
	// height - this is the actual crane-on-a-cable feel.
	//
	// The rope itself gets longer as the run goes on (see ropeLength_
	// below) - a longer string means a wider, harder-to-read swing arc
	// and a longer fall once released, which is the actual difficulty
	// knob the player asked for, distinct from swingSpeed_/swingMaxAngle_.
	// Lengths given in real meters (a 0.5-1.2m crane cable), converted
	// through kPixelsPerMeter like everything else physical here.
	static constexpr float kRopeLengthBaseMeters = 0.5f;
	static constexpr float kRopeLengthMaxMeters = 1.2f;
	static constexpr float kRopeLengthGrowthPerBlockMeters = 0.035f;
	static constexpr float kRopeLengthBase = kRopeLengthBaseMeters * kPixelsPerMeter;                     // 100px
	static constexpr float kRopeLengthMax = kRopeLengthMaxMeters * kPixelsPerMeter;                       // 240px
	static constexpr float kRopeLengthGrowthPerBlock = kRopeLengthGrowthPerBlockMeters * kPixelsPerMeter; // 7px
	static constexpr int kRopeSegmentCount = 7;
	static constexpr float kRopeSegmentSize = 8.0f;
	static constexpr float kAnchorWidth = 20.0f;
	static constexpr float kAnchorHeight = 12.0f;

	// The pivot is pinned to a fixed point always - horizontally at the
	// center of the screen, vertically near the top - and never follows
	// the stack. If the tower has drifted off-center, that's now the
	// player's problem to correct: the crane doesn't compensate for it,
	// so a leaning tower makes every subsequent drop measurably harder
	// to land, rather than the crane conveniently re-centering itself
	// over wherever the top block happens to be.
	static constexpr float kAnchorScreenX = kScreenWidth * 0.5f;
	static constexpr float kAnchorScreenY = 80.0f;

	// Vertical space the swing arc + hanging block need below the pivot.
	// Sized off kRopeLengthMax (the worst case) so clearance stays correct
	// even once the rope has grown all the way to its cap.
	static constexpr float kAnchorClearanceBelow = kRopeLengthMax + kBlockHeight + 30.0f;

	// Visual-only wobble, done the actual Tower Bloxx way: instead of an
	// ambient idle wave, every placement KICKS the tower sideways by an
	// amount proportional to how far off-center it landed AND how many of
	// its 10 weight units are hanging unsupported (see kBlockWeightUnits/
	// ResolveLanding()) - a block that barely overlapped shakes the whole
	// tower much harder than one that landed almost dead-center, even at
	// the same pixel offset. That kick springs back and damps out over
	// time (a simple damped harmonic oscillator) rather than just fading
	// a static amplitude. Play clean for a while and it settles down;
	// keep landing off-center and each new kick can land before the last
	// one has damped out, building up. Taller blocks move more than lower
	// ones (heightFactor); on top of that, each individual block's OWN
	// sway is further scaled by its own hanging-parts fraction (see
	// kSwayPartsMultiplier below) - a precariously-overlapped block
	// visibly shakes more than a solidly-seated one right next to it, on
	// top of everything above it inheriting the shared tower-wide sway.
	// Purely cosmetic: applied only at draw time, on top of block.x -
	// overlap/lean/balance math never sees any of this.
	static constexpr float kWobbleStiffness = 40.0f;        // how hard it springs back
	static constexpr float kWobbleDamping = 6.0f;           // how fast the wobble settles
	static constexpr float kWobbleImpulsePerOffset = 0.06f; // how hard a bad drop kicks it, per pixel of offset
	static constexpr float kWobblePartsMultiplier = 0.5f;   // extra kick per hanging weight-unit (parts), on top of the offset above
	static constexpr float kSwayHeightScale = 1.0f;         // master intensity knob, tune to taste
	static constexpr float kSwayMaxHeightBlocks = 25.0f;    // caps amplitude growth for very tall towers
	static constexpr float kSwayMaxOffset = 40.0f;          // hard safety clamp, px
	static constexpr float kSwayPartsMultiplier = 0.6f;     // extra per-block sway per hanging weight-unit (parts), on top of heightFactor

	// Screen shake, triggered by TriggerImpact() every time a block
	// actually lands on the one below it (a real collision, not a total
	// miss - see fallWasMiss_). Reuses the same instability measure as
	// the tower-wide wobble kick above (how much of the block's weight is
	// hanging unsupported, see kBlockWeightUnits/kUnstableOverlapRatio),
	// so a shaky, barely-stable landing rattles the screen much harder
	// than a clean, centered one - both effects grow together off the
	// same landing, rather than the shake being a separate, disconnected
	// system. Decays to zero linearly over kShakeDuration; the two
	// different frequencies for X and Y keep the jitter from moving in a
	// simple diagonal line. Purely cosmetic: applied only at draw time
	// (see shakeOffsetX_/shakeOffsetY_), never touches block.x/y or any
	// of the balance/overlap math.
	static constexpr float kShakeDuration = 0.3f;           // seconds for one shake event to fully decay
	static constexpr float kShakeFrequencyX = 45.0f;        // rad/s, how fast the X jitter oscillates
	static constexpr float kShakeFrequencyY = 61.0f;        // rad/s, deliberately different from X so it doesn't look like a straight line
	static constexpr float kShakeBaseMagnitude = 4.0f;      // px, shake size even on a perfectly centered landing
	static constexpr float kShakeMaxExtraMagnitude = 22.0f; // px, additional shake at max instability (right at the collapse threshold)

	// Small square "debris" sprites spawned at the contact seam by
	// TriggerImpact(), one burst per landing. Count and speed both scale
	// with the same instability measure as the shake above - a shaky
	// landing kicks up more, faster-flying debris than a clean one.
	// Falls under the real kGravity (see UpdateParticles()), just like
	// everything else that falls in this file.
	static constexpr int kParticleBaseCount = 4;              // spawned even on a perfectly centered landing
	static constexpr int kParticleMaxExtraCount = 8;          // additional pieces at max instability
	static constexpr float kParticleSize = 6.0f;              // px, small square sprite
	static constexpr float kParticleLifetimeSeconds = 0.5f;   // how long a piece lives before disappearing
	static constexpr float kParticleBaseSpeed = 40.0f;        // px/s downward, even at max instability = 0
	static constexpr float kParticleMaxExtraSpeed = 90.0f;    // px/s additional downward speed at max instability
	static constexpr float kParticleHorizontalSpread = 70.0f; // px/s, +/- random sideways speed range

	// The actual toppling-over collapse, once the run ends because the
	// tower itself got too unstable (unstable contact, or its center of
	// mass drifting off the base - see kComSupportMargin) - as opposed to
	// simply missing the stack entirely, which doesn't shake the existing
	// tower at all and still ends things instantly.
	//
	// For one instant, the tower is still a single rigid shape pivoting
	// around the base's ground contact point - that's how a real
	// building's collapse starts too - and that instant of rotation is
	// only used to hand each block a starting velocity (see
	// StartCollapse()). From the very next frame on, every block is its
	// OWN free-falling body: its own gravity, its own straight-line
	// drift, its own spin, no longer chained to its neighbors - so the
	// tower visibly breaks apart as it falls instead of swinging down as
	// one stiff plank. It uses the same kGravity as everything else that
	// falls in this file (real gravity doesn't care how much something
	// weighs), so there's deliberately no "heavier tower falls harder"
	// fudge factor here - a block higher up already starts out moving
	// faster than one near the pivot purely because it's farther from
	// the rotation axis, exactly like the top of a real toppling tower
	// whipping around faster than its base, with no extra scripting
	// needed to fake that.
	static constexpr float kCollapseSeedAngularVelocity = 2.2f; // rad/s - the initial "snap" of rotation that kicks everything loose
	static constexpr float kCollapseMaxDuration = 3.0f;         // seconds - safety cutoff in case a block ends up resting on-screen forever

	// The screen-space Y the top block is held at once the camera starts
	// scrolling. Must leave at least kAnchorClearanceBelow of room under
	// the (fixed) pivot, or the string/block would overlap or pass it -
	// that was the earlier "string gets pushed off the top" bug.
	static constexpr float kTopBlockScreenTarget = kAnchorScreenY + kAnchorClearanceBelow + 60.0f;

	// Scrolling background: 4 real images (1280x720 each, loaded in
	// LoadBackgroundTextures()), each covering kBackgroundBandHeight of
	// world height, stacked seamlessly from the ground upward.
	// background1/2/3 (index 0/1/2) each show once; background4 (index
	// 3) repeats forever past that, since the tower can grow
	// indefinitely (kEnableClearGoal is off by default). Only drawn
	// during normal play (kSwinging/kFalling/kBouncing) - background_
	// itself (the plain solid-color sprite) is what shows through for
	// the red/green kGameOver/kCleared tints, so the tiles deliberately
	// don't cover it during those states (see Draw()).
	static constexpr int kBackgroundImageCount = 4;
	static constexpr float kBackgroundBandHeight = kScreenHeight;
	// Exactly 2 tile sprites are ever needed at once, since each band is
	// exactly one screen height tall - see SyncBackgroundTiles().
	static constexpr int kBackgroundTileSlotCount = 2;

	// Once released, the block detaches from the string and drops under
	// gravity to wherever it's actually going to land (or past the bottom
	// of the screen, if it missed). This is real Earth gravity (9.8
	// m/s^2), converted through kPixelsPerMeter like everything else
	// physical in this file - the same constant used for the collapse
	// physics too, so a falling block always falls at the same real rate
	// no matter which system dropped it.
	static constexpr float kGravity = 9.8f * kPixelsPerMeter; // ~1960 px/s^2

	// Score/high-score display, now drawn with real character images
	// (loaded once into glyphTextureHandles_ - see LoadGlyphs()) instead
	// of the earlier sprite-based seven-segment digits. Fixed to the
	// screen (no camera offset), top-right corner: "SCORE" + its digits
	// on one row, "HIGH" + its digits directly below - both right-aligned
	// at the same screen edge (see kScoreRowX/kHighScoreRowX, which
	// differ since the two labels are different lengths).
	static constexpr int kScoreDigitCount = 5;    // "00000".."99999" - bumped from 4 now that scores run 100/500 per block instead of 1/2
	static constexpr float kGlyphSize = 28.0f;    // displayed at this size regardless of the source image's own resolution
	static constexpr float kGlyphSpacing = 32.0f; // distance between glyph origins, label letters and digits alike
	static constexpr float kScoreRowY = 20.0f;
	static constexpr float kHighScoreRowY = kScoreRowY + kGlyphSpacing + 8.0f; // directly below the live score row
	static constexpr int kScoreLabelLength = 5;                                // "SCORE"
	static constexpr int kHighScoreLabelLength = 4;                            // "HIGH"
	static constexpr float kScoreRowX = kScreenWidth - 24.0f - static_cast<float>(kScoreLabelLength + kScoreDigitCount) * kGlyphSpacing;
	static constexpr float kHighScoreRowX = kScreenWidth - 24.0f - static_cast<float>(kHighScoreLabelLength + kScoreDigitCount) * kGlyphSpacing;

	// Life icons - top-left, clear of the score/high score stack now on
	// the top-right (see kScoreRowX/kHighScoreRowX above). One sprite per
	// kStartingLives, built once in Initialize(); DrawLives() just tints
	// each one full or spent based on the current lives_ each frame (see
	// kLifeIconPath), rather than swapping textures - no separate "empty
	// heart" image needed unless you want one later.
	static constexpr const char* kLifeIconPath = "./Resources/life/life.png"; // placeholder - replace once you have the real icon
	static constexpr float kLifeIconSize = 28.0f;
	static constexpr float kLifeIconSpacing = 36.0f;
	static constexpr float kLifeIconX = 24.0f;
	static constexpr float kLifeIconY = 20.0f;

	// --- state ----------------------------------------------------------
	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* background_ = nullptr;

	// backgroundTextureHandles_[0..3] = background1..4 (see
	// kBackgroundImageCount). backgroundTileSprites_ are the 2 actual
	// on-screen sprites, repositioned every frame by
	// SyncBackgroundTiles(); backgroundTileTextureIndex_ tracks which
	// texture each slot currently shows, so the sprite is only recreated
	// (delete+Create) when that actually changes, not every frame.
	uint32_t backgroundTextureHandles_[kBackgroundImageCount] = {};
	KamataEngine::Sprite* backgroundTileSprites_[kBackgroundTileSlotCount] = {nullptr, nullptr};
	int backgroundTileTextureIndex_[kBackgroundTileSlotCount] = {-1, -1};

	KamataEngine::Sprite* anchorSprite_ = nullptr;    // fixed pivot the string hangs from
	std::vector<KamataEngine::Sprite*> ropeSegments_; // small beads tracing the string
	std::vector<KamataEngine::Vector4> palette_;

	// blockTextureHandles_[colorIndex][animFrame] - loaded once in
	// Initialize() from the 9 door/torch sprite images (3 colors x 3
	// frames each). See ApplyBlockTexture()/UpdateBlockAnimations().
	uint32_t blockTextureHandles_[kColorCycleCount][kBlockAnimFrameCount] = {};

	// One texture handle per character used on screen ('0'-'9', plus the
	// letters needed to spell SCORE and HIGH) - loaded once in
	// LoadGlyphs(). Shared by every label/digit sprite below; nothing
	// here is per-instance.
	std::unordered_map<char, uint32_t> glyphTextureHandles_;

	// "SCORE"/"HIGH" labels - built once in Initialize(), never rebuilt
	// (the text itself never changes). See BuildLabelSprites().
	std::vector<KamataEngine::Sprite*> scoreLabelSprites_;
	std::vector<KamataEngine::Sprite*> highScoreLabelSprites_;

	// Exactly kScoreDigitCount slots each, indexed by digit position (not
	// compacted) - a slot is nullptr when that digit is a blanked leading
	// zero (see RebuildDigitSprites()). Rebuilt only when score_/
	// highScore_ actually changes, not every frame - see ResolveLanding()/
	// SaveHighScoreIfNeeded().
	std::vector<KamataEngine::Sprite*> scoreDigitSprites_;
	std::vector<KamataEngine::Sprite*> highScoreDigitSprites_;

	// One sprite per kStartingLives, built once in Initialize() at fixed
	// screen positions (see kLifeIcon* above) - DrawLives() just tints
	// each one each frame based on the current lives_, never recreated.
	uint32_t lifeIconTextureHandle_ = 0;
	std::vector<KamataEngine::Sprite*> lifeIconSegments_;

	std::vector<Block> blocks_; // index 0 = base block
	Block swingingBlock_{};     // not yet placed
	State state_ = State::kSwinging;

	// Impact debris, spawned by TriggerImpact() and aged/removed in
	// UpdateParticles() every frame regardless of game state, so debris
	// from the last drop keeps falling even through kGameOver/kCollapsing.
	std::vector<Particle> particles_;

	// Camera-shake state, driven by TriggerImpact()/UpdateCameraShake().
	// shakeElapsed_ counts up from 0 each time a shake is triggered;
	// shakeOffsetX_/Y_ are the actual per-frame pixel offsets applied at
	// draw time (see SyncSpritePositions()) and are recomputed every
	// frame from shakeElapsed_/shakeMagnitude_, not integrated state.
	float shakeElapsed_ = kShakeDuration; // starts "already decayed" - no shake before the first landing
	float shakeMagnitude_ = 0.0f;
	float shakeOffsetX_ = 0.0f;
	float shakeOffsetY_ = 0.0f;

	float swingTime_ = 0.0f;
	float swingSpeed_ = 1.6f;
	float swingMaxAngle_ = 0.55f;        // radians either side of straight-down
	float ropeLength_ = kRopeLengthBase; // current string length; grows with height
	float anchorX_ = 0.0f;               // world-space pivot the string hangs from (fixed, screen-centered)
	float anchorY_ = 0.0f;

	// Moving-anchor phase (see kAnchorMoveAmplitude/kAnchorMoveFrequency) -
	// counts up every frame regardless of state; only actually applied to
	// anchorX_ while State::kSwinging (see Update()).
	float anchorMoveTimer_ = 0.0f;

	// Wind-gust state (see kWindEventMinInterval/kWindGustDuration/
	// UpdateWind()). windTimer_ counts up toward nextWindEventTime_ (a
	// freshly randomized gap each time); windGustTimer_ counts through
	// the current gust's duration once one starts; windOffsetX_ is the
	// actual per-frame sideways push, recomputed each frame from
	// windGustTimer_ (not integrated), and is only ever added to
	// swingingBlock_.x while State::kSwinging (see Update()).
	float windTimer_ = 0.0f;
	float nextWindEventTime_ = kWindEventMinInterval;
	float windGustTimer_ = kWindGustDuration; // starts "already finished" - no gust before the first trigger
	float windGustDirection_ = 1.0f;
	float windOffsetX_ = 0.0f;

	// True for the one block spawned right after every kRestZoneInterval-th
	// successful placement (see SpawnSwingingBlock()) - eases that one
	// block's swing and suppresses wind/anchor drift for it.
	bool isRestZoneActive_ = false;

	int score_ = 0;
	int lives_ = kStartingLives; // see kStartingLives/TriggerBounce()
	float cameraY_ = 0.0f;

	// Consecutive successful (non-bounced) placements - see
	// kStreakBonusInterval/kLifeRecoveryStreak/ResolveLanding(). Reset to
	// 0 by TriggerBounce() and at the start of every run (Reset()).
	int streak_ = 0;

	// Loaded once in Initialize() from kHighScoreFilePath; rewritten to
	// disk by SaveHighScoreIfNeeded() any time score_ beats it. Persists
	// across runs within the same process AND across launches (it's a
	// file on disk), unlike everything else here.
	int highScore_ = 0;

	// Next index NextPlacedColorIndex() will hand out, cycling through
	// 0..kColorCycleCount-1 in order. Shared across the base block
	// (Reset()), every newly placed block (ResolveLanding()), and the top
	// block's periodic recolor (UpdatePlacedBlockColors()), so placed
	// blocks always step through the same 0,1,2,0,1,2... sequence rather
	// than repeating or picking randomly.
	int nextPlacedColorIndex_ = 0;

	// Damped-spring state for the reactive wobble described above.
	float wobbleOffset_ = 0.0f;
	float wobbleVelocity_ = 0.0f;

	// Toppling-collapse state (State::kCollapsing). Per-block velocity/
	// rotation now lives on each Block (see the struct above); this is
	// just how long the collapse has been running, for the
	// kCollapseMaxDuration safety cutoff.
	float collapseTimer_ = 0.0f;

	// --- falling (post-release) state ------------------------------------
	float fallVelocityY_ = 0.0f;
	float fallTargetY_ = 0.0f;      // world-space Y the block stops at once it lands
	float fallOverlapWidth_ = 0.0f; // actual AABB overlap with the block below, computed at release; 0 = no collision at all
	bool fallWasMiss_ = false;      // true only when there's NO collision at all (nothing to land on)

	// --- helpers ----------------------------------------------------------
	void Reset();
	void SpawnSwingingBlock();
	void BeginFall();
	void ResolveLanding();
	void StartCollapse(float toppleDirectionOffset);
	void SyncSpritePositions();
	void SyncCollapseVisual();
	// Loads backgroundTextureHandles_[0..3] - called once from
	// Initialize(). See the kBackgroundImageCount comment in GameScene.h
	// for the current placeholder-color situation.
	void LoadBackgroundTextures();
	// Figures out which 2 background bands are visible for the current
	// cameraY_ and repositions/re-textures backgroundTileSprites_
	// accordingly; called once per frame from Update() (only while
	// actually playing - see the kBackgroundImageCount comment).
	void SyncBackgroundTiles();
	void DrawScore();
	// Loads glyphTextureHandles_ for every character used on screen
	// ('0'-'9','S','C','O','R','E','H','I','G') - called once from
	// Initialize().
	void LoadGlyphs();
	// Builds a fixed, never-rebuilt row of one sprite per character of
	// text, left to right from (originX, originY) - used for the
	// "SCORE"/"HIGH" labels.
	std::vector<KamataEngine::Sprite*> BuildLabelSprites(const std::string& text, float originX, float originY);
	// Deletes whatever's currently in sprites and rebuilds it as exactly
	// kScoreDigitCount slots for value, one glyph sprite per digit
	// (nullptr for a blanked leading zero) - called only when the
	// underlying value actually changes (see ResolveLanding()/
	// SaveHighScoreIfNeeded()), not every frame.
	void RebuildDigitSprites(int value, std::vector<KamataEngine::Sprite*>& sprites, float originX, float originY);
	// Draws every non-null sprite in the given row - shared by the label
	// and digit rows alike.
	void DrawGlyphRow(const std::vector<KamataEngine::Sprite*>& sprites) const;
	// Tints each of lifeIconSegments_ full-color or dimmed/grey based on
	// whether that slot is still within the current lives_ count; called
	// once per frame from Draw().
	void DrawLives();
	float TopBlockY() const;
	// Weighted average of every placed block's center X, weighted by each
	// block's mass (see kBlockWeightUnits) - the whole tower's actual
	// center of mass, seesaw-style, used to decide whether it's still
	// standing on its own base (see kComSupportMargin).
	float ComputeCenterOfMassX() const;

	// Called once from ResolveLanding() whenever a block actually lands
	// (not a total miss): kicks off the screen shake and spawns one burst
	// of debris particles at the contact point, both scaled by
	// overlapRatio (see the kShake*/kParticle* tunables above).
	void TriggerImpact(float overlapRatio, float impactCenterX, float impactY);
	// Recomputes shakeOffsetX_/shakeOffsetY_ for the current frame from
	// shakeElapsed_/shakeMagnitude_; called once per frame from Update().
	void UpdateCameraShake(float deltaTime);
	// Ages and moves every particle in particles_ under real gravity,
	// deleting the sprite and removing any whose life has run out; called
	// once per frame from Update().
	void UpdateParticles(float deltaTime);
	// Advances the current top block's colorCycleTimer and gives it a
	// fresh random color (see kColorCycleInterval/kColorCycleCount) once
	// that timer rolls over; called once per frame from Update(). Only
	// ever touches blocks_.back() - never swingingBlock_, and never any
	// block once something has landed on top of it.
	void UpdatePlacedBlockColors(float deltaTime);
	// Returns the next color index in the 0,1,2,0,1,2... cycle (see
	// nextPlacedColorIndex_) and advances it. Used for every placed
	// block's color - base block, newly landed blocks, and the top
	// block's periodic recolor - but NOT the still-swinging block, which
	// stays random (see SpawnSwingingBlock()).
	int NextPlacedColorIndex();
	// (Re)creates block.sprite using blockTextureHandles_[block.colorIndex]
	// [block.animFrame], at block.x/y/width/height/rotation - called
	// whenever a block's color or animation frame changes. Recreates
	// rather than swapping the texture on the existing sprite, since this
	// KamataEngine build's Sprite doesn't expose a way to change a
	// sprite's texture after Create() - if your Sprite class does have
	// one (e.g. SetTextureHandle()), swap it in here instead; it'd be
	// cheaper than deleting/recreating every frame tick.
	void ApplyBlockTexture(Block& block);
	// Advances every block's (base, placed, and the still-swinging one)
	// own animFrame/animTimer and calls ApplyBlockTexture() whenever a
	// frame rolls over; called once per frame from Update(), regardless
	// of game state.
	void UpdateBlockAnimations(float deltaTime);
	// Called from ResolveLanding() when swingingBlock_'s colorIndex
	// doesn't match the current top of the stack's: gives swingingBlock_
	// a bounce-away velocity (see kBounce* above) and switches to
	// State::kBouncing instead of placing it. Direction is away from
	// whichever side of the tower it landed on.
	void TriggerBounce(float directionAwayFromTower);
	// Advances windTimer_/windGustTimer_ and recomputes windOffsetX_ (see
	// the kWindEvent*/kWindGust* tunables); called once per frame from
	// Update(), regardless of state. Never starts a new gust while
	// isRestZoneActive_ is true, but lets an already-active one finish.
	void UpdateWind(float deltaTime);
	// Rewrites kHighScoreFilePath with score_ if it beats highScore_.
	// Called after every score_ change in ResolveLanding(); a no-op
	// otherwise (cheap enough not to bother debouncing further).
	void SaveHighScoreIfNeeded();
};