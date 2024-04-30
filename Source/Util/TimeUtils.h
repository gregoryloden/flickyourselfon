#include "General/General.h"

class TimeUtils {
public:
	//Prevent allocation
	TimeUtils() = delete;
	//write a date-and-time timestamp to the stream
	static void appendTimestamp(stringstream* message);
};
