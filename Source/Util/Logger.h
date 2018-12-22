#include "General/General.h"

class GameState;
template <class Type> class CircularStateQueue;

class Logger {
private:
	class Message onlyInDebug(: public ObjCounter) {
	public:
		string message;
		int timestamp;

		Message(objCounterParameters());
		~Message();
	};

	static SDL_RWops* file;
	static bool queueLogMessages;
	static thread_local stringstream* currentMessageStringstream;
	static thread_local CircularStateQueue<Message>* currentThreadLogQueue;
	static CircularStateQueue<Message>* mainLogQueue;
	static CircularStateQueue<Message>* renderLogQueue;
	static bool threadRunning;
	static thread* logThread;

public:
	static void beginLogging();
	static void beginMultiThreadedLogging();
	static void endMultiThreadedLogging();
	static void endLogging();
private:
	static void logLoop();
public:
	static void setupLoggingForRenderThread();
	static void log(const char* message);
	static void logString(string& message);
};
