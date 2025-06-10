#include "General.h"
#include "Util/Config.h"
#include "Util/Logger.h"

#define declareGlFunctionSource(name) name##_Type name;
#define assignAndVerifyGlFunction(name) \
	name = (name##_Type)SDL_GL_GetProcAddress(#name);\
	if (name == nullptr)\
		return false;

declareGlFunctionSource(glGenFramebuffers);
declareGlFunctionSource(glBindFramebuffer);
declareGlFunctionSource(glGenRenderbuffers);
declareGlFunctionSource(glBindRenderbuffer);
declareGlFunctionSource(glRenderbufferStorage);
declareGlFunctionSource(glFramebufferRenderbuffer);
declareGlFunctionSource(glBlitFramebuffer);
declareGlFunctionSource(glFramebufferTexture2D);
#ifdef DEBUG
	declareGlFunctionSource(glCheckFramebufferStatus);
#endif

bool Opengl::initExtensions() {
	assignAndVerifyGlFunction(glGenFramebuffers);
	assignAndVerifyGlFunction(glBindFramebuffer);
	assignAndVerifyGlFunction(glGenRenderbuffers);
	assignAndVerifyGlFunction(glBindRenderbuffer);
	assignAndVerifyGlFunction(glRenderbufferStorage);
	assignAndVerifyGlFunction(glFramebufferRenderbuffer);
	assignAndVerifyGlFunction(glBlitFramebuffer);
	assignAndVerifyGlFunction(glFramebufferTexture2D);
	#ifdef DEBUG
		assignAndVerifyGlFunction(glCheckFramebufferStatus);
	#endif
	return true;
}
void Opengl::orientRenderTarget(bool topDown) {
	glLoadIdentity();
	if (topDown)
		glOrtho(0, (GLdouble)Config::windowScreenWidth, (GLdouble)Config::windowScreenHeight, 0, -1, 1);
	else
		glOrtho(0, (GLdouble)Config::windowScreenWidth, 0, (GLdouble)Config::windowScreenHeight, -1, 1);
}
void Opengl::clearBackground() {
	glClearColor(Config::backgroundColorRed, Config::backgroundColorGreen, Config::backgroundColorBlue, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
#ifdef DEBUG
	void Opengl::checkAndLogFrameBufferStatus(GLenum target, const char* frameBufferName) {
		GLenum frameBufferStatus = glCheckFramebufferStatus(target);
		if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE) {
			GLenum error = glGetError();
			stringstream message;
			message << "ERROR: " << frameBufferName << " not complete: status 0x" << hex << frameBufferStatus
				<< " error 0x" << error;
			Logger::debugLogger.logString(message.str());
		}
	}
#endif
