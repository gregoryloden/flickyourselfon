#include "Logger.h"
#include "CircularStateQueue.h"

Logger::Message::Message(string& pMessage)
: message(pMessage)
, timestamp(SDL_GetTicks()) {
}
Logger::Message::~Message() {}
SDL_RWops* Logger::file = nullptr;
bool Logger::queueLogMessages = false;
thread_local Logger::Message* Logger::currentWritableMessage = nullptr;
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
	mainLogQueue = newTracked(CircularStateQueue<Message>, (new Message(), new Message()));
	renderLogQueue = newTracked(CircularStateQueue<Message>, (new Message(), new Message()));
	mainLogQueue->finishReadingFromState();
	renderLogQueue->finishReadingFromState();
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
	//we might get logs before we've had time to set up our log queue
	//if that happens, write directly to the file
	//we don't have to worry about multithreading since this should only happen when only one thread is running
	if (!queueLogMessages) {
		SDL_RWwrite(file, message.c_str(), 1, message.length());
		return;
	//if we're logging this in the middle of logging something else, append this to our other message
	} else if (currentWritableMessage != nullptr) {
		currentWritableMessage->message.append(message);
		return;
	}

	currentWritableMessage = currentThreadLogQueue->getNextWritableState();
	if (currentWritableMessage == nullptr) {
		currentWritableMessage = new Message(message);
		currentThreadLogQueue->addWritableState(currentWritableMessage);
	} else
		currentWritableMessage->message.assign(message);
	currentThreadLogQueue->finishWritingToState();
	currentWritableMessage = nullptr;
}
//stop the logging thread and delete the queues, but leave the file open for single threaded logging
void Logger::endMultiThreadedLogging() {
	threadRunning = false;
	logThread->join();
	delete logThread;
	logThread = nullptr;
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
