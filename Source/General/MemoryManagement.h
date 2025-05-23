#ifdef DEBUG
	/**/#define TRACK_OBJ_IDS
	#ifdef TRACK_OBJ_IDS
		#define objCounterLocalArguments(className) #className, __FILE__, __LINE__
		#define objCounterLocalArgumentsComma(className) objCounterLocalArguments(className),
		#define objCounterParameters() const char* pObjType, const char* pObjFile, int pObjLine
		#define objCounterParametersComma() objCounterParameters(),
		#define objCounterArguments() pObjType, pObjFile, pObjLine
		#define objCounterArgumentsComma() objCounterArguments(),
		#define newInPlaceWithoutArgs(className, varName) className varName (objCounterLocalArguments(className))
	#endif
#endif
#ifndef objCounterLocalArguments
	#define objCounterLocalArguments(className)
	#define objCounterLocalArgumentsComma(className)
	#define objCounterParameters()
	#define objCounterParametersComma()
	#define objCounterArguments()
	#define objCounterArgumentsComma()
	#define newInPlaceWithoutArgs(className, varName) className varName
#endif
#define newWithArgs(className, ...) new className(objCounterLocalArgumentsComma(className) __VA_ARGS__)
#define newWithoutArgs(className) new className(objCounterLocalArguments(className))
#define newInPlaceWithArgs(className, varName, ...) className varName (objCounterLocalArgumentsComma(className) __VA_ARGS__)
#define produceWithArgs(className, ...) className::produce(objCounterLocalArgumentsComma(className) __VA_ARGS__)
#define produceWithoutArgs(className) className::produce(objCounterLocalArguments(className))

#ifdef DEBUG
	class ObjCounter {
	private:
		static int objCount;
		static int untrackedObjCount;
		static int nextObjID;
		#ifdef TRACK_OBJ_IDS
			static ObjCounter* headObjCounter;
			static ObjCounter* tailObjCounter;

			const char* objType;
			const char* objFile;
			int objLine;
			int objID;
			ObjCounter* prevObjCounter;
			ObjCounter* nextObjCounter;
		#endif

	public:
		ObjCounter(objCounterParameters());
		virtual ~ObjCounter();

		//assign the initial object ID to exclude any statically-allocated ObjCounters
		static void start();
		//check for any non-deallocated objects
		static void end();
	};
#endif
