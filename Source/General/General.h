#ifndef GENERAL_H
#define GENERAL_H
#ifdef WIN32
	#include <SDL.h>
	#include <SDL_image.h>
	#include <SDL_opengl.h>
	#include <gl/GL.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_opengl.h>
	#include <SDL2_image/SDL_image.h>
	#include <OpenGL/OpenGL.h>
#endif
#include <deque>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "MathUtils.h"
#include "MemoryManagement.h"
using namespace std;

#undef RGB

#define COMMA ,
#define HAS_BITMASK(a, b) ((a & b) == b)
#ifdef DEBUG
	#define onlyInDebug(x) x
#else
	#define onlyInDebug(x)
#endif
#endif
