#include "General/General.h"

class StringUtils {
public:
	//returns whether the first string starts with the second string
	static bool startsWith(const string& s, const string& prefix);
	//skip any non-digit characters, and then get the integer value at that position (or 0 if we reached the end of the string)
	//	and write it to outValue
	//returns the string at the first position after the parsed int, or the end of the string
	static const char* parseNextInt(const char* s, int* outValue);
	//find the 4-10 digits at the beginning of a log line, dropping the leading spaces and decimal point
	//returns the string at the first position after the parsed timestamp
	static const char* parseLogFileTimestamp(const char* s, int* outValue);
	//parse the two position values at the beginning of a string, separated by spaces
	//returns the string at the first position after the parsed position
	static const char* parsePosition(const char* s, int* outX, int* outY);
};
