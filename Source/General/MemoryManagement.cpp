#include "General.h"
#include "Logger.h"

#ifdef DEBUG
	#ifdef TRACK_OBJ_IDS
		//**/#define PRINT_OBJ_ADD_OR_REMOVE

		ObjCounter* ObjCounter::headObjCounter = nullptr;
		ObjCounter* ObjCounter::tailObjCounter = nullptr;
		const char* ObjCounter::preparedObjType = "";
		const char* ObjCounter::preparedObjFile = "";
		int ObjCounter::preparedObjLine = -1;
	#endif
	int ObjCounter::objCount = 0;
	int ObjCounter::untrackedObjCount = 0;
	int ObjCounter::nextObjID = 0;
	ObjCounter::ObjCounter()
	#ifdef TRACK_OBJ_IDS
		: objType(preparedObjType)
		, objFile(preparedObjFile)
		, objLine(preparedObjLine)
		, objID(nextObjID)
		, prevObjCounter(nullptr)
		, nextObjCounter(nullptr)
	#endif
	{
		objCount++;
		nextObjID++;

		#ifdef PRINT_OBJ_ADD_OR_REMOVE
			stringstream logMessage;
			logMessage << "  Added " << (void*)this << " " << objType << " " << objID << ", obj count: " << objCount << "\n";
			Logger::logString(logMessage.str());
		#endif
		#ifdef TRACK_OBJ_IDS
			if (tailObjCounter != nullptr)
				tailObjCounter->nextObjCounter = this;
			prevObjCounter = tailObjCounter;
			tailObjCounter = this;
			if (headObjCounter == nullptr)
				headObjCounter = this;
		#endif
	}
	ObjCounter::~ObjCounter() {
		objCount--;
		#ifdef PRINT_OBJ_ADD_OR_REMOVE
			stringstream logMessage;
			logMessage << "Deleted " << (void*)this << " " << objType << " " << objID << ", obj count: " << objCount << "\n";
			Logger::logString(logMessage.str());
		#endif
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
		stringstream logMessage;
		#ifdef TRACK_OBJ_IDS
			for (; headObjCounter != nullptr; headObjCounter = headObjCounter->nextObjCounter)
				logMessage
					<< "      Remaining object: " << (void*)headObjCounter
					<< " " << headObjCounter->objType
					<< " " << headObjCounter->objID
					<< ", line " << headObjCounter->objLine
					<< " file " << headObjCounter->objFile
					<< "\n";
		#endif
		logMessage << "Total remaining objects: " << (objCount - untrackedObjCount) << "\n";
		logMessage << "Total objects used: " << (nextObjID - untrackedObjCount) << " + " << untrackedObjCount << " untracked\n";
		Logger::logString(logMessage.str());
	}
	//save object information for the object we're about to construct
	void ObjCounter::prepareTracking(const char* pObjType, const char* pObjFile, int pObjLine) {
		preparedObjType = pObjType;
		preparedObjFile = pObjFile;
		preparedObjLine = pObjLine;
	}
#endif
