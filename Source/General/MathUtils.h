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
	static constexpr double sqrtConst(double f) {
		//Newton's method
		double lastGuess = 0;
		double guess = f / 2;
		while (guess != lastGuess) {
			lastGuess = guess;
			guess = (guess + f / guess) / 2;
		}
		return guess;
	}
	static constexpr double powConst(double val, double exp) {
		bool inverse = exp < 0;
		if (inverse)
			exp = -exp;
		double result = 1;
		for (; exp >= 1; exp -= 1)
			result *= val;
		double mult = val;
		while (exp > 0) {
			mult = sqrtConst(mult);
			exp *= 2;
			if (exp >= 1) {
				exp -= 1;
				result *= mult;
			}
		}
		return inverse ? 1 / result : result;
	}
};
