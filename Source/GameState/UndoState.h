#include "Util/PooledReferenceCounter.h"

#define newMoveUndoState(next, fromX, fromY, fromHeight) produceWithArgs(MoveUndoState, next, fromX, fromY, fromHeight)

class PlayerState;

class UndoState: public PooledReferenceCounter {
public:
	const int typeIdentifier;
	ReferenceCounterHolder<UndoState> next;

	UndoState(objCounterParametersComma() int pTypeIdentifier);
	virtual ~UndoState();

protected:
	//release the next state before this is returned to the pool
	virtual void prepareReturnToPool();
	//get the next class type identifier for a subclass
	static int getNextClassTypeIdentifier();
public:
	//apply the effect of this state as an undo or a redo
	virtual void handle(PlayerState* playerState, bool isUndo, int ticksTime) = 0;
};
class MoveUndoState: public UndoState {
public:
	static const int classTypeIdentifier;

private:
	float fromX;
	float fromY;
	char fromHeight;

public:
	MoveUndoState(objCounterParameters());
	virtual ~MoveUndoState();

	//initialize and return a MoveUndoState
	static MoveUndoState* produce(objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY, char pFromHeight);
	//release a reference to this MoveUndoState and return it to the pool if applicable
	virtual void release();
	//move the player to the stored position
	virtual void handle(PlayerState* playerState, bool isUndo, int ticksTime);
};
