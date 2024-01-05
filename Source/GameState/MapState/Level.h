#include "General/General.h"

#define newLevel() newWithoutArgs(Level)

class Level;

namespace LevelTypes {
	class Plane onlyInDebug(: public ObjCounter) {
	private:
		//Should only be allocated within an object, on the stack, or as a static object
		class Tile {
		public:
			int x;
			int y;

			Tile(int pX, int pY);
			virtual ~Tile();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			int railByteIndex;
			unsigned int railByteMask;
			unsigned int inverseRailByteMask;
			int railBitShift;

			Connection(Plane* pToPlane, int pRailByteIndex, int pRailBitShift);
			virtual ~Connection();
		};

		Level* owningLevel;
		vector<Tile> tiles;
		vector<Connection> connections;
		vector<short> switchIds;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel);
		virtual ~Plane();

		void addTile(int x, int y) { tiles.push_back(Tile(x, y)); }
		//add a connection to another plane, if we don't already have one
		void addConnection(Plane* toPlane, short railId);
		//track a switch in this plane
		void addSwitchId(short switchId);
	};
}
class Level onlyInDebug(: public ObjCounter) {
public:
	static const int absentRailByteIndex = -1;
	static const int railByteMaskBitCount = 4;
	static const unsigned int baseRailByteMask = (1 << railByteMaskBitCount) - 1;

private:
	vector<LevelTypes::Plane*> planes;
	int railByteMaskBitsTracked;
	LevelTypes::Plane* victoryPlane;
	char minimumRailColor;

public:
	Level(objCounterParameters());
	virtual ~Level();

	void assignVictoryPlane(LevelTypes::Plane* pVictoryPlane) { victoryPlane = pVictoryPlane; }
	void setMinimumRailColor(char color) { minimumRailColor = MathUtils::max(color, minimumRailColor); }
	//add a new plane to this level
	LevelTypes::Plane* addNewPlane();
	//create a byte mask for a new rail
	void getNextRailByteMask(int* outRailByteIndex, int* outRailBitShift);
};
