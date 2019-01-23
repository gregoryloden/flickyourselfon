class Config {
public:
	static const int gameScreenWidth = 199;
	static const int gameScreenHeight = 149;
	static const int defaultPixelWidth = 3;
	static const int defaultPixelHeight = 3;
	static const float backgroundColorRed;
	static const float backgroundColorGreen;
	static const float backgroundColorBlue;
	static const int ticksPerSecond = 1000;
	static const int updatesPerSecond = 48;

	static int refreshRate;
	static SDL_Scancode kickKey;
	static SDL_Scancode leftKey;
	static SDL_Scancode rightKey;
	static SDL_Scancode upKey;
	static SDL_Scancode downKey;
};
