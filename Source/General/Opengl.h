#ifdef WIN32
	#include <SDL_opengl.h>
#else
	#include <SDL2/SDL_opengl.h>
#endif

#define declareGlFunction(returnType, params, name) \
	typedef returnType(GLAPIENTRY* name##_Type)params;\
	extern name##_Type name;

declareGlFunction(void, (GLsizei n, GLuint* framebuffers), glGenFramebuffers);
declareGlFunction(void, (GLenum target, GLuint framebuffer), glBindFramebuffer);
declareGlFunction(void, (GLsizei n, GLuint* renderbuffers), glGenRenderbuffers);
declareGlFunction(void, (GLenum target, GLuint renderbuffer), glBindRenderbuffer);
declareGlFunction(void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height), glRenderbufferStorage);
declareGlFunction(
	void, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer), glFramebufferRenderbuffer);
declareGlFunction(
	void,
	(
		GLint srcX0,
		GLint srcY0,
		GLint srcX1,
		GLint srcY1,
		GLint dstX0,
		GLint dstY0,
		GLint dstX1,
		GLint dstY1,
		GLbitfield mask,
		GLenum filter
	),
	glBlitFramebuffer);
declareGlFunction(
	void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level), glFramebufferTexture2D);
#ifdef DEBUG
	declareGlFunction(GLenum, (GLenum target), glCheckFramebufferStatus);
#endif

class Opengl {
public:
	//Prevent allocation
	Opengl() = delete;
	//init all OpenGL extensions
	//returns false if any failed
	static bool initExtensions();
	//clear the background with a solid color
	static void clearBackground();
	#ifdef DEBUG
		//check that the given frame buffer is complete
		static void checkAndLogFrameBufferStatus(GLenum target, const char* frameBufferName);
	#endif
};
