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

		Level* owningLevel;
		vector<Tile> tiles;
		vector<short> switchIds;

	public:
		Plane(objCounterParametersComma() Level* pOwningLevel);
		virtual ~Plane();

		void addTile(int x, int y) { tiles.push_back(Tile(x, y)); }
		//track a switch in this plane
		void addSwitchId(short switchId);
	};
}
class Level onlyInDebug(: public ObjCounter) {
private:
	vector<LevelTypes::Plane*> planes;
	LevelTypes::Plane* victoryPlane;

public:
	Level(objCounterParameters());
	virtual ~Level();

	void assignVictoryPlane(LevelTypes::Plane* pVictoryPlane) { victoryPlane = pVictoryPlane; }
	//add a new plane to this level
	LevelTypes::Plane* addNewPlane();
};
