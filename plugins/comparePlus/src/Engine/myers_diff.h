/* diff - compute a shortest edit script (SES) given two sequences
 * Copyright (c) 2004 Michael B. Allen <mba2000 ioplex.com>
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/* This algorithm is basically Myers' solution to SES/LCS with
 * the Hirschberg linear space refinement as described in the
 * following publication:
 *
 *   E. Myers, "An O(ND) Difference Algorithm and Its Variations",
 *   Algorithmica 1, 2 (1986), 251-266.
 *   http://www.cs.arizona.edu/people/gene/PAPERS/diff.ps
 *
 * This is the same algorithm used by GNU diff(1).
 */

/* Modified into C++ template class DiffAlgo
 * Copyright (C) 2017-2025  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include "diff_types.h"

#include <cstdint>
#include <cstdlib>
#include <climits>


/**
 *  \class  DiffAlgo
 *  \brief  Compares and makes a differences list between two sequences (elements are template, must have operator==)
 */
template <typename Elem, typename UserDataT = void>
class DiffAlgo
{
public:
	DiffAlgo(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size, diff_results<UserDataT>& diff,
			IsCancelledFn isCancelled = nullptr);

	// Runs the actual compare and fills the differences in diff member.
	// The diff algorithm assumes the sequences begin with a diff so provide here the offset to the first difference.
	void operator()(intptr_t off);

	DiffAlgo(const DiffAlgo&) = delete;
	const DiffAlgo& operator=(const DiffAlgo&) = delete;

private:
	static constexpr int		_cCancelCheckItrInterval {3000};
	static constexpr intptr_t	_cDmax {INTPTR_MAX};

	template <typename T>
	struct varray
	{
	public:
		varray() {};
		~varray() {};

		// Be very careful when using the returned T reference! It may become invalid on consecutive calls to get()
		// because the vector memory might be reallocated!
		inline T& get(size_t i)
		{
			if (_buf.size() <= i)
				_buf.resize(i + 1, { 0 });

			return _buf[i];
		};

		inline std::vector<T>& get()
		{
			return _buf;
		};

	private:
		std::vector<T> _buf;
	};

	struct middle_snake {
		intptr_t x, y, u, v;
	};

	inline intptr_t& _v(intptr_t k, intptr_t r);
	intptr_t _find_middle_snake(intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend, middle_snake& ms);
	intptr_t _ses(intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend);

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	diff_results<UserDataT>& _diff;

	IsCancelledFn _isCancelled;
	int _cancelCheckCount;

	varray<intptr_t> _buf;
};


template <typename Elem, typename UserDataT>
DiffAlgo<Elem, UserDataT>::DiffAlgo(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		diff_results<UserDataT>& diff, IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _diff(diff),
	_isCancelled(isCancelled), _cancelCheckCount(_cCancelCheckItrInterval)
{
}


template <typename Elem, typename UserDataT>
void DiffAlgo<Elem, UserDataT>::operator()(intptr_t off)
{
	if (_ses(off, _a_size, off, _b_size) == -1)
		_diff.clear();

	// Wipe temporal buffer to free memory
	_buf.get().clear();
}


template <typename Elem, typename UserDataT>
inline intptr_t& DiffAlgo<Elem, UserDataT>::_v(intptr_t k, intptr_t r)
{
	// Pack -N to N into 0 to 2 * N
	const intptr_t j = (k <= 0) ? (-k * 4 + r) : (k * 4 + (r - 2));

	return _buf.get(j);
}


template <typename Elem, typename UserDataT>
intptr_t DiffAlgo<Elem, UserDataT>::_find_middle_snake(
	intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend, middle_snake& ms)
{
	const intptr_t delta = aend - bend;
	const intptr_t odd = delta & 1;
	const intptr_t mid = (aend + bend) / 2 + odd;

	_v(1, 0) = 0;
	_v(delta - 1, 1) = aend;

	for (intptr_t d = 0; d <= mid; ++d)
	{
		intptr_t k, x, y;

		if ((2 * d - 1) >= _cDmax)
			return _cDmax;

		if (!--_cancelCheckCount)
		{
			if (_isCancelled && _isCancelled())
				return -1;

			_cancelCheckCount = _cCancelCheckItrInterval;
		}

		for (k = d; k >= -d; k -= 2)
		{
			if (k == -d || (k != d && _v(k - 1, 0) < _v(k + 1, 0)))
				x = _v(k + 1, 0);
			else
				x = _v(k - 1, 0) + 1;

			y = x - k;

			ms.x = x;
			ms.y = y;

			while (x < aend && y < bend &&  _a[aoff + x] == _b[boff + y])
			{
				++x;
				++y;
			}

			_v(k, 0) = x;

			if (odd && k >= (delta - (d - 1)) && k <= (delta + (d - 1)))
			{
				if (x >= _v(k, 1))
				{
					ms.u = x;
					ms.v = y;
					return 2 * d - 1;
				}
			}
		}

		for (k = d; k >= -d; k -= 2)
		{
			intptr_t kr = (aend - bend) + k;

			if (k == d || (k != -d && _v(kr - 1, 1) < _v(kr + 1, 1)))
			{
				x = _v(kr - 1, 1);
			}
			else
			{
				x = _v(kr + 1, 1) - 1;
			}

			y = x - kr;

			ms.u = x;
			ms.v = y;

			while (x > 0 && y > 0 &&  _a[aoff + x - 1] == _b[boff + y - 1])
			{
				--x;
				--y;
			}

			_v(kr, 1) = x;

			if (!odd && kr >= -d && kr <= d)
			{
				if (x <= _v(kr, 0))
				{
					ms.x = x;
					ms.y = y;

					return 2 * d;
				}
			}
		}
	}

	return -1;
}


template <typename Elem, typename UserDataT>
intptr_t DiffAlgo<Elem, UserDataT>::_ses(
	intptr_t aoff, intptr_t aend, intptr_t boff, intptr_t bend)
{
	middle_snake ms = { 0 };
	intptr_t d;

	if (aend == 0)
	{
		_diff._add(diff_type::DIFF_IN_2, boff, bend);
		d = bend;
	}
	else if (bend == 0)
	{
		_diff._add(diff_type::DIFF_IN_1, aoff, aend);
		d = aend;
	}
	else
	{
		// Find the middle "snake" around which we
		// recursively solve the sub-problems.
		d = _find_middle_snake(aoff, aend, boff, bend, ms);
		if (d == -1)
			return -1;

		if (d >= _cDmax)
			return _cDmax;

		if (d > 1)
		{
			if (_ses(aoff, ms.x, boff, ms.y) == -1)
				return -1;

			_diff._add(diff_type::DIFF_MATCH, aoff + ms.x, ms.u - ms.x);

			aoff += ms.u;
			boff += ms.v;
			aend -= ms.u;
			bend -= ms.v;

			if (_ses(aoff, aend, boff, bend) == -1)
				return -1;
		}
		else
		{
			intptr_t x = ms.x;
			intptr_t u = ms.u;

			/* There are only 4 base cases when the
			 * edit distance is 1.
			 *
			 * aend > bend   bend > aend
			 *
			 *   -       |
			 *    \       \    x != u
			 *     \       \
			 *
			 *   \       \
			 *    \       \    x == u
			 *     -       |
			 */

			if (bend > aend)
			{
				if (x == u)
				{
					_diff._add(diff_type::DIFF_MATCH, aoff, aend);
					_diff._add(diff_type::DIFF_IN_2, boff + (bend - 1), 1);
				}
				else
				{
					_diff._add(diff_type::DIFF_IN_2, boff, 1);
					_diff._add(diff_type::DIFF_MATCH, aoff, aend);
				}
			}
			else
			{
				if (x == u)
				{
					_diff._add(diff_type::DIFF_MATCH, aoff, bend);
					_diff._add(diff_type::DIFF_IN_1, aoff + (aend - 1), 1);
				}
				else
				{
					_diff._add(diff_type::DIFF_IN_1, aoff, 1);
					_diff._add(diff_type::DIFF_MATCH, aoff + 1, bend);
				}
			}
		}
	}

	return d;
}
