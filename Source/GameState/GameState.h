#include "Util/PooledReferenceCounter.h"

#define newGameState() newWithoutArgs(GameState)

class DynamicCameraAnchor;
class EntityState;
class Holder_EntityAnimationComponentVector;
class MapState;
class PauseState;
class PlayerState;

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
	static const int introTitleFadeOutTicksDuration = 500;
	static const int introTitleDisplayTicksDuration = 2000;
	static const int introAnimationStartTicksTime =
		introTitleFadeInStartTicksTime
			+ introTitleFadeInTicksDuration
			+ introTitleDisplayTicksDuration
			+ introTitleFadeOutTicksDuration
			+ 500;
	static const int bootExplanationFadeInStartTicksTime = 10900;
	static const int bootExplanationFadeInTicksDuration = 500;
	static const int bootExplanationDisplayTicksDuration = 1000;
	static const int bootExplanationFadeOutTicksDuration = 500;
	static const int radioTowerExplanationFadeInStartTicksTime = 16000;
	static const int radioTowerExplanationFadeInTicksDuration = 500;
	static const int radioTowerExplanationDisplayTicksDuration = 3000;
	static const int radioTowerExplanationFadeOutTicksDuration = 500;
	static const int goalExplanationFadeInStartTicksTime = 23000;
	static const int goalExplanationFadeInTicksDuration = 500;
	static const int goalExplanationDisplayTicksDuration = 2000;
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
	static const float squareSwitchesAnimationCenterWorldX;
	static const float squareSwitchesAnimationCenterWorldY;
	static const float triangleSwitchesAnimationCenterWorldX;
	static const float triangleSwitchesAnimationCenterWorldY;
	static const float sawSwitchesAnimationCenterWorldX;
	static const float sawSwitchesAnimationCenterWorldY;
	static const float sineSwitchesAnimationCenterWorldX;
	static const float sineSwitchesAnimationCenterWorldY;
	//title screen and intro explanation text
	static const char* titleGameName;
	static const char* titleCreditsLine1;
	static const char* titleCreditsLine2;
	static const char* titlePostCreditsMessage;
	static const char* bootExplanationMessage1;
	static const char* bootExplanationMessage2;
	static const char* radioTowerExplanationMessageLine1;
	static const char* radioTowerExplanationMessageLine2;
	static const char* radioTowerExplanationMessageLine3;
	static const char* radioTowerExplanationMessageLine4;
	static const char* goalExplanationMessageLine1;
	static const char* goalExplanationMessageLine2;
	static const char* goalExplanationMessageLine3;
	//save file names and values
	#ifdef DEBUG
		static const char* replayFileName;
	#endif
public:
	static const char* savedGameFileName;
private:
	static const string sawIntroAnimationFilePrefix;

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
	void updateWithPreviousGameState(GameState* prev, int ticksTime);
	void setPlayerCamera();
	void setDynamicCamera();
	void startRadioTowerAnimation(int ticksTime);
	void render(int ticksTime);
	void renderTextDisplay(int gameTicksTime);
	void saveState();
	void loadInitialState(int ticksTime);
	#ifdef DEBUG
		bool loadReplay();
		void addMoveWithGhost(
			Holder_EntityAnimationComponentVector* replayComponentsHolder,
			float startX,
			float startY,
			float endX,
			float endY,
			int ticksDuration);
	#endif
	void beginIntroAnimation(int ticksTime);
	void resetGame(int ticksTime);
};
