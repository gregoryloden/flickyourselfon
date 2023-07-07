#include "General/General.h"

template <class Type> class CircularStateQueue;

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
	bool editorEnabled;

	Logger(const char* pFileName, ios_base::openmode pFileFlags);
	virtual ~Logger();

public:
	void enableInEditor() { editorEnabled = true; }
	//open the file for writing on a single thread and add this to the list of loggers
	void beginLogging();
	//start the logging thread
	//this should only be called on the main thread
	static void beginMultiThreadedLogging();
	//create the log queue for this thread
	//does not lock- callers are responsible for ensuring only one thread runs this at a time
	static void setupLogQueue();
	//stop the logging thread and delete the queues, but leave the files open for single threaded logging
	static void endMultiThreadedLogging();
	//finally close the file and remove this from the list of loggers
	void endLogging();
private:
	//loop and process any log messages that were written in another thread
	static void logLoop();
public:
	//log a message to the current thread's log queue
	void log(const char* message);
	//log a message to the current thread's log queue
	void logString(const string& message);
private:
	//add a message to the queue, creating a new Message if necessary
	void queueMessage(stringstream* message, int timestamp);
	//write any pending messages to the file
	void writePendingMessages();
};
