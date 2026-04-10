/* Fast Myers Diff algorithm ported to C++ from TypeScript implementation and
 * modified into template class DiffAlgo
 * Copyright (C) 2024-2025  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include "diff_types.h"

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>


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
	static constexpr int _cCancelCheckItrInterval {300000};

	struct DiffState
	{
		intptr_t i;
		intptr_t N;
		intptr_t j;
		intptr_t M;

		intptr_t Z;

		intptr_t pxs;
		intptr_t pxe;
		intptr_t pys;
		intptr_t pye;
		intptr_t oxs;
		intptr_t oxe;
		intptr_t oys;
		intptr_t oye;

		std::vector<intptr_t>& stack;
		std::vector<intptr_t>& buf;
	};

	inline bool _cancel_check();

	inline void _to_diff_blocks(intptr_t& aoff, intptr_t& boff, intptr_t as, intptr_t ae, intptr_t bs, intptr_t be);

	intptr_t _diff_core(intptr_t aoff, intptr_t asize, intptr_t boff, intptr_t bsize);
	int _diff_internal(DiffState& state, int c);

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	diff_results<UserDataT>& _diff;

	IsCancelledFn _isCancelled;
	int _cancelCheckCount;
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
	if (_diff_core(off, _a_size, off, _b_size) == -1)
		_diff.clear();
}


template <typename Elem, typename UserDataT>
inline bool DiffAlgo<Elem, UserDataT>::_cancel_check()
{
	if (!--_cancelCheckCount)
	{
		if (_isCancelled && _isCancelled())
			return true;

		_cancelCheckCount = _cCancelCheckItrInterval;
	}

	return false;
}


template <typename Elem, typename UserDataT>
inline void DiffAlgo<Elem, UserDataT>::_to_diff_blocks(intptr_t& aoff, intptr_t& boff,
	intptr_t as, intptr_t ae, intptr_t bs, intptr_t be)
{
	if (as - aoff > 0)
		_diff._add(diff_type::DIFF_MATCH, aoff, as - aoff);

	if (ae - as > 0)
	{
		_diff._add(diff_type::DIFF_IN_1, as, ae - as);
		aoff = ae;
	}

	if (be - bs > 0)
	{
		_diff._add(diff_type::DIFF_IN_2, bs, be - bs);
		aoff = ae;
		boff = be;
	}
}


template <typename Elem, typename UserDataT>
intptr_t DiffAlgo<Elem, UserDataT>::_diff_core(intptr_t aoff, intptr_t asize, intptr_t boff, intptr_t bsize)
{
	const intptr_t Z = 2 * (std::min(asize, bsize) + 1);

	std::vector<intptr_t> stack;
	std::vector<intptr_t> buf;

	DiffState state {
		aoff, asize, boff, bsize, Z,
		-1, -1, -1, -1,
		-1, -1, -1, -1,
		stack, buf
	};

	for (int c = 0; c < 2;)
	{
		c = _diff_internal(state, c);

		if (c < 0)
			return -1;

		if (c == 1)
		{
			// LOGD(LOG_ALGO, "O -> " + std::to_string(state.oxs) + ", " + std::to_string(state.oxe) + " / " +
			// std::to_string(state.oys) + ", " + std::to_string(state.oye) + "\n");

			_to_diff_blocks(aoff, boff, state.oxs, state.oxe, state.oys, state.oye);

			continue;
		}

		if (state.pxs >= 0)
		{
			// LOGD(LOG_ALGO, "P -> " + std::to_string(state.pxs) + ", " + std::to_string(state.pxe) + " / " +
			// std::to_string(state.pys) + ", " + std::to_string(state.pye) + "\n");

			_to_diff_blocks(aoff, boff, state.pxs, state.pxe, state.pys, state.pye);

			continue;
		}

		break;
	}

	return 0;
}


// Find the list of differences between 2 lists by
// recursive subdivision, requring O(min(N,M)) space
// and O(min(N,M)*D) worst-case execution time where
// D is the number of differences.
template <typename Elem, typename UserDataT>
int DiffAlgo<Elem, UserDataT>::_diff_internal(DiffState& state, int c)
{
	intptr_t i = state.i;
	intptr_t N = state.N;
	intptr_t j = state.j;
	intptr_t M = state.M;
	intptr_t Z = state.Z;

	std::vector<intptr_t>& stack = state.stack;
	std::vector<intptr_t>& buf = state.buf;

	for (;;)
	{
		switch(c)
		{
			case 0:
			{
				Z_block: while (N > 0 && M > 0)
				{
					buf.assign(3 * Z, 0);
					intptr_t* b = buf.data() + Z;

					const intptr_t W = N - M;
					const intptr_t L = N + M;
					const intptr_t parity = L & 1;
					const intptr_t offsetx = i + N - 1;
					const intptr_t offsety = j + M - 1;
					const intptr_t hmax = (L + parity) / 2;

					intptr_t z;

					// h_loop
					for (intptr_t h = 0; h <= hmax; ++h)
					{
						const intptr_t kmin = 2 * std::max((intptr_t)0, h - M) - h;
						const intptr_t kmax = h - 2 * std::max((intptr_t)0, h - N);

						// Forward pass
						for (intptr_t k = kmin; k <= kmax; k += 2)
						{
							if (_cancel_check())
								return -1;

							const intptr_t gkm = b[k - 1 - Z * (intptr_t)std::floor((k - 1)/Z)];
							const intptr_t gkp = b[k + 1 - Z * (intptr_t)std::floor((k + 1)/Z)];
							const intptr_t u = (k == -h || (k != h && gkm < gkp)) ? gkp : gkm + 1;
							const intptr_t v = u - k;

							intptr_t x = u;
							intptr_t y = v;

							while (x < N && y < M && (_a[i + x] == _b[j + y]))
								++x, ++y;

							b[k - Z * (intptr_t)std::floor(k/Z)] = x;

							if (parity == 1 && (z = W - k) >= 1 - h && z < h &&
								x + b[Z + z - Z * (intptr_t)std::floor(z/Z)] >= N)
							{
								if (h > 1 || x != u)
								{
									stack.push_back(i + x);
									stack.push_back(N - x);
									stack.push_back(j + y);
									stack.push_back(M - y);

									N = u;
									M = v;
									Z = 2 * (std::min(N, M) + 1);

									goto Z_block;
								}
								else
								{
									goto h_loop_end;
								}
							}
						}

						// Reverse pass
						for (intptr_t k = kmin; k <= kmax; k += 2)
						{
							if (_cancel_check())
								return -1;

							const intptr_t pkm = b[Z + k - 1 - Z * (intptr_t)std::floor((k - 1)/Z)];
							const intptr_t pkp = b[Z + k + 1 - Z * (intptr_t)std::floor((k + 1)/Z)];
							const intptr_t u = (k == -h || (k != h && pkm < pkp)) ? pkp : pkm + 1;
							const intptr_t v = u - k;

							intptr_t x = u;
							intptr_t y = v;

							while (x < N && y < M && (_a[offsetx - x] == _b[offsety - y]))
								++x, ++y;

							b[Z + k - Z * (intptr_t)std::floor(k/Z)] = x;

							if (parity == 0 && (z = W - k) >= -h && z <= h &&
								x + b[z - Z * (intptr_t)std::floor(z/Z)] >= N)
							{
								if (h > 0 || x != u)
								{
									stack.push_back(i + N - u);
									stack.push_back(u);
									stack.push_back(j + M - v);
									stack.push_back(v);

									N = N - x;
									M = M - y;
									Z = 2 * (std::min(N, M) + 1);

									goto Z_block;
								}
								else
								{
									goto h_loop_end;
								}
							}
						}
					}

					h_loop_end:;

					if (N == M)
						continue;

					if (M > N)
					{
						i += N;
						j += N;
						M -= N;
						N = 0;
					}
					else
					{
						i += M;
						j += M;
						N -= M;
						M = 0;
					}

					// We already know either N or M is zero, so we can
					// skip the extra check at the top of the loop.
					break;
				}

				// yield delete_start, delete_end, insert_start, insert_end
				// At this point, at least one of N & M is zero, or we
				// wouldn't have gotten out of the preceding loop yet.
				if (N + M != 0)
				{
					if (state.pxe == i || state.pye == j)
					{
						// it is a contiguous difference extend the existing one
						state.pxe = i + N;
						state.pye = j + M;
					}
					else
					{
						const intptr_t sx = state.pxs;

						state.oxs = state.pxs;
						state.oxe = state.pxe;
						state.oys = state.pys;
						state.oye = state.pye;

						// Defer this one until we can check the next one
						state.pxs = i;
						state.pxe = i + N;
						state.pys = j;
						state.pye = j + M;

						if (sx >= 0)
						{
							state.i = i;
							state.N = N;
							state.j = j;
							state.M = M;
							state.Z = Z;

							return 1;
						}
					}
				}
			}
			// Intentional fall-through

			case 1:
			{
				if (stack.empty())
					return 2;

				M = stack.back();
				stack.pop_back();
				j = stack.back();
				stack.pop_back();
				N = stack.back();
				stack.pop_back();
				i = stack.back();
				stack.pop_back();

				Z = 2 * (std::min(N, M) + 1);
				c = 0;
			}
		}
	}

	return -1;
}
