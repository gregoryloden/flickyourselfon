#include "UndoState.h"
#include "GameState/HintState.h"
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
	next.clear();
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
	return playerState->undoMove(fromX, fromY, MapState::invalidHeight, nullptr, isUndo, ticksTime);
}

//////////////////////////////// ClimbFallUndoState ////////////////////////////////
const int ClimbFallUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
ClimbFallUndoState::ClimbFallUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, fromX(0)
, fromY(0)
, fromHeight(0)
, fromHint(nullptr) {
}
ClimbFallUndoState::~ClimbFallUndoState() {}
ClimbFallUndoState* ClimbFallUndoState::produce(
	objCounterParametersComma()
	ReferenceCounterHolder<UndoState>& stack,
	float pFromX,
	float pFromY,
	char pFromHeight,
	Hint* pFromHint)
{
	initializeWithNewFromPoolAndPlaceIntoStack(c, ClimbFallUndoState, stack)
	c->fromX = pFromX;
	c->fromY = pFromY;
	c->fromHeight = pFromHeight;
	c->fromHint = pFromHint;
	return c;
}
pooledReferenceCounterDefineRelease(ClimbFallUndoState)
bool ClimbFallUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	return playerState->undoMove(fromX, fromY, fromHeight, fromHint, isUndo, ticksTime);
}

//////////////////////////////// RideRailUndoState ////////////////////////////////
const int RideRailUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
RideRailUndoState::RideRailUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, railId(0)
, fromHint(nullptr) {
}
RideRailUndoState::~RideRailUndoState() {}
RideRailUndoState* RideRailUndoState::produce(
	objCounterParametersComma() ReferenceCounterHolder<UndoState>& stack, short pRailId, Hint* pFromHint)
{
	initializeWithNewFromPoolAndPlaceIntoStack(r, RideRailUndoState, stack)
	r->railId = pRailId;
	r->fromHint = pFromHint;
	return r;
}
pooledReferenceCounterDefineRelease(RideRailUndoState)
bool RideRailUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoRideRail(railId, fromHint, isUndo, ticksTime);
	return true;
}

//////////////////////////////// KickSwitchUndoState ////////////////////////////////
const int KickSwitchUndoState::classTypeIdentifier = UndoState::getNextClassTypeIdentifier();
KickSwitchUndoState::KickSwitchUndoState(objCounterParameters())
: UndoState(objCounterArguments())
, switchId(0)
, direction(SpriteDirection::Down)
, fromHint(nullptr) {
}
KickSwitchUndoState::~KickSwitchUndoState() {}
KickSwitchUndoState* KickSwitchUndoState::produce(
	objCounterParametersComma()
	ReferenceCounterHolder<UndoState>& stack,
	short pSwitchId,
	SpriteDirection pDirection,
	Hint* pFromHint)
{
	initializeWithNewFromPoolAndPlaceIntoStack(k, KickSwitchUndoState, stack)
	k->switchId = pSwitchId;
	k->direction = pDirection;
	k->fromHint = pFromHint;
	return k;
}
pooledReferenceCounterDefineRelease(KickSwitchUndoState)
bool KickSwitchUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoKickSwitch(switchId, direction, fromHint, isUndo, ticksTime);
	return true;
}

//////////////////////////////// KickResetSwitchUndoState::RailUndoState ////////////////////////////////
KickResetSwitchUndoState::RailUndoState::RailUndoState(short pRailId, char pFromTargetTileOffset, char pFromMovementDirection)
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
, railUndoStates()
, fromHint(nullptr) {
}
KickResetSwitchUndoState::~KickResetSwitchUndoState() {}
KickResetSwitchUndoState* KickResetSwitchUndoState::produce(
	objCounterParametersComma()
	ReferenceCounterHolder<UndoState>& stack,
	short pResetSwitchId,
	SpriteDirection pDirection,
	Hint* pFromHint)
{
	initializeWithNewFromPoolAndPlaceIntoStack(k, KickResetSwitchUndoState, stack)
	k->resetSwitchId = pResetSwitchId;
	k->direction = pDirection;
	k->railUndoStates.clear();
	k->fromHint = pFromHint;
	return k;
}
pooledReferenceCounterDefineRelease(KickResetSwitchUndoState)
bool KickResetSwitchUndoState::handle(PlayerState* playerState, bool isUndo, int ticksTime) {
	playerState->undoKickResetSwitch(resetSwitchId, direction, this, fromHint, isUndo, ticksTime);
	return true;
}
