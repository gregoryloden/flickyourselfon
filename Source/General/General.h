#ifndef GENERAL_H
#define GENERAL_H
#ifdef WIN32
	#include <SDL.h>
	#include <SDL_image.h>
	#include <SDL_mixer.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2_image/SDL_image.h>
	#include <SDL2_mixer/SDL_mixer.h>
#endif
#include <array>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <math.h>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "MathUtils.h"
#include "MemoryManagement.h"
#include "Opengl.h"
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
