#include "General.h"
#include "Util/Logger.h"

#ifdef DEBUG
	#ifdef TRACK_OBJ_IDS
		//**/#define LOG_OBJ_ADD_OR_REMOVE

		ObjCounter* ObjCounter::headObjCounter = nullptr;
		ObjCounter* ObjCounter::tailObjCounter = nullptr;
	#endif
	int ObjCounter::objCount = 0;
	int ObjCounter::untrackedObjCount = 0;
	int ObjCounter::nextObjID = 0;
	ObjCounter::ObjCounter(objCounterParameters())
	#ifdef TRACK_OBJ_IDS
		: objType(pObjType)
		, objFile(pObjFile)
		, objLine(pObjLine)
		, objID(nextObjID)
		, prevObjCounter(nullptr)
		, nextObjCounter(nullptr)
	#endif
	{
		objCount++;
		nextObjID++;

		#ifdef LOG_OBJ_ADD_OR_REMOVE
			stringstream logMessage;
			logMessage << "  Added " << (void*)this << " " << objType << " " << objID << ", obj count: " << objCount;
			Logger::debugLogger.logString(logMessage.str());
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
		#ifdef LOG_OBJ_ADD_OR_REMOVE
			stringstream logMessage;
			logMessage << "Deleted " << (void*)this << " " << objType << " " << objID << ", obj count: " << objCount;
			Logger::debugLogger.logString(logMessage.str());
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
	void ObjCounter::start() {
		untrackedObjCount = nextObjID;
		#ifdef TRACK_OBJ_IDS
			//reset the list, it's ok to leave any objects linked
			headObjCounter = nullptr;
			tailObjCounter = nullptr;
		#endif
	}
	void ObjCounter::end() {
		stringstream logMessage;
		#ifdef TRACK_OBJ_IDS
			for (; headObjCounter != nullptr; headObjCounter = headObjCounter->nextObjCounter) {
				logMessage.str(string());
				logMessage
					<< "      Remaining object: " << (void*)headObjCounter
					<< " " << headObjCounter->objType
					<< " " << headObjCounter->objID
					<< ", line " << headObjCounter->objLine
					<< " file " << headObjCounter->objFile;
				Logger::debugLogger.logString(logMessage.str());
			}
		#endif
		logMessage.str(string());
		logMessage << "Total remaining objects: " << (objCount - untrackedObjCount);
		Logger::debugLogger.logString(logMessage.str());
		logMessage.str(string());
		logMessage << "Total objects used: " << (nextObjID - untrackedObjCount) << " + " << untrackedObjCount << " untracked";
		Logger::debugLogger.logString(logMessage.str());
	}
#endif
