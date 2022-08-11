#include "StringUtils.h"

//returns whether the first string starts with the second string
bool StringUtils::startsWith(const string& s, const string& prefix) {
	return s.compare(0, prefix.size(), prefix) == 0;
}
//skip any non-digit characters, and then get the integer value at that position (or 0 if we reached the end of the string) and
//	write it to outValue
//returns the string at the first position after the parsed int, or the end of the string
const char* StringUtils::parseNextInt(const char* s, int* outValue) {
	char c;
	for (; true; s++) {
		c = *s;
		if (c == 0) {
			*outValue = 0;
			return s;
		} else if (c >= '0' && c <= '9')
			break;
	}

	int val = 0;
	do {
		val = val * 10 + (int)(c - '0');
		s++;
		c = *s;
	} while (c >= '0' && c <= '9');

	*outValue = val;
	return s;
}
//find the 4-10 digits at the beginning of a log line, dropping the leading spaces and decimal point
//returns the string at the first position after the parsed timestamp
const char* StringUtils::parseLogFileTimestamp(const char* s, int* outValue) {
	int seconds;
	int milliseconds;
	s = parseNextInt(s, &seconds);
	s = parseNextInt(s, &milliseconds);
	*outValue = seconds * 1000 + milliseconds;
	return s;
}
//parse the two position values at the beginning of a string, separated by spaces
//returns the string at the first position after the parsed position
const char* StringUtils::parsePosition(const char* s, int* outX, int* outY) {
	s = parseNextInt(s, outX);
	return parseNextInt(s, outY);
}
