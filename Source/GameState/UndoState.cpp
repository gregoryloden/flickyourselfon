#include "UndoState.h"
#include "GameState/PlayerState.h"

//////////////////////////////// UndoState ////////////////////////////////
UndoState::UndoState(objCounterParameters())
: PooledReferenceCounter(objCounterArguments())
, next(nullptr) {
}
UndoState::~UndoState() {}
void UndoState::prepareReturnToPool() {
	next.set(nullptr);
}
int UndoState::getNextClassTypeIdentifier() {
	static int nextClassTypeIdentifier = 0;
	nextClassTypeIdentifier++;
	return nextClassTypeIdentifier;
}

//////////////////////////////// MoveUndoState ////////////////////////////////
const int MoveUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
MoveUndoState::MoveUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, fromX(0)
, fromY(0)
, fromHeight(0) {
}
MoveUndoState::~MoveUndoState() {}
MoveUndoState* MoveUndoState::produce(
	objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY, char pFromHeight)
{
	initializeWithNewFromPool(m, MoveUndoState)
	m->next.set(pNext);
	m->fromX = pFromX;
	m->fromY = pFromY;
	m->fromHeight = pFromHeight;
	return m;
}
pooledReferenceCounterDefineRelease(MoveUndoState)
void MoveUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoMove(fromX, fromY, fromHeight, isUndo, ticksTime);
}
