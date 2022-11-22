// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <utility>
#include <random>

// Base interface for line sorting.
class ISorter
{
private:
	bool _isDescending = true;
	size_t _fromColumn = 0;
	size_t _toColumn = 0;

protected:
	bool isDescending() const
	{
		return _isDescending;
	}

	generic_string getSortKey(const generic_string& input)
	{
		if (isSortingSpecificColumns())
		{
			if (input.length() < _fromColumn)
			{
				// prevent an std::out_of_range exception
				return TEXT("");
			}
			else if (_fromColumn == _toColumn)
			{
				// get characters from the indicated column to the end of the line
				return input.substr(_fromColumn);
			}
			else
			{
				// get characters between the indicated columns, inclusive
				return input.substr(_fromColumn, _toColumn - _fromColumn);
			}
		}
		else
		{
			return input;
		}
	}

	bool isSortingSpecificColumns()
	{
		return _toColumn != 0;
	}

public:
	ISorter(bool isDescending, size_t fromColumn, size_t toColumn) : _isDescending(isDescending), _fromColumn(fromColumn), _toColumn(toColumn)
	{
		assert(_fromColumn <= _toColumn);
	};
	virtual ~ISorter() { };
	virtual std::vector<generic_string> sort(std::vector<generic_string> lines) = 0;
};

// Implementation of lexicographic sorting of lines.
class LexicographicSorter : public ISorter
{
public:
	LexicographicSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) { };
	
	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		// Note that both branches here are equivalent in the sense that they always give the same answer.
		// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
		// getSortKey() so many times.
		if (isSortingSpecificColumns())
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
			{
				if (isDescending())
				{
					return getSortKey(a).compare(getSortKey(b)) > 0;
					
				}
				else
				{
					return getSortKey(a).compare(getSortKey(b)) < 0;
				}
			});
		}
		else
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
			{
				if (isDescending())
				{
					return a.compare(b) > 0;
				}
				else
				{
					return a.compare(b) < 0;
				}
			});
		}
		return lines;
	}
};

// Implementation of lexicographic sorting of lines, ignoring character casing
class LexicographicCaseInsensitiveSorter : public ISorter
{
public:
	LexicographicCaseInsensitiveSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) { };

	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		// Note that both branches here are equivalent in the sense that they always give the same answer.
		// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
		// getSortKey() so many times.
		if (isSortingSpecificColumns())
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
				{
					if (isDescending())
					{
						return OrdinalIgnoreCaseCompareStrings(getSortKey(a).c_str(), getSortKey(b).c_str()) > 0;
					}
					else
					{
						return OrdinalIgnoreCaseCompareStrings(getSortKey(a).c_str(), getSortKey(b).c_str()) < 0;
					}
				});
		}
		else
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
				{
					if (isDescending())
					{
						return OrdinalIgnoreCaseCompareStrings(a.c_str(), b.c_str()) > 0;
					}
					else
					{
						return OrdinalIgnoreCaseCompareStrings(a.c_str(), b.c_str()) < 0;
					}
				});
		}
		return lines;
	}
};

class IntegerSorter : public ISorter
{
public:
	IntegerSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) { };

	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		if (isSortingSpecificColumns())
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string aIn, generic_string bIn)
			{
				generic_string a = getSortKey(aIn);
				generic_string b = getSortKey(bIn);

				long long compareResult = 0;
				size_t aNumIndex = 0;
				size_t bNumIndex = 0;
				while (compareResult == 0)
				{
					if (aNumIndex >= a.length() || bNumIndex >= b.length())
					{
						compareResult = a.compare(min(aNumIndex, a.length()), generic_string::npos, b, min(bNumIndex, b.length()), generic_string::npos);
						break;
					}

					bool aChunkIsNum = a[aNumIndex] >= L'0' && a[aNumIndex] <= L'9';
					bool bChunkIsNum = b[bNumIndex] >= L'0' && b[bNumIndex] <= L'9';

					int aNumSign = 1;
					// Could be start of negative number
					if (!aChunkIsNum && (aNumIndex + 1) < a.length())
					{
						aChunkIsNum = (a[aNumIndex] == L'-' && (a[aNumIndex + 1] >= L'0' && a[aNumIndex + 1] <= L'9'));
						aNumSign = -1;
					}

					int bNumSign = 1;
					if (!bChunkIsNum && (bNumIndex + 1) < b.length())
					{
						bChunkIsNum = (b[bNumIndex] == L'-' && (b[bNumIndex + 1] >= L'0' && b[bNumIndex + 1] <= L'9'));
						bNumSign = -1;
					}

					// One is number and one is string
					if (aChunkIsNum != bChunkIsNum)
					{
						compareResult = a[aNumIndex] - b[bNumIndex];

						// compareResult isn't necessarily 0
						// consider this case: "0-0", "0-"
						// "-0" is considered a number, but "-" isn't
						// but we are comparing two "-', which is the same
						aNumIndex++;
						bNumIndex++;
					}
					// Both are numbers
					else if (aChunkIsNum)
					{
						// if the sign is differemt, just return the compareResult
						if (aNumSign != bNumSign)
						{
							if (aNumSign == 1)
							{
								compareResult = 1;
							}
							else
							{
								compareResult = -1;
							}
							// No need to update anything; compareResult != 0; will break out while loop
						}
						else
						{
							if (aNumSign == -1)
							{
								aNumIndex++;
								bNumIndex++;
							}

							size_t aNumEnd = a.find_first_not_of(L"1234567890", aNumIndex);
							if (aNumEnd == generic_string::npos)
							{
								aNumEnd = a.length();
							}

							size_t bNumEnd = b.find_first_not_of(L"1234567890", bNumIndex);
							if (bNumEnd == generic_string::npos)
							{
								bNumEnd = b.length();
							}

							int aZeroNum = 0;
							while (aNumIndex < a.length() && a[aNumIndex] == '0')
							{
								aZeroNum++;
								aNumIndex++;
							}

							int bZeroNum = 0;
							while (bNumIndex < b.length() && b[bNumIndex] == '0')
							{
								bZeroNum++;
								bNumIndex++;
							}

							size_t aNumLength = aNumEnd - aNumIndex;
							size_t bNumLength = bNumEnd - bNumIndex;

							// aNum is longer than bNum, must be larger (smaller if negative)
							if (aNumLength > bNumLength)
							{
								compareResult = 1 * aNumSign;
								// No need to update anything; compareResult != 0; will break out while loop
							}
							// bNum is longer than aNum, must be larger (smaller if negative)
							else if (aNumLength < bNumLength)
							{
								compareResult = -1 * aNumSign;
								// No need to update anything; compareResult != 0; will break out while loop
							}
							else
							{
								// the lengths of the numbers are equal
								// compare the two number. However, we can not use std::stoll
								// because the number strings can be every large, well over the maximum long long value
								// thus, we compare each digit one by one
								while (compareResult == 0
									&& aNumIndex < a.length()
									&& (a[aNumIndex] >= L'0' && a[aNumIndex] <= L'9')
									&& bNumIndex < b.length()
									&& (b[bNumIndex] >= L'0' && b[bNumIndex] <= L'9'))
								{
									compareResult = (a[aNumIndex] - b[bNumIndex]) * aNumSign;
									aNumIndex++;
									bNumIndex++;
								}

								if (compareResult == 0)
								{
									compareResult = bZeroNum - aZeroNum;
								}
							}
						}
					}
					// Both are strings
					else
					{
						if (a[aNumIndex] == L'-')
						{
							aNumIndex++;
						}
						if (b[bNumIndex] == L'-')
						{
							bNumIndex++;
						}

						size_t aChunkEnd = a.find_first_of(L"1234567890-", aNumIndex);
						size_t bChunkEnd = b.find_first_of(L"1234567890-", bNumIndex);
						compareResult = a.compare(aNumIndex, aChunkEnd - aNumIndex, b, bNumIndex, bChunkEnd - bNumIndex);
						aNumIndex = aChunkEnd;
						bNumIndex = bChunkEnd;
					}
				}

				if (isDescending())
				{
					return compareResult > 0;
				}
				else
				{
					return compareResult < 0;
				}
			});
		}
		else
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string aIn, generic_string bIn)
			{
				generic_string a = aIn;
				generic_string b = bIn;

				long long compareResult = 0;
				size_t aNumIndex = 0;
				size_t bNumIndex = 0;
				while (compareResult == 0)
				{
					if (aNumIndex >= a.length() || bNumIndex >= b.length())
					{
						compareResult = a.compare(min(aNumIndex, a.length()), generic_string::npos, b, min(bNumIndex, b.length()), generic_string::npos);
						break;
					}

					bool aChunkIsNum = a[aNumIndex] >= L'0' && a[aNumIndex] <= L'9';
					bool bChunkIsNum = b[bNumIndex] >= L'0' && b[bNumIndex] <= L'9';

					int aNumSign = 1;
					// Could be start of negative number
					if (!aChunkIsNum && (aNumIndex + 1) < a.length())
					{
						aChunkIsNum = (a[aNumIndex] == L'-' && (a[aNumIndex + 1] >= L'0' && a[aNumIndex + 1] <= L'9'));
						aNumSign = -1;
					}

					int bNumSign = 1;
					if (!bChunkIsNum && (bNumIndex + 1) < b.length())
					{
						bChunkIsNum = (b[bNumIndex] == L'-' && (b[bNumIndex + 1] >= L'0' && b[bNumIndex + 1] <= L'9'));
						bNumSign = -1;
					}

					// One is number and one is string
					if (aChunkIsNum != bChunkIsNum)
					{
						compareResult = a[aNumIndex] - b[bNumIndex];

						// compareResult isn't necessarily 0
						// consider this case: "0-0", "0-"
						// "-0" is considered a number, but "-" isn't
						// but we are comparing two "-', which is the same
						// increment the indexes for this case and continue the loop
						aNumIndex++;
						bNumIndex++;
					}
					// Both are numbers
					else if (aChunkIsNum)
					{
						// if the sign is differemt, just return the compareResult
						if (aNumSign != bNumSign)
						{
							if (aNumSign == 1)
							{
								compareResult = 1;
							}
							else
							{
								compareResult = -1;
							}
							// No need to update anything; compareResult != 0; will break out while loop
						}
						else
						{
							if (aNumSign == -1)
							{
								aNumIndex++;
								bNumIndex++;
							}

							size_t aNumEnd = a.find_first_not_of(L"1234567890", aNumIndex);
							if (aNumEnd == generic_string::npos)
							{
								aNumEnd = a.length();
							}

							size_t bNumEnd = b.find_first_not_of(L"1234567890", bNumIndex);
							if (bNumEnd == generic_string::npos)
							{
								bNumEnd = b.length();
							}

							int aZeroNum = 0;
							while (aNumIndex < a.length() && a[aNumIndex] == '0')
							{
								aZeroNum++;
								aNumIndex++;
							}

							int bZeroNum = 0;
							while (bNumIndex < b.length() && b[bNumIndex] == '0')
							{
								bZeroNum++;
								bNumIndex++;
							}

							size_t aNumLength = aNumEnd - aNumIndex;
							size_t bNumLength = bNumEnd - bNumIndex;

							// aNum is longer than bNum, must be larger (smaller if negative)
							if (aNumLength > bNumLength)
							{
								compareResult = 1 * aNumSign;
								// No need to update anything; compareResult != 0; will break out while loop
							}
							// bNum is longer than aNum, must be larger (smaller if negative)
							else if (aNumLength < bNumLength)
							{
								compareResult = -1 * aNumSign;
								// No need to update anything; compareResult != 0; will break out while loop
							}
							else
							{
								// the lengths of the numbers are equal
								// compare the two number. However, we can not use std::stoll
								// because the number strings can be every large, well over the maximum long long value
								// thus, we compare each digit one by one
								while (compareResult == 0
									&& aNumIndex < a.length()
									&& (a[aNumIndex] >= L'0' && a[aNumIndex] <= L'9')
									&& bNumIndex < b.length()
									&& (b[bNumIndex] >= L'0' && b[bNumIndex] <= L'9'))
								{
									compareResult = (a[aNumIndex] - b[bNumIndex]) * aNumSign;
									aNumIndex++;
									bNumIndex++;
								}

								if (compareResult == 0)
								{
									compareResult = bZeroNum - aZeroNum;
								}
							}
						}
					}
					// Both are strings
					else
					{
						if (a[aNumIndex] == L'-')
						{
							aNumIndex++;
						}
						if (b[bNumIndex] == L'-')
						{
							bNumIndex++;
						}

						size_t aChunkEnd = a.find_first_of(L"1234567890-", aNumIndex);
						size_t bChunkEnd = b.find_first_of(L"1234567890-", bNumIndex);
						compareResult = a.compare(aNumIndex, aChunkEnd - aNumIndex, b, bNumIndex, bChunkEnd - bNumIndex);
						aNumIndex = aChunkEnd;
						bNumIndex = bChunkEnd;
					}
				}

				if (isDescending())
				{
					return compareResult > 0;
				}
				else
				{
					return compareResult < 0;
				}
			});
		}
	

		return lines;
	}
};

// Convert each line to a number and then sort.
// The conversion must be implemented in classes which inherit from this, see prepareStringForConversion and convertStringToNumber.
template<typename T_Num>
class NumericSorter : public ISorter
{
public:
	NumericSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn)
	{
#ifdef __MINGW32__
		_usLocale = NULL;
#else
		_usLocale = ::_create_locale(LC_NUMERIC, "en-US");
#endif
	};

	~NumericSorter()
	{
#ifndef __MINGW32__
		::_free_locale(_usLocale);
#endif
	}
	
	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		// Note that empty lines are filtered out and added back manually to the output at the end.
		std::vector<std::pair<size_t, T_Num>> nonEmptyInputAsNumbers;
		std::vector<generic_string> empties;
		nonEmptyInputAsNumbers.reserve(lines.size());
		for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
		{
			const generic_string originalLine = lines[lineIndex];
			const generic_string preparedLine = prepareStringForConversion(originalLine);
			if (considerStringEmpty(preparedLine))
			{
				empties.push_back(originalLine);
			}
			else
			{
				try
				{
					nonEmptyInputAsNumbers.push_back(std::make_pair(lineIndex, convertStringToNumber(preparedLine)));
				}
				catch (...)
				{
					throw lineIndex;
				}
			}
		}
		assert(nonEmptyInputAsNumbers.size() + empties.size() == lines.size());
		const bool descending = isDescending();
		std::sort(nonEmptyInputAsNumbers.begin(), nonEmptyInputAsNumbers.end(), [descending](std::pair<size_t, T_Num> a, std::pair<size_t, T_Num> b)
		{
			if (descending)
			{
				return a.second > b.second;
			}
			else
			{
				return a.second < b.second;
			}
		});
		std::vector<generic_string> output;
		output.reserve(lines.size());
		if (!isDescending())
		{
			output.insert(output.end(), empties.begin(), empties.end());
		}
		for (auto it = nonEmptyInputAsNumbers.begin(); it != nonEmptyInputAsNumbers.end(); ++it)
		{
			output.push_back(lines[it->first]);
		}
		if (isDescending())
		{
			output.insert(output.end(), empties.begin(), empties.end());
		}
		assert(output.size() == lines.size());
		return output;
	}

protected:
	bool considerStringEmpty(const generic_string& input)
	{
		// String has something else than just whitespace.
		return input.find_first_not_of(TEXT(" \t\r\n")) == std::string::npos;
	}

	// Prepare the string for conversion to number.
	virtual generic_string prepareStringForConversion(const generic_string& input) = 0;

	// Should convert the input string to a number of the correct type.
	// If unable to convert, throw either std::invalid_argument or std::out_of_range.
	virtual T_Num convertStringToNumber(const generic_string& input) = 0;

	// We need a fixed locale so we get the same string-to-double behavior across all computers.
	// This is the "enUS" locale.
	_locale_t _usLocale;
};

// Converts lines to double before sorting (assumes decimal comma).
class DecimalCommaSorter : public NumericSorter<double>
{
public:
	DecimalCommaSorter(bool isDescending, size_t fromColumn, size_t toColumn) : NumericSorter<double>(isDescending, fromColumn, toColumn) { };

protected:
	generic_string prepareStringForConversion(const generic_string& input) override
	{
		generic_string admissablePart = stringTakeWhileAdmissable(getSortKey(input), TEXT(" \t\r\n0123456789,-"));
		return stringReplace(admissablePart, TEXT(","), TEXT("."));
	}

	double convertStringToNumber(const generic_string& input) override
	{
		return stodLocale(input, _usLocale);
	}
};

// Converts lines to double before sorting (assumes decimal dot).
class DecimalDotSorter : public NumericSorter<double>
{
public:
	DecimalDotSorter(bool isDescending, size_t fromColumn, size_t toColumn) : NumericSorter<double>(isDescending, fromColumn, toColumn) { };

protected:
	generic_string prepareStringForConversion(const generic_string& input) override
	{
		return stringTakeWhileAdmissable(getSortKey(input), TEXT(" \t\r\n0123456789.-"));
	}

	double convertStringToNumber(const generic_string& input) override
	{
		return stodLocale(input, _usLocale);
	}
};

class ReverseSorter : public ISorter
{
public:
	ReverseSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) { };

	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		std::reverse(lines.begin(), lines.end());
		return lines;
	}
};

class RandomSorter : public ISorter
{
public:
	unsigned seed;
	RandomSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn)
	{
		seed = static_cast<unsigned>(time(NULL));
	}
	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		std::shuffle(lines.begin(), lines.end(), std::default_random_engine(seed));
		return lines;
	}
};

