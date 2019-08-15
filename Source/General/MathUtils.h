class MathUtils {
public:
	static int min(int a, int b) { return a < b ? a : b; }
	static float fmin(float a, float b) { return a < b ? a : b; }
	static int max(int a, int b) { return a > b ? a : b; }
	static float fmax(float a, float b) { return a > b ? a : b; }
	static float fsqr(float a) { return a * a; }
};
