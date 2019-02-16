#include "StringUtils.h"

bool StringUtils::startsWith(const string& s, const string& prefix) {
	return s.compare(0, prefix.size(), prefix) == 0;
}
