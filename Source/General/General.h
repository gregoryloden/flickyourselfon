#ifndef GENERAL_H
#define GENERAL_H
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <gl\GL.h>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "Config.h"
#include "FYOMath.h"
#include "MemoryManagement.h"
using namespace std;

#define COMMA ,
#ifdef DEBUG
	#define onlyInDebug(x) x
#else
	#define onlyInDebug(x)
#endif
#endif
