#include "TimeUtils.h"

void TimeUtils::appendTimestamp(stringstream* message) {
	char timestamp [sizeof("2000-01-01 00:00:00")];
	time_t now = time(nullptr);
	tm nowTm;
	#ifdef WIN32
		localtime_s(&nowTm, &now);
	#else
		localtime_r(&now, &nowTm);
	#endif
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &nowTm);
	*message << timestamp;
}
