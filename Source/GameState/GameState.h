#include "Util/PooledReferenceCounter.h"

class DynamicCameraAnchor;
class EntityState;
class MapState;
class PauseState;
class PlayerState;

#define newGameState() newWithoutArgs(GameState)

class GameState onlyInDebug(: public ObjCounter) {
private:
	static const char* savedGameFileName;
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
	void render(int ticksTime);
	void saveState();
	void loadInitialState();
};
