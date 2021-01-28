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


#ifndef NPP_SORTERS_H
#define NPP_SORTERS_H

#include <algorithm>
#include <utility>
#include <random>

// Base interface for line sorting.
class ISorter
{
private:
	bool _isDescending;
	size_t _fromColumn, _toColumn;

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

// Treat consecutive numerals as one number
// Otherwise it is a lexicographic sort
class NaturalSorter : public ISorter
{
public:
	NaturalSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn) { };

	std::vector<generic_string> sort(std::vector<generic_string> lines) override
	{
		// Note that both branches here are equivalent in the sense that they always give the same answer.
		// However, if we are *not* sorting specific columns, then we get a 40% speed improvement by not calling
		// getSortKey() so many times.
		if (isSortingSpecificColumns())
		{
			std::sort(lines.begin(), lines.end(), [this](generic_string aIn, generic_string bIn)
			{
				generic_string a = getSortKey(aIn);
				generic_string b = getSortKey(bIn);

				long long compareResult = 0;
				size_t i = 0;
				while (compareResult == 0)
				{
					if (i >= a.length() || i >= b.length())
					{
						compareResult = a.compare(min(i, a.length()), generic_string::npos, b, min(i, b.length()), generic_string::npos);
						break;
					}

					bool aChunkIsNum = a[i] >= L'0' && a[i] <= L'9';
					bool bChunkIsNum = b[i] >= L'0' && b[i] <= L'9';

					// One is number and one is string
					if (aChunkIsNum != bChunkIsNum)
					{
						compareResult = a[i] - b[i];
						// No need to update i; compareResult != 0
					}
					// Both are numbers
					else if (aChunkIsNum)
					{
						size_t delta = 0;

						// stoll crashes if number exceeds the limit for unsigned long long
						// Maximum value for a variable of type unsigned long long | 18446744073709551615
						// So take the max length 18 to convert the number
						const size_t maxLen = 18;
						compareResult = std::stoll(a.substr(i, maxLen)) - std::stoll(b.substr(i, maxLen), &delta);
						i += delta;
					}
					// Both are strings
					else
					{
						size_t aChunkEnd = a.find_first_of(L"1234567890", i);
						size_t bChunkEnd = b.find_first_of(L"1234567890", i);
						compareResult = a.compare(i, aChunkEnd - i, b, i, bChunkEnd - i);
						i = aChunkEnd;
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
			std::sort(lines.begin(), lines.end(), [this](generic_string a, generic_string b)
			{
				long long compareResult = 0;
				size_t i = 0;
				while (compareResult == 0)
				{
					if (i >= a.length() || i >= b.length())
					{
						compareResult = a.compare(min(i,a.length()), generic_string::npos, b, min(i,b.length()), generic_string::npos);
						break;
					}

					bool aChunkIsNum = a[i] >= L'0' && a[i] <= L'9';
					bool bChunkIsNum = b[i] >= L'0' && b[i] <= L'9';

					// One is number and one is string
					if (aChunkIsNum != bChunkIsNum)
					{
						compareResult = a[i] - b[i];
						// No need to update i; compareResult != 0
					}
					// Both are numbers
					else if (aChunkIsNum)
					{
						size_t delta = 0;

						// stoll crashes if number exceeds the limit for unsigned long long
						// Maximum value for a variable of type unsigned long long | 18446744073709551615
						// So take the max length 18 to convert the number
						const size_t maxLen = 18;
						compareResult = std::stoll(a.substr(i, maxLen)) - std::stoll(b.substr(i, maxLen), &delta);
						i += delta;
					}
					// Both are strings
					else
					{
						size_t aChunkEnd = a.find_first_of(L"1234567890", i);
						size_t bChunkEnd = b.find_first_of(L"1234567890", i);
						compareResult = a.compare(i, aChunkEnd-i, b, i, bChunkEnd-i);
						i = aChunkEnd;
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

#endif //NPP_SORTERS_H
