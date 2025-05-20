#include "General/General.h"

class VectorUtils {
public:
	//Prevent allocation
	VectorUtils() = delete;
	//returns the index of the given item if found, or the size of the vector if not
	template <class Type> static unsigned int indexOf(vector<Type>& v, Type t) {
		return (unsigned int)(std::find(v.begin(), v.end(), t) - v.begin());
	}
	//check if the vector includes the given item
	template <class Type> static bool includes(vector<Type>& v, Type t) {
		return std::find(v.begin(), v.end(), t) != v.end();
	}
	//check if any item of the vector matches the given check
	template <class Type, class Check> static bool anyMatch(vector<Type>& v, Check check) {
		return std::find_if(v.begin(), v.end(), check) != v.end();
	}
};
