// macro_manager.h — Macro recording, playback, and persistence
// Part of the Notepad++ macOS port.

#pragma once
#include <string>
#include <vector>

struct MacroStep
{
	int message;
	uintptr_t wParam;
	intptr_t lParam;
	std::string text; // Copied string payload (when lParam is a string pointer)
};

class MacroManager
{
public:
	static MacroManager& instance();

	// Recording
	void startRecording(void* sci);
	void stopRecording(void* sci);
	bool isRecording() const;
	void recordStep(int message, uintptr_t wParam, intptr_t lParam);

	// Playback
	void playback(void* sci);
	void playbackMultiple(void* sci, int count);
	bool hasRecordedMacro() const;

	// Persistence
	void saveMacro(const std::string& name);
	void loadMacro(const std::string& name);
	std::vector<std::string> savedMacroNames() const;

private:
	MacroManager() = default;
	bool _recording = false;
	std::vector<MacroStep> _currentMacro;

	std::string macrosDir() const;
	std::string macrosPath() const;
};
