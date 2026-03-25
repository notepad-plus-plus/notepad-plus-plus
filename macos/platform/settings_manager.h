#pragma once
// Settings Manager for Notepad++ macOS Port
// Reads/writes settings to ~/.npp-macos/settings.json using NSJSONSerialization.

#include <string>
#include <vector>

struct AppSettings
{
	// Window geometry
	double windowX = 100;
	double windowY = 100;
	double windowWidth = 1000;
	double windowHeight = 750;

	// Editor preferences
	std::string fontName = "Menlo";
	int fontSize = 13;
	int tabWidth = 4;

	// View state
	bool wordWrap = false;
	bool showLineNumbers = true;
	int zoomLevel = 0;
	bool showCaretLine = true;
	bool autoIndent = true;
	bool autoCloseBrackets = true;
	bool useTabs = false;
	bool showWhitespace = false;
	bool showEol = false;
	bool showIndentGuides = false;
	bool syncScrolling = false;
	bool documentMap = false;
	bool showChangeHistory = true;
	int documentMapWidth = 140;

	// Recent files
	std::vector<std::string> recentFiles;
};

class SettingsManager
{
public:
	static SettingsManager& instance();

	// Load settings from ~/.npp-macos/settings.json
	// Returns true if file was found and parsed successfully.
	bool load();

	// Save current settings to ~/.npp-macos/settings.json
	bool save();

	AppSettings settings;

private:
	SettingsManager() = default;
	std::string settingsDir() const;
	std::string settingsPath() const;
};
