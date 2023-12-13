#include "Util/PooledReferenceCounter.h"

#define newMoveUndoState(next, fromX, fromY) produceWithArgs(MoveUndoState, next, fromX, fromY)
#define newClimbFallUndoState(next, fromX, fromY, fromHeight) \
	produceWithArgs(ClimbFallUndoState, next, fromX, fromY, fromHeight)

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
