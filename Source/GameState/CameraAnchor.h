#include "General/General.h"

class CameraAnchor onlyInDebug(: public ObjCounter) {
public:
	CameraAnchor(objCounterParameters());
	~CameraAnchor();

	virtual float getCameraCenterX(int ticksTime) = 0;
	virtual float getCameraCenterY(int ticksTime) = 0;
};
