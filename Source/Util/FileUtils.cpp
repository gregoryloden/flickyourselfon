#include "FileUtils.h"
#ifdef __APPLE__
	#include <CoreFoundation/CFBundle.h>
	#include <pwd.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#ifdef WIN32
	const string FileUtils::localAppDataDir = []() {
		size_t bufferSize = MAX_PATH;
		char buffer[MAX_PATH] = {};
		char* bufferAsPointer = buffer;
		_dupenv_s(&bufferAsPointer, &bufferSize, "LOCALAPPDATA");
		return string(bufferAsPointer) + "\\KickYourselfOn";
	}();
#endif
const string FileUtils::imagesFolder = "images/";
SDL_Surface* FileUtils::loadImage(const char *imagePath) {
	#ifdef WIN32
		string fullPath = imagesFolder + imagePath;
		return IMG_Load(fullPath.c_str());
	#else
		CFBundleRef mainBundle = CFBundleGetMainBundle();
		CFStringRef cfImagePath = CFStringCreateWithCString(nullptr, imagePath, kCFStringEncodingUTF8);
		CFURLRef imageUrl = CFBundleCopyResourceURL(mainBundle, cfImagePath, nullptr, nullptr);
		CFStringRef cfFullPath = CFURLCopyFileSystemPath(imageUrl, kCFURLPOSIXPathStyle);
		const char* fullPath = CFStringGetCStringPtr(cfFullPath, kCFStringEncodingUTF8);
		SDL_Surface* image = IMG_Load(fullPath);

		CFRelease(cfFullPath);
		CFRelease(imageUrl);
		CFRelease(cfImagePath);
		return image;
	#endif
}
void FileUtils::saveImage(SDL_Surface* image, const char* imagePath) {
	string fullPath = imagesFolder + imagePath;
	IMG_SavePNG(image, fullPath.c_str());
}
void FileUtils::openFileForRead(ifstream* file, const char* filePath, FileReadLocation fileReadLocation) {
	#ifdef WIN32
		string fullPath;
		if (fileReadLocation == FileReadLocation::Installation)
			fullPath = filePath;
		else if (fileReadLocation == FileReadLocation::ApplicationData)
			fullPath = string(localAppDataDir) + '\\' + filePath;
	#else
		//TODO read from the installation directory
		string fullPath = getExpandedFileIoPrefix() + "/" + filePath;
	#endif
	file->open(fullPath.c_str());
}
void FileUtils::openFileForWrite(ofstream* file, const char* filePath, ios_base::openmode fileFlags) {
	#ifdef WIN32
		string fullPath = string(localAppDataDir) + '\\' + filePath;
	#else
		string expandedPath = getExpandedFileIoPrefix();
		mkdir(expandedPath.c_str(), 0x1FF);
		string fullPath = expandedPath + "/" + filePath;
	#endif
	file->open(fullPath.c_str(), fileFlags);
}
#ifdef __APPLE__
	string FileUtils::getExpandedFileIoPrefix() {
		passwd* p = getpwuid(getuid());
		return string(p->pw_dir) + "/Library/Application Support/kickyourselfon";
	}
#endif
