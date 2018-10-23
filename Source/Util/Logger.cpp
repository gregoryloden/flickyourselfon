#include "Logger.h"
#include "Util/CircularStateQueue.h"

Logger::Message::Message(objCounterParameters())
: onlyInDebug(ObjCounter(objCounterArguments()) COMMA)
message()
, timestamp(0) {
}
Logger::Message::~Message() {}
SDL_RWops* Logger::file = nullptr;
bool Logger::queueLogMessages = false;
thread_local stringstream* Logger::currentMessageStringstream = nullptr;
thread_local CircularStateQueue<Logger::Message>* Logger::currentThreadLogQueue = nullptr;
CircularStateQueue<Logger::Message>* Logger::mainLogQueue = nullptr;
CircularStateQueue<Logger::Message>* Logger::renderLogQueue = nullptr;
bool Logger::threadRunning = false;
thread* Logger::logThread = nullptr;
//open the file for writing on a single thread
void Logger::beginLogging() {
	file = SDL_RWFromFile("fyo_log.txt", "w");
	queueLogMessages = false;
}
//build the log queues and start the logging thread
//this should only be called on the main thread
void Logger::beginMultiThreadedLogging() {
	mainLogQueue = newWithArgs(CircularStateQueue<Message>, newWithoutArgs(Message), newWithoutArgs(Message));
	renderLogQueue = newWithArgs(CircularStateQueue<Message>, newWithoutArgs(Message), newWithoutArgs(Message));
	currentThreadLogQueue = mainLogQueue;
	queueLogMessages = true;
	threadRunning = true;
	logThread = new thread(logLoop);
}
//loop and process any log messages that were written in another thread
void Logger::logLoop() {
	while (true) {
		Message* mainLogMessage = mainLogQueue->getNextReadableState();
		Message* renderLogMessage = renderLogQueue->getNextReadableState();
		while (true) {
			if (mainLogMessage != nullptr
				&& (renderLogMessage == nullptr || mainLogMessage->timestamp <= renderLogMessage->timestamp))
			{
				SDL_RWwrite(file, mainLogMessage->message.c_str(), 1, mainLogMessage->message.length());
				mainLogMessage->message.clear();
				mainLogQueue->finishReadingFromState();
				mainLogMessage = mainLogQueue->getNextReadableState();
			} else if (renderLogMessage != nullptr) {
				SDL_RWwrite(file, renderLogMessage->message.c_str(), 1, renderLogMessage->message.length());
				renderLogMessage->message.clear();
				renderLogQueue->finishReadingFromState();
				renderLogMessage = renderLogQueue->getNextReadableState();
			} else
				break;
		}
		if (!threadRunning)
			return;
		else
			SDL_Delay(1);
	}
}
//use the render log queue for the current thread
void Logger::setupLoggingForRenderThread() {
	currentThreadLogQueue = renderLogQueue;
}
//log a message to the current thread's log queue
void Logger::log(const char* message) {
	logString(string(message));
}
//log a message to the current thread's log queue
void Logger::logString(string& message) {
	int timestamp = (int)SDL_GetTicks();
	stringstream messageWithTimestamp;
	messageWithTimestamp
		<< setw(7) << setfill(' ') << (timestamp / 1000)
		<< setw(1) << '.'
		<< setw(3) << setfill('0') << (timestamp % 1000)
		<< setw(1) << "  " << message << '\n';

	//we might get logs before we've had time to set up our log queue
	//if that happens, write directly to the file
	//we don't have to worry about multithreading since this should only happen when only one thread is running
	if (!queueLogMessages) {
		string messageWithTimestampString = messageWithTimestamp.str();
		SDL_RWwrite(file, messageWithTimestampString.c_str(), 1, messageWithTimestampString.length());
		return;
	//if we're logging this in the middle of logging something else, append this to our other message
	} else if (currentMessageStringstream != nullptr) {
		*currentMessageStringstream << messageWithTimestamp.str();
		return;
	}

	currentMessageStringstream = &messageWithTimestamp;
	Message* writableMessage = currentThreadLogQueue->getNextWritableState();
	if (writableMessage == nullptr) {
		writableMessage = newWithoutArgs(Message);
		currentThreadLogQueue->addWritableState(writableMessage);
	}
	writableMessage->message = messageWithTimestamp.str();
	writableMessage->timestamp = timestamp;
	currentThreadLogQueue->finishWritingToState();
	currentMessageStringstream = nullptr;
}
//stop the logging thread and delete the queues, but leave the file open for single threaded logging
void Logger::endMultiThreadedLogging() {
	threadRunning = false;
	logThread->join();
	delete logThread;
	queueLogMessages = false;
	//run the log loop once more to make sure all the queued messages are written
	logLoop();
	delete mainLogQueue;
	delete renderLogQueue;
}
//finally close the file
void Logger::endLogging() {
	SDL_RWclose(file);
}
