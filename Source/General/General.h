#ifndef GENERAL_H
#define GENERAL_H
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include <sstream>
#include <string>
#include <thread>
#include "MemoryManagement.h"
using namespace std;

#define COMMA ,
#ifdef DEBUG
	#define onlyInDebug(x) x
#else
	#define onlyInDebug(x)
#endif
#endif
