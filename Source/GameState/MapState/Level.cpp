#include "Level.h"

#define newPlane(owningLevel) newWithArgs(Plane, owningLevel)

//////////////////////////////// LevelTypes::Plane::ConnectionSwitch ////////////////////////////////
LevelTypes::Plane::ConnectionSwitch::ConnectionSwitch()
: affectedRailByteMaskData() {
}
LevelTypes::Plane::ConnectionSwitch::~ConnectionSwitch() {}

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask)
: toPlane(pToPlane)
, railByteIndex(pRailByteIndex)
, railTileOffsetByteMask(pRailTileOffsetByteMask) {
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
, connectionSwitches()
, connections() {
}
LevelTypes::Plane::~Plane() {
	//don't delete owningLevel, it owns and deletes this
}
int LevelTypes::Plane::addConnectionSwitch() {
	connectionSwitches.push_back(ConnectionSwitch());
	return (int)connectionSwitches.size() - 1;
}
bool LevelTypes::Plane::addConnection(Plane* toPlane, bool isRail) {
	//don't connect to a plane twice
	for (Connection& connection : connections) {
		if (connection.toPlane == toPlane)
			return true;
	}
	//add a climb/fall connection
	if (!isRail) {
		connections.push_back(Connection(toPlane, Level::absentRailByteIndex, 0));
		return true;
	}
	//add a rail connection to a plane that is already connected to this plane
	for (Connection& connection : toPlane->connections) {
		if (connection.toPlane == this) {
			connections.push_back(Connection(toPlane, connection.railByteIndex, connection.railTileOffsetByteMask));
			return true;
		}
	}
	return false;
}
void LevelTypes::Plane::addRailConnection(Plane* toPlane, LevelTypes::RailByteMaskData* railByteMaskData) {
	connections.push_back(
		Connection(
			toPlane, railByteMaskData->railByteIndex, Level::baseRailTileOffsetByteMask << railByteMaskData->railBitShift));
}

//////////////////////////////// LevelTypes::RailByteMaskData ////////////////////////////////
LevelTypes::RailByteMaskData::RailByteMaskData(short pRailId, int pRailByteIndex, int pRailBitShift)
: railId(pRailId)
, railByteIndex(pRailByteIndex)
, railBitShift(pRailBitShift) {
}
LevelTypes::RailByteMaskData::~RailByteMaskData() {}
using namespace LevelTypes;

//////////////////////////////// Level ////////////////////////////////
Level::Level(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
planes()
, railByteMaskData()
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
int Level::trackNextRail(short railId, char color) {
	minimumRailColor = MathUtils::max(color, minimumRailColor);
	int railByteIndex = railByteMaskBitsTracked / 32;
	int railBitShift = railByteMaskBitsTracked % 32;
	//make sure there are enough bits to fit the new mask
	if (railBitShift + railByteMaskBitCount > 32) {
		railByteIndex++;
		railBitShift = 0;
		railByteMaskBitsTracked = (railByteMaskBitsTracked / 32 + 1) * 32 + railByteMaskBitCount;
	} else
		railByteMaskBitsTracked += railByteMaskBitCount;
	railByteMaskData.push_back(RailByteMaskData(railId, railByteIndex, railBitShift));
	return (int)railByteMaskData.size() - 1;
}
