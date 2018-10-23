#include "PositionState.h"
#include "GameState/MapState.h"

PositionState::PositionState(objCounterParametersComma() float pX, float pY)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
x(pX)
, y(pY)
, dX(0)
, dY(0)
, speedPerTick(0.0f)
, lastUpdateTicksTime(0) {
}
PositionState::~PositionState() {}
//return whether this position is moving
bool PositionState::isMoving() {
	return (dX | dY) != 0;
}
//return whether these two states were last updated at the same time
bool PositionState::updatedSameTimeAs(PositionState* other) {
	return lastUpdateTicksTime == other->lastUpdateTicksTime;
}
//return the position's x coordinate at the given time
float PositionState::getX(int ticks) {
	return x + (float)dX * speedPerTick * (float)(ticks - lastUpdateTicksTime);
}
//return the position's x coordinate at the given time
float PositionState::getY(int ticks) {
	return y + (float)dY * speedPerTick * (float)(ticks - lastUpdateTicksTime);
}
//update this position state by reading from the previous state
void PositionState::updateWithPreviousPositionState(PositionState* prev, int ticksTime) {
	const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
	dX = (char)(keyboardState[SDL_SCANCODE_RIGHT] - keyboardState[SDL_SCANCODE_LEFT]);
	dY = (char)(keyboardState[SDL_SCANCODE_DOWN] - keyboardState[SDL_SCANCODE_UP]);
	speedPerTick = ((dX & dY) != 0 ? Map::diagonalSpeed : Map::speed) / 1000.0f;

	//if we didn't change direction, copy the position and we're done
	if (dX == prev->dX && dY == prev->dY) {
		x = prev->x;
		y = prev->y;
		lastUpdateTicksTime = prev->lastUpdateTicksTime;
	//we changed velocity in some way
	} else {
		//update the position to account for previous movement
		x = prev->getX(ticksTime);
		y = prev->getY(ticksTime);
		lastUpdateTicksTime = ticksTime;
	}
}
