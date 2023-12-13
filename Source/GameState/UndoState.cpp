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

//////////////////////////////// NoOpUndoState ////////////////////////////////
const int NoOpUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
NoOpUndoState::NoOpUndoState(objCounterParameters())
: UndoState(objCounterArguments()) {
}
NoOpUndoState::~NoOpUndoState() {}
NoOpUndoState* NoOpUndoState::produce(objCounterParametersComma() UndoState* pNext) {
	initializeWithNewFromPool(n, NoOpUndoState)
	n->next.set(pNext);
	return n;
}
pooledReferenceCounterDefineRelease(NoOpUndoState)
bool NoOpUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoNoOp(isUndo);
	return false;
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
bool MoveUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	return playerState->undoMove(fromX, fromY, MapState::invalidHeight, isUndo, ticksTime);
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
bool ClimbFallUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	return playerState->undoMove(fromX, fromY, fromHeight, isUndo, ticksTime);
}
