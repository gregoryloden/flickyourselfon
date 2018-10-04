#ifndef GENERAL_H
#define GENERAL_H
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include "MemoryManagement.h"

#define COMMA ,
#ifdef DEBUG
	#define onlyInDebug(x) x
#else
	#define onlyInDebug(x)
#endif
#endif
