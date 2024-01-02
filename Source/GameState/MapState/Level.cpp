#include "Level.h"

#define newPlane(owningLevel) newWithArgs(Plane, owningLevel)

//////////////////////////////// LevelTypes::Plane::Connection ////////////////////////////////
LevelTypes::Plane::Connection::Connection(Plane* pToPlane, short pRailId)
: toPlane(pToPlane)
, railId(pRailId) {
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
	connections.push_back(Connection(toPlane, railId));
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
, victoryPlane(nullptr) {
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
