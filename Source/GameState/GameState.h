#include "GameState/MapState/MapState.h"

#define newGameState() newWithoutArgs(GameState)

class PauseState;
class PlayerState;
enum class SpriteDirection : int;
namespace AudioTypes {
	class Music;
}

class GameState onlyInDebug(: public ObjCounter) {
private:
	enum class TextDisplayType: int {
		None,
		Intro,
		BootExplanation,
		RadioTowerExplanation,
		GoalExplanation,
		Outro
	};

	//title/text animation timing
	static const int introTitleFadeInStartTicksTime = 1000;
	static const int introTitleFadeInTicksDuration = 1000;
	static const int introTitleDisplayTicksDuration = 2000;
	static const int introTitleFadeOutTicksDuration = 500;
	static const int introAnimationStartTicksTime =
		introTitleFadeInStartTicksTime
			+ introTitleFadeInTicksDuration
			+ introTitleDisplayTicksDuration
			+ introTitleFadeOutTicksDuration
			+ 500;
	static const int bootExplanationFadeInStartTicksTime = introAnimationStartTicksTime + 5900;
	static const int bootExplanationFadeInTicksDuration = 500;
	static const int bootExplanationDisplayTicksDuration = 1000;
	static const int bootExplanationFadeOutTicksDuration = 500;
	static const int radioTowerExplanationFadeInStartTicksTime = introAnimationStartTicksTime + 11000;
	static const int radioTowerExplanationFadeInTicksDuration = 500;
	static const int radioTowerExplanationDisplayTicksDuration = 3500;
	static const int radioTowerExplanationFadeOutTicksDuration = 500;
	static const int goalExplanationFadeInStartTicksTime = introAnimationStartTicksTime + 18000;
	static const int goalExplanationFadeInTicksDuration = 500;
	static const int goalExplanationDisplayTicksDuration = 2500;
	static const int goalExplanationFadeOutTicksDuration = 500;
	//switches color activation timing
	static const int radioTowerMusicFadeOutMsDuration = 3000;
	static const int radioTowerInitialPauseAnimationTicks = 1500;
	static const int playerToRadioTowerAnimationTicks = 2000;
	static const int preRadioWavesAnimationTicks = 2000;
	static const int postRadioWavesAnimationTicks = 2000;
	static const int radioTowerToSwitchesAnimationTicks = 2000;
	static const int preSwitchesFadeInAnimationTicks = 1000;
	static const int postSwitchesFadeInAnimationTicks = 1000;
	static const int switchesToPlayerAnimationTicks = 2000;
	//save icon timing
	static const int saveIconShowDuration = 3000;
	//switches color activation positions
	static constexpr float squareSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 280;
	static constexpr float squareSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize + 80;
	static constexpr float triangleSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 180;
	static constexpr float triangleSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize - 103;
	static constexpr float sawSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 780;
	static constexpr float sawSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize - 150;
	static constexpr float sineSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 524;
	static constexpr float sineSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize + 130;
	//title screen and intro explanation text
public:
	static constexpr char* titleGameName = "Kick Yourself On";
private:
	static constexpr char* titleCreditsLine1 = "A game by";
	static constexpr char* titleCreditsLine2 = "Gregory Loden";
	static constexpr char* titlePostCreditsMessage = "Thanks for playing!";
	static constexpr char* bootExplanationMessage1 = "You are";
	static constexpr char* bootExplanationMessage2 = "a boot.";
	static constexpr char* radioTowerExplanationMessageLine1 = "Your local radio tower";
	static constexpr char* radioTowerExplanationMessageLine2 = "lost connection";
	static constexpr char* radioTowerExplanationMessageLine3 = "with its";
	static constexpr char* radioTowerExplanationMessageLine4 = "master transmitter relay.";
	static constexpr char* goalExplanationMessageLine1 = "Can you";
	static constexpr char* goalExplanationMessageLine2 = "guide this person";
	static constexpr char* goalExplanationMessageLine3 = "to turn it on?";
	//save file names and values
	#ifdef DEBUG
		static constexpr char* replayFileName = "kyo_replay.log";
	#endif
	static constexpr char* savedGameFileName = "kyo.sav";
	static constexpr char* levelsUnlockedFilePrefix = "levelsUnlocked ";
	static constexpr char* perpetualHintsFileValue = "perpetualHints";
	//render constants
	static const int saveIconEdgeSpacing = 10;
	static constexpr float saveIconMaxAlpha = 7.0f / 8.0f;

	int levelsUnlocked;
	bool perpetualHints;
	TextDisplayType textDisplayType;
	int titleAnimationStartTicksTime;
	ReferenceCounterHolder<PlayerState> playerState;
	ReferenceCounterHolder<MapState> mapState;
	ReferenceCounterHolder<DynamicCameraAnchor> dynamicCameraAnchor;
	EntityState* camera;
	int lastSaveTicksTime;
	ReferenceCounterHolder<PauseState> pauseState;
	int pauseStartTicksTime;
	int gameTimeOffsetTicksDuration;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	virtual ~GameState();

	//return whether updates and renders should stop
	bool getShouldQuitGame() { return shouldQuitGame; }
	//update this GameState by reading from the previous state
	void updateWithPreviousGameState(GameState* prev, int ticksTime);
	//set our camera to our player
	void setPlayerCamera();
	//set our camera to the dynamic camera anchor
	void setDynamicCamera();
private:
	//begin a radio tower animation for the various states
	void startRadioTowerAnimation(int ticksTime);
	//get the music for the given switch color
	AudioTypes::Music* getMusic(int lastActivatedSwitchColor);
	//start the music as indicated by the map state
	void startMusic();
public:
	//render this state, which was deemed to be the last state to need rendering
	void render(int ticksTime);
private:
	//render the title animation at the given time
	void renderTextDisplay(int gameTicksTime);
	//render the save icon if applicable
	void renderSaveIcon(int gameTicksTime);
	//save the state to a file
	void saveState(int gameTicksTime);
public:
	//initialize our state at the home screen (or load the save state in the editor)
	void loadInitialState(int ticksTime);
private:
	//load the state from the save file
	void loadSaveFile();
	#ifdef DEBUG
		//load a replay and finish initialization if a replay is available, return whether it was
		bool loadReplay();
		//move from one position to another, showing a ghost sprite at the end position
		void addMoveWithGhost(
			vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* replayComponents,
			float startX,
			float startY,
			float endX,
			float endY,
			int ticksDuration);
	#endif
	//give the camera and player their intro animations
	void beginIntroAnimation(int ticksTime);
	//add components to make the player start or continue walking, including sound effects
	//returns the length of this walking animation so far, which is equal to the given duration plus the previous duration
	int introAnimationWalk(
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>& playerAnimationComponents,
		SpriteDirection spriteDirection,
		int duration,
		int previousDuration);
	//reset all state
	void resetGame(int ticksTime);
};
