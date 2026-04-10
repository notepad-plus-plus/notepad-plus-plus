/* Common diff types used with DiffCalc and DiffAlgo
 * Copyright (C) 2025  Pavel Nedev <pg.nedev@gmail.com>
 */


#pragma once

// Should be defined before including windows.h so this header needs to be included above any windows.h inclusion.
// That cannot be guaranteed but add the define here anyway for completeness
#define NOMINMAX	1


#include <vector>
#include <functional>


using IsCancelledFn = std::function<bool()>;


enum class diff_type
{
	DIFF_MATCH,
	DIFF_IN_1,
	DIFF_IN_2
};


template <typename UserDataT>
struct diff_info
{
	diff_type	type;
	intptr_t	off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
	intptr_t	len;

	UserDataT	info;
};


template <>
struct diff_info<void>
{
	diff_type	type;
	intptr_t	off; // off into 1 if DIFF_MATCH and DIFF_IN_1 but into 2 if DIFF_IN_2
	intptr_t	len;
};


// It is merely a std::vector with some helper functions
template <typename UserDataT = void>
struct diff_results : public std::vector<diff_info<UserDataT>>
{
	intptr_t count_replaces() const
	{
		const intptr_t diffSize = static_cast<intptr_t>(this->size()) - 1;
		intptr_t replaces = 0;

		for (intptr_t i = 0; i < diffSize; ++i)
		{
			if (((*this)[i].type == diff_type::DIFF_IN_1) && ((*this)[i + 1].type == diff_type::DIFF_IN_2))
			{
				replaces += std::min((*this)[i].len, (*this)[i + 1].len);
				++i;
			}
		}

		return replaces;
	};

	inline void _add(diff_type type, intptr_t off, intptr_t len)
	{
		if (len == 0)
			return;

		if (this->empty() || (this->back().type != type))
		{
			diff_info<UserDataT> new_di;

			new_di.type = type;
			new_di.off = off;
			new_di.len = len;

			this->push_back(new_di);
		}
		else
		{
			this->back().len += len;
		}
	};

	void _swap_diff1_diff2()
	{
		intptr_t off2 = 0;
		diff_info<UserDataT>* reorderDiff = nullptr;

		// Swap DIFF_IN_1 and DIFF_IN_2
		for (auto& d : *this)
		{
			if (d.type == diff_type::DIFF_MATCH)
			{
				d.off = off2;
				off2 += d.len;

				reorderDiff = nullptr;
			}
			else if (d.type == diff_type::DIFF_IN_1)
			{
				d.type = diff_type::DIFF_IN_2;

				reorderDiff = &d;
			}
			else
			{
				d.type = diff_type::DIFF_IN_1;
				off2 += d.len;

				if (reorderDiff)
				{
					std::swap(reorderDiff->type, d.type);
					std::swap(reorderDiff->off,  d.off);
					std::swap(reorderDiff->len,  d.len);

					reorderDiff = nullptr;
				}
			}
		}
	};

	void _append(diff_results<UserDataT>&& diff, intptr_t aoff, intptr_t boff)
	{
		if (diff.empty())
			return;

		for (auto& d: diff)
			d.off += (d.type == diff_type::DIFF_IN_2) ? boff : aoff;

		auto dItr = diff.begin();

		if (this->size())
		{
			// Unite border diffs
			if (this->back().type == dItr->type)
			{
				this->back().len += dItr->len;
				dItr++;
			}
		}

		this->insert(this->end(), dItr, diff.end());
	};
};
