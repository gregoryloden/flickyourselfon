#include "GameState/EntityState.h"

#define newPlayerState(mapState) produceWithArgs(PlayerState, mapState)

class CollisionRect;
class EntityAnimation;
class KickAction;
class MapState;
class PositionState;
class Rail;
class SpriteAnimation;
namespace EntityAnimationTypes {
	class Component;
}

class PlayerState: public EntityState {
private:
	static const int railToRailTicksDuration = 80;
	static const int autoKickTriggerDelay = 400;
	static const float playerWidth;
	static const float playerHeight;
	static const float boundingBoxLeftOffset;
	static const float boundingBoxRightOffset;
	static const float boundingBoxTopOffset;
	static const float boundingBoxBottomOffset;
	static const float boundingBoxCenterYOffset;
public:
	static const float introAnimationPlayerCenterX;
	static const float introAnimationPlayerCenterY;
	static const float speedPerSecond;
	static const float diagonalSpeedPerSecond;
private:
	//only kick something if you're less than this distance from it
	//visually, you have to be 1 pixel away or closer
	static const float kickingDistanceLimit;
	static const string playerXFilePrefix;
	static const string playerYFilePrefix;

	char z;
	char xDirection;
	char yDirection;
	float lastXMovedDelta;
	float lastYMovedDelta;
	CollisionRect* collisionRect;
	SpriteAnimation* spriteAnimation;
	int spriteAnimationStartTicksTime;
	SpriteDirection spriteDirection;
	bool hasBoot;
	ReferenceCounterHolder<DynamicValue> ghostSpriteX;
	ReferenceCounterHolder<DynamicValue> ghostSpriteY;
	int ghostSpriteStartTicksTime;
	ReferenceCounterHolder<MapState> mapState;
	ReferenceCounterHolder<KickAction> availableKickAction;
	int autoKickStartTicksTime;
	bool canImmediatelyAutoKick;
	float lastControlledX;
	float lastControlledY;

public:
	PlayerState(objCounterParameters());
	virtual ~PlayerState();

	char getZ() { return z; }
	void obtainBoot() { hasBoot = true; }
	//initialize and return a PlayerState
	static PlayerState* produce(objCounterParametersComma() MapState* mapState);
	//copy the state of the other PlayerState
	void copyPlayerState(PlayerState* other);
	//release a reference to this PlayerState and return it to the pool if applicable
	virtual void release();
	//set the sprite direction, and also set x and y directions to match
	virtual void setDirection(SpriteDirection pSpriteDirection);
	//tell the game state to use the player as the next camera anchor, we will handle actually switching to a different camera
	//	somewhere else
	virtual void setNextCamera(GameState* nextGameState, int ticksTime);
	//set the animation to the given animation at the given time
	virtual void setSpriteAnimation(SpriteAnimation* pSpriteAnimation, int pSpriteAnimationStartTicksTime);
	//set the position of the ghost sprite, or clear it
	virtual void setGhostSprite(bool show, float x, float y, int ticksTime);
	//tell the map to kick a switch
	virtual void mapKickSwitch(short switchId, bool allowRadioTowerAnimation, int ticksTime);
	//tell the map to kick a reset switch
	virtual void mapKickResetSwitch(short resetSwitchId, int ticksTime);
	//return whether we have a kick action where we can show connections
	bool showTutorialConnectionsForKickAction();
	//if we have a reset switch kick action, return its id
	short getKickActionResetSwitchId();
	//update this PlayerState by reading from the previous state
	void updateWithPreviousPlayerState(PlayerState* prev, int ticksTime);
private:
	//update the position of this PlayerState by reading from the previous state
	void updatePositionWithPreviousPlayerState(PlayerState* prev, int ticksTime);
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
	void updateSpriteWithPreviousPlayerState(PlayerState* prev, int ticksTime, bool usePreviousStateSpriteAnimation);
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
	//auto-climb, auto-fall, or auto-ride-rail if we can
	void tryAutoKick(PlayerState* prev, int ticksTime);
public:
	//if we don't have a kicking animation, start one
	//this should be called after the player has been updated
	void beginKicking(int ticksTime);
private:
	//begin a kicking animation without changing any state
	void kickAir(int ticksTime);
	//begin a kicking animation and climb up to the next tile in whatever direction we're facing
	void kickClimb(float xMoveDistance, float yMoveDistance, int ticksTime);
	//begin a kicking animation and fall in whatever direction we're facing
	void kickFall(float xMoveDistance, float yMoveDistance, char fallHeight, int ticksTime);
	//begin a rail-riding animation and follow it to its other end
	void kickRail(short railId, float xPosition, float yPosition, int ticksTime);
public:
	//add the components for a rail-riding animation
	static void addRailRideComponents(
		short railId,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
		float xPosition,
		float yPosition,
		float* outFinalXPosition,
		float* outFinalYPosition);
private:
	//begin a kicking animation and set the switch to flip
	void kickSwitch(short switchId, int ticksTime);
public:
	//add the animation components for a switch kicking animation
	static void addKickSwitchComponents(
		short switchId,
		vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components,
		bool allowRadioTowerAnimation);
private:
	//begin a kicking animation and set the reset switch to flip
	void kickResetSwitch(short resetSwitchId, int ticksTime);
public:
	//add the animation components for a reset switch kicking animation
	static void addKickResetSwitchComponents(
		short resetSwitchId, vector<ReferenceCounterHolder<EntityAnimationTypes::Component>>* components);
	//render this player state, which was deemed to be the last state to need rendering
	void render(EntityState* camera, int ticksTime);
	//render the kick action for this player state if one is available
	void renderKickAction(EntityState* camera, bool hasRailsToReset, int ticksTime);
	//save this player state to the file
	void saveState(ofstream& file);
	//try to load state from the line of the file, return whether state was loaded
	bool loadState(string& line);
	//set the initial z for the player after loading the position
	void setInitialZ();
	//reset any state on the player that shouldn't exist for the intro animation
	void reset();
	#ifdef DEBUG
		//move the player as high as possible so that all rails render under
		void setHighestZ();
	#endif
};
