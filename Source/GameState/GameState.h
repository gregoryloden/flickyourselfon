#include "Util/PooledReferenceCounter.h"

class DynamicCameraAnchor;
class EntityState;
class MapState;
class PauseState;
class PlayerState;

#define newGameState() newWithoutArgs(GameState)

class GameState onlyInDebug(: public ObjCounter) {
private:
	enum class TitleAnimation: int {
		None,
		Intro,
		Outro
	};

	static const int introTitleFadeInTicksTime = 1000;
	static const int introTitleFadeInTicksDuration = 1000;
	static const int introTitleDisplayTicksTime = introTitleFadeInTicksTime + introTitleFadeInTicksDuration;
	static const int introTitleDisplayTicksDuration = 2000;
	static const int introTitleFadeOutTicksTime = introTitleDisplayTicksTime + introTitleDisplayTicksDuration;
	static const int introTitleFadeOutTicksDuration = 1000;
	static const int introPostTitleTicksTime = introTitleFadeOutTicksTime + introTitleFadeOutTicksDuration;
	static const int introPostTitleTicksDuration = 500;
	static const int introAnimationStartTicksTime = introPostTitleTicksTime + introPostTitleTicksDuration;
	static const int radioTowerInitialPauseAnimationTicks = 1500;
	static const int playerToRadioTowerAnimationTicks = 2000;
	static const int preRadioWavesAnimationTicks = 2000;
	static const int postRadioWavesAnimationTicks = 2000;
	static const int radioTowerToSwitchesAnimationTicks = 2000;
	static const int preSwitchesFadeInAnimationTicks = 1000;
	static const int postSwitchesFadeInAnimationTicks = 1000;
	static const int switchesToPlayerAnimationTicks = 2000;
	static const float squareSwitchesAnimationCenterWorldX;
	static const float squareSwitchesAnimationCenterWorldY;
	static const char* titleGameName;
	static const char* titleCreditsLine1;
	static const char* titleCreditsLine2;
	static const char* titlePostCreditsMessage;
public:
	static const char* savedGameFileName;
private:
	static const string sawIntroAnimationFilePrefix;

	bool sawIntroAnimation;
	TitleAnimation titleAnimation;
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
	void renderTitleAnimation(int gameTicksTime);
	void saveState();
	void loadInitialState(int ticksTime);
	void beginIntroAnimation(int ticksTime);
	void resetGame(int ticksTime);
};
