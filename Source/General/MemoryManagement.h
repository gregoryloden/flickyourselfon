#ifdef DEBUG
	/**/#define TRACK_OBJ_IDS
	#ifdef TRACK_OBJ_IDS
		#define onlyWhenTrackingIDs(x) x
	#else
		#define onlyWhenTrackingIDs(x)
	#endif
	#define newTracked(className, parameters) \
		(static_cast<className*>(ObjCounter::track(new className parameters, #className, __FILE__, __LINE__)))

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
		ObjCounter();
		virtual ~ObjCounter();

		static void start();
		static void end();
		static ObjCounter* track(ObjCounter* obj, const char* pObjType, const char* file, int line);
	};
#else
	#define newTracked(className, parameters) (new className parameters)
#endif
