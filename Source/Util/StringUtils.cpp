#include "StringUtils.h"

bool StringUtils::startsWith(const string& s, const string& prefix) {
	return s.compare(0, prefix.size(), prefix) == 0;
}
const char* StringUtils::parseNextInt(const char* s, int* outValue) {
	int sign = 1;
	char c;
	for (; true; s++) {
		c = *s;
		if (c == 0) {
			*outValue = 0;
			return s;
		} else if (c == '-')
			sign = -1;
		else if (c >= '0' && c <= '9')
			break;
	}

	int val = 0;
	do {
		val = val * 10 + (int)(c - '0');
		s++;
		c = *s;
	} while (c >= '0' && c <= '9');

	*outValue = val * sign;
	return s;
}
const char* StringUtils::parseLogFileTimestamp(const char* s, int* outValue) {
	int seconds;
	int milliseconds;
	s = parseNextInt(s, &seconds);
	s = parseNextInt(s, &milliseconds);
	*outValue = seconds * 1000 + milliseconds;
	return s;
}
const char* StringUtils::parsePosition(const char* s, int* outX, int* outY) {
	s = parseNextInt(s, outX);
	return parseNextInt(s, outY);
}
