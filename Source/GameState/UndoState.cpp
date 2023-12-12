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
, fromY(0) {
}
MoveUndoState::~MoveUndoState() {}
MoveUndoState* MoveUndoState::produce(objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY) {
	initializeWithNewFromPool(m, MoveUndoState)
	m->next.set(pNext);
	m->fromX = pFromX;
	m->fromY = pFromY;
	return m;
}
pooledReferenceCounterDefineRelease(MoveUndoState)
void MoveUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoMove(fromX, fromY, MapState::invalidHeight, isUndo, ticksTime);
}

//////////////////////////////// ClimbFallUndoState ////////////////////////////////
const int ClimbFallUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
ClimbFallUndoState::ClimbFallUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, fromX(0)
, fromY(0)
, fromHeight(0) {
}
ClimbFallUndoState::~ClimbFallUndoState() {}
ClimbFallUndoState* ClimbFallUndoState::produce(
	objCounterParametersComma() UndoState* pNext, float pFromX, float pFromY, char pFromHeight)
{
	initializeWithNewFromPool(c, ClimbFallUndoState)
	c->next.set(pNext);
	c->fromX = pFromX;
	c->fromY = pFromY;
	c->fromHeight = pFromHeight;
	return c;
}
pooledReferenceCounterDefineRelease(ClimbFallUndoState)
void ClimbFallUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoMove(fromX, fromY, fromHeight, isUndo, ticksTime);
}
