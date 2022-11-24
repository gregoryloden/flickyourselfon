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

		//copy the key bindings from the other KeyBindings
		void set(const KeyBindings* other);
		//get the name for this key code
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
	static const int editorMarginRight = 150;
	static const int editorMarginBottom = 60;
	static const int ticksPerSecond = 1000;
	static const int updatesPerSecond = 48;
	static const float defaultPixelWidth;
	static const float defaultPixelHeight;
	static const float editorDefaultPixelWidth;
	static const float editorDefaultPixelHeight;
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
	static int windowScreenWidth;
	static int windowScreenHeight;
	static int refreshRate;
	static KeyBindings keyBindings;
	static KeyBindings editingKeyBindings;
	static KickIndicators kickIndicators;

	//save the key bindings to the save file
	static void saveSettings();
	//load the key bindings from the save file, if it exists
	static void loadSettings();
};
