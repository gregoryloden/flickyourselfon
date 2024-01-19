#include "Logger.h"
#include "Editor/Editor.h"
#include "Util/CircularStateQueue.h"
#include "Util/Config.h"
#include "Util/FileUtils.h"

#define newMessage() newWithoutArgs(Logger::Message)
#define newLogQueueStack(logQueue, next) newWithArgs(Logger::LogQueueStack, logQueue, next)

//////////////////////////////// Logger::Message ////////////////////////////////
Logger::Message::Message(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
message()
, timestamp(0)
, owningLogger(nullptr) {
}
Logger::Message::~Message() {
	//don't delete the logger, it's owned somewhere else
}

//////////////////////////////// Logger::LogQueueStack ////////////////////////////////
Logger::LogQueueStack::LogQueueStack(objCounterParametersComma() CircularStateQueue<Message>* pLogQueue, LogQueueStack* pNext)
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
logQueue(pLogQueue)
, next(pNext) {
}
Logger::LogQueueStack::~LogQueueStack() {
	delete logQueue;
	delete next;
}

//////////////////////////////// Logger::PendingMessage ////////////////////////////////
Logger::PendingMessage::PendingMessage(stringstream* pMessage, Logger* pOwningLogger, PendingMessage* pNext)
: message(pMessage)
, owningLogger(pOwningLogger)
, next(pNext) {
}
Logger::PendingMessage::~PendingMessage() {
	//don't delete the logger or the next pending message, they're owned somewhere else
}

//////////////////////////////// Logger ////////////////////////////////
thread_local CircularStateQueue<Logger::Message>* Logger::currentThreadLogQueue = nullptr;
thread_local const char* Logger::currentThreadTagPrefix = " ";
thread_local Logger::PendingMessage* Logger::currentPendingMessage = nullptr;
bool Logger::threadRunning = false;
thread* Logger::logThread = nullptr;
Logger::LogQueueStack* Logger::logQueueStack = nullptr;
vector<Logger*> Logger::loggers;
Logger Logger::debugLogger ("kyo_debug.log", ios::out | ios::trunc);
Logger Logger::gameplayLogger ("kyo_gameplay.log", ios::out | ios::app);
Logger::Logger(const char* pFileName, ios_base::openmode pFileFlags)
: fileName(pFileName)
, file(nullptr)
, fileFlags(pFileFlags)
, preQueueMessages()
, messagesToWrite()
, hasMessagesToWrite(false)
, editorEnabled(false) {
}
Logger::~Logger() {
	//don't delete the file, assume it was already closed
}
void Logger::beginLogging() {
	if (Editor::isActive && !editorEnabled)
		return;
	file = new ofstream();
	FileUtils::openFileForWrite(file, fileName, fileFlags);
	loggers.push_back(this);
}
void Logger::beginMultiThreadedLogging() {
	threadRunning = true;
	logThread = new thread(logLoop);
}
void Logger::setupLogQueue(const char* threadTagPrefix) {
	currentThreadTagPrefix = threadTagPrefix;

	//create our new objects, these may cause logs so don't assign them to the static values yet
	int preQueueTimestamp = (int)SDL_GetTicks();
	CircularStateQueue<Message>* newThreadLogQueue = newCircularStateQueue(Message, newMessage(), newMessage());
	LogQueueStack* newLogQueueStack = newLogQueueStack(newThreadLogQueue, logQueueStack);

	//add states until we have at least one per logger, enough that we know we won't be adding any messages
	for (int statesNeeded = (int)loggers.size() - newThreadLogQueue->getStatesCount(); statesNeeded > 0; statesNeeded--)
		currentThreadLogQueue->addWritableState(newMessage());

	//now set the queue values and write the messages because we know we won't create any new objects/debug logs
	currentThreadLogQueue = newThreadLogQueue;
	logQueueStack = currentThreadLogQueueStack;
	for (Logger* logger : loggers) {
		logger->queueMessage(&logger->preQueueMessages, preQueueTimestamp);
		logger->preQueueMessages.str(string());
	}
}
void Logger::endMultiThreadedLogging() {
	threadRunning = false;
	logThread->join();
	delete logThread;
	//run the log loop once more to make sure all the queued messages are written
	logLoop();
	currentThreadLogQueue = nullptr;
	delete logQueueStack;
	logQueueStack = nullptr;
}
void Logger::endLogging() {
	if (Editor::isActive && !editorEnabled)
		return;
	for (int i = 0; i < (int)loggers.size(); i++) {
		if (loggers[i] == this) {
			loggers.erase(loggers.begin() + i);
			break;
		}
	}
	file->close();
	delete file;
	file = nullptr;
}
void Logger::logLoop() {
	Logger* lastWrittenLogger = &debugLogger;
	while (true) {
		//group all messages into their appropriate logger
		while (true) {
			Message* oldestMessage = nullptr;
			CircularStateQueue<Message>* oldestMessageLogQueue = nullptr;
			for (LogQueueStack* checkLogQueueStack = logQueueStack;
				checkLogQueueStack != nullptr;
				checkLogQueueStack = checkLogQueueStack->next)
			{
				CircularStateQueue<Message>* logQueue = checkLogQueueStack->logQueue;
				Message* message = logQueue->getNextReadableState();
				if (message != nullptr && (oldestMessage == nullptr || message->timestamp <= oldestMessage->timestamp)) {
					oldestMessage = message;
					oldestMessageLogQueue = logQueue;
				}
			}

			if (oldestMessage == nullptr)
				break;

			oldestMessage->owningLogger->messagesToWrite << oldestMessage->message;
			oldestMessage->message.clear();
			oldestMessage->owningLogger->hasMessagesToWrite = true;
			oldestMessageLogQueue->finishReadingFromState();
		}

		//write messages from their logger to the file
		if (lastWrittenLogger->hasMessagesToWrite)
			lastWrittenLogger->writePendingMessages();
		for (Logger* logger : loggers) {
			if (!logger->hasMessagesToWrite)
				continue;
			logger->writePendingMessages();
			lastWrittenLogger = logger;
		}

		if (!threadRunning)
			return;
		SDL_Delay(1);
	}
}
void Logger::log(const char* message) {
	logString(string(message));
}
void Logger::logString(const string& message) {
	if (Editor::isActive && !editorEnabled)
		return;
	int timestamp = (int)SDL_GetTicks();
	stringstream messageWithTimestamp;
	messageWithTimestamp
		<< setw(7) << setfill(' ') << (timestamp / Config::ticksPerSecond)
		<< setw(1) << '.'
		<< setw(3) << setfill('0') << (timestamp % Config::ticksPerSecond)
		<< setw(1) << ' ' << currentThreadTagPrefix << "  " << message << '\n';

	//we might get logs before we've initialized our log queue
	if (currentThreadLogQueue == nullptr) {
		//if the log thread is running, add this message to the pre-queue messages
		if (threadRunning)
			preQueueMessages << messageWithTimestamp.str();
		//if not, write directly to the file
		//any messages written before the file has been opened or after it has been closed will be simply discarded (ex
		//	ObjCounter messages for statically allocated objects)
		else if (file != nullptr)
			*file << messageWithTimestamp.str();
		return;
	}

	//if we're in the middle of writing to this logger already, append to the pending message
	for (PendingMessage* checkPendingMessage = currentPendingMessage;
		checkPendingMessage != nullptr;
		checkPendingMessage = checkPendingMessage->next)
	{
		if (checkPendingMessage->owningLogger == this) {
			*(checkPendingMessage->message) << messageWithTimestamp.str();
			return;
		}
	}

	queueMessage(&messageWithTimestamp, timestamp);
}
void Logger::queueMessage(stringstream* message, int timestamp) {
	Message* writableMessage = currentThreadLogQueue->getNextWritableState();
	//if we don't have any matching pending messages and we don't have any writable messages, make a new one
	if (writableMessage == nullptr) {
		PendingMessage pendingMessage (message, this, currentPendingMessage);
		currentPendingMessage = &pendingMessage;
		writableMessage = newMessage();
		currentThreadLogQueue->addWritableState(writableMessage);
		currentPendingMessage = nullptr;
	}
	writableMessage->message = message->str();
	writableMessage->timestamp = timestamp;
	writableMessage->owningLogger = this;
	currentThreadLogQueue->finishWritingToState();
}
void Logger::writePendingMessages() {
	*file << messagesToWrite.str();
	messagesToWrite.str(string());
	hasMessagesToWrite = false;
}
