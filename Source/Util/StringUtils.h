#include "General/General.h"

class StringUtils {
public:
	static bool startsWith(const string& s, const string& prefix);
	static int nonDigitPrefixLength(const char* s);
	static int parseNextInt(const char* s, int* outValue);
	static int parseLogFileTimestamp(const char* s);
	static void parsePosition(const char* s, int* outX, int* outY);
};
