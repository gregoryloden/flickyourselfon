#include "General/General.h"

class Config {
public:
	//Should only be allocated within an object, on the stack, or as a static object
	class KeyBindings {
	public:
		SDL_Scancode upKey;
		SDL_Scancode rightKey;
		SDL_Scancode downKey;
		SDL_Scancode leftKey;
		SDL_Scancode kickKey;
		SDL_Scancode showConnectionsKey;

		KeyBindings();
		virtual ~KeyBindings();

		void set(const KeyBindings* other);
		static const char* getKeyName(SDL_Scancode key);
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class KickIndicators {
	public:
		bool climb;
		bool fall;
		bool rail;
		bool switch0;
		bool resetSwitch;

		KickIndicators();
		virtual ~KickIndicators();
	};

	static const int gameScreenWidth = 221;
	static const int gameScreenHeight = 165;
	#ifdef EDITOR
	static const int editorMarginRight = 150;
	static const int editorMarginBottom = 60;
	static const int windowScreenWidth = gameScreenWidth + editorMarginRight;
	static const int windowScreenHeight = gameScreenHeight + editorMarginBottom;
	#else
	static const int windowScreenWidth = gameScreenWidth;
	static const int windowScreenHeight = gameScreenHeight;
	#endif
	static const int ticksPerSecond = 1000;
	static const int updatesPerSecond = 48;
	static const float defaultPixelWidth;
	static const float defaultPixelHeight;
	static const float backgroundColorRed;
	static const float backgroundColorGreen;
	static const float backgroundColorBlue;
	static const KeyBindings defaultKeyBindings;
	static const char* optionsFileName;
	static const string upKeyBindingFilePrefix;
	static const string rightKeyBindingFilePrefix;
	static const string downKeyBindingFilePrefix;
	static const string leftKeyBindingFilePrefix;
	static const string kickKeyBindingFilePrefix;
	static const string showConnectionsKeyBindingFilePrefix;
	static const string climbKickIndicatorFilePrefix;
	static const string fallKickIndicatorFilePrefix;
	static const string railKickIndicatorFilePrefix;
	static const string switchKickIndicatorFilePrefix;
	static const string resetSwitchKickIndicatorFilePrefix;

	static float currentPixelWidth;
	static float currentPixelHeight;
	static int refreshRate;
	static KeyBindings keyBindings;
	static KeyBindings editingKeyBindings;
	static KickIndicators kickIndicators;

	static void saveSettings();
	static void loadSettings();
};
