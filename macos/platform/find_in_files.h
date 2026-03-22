// find_in_files.h — Find in Files dialog and search engine
#pragma once
#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#include <atomic>
#include <functional>

struct FIFMatch {
	int lineNumber;       // 1-based
	int column;           // 0-based byte offset within line
	int matchLength;      // bytes
	std::string lineText; // trimmed line content (max 500 chars)
};

struct FIFFileResult {
	std::string filePath;
	std::vector<FIFMatch> matches;
};

struct FIFSearchResult {
	std::string searchTerm;
	std::string directory;
	std::string filters;
	int filesSearched = 0;
	int filesWithMatches = 0;
	int totalMatches = 0;
	std::vector<FIFFileResult> fileResults;
};

// Show the Find in Files dialog
void showFindInFilesDlg();

// Check if a search is currently running
bool isFIFSearchRunning();

// Cancel any running search
void cancelFIFSearch();
