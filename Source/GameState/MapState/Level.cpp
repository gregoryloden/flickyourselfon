#include "Level.h"

#define newLevelPlane(owningLevel) newWithArgs(LevelTypes::Plane, owningLevel)

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
, switchIds() {
}
LevelTypes::Plane::~Plane() {
	//don't delete owningLevel, it owns and deletes this
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
	Plane* plane = newLevelPlane(this);
	planes.push_back(plane);
	return plane;
}
