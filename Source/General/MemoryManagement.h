#ifdef DEBUG
	/**/#define TRACK_OBJ_IDS
	#ifdef TRACK_OBJ_IDS
		#define newWithArgs(className, ...) \
			new className(#className, __FILE__, __LINE__, __VA_ARGS__)
		#define newWithoutArgs(className) \
			new className(#className, __FILE__, __LINE__)
		#define objCounterParameters() const char* pObjType, const char* pObjFile, int pObjLine
		#define objCounterParametersComma() objCounterParameters(),
		#define objCounterArguments() pObjType, pObjFile, pObjLine
	#endif

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

		static void start();
		static void end();
	};
#endif
#ifndef newWithArgs
	#define newWithArgs(className, ...) new className(__VA_ARGS__)
	#define newWithoutArgs(className) new className()
	#define objCounterParameters()
	#define objCounterParametersComma()
	#define objCounterArguments()
#endif
