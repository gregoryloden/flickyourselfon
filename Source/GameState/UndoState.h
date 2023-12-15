#include "Util/PooledReferenceCounter.h"

#define stackNewNoOpUndoState(stack) stack.set(produceWithArgs(NoOpUndoState, stack.get()))
#define stackNewMoveUndoState(stack, fromX, fromY) stack.set(produceWithArgs(MoveUndoState, stack.get(), fromX, fromY))
#define stackNewClimbFallUndoState(stack, fromX, fromY, fromHeight) \
	stack.set(produceWithArgs(ClimbFallUndoState, stack.get(), fromX, fromY, fromHeight))
#define stackNewRideRailUndoState(stack, railId) stack.set(produceWithArgs(RideRailUndoState, stack.get(), railId))
#define stackNewKickSwitchUndoState(stack, switchId) stack.set(produceWithArgs(KickSwitchUndoState, stack.get(), switchId))

class PlayerState;

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
	static NoOpUndoState* produce(objCounterParametersComma() UndoState* pNext);
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
	static MoveUndoState* produce(objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY);
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

public:
	ClimbFallUndoState(objCounterParameters());
	virtual ~ClimbFallUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a ClimbFallUndoState
	static ClimbFallUndoState* produce(
		objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY, char pFromHeight);
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

public:
	RideRailUndoState(objCounterParameters());
	virtual ~RideRailUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a RideRailUndoState
	static RideRailUndoState* produce(objCounterParametersComma() UndoState* pNext, short pRailId);
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

public:
	KickSwitchUndoState(objCounterParameters());
	virtual ~KickSwitchUndoState();

	int getTypeIdentifier() { return classTypeIdentifier; }
	//initialize and return a KickSwitchUndoState
	static KickSwitchUndoState* produce(objCounterParametersComma() UndoState* pNext, short pSwitchId);
	//release a reference to this KickSwitchUndoState and return it to the pool if applicable
	virtual void release();
	//kick the switch
	virtual bool handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
