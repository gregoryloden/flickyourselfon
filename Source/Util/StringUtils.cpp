#include "StringUtils.h"

//returns whether the first string starts with the second string
bool StringUtils::startsWith(const string& s, const string& prefix) {
	return s.compare(0, prefix.size(), prefix) == 0;
}
//returns the number of non-digit characters at the start of s
int StringUtils::nonDigitPrefixLength(const char* s) {
	for (int i = 0; true; i++) {
		char c = s[i];
		if (c >= '0' && c <= '9')
			return i;
	}
}
//get the integer value at the start of the string (or 0 if there is none) and write it to outValue
//returns the number of characters parsed
int StringUtils::parseNextInt(const char* s, int* outValue) {
	int val = 0;
	for (int i = 0; true; i++) {
		char c = s[i];
		if (c >= '0' && c <= '9')
			val = val * 10 + (int)(c - '0');
		else {
			*outValue = val;
			return i;
		}
	}
}
//find the 4-10 digits at the beginning of a log line, dropping the leading spaces and decimal point
int StringUtils::parseLogFileTimestamp(const char* s) {
	int seconds;
	int milliseconds;
	s = s + nonDigitPrefixLength(s);
	s = s + parseNextInt(s, &seconds);
	parseNextInt(s + 1, &milliseconds);
	return seconds * 1000 + milliseconds;
}
//parse the two position values at the beginning of a string, separated by spaces
void StringUtils::parsePosition(const char* s, int* outX, int* outY) {
	s = s + parseNextInt(s, outX);
	parseNextInt(s + nonDigitPrefixLength(s), outY);
}
