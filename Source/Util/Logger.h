#include "General/General.h"

template <class Type> class CircularStateQueue;
class GameState;

//Should only be allocated within an object, on the stack, or as a static object
class Logger {
private:
	class Message onlyInDebug(: public ObjCounter) {
	public:
		string message;
		int timestamp;
		Logger* owningLogger;

		Message(objCounterParameters());
		virtual ~Message();
	};
	class LogQueueStack onlyInDebug(: public ObjCounter) {
	public:
		CircularStateQueue<Message>* logQueue;
		LogQueueStack* next;

		LogQueueStack(objCounterParametersComma() CircularStateQueue<Message>* pLogQueue, LogQueueStack* pNext);
		virtual ~LogQueueStack();
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class PendingMessage {
	public:
		stringstream* message;
		Logger* owningLogger;
		PendingMessage* next;

		PendingMessage(stringstream* pMessage, Logger* pOwningLogger, PendingMessage* pNext);
		virtual ~PendingMessage();
	};

	static thread_local CircularStateQueue<Message>* currentThreadLogQueue;
	static thread_local PendingMessage* currentPendingMessage;
	static bool threadRunning;
	static thread* logThread;
	static LogQueueStack* logQueueStack;
	static vector<Logger*> loggers;
public:
	static Logger debugLogger;
	static Logger gameplayLogger;

private:
	const char* fileName;
	ios_base::openmode fileFlags;
	ofstream* file;
	stringstream preQueueMessages;
	stringstream messagesToWrite;
	bool hasMessagesToWrite;

	Logger(const char* pFileName, ios_base::openmode pFileFlags);
	virtual ~Logger();

public:
	void beginLogging();
	static void beginMultiThreadedLogging();
	static void setupLogQueue();
	static void endMultiThreadedLogging();
	void endLogging();
private:
	static void logLoop();
public:
	void log(const char* message);
	void logString(const string& message);
private:
	void queueMessage(stringstream* message, int timestamp);
	void writePendingMessages();
};
