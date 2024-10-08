#include "Util/PooledReferenceCounter.h"

#define stackNewNoOpUndoState(stack) produceWithArgs(NoOpUndoState, stack)
#define stackNewMoveUndoState(stack, fromX, fromY) produceWithArgs(MoveUndoState, stack, fromX, fromY)
#define stackNewClimbFallUndoState(stack, fromX, fromY, fromHeight, fromHint) \
	produceWithArgs(ClimbFallUndoState, stack, fromX, fromY, fromHeight, fromHint)
#define stackNewRideRailUndoState(stack, railId, fromHint) produceWithArgs(RideRailUndoState, stack, railId, fromHint)
#define stackNewKickSwitchUndoState(stack, switchId, direction, fromHint) \
	produceWithArgs(KickSwitchUndoState, stack, switchId, direction, fromHint)
#define stackNewKickResetSwitchUndoState(stack, resetSwitchId, direction, fromHint) \
	produceWithArgs(KickResetSwitchUndoState, stack, resetSwitchId, direction, fromHint)

class Hint;
class PlayerState;
enum class SpriteDirection: int;

class UndoState: public PooledReferenceCounter {
public:
	ReferenceCounterHolder<UndoState> next;

	UndoState(objCounterParameters());
	virtual ~UndoState();

protected:
	//release the next state before this is returned to the pool
	virtual void prepareReturnToPool();
	//get the next class type identifier for a subclass
	static int getNextClassTypeIdentifier();
public:
	//get the type identifier for the implementing subclass
	virtual int getTypeIdentifier() = 0;
	//apply the effect of this state as an undo or a redo
	//returns false if we should process the next UndoState after this one, or true if we're done
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime) = 0;
};
class NoOpUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

	NoOpUndoState(objCounterParameters());
	virtual ~NoOpUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a NoOpUndoState
	static NoOpUndoState* produce(objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack);
	//release a reference to this NoOpUndoState and return it to the pool if applicable
	virtual void release();
	//do nothing, but have the PlayerState queue a state in the appropriate other undo state stack
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
class MoveUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

private:
	float fromX;
	float fromY;

public:
	MoveUndoState(objCounterParameters());
	virtual ~MoveUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a MoveUndoState
	static MoveUndoState* produce(
		objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, float pFromX, float pFromY);
	//release a reference to this MoveUndoState and return it to the pool if applicable
	virtual void release();
	//move the player to the stored position
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
class ClimbFallUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

private:
	float fromX;
	float fromY;
	char fromHeight;
	Hint* fromHint;

public:
	ClimbFallUndoState(objCounterParameters());
	virtual ~ClimbFallUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a ClimbFallUndoState
	static ClimbFallUndoState* produce(
		objCounterParametersComma()
		ReferenceCounterHolder<UndoState>& stack,
		float pFromX,
		float pFromY,
		char pFromHeight,
		Hint* pFromHint);
	//release a reference to this ClimbFallUndoState and return it to the pool if applicable
	virtual void release();
	//move the player to the stored position and height
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
class RideRailUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

private:
	short railId;
	Hint* fromHint;

public:
	RideRailUndoState(objCounterParameters());
	virtual ~RideRailUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a RideRailUndoState
	static RideRailUndoState* produce(
		objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, short pRailId, Hint* pFromHint);
	//release a reference to this RideRailUndoState and return it to the pool if applicable
	virtual void release();
	//send the player across a rail
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
class KickSwitchUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

private:
	short switchId;
	SpriteDirection direction;
	Hint* fromHint;

public:
	KickSwitchUndoState(objCounterParameters());
	virtual ~KickSwitchUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a KickSwitchUndoState
	static KickSwitchUndoState* produce(
		objCounterParametersComma()
		ReferenceCounterHolder<UndoState>& stack,
		short pSwitchId,
		SpriteDirection pDirection,
		Hint* pFromHint);
	//release a reference to this KickSwitchUndoState and return it to the pool if applicable
	virtual void release();
	//kick the switch
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
class KickResetSwitchUndoState: public UndoState {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class RailUndoState {
	public:
		const short railId;
		const char fromTargetTileOffset;
		const char fromMovementDirection;

		RailUndoState(short pRailId, char pFromTargetTileOffset, char pFromMovementDirection);
		virtual ~RailUndoState();
	};

	static const int classTypeIdentifier;

private:
	short resetSwitchId;
	SpriteDirection direction;
	vector<RailUndoState> railUndoStates;
	Hint* fromHint;

public:
	KickResetSwitchUndoState(objCounterParameters());
	virtual ~KickResetSwitchUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	vector<RailUndoState>* getRailUndoStates() { return &railUndoStates; }
	//initialize and return a KickResetSwitchUndoState
	static KickResetSwitchUndoState* produce(
		objCounterParametersComma()
		ReferenceCounterHolder<UndoState>& stack,
		short pResetSwitchId,
		SpriteDirection pDirection,
		Hint* pFromHint);
	//release a reference to this KickResetSwitchUndoState and return it to the pool if applicable
	virtual void release();
	//kick the switch
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
