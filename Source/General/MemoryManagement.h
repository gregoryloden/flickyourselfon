#ifdef DEBUG
	/**/#define TRACK_OBJ_IDS
	#ifdef TRACK_OBJ_IDS
		#define onlyWhenTrackingIDs(x) x
	#else
		#define onlyWhenTrackingIDs(x)
	#endif
	#define newTracked(className, parameters) \
		((ObjCounter::prepareTracking(#className, __FILE__, __LINE__), new className parameters))

	class ObjCounter {
	private:
		static int objCount;
		static int untrackedObjCount;
		static int nextObjID;
		#ifdef TRACK_OBJ_IDS
			static ObjCounter* headObjCounter;
			static ObjCounter* tailObjCounter;
			static const char* preparedObjType;
			static const char* preparedObjFile;
			static int preparedObjLine;

			const char* objType;
			const char* objFile;
			int objLine;
			int objID;
			ObjCounter* prevObjCounter;
			ObjCounter* nextObjCounter;
		#endif

	public:
		ObjCounter();
		virtual ~ObjCounter();

		static void start();
		static void end();
		static void prepareTracking(const char* pObjType, const char* pObjFile, int pObjLine);
	};
#else
	#define newTracked(className, parameters) (new className parameters)
#endif
