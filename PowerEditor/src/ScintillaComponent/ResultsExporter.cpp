#include "ResultsExporter.h"
#include "FindReplaceDlg.h"
#include "../MISC/Common/FileInterface.h"
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
	_metadata.searchQuery = _searchQuery;
	_metadata.searchScope = _searchScope;
	_metadata.totalMatches = static_cast<int>(_foundInfos.size());
	_metadata.filesMatched = 0;
	_metadata.filesSearched = 0;

	// Capture export time for reproducibility
	SYSTEMTIME st{};
	GetSystemTime(&st);  // Returns void, always succeeds
	_metadata.timestamp = st;

	_metadata.notepadppVersion = L"8.8.8";

	_metadata.matchCase = false;
	_metadata.matchWholeWord = false;
	_metadata.useRegularExpression = false;
	_metadata.wrapAround = false;
	_metadata.replaceQuery = L"";
	_metadata.searchDirectory = L"";
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
				result.filePath = foundInfo._fullPath;
				result.lineNumber = foundInfo._lineNumber;
				result.matchRanges.push_back(range);

				// Defensive bounds checking: ensure range indices are valid and not negative
				if (range.first >= 0 && range.first < range.second &&
					range.second <= static_cast<intptr_t>(lineContent.length())) {
					result.matchedText = lineContent.substr(range.first, range.second - range.first);
				}

				result.lineContent = lineContent;
				_results.push_back(result);
			}
		} else if (!lineContent.empty()) {
			// No ranges found but we have content, still add result with empty match
			ExportResult result;
			result.filePath = foundInfo._fullPath;
			result.lineNumber = foundInfo._lineNumber;
			result.lineContent = lineContent;
			_results.push_back(result);
		}
	}

	_metadata.filesMatched = static_cast<int>(uniqueFiles.size());
}

std::string ResultsExporter::formatAsCSV(const CSV_ExportOptions& csvOpts)
{
	std::ostringstream oss;

	if (csvOpts.includeHeaders) {
		if (csvOpts.includeFilePaths) {
			oss << "File" << csvOpts.delimiter;
		}
		oss << "Line" << csvOpts.delimiter << "Content";
		oss << "\r\n";
	}

	for (const auto& result : _results) {
		if (csvOpts.includeFilePaths) {
			std::string filePath = wstringToUtf8(result.filePath);
			oss << escapeCSVField(filePath, csvOpts.quoteFields) << csvOpts.delimiter;
		}

		oss << result.lineNumber << csvOpts.delimiter;

		// Export full line content (not just the matched text fragment)
		std::string lineContent = wstringToUtf8(result.lineContent);
		oss << escapeCSVField(lineContent, csvOpts.quoteFields);

		oss << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::escapeCSVField(const std::string& field, bool alwaysQuote)
{
	// RFC 4180 compliance requires quoting fields with special characters
	bool needsQuote = alwaysQuote ||
		field.find(',') != std::string::npos ||
		field.find(';') != std::string::npos ||
		field.find('"') != std::string::npos ||
		field.find('\n') != std::string::npos ||
		field.find('\r') != std::string::npos;

	if (!needsQuote) {
		return field;
	}

	// Escape quotes by doubling
	std::string escaped;
	escaped += '"';
	for (char c : field) {
		if (c == '"') {
			escaped += "\"\"";
		} else if (c == '\n') {
			escaped += "\\n";
		} else if (c == '\r') {
			escaped += "\\r";
		} else if (c == '\t') {
			escaped += "\\t";
		} else {
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
		oss << "    \"searchQuery\": \"" << jsonEscape(wstringToUtf8(_metadata.searchQuery)) << "\",\n";
		oss << "    \"searchScope\": \"" << jsonEscape(wstringToUtf8(_metadata.searchScope)) << "\",\n";
		oss << "    \"totalMatches\": " << _metadata.totalMatches << ",\n";
		oss << "    \"filesMatched\": " << _metadata.filesMatched << "\n";
		oss << "  },\n";
	}

	oss << "  \"results\": [\n";

	for (size_t i = 0; i < _results.size(); ++i) {
		const auto& result = _results[i];
		oss << "    {\n";
		if (jsonOpts.includeFilePaths) {
			oss << "      \"file\": \"" << jsonEscape(wstringToUtf8(result.filePath)) << "\",\n";
		}
		oss << "      \"line\": " << result.lineNumber << ",\n";
		oss << "      \"text\": \"" << jsonEscape(wstringToUtf8(result.lineContent)) << "\",\n";
		oss << "      \"match\": \"" << jsonEscape(wstringToUtf8(result.matchedText)) << "\"\n";
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
		resultsByFile[result.filePath].push_back(&result);
	}

	oss << "{\n";
	if (jsonOpts.includeMetadata) {
		oss << "  \"metadata\": {\n";
		oss << "    \"searchQuery\": \"" << jsonEscape(wstringToUtf8(_metadata.searchQuery)) << "\",\n";
		oss << "    \"searchScope\": \"" << jsonEscape(wstringToUtf8(_metadata.searchScope)) << "\",\n";
		oss << "    \"totalMatches\": " << _metadata.totalMatches << ",\n";
		oss << "    \"filesMatched\": " << _metadata.filesMatched << "\n";
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
			oss << "          \"line\": " << result->lineNumber << ",\n";
			oss << "          \"text\": \"" << jsonEscape(wstringToUtf8(result->lineContent)) << "\",\n";
			oss << "          \"match\": \"" << jsonEscape(wstringToUtf8(result->matchedText)) << "\"\n";
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

	oss << "Search Results: \"" << wstringToUtf8(_metadata.searchQuery) << "\" ("
		<< _metadata.totalMatches << " matches in " << _metadata.filesMatched << " file";
	if (_metadata.filesMatched != 1) oss << "s";
	oss << ")\r\n";
	oss << "-----------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result.filePath != currentFile) {
			currentFile = result.filePath;
			oss << wstringToUtf8(result.filePath) << "\r\n";
		}
		oss << "  " << result.lineNumber << ":   "
			<< wstringToUtf8(result.lineContent) << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::formatPlainTextReadable()
{
	std::ostringstream oss;

	oss << "SEARCH RESULTS EXPORT\r\n";
	oss << "Search Query:  " << wstringToUtf8(_metadata.searchQuery) << "\r\n";
	oss << "Search Scope:  " << wstringToUtf8(_metadata.searchScope) << "\r\n";
	oss << "Total Matches: " << _metadata.totalMatches << "\r\n";
	oss << "Files Matched: " << _metadata.filesMatched << "\r\n";
	oss << "\r\n---------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result.filePath != currentFile) {
			if (!currentFile.empty()) oss << "\r\n";
			currentFile = result.filePath;
			oss << wstringToUtf8(result.filePath) << "\r\n";
			oss << "---------------------------------------------------\r\n";
		}

		oss << "  Line " << result.lineNumber;
		if (!result.matchRanges.empty()) {
			oss << ", Column " << (result.matchRanges[0].first + 1);
		}
		oss << ":\r\n";
		oss << "    " << wstringToUtf8(result.lineContent) << "\r\n";
	}

	return oss.str();
}

std::string ResultsExporter::formatPlainTextFullDetails()
{
	std::ostringstream oss;

	oss << "SEARCH RESULTS EXPORT\r\n";
	oss << "Generated: " << getCurrentTimestamp() << "\r\n";
	oss << "Notepad++ Version: " << wstringToUtf8(_metadata.notepadppVersion) << "\r\n";
	oss << "---------------------------------------------------\r\n\r\n";

	oss << "Search Parameters:\r\n";
	oss << "  Query: " << wstringToUtf8(_metadata.searchQuery) << "\r\n";
	oss << "  Match Case: " << (_metadata.matchCase ? "Yes" : "No") << "\r\n";
	oss << "  Match Whole Word: " << (_metadata.matchWholeWord ? "Yes" : "No") << "\r\n";
	oss << "  Regular Expression: " << (_metadata.useRegularExpression ? "Yes" : "No") << "\r\n";
	oss << "  Scope: " << wstringToUtf8(_metadata.searchScope) << "\r\n";
	oss << "\r\nResults Summary:\r\n";
	oss << "  Total Matches: " << _metadata.totalMatches << "\r\n";
	oss << "  Files Matched: " << _metadata.filesMatched << "\r\n";
	oss << "\r\nResults:\r\n";
	oss << "---------------------------------------------------\r\n\r\n";

	std::wstring currentFile;
	for (const auto& result : _results) {
		if (result.filePath != currentFile) {
			if (!currentFile.empty()) oss << "\r\n";
			currentFile = result.filePath;
			oss << "FILE: " << wstringToUtf8(result.filePath) << "\r\n";
			oss << "---------------------------------------------------\r\n";
		}

		oss << "  Line " << result.lineNumber;
		if (!result.matchRanges.empty()) {
			oss << " (Column " << (result.matchRanges[0].first + 1) << ")";
		}
		oss << ":\r\n";
		oss << "    Full Text: " << wstringToUtf8(result.lineContent) << "\r\n";
		oss << "    Matched:   " << wstringToUtf8(result.matchedText) << "\r\n\r\n";
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
	if (str.empty()) return std::string();

	// Calculate required buffer size for UTF-8 conversion
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
	if (size_needed <= 0) {
		// Conversion size calculation failed, return empty string
		return std::string();
	}

	std::string result(size_needed, 0);
	int converted = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size_needed, NULL, NULL);
	if (converted <= 0) {
		// Conversion failed, return empty string instead of corrupted data
		return std::string();
	}

	return result;
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
