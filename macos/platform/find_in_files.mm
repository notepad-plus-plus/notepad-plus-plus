// find_in_files.mm — Find in Files dialog and search engine implementation
#include "find_in_files.h"
#include "file_operations.h"
#import <Cocoa/Cocoa.h>
#include <fnmatch.h>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <cstring>
#include <atomic>
#include <dispatch/dispatch.h>

// ---------------------------------------------------------------------------
// Search state
// ---------------------------------------------------------------------------
static std::atomic<bool> sFIFRunning{false};
static std::atomic<bool> sFIFCancel{false};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kMaxMatches = 10000;
static constexpr int kBatchSize = 20;
static constexpr size_t kMaxLineTextLength = 500;

// ---------------------------------------------------------------------------
// Callback type
// ---------------------------------------------------------------------------
using FIFCallback = std::function<void(const FIFSearchResult&, bool complete)>;

// ---------------------------------------------------------------------------
// isWordChar — helper for whole-word boundary checking
// ---------------------------------------------------------------------------
static bool isWordChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ---------------------------------------------------------------------------
// enumerateFiles — collect file paths matching glob filters under a directory
// ---------------------------------------------------------------------------
static std::vector<std::string> enumerateFiles(NSString* directory,
                                                const std::string& filters,
                                                bool recursive,
                                                bool includeHidden)
{
	static constexpr int kMaxFiles = 100000;
	std::vector<std::string> result;

	NSFileManager* fm = [NSFileManager defaultManager];
	NSURL* dirURL = [NSURL fileURLWithPath:directory isDirectory:YES];

	// Build enumeration options
	NSDirectoryEnumerationOptions options = 0;
	if (!recursive)
	{
		options |= NSDirectoryEnumerationSkipsSubdirectoryDescendants;
	}
	if (!includeHidden)
	{
		options |= NSDirectoryEnumerationSkipsHiddenFiles;
	}

	NSArray<NSURLResourceKey>* keys = @[NSURLIsRegularFileKey, NSURLNameKey];

	NSDirectoryEnumerator<NSURL*>* enumerator =
		[fm enumeratorAtURL:dirURL
		 includingPropertiesForKeys:keys
		 options:options
		 errorHandler:nil];

	// Parse filter patterns by splitting on ";"
	bool acceptAll = false;
	std::vector<std::string> patterns;

	if (filters.empty() || filters == "*.*" || filters == "*")
	{
		acceptAll = true;
	}
	else
	{
		std::istringstream stream(filters);
		std::string token;
		while (std::getline(stream, token, ';'))
		{
			// Trim whitespace
			size_t start = token.find_first_not_of(" \t");
			size_t end = token.find_last_not_of(" \t");
			if (start != std::string::npos)
			{
				patterns.push_back(token.substr(start, end - start + 1));
			}
		}
		if (patterns.empty())
		{
			acceptAll = true;
		}
	}

	for (NSURL* fileURL in enumerator)
	{
		if (static_cast<int>(result.size()) >= kMaxFiles)
		{
			break;
		}

		// Skip directories — only include regular files
		NSNumber* isRegularFile = nil;
		[fileURL getResourceValue:&isRegularFile forKey:NSURLIsRegularFileKey error:nil];
		if (![isRegularFile boolValue])
		{
			continue;
		}

		if (!acceptAll)
		{
			NSString* fileName = [fileURL lastPathComponent];
			const char* name = [fileName UTF8String];
			bool matched = false;
			for (const auto& pattern : patterns)
			{
				if (fnmatch(pattern.c_str(), name, FNM_CASEFOLD) == 0)
				{
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				continue;
			}
		}

		result.push_back([[fileURL path] UTF8String]);
	}

	return result;
}

// ---------------------------------------------------------------------------
// isBinaryFile — returns true if first 8KB of file contains null bytes
// ---------------------------------------------------------------------------
static bool isBinaryFile(NSString* path)
{
	NSError* error = nil;
	NSData* data = [NSData dataWithContentsOfFile:path
	                                      options:NSDataReadingMappedIfSafe
	                                        error:&error];
	if (!data)
	{
		return false;
	}

	static constexpr NSUInteger kCheckSize = 8192;
	NSUInteger length = [data length];
	if (length > kCheckSize)
	{
		length = kCheckSize;
	}

	const uint8_t* bytes = static_cast<const uint8_t*>([data bytes]);
	for (NSUInteger i = 0; i < length; ++i)
	{
		if (bytes[i] == 0x00)
		{
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// searchLinePlainText — search a single line for plain text matches
// ---------------------------------------------------------------------------
static void searchLinePlainText(const std::string& line,
                                const std::string& searchTerm,
                                const std::string& searchTermLower,
                                bool matchCase,
                                bool wholeWord,
                                int lineNumber,
                                FIFFileResult& fileResult,
                                int& totalMatches)
{
	const char* linePtr = line.c_str();
	size_t lineLen = line.size();
	size_t termLen = searchTerm.size();
	size_t pos = 0;

	while (pos + termLen <= lineLen && totalMatches < kMaxMatches)
	{
		const char* found = nullptr;

		if (matchCase)
		{
			found = strstr(linePtr + pos, searchTerm.c_str());
		}
		else
		{
			found = strcasestr(linePtr + pos, searchTerm.c_str());
		}

		if (!found)
		{
			break;
		}

		size_t matchPos = static_cast<size_t>(found - linePtr);

		// Whole-word boundary check
		if (wholeWord)
		{
			bool leftBoundary = (matchPos == 0) || !isWordChar(linePtr[matchPos - 1]);
			bool rightBoundary = (matchPos + termLen >= lineLen) ||
			                     !isWordChar(linePtr[matchPos + termLen]);
			if (!leftBoundary || !rightBoundary)
			{
				pos = matchPos + 1;
				continue;
			}
		}

		// Trim line text
		std::string lineText = line;
		if (lineText.size() > kMaxLineTextLength)
		{
			lineText = lineText.substr(0, kMaxLineTextLength);
		}

		FIFMatch match;
		match.lineNumber = lineNumber;
		match.column = static_cast<int>(matchPos);
		match.matchLength = static_cast<int>(termLen);
		match.lineText = lineText;
		fileResult.matches.push_back(std::move(match));
		++totalMatches;

		pos = matchPos + termLen;
	}
}

// ---------------------------------------------------------------------------
// searchLineRegex — search a single line for regex matches
// ---------------------------------------------------------------------------
static void searchLineRegex(const std::string& line,
                            const std::regex& pattern,
                            bool wholeWord,
                            int lineNumber,
                            FIFFileResult& fileResult,
                            int& totalMatches)
{
	std::sregex_iterator it(line.begin(), line.end(), pattern);
	std::sregex_iterator end;

	for (; it != end && totalMatches < kMaxMatches; ++it)
	{
		const std::smatch& m = *it;
		size_t matchPos = static_cast<size_t>(m.position());
		size_t matchLen = static_cast<size_t>(m.length());

		// Whole-word boundary check
		if (wholeWord)
		{
			bool leftBoundary = (matchPos == 0) ||
			                    !isWordChar(line[matchPos - 1]);
			bool rightBoundary = (matchPos + matchLen >= line.size()) ||
			                     !isWordChar(line[matchPos + matchLen]);
			if (!leftBoundary || !rightBoundary)
			{
				continue;
			}
		}

		// Trim line text
		std::string lineText = line;
		if (lineText.size() > kMaxLineTextLength)
		{
			lineText = lineText.substr(0, kMaxLineTextLength);
		}

		FIFMatch match;
		match.lineNumber = lineNumber;
		match.column = static_cast<int>(matchPos);
		match.matchLength = static_cast<int>(matchLen);
		match.lineText = lineText;
		fileResult.matches.push_back(std::move(match));
		++totalMatches;
	}
}

// ---------------------------------------------------------------------------
// doFindInFiles — background search engine
// ---------------------------------------------------------------------------
static void doFindInFiles(const std::string& searchTerm,
                          const std::string& directory,
                          const std::string& filters,
                          bool matchCase,
                          bool wholeWord,
                          bool useRegex,
                          bool recursive,
                          bool includeHidden,
                          FIFCallback callback)
{
	// Prevent concurrent searches
	if (sFIFRunning.load())
	{
		return;
	}

	sFIFRunning.store(true);
	sFIFCancel.store(false);

	// Enumerate files on the calling thread (fast enough, avoids threading issues)
	NSString* dirNS = [NSString stringWithUTF8String:directory.c_str()];
	std::vector<std::string> files = enumerateFiles(dirNS, filters, recursive, includeHidden);

	// Capture everything needed by the block
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		@autoreleasepool
		{
			FIFSearchResult result;
			result.searchTerm = searchTerm;
			result.directory = directory;
			result.filters = filters;
			int totalMatches = 0;

			// Prepare regex if needed
			std::regex regexPattern;
			bool regexValid = true;
			if (useRegex)
			{
				try
				{
					auto flags = std::regex_constants::ECMAScript;
					if (!matchCase)
					{
						flags |= std::regex_constants::icase;
					}
					regexPattern = std::regex(searchTerm, flags);
				}
				catch (const std::regex_error&)
				{
					regexValid = false;
				}
			}

			// Prepare lowercase search term for case-insensitive plain text search
			std::string searchTermLower;
			if (!useRegex && !matchCase)
			{
				searchTermLower.resize(searchTerm.size());
				for (size_t i = 0; i < searchTerm.size(); ++i)
				{
					searchTermLower[i] = static_cast<char>(
						std::tolower(static_cast<unsigned char>(searchTerm[i])));
				}
			}

			int filesProcessed = 0;

			for (const auto& filePath : files)
			{
				// Check cancel flag
				if (sFIFCancel.load())
				{
					break;
				}

				// Check match cap
				if (totalMatches >= kMaxMatches)
				{
					break;
				}

				@autoreleasepool
				{
					NSString* pathNS = [NSString stringWithUTF8String:filePath.c_str()];

					// Skip binary files
					if (isBinaryFile(pathNS))
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Read file data
					NSData* data = [NSData dataWithContentsOfFile:pathNS];
					if (!data || [data length] == 0)
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Detect encoding and decode to UTF-8
					int encoding = detectEncoding(data);
					std::string utf8Text = decodeFileData(data, encoding);

					if (utf8Text.empty())
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Search line-by-line
					FIFFileResult fileResult;
					fileResult.filePath = filePath;

					std::istringstream textStream(utf8Text);
					std::string line;
					int lineNumber = 0;

					while (std::getline(textStream, line) && totalMatches < kMaxMatches)
					{
						++lineNumber;

						// Remove trailing \r if present (for \r\n line endings)
						if (!line.empty() && line.back() == '\r')
						{
							line.pop_back();
						}

						if (useRegex)
						{
							if (regexValid)
							{
								searchLineRegex(line, regexPattern, wholeWord,
								                lineNumber, fileResult, totalMatches);
							}
						}
						else
						{
							searchLinePlainText(line, searchTerm, searchTermLower,
							                    matchCase, wholeWord,
							                    lineNumber, fileResult, totalMatches);
						}
					}

					++result.filesSearched;

					if (!fileResult.matches.empty())
					{
						++result.filesWithMatches;
						result.totalMatches += static_cast<int>(fileResult.matches.size());
						result.fileResults.push_back(std::move(fileResult));
					}

					++filesProcessed;

					// Post batch update to main thread every kBatchSize files
					if (filesProcessed % kBatchSize == 0)
					{
						FIFSearchResult batchResult = result;
						FIFCallback cb = callback;
						dispatch_async(dispatch_get_main_queue(), ^{
							cb(batchResult, false);
						});
					}
				}
			}

			// Post final result on main thread
			FIFSearchResult finalResult = std::move(result);
			FIFCallback cb = callback;
			dispatch_async(dispatch_get_main_queue(), ^{
				sFIFRunning.store(false);
				cb(finalResult, true);
			});
		}
	});
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void showFindInFilesDlg()
{
	// Stub — will be implemented in Task 8
}

bool isFIFSearchRunning()
{
	return sFIFRunning.load();
}

void cancelFIFSearch()
{
	sFIFCancel.store(true);
}
