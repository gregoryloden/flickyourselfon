#include "General/General.h"

class FileUtils {
private:
	#ifdef __APPLE__
		static const char* fileIoIntermediatePath;
	#endif

public:
	static SDL_Surface* loadImage(const char* imagePath);
	static void openFileForRead(ifstream* file, const char* filePath);
	static void openFileForWrite(ofstream* file, const char* filePath, ios_base::openmode fileFlags);
	#ifdef __APPLE__
		static string getExpandedFileIoPrefix();
	#endif
};
