#include "General.h"
#include "SpriteSheet.h"
#include "GameState.h"

#ifdef DEBUG
	#ifdef TRACK_OBJ_IDS
		ObjCounter* ObjCounter::headObjCounter = nullptr;
		ObjCounter* ObjCounter::tailObjCounter = nullptr;
	#endif
	int ObjCounter::objCount = 0;
	int ObjCounter::untrackedObjCount = 0;
	int ObjCounter::nextObjID = 0;
	ObjCounter::ObjCounter()
	#ifdef TRACK_OBJ_IDS
		: objType("(empty)")
		, objFile("")
		, objLine(-1)
		, objID(nextObjID)
		, prevObjCounter(nullptr)
		, nextObjCounter(nullptr)
	#endif
	{
		objCount++;
		nextObjID++;
	}
	ObjCounter::~ObjCounter() {
		objCount--;
		#ifdef TRACK_OBJ_IDS
			if (nextObjCounter != nullptr)
				nextObjCounter->prevObjCounter = prevObjCounter;
			else if (this == tailObjCounter)
				tailObjCounter = prevObjCounter;
			if (prevObjCounter != nullptr)
				prevObjCounter->nextObjCounter = nextObjCounter;
			else if (this == headObjCounter)
				headObjCounter = nextObjCounter;
		#endif
	}
	//assign the initial object ID to exclude any statically-allocated ObjCounters
	void ObjCounter::start() {
		untrackedObjCount = nextObjID;
		#ifdef TRACK_OBJ_IDS
			//reset the list, it's ok to leave any objects linked
			headObjCounter = nullptr;
			tailObjCounter = nullptr;
		#endif
	}
	//check for any non-deallocated objects
	void ObjCounter::end() {
		#ifdef TRACK_OBJ_IDS
			for (; headObjCounter != nullptr; headObjCounter = headObjCounter->nextObjCounter)
				;
		#endif
	}
	//track this object in the global list
	ObjCounter* ObjCounter::track(ObjCounter* obj, const char* pObjType, const char* file, int line) {
		obj->objType = pObjType;
		obj->objFile = file;
		obj->objLine = line;

		#ifdef TRACK_OBJ_IDS
			if (tailObjCounter != nullptr)
				tailObjCounter->nextObjCounter = obj;
			obj->prevObjCounter = tailObjCounter;
			tailObjCounter = obj;
			if (headObjCounter == nullptr)
				headObjCounter = obj;
		#endif
		return obj;
	}
#endif
