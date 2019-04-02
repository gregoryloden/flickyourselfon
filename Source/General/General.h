#ifndef GENERAL_H
#define GENERAL_H
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl/GL.h>
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
#ifdef DEBUG
	#define onlyInDebug(x) x
	//**/#define EDITOR 1
#else
	#define onlyInDebug(x)
#endif
#endif
