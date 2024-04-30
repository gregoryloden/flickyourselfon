class MathUtils {
public:
	static const float pi;
	static const float twoPi;
	static const float piOver3;

	//Prevent allocation
	MathUtils() = delete;
	static int min(int a, int b) { return a < b ? a : b; }
	static float fmin(float a, float b) { return a < b ? a : b; }
	static int max(int a, int b) { return a > b ? a : b; }
	static float fmax(float a, float b) { return a > b ? a : b; }
	static float fsqr(float a) { return a * a; }
	static constexpr float sqrtConst(float f) {
		//Newton's method
		float lastGuess = 0;
		float guess = f / 2;
		while (guess != lastGuess) {
			lastGuess = guess;
			guess = (guess + f / guess) / 2;
		}
		return guess;
	}
};
