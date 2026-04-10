/* Template class DiffCalc for diff generation, wrapping the actual diff algorithm and adding common post-processing
 * Copyright (C) 2025  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

#include <utility>

#include "diff_types.h"

#include "myers_diff.h"
// #include "fast_myers_diff.h"


#ifdef MULTITHREAD

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#endif // MULTITHREAD


/**
 *  \class  DiffCalc
 *  \brief  Compares and makes a differences list between two vectors (elements are template, must have operator==).
 *			Uses the diff algorithm implemented as DiffAlgo class from the included headers above
 */
template <typename Elem, typename UserDataT = void>
class DiffCalc
{
public:
	DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn isCancelled = nullptr);
	DiffCalc(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		IsCancelledFn isCancelled = nullptr);

	// Runs the actual compare and returns the differences
	diff_results<UserDataT> operator()(
			bool doSwapCheck = true, bool doDiffsCombine = false, bool doBoundaryShift = false,
			const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints = {});

	DiffCalc(const DiffCalc&) = delete;
	const DiffCalc& operator=(const DiffCalc&) = delete;

private:
	diff_results<UserDataT> _run_algo(
			const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize, bool doSwapCheck);

	void _combine_diffs(diff_results<UserDataT>& diff);
	void _shift_boundaries(diff_results<UserDataT>& diff);

	const Elem*	_a;
	intptr_t _a_size;
	const Elem*	_b;
	intptr_t _b_size;

	IsCancelledFn _isCancelled;
};


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const std::vector<Elem>& v1, const std::vector<Elem>& v2,
		IsCancelledFn isCancelled) :
	_a(v1.data()), _a_size(v1.size()), _b(v2.data()), _b_size(v2.size()), _isCancelled(isCancelled)
{
}


template <typename Elem, typename UserDataT>
DiffCalc<Elem, UserDataT>::DiffCalc(const Elem v1[], intptr_t v1_size, const Elem v2[], intptr_t v2_size,
		IsCancelledFn isCancelled) :
	_a(v1), _a_size(v1_size), _b(v2), _b_size(v2_size), _isCancelled(isCancelled)
{
}


template <typename Elem, typename UserDataT>
diff_results<UserDataT> DiffCalc<Elem, UserDataT>::_run_algo(
	const Elem* a, intptr_t asize, const Elem* b, intptr_t bsize, bool doSwapCheck)
{
	intptr_t off_s = 0;

	// The diff algorithm assumes we begin with a diff. The following ensures this is true by skipping any matches
	// in the beginning. This also helps to quickly process sequences that match entirely.
	while (off_s < asize && off_s < bsize && a[off_s] == b[off_s])
		++off_s;

	diff_results<UserDataT> diff;

	if (off_s)
		diff._add(diff_type::DIFF_MATCH, 0, off_s);

	if (asize == bsize && off_s == asize)
		return diff;

	intptr_t aend = asize - 1;
	intptr_t bend = bsize - 1;

	asize -= off_s;
	bsize -= off_s;

	intptr_t off_e = 0;

	while (off_e < asize && off_e < bsize && a[aend - off_e] == b[bend - off_e])
		++off_e;

	if (off_e)
	{
		asize -= off_e;
		bsize -= off_e;
	}

	// Compare with swapped sequences as well to see if result is more optimal
	if (doSwapCheck)
	{
		diff_results<UserDataT> swapDiff;

		// Add first matching block before continuing
		if (!diff.empty())
			swapDiff.push_back(diff[0]);

#ifdef MULTITHREAD

		std::thread thr;

		if (asize > 10000 && bsize > 10000 && std::thread::hardware_concurrency() > 1)
		{
			thr = std::thread([&]()
			{
				DiffAlgo<Elem, UserDataT>(b, bsize, a, asize, swapDiff, _isCancelled)(off_s);
			});
		}
		else
		{
			DiffAlgo<Elem, UserDataT>(b, bsize, a, asize, swapDiff, _isCancelled)(off_s);

			if (_isCancelled && _isCancelled())
				return {};
		}

#else // MULTITHREAD

		DiffAlgo<Elem, UserDataT>(b, bsize, a, asize, swapDiff, _isCancelled)(off_s);

		if (_isCancelled && _isCancelled())
			return {};

#endif // MULTITHREAD

		DiffAlgo<Elem, UserDataT>(a, asize, b, bsize, diff, _isCancelled)(off_s);

#ifdef MULTITHREAD

		if (thr.joinable())
			thr.join();

#endif // MULTITHREAD

		if (_isCancelled && _isCancelled())
			return {};

		// Check which result is more optimal
		if (diff.count_replaces() < swapDiff.count_replaces())
		{
			diff = std::move(swapDiff);
			diff._swap_diff1_diff2();
		}
	}
	else
	{
		DiffAlgo<Elem, UserDataT>(a, asize, b, bsize, diff, _isCancelled)(off_s);

		if (_isCancelled && _isCancelled())
			return {};
	}

	if (off_e)
		diff._add(diff_type::DIFF_MATCH, aend - off_e + 1, off_e);

	return diff;
}


template <typename Elem, typename UserDataT>
diff_results<UserDataT> DiffCalc<Elem, UserDataT>::operator()(
	bool doSwapCheck, bool doDiffsCombine, bool doBoundaryShift,
	const std::vector<std::pair<intptr_t, intptr_t>>& syncPoints)
{
	diff_results<UserDataT> diff;

	if (syncPoints.empty())
	{
		diff = _run_algo(_a, _a_size, _b, _b_size, doSwapCheck);
	}
	else
	{
		intptr_t apos = 0;
		intptr_t bpos = 0;

		for (const auto& syncP: syncPoints)
		{
			if (syncP.first  < apos || syncP.first  >= _a_size ||
				syncP.second < bpos || syncP.second >= _b_size)
				break;

			diff._append(
				_run_algo(&_a[apos], syncP.first - apos, &_b[bpos], syncP.second - bpos, doSwapCheck), apos, bpos);

			if (_isCancelled && _isCancelled())
				return {};

			apos = syncP.first;
			bpos = syncP.second;
		}

		diff._append(
			_run_algo(&_a[apos], _a_size - apos, &_b[bpos], _b_size - bpos, doSwapCheck), apos, bpos);
	}

	if (_isCancelled && _isCancelled())
		return {};

	if (doDiffsCombine)
		_combine_diffs(diff);

	if (doBoundaryShift)
		_shift_boundaries(diff);

	return diff;
}


// If a whole matching block is contained at the end of the next diff block shift match down:
// If [] surrounds the marked differences, basically [abc]d[efgd]hi is the same as [abcdefg]dhi
// We combine diffs to make results more compact and clean
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_combine_diffs(diff_results<UserDataT>& diff)
{
	for (intptr_t i = 1; i < static_cast<intptr_t>(diff.size()); ++i)
	{
		if (diff[i].type != diff_type::DIFF_MATCH)
			continue;

		if (i + 1 < static_cast<intptr_t>(diff.size()))
		{
			const Elem*	el	= _b;

			if (diff[i + 1].type == diff_type::DIFF_IN_1)
			{
				// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - diff endings don't match for sure
				if ((i + 2 < static_cast<intptr_t>(diff.size())) && (diff[i + 2].type == diff_type::DIFF_IN_2))
				{
					i += 2;
					continue;
				}

				el	= _a;
			}

			diff_info<UserDataT>& match = diff[i];
			diff_info<UserDataT>* next_diff = &diff[i + 1];

			if (match.len > next_diff->len)
			{
				++i;
				continue;
			}

			intptr_t match_len = match.len;

			intptr_t match_off = next_diff->off - 1;
			intptr_t check_off = next_diff->off + next_diff->len - 1;

			while ((match_len > 0) && (el[match_off] == el[check_off]))
			{
				--match_off;
				--check_off;
				--match_len;
			}

			if (match_len > 0)
			{
				++i;
				continue;
			}

			// The whole match is contained at the end of the next diff -
			// move the match down linking the surrounding diffs and matches

			// Link match to the next matching block
			if (i + 2 < static_cast<intptr_t>(diff.size()))
			{
				diff[i + 2].off -= match.len;
				diff[i + 2].len += match.len;
			}
			// Create new match block at the end
			else
			{
				diff_info<UserDataT> end_match;

				end_match.type = diff_type::DIFF_MATCH;
				end_match.off = match.off + next_diff->len;
				end_match.len = match.len;

				diff.emplace_back(end_match);
			}

			next_diff->off -= match.len;

			diff.erase(diff.begin() + i);

			next_diff = &diff[i];

			intptr_t k = i - 1;

			diff_info<UserDataT>* prev_diff = &diff[k];

			if (next_diff->type != prev_diff->type)
			{
				if ((k > 0) && (diff[k - 1].type == next_diff->type))
					prev_diff = &diff[--k];
			}

			// Merge diffs
			if (next_diff->type == prev_diff->type)
			{
				prev_diff->len += next_diff->len;

				diff.erase(diff.begin() + i);
				--i;
			}
			// Swap diffs to represent block replacement (DIFF_IN_1 followed by DIFF_IN_2)
			else if (next_diff->type == diff_type::DIFF_IN_1)
			{
				std::swap(prev_diff->type, next_diff->type);
				std::swap(prev_diff->off,  next_diff->off);
				std::swap(prev_diff->len,  next_diff->len);
			}

			// Check if previous match is suitable for combining
			if (k > 1)
				i = k - 2;
		}
	}
}


// Algorithm borrowed from WinMerge
// If the Elem after the DIFF_IN_1 is the same as the first Elem of the DIFF_IN_1, shift differences down:
// If [] surrounds the marked differences, basically [abb]a is the same as a[bba]
// Since most languages start with unique elem and end with repetitive elem (end, </node>, }, ], ), >, etc)
// we shift the differences down to make results look cleaner
template <typename Elem, typename UserDataT>
void DiffCalc<Elem, UserDataT>::_shift_boundaries(diff_results<UserDataT>& diff)
{
	for (intptr_t i = 0; i < static_cast<intptr_t>(diff.size()); ++i)
	{
		if (diff[i].type == diff_type::DIFF_MATCH)
			continue;

		if (i + 1 < static_cast<intptr_t>(diff.size()))
		{
			const Elem*	el	= _b;

			if (diff[i].type == diff_type::DIFF_IN_1)
			{
				// If there is DIFF_IN_2 after DIFF_IN_1 both sequences are changed - boundaries do not match for sure
				if (diff[i + 1].type == diff_type::DIFF_IN_2)
				{
					++i;
					continue;
				}

				el	= _a;
			}

			diff_info<UserDataT>& di = diff[i];
			diff_info<UserDataT>* next_match_di = &diff[i + 1];

			const intptr_t max_len = (di.len > next_match_di->len) ? next_match_di->len : di.len;

			intptr_t check_off = di.off + di.len;
			intptr_t shift_len = 0;

			while (shift_len < max_len && el[di.off] == el[check_off])
			{
				++di.off;
				++check_off;
				++shift_len;
			}

			// Diff block shifted - we need to adjust the surrounding matching blocks accordingly
			if (shift_len)
			{
				if (i > 0)
				{
					diff[i - 1].len += shift_len;
				}
				// Create new match block in the beginning
				else
				{
					diff_info<UserDataT> prev_match_di;

					prev_match_di.type = diff_type::DIFF_MATCH;
					prev_match_di.off = 0;
					prev_match_di.len = shift_len;

					diff.insert(diff.begin(), prev_match_di);
					++i;
				}

				next_match_di = &diff[i + 1];

				next_match_di->off += shift_len;
				next_match_di->len -= shift_len;

				// The whole match diff shifted - erase it and merge surrounding diff blocks
				if (next_match_di->len == 0)
				{
					intptr_t j = i + 1;

					diff.erase(diff.begin() + j);

					if (j < static_cast<intptr_t>(diff.size()) && diff[i].type == diff[j].type)
					{
						diff[i].len += diff[j].len;
						diff.erase(diff.begin() + j);

						// Diff blocks merged - recheck same diff
						--i;
					}
				}
			}
		}
	}
}
