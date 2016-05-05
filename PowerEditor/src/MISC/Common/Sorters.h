// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef NPP_SORTERS_H
#define NPP_SORTERS_H

#include <algorithm>

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
			return input.substr(_fromColumn, 1 + _toColumn - _fromColumn);
		}
		else
		{
			return input;
		}
	}

	bool isSortingSpecificColumns()
	{
		return _fromColumn != 0 && _toColumn != 0;
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
		// Note that both branches here are equivalent in the sense that they give always give the same answer.
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

// Convert each line to a number and then sort.
// The conversion must be implemented in classes which inherit from this, see prepareStringForConversion and convertStringToNumber.
template<typename T_Num>
class NumericSorter : public ISorter
{
public:
	NumericSorter(bool isDescending, size_t fromColumn, size_t toColumn) : ISorter(isDescending, fromColumn, toColumn)
	{
		_usLocale = ::_wcreate_locale(LC_NUMERIC, TEXT("en-US"));
	};

	~NumericSorter()
	{
		::_free_locale(_usLocale);
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
					nonEmptyInputAsNumbers.push_back(make_pair(lineIndex, convertStringToNumber(preparedLine)));
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

// Converts lines to long long before sorting.
class IntegerSorter : public NumericSorter<long long>
{
public:
	IntegerSorter(bool isDescending, size_t fromColumn, size_t toColumn) : NumericSorter<long long>(isDescending, fromColumn, toColumn) { };

protected:
	virtual generic_string prepareStringForConversion(const generic_string& input)
	{
		return stringTakeWhileAdmissable(getSortKey(input), TEXT(" \t\r\n0123456789-"));
	}

	long long convertStringToNumber(const generic_string& input) override
	{
		return std::stoll(input);
	}
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

#endif //NPP_SORTERS_H
