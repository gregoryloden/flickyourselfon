#include "General/General.h"

class PositionState onlyInDebug(: public ObjCounter) {
private:
	float x;
	float y;
	char dX;
	char dY;
	float speedPerTick;
	int lastUpdateTicksTime;

public:
	PositionState(objCounterParametersComma() float pX, float pY);
	~PositionState();

	char getDX() { return dX; }
	char getDY() { return dY; }
	float getX(int ticksTime);
	float getY(int ticksTime);
	bool isMoving();
	bool updatedSameTimeAs(PositionState* other);
	void updateWithPreviousPositionState(PositionState* prev, int ticksTime);
};
