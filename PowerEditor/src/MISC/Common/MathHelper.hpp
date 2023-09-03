// This file is part of Notepad++ project
// Copyright (C)2023 Klotzi111

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

#include <concepts>
#include <cassert>

class MathHelper
{
public:
	MathHelper() = delete;

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static bool isPowerOf2(T n) noexcept
	{
		return n > 0 && ((n & (n - 1)) == 0);
	}

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static bool isPowerOf2OrZero(T n) noexcept
	{
		return (n & (n - 1)) == 0;
	}

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static T roundUpToPowerOf2(T numToRound, T multiple) noexcept
	{
		assert(isPowerOf2(multiple));
		return numToRound == 0 ? multiple : ((numToRound + multiple - 1) & -multiple);
	}

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static T roundUpToMultiple(T numToRound, T multiple) noexcept
	{
		assert(multiple != 0);
		return numToRound == 0 ? 0 : (numToRound - 1 - (numToRound - 1) % multiple + multiple);
	}

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static T roundDownToPowerOf2(T numToRound, T multiple) noexcept
	{
		assert(isPowerOf2(multiple));
		return numToRound == 0 ? multiple : (numToRound & -multiple);
	}

	template<std::unsigned_integral  T>
	[[nodiscard]]
	static T roundDownToMultiple(T numToRound, T multiple) noexcept
	{
		assert(multiple != 0);
		return numToRound == 0 ? 0 : (numToRound - 1 - (numToRound - 1) % multiple);
	}
};




