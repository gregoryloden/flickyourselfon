#include "Level.h"
#include "GameState/MapState/MapState.h"

#define newPlane(owningLevel) newWithArgs(Plane, owningLevel)

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(Plane* pToPlane, int pRailByteIndex, int pRailBitShift)
: toPlane(pToPlane)
, railByteIndex(pRailByteIndex)
, railBitShift(pRailBitShift)
, railByteMask(Level::baseRailByteMask << pRailBitShift)
//this is fine because inverseRailByteMask is declared after railByteMask
, inverseRailByteMask(~railByteMask) {
}
LevelTypes::Plane::Connection::~Connection() {
	//don't delete the plane, it's owned by a Level
}

//////////////////////////////// LevelTypes::Plane::Tile ////////////////////////////////
LevelTypes::Plane::Tile::Tile(int pX, int pY)
: x(pX)
, y(pY) {
}
LevelTypes::Plane::Tile::~Tile() {}

//////////////////////////////// LevelTypes::Plane ////////////////////////////////
LevelTypes::Plane::Plane(objCounterParametersComma() Level* pOwningLevel)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
owningLevel(pOwningLevel)
, tiles()
, switchIds()
, connections() {
}
LevelTypes::Plane::~Plane() {
	//don't delete owningLevel, it owns and deletes this
}
void LevelTypes::Plane::addConnection(Plane* toPlane, short railId) {
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane)
			return;
	}
	if (railId == MapState::absentRailSwitchId)
		connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0));
	else {
		Connection* copyConnection = nullptr;
		for (Connection& connection : toPlane->connections) {
			if (connection.toPlane == this) {
				copyConnection = &connection;
				break;
			}
		}
		if (copyConnection != nullptr)
			connections.push_back(Connection(toPlane, copyConnection->railByteIndex, copyConnection->railBitShift));
		else {
			int railByteIndex;
			int railBitShift;
			owningLevel->getNextRailByteMask(&railByteIndex, &railBitShift);
			connections.push_back(Connection(toPlane, railByteIndex, railBitShift));
		}
	}
}
void LevelTypes::Plane::addSwitchId(short switchId) {
	for (short otherSwitchId : switchIds) {
		if (otherSwitchId == switchId)
			return;
	}
	switchIds.push_back(switchId);
}
using namespace LevelTypes;

//////////////////////////////// Level ////////////////////////////////
Level::Level(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
planes()
, railByteMaskBitsTracked(0)
, victoryPlane(nullptr)
, minimumRailColor(-1) {
}
Level::~Level() {
	for (Plane* plane : planes)
		delete plane;
	//don't delete victoryPlane, it's owned by another Level
}
Plane* Level::addNewPlane() {
	Plane* plane = newPlane(this);
	planes.push_back(plane);
	return plane;
}
void Level::getNextRailByteMask(int* outRailByteIndex, int* outRailBitShift) {
	*outRailByteIndex = railByteMaskBitsTracked / 32;
	*outRailBitShift = railByteMaskBitsTracked % 32;
	//make sure there are enough bits to fit the new mask
	if (*outRailBitShift + railByteMaskBitCount > 32) {
		*outRailByteIndex++;
		*outRailBitShift = 0;
		railByteMaskBitsTracked = railByteMaskBitsTracked / 32 * 32 + 32 + railByteMaskBitCount;
	} else
		railByteMaskBitsTracked += railByteMaskBitCount;
}
