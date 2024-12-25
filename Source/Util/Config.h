#ifndef CONFIG_H
#define CONFIG_H
#include "General/General.h"

namespace ConfigTypes {
	//Should only be allocated within an object, on the stack, or as a static object
	class KeyBindingSetting {
	public:
		const SDL_Scancode defaultValue;
		SDL_Scancode value;
		SDL_Scancode editingValue;
		const string filePrefix;

		KeyBindingSetting(SDL_Scancode pDefaultValue, string pFilePrefix, vector<KeyBindingSetting*>& containingList);
		virtual ~KeyBindingSetting();

		//get the name for this key code
		static const char* getKeyName(SDL_Scancode key);
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class MultiStateSetting {
	public:
		const vector<string> options;
		int state;
		const string filePrefix;

		MultiStateSetting(vector<string> pOptions, string pFilePrefix, vector<MultiStateSetting*>& containingList);
		virtual ~MultiStateSetting();

		string getSelectedOption() { return options[state]; }
		void cycleState(int increment) { state = (state + options.size() + increment) % options.size(); }
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class OnOffSetting: public MultiStateSetting {
	public:
		OnOffSetting(string pFilePrefix, vector<MultiStateSetting*>& containingList);
		virtual ~OnOffSetting();

		bool isOn() { return state == 0; }
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class HoldToggleSetting: public MultiStateSetting {
	public:
		HoldToggleSetting(string pFilePrefix, vector<MultiStateSetting*>& containingList);
		virtual ~HoldToggleSetting();

		bool isHold() { return state == 0; }
	};
	//Should only be allocated within an object, on the stack, or as a static object
	class VolumeSetting {
	public:
		static const int maxVolume = 32;
		static const int defaultVolume = maxVolume / 2;

		int volume;
		const string filePrefix;

		VolumeSetting(string pFilePrefix, vector<VolumeSetting*>& containingList);
		virtual ~VolumeSetting();
	};
}
class Config {
public:
	static const int gameScreenWidth = 221;
	static const int gameScreenHeight = 165;
	static const int editorMarginRight = 150;
	static const int editorMarginBottom = 60;
	static const int ticksPerSecond = 1000;
	static const int updatesPerSecond = 48;
	static constexpr float defaultPixelWidth = 4.0f;
	static constexpr float defaultPixelHeight = 4.0f;
	static constexpr float editorDefaultPixelWidth = 3.0f;
	static constexpr float editorDefaultPixelHeight = 3.0f;
	static constexpr float backgroundColorRed = 3.0f / 16.0f;
	static constexpr float backgroundColorGreen = 0.0f;
	static constexpr float backgroundColorBlue = 3.0f / 16.0f;
	static constexpr char* optionsFileName = "kyo.options";
	static const int heightBasedShadingOffValue = 1;
	static const int heightBasedShadingExtraValue = 2;

	static float currentPixelWidth;
	static float currentPixelHeight;
	static int windowScreenWidth;
	static int windowScreenHeight;
	static int refreshRate;
	static vector<ConfigTypes::KeyBindingSetting*> allKeyBindingSettings;
	static ConfigTypes::KeyBindingSetting upKeyBinding;
	static ConfigTypes::KeyBindingSetting rightKeyBinding;
	static ConfigTypes::KeyBindingSetting downKeyBinding;
	static ConfigTypes::KeyBindingSetting leftKeyBinding;
	static ConfigTypes::KeyBindingSetting kickKeyBinding;
	static ConfigTypes::KeyBindingSetting sprintKeyBinding;
	static ConfigTypes::KeyBindingSetting undoKeyBinding;
	static ConfigTypes::KeyBindingSetting redoKeyBinding;
	static ConfigTypes::KeyBindingSetting showConnectionsKeyBinding;
	static ConfigTypes::KeyBindingSetting mapCameraKeyBinding;
	static vector<ConfigTypes::MultiStateSetting*> allMultiStateSettings;
	static ConfigTypes::OnOffSetting climbKickIndicator;
	static ConfigTypes::OnOffSetting fallKickIndicator;
	static ConfigTypes::OnOffSetting railKickIndicator;
	static ConfigTypes::OnOffSetting switchKickIndicator;
	static ConfigTypes::OnOffSetting resetSwitchKickIndicator;
	static ConfigTypes::HoldToggleSetting showConnectionsMode;
	static ConfigTypes::MultiStateSetting heightBasedShading;
	static ConfigTypes::OnOffSetting showActivatedSwitchWaves;
	static ConfigTypes::OnOffSetting showBlockedFallEdges;
	static vector<ConfigTypes::VolumeSetting*> allVolumeSettings;
	static ConfigTypes::VolumeSetting masterVolume;
	static ConfigTypes::VolumeSetting musicVolume;
	static ConfigTypes::VolumeSetting soundsVolume;

	//Prevent allocation
	Config() = delete;
	//save the key bindings to the save file
	static void saveSettings();
	//load the key bindings from the save file, if it exists
	static void loadSettings();
private:
	//load a keybinding setting, if applicable
	static bool loadKeyBindingSetting(string& line);
	//load a multi-state setting, if applicable
	static bool loadMultiStateSetting(string& line);
	//load a volume setting, if applicable
	static bool loadVolumeSetting(string& line);
};
#endif
