#include "UndoState.h"
#include "GameState/PlayerState.h"

#define initializeWithNewFromPoolAndPlaceIntoStack(var, className, stack) \
	initializeWithNewFromPool(var, className) \
	var->next.set(stack.get()); \
	stack.set(var);

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
NoOpUndoState* NoOpUndoState::produce(objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack) {
	initializeWithNewFromPoolAndPlaceIntoStack(n, NoOpUndoState, stack)
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
MoveUndoState* MoveUndoState::produce(
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, float pFromX, float pFromY)
{
	initializeWithNewFromPoolAndPlaceIntoStack(m, MoveUndoState, stack)
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
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, float pFromX, float pFromY, char pFromHeight)
{
	initializeWithNewFromPoolAndPlaceIntoStack(c, ClimbFallUndoState, stack)
	c->fromX = pFromX;
	c->fromY = pFromY;
	c->fromHeight = pFromHeight;
	return c;
}
pooledReferenceCounterDefineRelease(ClimbFallUndoState)
bool ClimbFallUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	return playerState->undoMove(fromX, fromY, fromHeight, isUndo, ticksTime);
}

//////////////////////////////// RideRailUndoState ////////////////////////////////
const int RideRailUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
RideRailUndoState::RideRailUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, railId(0) {
}
RideRailUndoState::~RideRailUndoState() {}
RideRailUndoState* RideRailUndoState::produce(
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, short pRailId)
{
	initializeWithNewFromPoolAndPlaceIntoStack(r, RideRailUndoState, stack)
	r->railId = pRailId;
	return r;
}
pooledReferenceCounterDefineRelease(RideRailUndoState)
bool RideRailUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoRideRail(railId, isUndo, ticksTime);
	return true;
}

//////////////////////////////// KickSwitchUndoState ////////////////////////////////
const int KickSwitchUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
KickSwitchUndoState::KickSwitchUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, switchId(0)
, direction(SpriteDirection::Down) {
}
KickSwitchUndoState::~KickSwitchUndoState() {}
KickSwitchUndoState* KickSwitchUndoState::produce(
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, short pSwitchId, SpriteDirection pDirection)
{
	initializeWithNewFromPoolAndPlaceIntoStack(k, KickSwitchUndoState, stack)
	k->switchId = pSwitchId;
	k->direction = pDirection;
	return k;
}
pooledReferenceCounterDefineRelease(KickSwitchUndoState)
bool KickSwitchUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoKickSwitch(switchId, direction, isUndo, ticksTime);
	return true;
}

//////////////////////////////// KickResetSwitchUndoState::RailUndoState ////////////////////////////////
KickResetSwitchUndoState::RailUndoState::RailUndoState(short pRailId, float pFromTargetTileOffset, char pFromMovementDirection)
: railId(pRailId)
, fromTargetTileOffset(pFromTargetTileOffset)
, fromMovementDirection(pFromMovementDirection) {
}
KickResetSwitchUndoState::RailUndoState::~RailUndoState() {}

//////////////////////////////// KickResetSwitchUndoState ////////////////////////////////
const int KickResetSwitchUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
KickResetSwitchUndoState::KickResetSwitchUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, resetSwitchId(0)
, direction(SpriteDirection::Down)
, railUndoStates() {
}
KickResetSwitchUndoState::~KickResetSwitchUndoState() {}
KickResetSwitchUndoState* KickResetSwitchUndoState::produce(
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, short pResetSwitchId, SpriteDirection pDirection)
{
	initializeWithNewFromPoolAndPlaceIntoStack(k, KickResetSwitchUndoState, stack)
	k->resetSwitchId = pResetSwitchId;
	k->direction = pDirection;
	k->railUndoStates.clear();
	return k;
}
pooledReferenceCounterDefineRelease(KickResetSwitchUndoState)
bool KickResetSwitchUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoKickResetSwitch(resetSwitchId, direction, this, isUndo, ticksTime);
	return true;
}
