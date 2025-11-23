#include "ResultsExporter.h"
#include "FindReplaceDlg.h"
#include "../MISC/Common/FileInterface.h"
#include "../MISC/Common/Common.h"
#include "../Utf8_16.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <set>

ResultsExporter::ResultsExporter(const std::vector<FoundInfo>& foundInfos,
	const std::wstring& searchQuery,
	const std::wstring& searchScope)
	: _foundInfos(foundInfos)
	, _searchQuery(searchQuery)
	, _searchScope(searchScope)
{
	buildMetadata();
}

bool ResultsExporter::generateContent(const ExportOptions& options, std::string& content, std::wstring& errorMsg)
{
	try {
		gatherResults();

		// Check if we have any results to export
		if (_results.empty()) {
			errorMsg = L"No results to export.";
			return false;
		}

		// Format results with exception safety - catch all formatting errors
		try {
			switch (options.format) {
				case ExportFormat::CSV:
					content = formatAsCSV(options.csvOpts);
					break;
				case ExportFormat::JSON:
					content = formatAsJSON(options.jsonOpts);
					break;
				case ExportFormat::PlainText:
					content = formatAsPlainText(options.plainTextOpts);
					break;
			}
		} catch (const std::exception& ex) {
			// Formatting failed
			std::string msg(ex.what());
			errorMsg = L"Content generation failed: " + std::wstring(msg.begin(), msg.end());
			return false;
		}

		return true;
	}
	catch (const std::exception& ex) {
		std::string msg(ex.what());
		errorMsg = L"Content generation failed: " + std::wstring(msg.begin(), msg.end());
		return false;
	}
}

bool ResultsExporter::exportResults(const ExportOptions& options, std::wstring& errorMsg)
{
	try {
		// Validate output path - extract directory and verify it exists
		if (options.outputPath.empty()) {
			errorMsg = L"Output path is empty.";
			return false;
		}

		// Extract parent directory from full path
		size_t lastSlash = options.outputPath.find_last_of(L"\\/");
		std::wstring outputDir = (lastSlash != std::wstring::npos) ?
			options.outputPath.substr(0, lastSlash) : L".";

		// Verify parent directory exists
		DWORD dirAttr = ::GetFileAttributes(outputDir.c_str());
		if (dirAttr == INVALID_FILE_ATTRIBUTES || !(dirAttr & FILE_ATTRIBUTE_DIRECTORY)) {
			errorMsg = L"Output directory does not exist.";
			return false;
		}

		// Check if output file already exists before proceeding - file I/O safety check
		DWORD attr = ::GetFileAttributes(options.outputPath.c_str());
		if (attr != INVALID_FILE_ATTRIBUTES) {
			errorMsg = L"File already exists. Export cancelled.";
			return false;
		}

		// Generate formatted content (no file I/O, no safety checks for file existence)
		std::string formattedContent;
		if (!generateContent(options, formattedContent, errorMsg)) {
			return false;
		}

		if (!writeToFile(formattedContent, options.outputPath)) {
			errorMsg = L"Failed to write export file. Check permissions and disk space.";
			return false;
		}
		return true;
	}
	catch (const std::exception& ex) {
		std::string msg(ex.what());
		errorMsg = L"Export failed: " + std::wstring(msg.begin(), msg.end());
		return false;
	}
}

void ResultsExporter::buildMetadata()
{
	_metadata._searchQuery = _searchQuery;
	_metadata._searchScope = _searchScope;
	_metadata._totalMatches = static_cast<int>(_foundInfos.size());
	_metadata._filesMatched = 0;
	_metadata._filesSearched = 0;

	// Capture export time for reproducibility
	SYSTEMTIME st{};
	GetSystemTime(&st);  // Returns void, always succeeds
	_metadata._timestamp = st;

	_metadata._notepadppVersion = L"8.8.8";

	_metadata._matchCase = false;
	_metadata._matchWholeWord = false;
	_metadata._useRegularExpression = false;
	_metadata._wrapAround = false;
	_metadata._replaceQuery = L"";
	_metadata._searchDirectory = L"";
}

void ResultsExporter::gatherResults()
{
	_results.clear();
	std::set<std::wstring> uniqueFiles;

	for (size_t i = 0; i < _foundInfos.size(); ++i) {
		const auto& foundInfo = _foundInfos[i];
		uniqueFiles.insert(foundInfo._fullPath);

		// Use cached line content from FoundInfo, populated when search results were created
		// This allows exporting of unsaved files that don't exist on disk (e.g., "new 1")
		const std::wstring& lineContent = foundInfo._lineContent;

		// Export all matches on this line, not just the first one
		if (!foundInfo._ranges.empty() && !lineContent.empty()) {
			for (const auto& range : foundInfo._ranges) {
				ExportResult result;
				result._filePath = foundInfo._fullPath;
				result._lineNumber = foundInfo._lineNumber;
				result._matchRanges.push_back(range);

				// Defensive bounds checking: ensure range indices are valid and not negative
				if (range.first >= 0 && range.first < range.second &&
					range.second <= static_cast<intptr_t>(lineContent.length())) {
					result._matchedText = lineContent.substr(range.first, range.second - range.first);
				}

				result._lineContent = lineContent;
				_results.push_back(result);
			}
		} else if (!lineContent.empty()) {
			// No ranges found but we have content, still add result with empty match
			ExportResult result;
			result._filePath = foundInfo._fullPath;
			result._lineNumber = foundInfo._lineNumber;
			result._lineContent = lineContent;
			_results.push_back(result);
		}
	}

	_metadata._filesMatched = static_cast<int>(uniqueFiles.size());
}

std::string ResultsExporter::formatAsCSV(const CSV_ExportOptions& csvOpts)
{
	std::ostringstream oss;

	if (csvOpts.includeHeaders) {
		if (csvOpts.includeFilePaths) {
			oss << "File" << csvOpts.delimiter;
		}
		oss << "Line" << csvOpts.delimiter << "Match Offset" << csvOpts.delimiter
			<< "Matched Text" << csvOpts.delimiter << "Full Line Content";
		oss << "\r\n";
	}

	for (const auto& result : _results) {
		if (csvOpts.includeFilePaths) {
			std::string filePath = wstringToUtf8(result._filePath);
			oss << escapeCSVField(filePath, csvOpts.quoteFields) << csvOpts.delimiter;
		}

		oss << result._lineNumber << csvOpts.delimiter;

		// Export match offset - character position where match starts
		if (!result._matchRanges.empty()) {
			oss << result._matchRanges[0].first;
		} else {
			oss << "-1";  // No match found
		}
		oss << csvOpts.delimiter;

		// Export the matched text (the actual search result, not full line)
		std::string matchedText = wstringToUtf8(result._matchedText);
		oss << escapeCSVField(matchedText, csvOpts.quoteFields) << csvOpts.delimiter;

		// Export full line content (provides context for the match)
		std::string lineContent = wstringToUtf8(result._lineContent);
		oss << escapeCSVField(lineContent, csvOpts.quoteFields);

		oss << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::escapeCSVField(const std::string& field, bool alwaysQuote)
{
	// RFC 4180 compliance: wrap in quotes if field contains special characters (comma, newline, quote)
	// Do not convert newlines to escape sequences - CSV standard preserves them in quoted fields
	bool needsQuote = alwaysQuote ||
		field.find(',') != std::string::npos ||
		field.find(';') != std::string::npos ||
		field.find('"') != std::string::npos ||
		field.find('\n') != std::string::npos ||
		field.find('\r') != std::string::npos;

	if (!needsQuote) {
		return field;
	}

	// Escape double quotes by doubling them (RFC 4180 standard)
	// Preserve newlines and other characters as-is within quoted field
	std::string escaped;
	escaped += '"';
	for (char c : field) {
		if (c == '"') {
			// Double quotes escape as "" within quoted field
			escaped += "\"\"";
		} else {
			// Keep all other characters including newlines as-is
			escaped += c;
		}
	}
	escaped += '"';

	return escaped;
}

// JSON formatting
std::string ResultsExporter::formatAsJSON(const JSON_ExportOptions& jsonOpts)
{
	switch (jsonOpts.nesting) {
		case JSONNesting::Flat:
			return buildJSONFlat(jsonOpts);
		case JSONNesting::GroupedByFile:
			return buildJSONGroupedByFile(jsonOpts);
		case JSONNesting::GroupedByFileAndLine:
			return buildJSONGroupedByFileLine(jsonOpts);
		default:
			return buildJSONFlat(jsonOpts);
	}
}

std::string ResultsExporter::jsonEscape(const std::string& str)
{
	std::string result;
	for (unsigned char c : str) {
		switch (c) {
			case '"': result += "\\\""; break;
			case '\\': result += "\\\\"; break;
			case '\b': result += "\\b"; break;
			case '\f': result += "\\f"; break;
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case '\t': result += "\\t"; break;
			default:
				if (c < 32) {
					char buf[7];
					sprintf_s(buf, sizeof(buf), "\\u%04x", c);
					result += buf;
				} else {
					result += c;
				}
		}
	}
	return result;
}

std::string ResultsExporter::buildJSONFlat(const JSON_ExportOptions& jsonOpts)
{
	std::ostringstream oss;

	oss << "{\n";
	if (jsonOpts.includeMetadata) {
		oss << "  \"metadata\": {\n";
		oss << "    \"searchQuery\": \"" << jsonEscape(wstringToUtf8(_metadata._searchQuery)) << "\",\n";
		oss << "    \"searchScope\": \"" << jsonEscape(wstringToUtf8(_metadata._searchScope)) << "\",\n";
		oss << "    \"totalMatches\": " << _metadata._totalMatches << ",\n";
		oss << "    \"filesMatched\": " << _metadata._filesMatched << "\n";
		oss << "  },\n";
	}

	oss << "  \"results\": [\n";

	for (size_t i = 0; i < _results.size(); ++i) {
		const auto& result = _results[i];
		oss << "    {\n";
		if (jsonOpts.includeFilePaths) {
			oss << "      \"file\": \"" << jsonEscape(wstringToUtf8(result._filePath)) << "\",\n";
		}
		oss << "      \"line\": " << result._lineNumber << ",\n";
		oss << "      \"text\": \"" << jsonEscape(wstringToUtf8(result._lineContent)) << "\",\n";
		oss << "      \"match\": \"" << jsonEscape(wstringToUtf8(result._matchedText)) << "\"\n";
		oss << "    }";
		if (i < _results.size() - 1) {
			oss << ",";
		}
		oss << "\n";
	}

	oss << "  ]\n";
	oss << "}\n";

	return oss.str();
}

std::string ResultsExporter::buildJSONGroupedByFile(const JSON_ExportOptions& jsonOpts)
{
	std::ostringstream oss;
	std::map<std::wstring, std::vector<const ExportResult*>> resultsByFile;

	// Group results by file
	for (const auto& result : _results) {
		resultsByFile[result._filePath].push_back(&result);
	}

	oss << "{\n";
	if (jsonOpts.includeMetadata) {
		oss << "  \"metadata\": {\n";
		oss << "    \"searchQuery\": \"" << jsonEscape(wstringToUtf8(_metadata._searchQuery)) << "\",\n";
		oss << "    \"searchScope\": \"" << jsonEscape(wstringToUtf8(_metadata._searchScope)) << "\",\n";
		oss << "    \"totalMatches\": " << _metadata._totalMatches << ",\n";
		oss << "    \"filesMatched\": " << _metadata._filesMatched << "\n";
		oss << "  },\n";
	}

	oss << "  \"files\": {\n";

	size_t fileIdx = 0;
	for (const auto& [filePath, results] : resultsByFile) {
		oss << "    \"" << jsonEscape(wstringToUtf8(filePath)) << "\": {\n";
		oss << "      \"matchCount\": " << results.size() << ",\n";
		oss << "      \"results\": [\n";

		for (size_t i = 0; i < results.size(); ++i) {
			const auto* result = results[i];
			oss << "        {\n";
			oss << "          \"line\": " << result->_lineNumber << ",\n";
			oss << "          \"text\": \"" << jsonEscape(wstringToUtf8(result->_lineContent)) << "\",\n";
			oss << "          \"match\": \"" << jsonEscape(wstringToUtf8(result->_matchedText)) << "\"\n";
			oss << "        }";
			if (i < results.size() - 1) {
				oss << ",";
			}
			oss << "\n";
		}

		oss << "      ]\n";
		oss << "    }";
		if (fileIdx < resultsByFile.size() - 1) {
			oss << ",";
		}
		oss << "\n";
		fileIdx++;
	}

	oss << "  }\n";
	oss << "}\n";

	return oss.str();
}

std::string ResultsExporter::buildJSONGroupedByFileLine(const JSON_ExportOptions& jsonOpts)
{
	// For simplicity, default to flat format
	return buildJSONFlat(jsonOpts);
}

// PlainText formatting
std::string ResultsExporter::formatAsPlainText(const PlainText_ExportOptions& plainTextOpts)
{
	switch (plainTextOpts.style) {
		case PlainTextStyle::Compact:
			return formatPlainTextCompact();
		case PlainTextStyle::Readable:
			return formatPlainTextReadable();
		case PlainTextStyle::FullDetails:
			return formatPlainTextFullDetails();
		default:
			return formatPlainTextReadable();
	}
}

std::string ResultsExporter::formatPlainTextCompact()
{
	std::ostringstream oss;

	oss << "Search Results: \"" << wstringToUtf8(_metadata._searchQuery) << "\" ("
		<< _metadata._totalMatches << " matches in " << _metadata._filesMatched << " file";
	if (_metadata._filesMatched != 1) oss << "s";
	oss << ")\r\n";
	oss << "-----------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result._filePath != currentFile) {
			currentFile = result._filePath;
			oss << wstringToUtf8(result._filePath) << "\r\n";
		}
		oss << "  " << result._lineNumber << ":   "
			<< wstringToUtf8(result._lineContent) << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::formatPlainTextReadable()
{
	std::ostringstream oss;

	oss << "SEARCH RESULTS EXPORT\r\n";
	oss << "Search Query:  " << wstringToUtf8(_metadata._searchQuery) << "\r\n";
	oss << "Search Scope:  " << wstringToUtf8(_metadata._searchScope) << "\r\n";
	oss << "Total Matches: " << _metadata._totalMatches << "\r\n";
	oss << "Files Matched: " << _metadata._filesMatched << "\r\n";
	oss << "\r\n---------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result._filePath != currentFile) {
			if (!currentFile.empty()) oss << "\r\n";
			currentFile = result._filePath;
			oss << wstringToUtf8(result._filePath) << "\r\n";
			oss << "---------------------------------------------------\r\n";
		}

		oss << "  Line " << result._lineNumber;
		if (!result._matchRanges.empty()) {
			oss << ", Column " << (result._matchRanges[0].first + 1);
		}
		oss << ":\r\n";
		oss << "    " << wstringToUtf8(result._lineContent) << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::formatPlainTextFullDetails()
{
	std::ostringstream oss;

	oss << "SEARCH RESULTS EXPORT\r\n";
	oss << "Generated: " << getCurrentTimestamp() << "\r\n";
	oss << "Notepad++ Version: " << wstringToUtf8(_metadata._notepadppVersion) << "\r\n";
	oss << "---------------------------------------------------\r\n\r\n";

	oss << "Search Parameters:\r\n";
	oss << "  Query: " << wstringToUtf8(_metadata._searchQuery) << "\r\n";
	oss << "  Match Case: " << (_metadata._matchCase ? "Yes" : "No") << "\r\n";
	oss << "  Match Whole Word: " << (_metadata._matchWholeWord ? "Yes" : "No") << "\r\n";
	oss << "  Regular Expression: " << (_metadata._useRegularExpression ? "Yes" : "No") << "\r\n";
	oss << "  Scope: " << wstringToUtf8(_metadata._searchScope) << "\r\n";
	oss << "\r\nResults Summary:\r\n";
	oss << "  Total Matches: " << _metadata._totalMatches << "\r\n";
	oss << "  Files Matched: " << _metadata._filesMatched << "\r\n";
	oss << "\r\nResults:\r\n";
	oss << "---------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result._filePath != currentFile) {
			if (!currentFile.empty()) oss << "\r\n";
			currentFile = result._filePath;
			oss << "FILE: " << wstringToUtf8(result._filePath) << "\r\n";
			oss << "---------------------------------------------------\r\n";
		}

		oss << "  Line " << result._lineNumber;
		if (!result._matchRanges.empty()) {
			oss << " (Column " << (result._matchRanges[0].first + 1) << ")";
		}
		oss << ":\r\n";
		oss << "    Full Text: " << wstringToUtf8(result._lineContent) << "\r\n";
		oss << "    Matched:   " << wstringToUtf8(result._matchedText) << "\r\n\r\n";
	}

	return oss.str();
}

// File I/O
bool ResultsExporter::writeToFile(const std::string& content, const std::wstring& filePath, const std::wstring& encoding)
{
	try {
		Win32_IO_File outputFile(filePath.c_str());

		if (!outputFile.isOpened()) {
			return false;
		}

		// Write UTF-8 BOM if requested
		if (encoding == L"UTF-8") {
			const unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
			outputFile.write(reinterpret_cast<const char*>(utf8_bom), sizeof(utf8_bom));
		}

		// Write content
		if (!outputFile.write(content.c_str(), content.length())) {
			return false;
		}

		return true;
	}
	catch (...) {
		return false;
	}
}

// Utility methods
std::string ResultsExporter::wstringToUtf8(const std::wstring& str)
{
	return wstring2string(str, CP_UTF8);
}

std::string ResultsExporter::getCurrentTimestamp()
{
	SYSTEMTIME st;
	GetSystemTime(&st);

	std::ostringstream oss;
	oss << std::setfill('0')
		<< std::setw(4) << st.wYear << "-"
		<< std::setw(2) << st.wMonth << "-"
		<< std::setw(2) << st.wDay << " "
		<< std::setw(2) << st.wHour << ":"
		<< std::setw(2) << st.wMinute << ":"
		<< std::setw(2) << st.wSecond;

	return oss.str();
}
