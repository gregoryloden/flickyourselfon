#include "MapState/MapState.h"

#define newGameState() newWithoutArgs(GameState)

class PauseState;
class PlayerState;
namespace EntityAnimationTypes {
	class Component;
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
	static const int radioTowerInitialPauseAnimationTicks = 1500;
	static const int playerToRadioTowerAnimationTicks = 2000;
	static const int preRadioWavesAnimationTicks = 2000;
	static const int postRadioWavesAnimationTicks = 2000;
	static const int radioTowerToSwitchesAnimationTicks = 2000;
	static const int preSwitchesFadeInAnimationTicks = 1000;
	static const int postSwitchesFadeInAnimationTicks = 1000;
	static const int switchesToPlayerAnimationTicks = 2000;
	//switches color activation positions
	static constexpr float squareSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 280;
	static constexpr float squareSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize + 80;
	static constexpr float triangleSwitchesAnimationCenterWorldX = MapState::firstLevelTileOffsetX * MapState::tileSize + 180;
	static constexpr float triangleSwitchesAnimationCenterWorldY = MapState::firstLevelTileOffsetY * MapState::tileSize - 103;
	static constexpr float sawSwitchesAnimationCenterWorldX = 100.0f; //todo: pick a position
	static constexpr float sawSwitchesAnimationCenterWorldY = 100.0f; //todo: pick a position
	static constexpr float sineSwitchesAnimationCenterWorldX = 100.0f; //todo: pick a position
	static constexpr float sineSwitchesAnimationCenterWorldY = 100.0f; //todo: pick a position
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
	static constexpr char* sawIntroAnimationFileValue = "sawIntroAnimation";

	static vector<string> saveFile;

	bool sawIntroAnimation;
	TextDisplayType textDisplayType;
	int titleAnimationStartTicksTime;
	ReferenceCounterHolder<PlayerState> playerState;
	ReferenceCounterHolder<MapState> mapState;
	ReferenceCounterHolder<DynamicCameraAnchor> dynamicCameraAnchor;
	EntityState* camera;
	ReferenceCounterHolder<PauseState> pauseState;
	int pauseStartTicksTime;
	int gameTimeOffsetTicksDuration;
	bool shouldQuitGame;

public:
	GameState(objCounterParameters());
	virtual ~GameState();

	//return whether updates and renders should stop
	bool getShouldQuitGame() { return shouldQuitGame; }
	//internally store the state of the save file
	static void cacheSaveFile();
	//update this GameState by reading from the previous state
	void updateWithPreviousGameState(GameState* prev, int ticksTime);
	//set our camera to our player
	void setPlayerCamera();
	//set our camera to the dynamic camera anchor
	void setDynamicCamera();
	//begin a radio tower animation for the various states
	void startRadioTowerAnimation(int ticksTime);
	//render this state, which was deemed to be the last state to need rendering
	void render(int ticksTime);
	//render the title animation at the given time
	void renderTextDisplay(int gameTicksTime);
	//save the state to a file
	void saveState();
	//initialize our state at the home screen (or load the save state in the editor)
	void loadInitialState(int ticksTime);
	//load the cached saved state from the save file
	void loadCachedSavedState(int ticksTime);
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
	//reset all state
	void resetGame(int ticksTime);
};
