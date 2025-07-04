#include "GameState/EntityState.h"
#include "GameState/MapState/MapState.h"
#include "Util/Config.h"

#define newPlayerState(mapState) produceWithArgs(PlayerState, mapState)

class CollisionRect;
class Hint;
class KickAction;
enum class KickActionType: int;
class KickResetSwitchUndoState;
class SpriteAnimation;
class UndoState;
namespace EntityAnimationTypes {
	class Component;
}

class PlayerState: public EntityState {
public:
	enum class RideRailSpeed: int {
		Forward,
		FastForward,
		FastBackward,
	};

private:
	static constexpr float smallDistance = 1.0f / 256.0f;
	static constexpr float boundingBoxWidth = 11.0f;
	static constexpr float boundingBoxHeight = 5.0f;
	static constexpr float boundingBoxLeftOffset = boundingBoxWidth * -0.5f;
	static constexpr float boundingBoxRightOffset = boundingBoxWidth * 0.5f;
	static constexpr float boundingBoxCenterYOffset = 7.0f;
	static constexpr float boundingBoxTopOffset = boundingBoxCenterYOffset - boundingBoxHeight * 0.5f;
	static constexpr float boundingBoxBottomOffset = boundingBoxCenterYOffset + boundingBoxHeight * 0.5f;
public:
	static constexpr float introAnimationPlayerCenterX = 50.5f + (float)(MapState::firstLevelTileOffsetX * MapState::tileSize);
	static constexpr float introAnimationPlayerCenterY = 106.5f + (float)(MapState::firstLevelTileOffsetY * MapState::tileSize);
	static constexpr float baseSpeedPerTick = 40.0f / Config::ticksPerSecond;
private:
	//only kick something if you're less than this distance from it
	//visually, you have to be 1 pixel away or closer
	static constexpr float kickingDistanceLimit = 1.5f;
	static constexpr char* playerXFilePrefix = "playerX ";
	static constexpr char* playerYFilePrefix = "playerY ";
	static constexpr char* playerDirectionFilePrefix = "playerDirection ";
	static constexpr char* finishedMoveTutorialFileValue = "finishedMoveTutorial";
	static constexpr char* finishedKickTutorialFileValue = "finishedKickTutorial";
	static constexpr char* finishedUndoRedoTutorialFileValue = "finishedUndoRedoTutorial";
	static constexpr char* noClipFileValue = "noClip";

	static thread* hintSearchThread;
	static Hint* hintSearchStorage;

	char z;
	char xDirection;
	char yDirection;
	float lastXMovedDelta;
	float lastYMovedDelta;
	CollisionRect* collisionRect;
	SpriteAnimation* spriteAnimation;
	int spriteAnimationStartTicksTime;
	SpriteDirection spriteDirection;
	int lastStepSound;
	bool hasBoot;
	ReferenceCounterHolder<DynamicValue> ghostSpriteX;
	ReferenceCounterHolder<DynamicValue> ghostSpriteY;
	SpriteDirection ghostSpriteDirection;
	int ghostSpriteStartTicksTime;
	ReferenceCounterHolder<MapState> mapState;
	ReferenceCounterHolder<KickAction> availableKickAction;
	int autoKickStartTicksTime;
	bool canImmediatelyAutoKick;
	float lastControlledX;
	float lastControlledY;
	ReferenceCounterHolder<DynamicValue> worldGroundY;
	int worldGroundYStartTicksTime;
	float worldGroundYOffset;
	bool finishedMoveTutorial;
	bool finishedKickTutorial;
	bool undoRedoTutorialUnlocked;
	bool finishedUndoRedoTutorial;
	ReferenceCounterHolder<UndoState> undoState;
	ReferenceCounterHolder<UndoState> redoState;
	Hint* hint;
	ReferenceCounterHolder<DynamicValue> pauseX;
	ReferenceCounterHolder<DynamicValue> pauseY;
	SpriteDirection pauseSpriteDirection;
	char pauseZ;
	bool noClip;
	bool shouldEndGame;

public:
	PlayerState(objCounterParameters());
	virtual ~PlayerState();

	Hint* getHint() { return hint; }
	void obtainBoot() { hasBoot = true; }
private:
	bool showUndoRedoTutorial() { return !finishedUndoRedoTutorial && undoRedoTutorialUnlocked; }
public:
	bool shouldFreezePlayerForTutorial() { return showUndoRedoTutorial(); }
	bool getShouldEndGame() { return shouldEndGame; }
	//initialize and return a PlayerState
	static PlayerState* produce(objCounterParametersComma() MapState* mapState);
	//copy the state of the other PlayerState
	void copyPlayerState(PlayerState* other);
	//release a reference to this PlayerState and return it to the pool if applicable
	virtual void release();
protected:
	//release held states before this is returned to the pool
	virtual void prepareReturnToPool();
public:
	//get the world Y after translating to what the world-terrain-space-Y would be at height 0
	float getWorldGroundY(int ticksTime);
	//get the Z that the player appears at based on animation
	float getDynamicZ(int ticksTime);
	//get the X to zoom in on in the end-game outro animation
	float getEndGameBootX(int ticksTime);
	//get the Y to zoom in on in the end-game outro animation
	float getEndGameBootY(int ticksTime);
	//set the sprite direction, and also set x and y directions to match
	virtual void setDirection(SpriteDirection pSpriteDirection);
	//tell the game state to use the player as the next camera anchor, we will handle actually switching to a different camera
	//	somewhere else
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	//set the animation to the given animation at the given time
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	//set the position of the ghost sprite, or clear it
	virtual void setGhostSprite(bool show, float pX, float pY, SpriteDirection direction, int ticksTime);
	//tell the map to kick a switch
	virtual void mapKickSwitch(short switchId, bool moveRailsForward, bool allowRadioTowerAnimation, int ticksTime);
	//tell the map to kick a reset switch
	virtual void mapKickResetSwitch(short resetSwitchId, KickResetSwitchUndoState* kickResetSwitchUndoState, int ticksTime);
	//spawn a particle with the given SpriteAnimation
	virtual void spawnParticle(
		float pX, float pY, SpriteAnimation* pAnimation, SpriteDirection pDirection, int particleStartTicksTime);
	//generate a hint based on our current state, or use the given state if present
	virtual void generateHint(Hint* useHint, int ticksTime);
private:
	//return whether we have a kick action where we can show connections
	bool showTutorialConnectionsForKickAction();
public:
	//if we have a kick action matching the given type, write its railSwitchId out and return true, otherwise return false
	bool hasRailSwitchKickAction(KickActionType kickActionType, short* outRailSwitchId);
	//save the current state into the pause state
	void savePauseState();
	//move the player to the given level, or restore the state without a selected level
	void setLevelSelectState(int levelN);
	//update this PlayerState by reading from the previous state
	void updateWithPreviousPlayerState(PlayerState* prev, bool hasKeyboardControl, int ticksTime);
private:
	//update the position of this PlayerState by reading from the previous state
	void updatePositionWithPreviousPlayerState(PlayerState* prev, const Uint8* keyboardState, int ticksTime);
	//set the new value and update the left and right
	void setXAndUpdateCollisionRect(DynamicValue* newX);
	//set the new value and update the top and bottom
	void setYAndUpdateCollisionRect(DynamicValue* newY);
	//check if we moved onto map tiles of a different height, using the previous move direction to figure out where to look
	//if we did, move the player and don't render interpolated positions
	void collideWithEnvironmentWithPreviousPlayerState(PlayerState* prev);
	//check for collisions at the map tile, either because of a different height or because there's a new switch there
	void addMapCollisions(
		int mapX, int mapY, vector<ReferenceCounterHolder<CollisionRect>>& collidedRects, vector<short> seenSwitchIds);
	//returns the fraction of the update spent within the bounds of the given rect; since a collision only happens once the
	//	player is within the bounds in both directions, this is the min of the two collision durations
	float netCollisionDuration(CollisionRect* other);
	//returns the fraction of the update spent within the x bounds of the given rect, based on the velocity this frame
	float xCollisionDuration(CollisionRect* other);
	//returns the fraction of the update spent within the y bounds of the given rect, based on the velocity this frame
	float yCollisionDuration(CollisionRect* other);
	//update the sprite (and possibly the animation) of this PlayerState by reading from the previous state
	void updateSpriteWithPreviousPlayerState(
		PlayerState* prev, const Uint8* keyboardState, int ticksTime, bool restartSpriteAnimation);
	//set the kick action if one is available
	void setKickAction();
	//set a rail kick action if there is a raised rail we can ride across
	//returns whether it was set
	bool setRailKickAction(float xPosition, float yPosition);
	//set a switch kick action if there is a switch in front of the player
	//returns whether it was set
	bool setSwitchKickAction(float xPosition, float yPosition);
	//set a reset switch kick action if there is a reset switch in front of the player
	//returns whether it was set
	bool setResetSwitchKickAction(float xPosition, float yPosition);
	//set a climb kick action if we can climb
	//returns whether it was set
	bool setClimbKickAction(float xPosition, float yPosition);
	//set a fall kick action if we can fall
	//returns whether it was set
	bool setFallKickAction(float xPosition, float yPosition);
	//check if the player is able to move to the target position at the target height, or near it
	//if the center is valid but the edges are not, we can move the target position perpendicular to the given movement
	//	direction
	//returns whether we can move to outActualXPosition and outActualYPosition
	static bool checkCanMoveToPosition(
		float targetXPosition,
		float targetYPosition,
		char targetHeight,
		SpriteDirection moveDirection,
		float* outActualXPosition,
		float* outActualYPosition);
	//auto-climb, auto-fall, or auto-ride-rail if we can
	void tryAutoKick(PlayerState* prev, int ticksTime);
	//collect the completed hint if applicable
	void tryCollectCompletedHint(PlayerState* other, int ticksTime);
	//wait for the hint thread to finish, and clear it
	static void waitForHintThreadToFinish();
public:
	//if we don't have a kicking animation, start one
	//this should be called after the player has been updated
	void beginKicking(int ticksTime);
private:
	//begin a kicking animation without changing any state
	void kickAir(int ticksTime);
	//begin a kicking animation and climb up to the next tile in whatever direction we're facing
	void kickClimb(float currentX, float currentY, float targetX, float targetY, int ticksTime);
	//begin a kicking animation and fall in whatever direction we're facing
	void kickFall(float currentX, float currentY, float targetX, float targetY, char fallHeight, int ticksTime);
	//begin a rail-riding animation and follow it to its other end
	void kickRail(short railId, float xPosition, float yPosition, int ticksTime);
public:
	//add the components for a rail-riding animation
	static void addRailRideComponents(
		short railId,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
		float xPosition,
		float yPosition,
		RideRailSpeed rideRailSpeed,
		Hint* useHint,
		float* outFinalXPosition,
		float* outFinalYPosition,
		SpriteDirection* outFinalSpriteDirection);
private:
	//begin a kicking animation and set the switch to flip
	void kickSwitch(short switchId, int ticksTime);
public:
	//add the animation components for a switch kicking animation
	static void addKickSwitchComponents(
		short switchId,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
		bool moveRailsForward,
		bool allowRadioTowerAnimation,
		Hint* useHint);
private:
	//begin a kicking animation and set the reset switch to flip
	void kickResetSwitch(short resetSwitchId, int ticksTime);
public:
	//add the animation components for a reset switch kicking animation
	static void addKickResetSwitchComponents(
		short resetSwitchId,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
		KickResetSwitchUndoState* kickResetSwitchUndoState,
		Hint* useHint);
private:
	//set the undo/redo state to the given state, with special handling if we're deleting it
	static void setUndoState(ReferenceCounterHolder<UndoState>& holder, UndoState* newUndoState);
	//prepare for adding a new undo state; add a no-op undo state if there isn't one already, and clear the redo state
	void prepForNewUndoState();
public:
	//delete all undo and redo states
	void clearUndoRedoStates();
	//undo an action if there is one to undo
	void undo(int ticksTime);
	//redo an action if there is one to redo
	void redo(int ticksTime);
	//don't do anything, but queue a NoOpUndoState in the other undo state stack
	void undoNoOp(bool isUndo);
	//move the player to the given location, and queue a MoveUndoState in the other undo state stack
	//returns whether a move was scheduled or not
	bool undoMove(float fromX, float fromY, char fromHeight, Hint* fromHint, bool isUndo, int ticksTime);
	//travel across this rail, and queue a RideRailUndoState in the other undo state stack
	void undoRideRail(short railId, Hint* fromHint, bool isUndo, int ticksTime);
	//kick a switch, and queue a KickSwitchUndoState in the other undo state stack
	void undoKickSwitch(short switchId, SpriteDirection direction, Hint* fromHint, bool isUndo, int ticksTime);
	//restore rails to the positions they were before kicking this reset switch, and queue a KickResetSwitchUndoState in the
	//	other undo state stack
	void undoKickResetSwitch(
		short resetSwitchId,
		SpriteDirection direction,
		KickResetSwitchUndoState* kickResetSwitchUndoState,
		Hint* fromHint,
		bool isUndo,
		int ticksTime);
	//get the level that the player is currently on
	int getLevelN();
	//render this player state, which was deemed to be the last state to need rendering
	void render(EntityState* camera, int ticksTime);
	//render the kick action for this player state if one is available
	void renderKickAction(EntityState* camera, bool hasRailsToReset, int ticksTime);
	//render any applicable tutorials
	//returns whether a tutorial was rendered
	bool renderTutorials();
	//set this player state with the initial state for the home screen
	void setHomeScreenState();
	//save this player state to the file
	void saveState(ofstream& file);
	//try to load state from the line of the file, return whether state was loaded
	bool loadState(string& line);
	//set the initial z for the player after loading the position
	void setInitialZ();
	//reset any state on the player that shouldn't exist for the intro animation
	void reset();
	//move the player as high as possible so that all rails render under
	void setHighestZ();
};
