#include "FileUtils.h"
#ifdef __APPLE__
	#include <CoreFoundation/CFBundle.h>
	#include <sys/stat.h>
	#include <pwd.h>
	#include <unistd.h>
#endif

#ifdef __APPLE__
	const char* FileUtils::fileIoIntermediatePath = "/Library/Application Support/flickyourselfon";
#endif
//load an image
SDL_Surface* FileUtils::loadImage(const char *imagePath) {
	#ifdef WIN32
		string fullPath = string("images/") + imagePath;
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
//open a file for read
void FileUtils::openFileForRead(ifstream* file, const char* filePath) {
	#ifdef WIN32
		file->open(filePath);
	#else
		string fullPath = getExpandedFileIoPrefix() + "/" + filePath;
		file->open(fullPath.c_str());
	#endif
}
//open a file for write
void FileUtils::openFileForWrite(ofstream* file, const char* filePath, ios_base::openmode fileFlags) {
	#ifdef WIN32
		file->open(filePath, fileFlags);
	#else
		string expandedPath = getExpandedFileIoPrefix();
		mkdir(expandedPath.c_str(), 0x1FF);
		string fullPath = expandedPath + "/" + filePath;
		file->open(fullPath.c_str(), fileFlags);
	#endif
}
#ifdef __APPLE__
	//expand the user's home directory in the file io prefix
	string FileUtils::getExpandedFileIoPrefix() {
		passwd* p = getpwuid(getuid());
		return string(p->pw_dir) + fileIoIntermediatePath;
	}
#endif
