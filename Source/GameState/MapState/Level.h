#include "General/General.h"

#define newLevel() newWithoutArgs(Level)

class Level;
namespace LevelTypes {
	class RailByteMaskData;
}

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
		class ConnectionSwitch {
		public:
			vector<RailByteMaskData*> affectedRailByteMaskData;

			ConnectionSwitch();
			virtual ~ConnectionSwitch();
		};
		//Should only be allocated within an object, on the stack, or as a static object
		class Connection {
		public:
			Plane* toPlane;
			int railByteIndex;
			unsigned int railTileOffsetByteMask;

			Connection(Plane* pToPlane, int pRailByteIndex, int pRailTileOffsetByteMask);
			virtual ~Connection();
		};

		Level* owningLevel;
		vector<Tile> tiles;
		vector<ConnectionSwitch> connectionSwitches;
		vector<Connection> connections;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel);
		virtual ~Plane();

		void addTile(int x, int y) { tiles.push_back(Tile(x, y)); }
		Level* getOwningLevel() { return owningLevel; }
		void addRailConnectionToSwitch(LevelTypes::RailByteMaskData* railByteMaskData, int connectionSwitchesIndex) {
			connectionSwitches[connectionSwitchesIndex].affectedRailByteMaskData.push_back(railByteMaskData);
		}
		//add a switch
		//returns the index of the switch in this plane
		int addConnectionSwitch();
		//add a connection to another plane, if we don't already have one
		//returns true if the connection was redundant or added, and false if we need to add a rail connection instead
		bool addConnection(Plane* toPlane, bool isRail);
		//add a rail connection to another plane
		void addRailConnection(Plane* toPlane, LevelTypes::RailByteMaskData* railByteMaskData);
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class RailByteMaskData {
	public:
		short railId;
		int railByteIndex;
		int railBitShift;

		RailByteMaskData(short railId, int pRailByteIndex, int pRailBitShift);
		virtual ~RailByteMaskData();
	};
}
class Level onlyInDebug(: public ObjCounter) {
public:
	static const int absentRailByteIndex = -1;
	static const int railTileOffsetByteMaskBitCount = 3;
	static const int railMovementDirectionByteMaskBitCount = 1;
	static const int railByteMaskBitCount = railTileOffsetByteMaskBitCount + railMovementDirectionByteMaskBitCount;
	static const unsigned int baseRailTileOffsetByteMask = (1 << railTileOffsetByteMaskBitCount) - 1;
	static const unsigned int baseRailMovementDirectionByteMask =
		((1 << railMovementDirectionByteMaskBitCount) - 1) << railTileOffsetByteMaskBitCount;
	static const unsigned int baseRailByteMask = (1 << railByteMaskBitCount) - 1;

private:
	vector<LevelTypes::Plane*> planes;
	vector<LevelTypes::RailByteMaskData> railByteMaskData;
	int railByteMaskBitsTracked;
	LevelTypes::Plane* victoryPlane;
	char minimumRailColor;

public:
	Level(objCounterParameters());
	virtual ~Level();

	void assignVictoryPlane(LevelTypes::Plane* pVictoryPlane) { victoryPlane = pVictoryPlane; }
	int getRailByteMaskDataCount() { return (int)railByteMaskData.size(); }
	LevelTypes::RailByteMaskData* getRailByteMaskData(int i) { return &railByteMaskData[i]; }
	//add a new plane to this level
	LevelTypes::Plane* addNewPlane();
	//create a byte mask for a new rail
	//returns the index into the internal byte mask vector for use in getRailByteMaskData()
	int trackNextRail(short railId, char color);
};
