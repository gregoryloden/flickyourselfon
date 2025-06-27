#include "General/General.h"

class VectorUtils {
public:
	//Prevent allocation
	VectorUtils() = delete;
	//returns the number of items in the vector that match the given item
	template <class Type> static int countOf(vector<Type>& v, Type t) {
		return count(v.begin(), v.end(), t);
	}
	//check if the vector includes the given item
	template <class Type> static bool includes(vector<Type>& v, Type t) {
		return find(v.begin(), v.end(), t) != v.end();
	}
	//check if any item of the vector matches the given check
	template <class Type, class Check> static bool anyMatch(vector<Type>& v, Check check) {
		return find_if(v.begin(), v.end(), check) != v.end();
	}
	//remove any items that match the given check
	template <class Type, class Check> static void filterErase(vector<Type>& v, Check check) {
		v.erase(remove_if(v.begin(), v.end(), check), v.end());
	}
	//replace every value with the given value
	template <class Type> static void fill(vector<Type>& v, Type t) {
		v.assign(v.size(), t);
	}
};
