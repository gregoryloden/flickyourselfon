#include "General/General.h"

class StringUtils {
public:
	static bool startsWith(const string& s, const string& prefix);
	static const char* parseNextInt(const char* s, int* outValue);
	static const char* parseLogFileTimestamp(const char* s, int* outValue);
	static const char* parsePosition(const char* s, int* outX, int* outY);
};
