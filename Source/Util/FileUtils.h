#include "General/General.h"

class FileUtils {
public:
	enum class FileReadLocation: unsigned char {
		Installation,
		ApplicationData,
	};

	#ifdef WIN32
		static const string localAppDataDir;
	#endif
private:
	static const string imagesFolder;

public:
	//Prevent allocation
	FileUtils() = delete;
	//load an image
	static SDL_Surface* loadImage(const char* imagePath);
	//save an image
	static void saveImage(SDL_Surface* image, const char* imagePath);
	//open a file for read
	static void openFileForRead(ifstream* file, const char* filePath, FileReadLocation fileReadLocation);
	//open a file for write
	static void openFileForWrite(ofstream* file, const char* filePath, ios_base::openmode fileFlags);
	#ifdef __APPLE__
		//expand the user's home directory in the file io prefix
		static string getExpandedFileIoPrefix();
	#endif
};
