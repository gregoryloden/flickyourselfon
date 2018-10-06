#include "General.h"

class GameState;
template <class Type> class CircularStateQueue;

class Logger {
private:
	class Message {
	public:
		string message;
		Uint32 timestamp;

		Message(string& pMessage);
		Message(): Message(string()) {}
		~Message();
	};

	static SDL_RWops* file;
	static bool queueLogMessages;
	static thread_local Message* currentWritableMessage;
	static thread_local CircularStateQueue<Message>* currentThreadLogQueue;
	static CircularStateQueue<Message>* mainLogQueue;
	static CircularStateQueue<Message>* renderLogQueue;
	static bool threadRunning;
	static thread* logThread;

public:
	static void beginLogging();
	static void beginMultiThreadedLogging();
private:
	static void logLoop();
public:
	static void setupLoggingForRenderThread();
	static void log(const char* message);
	static void logString(string& message);
public:
	static void endMultiThreadedLogging();
	static void endLogging();
};
