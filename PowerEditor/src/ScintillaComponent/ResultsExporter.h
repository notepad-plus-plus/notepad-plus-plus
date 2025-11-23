#pragma once

#include <string>
#include <vector>
#include <windows.h>

enum class ExportFormat { CSV, JSON, PlainText };

// User can choose different delimiters for CSV flexibility
struct CSV_ExportOptions {
	char delimiter = ',';
	bool includeHeaders = true;
	bool quoteFields = true;
	bool includeFilePaths = true;
	bool includeLineNumbers = true;
};

// Support multiple nesting strategies for better readability in different tools
enum class JSONNesting { Flat, GroupedByFile, GroupedByFileAndLine };
struct JSON_ExportOptions {
	bool prettyPrint = true;
	int indentSize = 2;
	bool includeMetadata = true;
	bool includeFilePaths = true;
	JSONNesting nesting = JSONNesting::GroupedByFile;
};

// Provide multiple styles for readability in different contexts
enum class PlainTextStyle { Compact, Readable, FullDetails };
struct PlainText_ExportOptions {
	bool includeHeaders = true;
	bool includeFilePaths = true;
	bool includeLineNumbers = true;
	PlainTextStyle style = PlainTextStyle::Readable;
};

struct ExportOptions {
	ExportFormat format = ExportFormat::CSV;
	std::wstring outputPath;
	CSV_ExportOptions csvOpts;
	JSON_ExportOptions jsonOpts;
	PlainText_ExportOptions plainTextOpts;
};

struct ExportResult {
	std::wstring _filePath;
	size_t _lineNumber = 0;
	std::wstring _lineContent;
	std::vector<std::pair<intptr_t, intptr_t>> _matchRanges;
	std::wstring _matchedText;
};

// Captures search context for reproducibility in exports
struct SearchMetadata {
	std::wstring _searchQuery;
	std::wstring _replaceQuery;
	std::wstring _searchScope;
	std::wstring _searchDirectory;
	bool _matchCase = false;
	bool _matchWholeWord = false;
	bool _useRegularExpression = false;
	bool _wrapAround = false;
	int _totalMatches = 0;
	int _filesSearched = 0;
	int _filesMatched = 0;
	SYSTEMTIME _timestamp = {};
	std::wstring _notepadppVersion;
};

// Forward declaration of FoundInfo from FindReplaceDlg.h
struct FoundInfo;

class ResultsExporter {
public:
	ResultsExporter(const std::vector<FoundInfo>& foundInfos,
		const std::wstring& searchQuery,
		const std::wstring& searchScope);

	// Main export function - handles file I/O safety checks and writing
	bool exportResults(const ExportOptions& options, std::wstring& errorMsg);

	// Generate formatted content for preview - no file I/O, no safety checks
	bool generateContent(const ExportOptions& options, std::string& content, std::wstring& errorMsg);

private:
	std::vector<FoundInfo> _foundInfos;
	std::wstring _searchQuery;
	std::wstring _searchScope;
	SearchMetadata _metadata;
	std::vector<ExportResult> _results;

	// Helper methods
	void buildMetadata();
	void gatherResults();

	// Format-specific methods
	std::string formatAsCSV(const CSV_ExportOptions& csvOpts);
	std::string formatAsJSON(const JSON_ExportOptions& jsonOpts);
	std::string formatAsPlainText(const PlainText_ExportOptions& plainTextOpts);

	// CSV helpers
	std::string escapeCSVField(const std::string& field, bool alwaysQuote = false);

	// JSON helpers
	std::string jsonEscape(const std::string& str);
	std::string buildJSONFlat(const JSON_ExportOptions& jsonOpts);
	std::string buildJSONGroupedByFile(const JSON_ExportOptions& jsonOpts);
	std::string buildJSONGroupedByFileLine(const JSON_ExportOptions& jsonOpts);

	// PlainText helpers
	std::string formatPlainTextCompact();
	std::string formatPlainTextReadable();
	std::string formatPlainTextFullDetails();

	// File I/O
	bool writeToFile(const std::string& content, const std::wstring& filePath,
		const std::wstring& encoding = L"UTF-8");

	// Utility methods
	std::string wstringToUtf8(const std::wstring& str);
	std::string getCurrentTimestamp();
};
