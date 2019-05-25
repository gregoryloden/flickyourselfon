#include "Util/PooledReferenceCounter.h"

class DynamicCameraAnchor;
class EntityState;
class MapState;
class PauseState;
class PlayerState;

#define newGameState() newWithoutArgs(GameState)

class GameState onlyInDebug(: public ObjCounter) {
private:
	static const int radioTowerInitialPauseAnimationTicks = 1000;
	static const int playerToRadioTowerAnimationTicks = 3000;
	static const int preRadioWavesAnimationTicks = 2000;
	static const int postRadioWavesAnimationTicks = 2000;
	static const int radioTowerToSwitchesAnimationTicks = 2000;
	static const int preSwitchesFadeInAnimationTicks = 1000;
	static const int postSwitchesFadeInAnimationTicks = 1000;
	static const int switchesToPlayerAnimationTicks = 2000;
	static const float squareSwitchesAnimationCenterWorldX;
	static const float squareSwitchesAnimationCenterWorldY;
public:
	static const char* savedGameFileName;
private:
	static const string sawIntroAnimationFilePrefix;

	bool sawIntroAnimation;
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
	~GameState();

	//return whether updates and renders should stop
	bool getShouldQuitGame() { return shouldQuitGame; }
	void updateWithPreviousGameState(GameState* prev, int ticksTime);
	void setPlayerCamera();
	void setDynamicCamera();
	void startRadioTowerAnimation(int ticksTime);
	void render(int ticksTime);
	void saveState();
	void loadInitialState();
};
